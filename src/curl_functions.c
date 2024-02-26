/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * SPDX-License-Identifier: curl
 *
 ***************************************************************************/

/* <DESC>
 * Send SMTP email using implicit SSL
 * </DESC>
 */

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* This is a simple example showing how to send mail using libcurl's SMTP
 * capabilities. It builds on the smtp-mail.c example to add authentication
 * and, more importantly, transport security to protect the authentication
 * details from being snooped.
 *
 * Note that this example requires libcurl 7.20.0 or above.
 */

#define FROM_MAIL "<pimatic.esp32.communication@gmail.com>"   // change impleme
// #define TO_MAIL   "<pimatic.esp32.communication@gmail.com>"

char *payload_text = NULL;

struct upload_status {
    size_t bytes_read;
};

void curl_payload_text_set(char *Topic, char *Parameter, char *Value,
                           char *to_mail) {
    char *data = NULL;
    size_t required_size = snprintf(NULL, 0,
                                    "To: %s\r\n"
                                    "From: " FROM_MAIL "\r\n"
                                    "Subject: Event report\r\n"
                                    "\r\n"
                                    "Topic: %s\r\n"
                                    "Parameter: %s\r\n"
                                    "Value: %s",
                                    to_mail, Topic, Parameter, Value) +
                           1;
    data = calloc(1, required_size);
    sprintf(data,
            "To: %s\r\n"
            "From: " FROM_MAIL "\r\n"
            "Subject: Event report\r\n"
            "\r\n"
            "Topic: %s\r\n"
            "Parameter: %s\r\n"
            "Value: %s",
            to_mail, Topic, Parameter, Value);
    payload_text = data;
    return;
}

static size_t payload_source(char *ptr, size_t size, size_t nmemb,
                             void *userp) {
    struct upload_status *upload_ctx = (struct upload_status *) userp;
    const char *data;
    size_t room = size * nmemb;

    if ((size == 0) || (nmemb == 0) || ((size * nmemb) < 1)) {
        return 0;
    }

    data = &payload_text[upload_ctx->bytes_read];

    if (data) {
        size_t len = strlen(data);
        if (room < len)
            len = room;
        memcpy(ptr, data, len);
        upload_ctx->bytes_read += len;

        return len;
    }

    return 0;
}

int curl_send_email(char *email_username, char *email_smtps, char *email_secret,
                    char *to_mail) {
    CURL *curl;
    CURLcode res = CURLE_OK;
    struct curl_slist *recipients = NULL;
    struct upload_status upload_ctx = {0};

    curl = curl_easy_init();
    if (curl) {
        /* Set username and password */
        curl_easy_setopt(curl, CURLOPT_USERNAME, email_username);
        curl_easy_setopt(curl, CURLOPT_PASSWORD, email_secret);

        /* This is the URL for your mailserver. Note the use of smtps:// rather
         * than smtp:// to request a SSL based connection. */
        curl_easy_setopt(curl, CURLOPT_URL, email_smtps);

        /* If you want to connect to a site who is not using a certificate that
         * is signed by one of the certs in the CA bundle you have, you can skip
         * the verification of the server's certificate. This makes the
         * connection A LOT LESS SECURE.
         *
         * If you have a CA cert for the server stored someplace else than in
         * the default bundle, then the CURLOPT_CAPATH option might come handy
         * for you. */
#ifdef SKIP_PEER_VERIFICATION
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif

        /* If the site you are connecting to uses a different host name that
         * what they have mentioned in their server certificate's commonName (or
         * subjectAltName) fields, libcurl will refuse to connect. You can skip
         * this check, but this will make the connection less secure. */
#ifdef SKIP_HOSTNAME_VERIFICATION
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif

        /* Note that this option is not strictly required, omitting it will
         * result in libcurl sending the MAIL FROM command with empty sender
         * data. All autoresponses should have an empty reverse-path, and should
         * be directed to the address in the reverse-path which triggered them.
         * Otherwise, they could cause an endless loop. See RFC 5321
         * Section 4.5.5 for more details.
         */
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, FROM_MAIL);

        /* Add two recipients, in this particular case they correspond to the
         * To: and Cc: addressees in the header, but they could be any kind of
         * recipient. */
        recipients = curl_slist_append(recipients, to_mail);
        // recipients = curl_slist_append(recipients, CC_MAIL);
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

        /* We are using a callback function to specify the payload (the headers
         * and body of the message). You could just use the CURLOPT_READDATA
         * option to specify a FILE pointer to read from. */
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
        curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

        /* Since the traffic will be encrypted, it is useful to turn on debug
         * information within libcurl to see what is happening during the
         * transfer */
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        /* Send the message */
        res = curl_easy_perform(curl);

        /* Check for errors */
        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));

        /* Free the list of recipients */
        curl_slist_free_all(recipients);

        /* Always cleanup */
        curl_easy_cleanup(curl);
    }

    return (int) res;
}
void curl_free_payload_text() { free(payload_text); }