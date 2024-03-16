#include "isr.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "global.h"
#include "gpio.h"
#include <stdlib.h>

// Interrupt service routine, called after sending each DMA buffer
bool i2s_write_callback(i2s_chan_handle_t handle, i2s_event_data_t *event,
                        void *user_ctx) {
#ifdef HW_PROFILE
  gpio_set_level(47, 1);
  gpio_set_level(47, 0);
#endif
  vTaskNotifyGiveFromISR(xFillTask, NULL);

  return true;
}

void button_isr_handler(void *arg) {
  vTaskNotifyGiveFromISR(xButtonTask, NULL);
  gpio_intr_disable(BUTTON_PIN);
}
