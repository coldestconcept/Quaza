#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include "pkg_server.h"
#include "libprosperopkg/include/pkg_types.h"
#include "libprosperopkg/include/pfs_builder.h"
#include "libprosperopkg/include/pkg_header.h"

static int server_fd = -1;
static pkg_progress_t current_progress;
static pthread_mutex_t progress_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t creation_thread;

// URL decode helper
void url_decode(char* dst, const char* src, size_t dst_size) {
    char a, b;
    size_t i = 0;
    
    while (*src && i < dst_size - 1) {
        if ((*src == '%') &&
            ((a = src[1]) && (b = src[2])) &&
            (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a') a -= 'a' - 'A';
            if (a >= 'A') a -= ('A' - 10);
            else a -= '0';
            if (b >= 'a') b -= 'a' - 'A';
            if (b >= 'A') b -= ('A' - 10);
            else b -= '0';
            dst[i++] = 16 * a + b;
            src += 3;
        } else if (*src == '+') {
            dst[i++] = ' ';
            src++;
        } else {
            dst[i++] = *src++;
        }
    }
    dst[i] = '\0';
}

// Send HTTP response
void send_response(int client_fd, int status, const char* content_type,
                   const char* body) {
    char response[4096];
    const char* status_text = (status == 200) ? "OK" : 
                              (status == 404) ? "Not Found" : "Error";
    
    snprintf(response, sizeof(response),
             "HTTP/1.1 %d %s\r\n"
             "Content-Type: %s\r\n"
             "Access-Control-Allow-Origin: *\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             status, status_text, content_type, strlen(body), body);
    
    send(client_fd, response, strlen(response), 0);
}

void send_json_response(int client_fd, int status, const char* json) {
    send_response(client_fd, status, "application/json", json);
}

// API: Get status
void handle_status(int client_fd) {
    const char* json = "{\"status\":\"ok\",\"server\":\"Quaza PKG Server\"}";
    send_json_response(client_fd, 200, json);
}

// API: Create PKG
void handle_create(int client_fd, const char* path, const char* content_id) {
    pthread_mutex_lock(&progress_mutex);
    
    if (current_progress.status != PKG_STATUS_IDLE && 
        current_progress.status != PKG_STATUS_COMPLETE &&
        current_progress.status != PKG_STATUS_ERROR) {
        pthread_mutex_unlock(&progress_mutex);
        send_json_response(client_fd, 409, 
            "{\"error\":\"PKG creation already in progress\"}");
        return;
    }
    
    // Reset progress
    memset(&current_progress, 0, sizeof(current_progress));
    current_progress.status = PKG_STATUS_SCANNING;
    strncpy(current_progress.output_path, "/data/temp.pkg", 1024);
    
    pthread_mutex_unlock(&progress_mutex);
    
    // Start creation thread
    char* args = malloc(strlen(path) + strlen(content_id) + 2);
    sprintf(args, "%s|%s", path, content_id);
    pthread_create(&creation_thread, NULL, pkg_creation_thread, args);
    
    send_json_response(client_fd, 200, 
        "{\"status\":\"started\",\"message\":\"PKG creation started\"}");
}

// API: Get progress
void handle_progress(int client_fd) {
    pthread_mutex_lock(&progress_mutex);
    
    char json[2048];
    snprintf(json, sizeof(json),
        "{"
        "\"status\":%d,"
        "\"status_text\":\"%s\","
        "\"current_file\":\"%s\","
        "\"files_processed\":%lu,"
        "\"total_files\":%lu,"
        "\"bytes_processed\":%lu,"
        "\"total_bytes\":%lu,"
        "\"progress_percent\":%d,"
        "\"output_path\":\"%s\","
        "\"error\":\"%s\""
        "}",
        current_progress.status,
        current_progress.status == PKG_STATUS_IDLE ? "idle" :
        current_progress.status == PKG_STATUS_SCANNING ? "scanning" :
        current_progress.status == PKG_STATUS_BUILDING_PFS ? "building_pfs" :
        current_progress.status == PKG_STATUS_BUILDING_PKG ? "building_pkg" :
        current_progress.status == PKG_STATUS_COMPLETE ? "complete" : "error",
        current_progress.current_file,
        current_progress.files_processed,
        current_progress.total_files,
        current_progress.bytes_processed,
        current_progress.total_bytes,
        current_progress.progress_percent,
        current_progress.output_path,
        current_progress.error_message);
    
    pthread_mutex_unlock(&progress_mutex);
    
    send_json_response(client_fd, 200, json);
}

// Background thread for PKG creation
void* pkg_creation_thread(void* arg) {
    char* args = (char*)arg;
    char path[1024], content_id[37];
    
    // Parse arguments
    char* sep = strchr(args, '|');
    if (sep) {
        *sep = '\0';
        strcpy(path, args);
        strcpy(content_id, sep + 1);
    }
    free(args);
    
    // Create project
    pkg_project_t project = {0};
    strncpy(project.content_id, content_id, 36);
    project.content_id[36] = '\0';
    strcpy(project.passcode, "00000000000000000000000000000000");
    project.type = PKG_TYPE_PS5_GD;
    project.pfs_block_size = PFS_BLOCK_SIZE;
    
    // Build PKG
    int result = pkg_build(&project, path, current_progress.output_path);
    
    pthread_mutex_lock(&progress_mutex);
    if (result == PKG_SUCCESS) {
        current_progress.status = PKG_STATUS_COMPLETE;
        current_progress.progress_percent = 100;
    } else {
        current_progress.status = PKG_STATUS_ERROR;
        strncpy(current_progress.error_message, "PKG build failed", 512);
    }
    pthread_mutex_unlock(&progress_mutex);
    
    return NULL;
}

// Parse HTTP request
void handle_request(int client_fd, char* request) {
    char method[16], path[1024], version[16];
    
    // Parse request line
    sscanf(request, "%15s %1023s %15s", method, path, version);
    
    // Route request
    if (strcmp(path, PKG_API_STATUS) == 0) {
        handle_status(client_fd);
    } else if (strncmp(path, PKG_API_CREATE, strlen(PKG_API_CREATE)) == 0) {
        // Parse query parameters
        char* query = strchr(path, '?');
        char dump_path[1024] = "";
        char content_id[37] = "XXXXXX-XXXX00000_00-XXXXXXXXXX000000";
        
        if (query) {
            *query = '\0';
            query++;
            
            // Parse parameters
            char* param = strtok(query, "&");
            while (param) {
                char* eq = strchr(param, '=');
                if (eq) {
                    *eq = '\0';
                    char* key = param;
                    char* value = eq + 1;
                    
                    if (strcmp(key, "path") == 0) {
                        url_decode(dump_path, value, sizeof(dump_path));
                    } else if (strcmp(key, "content_id") == 0) {
                        url_decode(content_id, value, sizeof(content_id));
                    }
                }
                param = strtok(NULL, "&");
            }
        }
        
        handle_create(client_fd, dump_path, content_id);
    } else if (strcmp(path, PKG_API_PROGRESS) == 0) {
        handle_progress(client_fd);
    } else {
        send_json_response(client_fd, 404, "{\"error\":\"Not found\"}");
    }
}

int pkg_server_init(void) {
    struct sockaddr_in addr;
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return -1;
    }
    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PKG_SERVER_PORT);
    
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return -1;
    }
    
    if (listen(server_fd, PKG_SERVER_MAX_CONNECTIONS) < 0) {
        perror("listen");
        close(server_fd);
        return -1;
    }
    
    printf("[PKG Server] Listening on port %d\n", PKG_SERVER_PORT);
    return 0;
}

int pkg_server_run(void) {
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }
        
        // Read request
        char buffer[PKG_SERVER_BUFFER_SIZE];
        int n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (n > 0) {
            buffer[n] = '\0';
            handle_request(client_fd, buffer);
        }
        
        close(client_fd);
    }
    
    return 0;
}

void pkg_server_stop(void) {
    if (server_fd >= 0) {
        close(server_fd);
        server_fd = -1;
    }
}
