// Code taken from the ESP32 IDF WiFi station Example

#include "wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "global.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "nvs_flash.h"
#include <string.h>

static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about
 * two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const char *TAG = "wifi station";
static int s_retry_num = 0;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT) {
    if (event_id == WIFI_EVENT_STA_START) {
      ESP_LOGI(TAG, "WIFI Started");
      esp_wifi_connect();
    } else if (event_id == WIFI_EVENT_STA_CONNECTED) {
      ESP_LOGI(TAG, "WIFI Connected after %d retries", s_retry_num);
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
      wifi_event_sta_disconnected_t *disconn = event_data;
      ESP_LOGI(TAG, "WIFI Disconnected with reason %d", disconn->reason);
      // Until we are connected for the first time, try to connect a limited
      // number of times.  After we have connected successfully, any disconnect
      // will try to reconnect indefinitely.
      if ((s_retry_num < 10) ||
          (WIFI_CONNECTED_BIT & xEventGroupGetBits(s_wifi_event_group))) {
        esp_err_t ret = esp_wifi_connect();
        s_retry_num++;
        if (ret == ESP_OK) {
          ESP_LOGI(TAG, "WIFI Reconnect successful");
        } else {
          ESP_LOGI(TAG, "WIFI Reconnect: esp_wifi_connect() returned %d", ret);
        }
      } else {
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        ESP_LOGI(TAG, "WIFI connect to AP fail");
      }
    } else if (event_id == WIFI_EVENT_HOME_CHANNEL_CHANGE) {
      ESP_LOGI(TAG, "WIFI Channel Change");
    } else {
      ESP_LOGI(TAG, "unexpected WIFI_EVENT %d", event_id);
    }
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "IP ADDRESS:" IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

esp_err_t wifi_init(const char *ssid, const char *pass) {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  s_wifi_event_group = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_netif_init());

  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_t *netif = esp_netif_create_default_wifi_sta();

  // set our hostname
  esp_netif_set_hostname(netif, hostname);

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  // esp_wifi_set_ps(WIFI_PS_NONE); // turn off power saving

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

  wifi_config_t c = {.sta = {.scan_method = WIFI_ALL_CHANNEL_SCAN,
                             .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
                             .threshold = {.authmode = WIFI_AUTH_WPA2_PSK},
                             .pmf_cfg = {.capable = true, .required = false},
                             .rm_enabled = 1,
                             .mbo_enabled = 1,
                             .ft_enabled = 1}};
  snprintf((char *)c.sta.ssid, sizeof(c.sta.ssid), "%s", ssid);
  snprintf((char *)c.sta.password, sizeof(c.sta.password), "%s", pass);
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &c));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_LOGI(TAG, "wifi_init_sta finished.");

  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                         pdFALSE, pdFALSE, portMAX_DELAY);

  if (bits & WIFI_CONNECTED_BIT) {
    ESP_LOGI(TAG, "connected to ap SSID:%s", ssid);
    ret = ESP_OK;
  } else if (bits & WIFI_FAIL_BIT) {
    ESP_LOGE(TAG, "Failed to connect to SSID:%s", ssid);
    ret = ESP_FAIL;
  } else {
    ESP_LOGE(TAG, "UNEXPECTED EVENT");
    ret = ESP_FAIL;
  }
  return ret;
}
