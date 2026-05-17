#ifndef MYRPC_PROTOCOL_H
#define MYRPC_PROTOCOL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Константы */
#define BUFFER_SIZE 8192
#define MAX_USERNAME_LEN 256
#define MAX_COMMAND_LEN 4096
#define MAX_RESPONSE_LEN 65536

/* Типы протоколов */
typedef enum {
    PROTOCOL_TEXT,
    PROTOCOL_JSON
} protocol_type_t;

/* Формат запроса */
struct rpc_request {
    char username[MAX_USERNAME_LEN];
    char command[MAX_COMMAND_LEN];
    protocol_type_t protocol;
};

/* Формат ответа */
struct rpc_response {
    int code;          /* 0 - успех, 1 - ошибка */
    char result[MAX_RESPONSE_LEN];
};

/* Функции для работы с протоколами */
int parse_text_request(const char *data, struct rpc_request *req);
int parse_json_request(const char *data, struct rpc_request *req);
int parse_text_response(const char *data, struct rpc_response *resp);
int parse_json_response(const char *data, struct rpc_response *resp);
int format_text_response(const struct rpc_response *resp, char *buffer, int buflen);
int format_json_response(const struct rpc_response *resp, char *buffer, int buflen);

#endif
