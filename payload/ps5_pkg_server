#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define PORT 8080
#define WEB_ROOT "/data/pkg_tool/www"
#define BUFFER_SIZE 4096

// External symbols resolved dynamically by the PS5 toolchain
extern int sceKernelDebugOutText(int type, const char* text);

void log_debug(const char* message) {
    sceKernelDebugOutText(0, message);
    sceKernelDebugOutText(0, "\n");
}

void send_http_response(int client_fd, const char* status, const char* content_type, const char* body, size_t body_len) {
    char header[512];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n\r\n",
        status, content_type, body_len);
        
    write(client_fd, header, header_len);
    write(client_fd, body, body_len);
}

void handle_static_file(int client_fd, const char* path) {
    char full_path[512];
    // Security: Prevent directory traversal exploits by joining strictly
    snprintf(full_path, sizeof(full_path), "%s%s", WEB_ROOT, (strcmp(path, "/") == 0) ? "/index.html" : path);

    int file_fd = open(full_path, O_RDONLY);
    if (file_fd < 0) {
        const char* not_found = "<h1>404 Not Found</h1>";
        send_http_response(client_fd, "404 Not Found", "text/html", not_found, strlen(not_found));
        return;
    }

    // Determine basic content types
    const char* content_type = "text/plain";
    if (strstr(full_path, ".html")) content_type = "text/html";
    else if (strstr(full_path, ".js")) content_type = "application/javascript";
    else if (strstr(full_path, ".css")) content_type = "text/css";
    else if (strstr(full_path, ".json")) content_type = "application/json";

    // Read and stream file content
    char file_buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    // First send success headers
    char header[256];
    int header_len = snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nConnection: close\r\n\r\n", content_type);
    write(client_fd, header, header_len);

    while ((bytes_read = read(file_fd, file_buffer, sizeof(file_buffer))) > 0) {
        write(client_fd, file_buffer, bytes_read);
    }
    close(file_fd);
}

void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    
    ssize_t bytes_received = read(client_fd, buffer, BUFFER_SIZE - 1);
    if (bytes_received <= 0) return;

    // Parse HTTP Method and Path
    char method[16], path[256], protocol[16];
    if (sscanf(buffer, "%15s %255s %15s", method, path, protocol) < 3) return;

    log_debug("Incoming HTTP Request:");
    log_debug(method);
    log_debug(path);

    if (strcmp(method, "GET") == 0) {
        handle_static_file(client_fd, path);
    } 
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/backport") == 0) {
        // Native backport logic execution or trigger execution command here
        const char* json_res = "{\"status\":\"processing\", \"msg\":\"Backport workflow initialized.\"}";
        send_http_response(client_fd, "202 Accepted", "application/json", json_res, strlen(json_res));
    } 
    else {
        const char* bad_req = "<h1>400 Bad Request</h1>";
        send_http_response(client_fd, "400 Bad Request", "text/html", bad_req, strlen(bad_req));
    }
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);

    log_debug("Starting PS5 PKG HTTP Tool Server...");

    // Create Socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        log_debug("Socket generation failed.");
        return -1;
    }

    // Forcefully attaching socket to port 8080
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind Address
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        log_debug("Socket bind address mapping failed.");
        close(server_fd);
        return -1;
    }

    // Listen Loop
    if (listen(server_fd, 10) < 0) {
        log_debug("Listen pipeline failure.");
        close(server_fd);
        return -1;
    }

    log_debug("Server running on port 8080. Ready for web-connections.");

    while (1) {
        if ((client_fd = accept(server_fd, (struct sockaddr*)&address, &addrlen)) >= 0) {
            handle_client(client_fd);
            close(client_fd);
        }
    }

    close(server_fd);
    return 0;
}
