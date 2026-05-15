#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <getopt.h>
#include <pwd.h>

#include "../include/protocol.h"

#define VERSION "1.0"

void print_help(const char *progname) {
    printf("Usage: %s [OPTIONS]\n", progname);
    printf("Remote command execution client for myRPC system\n\n");
    printf("Options:\n");
    printf("  -c, --command CMD     Bash command to execute\n");
    printf("  -h, --host ADDR       Server IP address (default: localhost)\n");
    printf("  -p, --port PORT       Server port (default: 1234)\n");
    printf("  -s, --stream          Use TCP stream socket\n");
    printf("  -d, --dgram           Use UDP datagram socket\n");
    printf("  -j, --json            Use JSON protocol (default)\n");
    printf("  -t, --text            Use text protocol\n");
    printf("  -v, --version         Show version information\n");
    printf("      --help            Show this help message\n\n");
    printf("Examples:\n");
    printf("  %s -c \"ls -la\" -h 192.168.1.100 -p 1234 -s\n", progname);
}

int main(int argc, char *argv[]) {
    char *command = NULL;
    char *host = "localhost";
    int port = 1234;
    int socket_type = SOCK_STREAM;
    int sock_fd;
    struct sockaddr_in server_addr;
    struct hostent *server;
    char buffer[BUFFER_SIZE];
    char response_buf[MAX_RESPONSE_LEN];
    char username[256];
    struct passwd *pwd;
    
    static struct option long_options[] = {
        {"command", required_argument, 0, 'c'},
        {"host", required_argument, 0, 'h'},
        {"port", required_argument, 0, 'p'},
        {"stream", no_argument, 0, 's'},
        {"dgram", no_argument, 0, 'd'},
        {"json", no_argument, 0, 'j'},
        {"text", no_argument, 0, 't'},
        {"version", no_argument, 0, 'v'},
        {"help", no_argument, 0, 0},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "c:h:p:sdjtv", 
                              long_options, &option_index)) != -1) {
        switch (opt) {
            case 'c':
                command = optarg;
                break;
            case 'h':
                host = optarg;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 's':
                socket_type = SOCK_STREAM;
                break;
            case 'd':
                socket_type = SOCK_DGRAM;
                break;
            case 'v':
                printf("myRPC-client version %s\n", VERSION);
                exit(EXIT_SUCCESS);
            case 0:
                print_help(argv[0]);
                exit(EXIT_SUCCESS);
            default:
                print_help(argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    
    if (command == NULL) {
        fprintf(stderr, "Error: Command is required. Use -c or --command.\n");
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }
    
    pwd = getpwuid(getuid());
    if (pwd == NULL) {
        perror("getpwuid");
        exit(EXIT_FAILURE);
    }
    strncpy(username, pwd->pw_name, sizeof(username) - 1);
    username[sizeof(username) - 1] = '\0';
    
    sock_fd = socket(AF_INET, socket_type, 0);
    if (sock_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    server = gethostbyname(host);
    if (server == NULL) {
        fprintf(stderr, "Error: No such host '%s'\n", host);
        exit(EXIT_FAILURE);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    server_addr.sin_port = htons(port);
    
    /* Формируем JSON запрос */
    snprintf(buffer, sizeof(buffer), 
             "{\"login\":\"%s\",\"command\":\"%s\"}",
             username, command);
    
    if (socket_type == SOCK_STREAM) {
        if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("connect");
            exit(EXIT_FAILURE);
        }
        send(sock_fd, buffer, strlen(buffer), 0);
        memset(response_buf, 0, sizeof(response_buf));
        recv(sock_fd, response_buf, sizeof(response_buf) - 1, 0);
    } else {
        socklen_t addr_len = sizeof(server_addr);
        sendto(sock_fd, buffer, strlen(buffer), 0,
               (struct sockaddr *)&server_addr, addr_len);
        memset(response_buf, 0, sizeof(response_buf));
        recvfrom(sock_fd, response_buf, sizeof(response_buf) - 1, 0,
                (struct sockaddr *)&server_addr, &addr_len);
    }
    
    /* Парсим ответ */
    char *result = strstr(response_buf, "\"result\"");
    if (result) {
        result = strchr(result, ':');
        if (result) {
            result++;
            while (*result == ' ' || *result == '"') result++;
            char *end = strrchr(result, '"');
            if (end) *end = '\0';
            printf("%s", result);
        }
    }
    
    close(sock_fd);
    return 0;
}
