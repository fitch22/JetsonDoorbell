#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

extern char wifi_ssid[];
extern char wifi_pass[];
extern char hostname[16];

extern TaskHandle_t xFillTask;   // handle for fill task
extern TaskHandle_t xButtonTask; // handle for doorbell button task
extern SemaphoreHandle_t xPlay;  // Semaphore to allow only one tune to play

extern int16_t buff[];  // 16 bits aligned on 16 bit boundaries
extern uint8_t *buffer; // buffer to store WAV file segments
                        // (L+R samples, 16 bits each)
extern bool at_eof;
extern bool last_fill;
extern bool stereo;
extern FILE *fp;
extern i2s_chan_handle_t tx_chan; // I2S tx channel handle
extern char tune[64];
extern char vol[5];
extern float volume;
