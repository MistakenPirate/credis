#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "domain/handle.h"

int main(int argc, char *argv[]) {
    char *port_str = "6379";
    int port;
    char *host = "127.0.0.1";

    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (!strcmp(argv[i], "-p") && (i + 1) < argc) {
                port_str = argv[i + 1];
            }
            if (!strcmp(argv[i], "-h") && (i + 1) < argc) {
                host = argv[i + 1];
            }
        }
    }

    port = atoi(port_str);
    printf("%s : host | %d : port\n", host, port);

    // Start the server
    multiplexing(host, port);

    return 0;
}