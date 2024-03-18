#include "conf.h"
#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

char wifi_ssid[32];
char wifi_pass[64];
char hostname[16];

TaskHandle_t xFillTask = NULL;   // handle for fill task
TaskHandle_t xButtonTask = NULL; // handle for doorbell button task
SemaphoreHandle_t xPlay = NULL;  // Semaphore to allow only one tune to play

// The DMA buffer is 4092 bytes (not 4096)
// The I2S component keeps track of bytes, so should we
int16_t buff[DMA_BUFF_SIZE / 2];   // 16 bits aligned on 16 bit boundaries
uint8_t *buffer = (uint8_t *)buff; // buffer to store WAV file segments
                                   // (L+R samples, 16 bits each)
bool at_eof = false;
bool last_fill = false;

bool stereo = true;
uint32_t remaining_data; // wave file data left to read

FILE *fp;

i2s_chan_handle_t tx_chan; // I2S tx channel handle

// These are the currently selected tune file and volume
char tune[64];
char vol[5];
float volume = 0.2;
