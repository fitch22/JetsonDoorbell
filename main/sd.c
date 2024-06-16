#include "sd.h"
#include "conf.h"
#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "global.h"
#include "gpio.h"
#include "sdmmc_cmd.h"
#include "tag.h"
#include <string.h>

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
  slot_config.clk = CLK_PIN;
  slot_config.cmd = CMD_PIN;
  slot_config.d0 = DAT0_PIN;
  slot_config.d1 = DAT1_PIN;
  slot_config.d2 = DAT2_PIN;
  slot_config.d3 = DAT3_PIN;
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
esp_err_t open_file(const char *filename, uint16_t doorbell) {
  volume = doorbell == 2 ? volume2 : volume1;

  // wait on semaphore
  // if (xSemaphoreTake(xPlay, 0)) {
  ESP_LOGI(TAG, "Opening file %s", filename);

  wave_header_t header;
  chunk_header_t ck_header;
  fmt_chunk_t fmt_chunk;
  size_t count;

  fp = fopen(filename, "r");
  if (fp == NULL) {
    ESP_LOGE(TAG, "Failed to open file for reading");
    return ESP_FAIL;
  }
  // Read header (RIFFxxxxWAVE)
  fread(&header, sizeof(wave_header_t), 1, fp);
  // verify id == "RIFF"
  if (strncmp(header.id, "RIFF", 4)) {
    ESP_LOGE(TAG, "ID of first chunk != \"RIFF\"");
    fclose(fp);
    return ESP_FAIL;
  }
  // verify format == "WAVE"
  if (strncmp(header.format, "WAVE", 4)) {
    ESP_LOGE(TAG, "Chunk format != \"WAVE\"");
    fclose(fp);
    return ESP_FAIL;
  }

  // look for "fmt " chunk, skip any others
  while (1) {
    fread(&ck_header, sizeof(chunk_header_t), 1, fp);
    if (strncmp(ck_header.id, "fmt ", 4)) {
      fseek(fp, ck_header.size, SEEK_CUR); // skip over data
      ESP_LOGI(TAG, "Searching for \"fmt \", skipping \"%.4s\"", ck_header.id);
    } else {
      ESP_LOGI(TAG, "Found fmt chunk, size = %lu", ck_header.size);
      break;
    }
  }
  // found "fmt ", now read the format information
  fread(&fmt_chunk, sizeof(fmt_chunk_t), 1, fp);
  // verify PCM
  if (fmt_chunk.audio_format != 1) {
    ESP_LOGE(TAG, "PCM only audio format supported");
    fclose(fp);
    return ESP_FAIL;
  }
  // verify mono or stereo
  if (fmt_chunk.num_of_channels > 2) {
    ESP_LOGE(TAG, "Number of channels supported is 1 or 2");
    fclose(fp);
    return ESP_FAIL;
  }
  // verify sample rate <= 48K
  if (fmt_chunk.sample_rate > 48000) {
    ESP_LOGE(TAG, "Unsupported sample rate %lud", fmt_chunk.sample_rate);
    fclose(fp);
    return ESP_FAIL;
  }
  // verify 16bit samples
  if (fmt_chunk.bits_per_sample != 16) {
    ESP_LOGE(TAG, "Bits per sample must be 16");
    fclose(fp);
    return ESP_FAIL;
  }
  ESP_LOGI(TAG, "no channels: %d, sample rate: %ld, bits: %d",
           fmt_chunk.num_of_channels, fmt_chunk.sample_rate,
           fmt_chunk.bits_per_sample);
  stereo = fmt_chunk.num_of_channels == 2;

  // look for "data" chunk, skip any others
  while (1) {
    fread(&ck_header, sizeof(chunk_header_t), 1, fp);
    if (strncmp(ck_header.id, "data", 4)) {
      fseek(fp, ck_header.size, SEEK_CUR); // skip over data
      ESP_LOGI(TAG, "Searching for \"data\", skipping \"%.4s\"", ck_header.id);
    } else {
      ESP_LOGI(TAG, "Found data chunk, size = %lu", ck_header.size);
      break;
    }
  }

  // ESP_LOGI(TAG, "File is %s", stereo ? "stereo" : "mono");

  // adjust clock rate
  i2s_std_clk_config_t tx_std_clk =
      I2S_STD_CLK_DEFAULT_CONFIG(fmt_chunk.sample_rate);
  i2s_channel_reconfig_std_clock(tx_chan, &tx_std_clk);
  ESP_LOGI(TAG, "Set clock rate to %d", (int)tx_std_clk.sample_rate_hz);

  // When the source is mono, we have to duplicate each sample
  uint16_t dma_buffer_size = stereo ? DMA_BUFF_SIZE : DMA_BUFF_SIZE / 2;

  ESP_LOGI(TAG, "dma_buffer_size: %d", (int)dma_buffer_size);

  // read 2 DMA buffers to pre-load I2S transmitter with
  count = fread(buffer, 1, dma_buffer_size, fp);
  if (count != dma_buffer_size) {
    ESP_LOGE(TAG, "First read only %d elements", count);
    return ESP_FAIL;
  }
  remaining_data = ck_header.size - count;

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
  remaining_data -= count;

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
  //}
  return ESP_OK; // make compiler happy
}
