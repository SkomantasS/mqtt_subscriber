#include "mqtt_functions.h"
#include <mosquitto.h>
#include <stdio.h>
#include <stdlib.h>
#include <uci.h>

struct uci_context *ctx;
struct uci_package *pkg;

int uci_mqtt_data(struct mqtt_connection_settings *mqtt_conn_set) {
    int ret = 0;
    ctx = uci_alloc_context();
    if (!ctx) {
        fprintf(stderr, "Error: Unable to allocate UCI context\n");
        return -1;
    }
    pkg = NULL;
    if (uci_load(ctx, "mqtt_sub", &pkg) != UCI_OK) {
        fprintf(stderr, "Error: Failed to load package\n");
        uci_free_context(ctx);
        return -1;
    }
    struct uci_section *sec = uci_lookup_section(ctx, pkg, "connection_set");
    if (!sec) {
        fprintf(stderr, "Error: Failed to find section\n");
        uci_unload(ctx, pkg);
        uci_free_context(ctx);
        return -1;
    }
    const char *mqtt_port = uci_lookup_option_string(ctx, sec, "mqtt_port");
    const char *keepalive = uci_lookup_option_string(ctx, sec, "keepalive");
    const char *clean_session = uci_lookup_option_string(ctx, sec, "clean_session");
    const char *mqtt_broker = uci_lookup_option_string(ctx, sec, "mqtt_broker");
    const char *username = uci_lookup_option_string(ctx, sec, "username");
    const char *password = uci_lookup_option_string(ctx, sec, "password");

    struct uci_element *element;
    struct uci_option *option;
    option = uci_lookup_option(ctx, sec, "mqtt_topic");
    int i = 0;
    uci_foreach_element(&option->v.list, element) {   // FIND ALL TOPICS
        mqtt_conn_set->MQTT_TOPIC[i] = malloc(50 * sizeof(char));
        sprintf(mqtt_conn_set->MQTT_TOPIC[i], "%s", element->name);
        i++;
    }

    if (!mqtt_broker) {
        fprintf(stderr, "Error: Failed to find mqtt_broker option\n");
        uci_unload(ctx, pkg);
        uci_free_context(ctx);
        return -1;
    }
    printf("MQTT Broker IP: %s\n", mqtt_broker);

    mqtt_conn_set->MQTT_PORT = atoi(mqtt_port);
    mqtt_conn_set->keepalive = atoi(keepalive);
    mqtt_conn_set->clean_session = atoi(clean_session);
    mqtt_conn_set->MQTT_BROKER = mqtt_broker;
    mqtt_conn_set->username = username;
    mqtt_conn_set->password = password;

    return 0;
}

int uci_actions_data(struct mqtt_connection_settings *mqtt_conn_set) {
    int ret = 0;
    ctx = uci_alloc_context();
    if (!ctx) {
        fprintf(stderr, "Error: Unable to allocate UCI context\n");
        return -1;
    }
    pkg = NULL;
    if (uci_load(ctx, "mqtt_sub", &pkg) != UCI_OK) {
        fprintf(stderr, "Error: Failed to load package\n");
        uci_free_context(ctx);
        return -1;
    }
    for (int i = 0; i < ROWS; i++) {
        char event_section_name[10];
        sprintf(event_section_name, "event%d", i + 1);
        printf("%s\n", event_section_name);
        struct uci_section *sec = uci_lookup_section(ctx, pkg, event_section_name);
        if (!sec) {
            fprintf(stderr, "Error: Failed to find section, or out of events\n");
            uci_unload(ctx, pkg);
            uci_free_context(ctx);
            return -1;
        }
        printf("CIKLAS:%i\n", i);
        char *topic = uci_lookup_option_string(ctx, sec, "topic");
        char *parameter = uci_lookup_option_string(ctx, sec, "parameter");
        char *datatype = uci_lookup_option_string(ctx, sec, "datatype");
        char *comp_type = uci_lookup_option_string(ctx, sec, "comp_type");
        char *comp_value = uci_lookup_option_string(ctx, sec, "comp_value");
        char *email_username = uci_lookup_option_string(ctx, sec, "email_username");
        char *email_smtps = uci_lookup_option_string(ctx, sec, "email_smtps");
        char *email_secret = uci_lookup_option_string(ctx, sec, "email_secret");

        struct uci_element *element;
        struct uci_option *option;
        option = uci_lookup_option(ctx, sec, "recipiant");
        int j = 0;
        uci_foreach_element(&option->v.list, element) {   // FIND ALL RECIPIANTS
            mqtt_conn_set->recipiants[j] = malloc(50 * sizeof(char));
            sprintf(mqtt_conn_set->recipiants[j], "%s", element->name);
            j++;
        }

        sprintf(mqtt_conn_set->actions_set[i][0], "%s", topic);
        sprintf(mqtt_conn_set->actions_set[i][1], "%s", parameter);
        sprintf(mqtt_conn_set->actions_set[i][2], "%s", datatype);
        sprintf(mqtt_conn_set->actions_set[i][3], "%s", comp_type);
        sprintf(mqtt_conn_set->actions_set[i][4], "%s", comp_value);
        sprintf(mqtt_conn_set->actions_set[i][5], "%s", email_username);
        sprintf(mqtt_conn_set->actions_set[i][6], "%s", email_smtps);
        sprintf(mqtt_conn_set->actions_set[i][7], "%s", email_secret);
        // sprintf(mqtt_conn_set->actions_set[0][8], "%s", email_secret);
    }
    return 0;
}

void uci_unload_and_free_context() {
    uci_unload(ctx, pkg);
    uci_free_context(ctx);
}