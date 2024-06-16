#include "esp_check.h"

#define MOUNT_POINT "/sdcard"

esp_err_t sd_setup(void);
esp_err_t open_file(const char *filename, uint16_t doorbell);

typedef struct {
  char id[4];     // RIFF
  uint32_t size;  // rest of chunk
  char format[4]; // WAVE
} wave_header_t;

typedef struct {
  char id[4];
  uint32_t size;
} chunk_header_t;

typedef struct {
  uint16_t audio_format; /*!< PCM = 1, values other than 1 indicate some form of
                            compression */
  uint16_t num_of_channels; /*!< Mono = 1, Stereo = 2, etc. */
  uint32_t sample_rate;     /*!< 8000, 44100, etc. */
  uint32_t byte_rate;   /*!< ==SampleRate * NumChannels * BitsPerSample s/ 8 */
  uint16_t block_align; /*!< ==NumChannels * BitsPerSample / 8 */
  uint16_t bits_per_sample; /*!< 8 bits = 8, 16 bits = 16, etc. */
} fmt_chunk_t; /*!< The "fmt " subchunk describes the sound data's format */