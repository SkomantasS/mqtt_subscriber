#include "mqtt_functions.h"
#include "curl_functions.h"
#include <libubox/blobmsg_json.h>
#include <libubus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

int rc;

int mqtt_setup(struct mosquitto **mosq, struct mqtt_connection_settings mqtt_conn_set, struct arguments arguments, void *on_connect, void *on_subscribe,
               void *on_message) {
    mosquitto_lib_init();

    *mosq = mosquitto_new(NULL, mqtt_conn_set.clean_session, NULL);
    if (*mosq == NULL) {
        syslog(LOG_USER | LOG_INFO, "Error: Out of memory.\n");
        fprintf(stderr, "Error: Out of memory.\n");
        return 1;
    }

    mosquitto_connect_callback_set(*mosq, on_connect);
    mosquitto_user_data_set(*mosq, &mqtt_conn_set);
    mosquitto_subscribe_callback_set(*mosq, on_subscribe);
    mosquitto_message_callback_set(*mosq, on_message);
    if (strcmp(arguments.username, "") && strcmp(arguments.password, ""))
        mosquitto_username_pw_set(*mosq, mqtt_conn_set.username, mqtt_conn_set.password);
    if (arguments.ca_file && arguments.client_crt && arguments.client_key) {
        mosquitto_tls_set(*mosq, arguments.ca_file, NULL, arguments.client_crt, arguments.client_key, NULL);
        mosquitto_tls_opts_set(*mosq, 1, NULL, NULL);
    }
    rc = mosquitto_connect(*mosq, mqtt_conn_set.MQTT_BROKER, mqtt_conn_set.MQTT_PORT, mqtt_conn_set.keepalive);
    if (rc != MOSQ_ERR_SUCCESS) {
        mosquitto_destroy(*mosq);
        syslog(LOG_USER | LOG_INFO, "Error: %s\n", mosquitto_strerror(rc));
        fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
        return 1;
    }
    return 0;
}

void on_connect(struct mosquitto *mosq, void *obj, int reason_code) {
    int rc;
    struct mqtt_connection_settings *temporary_set = (struct mqtt_connection_settings *) obj;

    syslog(LOG_USER | LOG_INFO, "on_connect: %s\n", mosquitto_connack_string(reason_code));
    printf("on_connect: %s\n", mosquitto_connack_string(reason_code));
    if (reason_code != 0) {
        mosquitto_disconnect(mosq);
    }

    char *subTemp[MQTT_TOPIC_SIZE];
    int i = 0;
    while (strcmp(temporary_set->actions_set[i][0], "(empty)")) {
        subTemp[i] = temporary_set->actions_set[i][0];
        i++;
    }
    rc = mosquitto_subscribe_multiple(mosq, NULL, i, subTemp, 1, 0, NULL);
    // rc = mosquitto_subscribe(mosq, NULL, "home/outside/thermometer", 1);
    if (rc != MOSQ_ERR_SUCCESS) {
        syslog(LOG_USER | LOG_ERR, "Error subscribing: %s\n", mosquitto_strerror(rc));
        fprintf(stderr, "Error subscribing: %s\n", mosquitto_strerror(rc));
        mosquitto_disconnect(mosq);
    }
    return;
}

void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos) {
    (void) (obj);
    (void) (mid);
    int i;
    bool have_subscription = false;
    for (i = 0; i < qos_count; i++) {
        syslog(LOG_USER | LOG_INFO, "n_subscribe: %d:granted qos = %d\n", i, granted_qos[i]);
        printf("on_subscribe: %d:granted qos = %d\n", i, granted_qos[i]);
        if (granted_qos[i] <= 2) {
            have_subscription = true;
        }
    }
    if (have_subscription == false) {
        syslog(LOG_USER | LOG_ERR, "Error: All subscriptions rejected.\n");
        fprintf(stderr, "Error: All subscriptions rejected.\n");
        mosquitto_disconnect(mosq);
    }
}

static struct blob_buf b;
enum { TEMP_NAME, TEMP_ID, TEMP_DATA, __TEMP_MAX };
enum { TEMP_DATA_TEMP, TEMP_DATA_HUMID, __TEMP_DATA_MAX };
static const struct blobmsg_policy temp_policy[__TEMP_MAX] = {
    [TEMP_NAME] = {.name = "name", .type = BLOBMSG_TYPE_STRING},
    [TEMP_ID] = {.name = "id", .type = BLOBMSG_TYPE_STRING},
    [TEMP_DATA] = {.name = "data", .type = BLOBMSG_TYPE_TABLE},
};
char in_the_loop_parameter_value[10];
void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
    (void) (mosq);
    struct mqtt_connection_settings *temporary_set = (struct mqtt_connection_settings *) obj;
    struct blob_attr *tb[__TEMP_MAX];
    struct blob_attr *dat[__TEMP_DATA_MAX];
    blob_buf_init(&b, 0);
    blobmsg_add_json_from_string(&b, msg->payload);
    struct blob_attr *json_msg = b.head;
    blobmsg_parse(temp_policy, __TEMP_MAX, tb, blobmsg_data(json_msg), blobmsg_data_len(json_msg));
    if (tb[TEMP_DATA]) {
        for (int i = 0; i < ROWS; i++) {
            if (strcmp(temporary_set->actions_set[i][1], "(empty)")) {
                char in_the_loop_parameter[50];
                sprintf(in_the_loop_parameter, "%s", temporary_set->actions_set[i][1]);
                struct blobmsg_policy in_the_loop_policy[1] = {
                    [0] = {.name = in_the_loop_parameter, .type = BLOBMSG_TYPE_STRING},
                };
                blobmsg_parse(in_the_loop_policy, 1, dat, blobmsg_data(tb[TEMP_DATA]), blobmsg_data_len(tb[TEMP_DATA]));
                if (dat[0]) {
                    sprintf(in_the_loop_parameter_value, "%s", blobmsg_get_string(dat[0]));
                    mqtt_message_event_check(obj, msg->topic, &in_the_loop_parameter_value[0], i);
                }
            }
        }
    }
    syslog(LOG_USER | LOG_INFO, "%s %d %s\n", msg->topic, msg->qos, (char *) msg->payload);
    printf("%s %d %s\n", msg->topic, msg->qos, (char *) msg->payload);
    blob_buf_free(&b);
    return;
}

void mqtt_message_event_check(void *obj, char *msg_topic, char *in_the_loop_parameter_value, int i) {
    struct mqtt_connection_settings *mqtt_conn_set = (struct mqtt_connection_settings *) obj;
    char topic[50];
    char parameter[50];
    char datatype[50];
    char comp_type[50];
    char comp_value[50];
    char email_username[500];
    char email_smtps[500];
    char email_secret[500];
    sprintf(topic, "%s", mqtt_conn_set->actions_set[i][0]);
    sprintf(parameter, "%s", mqtt_conn_set->actions_set[i][1]);
    sprintf(datatype, "%s", mqtt_conn_set->actions_set[i][2]);
    sprintf(comp_type, "%s", mqtt_conn_set->actions_set[i][3]);
    sprintf(comp_value, "%s", mqtt_conn_set->actions_set[i][4]);
    sprintf(email_username, "%s", mqtt_conn_set->email_settings->email_username);
    sprintf(email_smtps, "%s", mqtt_conn_set->email_settings->email_smtps);
    sprintf(email_secret, "%s", mqtt_conn_set->email_settings->email_secret);
    int sendmail = 0;
    if (strcmp(topic, msg_topic)) {
        return;
    }
    if (!strcmp(datatype, "numeric")) {
        if (!strcmp(comp_type, "<")) {
            if (atoi(comp_value) < atoi(in_the_loop_parameter_value))
                sendmail = 1;
        }
        if (!strcmp(comp_type, ">")) {
            if (atoi(comp_value) > atoi(in_the_loop_parameter_value))
                sendmail = 1;
        }
        if (!strcmp(comp_type, "==")) {
            if (atoi(comp_value) == atoi(in_the_loop_parameter_value))
                sendmail = 1;
        }
        if (!strcmp(comp_type, "!=")) {
            if (atoi(comp_value) != atoi(in_the_loop_parameter_value))
                sendmail = 1;
        }
        if (!strcmp(comp_type, ">=")) {
            if (atoi(comp_value) >= atoi(in_the_loop_parameter_value))
                sendmail = 1;
        }
        if (!strcmp(comp_type, "<=")) {
            if (atoi(comp_value) <= atoi(in_the_loop_parameter_value))
                sendmail = 1;
        }
    }
    if (!strcmp(datatype, "alphanumeric")) {
        if (!strcmp(comp_type, "==")) {
            if (!strcmp(comp_value, in_the_loop_parameter_value))
                sendmail = 1;
        }
        if (!strcmp(comp_type, "!=")) {
            if (strcmp(comp_value, in_the_loop_parameter_value))
                sendmail = 1;
        }
    }
    if (sendmail == 1) {
        for (int j = 0; j < COLS; j++) {
            if (strcmp(mqtt_conn_set->recipiants[i][j], "(empty)")) {
                curl_payload_text_set(msg_topic, parameter, in_the_loop_parameter_value, mqtt_conn_set->recipiants[i][j], email_username);
                curl_send_email(email_username, email_smtps, email_secret, mqtt_conn_set->recipiants[i][j]);
            }
        }
        curl_free_payload_text();
    }
}