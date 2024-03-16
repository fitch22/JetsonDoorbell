#include "esp_check.h"

#define MOUNT_POINT "/sdcard"

esp_err_t sd_setup(void);
esp_err_t open_file(const char *filename);
