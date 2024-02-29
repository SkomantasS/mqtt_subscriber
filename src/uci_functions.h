unsigned char *hash_strings(char **strings, int num_strings);
int uci_mqtt_data(struct mqtt_connection_settings *mqtt_conn_set);
int uci_actions_data(struct mqtt_connection_settings *mqtt_conn_set);
void uci_unload_and_free_context();
struct uci_section *uci_unnamed_section_get(char *pkg_name, char *name);