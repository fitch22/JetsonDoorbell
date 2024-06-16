#include "net.h"
#include "esp_ota_ops.h"
#include "esp_vfs_fat.h"
#include "global.h"
#include "sd.h"
#include "tag.h"
#include <math.h>

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
      char *ptr;
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
      char *ptr;
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

// Handlers for various endpoints
static void handle_upload(struct mg_connection *c, struct mg_http_message *hm) {
  mg_http_upload(c, hm, &mg_fs_fat, "/tunes", 9999999);
}

static void handle_play(struct mg_connection *c, struct mg_http_message *hm) {
  char path[80], name[64], vol[5];
  mg_http_get_var(&hm->query, "name", name, sizeof(name));
  mg_http_get_var(&hm->query, "vol", vol, sizeof(vol));
  mg_snprintf(path, sizeof(path), MOUNT_POINT "/tunes/%s", name);
  if (name[0] == '\0') {
    mg_http_reply(c, 400, "", "%s", "name required");
  } else if (!mg_path_is_sane(path)) {
    mg_http_reply(c, 400, "", "%s", "invalid path");
  } else {
    mg_http_reply(c, 200, NULL, "");
    if (xSemaphoreTake(xPlay, 30000 / portTICK_PERIOD_MS)) {
      ESP_LOGI(TAG, "Playing tune %s from browser, volume = %s", name, vol);
      open_file(path, atoi(vol));
    }
  }
}

static void handle_list(struct mg_connection *c) {
  FRESULT res;
  FF_DIR dir;
  FILINFO fno;
  int nfile;
  static char names[512];

  names[0] = '\0';
  res = f_opendir(&dir, "/tunes"); /* Open the directory */
  if (res == FR_OK) {
    for (nfile = 0;; nfile++) {
      res = f_readdir(&dir, &fno); /* Read a directory item */
      if (res != FR_OK || fno.fname[0] == 0)
        break; /* Error or end of dir */
      if (nfile == 0)
        snprintf(names, sizeof(names), "%s", fno.fname);
      else {
        strcat(names, ",");
        strcat(names, fno.fname);
      }
      ESP_LOGI(TAG, "%10llu %s\n", fno.fsize, fno.fname);
    }
    f_closedir(&dir);
    ESP_LOGI(TAG, "%d files.\n", nfile);
    ESP_LOGI(TAG, "%s", names);
  } else {
    ESP_LOGI(TAG, "Failed to open \"%s\". (%u)\n", "/tunes", res);
  }
  mg_http_reply(c, 200, "Content-Type: text/plain\r\n", names);
}

static void handle_conf_get_file1(struct mg_connection *c,
                                  struct mg_http_message *hm) {

  // return the tune file
  mg_http_reply(c, 200, "Content-Type: text/plain\r\n", tune1);
  ESP_LOGI(TAG, "Retrieving Doorbell 1 tune: %s", tune1);
}

static void handle_conf_get_file2(struct mg_connection *c,
                                  struct mg_http_message *hm) {

  // return the tune file
  mg_http_reply(c, 200, "Content-Type: text/plain\r\n", tune2);
  ESP_LOGI(TAG, "Retrieving Doorbell 2 tune: %s", tune2);
}

static void handle_conf_get_volume1(struct mg_connection *c,
                                    struct mg_http_message *hm) {

  // return the volume
  mg_http_reply(c, 200, "Content-Type: text/plain\r\n", vol1);
  ESP_LOGI(TAG, "Retrieving Doorbell 1 volume: %sdB", vol1);
}

static void handle_conf_get_volume2(struct mg_connection *c,
                                    struct mg_http_message *hm) {

  // return the volume
  mg_http_reply(c, 200, "Content-Type: text/plain\r\n", vol2);
  ESP_LOGI(TAG, "Retrieving Doorbell 2 volume: %sdB", vol2);
}

static void handle_conf_set(struct mg_connection *c,
                            struct mg_http_message *hm) {
  char file[64], vstr[5];

  // see if we are setting the tune file or the volume
  if (mg_http_get_var(&hm->query, "file1", file, sizeof(file)) > 0) {
    ESP_LOGI(TAG, "Updating Doorbell 1 tune to: %s", file);
    strcpy(tune1, file);
    FILE *fp = fopen(MOUNT_POINT "/conf/filename1.txt", "w");
    fprintf(fp, "%s", file);
    fclose(fp);
  } else if (mg_http_get_var(&hm->query, "file2", file, sizeof(file)) > 0) {
    ESP_LOGI(TAG, "Updating Doorbell 2 tune to: %s", file);
    strcpy(tune2, file);
    FILE *fp = fopen(MOUNT_POINT "/conf/filename2.txt", "w");
    fprintf(fp, "%s", file);
    fclose(fp);
  } else if (mg_http_get_var(&hm->query, "volume1", vstr, sizeof(vstr)) > 0) {
    ESP_LOGI(TAG, "Updating Doorbell 1 volume to: %sdB", vstr);
    strcpy(vol1, vstr);
    FILE *fp = fopen(MOUNT_POINT "/conf/volume1.txt", "w");
    fprintf(fp, "%s", vstr);
    fclose(fp);
    char *ptr;
    volume1 = (float)pow(10, atof(vstr) / 20.);
  } else if (mg_http_get_var(&hm->query, "volume2", vstr, sizeof(vstr)) > 0) {
    ESP_LOGI(TAG, "Updating Doorbell 2 volume to: %sdB", vstr);
    strcpy(vol2, vstr);
    FILE *fp = fopen(MOUNT_POINT "/conf/volume2.txt", "w");
    fprintf(fp, "%s", vstr);
    fclose(fp);
    char *ptr;
    volume2 = (float)pow(10, atof(vstr) / 20.);
  }
  mg_http_reply(c, 200, NULL, "");
}

static void handle_firmware_upload(struct mg_connection *c,
                                   struct mg_http_message *hm) {
  char name[64], offset[20], total[20];
  struct mg_str data = hm->body;
  long ofs = -1, tot = -1;
  name[0] = offset[0] = '\0';
  static esp_partition_t *p = NULL;
  static esp_ota_handle_t handle;

  mg_http_get_var(&hm->query, "name", name, sizeof(name));
  mg_http_get_var(&hm->query, "offset", offset, sizeof(offset));
  mg_http_get_var(&hm->query, "total", total, sizeof(total));
  MG_INFO(("File %s, offset %s, len %lu", name, offset, data.len));

  if ((ofs = mg_json_get_long(mg_str(offset), "$", -1)) < 0 ||
      (tot = mg_json_get_long(mg_str(total), "$", -1)) < 0) {
    mg_http_reply(c, 500, "", "offset and total not set\n");
  } else if (ofs == 0 &&
             ((p = (esp_partition_t *)esp_ota_get_next_update_partition(
                   NULL)) == NULL ||
              esp_ota_begin(p, tot, &handle) != ESP_OK)) {
    mg_http_reply(c, 500, "", "esp_ota_begin(%ld) failed\n", tot);
  } else if (data.len > 0 &&
             esp_ota_write(handle, data.ptr, data.len) != ESP_OK) {
    mg_http_reply(c, 500, "", "esp_ota_write(%lu) @%ld failed\n", data.len,
                  ofs);
    esp_ota_abort(handle);
  } else if (data.len == 0 && esp_ota_end(handle) != ESP_OK) {
    mg_http_reply(c, 500, "", "esp_ota_end() failed\n", tot);
  } else {
    mg_http_reply(c, 200, NULL, "");
    if (data.len == 0) {
      // Successful esp_ota_end() called, update boot partition and schedule
      // device reboot
      esp_ota_set_boot_partition(p);
      mg_timer_add(c->mgr, 500, 0, (void (*)(void *))esp_restart, NULL);
      ESP_LOGI(TAG, "Time to reboot!");
    }
  }
}

static void handle_firmware_info(struct mg_connection *c) {
  const esp_partition_t *p = esp_ota_get_boot_partition();
  esp_app_desc_t app_desc; // app description
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
    mg_http_reply(c, 200, "Content-Type: text/plain\r\n", info);
  }
}

/* HTTP request handler function. It implements the following endpoints:
 *  /upload - Saves the next file chunk
 *  /play - will play selected song
 *  /list - provides a list of files in /tunes folder
 *  /conf/get/file - get configuration file
 *  /conf/set - set configuration
 *  /firmware/upload - upload new firmware
 *  /firmware/info - gather details of currently booted image
 *  all other URI - serves /web_root folder
 */
void cb(struct mg_connection *c, int ev, void *ev_data) {
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *)ev_data;
    if (mg_http_match_uri(hm, "/upload")) {
      handle_upload(c, hm);
    } else if (mg_http_match_uri(hm, "/play")) {
      handle_play(c, hm);
    } else if (mg_http_match_uri(hm, "/list")) {
      // return list of files in /tunes folder
      handle_list(c);
    } else if (mg_http_match_uri(hm, "/conf/get/file1")) {
      handle_conf_get_file1(c, hm);
    } else if (mg_http_match_uri(hm, "/conf/get/file2")) {
      handle_conf_get_file2(c, hm);
    } else if (mg_http_match_uri(hm, "/conf/get/volume1")) {
      handle_conf_get_volume1(c, hm);
    } else if (mg_http_match_uri(hm, "/conf/get/volume2")) {
      handle_conf_get_volume2(c, hm);
    } else if (mg_http_match_uri(hm, "/conf/set")) {
      handle_conf_set(c, hm);
    } else if (mg_http_match_uri(hm, "/firmware/upload")) {
      handle_firmware_upload(c, hm);
    } else if (mg_http_match_uri(hm, "/firmware/info")) {
      handle_firmware_info(c);
    } else {
      struct mg_http_serve_opts opts = {.root_dir = "/web_root",
                                        .fs = &mg_fs_fat};
      mg_http_serve_dir(c, ev_data, &opts);
    }
  }
}
