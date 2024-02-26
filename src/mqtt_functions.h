#include <mosquitto.h>
#define MQTT_TOPIC_SIZE 10
#define ACTIONS_SIZE    10
#define ROWS            ACTIONS_SIZE
#define COLS            9

struct mqtt_connection_settings {
    int MQTT_PORT;
    int keepalive;
    bool clean_session;
    char *MQTT_BROKER;
    char **MQTT_TOPIC;
    char *username;
    char *password;
    char ***actions_set;
    char ***recipiants;
};

int mqtt_setup(struct mosquitto **mosq, struct mqtt_connection_settings mqtt_conn_set, void *on_connect, void *on_subscribe, void *on_message);
void on_connect(struct mosquitto *mosq, void *obj, int reason_code);
void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos);
void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg);
void mqtt_message_event_check(void *obj, char *msg_topic, char *temp, char *humidity);