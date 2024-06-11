#include "conf.h"
#include "driver/gpio.h"
#include "global.h"
#include "gpio.h"
#include "sd.h"
#include "tag.h"

void dma_buffer_fill_task(void *pvParameters) {
  for (;;) {
    // Block and wait for interrupt
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    if (at_eof && last_fill) {
      ESP_LOGI(TAG, "All done!");
      at_eof = false;
      last_fill = false;
      ESP_ERROR_CHECK(i2s_channel_disable(tx_chan));
      fclose(fp);
      ESP_LOGI(TAG, "Shutting down MAX98357A");
      gpio_set_level(SD_MODE_PIN, 0); // turn off MAX98357A
      gpio_intr_enable(BUTTON_PIN);
      xSemaphoreGive(xPlay);
    } else if (at_eof) {
      ESP_LOGI(TAG, "Very last buffer");
      for (uint16_t i = 0; i < DMA_BUFF_SIZE; i++)
        buffer[i] = 0;
      if (i2s_channel_write(tx_chan, buffer, DMA_BUFF_SIZE, NULL, 1000) !=
          ESP_OK)
        ESP_LOGE(TAG, "i2s write failed at EOF");
      last_fill = true;
    } else {
      uint16_t dma_buffer_size = stereo ? DMA_BUFF_SIZE : DMA_BUFF_SIZE / 2;
      uint32_t read_size; // how many bytes to read
      read_size = remaining_data > dma_buffer_size ? (uint32_t)dma_buffer_size
                                                   : remaining_data;
#ifdef HW_PROFILE
      gpio_set_level(LA1, 1);
#endif
      size_t count = fread(buffer, 1, read_size, fp);
      remaining_data -= (uint32_t)count;
#ifdef HW_PROFILE
      gpio_set_level(LA1, 0);
#endif
#ifdef HW_PROFILE
      gpio_set_level(LA2, 1);
#endif
      // duplicate if mono
      if (!stereo) {
        for (uint16_t i = 0; i < count / 2; i++) {
          buff[dma_buffer_size - 1 - (2 * i)] =
              buff[dma_buffer_size - 2 - (2 * i)] =
                  buff[(dma_buffer_size / 2) - 1 - i] * volume;
        }
      } else {
        for (uint16_t i = 0; i < (count / 2); i++)
          buff[i] = (int16_t)(buff[i] * volume);
      }
#ifdef HW_PROFILE
      gpio_set_level(LA2, 0);
#endif
      if ((count < read_size) || (remaining_data == 0)) {
        // ESP_LOGI(TAG, "count = %d, read_size = %lu, remaining_data = %lu",
        //          count, read_size, remaining_data);
        ESP_LOGI(TAG, "At EOF");
        for (uint16_t i = count; i < DMA_BUFF_SIZE; i++)
          buffer[i] = 0;
        at_eof = true;
      }
#ifdef HW_PROFILE
      gpio_set_level(LA3, 1);
#endif
      if (i2s_channel_write(tx_chan, buffer, DMA_BUFF_SIZE, NULL, 1000) !=
          ESP_OK)
        ESP_LOGE(TAG, "i2s write failed.");
#ifdef HW_PROFILE
      gpio_set_level(LA3, 0);
#endif
    }
  }
}

void button_play_task(void *pvParameters) {
  char path[80];

  for (;;) {
    // Block and wait for interrupt
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "Playing tune %s from button", tune);
    sprintf(path, "%s/tunes/%s", MOUNT_POINT, tune);
    open_file(path);
  }
}
