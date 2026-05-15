#ifndef MYRPC_CONFIG_H
#define MYRPC_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <syslog.h>
#include <sys/socket.h>

#define CONFIG_FILE "/etc/myRPC/myrpc.conf"
#define USERS_FILE "/etc/myRPC/users.conf"

/* Конфигурационные параметры */
struct server_config {
    int port;
    int socket_type;  /* SOCK_STREAM или SOCK_DGRAM */
    int daemon_mode;
    int log_facility; /* syslog facility */
};

/* Функции конфигурации */
int load_config(struct server_config *cfg);
int reload_config(struct server_config *cfg);
int is_user_allowed(const char *username);

#endif
