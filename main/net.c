#include "net.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_vfs_fat.h"
#include "global.h"
#include "sd.h"
#include "tag.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

esp_err_t restore_conf(void) {
  char tmp[64];
  FILE *fp = fopen(MOUNT_POINT "/conf/filename1.txt", "r");
  if (fp != NULL) {
    fscanf(fp, "%s", tmp);
    if (tmp[0] != '\0') {
      ESP_LOGI(TAG, "Restoring Doorbell 1 tune: %s", tmp);
      strcpy(tune1, tmp);
    }
    fclose(fp);
  } else {
    ESP_LOGI(TAG, "Could not open \"/conf/filename1.txt\" for reading.");
    return ESP_FAIL;
  }

  fp = fopen(MOUNT_POINT "/conf/filename2.txt", "r");
  if (fp != NULL) {
    fscanf(fp, "%s", tmp);
    if (tmp[0] != '\0') {
      ESP_LOGI(TAG, "Restoring Doorbell 2 tune: %s", tmp);
      strcpy(tune2, tmp);
    }
    fclose(fp);
  } else {
    ESP_LOGI(TAG, "Could not open \"/conf/filename2.txt\" for reading.");
    return ESP_FAIL;
  }

  fp = fopen(MOUNT_POINT "/conf/volume1.txt", "r");
  if (fp != NULL) {
    fscanf(fp, "%s", tmp);
    if (tmp[0] != '\0') {
      ESP_LOGI(TAG, "Restoring Doorbell 1 volume: %sdB", tmp);
      strcpy(vol1, tmp);
      volume1 = (float)pow(10, atof(tmp) / 20.);
    }
    fclose(fp);
  } else {
    ESP_LOGI(TAG, "Could not open \"/conf/volume1.txt\" for reading.");
    return ESP_FAIL;
  }

  fp = fopen(MOUNT_POINT "/conf/volume2.txt", "r");
  if (fp != NULL) {
    fscanf(fp, "%s", tmp);
    if (tmp[0] != '\0') {
      ESP_LOGI(TAG, "Restoring Doorbell 2 volume: %sdB", tmp);
      strcpy(vol2, tmp);
      volume2 = (float)pow(10, atof(tmp) / 20.);
    }
    fclose(fp);
  } else {
    ESP_LOGI(TAG, "Could not open \"/conf/volume2.txt\" for reading.");
    return ESP_FAIL;
  }

  fp = fopen(MOUNT_POINT "/conf/ssid.txt", "r");
  if (fp != NULL) {
    fscanf(fp, "%s", tmp);
    if (tmp[0] != '\0') {
      ESP_LOGI(TAG, "Found SSID: %s", tmp);
      strcpy(wifi_ssid, tmp);
    }
    fclose(fp);
  } else {
    ESP_LOGI(TAG, "Could not open \"/conf/ssid.txt\" for reading.");
    return ESP_FAIL;
  }

  // passwords might have spaces in them so can't use fscanf()
  fp = fopen(MOUNT_POINT "/conf/password.txt", "r");
  if (fp != NULL) {
    fgets(tmp, 64, fp);
    // strip CR/LF if they exist
    if (tmp[strlen(tmp) - 1] == '\n') {
      tmp[strlen(tmp) - 1] = '\0';
    }
    if (tmp[strlen(tmp) - 1] == '\r') {
      tmp[strlen(tmp) - 1] = '\0';
    }
    if (tmp[0] != '\0') {
      ESP_LOGI(TAG, "Found Wifi Password: \"%s\"", tmp);
      strcpy(wifi_pass, tmp);
    }
    fclose(fp);
  } else {
    ESP_LOGI(TAG, "Could not open \"/conf/password.txt\" for reading.");
    return ESP_FAIL;
  }

  strcpy(hostname, "jetson"); // default if no file or no file entry
  fp = fopen(MOUNT_POINT "/conf/hostname.txt", "r");
  if (fp != NULL) {
    tmp[0] = '\0';
    fscanf(fp, "%s", tmp);
    if (tmp[0] != '\0') {
      ESP_LOGI(TAG, "Found hostname: %s", tmp);
      strcpy(hostname, tmp);
    } else {
      ESP_LOGW(TAG, "Hostname file found but with no hostname");
    }
    fclose(fp);
  } else {
    ESP_LOGI(TAG, "No hostname file, using default: \"jetson\"");
  }

  return ESP_OK;
}

// ---------------------------------------------------------------------------
// Embedded web file symbols
// ---------------------------------------------------------------------------
extern const uint8_t index_html_start[]     asm("_binary_index_html_start");
extern const uint8_t index_html_end[]       asm("_binary_index_html_end");
extern const uint8_t fw_html_start[]        asm("_binary_fw_html_start");
extern const uint8_t fw_html_end[]          asm("_binary_fw_html_end");
extern const uint8_t app_js_start[]         asm("_binary_app_js_start");
extern const uint8_t app_js_end[]           asm("_binary_app_js_end");
extern const uint8_t fw_js_start[]          asm("_binary_fw_js_start");
extern const uint8_t fw_js_end[]            asm("_binary_fw_js_end");
extern const uint8_t app_css_start[]        asm("_binary_app_css_start");
extern const uint8_t app_css_end[]          asm("_binary_app_css_end");
extern const uint8_t favicon_ico_start[]    asm("_binary_favicon_ico_start");
extern const uint8_t favicon_ico_end[]      asm("_binary_favicon_ico_end");
extern const uint8_t background_jpg_start[] asm("_binary_background_jpg_start");
extern const uint8_t background_jpg_end[]   asm("_binary_background_jpg_end");

// ---------------------------------------------------------------------------
// Static file handlers
// ---------------------------------------------------------------------------
static esp_err_t handle_index(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, (const char *)index_html_start,
                  index_html_end - index_html_start);
  return ESP_OK;
}

static esp_err_t handle_fw_html(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, (const char *)fw_html_start,
                  fw_html_end - fw_html_start);
  return ESP_OK;
}

static esp_err_t handle_app_js(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/javascript");
  httpd_resp_send(req, (const char *)app_js_start, app_js_end - app_js_start);
  return ESP_OK;
}

static esp_err_t handle_fw_js(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/javascript");
  httpd_resp_send(req, (const char *)fw_js_start, fw_js_end - fw_js_start);
  return ESP_OK;
}

static esp_err_t handle_app_css(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/css");
  httpd_resp_send(req, (const char *)app_css_start,
                  app_css_end - app_css_start);
  return ESP_OK;
}

static esp_err_t handle_favicon(httpd_req_t *req) {
  httpd_resp_set_type(req, "image/x-icon");
  httpd_resp_send(req, (const char *)favicon_ico_start,
                  favicon_ico_end - favicon_ico_start);
  return ESP_OK;
}

static esp_err_t handle_background(httpd_req_t *req) {
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_send(req, (const char *)background_jpg_start,
                  background_jpg_end - background_jpg_start);
  return ESP_OK;
}

// ---------------------------------------------------------------------------
// API handlers
// ---------------------------------------------------------------------------
static esp_err_t handle_list(httpd_req_t *req) {
  FRESULT res;
  FF_DIR dir;
  FILINFO fno;
  int nfile;
  static char names[512];

  names[0] = '\0';
  res = f_opendir(&dir, "/tunes");
  if (res == FR_OK) {
    for (nfile = 0;; nfile++) {
      res = f_readdir(&dir, &fno);
      if (res != FR_OK || fno.fname[0] == 0)
        break;
      if (nfile == 0)
        snprintf(names, sizeof(names), "%s", fno.fname);
      else {
        strcat(names, ",");
        strcat(names, fno.fname);
      }
      ESP_LOGI(TAG, "%10lu %s\n", fno.fsize, fno.fname);
    }
    f_closedir(&dir);
    ESP_LOGI(TAG, "%d files.\n", nfile);
    ESP_LOGI(TAG, "%s", names);
  } else {
    ESP_LOGI(TAG, "Failed to open \"%s\". (%u)\n", "/tunes", res);
  }
  httpd_resp_set_type(req, "text/plain");
  httpd_resp_send(req, names, strlen(names));
  return ESP_OK;
}

static esp_err_t handle_play(httpd_req_t *req) {
  char query[128], name[64], vol[5], path[80];
  query[0] = name[0] = vol[0] = '\0';

  httpd_req_get_url_query_str(req, query, sizeof(query));
  httpd_query_key_value(query, "name", name, sizeof(name));
  httpd_query_key_value(query, "vol", vol, sizeof(vol));

  if (name[0] == '\0') {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "name required");
    return ESP_OK;
  }
  if (strstr(name, "..")) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid path");
    return ESP_OK;
  }

  snprintf(path, sizeof(path), MOUNT_POINT "/tunes/%s", name);
  httpd_resp_send(req, NULL, 0);
  if (xSemaphoreTake(xPlay, 30000 / portTICK_PERIOD_MS)) {
    ESP_LOGI(TAG, "Playing tune %s from browser, volume = %s", name, vol);
    open_file(path, atoi(vol));
  }
  return ESP_OK;
}

static esp_err_t handle_conf_get_file1(httpd_req_t *req) {
  ESP_LOGI(TAG, "Retrieving Doorbell 1 tune: %s", tune1);
  httpd_resp_set_type(req, "text/plain");
  httpd_resp_send(req, tune1, strlen(tune1));
  return ESP_OK;
}

static esp_err_t handle_conf_get_file2(httpd_req_t *req) {
  ESP_LOGI(TAG, "Retrieving Doorbell 2 tune: %s", tune2);
  httpd_resp_set_type(req, "text/plain");
  httpd_resp_send(req, tune2, strlen(tune2));
  return ESP_OK;
}

static esp_err_t handle_conf_get_volume1(httpd_req_t *req) {
  ESP_LOGI(TAG, "Retrieving Doorbell 1 volume: %sdB", vol1);
  httpd_resp_set_type(req, "text/plain");
  httpd_resp_send(req, vol1, strlen(vol1));
  return ESP_OK;
}

static esp_err_t handle_conf_get_volume2(httpd_req_t *req) {
  ESP_LOGI(TAG, "Retrieving Doorbell 2 volume: %sdB", vol2);
  httpd_resp_set_type(req, "text/plain");
  httpd_resp_send(req, vol2, strlen(vol2));
  return ESP_OK;
}

static esp_err_t handle_conf_set(httpd_req_t *req) {
  char query[128], file[64], vstr[5];
  query[0] = '\0';

  if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) {
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
  }

  if (httpd_query_key_value(query, "file1", file, sizeof(file)) == ESP_OK) {
    ESP_LOGI(TAG, "Updating Doorbell 1 tune to: %s", file);
    strcpy(tune1, file);
    FILE *fp = fopen(MOUNT_POINT "/conf/filename1.txt", "w");
    fprintf(fp, "%s", file);
    fclose(fp);
  } else if (httpd_query_key_value(query, "file2", file, sizeof(file)) ==
             ESP_OK) {
    ESP_LOGI(TAG, "Updating Doorbell 2 tune to: %s", file);
    strcpy(tune2, file);
    FILE *fp = fopen(MOUNT_POINT "/conf/filename2.txt", "w");
    fprintf(fp, "%s", file);
    fclose(fp);
  } else if (httpd_query_key_value(query, "volume1", vstr, sizeof(vstr)) ==
             ESP_OK) {
    ESP_LOGI(TAG, "Updating Doorbell 1 volume to: %sdB", vstr);
    strcpy(vol1, vstr);
    FILE *fp = fopen(MOUNT_POINT "/conf/volume1.txt", "w");
    fprintf(fp, "%s", vstr);
    fclose(fp);
    volume1 = (float)pow(10, atof(vstr) / 20.);
  } else if (httpd_query_key_value(query, "volume2", vstr, sizeof(vstr)) ==
             ESP_OK) {
    ESP_LOGI(TAG, "Updating Doorbell 2 volume to: %sdB", vstr);
    strcpy(vol2, vstr);
    FILE *fp = fopen(MOUNT_POINT "/conf/volume2.txt", "w");
    fprintf(fp, "%s", vstr);
    fclose(fp);
    volume2 = (float)pow(10, atof(vstr) / 20.);
  }
  httpd_resp_send(req, NULL, 0);
  return ESP_OK;
}

static esp_err_t handle_upload(httpd_req_t *req) {
  char query[128], name[64], offset_str[16], path[80];
  query[0] = name[0] = offset_str[0] = '\0';

  if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK ||
      httpd_query_key_value(query, "file", name, sizeof(name)) != ESP_OK) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "file required");
    return ESP_OK;
  }
  if (strstr(name, "..")) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid path");
    return ESP_OK;
  }

  snprintf(path, sizeof(path), MOUNT_POINT "/tunes/%s", name);
  long offset = 0;
  if (httpd_query_key_value(query, "offset", offset_str,
                            sizeof(offset_str)) == ESP_OK)
    offset = atol(offset_str);

  FILE *f;
  if (offset == 0) {
    f = fopen(path, "wb");
  } else {
    f = fopen(path, "r+b");
    if (f)
      fseek(f, offset, SEEK_SET);
  }
  if (!f) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "fopen failed");
    return ESP_OK;
  }

  char buf[512];
  int received;
  size_t remaining = req->content_len;
  while (remaining > 0) {
    size_t to_recv = remaining < sizeof(buf) ? remaining : sizeof(buf);
    received = httpd_req_recv(req, buf, to_recv);
    if (received <= 0)
      break;
    fwrite(buf, 1, received, f);
    remaining -= received;
  }
  fclose(f);
  httpd_resp_send(req, NULL, 0);
  return ESP_OK;
}

static esp_err_t handle_firmware_upload(httpd_req_t *req) {
  char query[128], offset_str[16], total_str[20];
  query[0] = offset_str[0] = total_str[0] = '\0';
  static esp_partition_t *p = NULL;
  static esp_ota_handle_t ota_handle;

  if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK ||
      httpd_query_key_value(query, "offset", offset_str,
                            sizeof(offset_str)) != ESP_OK ||
      httpd_query_key_value(query, "total", total_str,
                            sizeof(total_str)) != ESP_OK) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "offset and total not set\n");
    return ESP_OK;
  }

  long ofs = atol(offset_str);
  long tot = atol(total_str);
  ESP_LOGI(TAG, "OTA offset %ld, total %ld, content_len %d", ofs, tot,
           req->content_len);

  if (req->content_len == 0) {
    // Final empty POST: finish OTA and reboot
    if (esp_ota_end(ota_handle) != ESP_OK) {
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                          "esp_ota_end() failed\n");
      return ESP_OK;
    }
    esp_ota_set_boot_partition(p);
    httpd_resp_send(req, NULL, 0);
    ESP_LOGI(TAG, "Time to reboot!");
    vTaskDelay(500 / portTICK_PERIOD_MS);
    esp_restart();
    return ESP_OK;
  }

  if (ofs == 0) {
    p = (esp_partition_t *)esp_ota_get_next_update_partition(NULL);
    if (p == NULL || esp_ota_begin(p, tot, &ota_handle) != ESP_OK) {
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                          "esp_ota_begin() failed\n");
      return ESP_OK;
    }
  }

  char buf[1024];
  int received;
  size_t remaining = req->content_len;
  while (remaining > 0) {
    size_t to_recv = remaining < sizeof(buf) ? remaining : sizeof(buf);
    received = httpd_req_recv(req, buf, to_recv);
    if (received <= 0) {
      esp_ota_abort(ota_handle);
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                          "recv failed\n");
      return ESP_OK;
    }
    if (esp_ota_write(ota_handle, buf, received) != ESP_OK) {
      esp_ota_abort(ota_handle);
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                          "esp_ota_write() failed\n");
      return ESP_OK;
    }
    remaining -= received;
  }
  httpd_resp_send(req, NULL, 0);
  return ESP_OK;
}

static esp_err_t handle_firmware_info(httpd_req_t *req) {
  const esp_partition_t *p = esp_ota_get_boot_partition();
  esp_app_desc_t app_desc;
  char info[128] = {0};

  if (esp_ota_get_partition_description(p, &app_desc) == ESP_OK) {
    snprintf(info, sizeof(info), "%s", app_desc.project_name);
    strcat(info, ",");
    strcat(info, app_desc.date);
    strcat(info, " ");
    strcat(info, app_desc.time);
    strcat(info, ",");
    strcat(info, app_desc.version);
    strcat(info, ",");
    strcat(info, p->label);
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, info, strlen(info));
  }
  return ESP_OK;
}

// ---------------------------------------------------------------------------
// Server startup
// ---------------------------------------------------------------------------
esp_err_t start_webserver(void) {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  config.max_uri_handlers = 20; // we register 17 handlers
  config.stack_size = 8192;     // increased from default 4096 for file I/O

  httpd_handle_t server;
  if (httpd_start(&server, &config) != ESP_OK)
    return ESP_FAIL;

  // Static file handlers
  httpd_uri_t uri_index = {
      .uri = "/", .method = HTTP_GET, .handler = handle_index};
  httpd_uri_t uri_fw_html = {
      .uri = "/fw.html", .method = HTTP_GET, .handler = handle_fw_html};
  httpd_uri_t uri_app_js = {
      .uri = "/app.js", .method = HTTP_GET, .handler = handle_app_js};
  httpd_uri_t uri_fw_js = {
      .uri = "/fw.js", .method = HTTP_GET, .handler = handle_fw_js};
  httpd_uri_t uri_app_css = {
      .uri = "/app.css", .method = HTTP_GET, .handler = handle_app_css};
  httpd_uri_t uri_favicon = {
      .uri = "/favicon.ico", .method = HTTP_GET, .handler = handle_favicon};
  httpd_uri_t uri_background = {.uri = "/background.jpg",
                                 .method = HTTP_GET,
                                 .handler = handle_background};
  // API handlers
  httpd_uri_t uri_list = {
      .uri = "/list", .method = HTTP_GET, .handler = handle_list};
  httpd_uri_t uri_play = {
      .uri = "/play", .method = HTTP_GET, .handler = handle_play};
  httpd_uri_t uri_cfg_file1 = {.uri = "/conf/get/file1",
                                .method = HTTP_GET,
                                .handler = handle_conf_get_file1};
  httpd_uri_t uri_cfg_file2 = {.uri = "/conf/get/file2",
                                .method = HTTP_GET,
                                .handler = handle_conf_get_file2};
  httpd_uri_t uri_cfg_vol1 = {.uri = "/conf/get/volume1",
                               .method = HTTP_GET,
                               .handler = handle_conf_get_volume1};
  httpd_uri_t uri_cfg_vol2 = {.uri = "/conf/get/volume2",
                               .method = HTTP_GET,
                               .handler = handle_conf_get_volume2};
  httpd_uri_t uri_cfg_set = {
      .uri = "/conf/set", .method = HTTP_GET, .handler = handle_conf_set};
  httpd_uri_t uri_upload = {
      .uri = "/upload", .method = HTTP_POST, .handler = handle_upload};
  httpd_uri_t uri_fw_upload = {.uri = "/firmware/upload",
                                .method = HTTP_POST,
                                .handler = handle_firmware_upload};
  httpd_uri_t uri_fw_info = {.uri = "/firmware/info",
                              .method = HTTP_GET,
                              .handler = handle_firmware_info};

  httpd_register_uri_handler(server, &uri_index);
  httpd_register_uri_handler(server, &uri_fw_html);
  httpd_register_uri_handler(server, &uri_app_js);
  httpd_register_uri_handler(server, &uri_fw_js);
  httpd_register_uri_handler(server, &uri_app_css);
  httpd_register_uri_handler(server, &uri_favicon);
  httpd_register_uri_handler(server, &uri_background);
  httpd_register_uri_handler(server, &uri_list);
  httpd_register_uri_handler(server, &uri_play);
  httpd_register_uri_handler(server, &uri_cfg_file1);
  httpd_register_uri_handler(server, &uri_cfg_file2);
  httpd_register_uri_handler(server, &uri_cfg_vol1);
  httpd_register_uri_handler(server, &uri_cfg_vol2);
  httpd_register_uri_handler(server, &uri_cfg_set);
  httpd_register_uri_handler(server, &uri_upload);
  httpd_register_uri_handler(server, &uri_fw_upload);
  httpd_register_uri_handler(server, &uri_fw_info);

  ESP_LOGI(TAG, "HTTP server started on port %d", config.server_port);
  return ESP_OK;
}
