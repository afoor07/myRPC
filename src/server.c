#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <wait.h>

#include "../include/config.h"
#include "../include/protocol.h"

static volatile sig_atomic_t running = 1;
static int server_fd = -1;
struct server_config cfg;

/* Обработчик сигналов */
void signal_handler(int signo) {
    if (signo == SIGINT || signo == SIGTERM) {
        syslog(LOG_INFO, "Received shutdown signal %d", signo);
        running = 0;
        if (server_fd != -1) {
            close(server_fd);
        }
    } else if (signo == SIGCHLD) {
        while (waitpid(-1, NULL, WNOHANG) > 0);
    }
}

/* Выполнение команды */
int execute_command(const char *command, char *output, size_t output_size) {
    FILE *fp;
    char buffer[1024];
    size_t total = 0;
    
    fp = popen(command, "r");
    if (fp == NULL) {
        snprintf(output, output_size, "Failed to execute command");
        return 1;
    }
    
    output[0] = '\0';
    while (fgets(buffer, sizeof(buffer), fp) != NULL && total < output_size - 1) {
        strncat(output, buffer, output_size - total - 1);
        total += strlen(buffer);
    }
    
    int status = pclose(fp);
    if (status == -1) {
        return 1;
    }
    
    return WEXITSTATUS(status);
}

/* Обработка клиента */
void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    struct rpc_request request;
    struct rpc_response response;
    char output[MAX_RESPONSE_LEN];
    int n;
    
    memset(buffer, 0, sizeof(buffer));
    n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) {
        close(client_fd);
        return;
    }
    
    syslog(LOG_INFO, "Received request: %s", buffer);
    
    /* Пробуем разобрать JSON */
    if (parse_json_request(buffer, &request) == 0) {
        syslog(LOG_INFO, "JSON request from user: %s", request.username);
    } else if (parse_text_request(buffer, &request) == 0) {
        syslog(LOG_INFO, "Text request from user: %s", request.username);
    } else {
        syslog(LOG_ERR, "Invalid request format");
        response.code = 1;
        snprintf(response.result, sizeof(response.result), "Invalid request format");
        goto send_response;
    }
    
    /* Проверяем пользователя */
    if (!is_user_allowed(request.username)) {
        syslog(LOG_WARNING, "Unauthorized user: %s", request.username);
        response.code = 1;
        snprintf(response.result, sizeof(response.result), 
                "User %s not authorized", request.username);
        goto send_response;
    }
    
    /* Выполняем команду */
    syslog(LOG_INFO, "Executing: %s", request.command);
    response.code = execute_command(request.command, output, sizeof(output));
    strncpy(response.result, output, sizeof(response.result) - 1);
    response.result[sizeof(response.result) - 1] = '\0';
    
send_response:
    format_json_response(&response, buffer, sizeof(buffer));
    send(client_fd, buffer, strlen(buffer), 0);
    close(client_fd);
}

/* Запуск сервера */
void run_server(void) {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd;
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        syslog(LOG_ERR, "Socket creation failed: %m");
        exit(EXIT_FAILURE);
    }
    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(cfg.port);
    
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        syslog(LOG_ERR, "Bind failed: %m");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_fd, 10) < 0) {
        syslog(LOG_ERR, "Listen failed: %m");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    syslog(LOG_INFO, "Server listening on port %d", cfg.port);
    
    while (running) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            if (running) {
                syslog(LOG_WARNING, "Accept failed: %m");
            }
            continue;
        }
        
        pid_t pid = fork();
        if (pid == 0) {
            close(server_fd);
            handle_client(client_fd);
            exit(EXIT_SUCCESS);
        } else if (pid > 0) {
            close(client_fd);
        } else {
            syslog(LOG_ERR, "Fork failed: %m");
            close(client_fd);
        }
    }
    
    close(server_fd);
}

int main(int argc, char *argv[]) {
    openlog("myRPC-server", LOG_PID | LOG_CONS, LOG_DAEMON);
    
    /* Загружаем конфигурацию */
    cfg.port = 1234;
    cfg.socket_type = SOCK_STREAM;
    cfg.daemon_mode = 0;
    
    load_config(&cfg);
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGCHLD, signal_handler);
    
    syslog(LOG_INFO, "myRPC-server starting on port %d", cfg.port);
    
    run_server();
    
    syslog(LOG_INFO, "myRPC-server shutting down");
    closelog();
    return 0;
}
