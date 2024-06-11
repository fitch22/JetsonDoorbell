// Logic Analyzer pins for debugging
#define LA0 47
#define LA1 48
#define LA2 42
#define LA3 9
#define LA_SEL ((1ULL << LA0) | (1ULL << LA1) | (1ULL << LA2) | (1ULL << LA3))

// Other GPIO
#define BUTTON_PIN 10                       // used for doorbell button
#define BUTTON_PIN_SEL (1ULL << BUTTON_PIN) // GPIO 10
#define SD_MODE_PIN 11                      // enable/disable for MAX98357A
#define SD_MODE_PIN_SEL (1ULL << SD_MODE_PIN)
#define CD_PIN 17 // sd card detect
#define CD_PIN_SEL (1ULL << CD_PIN)
#define BLINK_PIN 18 // Error LED
#define BLINK_PIN_SEL (1ULL << BLINK_PIN)

// SDIO
#define CLK_PIN 7
#define CMD_PIN 6
#define DAT0_PIN 15
#define DAT1_PIN 16
#define DAT2_PIN 4
#define DAT3_PIN 5

// I2S
#define BCLK_PIN 13
#define WS_PIN 14
#define DIN_PIN 12

void setup_gpio(void);
