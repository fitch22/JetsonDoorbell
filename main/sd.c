#include "sd.h"
#include "conf.h"
#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "format_wav.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "global.h"
#include "gpio.h"
#include "sdmmc_cmd.h"
#include "tag.h"

esp_err_t sd_setup(void) {

  // Check if an SD card is present
  if (gpio_get_level(CD_PIN)) {
    ESP_LOGE(TAG, "No SD card present");
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "SD Setup()");

  // Mount filesystem
  esp_vfs_fat_mount_config_t mount_config = {.format_if_mount_failed = false,
                                             .allocation_unit_size = 16 * 1024,
                                             .max_files = 20,
                                             .disk_status_check_enable = true};

  sdmmc_card_t *card; // sd card info
  const char mount_point[] = MOUNT_POINT;

  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  host.max_freq_khz = SDMMC_FREQ_HIGHSPEED; // best we can get is 40MHz

  // This initializes the slot without card detect (CD) and write protect (WP)
  // signals. Modify slot_config.gpio_cd and slot_config.gpio_wp if your board
  // has these signals.
  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  slot_config.width = 4;
  slot_config.clk = 7; // 6;
  slot_config.cmd = 6; // 7;
  slot_config.d0 = 15; // 5;
  slot_config.d1 = 16; // 4;
  slot_config.d2 = 4;  // 16;
  slot_config.d3 = 5;  // 15;
  slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

  ESP_LOGI(TAG, "Mounting filesystem");

  // Mount filesystem
  ESP_ERROR_CHECK(esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config,
                                          &mount_config, &card));
  ESP_LOGI(TAG, "Filesystem mounted");
  return ESP_OK;
}

// Opens the file, verifies the header to be the right file type, pre-loads the
// first 2 DMA buffers
esp_err_t open_file(const char *filename) {
  // wait on semaphore
  if (xSemaphoreTake(xPlay, 0)) {
    ESP_LOGI(TAG, "Opening file %s", filename);

    wav_header_t header;
    size_t count;

    fp = fopen(filename, "r");
    if (fp == NULL) {
      ESP_LOGE(TAG, "Failed to open file for reading");
      return ESP_FAIL;
    }
    count = fread(&header, sizeof(wav_header_t), 1, fp);
    ESP_LOGI(TAG, "no channels: %d, sample rate: %ld, bits: %d",
             header.fmt_chunk.num_of_channels, header.fmt_chunk.sample_rate,
             header.fmt_chunk.bits_per_sample);

    stereo = header.fmt_chunk.num_of_channels == 2;
    ESP_LOGI(TAG, "File is %s", stereo ? "stereo" : "mono");
    // adjust clock rate
    i2s_std_clk_config_t tx_std_clk =
        I2S_STD_CLK_DEFAULT_CONFIG(header.fmt_chunk.sample_rate);
    i2s_channel_reconfig_std_clock(tx_chan, &tx_std_clk);
    ESP_LOGI(TAG, "New clock rate is %d", (int)tx_std_clk.sample_rate_hz);

    // When the source is mono, we have to duplicate each sample
    uint16_t dma_buffer_size = stereo ? DMA_BUFF_SIZE : DMA_BUFF_SIZE / 2;

    ESP_LOGI(TAG, "dma_buffer_size: %d", (int)dma_buffer_size);

    // read 2 DMA buffers to pre-load I2S transmitter with
    count = fread(buffer, 1, dma_buffer_size, fp);
    if (count != dma_buffer_size) {
      ESP_LOGE(TAG, "First read only %d elements", count);
      return ESP_FAIL;
    }

    // duplicate if mono
    if (!stereo) {
      for (uint16_t i = 0; i < dma_buffer_size / 2; i++) {
        buff[dma_buffer_size - 1 - (2 * i)] =
            buff[dma_buffer_size - 2 - (2 * i)] =
                buff[(dma_buffer_size / 2) - 1 - i] * volume;
      }
    } else {
      for (uint16_t i = 0; i < (dma_buffer_size / 2); i++)
        buff[i] = (int16_t)(buff[i] * volume);
    }
    ESP_LOGI(TAG, "First read OK");
    ESP_ERROR_CHECK(
        i2s_channel_preload_data(tx_chan, buffer, DMA_BUFF_SIZE, &count));
    if (count != DMA_BUFF_SIZE) {
      ESP_LOGE(TAG, "First preload only %d bytes", count);
      return ESP_FAIL;
    }
    ESP_LOGI(TAG, "First pre-load OK");
    count = fread(buffer, 1, dma_buffer_size, fp);
    if (count != dma_buffer_size) {
      ESP_LOGE(TAG, "Second read only %d elements", count);
      return ESP_FAIL;
    }
    // duplicate if mono
    if (!stereo) {
      for (uint16_t i = 0; i < dma_buffer_size / 2; i++) {
        buff[dma_buffer_size - 1 - (2 * i)] =
            buff[dma_buffer_size - 2 - (2 * i)] =
                buff[(dma_buffer_size / 2) - 1 - i] * volume;
      }
    } else {
      for (uint16_t i = 0; i < (dma_buffer_size / 2); i++)
        buff[i] = (int16_t)(buff[i] * volume);
    }
    ESP_LOGI(TAG, "Second read OK");
    ESP_ERROR_CHECK(
        i2s_channel_preload_data(tx_chan, buffer, DMA_BUFF_SIZE, &count));
    if (count != DMA_BUFF_SIZE) {
      ESP_LOGE(TAG, "Second preload only %d bytes", count);
      return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Second pre-load OK");

    // turn on MAX98357A
    ESP_LOGI(TAG, "Enabling MAX98357A");
    gpio_set_level(SD_MODE_PIN, 1);
    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
    return ESP_OK;
  }
  return ESP_OK; // make compiler happy
}
