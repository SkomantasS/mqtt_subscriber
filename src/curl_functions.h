void curl_payload_text_set(char *Topic, char *Parameter, char *Value, char *to_mail, char *from_mail);
int curl_send_email(char *email_username, char *email_smtps, char *email_secret, char *to_mail);
void curl_free_payload_text();