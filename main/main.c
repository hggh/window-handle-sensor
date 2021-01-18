#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/rtc_cntl_reg.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "driver/rtc_cntl.h"
#include "esp_sleep.h"
#include "esp_wifi.h"
#include "mqtt_client.h"
#include <driver/adc_common.h>

#define uS_TO_S_FACTOR 1000000

#define HALL_SENSOR_1 GPIO_NUM_4
#define HALL_SENSOR_2 GPIO_NUM_6
#define HALL_SENSOR_3 GPIO_NUM_7

#define BATTERY_ENABLE_ADC GPIO_NUM_2

#define LED_GREEN GPIO_NUM_35
#define LED_RED GPIO_NUM_34
#define LED_BLUE GPIO_NUM_33

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

#include "wifi_mqtt.h"

void setup_hall_sensor_pins() {
  rtc_gpio_deinit(HALL_SENSOR_1);
  gpio_pullup_dis(HALL_SENSOR_1);
  gpio_pulldown_dis(HALL_SENSOR_1);
  gpio_set_direction(HALL_SENSOR_1, GPIO_MODE_INPUT);

  rtc_gpio_deinit(HALL_SENSOR_2);
  gpio_pullup_dis(HALL_SENSOR_2);
  gpio_pulldown_dis(HALL_SENSOR_2);
  gpio_set_direction(HALL_SENSOR_2, GPIO_MODE_INPUT);

  rtc_gpio_deinit(HALL_SENSOR_3);
  gpio_pullup_dis(HALL_SENSOR_3);
  gpio_pulldown_dis(HALL_SENSOR_3);
  gpio_set_direction(HALL_SENSOR_3, GPIO_MODE_INPUT);
}

uint8_t get_window_status() {
  uint8_t hall1_status = gpio_get_level(HALL_SENSOR_1);
  uint8_t hall2_status = gpio_get_level(HALL_SENSOR_2);
  uint8_t hall3_status = gpio_get_level(HALL_SENSOR_3);

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

void setup_leds() {
  gpio_set_direction(LED_GREEN, GPIO_MODE_OUTPUT);

  gpio_set_direction(LED_RED, GPIO_MODE_OUTPUT);

  gpio_set_direction(LED_BLUE, GPIO_MODE_OUTPUT);
}

void led_on(gpio_num_t led) {
  gpio_set_level(led, 1);
}

void led_off(gpio_num_t led) {
  gpio_set_level(led, 0);
}

void set_led_by_window_status() {
  led_off(LED_GREEN);
  led_off(LED_BLUE);

  uint8_t window_state = get_window_status();
  if (window_state == WINDOW_STATUS_CLOSED) {
    led_on(LED_GREEN);
    return;
  }
  if (window_state == WINDOW_STATUS_OPENED) {
    led_on(LED_GREEN);
    return;
  }
  if (window_state == WINDOW_STATUS_HALF_OPENED) {
    led_on(LED_GREEN);
    return;
  }
  // handle has no correct position, light blue
  led_on(LED_BLUE);
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
  sprintf(mqtt_topic, "/ws/%s/state", HOSTNAME);
  int s = send_mqtt(mqtt_topic, mqtt_data);
  if (s == 1) {
    return 1;
  }

  return -1;
}

int set_deep_sleep_wakeup() {
  gpio_num_t gpio_current_low;
  if (gpio_get_level(HALL_SENSOR_1) == 0) {
    gpio_current_low = HALL_SENSOR_1;
  }
  else if (gpio_get_level(HALL_SENSOR_2) == 0) {
    gpio_current_low = HALL_SENSOR_2;
  }
  else if (gpio_get_level(HALL_SENSOR_3) == 0) {
    gpio_current_low = HALL_SENSOR_3;
  }
  else {
    return -1;
  }

  esp_sleep_enable_ext0_wakeup(gpio_current_low, 1);
  
  return 0;
}

void app_main(void)
{
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  esp_deep_sleep_disable_rom_logging();
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

  setup_leds();
  setup_hall_sensor_pins();

  while(true) {
    setup_wifi();
    set_led_by_window_status();
    uint8_t window_state = get_window_status();

#ifdef DEBUG
    printf("Window State is: %d\n", window_state);
#endif
    send_window_status(window_state);

    if (set_deep_sleep_wakeup() == 0) {
      break;
    }
    else {
#ifdef DEBUG
      printf("No deep sleep wakeup source can be used, because state is unkown, go into lightsleep..\n");
      vTaskDelay(20 / portTICK_RATE_MS);
#endif    
      gpio_wakeup_enable(HALL_SENSOR_1, gpio_get_level(HALL_SENSOR_1) == 1 ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);
      gpio_wakeup_enable(HALL_SENSOR_2, gpio_get_level(HALL_SENSOR_2) == 1 ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);
      gpio_wakeup_enable(HALL_SENSOR_3, gpio_get_level(HALL_SENSOR_3) == 1 ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);
      esp_wifi_stop();
      adc_power_off();
      esp_sleep_enable_gpio_wakeup();
      esp_light_sleep_start();
     
      // wakeup from light sleep
      esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    }
  }

  led_off(LED_RED);
  led_off(LED_GREEN);
  led_off(LED_BLUE);

  rtc_gpio_isolate(HALL_SENSOR_1);
  rtc_gpio_isolate(HALL_SENSOR_2);
  rtc_gpio_isolate(HALL_SENSOR_3);

  esp_mqtt_client_stop(mqtt_client);
  esp_wifi_stop();
  adc_power_off();

  esp_sleep_enable_timer_wakeup((uint64_t)(60 * 60 * 24) * (uint64_t)uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

