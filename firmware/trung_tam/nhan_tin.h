// nhan_tin.h — Nhận bản tin đếm xe {"id":1,"seq":1042,"s":3,"l":1,"emg":0,"ut":0}
// từ camera qua ESP-NOW. emg: xe ưu tiên ở làn nào (0 không có, 1 làn ngoài, 2 làn trong);
// ut: loại xe ưu tiên (1 cứu hỏa, 2 quân sự, 3 công an, 4 cứu thương). Bản tin không có "ut" mà
// emg khác 0 thì coi là xe cứu thương. Hàm gọi lại của ESP-NOW chạy ở luồng WiFi nên chỉ chép gói vào
// hàng đợi; vòng lặp chính lấy ra và giải mã.
// Ngoài số đếm, camera còn gửi tin trạng thái {"tt":1,"id":1,"cd":2,"ip":"192.168.4.11",...} mỗi 3 giây
// (xem firmware/camera/mang.h); trung tâm gửi lệnh xuống camera bằng guiLenhCam().
// Khi DUNG_WEB là 1, trung tâm phát WiFi trên đúng KENH_WIFI để camera và điện thoại nối vào.
#pragma once
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <ArduinoJson.h>
#include "cau_hinh.h"
#include "cau_hinh_mang.h"

struct GoiTin {
  char noi_dung[250];        // gói ESP-NOW dài tối đa 250 byte
};

static const uint8_t DIA_CHI_PHAT_CHUNG[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static bool co_phat_lenh = false;

// Trạng thái từng camera, lấy từ tin "tt"
struct TrangThaiCam {
  bool co;
  char ip[16];
  int cd;                    // chế độ: 1 thu ảnh, 2 đếm, 3 giả lập
  float kh;                  // khung/giây
  int rs;                    // sóng WiFi (dBm), 0 là camera chưa nối WiFi trung tâm
  int vl;                    // 1: đang dùng vùng đếm lưu trong board
  int lat;                   // bit 0 lật dọc, bit 1 lật ngang
  uint32_t up;               // camera đã chạy bao nhiêu giây
  uint32_t t;                // millis() lúc nhận tin gần nhất
};
static TrangThaiCam tt_cam[5];

struct BanTin {
  int id;
  long seq;       // -1 nếu bản tin không có seq (gõ tay)
  int s, l, emg;
  int ut;         // loại xe ưu tiên, 0 nếu bản tin không ghi
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
  WiFi.persistent(false);
#if DUNG_WEB
  WiFi.mode(WIFI_AP_STA);                               // phát WiFi; kênh của WiFi này cũng là kênh ESP-NOW
  WiFi.setSleep(false);
  if (!WiFi.softAP(TEN_WIFI_TRUNG_TAM, MAT_KHAU_TRUNG_TAM, KENH_WIFI, 0, SO_MAY_TOI_DA)) Serial.println("KHÔNG phát được WiFi.");
#else
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_promiscuous(true);                       // cố định kênh, không nối vào mạng WiFi nào
  esp_wifi_set_channel(KENH_WIFI, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
#endif
  if (esp_now_init() != ESP_OK) return false;
  esp_now_register_recv_cb(khiNhanEspNow);
  esp_now_peer_info_t noi_nhan;
  memset(&noi_nhan, 0, sizeof(noi_nhan));
  memcpy(noi_nhan.peer_addr, DIA_CHI_PHAT_CHUNG, 6);
  noi_nhan.channel = 0;                                 // kênh đang dùng
  noi_nhan.ifidx = DUNG_WEB ? WIFI_IF_AP : WIFI_IF_STA;
  noi_nhan.encrypt = false;
  co_phat_lenh = esp_now_add_peer(&noi_nhan) == ESP_OK;
  return true;
}

// Gửi một lệnh cho camera, ví dụ guiLenhCam(0, "nhay", "") cho cả 4 camera cùng nháy LED.
// den: 0 là mọi camera, 1..4 là một camera. them: các trường thêm, ví dụ "\"doc\":1,\"ngang\":1".
inline bool guiLenhCam(int den, const char *lenh, const char *them) {
  if (!co_phat_lenh) return false;
  char b[200];
  int n = snprintf(b, sizeof(b), "{\"lenh\":\"%s\",\"den\":%d%s%s}", lenh, den, (them && *them) ? "," : "", them ? them : "");
  if (n <= 0 || n >= (int)sizeof(b)) return false;
  return esp_now_send(DIA_CHI_PHAT_CHUNG, (const uint8_t *)b, n) == ESP_OK;
}

// Tin trạng thái của camera. Trả về true nếu chuỗi là tin trạng thái (dù hỏng hay không).
inline bool nhanTrangThaiCam(const char *chuoi, uint32_t bay_gio) {
  if (!strstr(chuoi, "\"tt\"")) return false;
#if ARDUINOJSON_VERSION_MAJOR >= 7
  JsonDocument d;
#else
  StaticJsonDocument<256> d;
#endif
  if (deserializeJson(d, chuoi)) return true;
  int id = d["id"] | 0;
  if (id < 1 || id > 4) return true;
  TrangThaiCam &c = tt_cam[id];
  c.co = true;
  snprintf(c.ip, sizeof(c.ip), "%s", (const char *)(d["ip"] | ""));
  c.cd = d["cd"] | 0; c.kh = d["kh"] | 0.0f; c.rs = d["rs"] | 0; c.vl = d["vl"] | 0; c.lat = d["lat"] | 0;
  c.up = d["up"] | 0; c.t = bay_gio;
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
  b.ut = tai_lieu["ut"] | 0;
  return b.id >= 1 && b.id <= 4;
}
