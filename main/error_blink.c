// blinks error code on LED
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gpio.h"
#include <stdint.h>

void error_blink(uint8_t code) {
  for (;;) {
    vTaskDelay(3000 / portTICK_PERIOD_MS); // 3 seconds between codes
    for (uint8_t i = 0; i < code; i++) {
      gpio_set_level(BLINK_PIN, 1);
      vTaskDelay(500 / portTICK_PERIOD_MS);
      gpio_set_level(BLINK_PIN, 0);
      vTaskDelay(500 / portTICK_PERIOD_MS);
    }
  }
}