static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  switch (event_id) {
    case SYSTEM_EVENT_STA_START:
      esp_wifi_connect();
      break;
    case WIFI_EVENT_STA_CONNECTED:
      wifi_connection_status = true;
      break;
  }
}

void setup_wifi() {
  esp_event_loop_create_default();

  ESP_ERROR_CHECK(esp_netif_init());
  esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
  esp_netif_dhcpc_stop(sta_netif);
  esp_netif_ip_info_t ip_info;

  ip_info.ip.addr = ipaddr_addr(IP);
  ip_info.gw.addr = ipaddr_addr(GATEWAY);
  ip_info.netmask.addr = ipaddr_addr(SUBNET);

  esp_netif_set_ip_info(sta_netif, &ip_info);

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  wifi_config_t wifi_config = {
    .sta = {
      .ssid = SECRET_SSID,
      .password = SECRET_PASS,
      .threshold.authmode = WIFI_AUTH_WPA2_PSK,
    },
  };
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
  esp_wifi_start();
}

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event) {
  switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
      mqtt_client_connected = true;
      break;
    case MQTT_EVENT_BEFORE_CONNECT:
      // ignore event
      break;
    case MQTT_EVENT_PUBLISHED:
      mqtt_message_id = event->msg_id;
      break;
    default:
      printf("Other event id:%d", event->event_id);
      break;
  }
  return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
  mqtt_event_handler_cb(event_data);
}

void setup_mqtt() {
  esp_mqtt_client_config_t mqtt_cfg = {
    .host = MQTT_SERVER,
    .port = 1883,
    .username = MQTT_USERNAME,
    .password = MQTT_PASSWORD,
    .client_id = HOSTNAME,
  };
  mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, mqtt_client);
  esp_mqtt_client_start(mqtt_client);
}


int connect_to_wifi() {
  wifi_connection_status = false;
  wifi_connection_time_amount = 0;
  while(1) {
    if (wifi_connection_status == true) {
      return 0;
    }
    if (wifi_connection_time_amount > 4000) {
#ifdef DEBUG
      printf("WIFI Connect did not happen, break free\n");
#endif
      break;
    }
    wifi_connection_time_amount += 10;
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }

  return -1;
}

int connect_to_mqtt() {
  mqtt_client_connected = false;
  mqtt_connection_time_amount = 0;
  setup_mqtt();
  while(1) {
    if (mqtt_client_connected == true) {
      return 0;
    }
    if (mqtt_connection_time_amount > 1500) {
#ifdef DEBUG
      printf("MQTT Connect does not happen, break free\n");
#endif
      break;
    }
    mqtt_connection_time_amount += 10;
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
  return -1;
}

int send_mqtt(char topic[], char data[]) {
  mqtt_message_id = -2;
  int msg_id = esp_mqtt_client_publish(mqtt_client, topic, data, strlen(data), 2, 1);
  int64_t mqtt_send = esp_timer_get_time();

  while(1) {
    if (mqtt_message_id == msg_id) {
#ifdef DEBUG
      printf("MQTT Message %d delivered\n", mqtt_message_id);
#endif
      return 1;
    }
    if (esp_timer_get_time() > mqtt_send + (1000 * 1000)) {
      return -2;
    }
  }
  return -1;
}
