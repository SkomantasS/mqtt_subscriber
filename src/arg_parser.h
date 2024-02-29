#include <argp.h>

struct arguments {
    int mqtt_port;
    char mqtt_broker[30];
    int keepalive;
    int clean_session;
    char username[30];
    char password[30];
    char *ca_file;
    char *client_crt;
    char *client_key;
};

error_t parse_opt(int key, char *arg, struct argp_state *state);
