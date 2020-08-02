#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
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
#define WINDOW_STATUS_CLOSED 1
#define WINDOW_STATUS_OPENED 2
#define WINDOW_STATUS_HALF_OPENED 3

const uint8_t window_handle = WINDOW_HANDLE_RIGHT;

double get_window_status() {
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
  printf("%d - %d - %d : %d \n", ulp_hall1_status & UINT16_MAX, ulp_hall2_status & UINT16_MAX, ulp_hall3_status & UINT16_MAX, ulp_event_counter & UINT16_MAX);

  double window_state = get_window_status();
  if (window_state == WINDOW_STATUS_CLOSED) {
    led_on(LED_D1);
  }
  if (window_state == WINDOW_STATUS_OPENED) {
    led_on(LED_D2);
  }
  if (window_state == WINDOW_STATUS_HALF_OPENED) {
    led_on(LED_D3);
  }
  if (window_state == WINDOW_STATUS_UNDEFINED) {
    led_on(LED_D4);
  }

  vTaskDelay(500 / portTICK_PERIOD_MS);

  // switch off all leds before sleep
  led_off(LED_D1);
  led_off(LED_D2);
  led_off(LED_D3);
  led_off(LED_D4);

  printf("Entering deep sleep\n\n");
  esp_sleep_enable_ulp_wakeup();
  esp_deep_sleep_start();
}
