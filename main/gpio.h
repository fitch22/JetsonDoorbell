// These outputs are just for debugging
#ifdef HW_PROFILE
#define GPIO_OUTPUT_IO_47 47
#define GPIO_OUTPUT_IO_48 48
#define GPIO_OUTPUT_IO_42 42
#define GPIO_OUTPUT_IO_9 9
#define GPIO_OUTPUT_PIN_SEL                                                    \
  ((1ULL << GPIO_OUTPUT_IO_47) | (1ULL << GPIO_OUTPUT_IO_48) |                 \
   (1ULL << GPIO_OUTPUT_IO_42) | (1ULL << GPIO_OUTPUT_IO_9))
#endif
#define BUTTON_PIN 10                       // used for doorbell button
#define BUTTON_PIN_SEL (1ULL << BUTTON_PIN) // GPIO 10
#define SD_MODE_PIN 11                      // enable/disable for MAX98357A
#define SD_MODE_PIN_SEL (1ULL << SD_MODE_PIN)
#define CD_PIN 17 // sd card detect
#define CD_PIN_SEL (1ULL << CD_PIN)
#define BLINK_PIN 18 // Error LED
#define BLINK_PIN_SEL (1ULL << BLINK_PIN)

void setup_gpio(void);
