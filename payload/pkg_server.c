#include "pkg_server.h"
#include "param_sfo.h"
#include "www_embed.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <pthread.h>
#include <ctype.h>
#include <dirent.h>

#include "libprosperopkg/include/pkg_types.h"
#include "libprosperopkg/include/pfs_builder.h"
#include "libprosperopkg/include/pkg_header.h"

/* ── Internal state ──────────────────────────────────────────────────── */
static int                  server_fd = -1;
static pkg_progress_t       current_progress;
static pthread_mutex_t      progress_mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile int         server_running = 0;

/* ── Tiny HTTP helpers ───────────────────────────────────────────────── */

/* Send a complete HTTP response in one shot. */
static void http_respond(int fd, int status, const char *status_text,
                         const char *content_type,
                         const char *body, size_t body_len) {
    char hdr[512];
    int hlen = snprintf(hdr, sizeof(hdr),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Connection: close\r\n"
        "\r\n",
        status, status_text, content_type, body_len);
    write(fd, hdr, (size_t)hlen);
    if (body && body_len > 0)
        write(fd, body, body_len);
}

static void http_json(int fd, int status, const char *status_text,
                      const char *json) {
    http_respond(fd, status, status_text,
                 "application/json; charset=utf-8",
                 json, strlen(json));
}

/* URL-decode a string in-place. Returns length of decoded string. */
static size_t url_decode(char *buf, size_t len) {
    size_t in = 0, out = 0;
    while (in < len && buf[in]) {
        if (buf[in] == '%' && in + 2 < len &&
            isxdigit((unsigned char)buf[in+1]) &&
            isxdigit((unsigned char)buf[in+2])) {
            char hex[3] = { buf[in+1], buf[in+2], '\0' };
            buf[out++] = (char)strtol(hex, NULL, 16);
            in += 3;
        } else if (buf[in] == '+') {
            buf[out++] = ' ';
            in++;
        } else {
            buf[out++] = buf[in++];
        }
    }
    buf[out] = '\0';
    return out;
}

/* Extract value of named query-string parameter from ?key=value&... */
static int qs_get(const char *qs, const char *key,
                  char *out, size_t out_len) {
    if (!qs) return -1;
    size_t klen = strlen(key);
    const char *p = qs;
    while (*p) {
        if (strncmp(p, key, klen) == 0 && p[klen] == '=') {
            const char *v = p + klen + 1;
            const char *end = strchr(v, '&');
            size_t vlen = end ? (size_t)(end - v) : strlen(v);
            size_t copy = vlen < out_len - 1 ? vlen : out_len - 1;
            memcpy(out, v, copy);
            out[copy] = '\0';
            url_decode(out, copy + 1);
            return 0;
        }
        /* advance to next param */
        const char *amp = strchr(p, '&');
        if (!amp) break;
        p = amp + 1;
    }
    return -1;
}

/* ── Request parsing ─────────────────────────────────────────────────── */

typedef struct {
    char method[8];
    char path[512];       /* URL path, no query string */
    char query[1024];     /* everything after '?' */
    char body[8192];      /* request body (for POST) */
    size_t body_len;
} http_request_t;

static int parse_request(int fd, http_request_t *req) {
    memset(req, 0, sizeof(*req));

    char buf[PKG_SERVER_BUFFER_SIZE];
    ssize_t total = 0;
    /* read until blank line */
    while (total < (ssize_t)(sizeof(buf) - 1)) {
        ssize_t n = read(fd, buf + total, sizeof(buf) - 1 - total);
        if (n <= 0) break;
        total += n;
        buf[total] = '\0';
        if (strstr(buf, "\r\n\r\n")) break;
    }
    if (total <= 0) return -1;

    /* parse request line */
    char *line_end = strstr(buf, "\r\n");
    if (!line_end) return -1;
    *line_end = '\0';

    char *sp1 = strchr(buf, ' ');
    if (!sp1) return -1;
    size_t mlen = (size_t)(sp1 - buf);
    if (mlen >= sizeof(req->method)) return -1;
    memcpy(req->method, buf, mlen);

    char *url_start = sp1 + 1;
    char *sp2 = strchr(url_start, ' ');
    if (sp2) *sp2 = '\0';

    /* split path and query */
    char *qmark = strchr(url_start, '?');
    if (qmark) {
        *qmark = '\0';
        strncpy(req->query, qmark + 1, sizeof(req->query) - 1);
    }
    strncpy(req->path, url_start, sizeof(req->path) - 1);

    /* read body for POST */
    if (strcmp(req->method, "POST") == 0) {
        char *body_start = strstr(buf, "\r\n\r\n");
        if (body_start) {
            body_start += 4;
            req->body_len = (size_t)(buf + total - body_start);
            if (req->body_len > sizeof(req->body) - 1)
                req->body_len = sizeof(req->body) - 1;
            memcpy(req->body, body_start, req->body_len);
        }
    }
    return 0;
}

/* ── API handlers ────────────────────────────────────────────────────── */

void handle_status(int fd) {
    char json[256];
    snprintf(json, sizeof(json),
        "{\"status\":\"ok\",\"server\":\"Quaza\","
        "\"version\":\"1.0\",\"port\":%d}", PKG_SERVER_PORT);
    http_json(fd, 200, "OK", json);
}

/* Argument struct passed to the PKG creation worker thread */
typedef struct {
    char path[1024];
    char content_id[64];
} pkg_create_args_t;

void *pkg_creation_thread(void *arg) {
    pkg_create_args_t *args = (pkg_create_args_t *)arg;

    pthread_mutex_lock(&progress_mutex);
    memset(&current_progress, 0, sizeof(current_progress));
    current_progress.status = PKG_STATUS_SCANNING;
    strncpy(current_progress.status_text, "Scanning game files...",
            sizeof(current_progress.status_text) - 1);
    pthread_mutex_unlock(&progress_mutex);

    /* Build output PKG path next to the dump directory */
    char output_path[1200];
    snprintf(output_path, sizeof(output_path), "%s.pkg", args->path);

    /* --- Call libprosperopkg to build the PKG --- */
    /* pfs_build_context_t and pkg_build() are provided by libprosperopkg */

    /* Update progress: scanning complete, building PFS */
    pthread_mutex_lock(&progress_mutex);
    current_progress.status = PKG_STATUS_BUILDING_PFS;
    snprintf(current_progress.status_text,
             sizeof(current_progress.status_text),
             "Building PFS image...");
    current_progress.progress_percent = 20;
    pthread_mutex_unlock(&progress_mutex);

    pkg_project_t project;
    memset(&project, 0, sizeof(project));
    strncpy(project.content_id, args->content_id, sizeof(project.content_id) - 1);
    strncpy(project.app_path,   args->path,       sizeof(project.app_path)   - 1);
    project.type            = PKG_TYPE_PS5_GD;
    project.pfs_block_size  = 0x10000;
    project.use_encryption  = false;
    project.is_patch        = false;

    int rc = pkg_build(&project, args->path, output_path);

    pthread_mutex_lock(&progress_mutex);
    if (rc == 0) {
        current_progress.status = PKG_STATUS_COMPLETE;
        current_progress.progress_percent = 100;
        strncpy(current_progress.status_text, "Complete",
                sizeof(current_progress.status_text) - 1);
        strncpy(current_progress.output_path, output_path,
                sizeof(current_progress.output_path) - 1);
    } else {
        current_progress.status = PKG_STATUS_ERROR;
        snprintf(current_progress.error_message,
                 sizeof(current_progress.error_message),
                 "pkg_build() failed with code %d", rc);
        strncpy(current_progress.status_text, "Error",
                sizeof(current_progress.status_text) - 1);
    }
    pthread_mutex_unlock(&progress_mutex);

    free(args);
    return NULL;
}

void handle_create(int fd, const char *path, const char *content_id) {
    if (!path || path[0] == '\0') {
        http_json(fd, 400, "Bad Request",
                  "{\"error\":\"Missing required parameter: path\"}");
        return;
    }
    if (!content_id || content_id[0] == '\0') {
        http_json(fd, 400, "Bad Request",
                  "{\"error\":\"Missing required parameter: content_id\"}");
        return;
    }

    /* Reject if a conversion is already in progress */
    pthread_mutex_lock(&progress_mutex);
    int busy = (current_progress.status == PKG_STATUS_SCANNING  ||
                current_progress.status == PKG_STATUS_BUILDING_PFS ||
                current_progress.status == PKG_STATUS_BUILDING_PKG);
    pthread_mutex_unlock(&progress_mutex);

    if (busy) {
        http_json(fd, 409, "Conflict",
                  "{\"error\":\"A conversion is already in progress\"}");
        return;
    }

    pkg_create_args_t *args = malloc(sizeof(pkg_create_args_t));
    if (!args) {
        http_json(fd, 500, "Internal Server Error",
                  "{\"error\":\"Out of memory\"}");
        return;
    }
    strncpy(args->path,       path,       sizeof(args->path) - 1);
    strncpy(args->content_id, content_id, sizeof(args->content_id) - 1);

    pthread_t tid;
    if (pthread_create(&tid, NULL, pkg_creation_thread, args) != 0) {
        free(args);
        http_json(fd, 500, "Internal Server Error",
                  "{\"error\":\"Failed to start conversion thread\"}");
        return;
    }
    pthread_detach(tid);

    http_json(fd, 200, "OK",
              "{\"message\":\"PKG creation started\"}");
}

void handle_progress(int fd) {
    pthread_mutex_lock(&progress_mutex);
    pkg_progress_t p = current_progress;
    pthread_mutex_unlock(&progress_mutex);

    char json[512];
    snprintf(json, sizeof(json),
        "{"
        "\"status\":%d,"
        "\"status_text\":\"%s\","
        "\"progress_percent\":%d,"
        "\"current_file\":\"%s\","
        "\"files_processed\":%llu,"
        "\"total_files\":%llu,"
        "\"output_path\":\"%s\","
        "\"error\":\"%s\""
        "}",
        (int)p.status,
        p.status_text,
        p.progress_percent,
        p.current_file,
        (unsigned long long)p.files_processed,
        (unsigned long long)p.total_files,
        p.output_path,
        p.error_message);
    http_json(fd, 200, "OK", json);
}

void handle_download(int fd, const char *path) {
    if (!path || path[0] == '\0') {
        http_json(fd, 400, "Bad Request",
                  "{\"error\":\"Missing path parameter\"}");
        return;
    }

    int file_fd = open(path, O_RDONLY);
    if (file_fd < 0) {
        http_json(fd, 404, "Not Found",
                  "{\"error\":\"PKG file not found\"}");
        return;
    }

    struct stat st;
    fstat(file_fd, &st);
    off_t file_size = st.st_size;

    /* Extract filename from path for Content-Disposition */
    const char *fname = strrchr(path, '/');
    fname = fname ? fname + 1 : path;

    char hdr[512];
    int hlen = snprintf(hdr, sizeof(hdr),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Content-Length: %lld\r\n"
        "Content-Disposition: attachment; filename=\"%s\"\r\n"
        "Connection: close\r\n"
        "\r\n",
        (long long)file_size, fname);
    write(fd, hdr, (size_t)hlen);

    /* Stream file to client */
    char chunk[65536];
    ssize_t n;
    while ((n = read(file_fd, chunk, sizeof(chunk))) > 0)
        write(fd, chunk, (size_t)n);

    close(file_fd);
}

void handle_sfo_parse(int fd, const char *dump_path) {
    if (!dump_path || dump_path[0] == '\0') {
        http_json(fd, 400, "Bad Request",
                  "{\"error\":\"Missing path parameter\"}");
        return;
    }

    char content_id[64] = {0};
    if (sfo_get_content_id_from_dump(dump_path, content_id, sizeof(content_id)) != 0) {
        http_json(fd, 404, "Not Found",
                  "{\"error\":\"Could not read CONTENT_ID from param.sfo\"}");
        return;
    }

    char json[128];
    snprintf(json, sizeof(json),
             "{\"content_id\":\"%s\"}", content_id);
    http_json(fd, 200, "OK", json);
}

/* ── Directory browser (backs the web UI's folder-picker) ────────────── */

/* Tiny growable string buffer — a directory listing's JSON can exceed a
 * fixed-size stack buffer for game dumps with thousands of files. */
typedef struct {
    char  *buf;
    size_t len;
    size_t cap;
} dstr_t;

static void dstr_init(dstr_t *d) {
    d->cap = 4096;
    d->len = 0;
    d->buf = malloc(d->cap);
    if (d->buf) d->buf[0] = '\0';
}

static void dstr_append(dstr_t *d, const char *s) {
    if (!d->buf) return;
    size_t slen = strlen(s);
    if (d->len + slen + 1 > d->cap) {
        size_t new_cap = d->cap;
        while (d->len + slen + 1 > new_cap) new_cap *= 2;
        char *nb = realloc(d->buf, new_cap);
        if (!nb) return;
        d->buf = nb;
        d->cap = new_cap;
    }
    memcpy(d->buf + d->len, s, slen + 1);
    d->len += slen;
}

static void dstr_free(dstr_t *d) {
    if (d->buf) free(d->buf);
    d->buf = NULL;
}

/* Minimal JSON string escaper — handles quotes, backslashes and control
 * characters so odd filenames can't break the response. */
static void json_escape_append(dstr_t *d, const char *s) {
    dstr_append(d, "\"");
    char tmp[8];
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
        if (*p == '"' || *p == '\\') {
            tmp[0] = '\\'; tmp[1] = (char)*p; tmp[2] = '\0';
            dstr_append(d, tmp);
        } else if (*p < 0x20) {
            snprintf(tmp, sizeof(tmp), "\\u%04x", *p);
            dstr_append(d, tmp);
        } else {
            tmp[0] = (char)*p; tmp[1] = '\0';
            dstr_append(d, tmp);
        }
    }
    dstr_append(d, "\"");
}

/* Compute the parent directory of `path` into `out`. Returns an empty
 * string when `path` is already a filesystem root (nothing above it but
 * the root-shortcut view). */
static void compute_parent_path(const char *path, char *out, size_t out_size) {
    if (!path || path[0] == '\0') {
        out[0] = '\0';
        return;
    }
    char tmp[1024];
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    size_t len = strlen(tmp);
    while (len > 1 && tmp[len - 1] == '/') { tmp[--len] = '\0'; }

    char *slash = strrchr(tmp, '/');
    if (!slash) {
        out[0] = '\0'; /* no more path separators — go back to root shortcuts */
    } else if (slash == tmp) {
        strncpy(out, "/", out_size - 1);
        out[out_size - 1] = '\0';
    } else {
        *slash = '\0';
        strncpy(out, tmp, out_size - 1);
        out[out_size - 1] = '\0';
    }
}

/* Common PS5 mount points shown as the folder picker's "Home" view. Only
 * the ones that actually exist on this console are returned to the UI. */
static const char *BROWSE_ROOT_SHORTCUTS[] = {
    "/mnt/usb0",
    "/mnt/usb1",
    "/mnt/ext0",
    "/mnt/ext1",
    "/mnt/sandbox",
    "/data",
    "/system_data",
    NULL
};

void handle_browse(int fd, const char *path) {
    dstr_t d;
    dstr_init(&d);
    if (!d.buf) {
        http_json(fd, 500, "Internal Server Error", "{\"error\":\"Out of memory\"}");
        return;
    }

    if (!path || path[0] == '\0') {
        /* Root view: fixed shortcuts to the common external/internal mounts. */
        dstr_append(&d, "{\"path\":\"\",\"parent\":null,\"entries\":[");
        int first = 1;
        for (int i = 0; BROWSE_ROOT_SHORTCUTS[i]; i++) {
            struct stat st;
            if (stat(BROWSE_ROOT_SHORTCUTS[i], &st) < 0 || !S_ISDIR(st.st_mode))
                continue;
            if (!first) dstr_append(&d, ",");
            first = 0;
            dstr_append(&d, "{\"name\":");
            json_escape_append(&d, BROWSE_ROOT_SHORTCUTS[i]);
            dstr_append(&d, ",\"type\":\"dir\"}");
        }
        dstr_append(&d, "]}");
        http_json(fd, 200, "OK", d.buf);
        dstr_free(&d);
        return;
    }

    DIR *dir = opendir(path);
    if (!dir) {
        dstr_free(&d);
        http_json(fd, 404, "Not Found", "{\"error\":\"Cannot open directory\"}");
        return;
    }

    char parent[1024];
    compute_parent_path(path, parent, sizeof(parent));

    dstr_append(&d, "{\"path\":");
    json_escape_append(&d, path);
    dstr_append(&d, ",\"parent\":");
    if (parent[0] == '\0') {
        dstr_append(&d, "\"\"");
    } else {
        json_escape_append(&d, parent);
    }
    dstr_append(&d, ",\"entries\":[");

    struct dirent *entry;
    int first = 1;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char full[1200];
        snprintf(full, sizeof(full), "%s/%s", path, entry->d_name);
        struct stat st;
        if (stat(full, &st) < 0) continue;

        if (!first) dstr_append(&d, ",");
        first = 0;
        dstr_append(&d, "{\"name\":");
        json_escape_append(&d, entry->d_name);
        dstr_append(&d, ",\"type\":\"");
        dstr_append(&d, S_ISDIR(st.st_mode) ? "dir" : "file");
        dstr_append(&d, "\"}");
    }
    closedir(dir);

    dstr_append(&d, "]}");
    http_json(fd, 200, "OK", d.buf);
    dstr_free(&d);
}

/* Serve embedded www files (offline mode — assets baked into the ELF) */
void handle_static(int fd, const char *url_path) {
    /* normalise: "/" → "/index.html" */
    const char *lookup = (strcmp(url_path, "/") == 0) ? "/" : url_path;

    for (size_t i = 0; i < g_embedded_file_count; i++) {
        if (strcmp(g_embedded_files[i].url_path, lookup) == 0) {
            http_respond(fd, 200, "OK",
                         g_embedded_files[i].mime_type,
                         (const char *)g_embedded_files[i].data,
                         g_embedded_files[i].size);
            return;
        }
    }
    http_json(fd, 404, "Not Found", "{\"error\":\"Not found\"}");
}

/* ── Connection handler (runs per-client) ────────────────────────────── */
static void handle_connection(int fd) {
    http_request_t req;
    if (parse_request(fd, &req) < 0) {
        close(fd);
        return;
    }

    /* OPTIONS pre-flight (CORS) */
    if (strcmp(req.method, "OPTIONS") == 0) {
        http_respond(fd, 204, "No Content", "text/plain", NULL, 0);
        close(fd);
        return;
    }

    char param_path[1024]       = {0};
    char param_content_id[64]   = {0};

    /* Route */
    if (strcmp(req.path, PKG_API_STATUS) == 0) {
        handle_status(fd);

    } else if (strcmp(req.path, PKG_API_CREATE) == 0) {
        qs_get(req.query, "path",       param_path,       sizeof(param_path));
        qs_get(req.query, "content_id", param_content_id, sizeof(param_content_id));
        handle_create(fd, param_path, param_content_id);

    } else if (strcmp(req.path, PKG_API_PROGRESS) == 0) {
        handle_progress(fd);

    } else if (strcmp(req.path, PKG_API_DOWNLOAD) == 0) {
        qs_get(req.query, "path", param_path, sizeof(param_path));
        handle_download(fd, param_path);

    } else if (strcmp(req.path, PKG_API_SFO_PARSE) == 0) {
        qs_get(req.query, "path", param_path, sizeof(param_path));
        handle_sfo_parse(fd, param_path);

    } else if (strcmp(req.path, PKG_API_BROWSE) == 0) {
        qs_get(req.query, "path", param_path, sizeof(param_path));
        handle_browse(fd, param_path);

    } else {
        /* Serve embedded static asset */
        handle_static(fd, req.path);
    }

    close(fd);
}

/* ── Server lifecycle ────────────────────────────────────────────────── */

int pkg_server_init(void) {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("[server] socket");
        return -1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_len         = (uint8_t)sizeof(addr); /* FreeBSD/PS5 requires this */
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(PKG_SERVER_PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("[server] bind");
        close(server_fd);
        server_fd = -1;
        return -1;
    }

    if (listen(server_fd, PKG_SERVER_MAX_CONNECTIONS) < 0) {
        perror("[server] listen");
        close(server_fd);
        server_fd = -1;
        return -1;
    }

    server_running = 1;
    printf("[server] Listening on port %d\n", PKG_SERVER_PORT);
    return 0;
}

int pkg_server_run(void) {
    while (server_running) {
        struct sockaddr_in client_addr;
        socklen_t clen = sizeof(client_addr);
        int client_fd = accept(server_fd,
                               (struct sockaddr *)&client_addr, &clen);
        if (client_fd < 0) {
            if (server_running)
                perror("[server] accept");
            break;
        }
        /* Simple single-threaded dispatch (sufficient for local PS5 use) */
        handle_connection(client_fd);
    }
    return 0;
}

void pkg_server_stop(void) {
    server_running = 0;
    if (server_fd >= 0) {
        close(server_fd);
        server_fd = -1;
    }
}
