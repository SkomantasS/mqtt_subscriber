#include "mqtt_functions.h"
#include "curl_functions.h"
#include <libubox/blobmsg_json.h>
#include <libubus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

int rc;

// char *CA_CERT = "/certs/ca.cert.pem";
// char *CLIENT_CRT = "/certs/client.cert.pem";
// char *CLIENT_KEY = "/certs/client.key.pem";

char *CA_CERT = "/../etc/ssl/certs/ca.cert.pem";
char *CLIENT_CRT = "/../etc/ssl/certs/client.cert.pem";
char *CLIENT_KEY = "/../etc/ssl/certs/client.key.pem";

int mqtt_setup(struct mosquitto **mosq, struct mqtt_connection_settings mqtt_conn_set, void *on_connect, void *on_subscribe, void *on_message) {
    char CA_CERT_P[150];
    char CLIENT_CRT_P[150];
    char CLIENT_KEY_P[150];

    strcpy(CA_CERT_P, getenv("HOME"));
    strcpy(CLIENT_CRT_P, getenv("HOME"));
    strcpy(CLIENT_KEY_P, getenv("HOME"));

    strcat(CA_CERT_P, CA_CERT);
    strcat(CLIENT_CRT_P, CLIENT_CRT);
    strcat(CLIENT_KEY_P, CLIENT_KEY);

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

    mosquitto_username_pw_set(*mosq, mqtt_conn_set.username, mqtt_conn_set.password);
    mosquitto_tls_set(*mosq, CA_CERT_P, NULL, CLIENT_CRT_P, CLIENT_KEY_P, NULL);
    mosquitto_tls_opts_set(*mosq, 1, NULL, NULL);

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

    printf("MQTT_PORT:%i\n", temporary_set->MQTT_PORT);
    printf("keepalive:%i\n", temporary_set->keepalive);
    printf("clean_session:%i\n", temporary_set->clean_session);
    printf("MQTT_BROKER:%s\n", temporary_set->MQTT_BROKER);
    printf("username:%s\n", temporary_set->username);
    printf("password:%s\n", temporary_set->password);

    syslog(LOG_USER | LOG_INFO, "on_connect: %s\n", mosquitto_connack_string(reason_code));
    printf("on_connect: %s\n", mosquitto_connack_string(reason_code));
    if (reason_code != 0) {
        mosquitto_disconnect(mosq);
    }

    char *subTemp[MQTT_TOPIC_SIZE];
    int i = 0;
    while (temporary_set->MQTT_TOPIC[i]) {
        subTemp[i] = temporary_set->MQTT_TOPIC[i];
        i++;
    }

    rc = mosquitto_subscribe_multiple(mosq, NULL, i, subTemp, 1, 0, NULL);
    if (rc != MOSQ_ERR_SUCCESS) {
        syslog(LOG_USER | LOG_ERR, "Error subscribing: %s\n", mosquitto_strerror(rc));
        fprintf(stderr, "Error subscribing: %s\n", mosquitto_strerror(rc));
        mosquitto_disconnect(mosq);
    }
    return;
}

void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos) {
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
static const struct blobmsg_policy temp_data_policy[__TEMP_DATA_MAX] = {
    [TEMP_DATA_TEMP] = {.name = "temp", .type = BLOBMSG_TYPE_STRING},
    [TEMP_DATA_HUMID] = {.name = "humidity", .type = BLOBMSG_TYPE_STRING},
};

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
    int ret = 0;
    struct mqtt_connection_settings *temporary_set = (struct mqtt_connection_settings *) obj;

    struct blob_attr *tb[__TEMP_MAX];
    struct blob_attr *dat[__TEMP_DATA_MAX];
    blob_buf_init(&b, 0);
    blobmsg_add_json_from_string(&b, msg->payload);
    // blobmsg_add_json_from_string(&b, msg_payload);
    struct blob_attr *json_msg = b.head;
    blobmsg_parse(temp_policy, __TEMP_MAX, tb, blobmsg_data(json_msg), blobmsg_data_len(json_msg));
    if (tb[TEMP_DATA]) {
        printf("success on data\n");
    } else
        printf("fail data\n");

    blobmsg_parse(temp_data_policy, __TEMP_DATA_MAX, dat, blobmsg_data(tb[TEMP_DATA]), blobmsg_data_len(tb[TEMP_DATA]));
    if (tb[TEMP_NAME])
        printf("name: %s\n", blobmsg_get_string(tb[TEMP_NAME]));
    else
        printf("fail name\n");
    if (tb[TEMP_ID])
        printf("id: %s\n", blobmsg_get_string(tb[TEMP_ID]));
    else
        printf("fail id\n");
    if (dat[TEMP_DATA_TEMP])
        printf("temp: %i\n", atoi(blobmsg_get_string(dat[TEMP_DATA_TEMP])));
    else
        printf("fail temp\n");
    if (dat[TEMP_DATA_HUMID])
        printf("humidity: %i\n", atoi(blobmsg_get_string(dat[TEMP_DATA_HUMID])));
    else
        printf("fail humidity\n");

    printf("ERROR: %i\n", ret);

    syslog(LOG_USER | LOG_INFO, "%s %d %s\n", msg->topic, msg->qos, (char *) msg->payload);
    printf("%s %d %s\n", msg->topic, msg->qos, (char *) msg->payload);
    char temp[10];
    sprintf(temp, "%s", blobmsg_get_string(dat[TEMP_DATA_TEMP]));
    char humidity[10];
    sprintf(humidity, "%s", blobmsg_get_string(dat[TEMP_DATA_HUMID]));

    mqtt_message_event_check(obj, msg->topic, &temp[0], &humidity[0]);

    blob_buf_free(&b);
    return;
}

void mqtt_message_event_check(void *obj, char *msg_topic, char *temp, char *humidity) {
    struct mqtt_connection_settings *temporary_set = (struct mqtt_connection_settings *) obj;
    char topic[50];
    char parameter[50];
    char datatype[50];
    char comp_type[50];
    char comp_value[50];
    char email_username[50];
    char email_smtps[50];
    char email_secret[50];
    for (int i = 0; i < ROWS; i++) {
        sprintf(topic, "%s", temporary_set->actions_set[i][0]);
        sprintf(parameter, "%s", temporary_set->actions_set[i][1]);
        sprintf(datatype, "%s", temporary_set->actions_set[i][2]);
        sprintf(comp_type, "%s", temporary_set->actions_set[i][3]);
        sprintf(comp_value, "%s", temporary_set->actions_set[i][4]);
        sprintf(email_username, "%s", temporary_set->actions_set[i][5]);
        sprintf(email_smtps, "%s", temporary_set->actions_set[i][6]);
        sprintf(email_secret, "%s", temporary_set->actions_set[i][7]);
        // printf("topic:%s\n", topic);
        // printf("parameter:%s\n", parameter);
        // printf("datatype:%s\n", datatype);
        // printf("comp_type:%s\n", comp_type);
        // printf("comp_value:%s\n", comp_value);
        // printf("email_username:%s\n", email_username);
        // printf("email_smtps:%s\n", email_smtps);
        // printf("email_secret:%s\n", email_secret);
        int sendmail = 0;
        char sensor_data[10];
        printf("topic cmp %i\n", strcmp(topic, msg_topic));
        if (strcmp(topic, msg_topic)) {
            continue;
            printf("continuing");
        }
        if (!strcmp(parameter, "temp"))
            sprintf(sensor_data, "%s", temp);
        if (!strcmp(parameter, "humidity"))
            sprintf(sensor_data, "%s", humidity);
        if (!strcmp(datatype, "numeric")) {
            if (!strcmp(comp_type, "<")) {
                if (atoi(comp_value) < atoi(sensor_data))
                    sendmail = 1;
            }
            if (!strcmp(comp_type, ">")) {
                if (atoi(comp_value) > atoi(sensor_data))
                    sendmail = 1;
            }
            if (!strcmp(comp_type, "==")) {
                if (atoi(comp_value) == atoi(sensor_data))
                    sendmail = 1;
            }
            if (!strcmp(comp_type, "!=")) {
                if (atoi(comp_value) != atoi(sensor_data))
                    sendmail = 1;
            }
            if (!strcmp(comp_type, ">=")) {
                if (atoi(comp_value) >= atoi(sensor_data))
                    sendmail = 1;
            }
            if (!strcmp(comp_type, "<=")) {
                if (atoi(comp_value) <= atoi(sensor_data))
                    sendmail = 1;
            }
        }
        if (!strcmp(datatype, "alphanumeric")) {
            if (!strcmp(comp_type, "==")) {
                if (!strcmp(comp_value, sensor_data))
                    sendmail = 1;
            }
            if (!strcmp(comp_type, "!=")) {
                if (strcmp(comp_value, sensor_data))
                    sendmail = 1;
            }
        }
        if (sendmail == 1) {
            curl_payload_text_set(msg_topic, parameter, sensor_data, email_username);
            for (int j = 0; j < COLS; j++) {
                if (strcmp(temporary_set->recipiants[i][j], "(empty)"))
                    curl_send_email(email_username, email_smtps, email_secret, temporary_set->recipiants[i][j]);
            }
            curl_free_payload_text();
        }
    }
}