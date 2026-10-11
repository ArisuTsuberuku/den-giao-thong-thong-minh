// nhan_tin.h — Nhận bản tin đếm xe {"id":1,"seq":1042,"s":3,"l":1,"emg":0}
// từ camera qua ESP-NOW. Hàm gọi lại của ESP-NOW chạy ở luồng WiFi nên chỉ chép gói vào
// hàng đợi; vòng lặp chính lấy ra và giải mã.
#pragma once
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <ArduinoJson.h>
#include "cau_hinh.h"

struct GoiTin {
  char noi_dung[120];
};

struct BanTin {
  int id;
  long seq;       // -1 nếu bản tin không có seq (gõ tay)
  int s, l, emg;
};

static QueueHandle_t hang_doi_tin = NULL;

#if ESP_ARDUINO_VERSION_MAJOR >= 3
static void khiNhanEspNow(const esp_now_recv_info_t *thong_tin, const uint8_t *du_lieu, int dai) {
  (void)thong_tin;
#else
static void khiNhanEspNow(const uint8_t *dia_chi, const uint8_t *du_lieu, int dai) {
  (void)dia_chi;
#endif
  GoiTin g;
  if (dai <= 0 || dai >= (int)sizeof(g.noi_dung)) return;
  memcpy(g.noi_dung, du_lieu, dai);
  g.noi_dung[dai] = 0;
  xQueueSend(hang_doi_tin, &g, 0);
}

inline bool nhanTinKhoiTao() {
  hang_doi_tin = xQueueCreate(16, sizeof(GoiTin));
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_promiscuous(true);                       // cố định kênh, không nối vào mạng WiFi nào
  esp_wifi_set_channel(KENH_WIFI, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  if (esp_now_init() != ESP_OK) return false;
  esp_now_register_recv_cb(khiNhanEspNow);
  return true;
}

inline bool nhanTinLay(GoiTin &g) {
  return hang_doi_tin != NULL && xQueueReceive(hang_doi_tin, &g, 0) == pdTRUE;
}

// Giải mã một dòng JSON. Trả về false nếu sai cú pháp hoặc thiếu id.
inline bool giaiMaBanTin(const char *chuoi, BanTin &b) {
#if ARDUINOJSON_VERSION_MAJOR >= 7
  JsonDocument tai_lieu;
#else
  StaticJsonDocument<192> tai_lieu;
#endif
  if (deserializeJson(tai_lieu, chuoi)) return false;
  b.id = tai_lieu["id"] | 0;
  b.seq = tai_lieu["seq"] | -1L;
  b.s = tai_lieu["s"] | 0;
  b.l = tai_lieu["l"] | 0;
  b.emg = tai_lieu["emg"] | 0;
  return b.id >= 1 && b.id <= 4;
}
