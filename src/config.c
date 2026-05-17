#include "../include/config.h"

int load_config(struct server_config *cfg) {
    FILE *f = fopen(CONFIG_FILE, "r");
    char line[512];
    
    if (!f) {
        return 0;
    }
    
    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        char *key, *value;
        
        while (isspace(*p)) p++;
        if (*p == '#' || *p == '\n') continue;
        
        key = p;
        while (*p && !isspace(*p) && *p != '=') p++;
        *p++ = '\0';
        
        while (isspace(*p) || *p == '=') p++;
        while (isspace(*p)) p++;
        
        value = p;
        while (*p && *p != '\n') p++;
        *p = '\0';
        
        if (strcmp(key, "port") == 0) {
            cfg->port = atoi(value);
        } else if (strcmp(key, "socket_type") == 0) {
            if (strcmp(value, "stream") == 0) {
                cfg->socket_type = SOCK_STREAM;
            }
        }
    }
    
    fclose(f);
    return 0;
}

int reload_config(struct server_config *cfg) {
    return load_config(cfg);
}

int is_user_allowed(const char *username) {
    FILE *f = fopen(USERS_FILE, "r");
    char line[256];
    
    if (!f) return 0;
    
    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        while (isspace(*p)) p++;
        if (*p == '#') continue;
        
        char *nl = strchr(p, '\n');
        if (nl) *nl = '\0';
        
        if (strcmp(p, username) == 0) {
            fclose(f);
            return 1;
        }
    }
    
    fclose(f);
    return 0;
}
