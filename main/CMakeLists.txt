idf_component_register(SRCS "error_blink.c" "global.c" "i2s.c" "task.c" "isr.c" "gpio.c" "sd.c" "main.c"
                            "wifi.c"
                            "net.c"
                            "mongoose.c")
component_compile_options(-DMG_ENABLE_LINES)
component_compile_options(-DMG_ENABLE_FATFS)
component_compile_options(-DHTTP_URL="http://0.0.0.0:80")
component_compile_options(-DHTTPS_URL="https://0.0.0.0:443")
component_compile_options(-DMG_TLS=MG_TLS_NONE) # change to 'MG_TLS_MBED' to enable TLS
component_compile_options(-DHW_PROFILE)
