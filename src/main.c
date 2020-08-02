#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_sleep.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp32/ulp.h"
#include "ulp_main.h"
#include "mqtt_client.h"

#include "lwip/err.h"
#include "lwip/sys.h"

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_main_bin_end");

#define LED_D1 GPIO_NUM_19
#define LED_D2 GPIO_NUM_18
#define LED_D3 GPIO_NUM_17
#define LED_D4 GPIO_NUM_16

#define WINDOW_HANDLE_RIGHT 1
#define WINDOW_HANDLE_LEFT 2

#define WINDOW_STATUS_UNDEFINED 0
#define WINDOW_STATUS_CLOSED 2
#define WINDOW_STATUS_OPENED 4
#define WINDOW_STATUS_HALF_OPENED 6

#include "config.h"

volatile bool wifi_connection_status = false;
int wifi_connection_time_amount = 0;

volatile bool mqtt_client_connected = false;
int mqtt_connection_time_amount = 0;
esp_mqtt_client_handle_t mqtt_client;
volatile int mqtt_message_id = 0;

uint8_t get_window_status() {
  uint8_t hall1_status = ulp_hall1_status & UINT16_MAX;
  uint8_t hall2_status = ulp_hall2_status & UINT16_MAX;
  uint8_t hall3_status = ulp_hall3_status & UINT16_MAX;

  if (window_handle == WINDOW_HANDLE_RIGHT) {
    if (hall1_status == 0 && hall2_status == 1 && hall3_status == 1) {
      return WINDOW_STATUS_CLOSED;
    }
    if (hall1_status == 1 && hall2_status == 0 && hall3_status == 1) {
      return WINDOW_STATUS_OPENED;
    }
    if (hall1_status == 1 && hall2_status == 1 && hall3_status == 0) {
      return WINDOW_STATUS_HALF_OPENED;
    }
  }
  else {
    if (hall1_status == 1 && hall2_status == 1 && hall3_status == 0) {
      return WINDOW_STATUS_CLOSED;
    }
    if (hall1_status == 1 && hall2_status == 0 && hall3_status == 1) {
      return WINDOW_STATUS_OPENED;
    }
    if (hall1_status == 0 && hall2_status == 1 && hall3_status == 1) {
      return WINDOW_STATUS_HALF_OPENED;
    }
  }
  return WINDOW_STATUS_UNDEFINED;
}

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

  tcpip_adapter_init();
  tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);
  tcpip_adapter_ip_info_t ip_info;
  ip_info.ip.addr = ipaddr_addr(IP);
  ip_info.gw.addr = ipaddr_addr(GATEWAY);
  ip_info.netmask.addr = ipaddr_addr(SUBNET);

  tcpip_adapter_set_ip_info(WIFI_IF_STA, &ip_info);

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  wifi_config_t wifi_config = {
    .sta = {
      .ssid = SECRET_SSID,
      .password = SECRET_PASS,
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

void setup_leds() {
  gpio_reset_pin(LED_D1);
  gpio_set_direction(LED_D1, GPIO_MODE_OUTPUT);

  gpio_reset_pin(LED_D2);
  gpio_set_direction(LED_D2, GPIO_MODE_OUTPUT);

  gpio_reset_pin(LED_D3);
  gpio_set_direction(LED_D3, GPIO_MODE_OUTPUT);

  gpio_reset_pin(LED_D4);
  gpio_set_direction(LED_D4, GPIO_MODE_OUTPUT);
}

void led_on(gpio_num_t led) {
  gpio_set_level(led, 1);
}

void led_off(gpio_num_t led) {
  gpio_set_level(led, 0);
}

static void init_ulp_program() {
  esp_err_t err = ulp_load_binary(0, ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
  ESP_ERROR_CHECK(err);

  gpio_num_t gpio_hall1 = GPIO_NUM_26;
  gpio_num_t gpio_hall2 = GPIO_NUM_33;
  gpio_num_t gpio_hall3 = GPIO_NUM_32;

//    GPIO 26 - RTC 7 - Hall 1
//    GPIO 33 - RTC 8 - Hall 2
//    GPIO 32 - RTC 9 - Hall 3

  rtc_gpio_init(gpio_hall1);
  rtc_gpio_set_direction(gpio_hall1, RTC_GPIO_MODE_INPUT_ONLY);
  rtc_gpio_pulldown_dis(gpio_hall1);
  rtc_gpio_pullup_dis(gpio_hall1);
  rtc_gpio_hold_en(gpio_hall1);
  rtc_gpio_isolate(GPIO_NUM_26);

  rtc_gpio_init(gpio_hall2);
  rtc_gpio_set_direction(gpio_hall2, RTC_GPIO_MODE_INPUT_ONLY);
  rtc_gpio_pulldown_dis(gpio_hall2);
  rtc_gpio_pullup_dis(gpio_hall2);
  rtc_gpio_hold_en(gpio_hall2);
  rtc_gpio_isolate(GPIO_NUM_33);

  rtc_gpio_init(gpio_hall3);
  rtc_gpio_set_direction(gpio_hall3, RTC_GPIO_MODE_INPUT_ONLY);
  rtc_gpio_pulldown_dis(gpio_hall3);
  rtc_gpio_pullup_dis(gpio_hall3);
  rtc_gpio_hold_en(gpio_hall3);
  rtc_gpio_isolate(GPIO_NUM_32);

  ulp_set_wakeup_period(0, 20 * 1000); // 20 ms
  err = ulp_run(&ulp_entry - RTC_SLOW_MEM);
  ESP_ERROR_CHECK(err);
}

void set_led_by_window_status() {
  led_off(LED_D1);
  led_off(LED_D2);
  led_off(LED_D3);

  uint8_t window_state = get_window_status();
  if (window_state == WINDOW_STATUS_CLOSED) {
    led_on(LED_D1);
  }
  if (window_state == WINDOW_STATUS_OPENED) {
    led_on(LED_D2);
  }
  if (window_state == WINDOW_STATUS_HALF_OPENED) {
    led_on(LED_D3);
  }
}

int connect_to_wifi() {
  wifi_connection_status = false;
  wifi_connection_time_amount = 0;
  setup_wifi();
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

int send_window_status(int window_status) {

  if (connect_to_wifi() != 0) {
    // TODO: write error log and send if next time?!?
    return -1;
  }

#ifdef DEBUG
  printf("Wifi connection time was: %d\n", wifi_connection_time_amount);
#endif

  if (connect_to_mqtt() != 0) {
    // TODO: write error log and send if next time?!?
    return -1;
  }

#ifdef DEBUG
  printf("MQTT connection time was: %d\n", mqtt_connection_time_amount);
#endif
  

  char mqtt_data[10];
  sprintf(mqtt_data, "%d", window_status);
  char mqtt_topic[40];
  sprintf(mqtt_topic, "/ws/%s/status", HOSTNAME);
  
  mqtt_message_id = -2;
  int msg_id = esp_mqtt_client_publish(mqtt_client, mqtt_topic, mqtt_data, strlen(mqtt_data), 2, 1);
  while(1) {
    if (mqtt_message_id == msg_id) {
#ifdef DEBUG
      printf("MQTT Message %d delivered\n", mqtt_message_id);
#endif
      break;
    }
    // TODO: timeout, move to extra method for sending also battery status
  }

  return 0;
}

void app_main() {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // suppress boot message
  esp_deep_sleep_disable_rom_logging();

  // setup leds
  setup_leds();

  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  if (cause != ESP_SLEEP_WAKEUP_ULP) {
    printf("Not ULP wakeup, initializing ULP\n");
    init_ulp_program();
  }
  set_led_by_window_status();

  int current_window_status = get_window_status();
  send_window_status(current_window_status);


  if (current_window_status != get_window_status()) {
    // TODO: MEH! Window status changed while sending?!?
#ifdef DEBUG
    printf("[WARNING] Window Status changed while sending: %d => %d\n", current_window_status, get_window_status());
#endif
  }


  // switch off all leds before sleep
  led_off(LED_D1);
  led_off(LED_D2);
  led_off(LED_D3);
  led_off(LED_D4);

  printf("Entering deep sleep\n\n");
  esp_sleep_enable_ulp_wakeup();
  esp_deep_sleep_start();
}
