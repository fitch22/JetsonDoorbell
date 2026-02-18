#include "driver/gpio.h"
#include "error_blink.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "global.h"
#include "gpio.h"
#include "i2s.h"
#include "net.h"
#include "sd.h"
#include "sdkconfig.h"
#include "task.h"
#include "wifi.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern SemaphoreHandle_t xPlay;

void app_main(void) {
  setup_gpio();

  gpio_set_level(BLINK_PIN, 1);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  gpio_set_level(BLINK_PIN, 0);

  // if there is no SD card, no point in proceeding
  if (sd_setup() != ESP_OK)
    error_blink(1);

  i2s_setup();

  if (restore_conf() != ESP_OK)
    error_blink(2);

  xTaskCreate(dma_buffer_fill_task, "Fill Task", 4096, NULL, 10, &xFillTask);
  xPlay = xSemaphoreCreateBinary();
  xSemaphoreGive(xPlay);
  xTaskCreate(button1_play_task, "Button1 Task", 4096, NULL, 10, &xButton1Task);
  xTaskCreate(button2_play_task, "Button2 Task", 4096, NULL, 10, &xButton2Task);

  if (wifi_init(wifi_ssid, wifi_pass) != ESP_OK)
    error_blink(3);

  if (start_webserver() != ESP_OK)
    error_blink(4);
  // app_main can return; httpd and all tasks continue running
}
