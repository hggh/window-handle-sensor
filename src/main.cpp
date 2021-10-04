#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_sleep.h"
#include <driver/adc_common.h>
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_wifi.h"

#include <Arduino.h>

#include <WiFi.h>
#include <MQTT.h>

#define uS_TO_S_FACTOR 1000000

#define LED_GREEN GPIO_NUM_35
#define LED_RED GPIO_NUM_34
#define LED_BLUE GPIO_NUM_33
#define HALL_SENSOR_1 GPIO_NUM_4
#define HALL_SENSOR_2 GPIO_NUM_6
#define HALL_SENSOR_3 GPIO_NUM_7

#define DEFAULT_VREF    1100
#define NO_OF_SAMPLES   10

static esp_adc_cal_characteristics_t adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_7;
static const adc_bits_width_t width = ADC_WIDTH_BIT_13;
static const adc_atten_t atten = ADC_ATTEN_DB_11;

#define BATTERY_ENABLE_ADC GPIO_NUM_2
#define ADC_GPIO_PIN GPIO_NUM_8
//#define ADC_GPIO_PIN_ARDUINO 8
static float volt_r1 = 10000.0;
static float volt_r2 = 10000.0;

#define WINDOW_HANDLE_RIGHT 1
#define WINDOW_HANDLE_LEFT 2

#define WINDOW_STATUS_UNDEFINED 0
#define WINDOW_STATUS_CLOSED 2
#define WINDOW_STATUS_OPENED 4
#define WINDOW_STATUS_HALF_OPENED 6

#include "config.h"

/*
 * global vars
 */
RTC_DATA_ATTR unsigned int bootCount = 0;
MQTTClient mqtt;
WiFiClient net;
esp_sleep_source_t w_cause;

float get_battery_voltage() {
  adc_power_acquire();

  rtc_gpio_hold_dis(BATTERY_ENABLE_ADC);
  gpio_reset_pin(BATTERY_ENABLE_ADC);
  gpio_reset_pin(ADC_GPIO_PIN);

  gpio_set_direction(BATTERY_ENABLE_ADC, GPIO_MODE_OUTPUT);
  gpio_set_direction(ADC_GPIO_PIN, GPIO_MODE_INPUT);
  gpio_set_level(BATTERY_ENABLE_ADC, 1);

  adc1_config_width(width);
  adc1_config_channel_atten((adc1_channel_t)channel, atten);

  esp_adc_cal_characterize(ADC_UNIT_1, atten, width, DEFAULT_VREF, &adc_chars);

  uint32_t adc_reading = 0;
  for (int i = 0; i < NO_OF_SAMPLES; i++) {
    adc_reading += adc1_get_raw((adc1_channel_t)channel);
  }
  adc_reading /= NO_OF_SAMPLES;
  // get mV
  uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, &adc_chars);
  float battery_voltage = (voltage / 1000.0) / (volt_r2 / ( volt_r1 + volt_r2));

  gpio_set_level(BATTERY_ENABLE_ADC, 0);
  adc_power_release();
  return battery_voltage;
}


void connect_mqtt() {
  mqtt.connect(HOSTNAME, MQTT_USERNAME, MQTT_PASSWORD);
}

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

bool send_window_status(int window_status) {
  if (mqtt.publish(String("/ws/" + String(MQTT_TOPIC_ID) + "/state").c_str(), String(window_status).c_str(), false, 2)) {
    led_on(LED_GREEN);
    delay(5);
    return true;
  }
  return false;
}

bool send_boot_count(unsigned int boot_count) {
  return mqtt.publish(String("/ws/" + String(MQTT_TOPIC_ID) + "/boot_count").c_str(), String(boot_count).c_str(), false, 2);
}

bool send_battery_voltage(float battery_voltage) {
  return mqtt.publish(String("/ws/" + String(MQTT_TOPIC_ID) + "/battery").c_str(), String(battery_voltage).c_str(), false, 2);
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

void goto_deep_sleep() {
  mqtt.disconnect();
#ifdef DEBUG
  Serial.println("Going to deep sleep....");
#endif

  led_off(LED_RED);
  led_off(LED_GREEN);
  led_off(LED_BLUE);

  rtc_gpio_isolate(HALL_SENSOR_1);
  rtc_gpio_isolate(HALL_SENSOR_2);
  rtc_gpio_isolate(HALL_SENSOR_3);
  rtc_gpio_isolate(BATTERY_ENABLE_ADC);
  rtc_gpio_isolate(ADC_GPIO_PIN);

  esp_wifi_stop();
  adc_power_off();
  esp_sleep_enable_timer_wakeup((uint64_t)(60 * 60 * 24) * (uint64_t)uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void setup() {
#ifdef DEBUG
  Serial.begin(115200);
#endif
  ++bootCount;
  w_cause = esp_sleep_get_wakeup_cause();
  esp_deep_sleep_disable_rom_logging();
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

  if (w_cause != ESP_SLEEP_WAKEUP_TIMER) {
    led_on(LED_BLUE);
  }

  setup_leds();
  setup_hall_sensor_pins();

  WiFi.setHostname(HOSTNAME);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SECRET_SSID, SECRET_PASS);

  mqtt.begin(MQTT_SERVER, net);

  int wifi_conn_status = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1);
    wifi_conn_status++;

    if (wifi_conn_status > 4000) {
#ifdef DEBUG
      Serial.println("Could not connect to wifi after 4000ms ... restart");
#endif
      ESP.restart();
    }
  }
  led_off(LED_BLUE);
  connect_mqtt();

  uint8_t window_state = get_window_status();
#ifdef DEBUG
    Serial.print("Window State is: ");
    Serial.println(window_state);
    Serial.print("Boot count: ");
    Serial.println(bootCount);
#endif
  if (send_window_status(window_state) == false) {
    led_on(LED_RED);
    delay(10);
    send_window_status(window_state);
  }
  send_boot_count(bootCount);
  send_battery_voltage(get_battery_voltage());
  mqtt.loop();

  if (set_deep_sleep_wakeup() == 0) {
    goto_deep_sleep();
  }
}

void loop() {
  led_off(LED_BLUE);
  led_off(LED_GREEN);
  led_on(LED_RED);
  gpio_wakeup_enable(HALL_SENSOR_1, gpio_get_level(HALL_SENSOR_1) == 1 ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);
  gpio_wakeup_enable(HALL_SENSOR_2, gpio_get_level(HALL_SENSOR_2) == 1 ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);
  gpio_wakeup_enable(HALL_SENSOR_3, gpio_get_level(HALL_SENSOR_3) == 1 ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);
  esp_wifi_stop();
  esp_sleep_enable_gpio_wakeup();
  esp_light_sleep_start();
      
  esp_wifi_start();
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

  uint8_t window_state = get_window_status();
  int wifi_conn_status = 0;
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(SECRET_SSID, SECRET_PASS);
  }
  while (WiFi.status() != WL_CONNECTED) {
    delay(1);
    wifi_conn_status++;
    if (wifi_conn_status > 4000) {
#ifdef DEBUG
      Serial.println("Could not connect to wifi after 4000ms ... restart");
#endif
      ESP.restart();
    }
  }
  led_off(LED_BLUE);
  connect_mqtt();
  mqtt.loop();

  if (send_window_status(window_state) == false) {
    delay(10);
    send_window_status(window_state);
  }
  send_boot_count(bootCount);
  if (set_deep_sleep_wakeup() == 0) {
    goto_deep_sleep();
  }
}
