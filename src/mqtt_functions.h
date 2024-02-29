#include "arg_parser.h"
#include <mosquitto.h>

#define MQTT_TOPIC_SIZE 10
#define ACTIONS_SIZE    10
#define ROWS            ACTIONS_SIZE
#define COLS            6
#define MAX_RECIPIANTS  10

struct mqtt_connection_settings {
    int MQTT_PORT;
    int keepalive;
    bool clean_session;
    char *MQTT_BROKER;
    struct uci_email_settings *email_settings;
    char *username;
    char *password;
    char ***actions_set;
    char ***recipiants;
};

struct uci_email_settings {
    char *email_username;
    char *email_smtps;
    char *email_secret;
};

int mqtt_setup(struct mosquitto **mosq, struct mqtt_connection_settings mqtt_conn_set, struct arguments arguments, void *on_connect, void *on_subscribe,
               void *on_message);
void on_connect(struct mosquitto *mosq, void *obj, int reason_code);
void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos);
void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg);
void mqtt_message_event_check(void *obj, char *msg_topic, char *in_the_loop_parameter, int i);