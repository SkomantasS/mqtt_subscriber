#include "mqtt_functions.h"
#include "uci_functions.h"
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
    syslog(LOG_USER | LOG_INFO, "sigterm\n");
    end = 1;
}

void allocate_topic_array(char ****member, int rows, int cols) {
    // Allocate memory for the array of pointers
    *member = malloc(rows * sizeof(char **));
    for (int i = 0; i < rows; i++) {
        // Allocate memory for each row (array of pointers)
        (*member)[i] = malloc((cols + 1) * sizeof(char *));   // Extra space for NULL terminator
        for (int j = 0; j < cols; j++) {
            // Allocate memory for each pointer (topic string)
            (*member)[i][j] = malloc(50 * sizeof(char));   // Assuming max topic length is 50
            sprintf((*member)[i][j], "(empty)", i, j);     // Example initialization
        }
        // Set the last pointer in each row to NULL
        (*member)[i][cols] = NULL;
    }
}

void free_topic_array(char ****member, int rows, int cols) {
    // Free memory for each pointer in the array
    for (int i = 0; i < rows; i++) {
        for (int j = 0; (*member)[i][j] != NULL; j++) {
            free((*member)[i][j]);
        }
        // Free memory for each row (array of pointers)
        free((*member)[i]);
    }
    // Free memory for the array itself
    free(*member);
    // Set the member pointer to NULL to indicate it's no longer valid
    *member = NULL;
}

int main() {
    const char *LOGNAME = "MQTT_SUB";
    openlog(LOGNAME, LOG_PID, LOG_USER);
    syslog(LOG_USER | LOG_INFO, "Starting mqtt_sub\n");

    struct sigaction psa;
    psa.sa_handler = pSigHandler;
    sigaction(SIGTERM, &psa, NULL);

    struct mosquitto *mosq = NULL;
    struct mqtt_connection_settings mqtt_conn_set;

    mqtt_conn_set.MQTT_TOPIC = calloc(1, MQTT_TOPIC_SIZE * sizeof(char *));
    mqtt_conn_set.recipiants = calloc(1, ACTIONS_SIZE * sizeof(char *));
    allocate_topic_array(&mqtt_conn_set.actions_set, ROWS, COLS);
    allocate_topic_array(&mqtt_conn_set.recipiants, ROWS, COLS);

    uci_mqtt_data(&mqtt_conn_set);
    uci_actions_data(&mqtt_conn_set);

    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (strcmp(mqtt_conn_set.actions_set[i][j], "(empty)"))
                printf("%s\n", mqtt_conn_set.actions_set[i][j]);
        }
    }

    mqtt_setup(&mosq, mqtt_conn_set, &on_connect, &on_subscribe, &on_message);

    mosquitto_loop_start(mosq);
    syslog(LOG_USER | LOG_INFO, "Started mosquitto_loop\n");
    while (end == 0) {
    }
    mosquitto_loop_stop(mosq, true);

    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    for (int i = 0; i < MQTT_TOPIC_SIZE; i++) {
        free(mqtt_conn_set.MQTT_TOPIC[i]);
    }
    free(mqtt_conn_set.MQTT_TOPIC);
    free_topic_array(&mqtt_conn_set.actions_set, ROWS, COLS);
    free_topic_array(&mqtt_conn_set.recipiants, ROWS, COLS);
    uci_unload_and_free_context();
    syslog(LOG_USER | LOG_INFO, "Stopping mqtt_sub\n");
    closelog();
    return 0;
}