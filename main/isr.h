#include "driver/i2s_std.h"

bool i2s_write_callback(i2s_chan_handle_t handle, i2s_event_data_t *event,
                        void *user_ctx);
void button1_isr_handler(void *arg);
void button2_isr_handler(void *arg);
