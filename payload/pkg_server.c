#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <ctype.h>
#include "pkg_server.h"
#include "libprosperopkg/include/pkg_types.h"
#include "libprosperopkg/include/pfs_builder.h"
#include "libprosperopkg/include/pkg_header.h"

static int server_fd = -1;
static pkg_progress_t current_progress;
static pthread_mutex_t progress_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread
