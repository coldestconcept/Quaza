#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "pkg_server.h"

static volatile int running = 1;

void signal_handler(int sig) {
    running = 0;
    pkg_server_stop();
}

int main(int argc, char** argv) {
    printf("Quaza PKG Server\n");
    printf("================\n");
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize server
    if (pkg_server_init() < 0) {
        fprintf(stderr, "Failed to initialize server\n");
        return 1;
    }
    
    // Run server
    printf("Server running. Press Ctrl+C to stop.\n");
    pkg_server_run();
    
    printf("Server stopped.\n");
    return 0;
}
