#include "driver/gpio.h"
#include "gpio.h"
#include "isr.h"

void setup_gpio(void) {
  gpio_config_t io_conf = {};

#ifdef HW_PROFILE
  // GPIO outputs
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 0;
  gpio_config(&io_conf);
#endif

  // SD_MODE is open drain
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT_OD;
  io_conf.pin_bit_mask = SD_MODE_PIN_SEL;
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 0;
  gpio_config(&io_conf);

  // CD is input with pullup
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = CD_PIN_SEL;
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 1;
  gpio_config(&io_conf);

  // CD is input with pullup
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = BLINK_PIN_SEL;
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 0;
  gpio_config(&io_conf);

  // INT pin is active low
  io_conf.intr_type = GPIO_INTR_NEGEDGE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = BUTTON_PIN_SEL;
  io_conf.pull_up_en = 1;
  gpio_config(&io_conf);

  // install gpio isr service
  gpio_install_isr_service(0);
  // hook isr handler for specific gpio pin
  gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, NULL);
}
