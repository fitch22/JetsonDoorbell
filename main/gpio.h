// Logic Analyzer pins for debugging
#define LA0 4
#define LA1 5
#define LA2 6
#define LA3 7
#define LA_SEL ((1ULL << LA0) | (1ULL << LA1) | (1ULL << LA2) | (1ULL << LA3))

// Other GPIO
#define BUTTON_PIN 17                       // used for doorbell button
#define BUTTON_PIN_SEL (1ULL << BUTTON_PIN) // GPIO 10
#define BUTTON2_PIN 18
#define BUTTON2_PIN_SEL (1ULL << BUTTON2_PIN)
#define SD_MODE_PIN 14 // enable/disable for MAX98357A
#define SD_MODE_PIN_SEL (1ULL << SD_MODE_PIN)
#define CD_PIN 8 // sd card detect
#define CD_PIN_SEL (1ULL << CD_PIN)
#define BLINK_PIN 13 // Error LED
#define BLINK_PIN_SEL (1ULL << BLINK_PIN)

// SDIO
#define CLK_PIN 9
#define CMD_PIN 10
#define DAT0_PIN 1
#define DAT1_PIN 3
#define DAT2_PIN 12
#define DAT3_PIN 11

// I2S
#define BCLK_PIN 2
#define WS_PIN 38
#define DIN_PIN 21

void setup_gpio(void);
