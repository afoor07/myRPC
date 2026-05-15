#include "../include/protocol.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Разбор текстового запроса: "username": "command" */
int parse_text_request(const char *data, struct rpc_request *req) {
    char *p, *q;
    char buffer[BUFFER_SIZE];
    
    strncpy(buffer, data, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    /* Ищем открывающую кавычку */
    p = strchr(buffer, '"');
    if (!p) return -1;
    p++;
    
    /* Ищем закрывающую кавычку */
    q = strchr(p, '"');
    if (!q) return -1;
    *q = '\0';
    
    /* Копируем имя пользователя */
    strncpy(req->username, p, MAX_USERNAME_LEN - 1);
    req->username[MAX_USERNAME_LEN - 1] = '\0';
    
    /* Ищем команду */
    p = strchr(q + 1, '"');
    if (!p) return -1;
    p++;
    
    q = strchr(p, '"');
    if (!q) return -1;
    *q = '\0';
    
    /* Пропускаем пробелы */
    while (*p == ' ') p++;
    
    /* Копируем команду */
    strncpy(req->command, p, MAX_COMMAND_LEN - 1);
    req->command[MAX_COMMAND_LEN - 1] = '\0';
    
    req->protocol = PROTOCOL_TEXT;
    return 0;
}

/* Упрощенная версия для JSON (без libjson-c) */
int parse_json_request(const char *data, struct rpc_request *req) {
    char *p, *q;
    char buffer[BUFFER_SIZE];
    
    strncpy(buffer, data, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    /* Ищем "login" */
    p = strstr(buffer, "\"login\"");
    if (!p) return -1;
    
    /* Ищем двоеточие */
    p = strchr(p, ':');
    if (!p) return -1;
    p++;
    
    /* Пропускаем пробелы и кавычки */
    while (*p == ' ' || *p == '\t') p++;
    if (*p != '"') return -1;
    p++;
    
    /* Ищем закрывающую кавычку */
    q = strchr(p, '"');
    if (!q) return -1;
    *q = '\0';
    
    /* Копируем имя пользователя */
    strncpy(req->username, p, MAX_USERNAME_LEN - 1);
    req->username[MAX_USERNAME_LEN - 1] = '\0';
    
    /* Ищем "command" */
    p = strstr(q + 1, "\"command\"");
    if (!p) return -1;
    
    /* Ищем двоеточие */
    p = strchr(p, ':');
    if (!p) return -1;
    p++;
    
    /* Пропускаем пробелы и кавычки */
    while (*p == ' ' || *p == '\t') p++;
    if (*p != '"') return -1;
    p++;
    
    /* Ищем закрывающую кавычку */
    q = strchr(p, '"');
    if (!q) return -1;
    *q = '\0';
    
    /* Копируем команду */
    strncpy(req->command, p, MAX_COMMAND_LEN - 1);
    req->command[MAX_COMMAND_LEN - 1] = '\0';
    
    req->protocol = PROTOCOL_JSON;
    return 0;
}

/* Разбор текстового ответа: code: "result" */
int parse_text_response(const char *data, struct rpc_response *resp) {
    char *p, *q;
    char buffer[BUFFER_SIZE];
    
    strncpy(buffer, data, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    /* Ищем код */
    p = buffer;
    resp->code = atoi(p);
    
    /* Ищем результат */
    p = strchr(p, '"');
    if (!p) {
        resp->result[0] = '\0';
        return 0;
    }
    p++;
    
    q = strchr(p, '"');
    if (q) *q = '\0';
    
    strncpy(resp->result, p, MAX_RESPONSE_LEN - 1);
    resp->result[MAX_RESPONSE_LEN - 1] = '\0';
    
    return 0;
}

/* Разбор JSON ответа */
int parse_json_response(const char *data, struct rpc_response *resp) {
    char *p, *q;
    char buffer[BUFFER_SIZE];
    
    strncpy(buffer, data, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    /* Ищем "code" */
    p = strstr(buffer, "\"code\"");
    if (!p) return -1;
    
    /* Ищем двоеточие */
    p = strchr(p, ':');
    if (!p) return -1;
    p++;
    
    /* Пропускаем пробелы */
    while (*p == ' ' || *p == '\t') p++;
    
    /* Читаем код */
    resp->code = atoi(p);
    
    /* Ищем "result" */
    p = strstr(buffer, "\"result\"");
    if (!p) return -1;
    
    /* Ищем двоеточие */
    p = strchr(p, ':');
    if (!p) return -1;
    p++;
    
    /* Пропускаем пробелы и кавычки */
    while (*p == ' ' || *p == '\t') p++;
    if (*p != '"') {
        /* Может быть null */
        resp->result[0] = '\0';
        return 0;
    }
    p++;
    
    /* Ищем закрывающую кавычку */
    q = strchr(p, '"');
    if (q) *q = '\0';
    
    strncpy(resp->result, p, MAX_RESPONSE_LEN - 1);
    resp->result[MAX_RESPONSE_LEN - 1] = '\0';
    
    return 0;
}

/* Форматирование текстового ответа */
int format_text_response(const struct rpc_response *resp, char *buffer, int buflen) {
    return snprintf(buffer, buflen, "%d: \"%s\"", resp->code, resp->result);
}

/* Форматирование JSON ответа */
int format_json_response(const struct rpc_response *resp, char *buffer, int buflen) {
    return snprintf(buffer, buflen, "{\"code\":%d,\"result\":\"%s\"}", 
                    resp->code, resp->result);
}
