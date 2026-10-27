// mang.h — WiFi, ESP-NOW và lệnh điều khiển từ board trung tâm.
//
// Cách nối: board trung tâm phát WiFi "TRUNG-TAM-SaBan" trên đúng KENH_WIFI. Camera nối vào WiFi đó với
// địa chỉ cố định 192.168.4.(10 + CAM_ID). Nhờ nối vào, camera luôn ở đúng kênh của trung tâm nên ESP-NOW
// vẫn chạy như cũ, đồng thời máy tính hay điện thoại nối vào WiFi trung tâm mở được trang web của từng camera.
// Trung tâm chưa bật thì camera vẫn chạy, giữ kênh KENH_WIFI và tự nối lại khi trung tâm có WiFi.
// Trung tâm nối vào hotspot máy tính thì WiFi của trung tâm chuyển sang kênh của hotspot; camera rớt ra
// vài giây rồi tự tìm và nối lại ở kênh mới, ESP-NOW cũng theo kênh mới.
//
// Lệnh trung tâm gửi xuống (ESP-NOW phát chung, cả 4 camera cùng nhận một lúc):
//   {"lenh":"xem","den":0}        gửi ngay tin trạng thái        "den": 0 là mọi camera, 1..4 là CAM đó
//   {"lenh":"nhay","den":2}       nháy LED đỏ 3 giây để biết board nào là CAM 2
//   {"lenh":"khoidong","den":0}   khởi động lại
//   {"lenh":"vungmacdinh","den":3} bỏ vùng và kiểu lật đã lưu, về số trong cau_hinh.h
//   {"lenh":"lat","den":0,"doc":1,"ngang":1}  lật ảnh (cả hai là 1: xoay 180°), lưu vào board
//   {"lenh":"gia","den":1,"s":3,"l":1,"emg":0,"ut":0}   đặt số đếm giả (chỉ ở chế độ giả lập)
// Tin trạng thái camera gửi lên mỗi 3 giây:
//   {"tt":1,"id":1,"cd":2,"ip":"192.168.4.11","kh":1.0,"rs":-48,"vl":1,"lat":3,"up":125}
//   cd: chế độ (1 thu ảnh, 2 đếm, 3 giả lập); kh: khung/giây; rs: sóng WiFi (dBm, 0 là chưa nối);
//   vl: 1 nếu đang dùng vùng lưu trong board; lat: bit 0 lật dọc, bit 1 lật ngang; up: số giây từ lúc bật máy.
#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "cau_hinh_mang.h"
#include "vung_dem.h"

static WebServer may_chu(80);
static const uint8_t DIA_CHI_PHAT_CHUNG[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static bool mang_tu_phat = false;            // true: không thấy trung tâm, camera tự phát WiFi (chế độ thu ảnh)
static bool mang_co_espnow = false;
static bool mang_co_web = false;
static float mang_khung_giay = 0;            // chế độ đếm ghi vào đây để báo về trung tâm
static uint32_t mang_t_trang_thai = 0, mang_t_nhay = 0, mang_t_khoi_dong = 0, mang_t_kenh = 0, mang_t_noi = 0;
static uint8_t mang_kenh = KENH_WIFI;       // kênh của lần nối WiFi trung tâm gần nhất (trung tâm nối hotspot thì kênh đổi theo hotspot)
static void (*khiLenhGia)(int s, int l, int emg, int ut) = NULL;   // chế độ giả lập gán hàm này

struct LenhCam { char nd[200]; };
static QueueHandle_t hang_doi_lenh_cam = NULL;

// Lấy số nguyên sau "khoa": trong một chuỗi JSON phẳng; không có thì trả mac_dinh.
inline int laySoJson(const char *json, const char *khoa, int mac_dinh) {
  char mau[24];
  snprintf(mau, sizeof(mau), "\"%s\":", khoa);
  const char *p = strstr(json, mau);
  if (!p) return mac_dinh;
  p += strlen(mau);
  while (*p == ' ') p++;
  if (*p != '-' && (*p < '0' || *p > '9')) return mac_dinh;
  return atoi(p);
}

#if ESP_ARDUINO_VERSION_MAJOR >= 3
static void khiNhanLenhCam(const esp_now_recv_info_t *tt, const uint8_t *du_lieu, int dai) {
  (void)tt;
#else
static void khiNhanLenhCam(const uint8_t *dia_chi, const uint8_t *du_lieu, int dai) {
  (void)dia_chi;
#endif
  LenhCam g;
  if (!hang_doi_lenh_cam || dai <= 0 || dai >= (int)sizeof(g.nd)) return;
  memcpy(g.nd, du_lieu, dai);
  g.nd[dai] = 0;
  if (!strstr(g.nd, "\"lenh\"")) return;      // bỏ qua bản tin đếm xe của các camera khác
  xQueueSend(hang_doi_lenh_cam, &g, 0);
}

inline void mangGiuKenh() {                   // chưa nối được trung tâm: đưa radio về kênh trung tâm dùng gần nhất
  uint8_t kenh = 0;
  wifi_second_chan_t phu;
  if (esp_wifi_get_channel(&kenh, &phu) == ESP_OK && kenh == mang_kenh) return;
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(mang_kenh, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
}

// cho_phep_tu_phat: chế độ thu ảnh được tự phát WiFi riêng khi không thấy trung tâm.
inline bool mangKhoiTao(bool cho_phep_tu_phat) {
  hang_doi_lenh_cam = xQueueCreate(6, sizeof(LenhCam));
#if CHAN_LED_DO >= 0
  pinMode(CHAN_LED_DO, OUTPUT);
  digitalWrite(CHAN_LED_DO, !LED_DO_SANG);
#endif
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);                       // bắt buộc: WiFi ngủ giữa hai beacon sẽ lỡ lệnh ESP-NOW của trung tâm
  bool da_noi = false;
#if NOI_WIFI_TRUNG_TAM
  WiFi.setAutoReconnect(true);
  WiFi.config(IPAddress(192, 168, 4, IP_CAM_BYTE_CUOI), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.begin(TEN_WIFI_TRUNG_TAM, MAT_KHAU_TRUNG_TAM, KENH_WIFI);
  Serial.printf("Đang nối WiFi trung tâm \"%s\"", TEN_WIFI_TRUNG_TAM);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < CHO_NOI_MS) { delay(250); Serial.print('.'); }
  da_noi = WiFi.status() == WL_CONNECTED;
  mang_t_noi = millis();
  if (da_noi) {
    mang_kenh = (uint8_t)WiFi.channel();
    Serial.printf(" xong (kênh %d). Địa chỉ: http://%s\n", mang_kenh, WiFi.localIP().toString().c_str());
  }
  else Serial.println(" không thấy (trung tâm chưa bật?).");
#endif
  if (!da_noi && cho_phep_tu_phat && TU_PHAT_KHI_KHONG_THAY) {
    char ten[24];
    snprintf(ten, sizeof(ten), "CAM%d-SaBan", CAM_ID);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);
    if (!WiFi.softAP(ten, MAT_KHAU_WIFI, KENH_WIFI)) return false;
    mang_tu_phat = true;
    Serial.printf("Tự phát WiFi \"%s\", mật khẩu \"%s\". Nối máy tính vào WiFi đó rồi mở http://%s\n",
                  ten, MAT_KHAU_WIFI, WiFi.softAPIP().toString().c_str());
  } else if (!da_noi) {
    mangGiuKenh();
  }
  if (esp_now_init() == ESP_OK) {
    esp_now_peer_info_t noi_nhan;
    memset(&noi_nhan, 0, sizeof(noi_nhan));
    memcpy(noi_nhan.peer_addr, DIA_CHI_PHAT_CHUNG, 6);
    noi_nhan.channel = 0;                     // 0: kênh đang dùng (luôn là kênh của WiFi trung tâm hoặc KENH_WIFI)
    noi_nhan.ifidx = mang_tu_phat ? WIFI_IF_AP : WIFI_IF_STA;
    noi_nhan.encrypt = false;
    mang_co_espnow = esp_now_add_peer(&noi_nhan) == ESP_OK;
    esp_now_register_recv_cb(khiNhanLenhCam);
  }
  return mang_co_espnow;
}

inline bool mangGui(const char *nd) {
  if (!mang_co_espnow) return false;
  return esp_now_send(DIA_CHI_PHAT_CHUNG, (const uint8_t *)nd, strlen(nd)) == ESP_OK;
}

inline void mangGuiTrangThai() {
  char b[200];
  bool noi = WiFi.status() == WL_CONNECTED;
  String ip = mang_tu_phat ? WiFi.softAPIP().toString() : (noi ? WiFi.localIP().toString() : String(""));
  snprintf(b, sizeof(b), "{\"tt\":1,\"id\":%d,\"cd\":%d,\"ip\":\"%s\",\"kh\":%.1f,\"rs\":%d,\"vl\":%d,\"lat\":%d,\"up\":%lu}",
           CAM_ID, CHE_DO, ip.c_str(), mang_khung_giay, noi ? (int)WiFi.RSSI() : 0, vung_da_luu ? 1 : 0,
           lat_doc | (lat_ngang << 1), (unsigned long)(millis() / 1000));
  mangGui(b);
}

inline void mangXuLyLenh(const char *nd) {
  int den = laySoJson(nd, "den", 0);
  if (den != 0 && den != CAM_ID) return;
  if (strstr(nd, "\"lenh\":\"xem\"")) { mangGuiTrangThai(); }
  else if (strstr(nd, "\"lenh\":\"nhay\"")) { mang_t_nhay = millis() | 1; }
  else if (strstr(nd, "\"lenh\":\"khoidong\"")) {
    Serial.println("Trung tâm yêu cầu khởi động lại.");
    mang_t_khoi_dong = (millis() + 200 + 150 * CAM_ID) | 1;   // lệch nhau một chút để 4 camera không bật cùng lúc
  }
  else if (strstr(nd, "\"lenh\":\"vungmacdinh\"")) { vungXoa(); Serial.println("Đã về vùng đếm trong cau_hinh.h."); mangGuiTrangThai(); }
  else if (strstr(nd, "\"lenh\":\"lat\"")) {
    latDat(laySoJson(nd, "doc", lat_doc), laySoJson(nd, "ngang", lat_ngang));
    Serial.printf("Trung tâm yêu cầu lật ảnh: dọc %d, ngang %d\n", lat_doc, lat_ngang);
    mangGuiTrangThai();
  }
  else if (strstr(nd, "\"lenh\":\"gia\"")) {
    if (khiLenhGia) khiLenhGia(laySoJson(nd, "s", 0), laySoJson(nd, "l", 0), laySoJson(nd, "emg", 0), laySoJson(nd, "ut", 0));
  }
}

// Gọi trong loop(), càng thường càng tốt (mỗi khung một lần ở chế độ đếm là đủ).
inline void mangChay() {
  if (mang_co_web) may_chu.handleClient();
  LenhCam g;
  while (hang_doi_lenh_cam && xQueueReceive(hang_doi_lenh_cam, &g, 0) == pdTRUE) mangXuLyLenh(g.nd);
  uint32_t bay_gio = millis();
  if (bay_gio - mang_t_trang_thai >= CHU_KI_TRANG_THAI_MS) { mang_t_trang_thai = bay_gio; mangGuiTrangThai(); }
#if CHAN_LED_DO >= 0
  if (mang_t_nhay) {
    uint32_t da = bay_gio - mang_t_nhay;
    digitalWrite(CHAN_LED_DO, (da < 3000 && (da / 150) % 2 == 0) ? LED_DO_SANG : !LED_DO_SANG);
    if (da >= 3000) mang_t_nhay = 0;
  }
#endif
  if (mang_t_khoi_dong && (int32_t)(bay_gio - mang_t_khoi_dong) >= 0) ESP.restart();
#if NOI_WIFI_TRUNG_TAM
  // Chưa nối (hoặc rớt) mà không đang thử nối thì giữ radio ở KENH_WIFI để ESP-NOW không lạc kênh.
  if (!mang_tu_phat && bay_gio - mang_t_kenh >= 1000) {
    mang_t_kenh = bay_gio;
    if (WiFi.status() == WL_CONNECTED) { mang_kenh = (uint8_t)WiFi.channel(); mang_t_noi = bay_gio; }
    else if (bay_gio - mang_t_noi >= 20000) { mang_t_noi = bay_gio; WiFi.begin(TEN_WIFI_TRUNG_TAM, MAT_KHAU_TRUNG_TAM, mang_kenh); }
    else if (bay_gio - mang_t_noi >= 6000) mangGiuKenh();
  }
#endif
}

// Thêm tiêu đề cho phép trang web của trung tâm (địa chỉ 192.168.4.1) đọc dữ liệu của camera.
inline void mangChoPhepTrungTam() {
  may_chu.sendHeader("Access-Control-Allow-Origin", "*");
}
