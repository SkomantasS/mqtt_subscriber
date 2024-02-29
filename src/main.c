// #include "arg_parser.h"
#include "mqtt_functions.h"
#include "uci_functions.h"
#include <argp.h>
#include <mosquitto.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <syslog.h>
#include <uci.h>

volatile sig_atomic_t end = 0;

static void pSigHandler(int signo) {
    (void) (signo);
    syslog(LOG_USER | LOG_INFO, "sigterm\n");
    end = 1;
}

void allocate_topic_array(char ****member, int rows, int cols);
void free_topic_array(char ****member, int rows);

int main(int argc, char **argv) {
    // argp begin
    int ret = 0;
    char doc[] = "Program to subscribe to MQTT topics, wait for conditions to be met and send emails when they are.";
    char args_doc[] = "<Device-ID> <Pruduct-ID> <Device-secret>";
    struct argp_option options[] = {
        {0, 'p', "port", 0, "Mqtt port. if not specified 1883 will be used", 0},
        {0, 'h', "mqtt-broker", 0, "Mqtt broker. if not specified localhost will be used", 0},
        {0, 'k', "keep-alive", 0, "Keep alive. if not specified 60 seconds will be used", 0},
        {0, 'c', "clean-session", 0, "Clean session, if not specified 1 (true) will be used", 0},
        {0, 'u', "username", 0, "Username", 0},
        {0, 'P', "password", 0, "Password", 0},
        {"cafile", 991, "cafile", OPTION_ARG_OPTIONAL, "Cafile, if flag is used but path not specified will look in /etc/ssl/certs/ca.cert.pem", 0},
        {"cert", 992, "cert", OPTION_ARG_OPTIONAL, "Client cert file, if flag is used but path is not specified will look in /etc/ssl/certs/client.cert.pem",
         0},
        {"key", 993, "key", OPTION_ARG_OPTIONAL, "Client key file, if flag is used but path not specified will look in /etc/ssl/certs/client.key.pem", 0},
        {0},
    };
    struct argp argp = {options, parse_opt, args_doc, doc, 0, 0, 0};

    struct arguments arguments = {
        .mqtt_port = 1883,
        .mqtt_broker = "localhost",
        .keepalive = 60,
        .clean_session = 1,
        .username = {0},
        .password = {0},
        .ca_file = NULL,
        .client_crt = NULL,
        .client_key = NULL,
    };

    ret = argp_parse(&argp, argc, argv, 0, 0, &arguments);
    if (ret) {
        return ret;
    }
    // argp end

    const char *LOGNAME = "MQTT_SUB";
    openlog(LOGNAME, LOG_PID, LOG_USER);
    syslog(LOG_USER | LOG_INFO, "Starting mqtt_sub\n");

    struct sigaction psa;
    psa.sa_handler = pSigHandler;
    sigaction(SIGTERM, &psa, NULL);

    struct mosquitto *mosq = NULL;
    struct mqtt_connection_settings mqtt_conn_set;

    mqtt_conn_set.recipiants = calloc(1, ACTIONS_SIZE * sizeof(char *));
    allocate_topic_array(&mqtt_conn_set.actions_set, ROWS, COLS);
    allocate_topic_array(&mqtt_conn_set.recipiants, ROWS, MAX_RECIPIANTS);
    mqtt_conn_set.email_settings = calloc(1, 3 * ((sizeof(char) * 100 + 1)));
    mqtt_conn_set.email_settings->email_username = calloc(1, sizeof(char) * 100 + 1);
    mqtt_conn_set.email_settings->email_smtps = calloc(1, sizeof(char) * 100 + 1);
    mqtt_conn_set.email_settings->email_secret = calloc(1, sizeof(char) * 100 + 1);
    uci_mqtt_data(&mqtt_conn_set);

    mqtt_conn_set.clean_session = arguments.clean_session;
    mqtt_conn_set.keepalive = arguments.keepalive;
    mqtt_conn_set.MQTT_BROKER = arguments.mqtt_broker;
    mqtt_conn_set.MQTT_PORT = arguments.mqtt_port;
    mqtt_conn_set.username = arguments.username;
    mqtt_conn_set.password = arguments.password;

    uci_actions_data(&mqtt_conn_set);
    mqtt_setup(&mosq, mqtt_conn_set, arguments, &on_connect, &on_subscribe, &on_message);

    mosquitto_loop_start(mosq);
    syslog(LOG_USER | LOG_INFO, "Started mosquitto_loop\n");
    while (end == 0) {
    }
    mosquitto_loop_stop(mosq, true);

    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    free_topic_array(&mqtt_conn_set.actions_set, ROWS);
    free_topic_array(&mqtt_conn_set.recipiants, ROWS);
    uci_unload_and_free_context();
    syslog(LOG_USER | LOG_INFO, "Stopping mqtt_sub\n");
    closelog();
    return 0;
}

void allocate_topic_array(char ****member, int rows, int cols) {
    *member = malloc(rows * sizeof(char **));
    for (int i = 0; i < rows; i++) {
        (*member)[i] = malloc((cols + 1) * sizeof(char *));
        for (int j = 0; j < cols; j++) {
            (*member)[i][j] = malloc(50 * sizeof(char));
            sprintf((*member)[i][j], "(empty)");
        }
        (*member)[i][cols] = NULL;
    }
}

void free_topic_array(char ****member, int rows) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; (*member)[i][j] != NULL; j++) {
            free((*member)[i][j]);
        }
        free((*member)[i]);
    }
    free(*member);
    *member = NULL;
}

// DONE//kaip argp general info
// DONE// kaip type sectionai ieskomi eventai
// DONE//vienas email serveris siuntimo
// sqlite saugoti topico duomenis
// lua scripta naudot pagal topic pafiltruot

// DONE//subscribe to topics from events
// hardcode default broker port settings tls etc
// ignore exactly identical events

// DONE//jei viena msg tenkina dvi salygas
// DONE//jei brokeris trumpai atsijungia kaip veikia programa

// wireshark naudojant patikrinti ar tls encryptina