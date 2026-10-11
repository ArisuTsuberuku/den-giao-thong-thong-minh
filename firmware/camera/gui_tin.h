// gui_tin.h — Gửi bản tin {"id":1,"seq":1042,"s":3,"l":1,"emg":0,"ut":0} bằng ESP-NOW.
// emg: xe ưu tiên ở làn nào (0 không có, 1 làn ngoài, 2 làn trong); ut: loại xe ưu tiên
// (1 cứu hỏa, 2 quân sự, 3 công an, 4 cứu thương).
// Gửi kiểu phát chung (địa chỉ FF:FF:FF:FF:FF:FF) trên kênh cố định, nên không cần biết
// địa chỉ MAC của board trung tâm và thay board không phải sửa code.
#pragma once
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "cau_hinh.h"

static const uint8_t DIA_CHI_PHAT_CHUNG[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint32_t so_thu_tu_goi = 0;

inline bool guiTinKhoiTao() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_promiscuous(true);                       // cố định kênh, không nối vào mạng WiFi nào
  esp_wifi_set_channel(KENH_WIFI, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  if (esp_now_init() != ESP_OK) return false;
  esp_now_peer_info_t noi_nhan;
  memset(&noi_nhan, 0, sizeof(noi_nhan));
  memcpy(noi_nhan.peer_addr, DIA_CHI_PHAT_CHUNG, 6);
  noi_nhan.channel = KENH_WIFI;
  noi_nhan.encrypt = false;
  return esp_now_add_peer(&noi_nhan) == ESP_OK;
}

// Gửi một gói; chuỗi JSON vừa gửi được chép ra da_gui (nếu khác NULL) để in Serial.
inline bool guiBanTin(uint8_t s, uint8_t l, uint8_t emg, uint8_t ut, char *da_gui, size_t co) {
  char goi[96];
  int dai = snprintf(goi, sizeof(goi), "{\"id\":%d,\"seq\":%lu,\"s\":%u,\"l\":%u,\"emg\":%u,\"ut\":%u}",
                     CAM_ID, (unsigned long)so_thu_tu_goi++, s, l, emg, ut);
  if (da_gui) snprintf(da_gui, co, "%s", goi);
  return esp_now_send(DIA_CHI_PHAT_CHUNG, (const uint8_t *)goi, dai) == ESP_OK;
}
