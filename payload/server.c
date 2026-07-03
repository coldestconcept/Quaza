#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define BUFFER_SIZE 8192

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    
    int read_bytes = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (read_bytes < 0) {
        close(client_socket);
        return;
    }

    // Basic HTTP Request parsing
    char method[16], path[256];
    sscanf(buffer, "%s %s", method, path);

    // Routing Logic
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/convert") == 0) {
        // Handle PKG conversion trigger
        char response[] = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: application/json\r\n"
                          "Access-Control-Allow-Origin: *\r\n\r\n"
                          "{\"status\":\"processing\", \"message\":\"PKG generation started natively.\"}";
        send(client_socket, response, strlen(response), 0);
        
        // Execute the LibProsperoPKG / ShadowMount processing asynchronously
        // trigger_pkg_conversion();
        
    } else if (strcmp(method, "GET") == 0 && strcmp(path, "/api/status") == 0) {
        char response[] = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: application/json\r\n"
                          "Access-Control-Allow-Origin: *\r\n\r\n"
                          "{\"status\":\"idle\", \"progress\": 0}";
        send(client_socket, response, strlen(response), 0);
    } else {
        // Fallback or 404 for unknown endpoints
        char response[] = "HTTP/1.1 404 Not Found\r\n"
                          "Content-Type: text/plain\r\n\r\n"
                          "Endpoint Not Found";
        send(client_socket, response, strlen(response), 0);
    }

    close(client_socket);
}

int start_server() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        return -1;
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        return -1;
    }
    
    if (listen(server_fd, 10) < 0) {
        return -1;
    }

    while (1) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) >= 0) {
            handle_client(client_socket);
        }
        usleep(10000); // Prevent CPU hammering on the main loop thread
    }
    
    return 0;
}
