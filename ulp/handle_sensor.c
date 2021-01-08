#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "ulp_riscv/ulp_riscv.h"
#include "ulp_riscv/ulp_riscv_utils.h"

#define HALL1_GPIO 4
#define HALL2_GPIO 5
#define HALL3_GPIO 6
#define HALL4_GPIO 7

uint8_t hall1_status_cur;
uint8_t hall2_status_cur;
uint8_t hall3_status_cur;

uint8_t hall1_status;
uint8_t hall2_status;
uint8_t hall3_status;

static inline uint8_t gpio_get_level(uint8_t gpio_num) {
  return (uint8_t)((REG_GET_FIELD(RTC_GPIO_IN_REG, RTC_GPIO_IN_NEXT) & BIT(gpio_num)) ? 1 : 0);
}

int main (void) {
  hall1_status_cur = gpio_get_level(HALL1_GPIO);
  if (hall1_status_cur != hall1_status) {
    hall1_status = hall1_status_cur;
    ulp_riscv_wakeup_main_processor();
  }

  hall2_status_cur = gpio_get_level(HALL2_GPIO);
  if (hall2_status_cur != hall2_status) {
    hall2_status = hall2_status_cur;
    ulp_riscv_wakeup_main_processor();
  }

  hall3_status_cur = gpio_get_level(HALL3_GPIO);
  if (hall3_status_cur != hall3_status) {
    hall3_status = hall3_status_cur;
    ulp_riscv_wakeup_main_processor();
  }

  return 0;
}
