#ifndef PKG_SERVER_H
#define PKG_SERVER_H

#include <stdint.h>

/* ── Network configuration ───────────────────────────────────────────── */
#define PKG_SERVER_PORT             4242
#define PKG_SERVER_MAX_CONNECTIONS  10
#define PKG_SERVER_BUFFER_SIZE      65536

/* ── API endpoints ───────────────────────────────────────────────────── */
#define PKG_API_STATUS      "/api/status"
#define PKG_API_CREATE      "/api/pkg/create"
#define PKG_API_PROGRESS    "/api/pkg/progress"
#define PKG_API_DOWNLOAD    "/api/pkg/download"
#define PKG_API_SFO_PARSE   "/api/sfo/parse"   /* ?path=<dump_dir> → {"content_id":"..."} */

/* ── PKG creation status enum ────────────────────────────────────────── */
typedef enum {
    PKG_STATUS_IDLE        = 0,
    PKG_STATUS_SCANNING    = 1,
    PKG_STATUS_BUILDING_PFS = 2,
    PKG_STATUS_BUILDING_PKG = 3,
    PKG_STATUS_COMPLETE    = 4,
    PKG_STATUS_ERROR       = 5
} pkg_creation_status_t;

/* ── Progress state shared between server and worker thread ─────────── */
typedef struct {
    pkg_creation_status_t status;
    char     current_file[256];
    uint64_t files_processed;
    uint64_t total_files;
    uint64_t bytes_processed;
    uint64_t total_bytes;
    char     output_path[1024];
    char     error_message[512];
    int      progress_percent;
    char     status_text[128];
} pkg_progress_t;

/* ── Server lifecycle ────────────────────────────────────────────────── */
int  pkg_server_init(void);
int  pkg_server_run(void);    /* blocks until stopped */
void pkg_server_stop(void);

/* ── Request handlers (called internally by the HTTP dispatch loop) ─── */
void handle_status(int client_fd);
void handle_create(int client_fd, const char *path, const char *content_id);
void handle_progress(int client_fd);
void handle_download(int client_fd, const char *path);
void handle_sfo_parse(int client_fd, const char *dump_path);
void handle_static(int client_fd, const char *url_path);

/* ── Background PKG worker ───────────────────────────────────────────── */
void *pkg_creation_thread(void *arg);

#endif /* PKG_SERVER_H */
