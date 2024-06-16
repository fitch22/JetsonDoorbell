#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

extern char wifi_ssid[];
extern char wifi_pass[];
extern char hostname[16];

extern TaskHandle_t xFillTask;    // handle for fill task
extern TaskHandle_t xButton1Task; // handle for doorbell button1 task
extern TaskHandle_t xButton2Task; // handle for doorbell button2 task
extern SemaphoreHandle_t xPlay;   // Semaphore to allow only one tune to play

extern int16_t buff[];  // 16 bits aligned on 16 bit boundaries
extern uint8_t *buffer; // buffer to store WAV file segments
                        // (L+R samples, 16 bits each)
extern bool at_eof;
extern bool last_fill;
extern bool stereo;
extern uint32_t remaining_data;
extern FILE *fp;
extern i2s_chan_handle_t tx_chan; // I2S tx channel handle
extern char tune1[64];
extern char tune2[64];
extern char vol1[5];
extern char vol2[5];
extern float volume;
extern float volume1;
extern float volume2;
