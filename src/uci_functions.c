#include "mqtt_functions.h"
#include <mosquitto.h>
#include <openssl/evp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uci.h>

struct uci_context *ctx;
struct uci_package *pkg;

#define MAX_STRINGS 1000   // Maximum number of strings
#define HASH_LENGTH 32     // SHA-256 hash length in bytes

unsigned char *hash_strings(char **strings, int num_strings) {
    EVP_MD_CTX *mdctx;
    const EVP_MD *md;
    unsigned char *hash = malloc(HASH_LENGTH);

    OpenSSL_add_all_digests();
    md = EVP_get_digestbyname("sha256");

    if (!md) {
        printf("Unknown message digest\n");
        exit(1);
    }

    mdctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mdctx, md, NULL);

    for (int i = 0; i < num_strings; i++) {
        EVP_DigestUpdate(mdctx, strings[i], strlen(strings[i]));
    }

    EVP_DigestFinal_ex(mdctx, hash, NULL);
    EVP_MD_CTX_free(mdctx);

    return hash;
}
int uci_mqtt_data(struct mqtt_connection_settings *mqtt_conn_set) {
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
    struct uci_section *sec = uci_lookup_section(ctx, pkg, "emai_sender_info");
    if (!sec) {
        fprintf(stderr, "Error: Failed to find section\n");
        uci_unload(ctx, pkg);
        uci_free_context(ctx);
        return -1;
    }
    // const char *email_username = uci_lookup_option_string(ctx, sec, "email_username");
    // const char *email_smtps = uci_lookup_option_string(ctx, sec, "email_smtps");
    // const char *email_secret = uci_lookup_option_string(ctx, sec, "email_secret");

    // if (!email_username || !email_smtps || !email_secret) {
    //     fprintf(stderr, "Error: Failed to find email options\n");
    //     uci_unload(ctx, pkg);
    //     uci_free_context(ctx);
    //     return -1;
    // }
    sprintf(mqtt_conn_set->email_settings->email_username, "%s", uci_lookup_option_string(ctx, sec, "email_username"));
    sprintf(mqtt_conn_set->email_settings->email_smtps, "%s", uci_lookup_option_string(ctx, sec, "email_smtps"));
    sprintf(mqtt_conn_set->email_settings->email_secret, "%s", uci_lookup_option_string(ctx, sec, "email_secret"));
    // mqtt_conn_set->email_settings->email_username = uci_lookup_option_string(ctx, sec, "email_username");
    // mqtt_conn_set->email_settings->email_smtps = uci_lookup_option_string(ctx, sec, "email_smtps");
    // mqtt_conn_set->email_settings->email_secret = uci_lookup_option_string(ctx, sec, "email_secret");

    return 0;
}

struct uci_section *uci_unnamed_section_get(char *pkg_name, char *name) {
    struct uci_ptr ptr = {
        .package = pkg_name,
        .section = name,
        .flags = (name && *name == '@') ? UCI_LOOKUP_EXTENDED : 0,
    };
    if (!ctx) {
        ctx = uci_alloc_context();
        if (!ctx)
            return NULL;
    }
    if (uci_lookup_ptr(ctx, &ptr, NULL, true))
        return NULL;
    return ptr.s;
}

int uci_actions_data(struct mqtt_connection_settings *mqtt_conn_set) {
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
        char event_section_name[20];
        sprintf(event_section_name, "@mqtt_sub[%d]", i + 1);
        struct uci_section *sec = uci_unnamed_section_get("mqtt_sub", event_section_name);
        if (sec == NULL) {
            fprintf(stderr, "Error: Failed to find section, or out of events\n");
            uci_unload(ctx, pkg);
            uci_free_context(ctx);
            return -1;
        }

        struct uci_element *element;
        struct uci_option *option;
        option = uci_lookup_option(ctx, sec, "recipiant");
        int j = 0;
        uci_foreach_element(&option->v.list, element) {   // FIND ALL RECIPIANTS
            sprintf(mqtt_conn_set->recipiants[i][j], "%s", element->name);
            j++;
        }

        sprintf(mqtt_conn_set->actions_set[i][0], "%s", uci_lookup_option_string(ctx, sec, "topic"));
        sprintf(mqtt_conn_set->actions_set[i][1], "%s", uci_lookup_option_string(ctx, sec, "parameter"));
        sprintf(mqtt_conn_set->actions_set[i][2], "%s", uci_lookup_option_string(ctx, sec, "datatype"));
        sprintf(mqtt_conn_set->actions_set[i][3], "%s", uci_lookup_option_string(ctx, sec, "comp_type"));
        sprintf(mqtt_conn_set->actions_set[i][4], "%s", uci_lookup_option_string(ctx, sec, "comp_value"));

        int num_strings = 6;
        unsigned char *hash = hash_strings(mqtt_conn_set->actions_set[i], num_strings);
        sprintf(mqtt_conn_set->actions_set[i][5], "%s", hash);
    }
    return 0;
}

void uci_unload_and_free_context() {
    uci_unload(ctx, pkg);
    uci_free_context(ctx);
}