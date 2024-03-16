#include "driver/gpio.h"
#include "error_blink.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "global.h"
#include "gpio.h"
#include "i2s.h"
#include "mongoose.h"
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
  xTaskCreate(button_play_task, "Button Task", 4096, NULL, 10, &xButtonTask);

  if (wifi_init(wifi_ssid, wifi_pass) != ESP_OK)
    error_blink(3);

  // Connected to WiFi, now start HTTP server
  struct mg_mgr mgr;
  mg_log_set(MG_LL_DEBUG); // Set log level
  mg_mgr_init(&mgr);
  MG_INFO(("Mongoose version : v%s", MG_VERSION));
  MG_INFO(("Listening on     : %s", HTTP_URL));

  mg_http_listen(&mgr, HTTP_URL, cb, NULL);

  for (;;)
    mg_mgr_poll(&mgr, 1000); // Infinite event loop
}
