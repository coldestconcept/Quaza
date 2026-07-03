#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "pkg_server.h"

static volatile int running = 1;

void signal_handler(int sig) {
    running = 0;
    pkg_server_stop();
}

int main(int argc, char** argv) {
    printf("Quaza PKG Server\n");
    printf("================\n");
    printf("Native PS5 PKG Converter Payload\n\n");
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    if
