#ifndef PKG_SERVER_H
#define PKG_SERVER_H

#include <stdint.h>

// HTTP server configuration
#define PKG_SERVER_PORT 8080
#define PKG_SERVER_MAX_CONNECTIONS 10
#define PKG_SERVER_BUFFER_SIZE 65536

// API endpoints
#define PKG_API_STATUS      "/api/status"
#define PKG_API_CREATE      "/api/pkg/create"
#define PKG_API_PROGRESS    "/api/pkg/progress"
#define PKG_API_DOWNLOAD    "/api/pkg/download"

// PKG creation status
typedef enum {
    PKG_STATUS_IDLE = 0,
    PKG_STATUS_SCANNING,
    PKG_STATUS_BUILDING_PFS,
    PKG_STATUS_BUILDING_PKG,
    PKG_STATUS_COMPLETE,
    PKG_STATUS_ERROR
} pkg_creation_status_t;

typedef struct {
    pkg_creation_status_t status;
    char current_file[256];
    uint64_t files_processed;
    uint64_t total_files;
    uint64_t bytes_processed;
    uint64_t total_bytes;
    char output_path[1024];
    char error_message[512];
    int progress_percent;
} pkg_progress_t;

// Server functions
int pkg_server_init(void);
int pkg_server_run(void);
void pkg_server_stop(void);

// API handlers
void handle_status(int client_fd);
void handle_create(int client_fd, const char* path, const char* content_id);
void handle_progress(int client_fd);
void handle_download(int client_fd, const char* path);

// Background task
void* pkg_creation_thread(void* arg);

#endif
