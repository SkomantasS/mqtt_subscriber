#include "arg_parser.h"
#include <stdlib.h>
#include <string.h>

error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;
    switch (key) {
    case 'p':
        arguments->mqtt_port = atoi(arg);
        break;
    case 'h':
        strcpy(arguments->mqtt_broker, arg);
        break;
    case 'k':
        arguments->keepalive = atoi(arg);
        break;
    case 'c':
        arguments->clean_session = 1;
        break;
    case 'u':
        strcpy(arguments->username, arg);
        break;
    case 'P':
        strcpy(arguments->password, arg);
        break;
    case 991:
        arguments->ca_file = malloc(50 * sizeof(char));
        strcpy(arguments->ca_file, "/etc/ssl/certs/ca.cert.pem");
        if (arg)
            strcpy(arguments->ca_file, arg);
        break;
    case 992:
        arguments->client_crt = malloc(50 * sizeof(char));
        strcpy(arguments->client_crt, "/etc/ssl/certs/client.cert.pem");
        if (arg)
            strcpy(arguments->client_crt, arg);
        break;
    case 993:
        arguments->client_key = malloc(50 * sizeof(char));
        strcpy(arguments->client_key, "/etc/ssl/certs/client.key.pem");
        if (arg)
            strcpy(arguments->client_key, arg);
        break;

    case ARGP_KEY_FINI:

        break;
    }
    return 0;
}
