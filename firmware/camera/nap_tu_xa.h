// nap_tu_xa.h — Nạp firmware camera qua WiFi (kể cả firmware có mô hình nhận diện mới), có đường lui.
//
// Hai đường nạp:
//   1. Qua trung tâm (cách chính): thầy tải tệp .bin lên trang của trung tâm, trung tâm ra lệnh "nap" qua
//      ESP-NOW; camera tự tải http://<trung tâm>/fw/cam.bin (thư viện HTTPUpdate, có kiểm tra MD5) rồi ghi
//      vào phân vùng OTA còn trống và khởi động lại. Camera báo tiến độ trong tin trạng thái.
//   2. Thẳng trên trang của camera (khi không có trung tâm, camera tự phát WiFi "CAM<số>-SaBan"): chọn tệp,
//      camera kiểm tra tệp trong lúc nhận (đúng chip, đúng firmware camera của dự án) rồi ghi.
//
// Đường lui: bản mới phải tự xác nhận chạy ổn trong 120 giây (camera chụp được; nối lại được trung tâm nếu
// lúc nạp đang nối; ở chế độ đếm thì mô hình chạy được một khung). Quá hạn, hoặc treo, hoặc khởi động lại
// trước khi xác nhận, thì board tự quay về bản cũ (rollback của bootloader). Tệp sai thì không ghi gì.
// Cần bảng phân vùng có OTA: ESP32-CAM chọn "Minimal SPIFFS (1.9MB APP with OTA)" khi nạp USB lần đầu.
#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPUpdate.h>
#include <Update.h>
#include <Preferences.h>
#include <esp_ota_ops.h>
#include <esp_timer.h>
#include "kiem_anh_fw.h"
#include "danh_tinh.h"

#define DAU_FW_CAMERA "DGTTM-FW|camera|"
__attribute__((used)) static const char DAU_FW[] = DAU_FW_CAMERA __DATE__ " " __TIME__;
inline const char *phienBanFw() { return DAU_FW + sizeof(DAU_FW_CAMERA) - 1; }

#define NAP_HAN_XAC_NHAN_MS 120000
#define NAP_CHAY_IT_NHAT_MS 20000

extern "C" bool verifyRollbackLater() { return true; }

// Tình trạng nạp, báo về trung tâm trong tin trạng thái
static uint8_t nap_tt = 0;          // 0 không, 1 đang tải, 2 tải xong sắp khởi động lại, 3 lỗi
static uint8_t nap_pt = 0;          // phần trăm đã tải
static int nap_ma_loi = 0;
static bool nap_cho_xn = false, nap_co_ban_loi = false, nap_can_trung_tam = false;
static esp_timer_handle_t nap_hen = nullptr;
// Các chế độ đặt hai cờ này khi phần của mình chạy được (dùng để xác nhận bản mới)
static bool khoe_may_anh = false, khoe_mo_hinh = false;

static void napHetHan(void *) { esp_ota_mark_app_invalid_rollback_and_reboot(); }

inline void napKhoiDong() {
  const esp_partition_t *chay = esp_ota_get_running_partition();
  esp_ota_img_states_t st;
  if (chay && esp_ota_get_state_partition(chay, &st) == ESP_OK && st == ESP_OTA_IMG_PENDING_VERIFY) {
    nap_cho_xn = true;
    Preferences p;
    if (p.begin("nap", true)) { nap_can_trung_tam = p.getUChar("tt", 0) != 0; p.end(); }
    esp_timer_create_args_t a = {};
    a.callback = napHetHan;
    a.name = "nap_han";
    if (esp_timer_create(&a, &nap_hen) == ESP_OK) esp_timer_start_once(nap_hen, (uint64_t)NAP_HAN_XAC_NHAN_MS * 1000ULL);
  }
  nap_co_ban_loi = esp_ota_get_last_invalid_partition() != NULL;
}

// Gọi trong loop(). Trả về true đúng lần vừa xác nhận.
inline bool napXacNhanChay(bool co_espnow) {
  if (!nap_cho_xn || millis() < NAP_CHAY_IT_NHAT_MS) return false;
  bool khoe = co_espnow;
  if (che_do != CHE_DO_GIA_LAP) khoe = khoe && khoe_may_anh;
  if (che_do == CHE_DO_DEM) khoe = khoe && khoe_mo_hinh;
  if (nap_can_trung_tam) khoe = khoe && WiFi.status() == WL_CONNECTED;
  if (!khoe || esp_ota_mark_app_valid_cancel_rollback() != ESP_OK) return false;
  if (nap_hen) esp_timer_stop(nap_hen);
  nap_cho_xn = false;
  return true;
}

inline bool napCoOta() { return esp_ota_get_next_update_partition(NULL) != NULL; }

// Đánh dấu trước khi khởi động lại vào bản mới: giữ số CAM, chế độ; ghi là có cần nối trung tâm không.
inline void napChuanBiKhoiDong(bool tu_trung_tam) {
  danhTinhDanhDauNapXa();
  Preferences p;
  if (p.begin("nap", false)) { p.putUChar("tt", tu_trung_tam ? 1 : 0); p.end(); }
}

// ---------- 1. Tải từ trung tâm ----------
static void (*nap_bao)() = nullptr;          // gửi tin trạng thái (mang.h gán)
static uint32_t nap_t_bao = 0;

static void napTienDo(int da, int tong) {
  if (tong <= 0) return;
  nap_pt = (uint8_t)((int64_t)da * 100 / tong);
  if (nap_bao && millis() - nap_t_bao >= 1500) { nap_t_bao = millis(); nap_bao(); }
}

// Chặn cho tới khi tải xong (vài chục giây). Thành công thì khởi động lại vào bản mới, không trả về.
inline void napTaiTuTrungTam() {
  nap_tt = 1; nap_pt = 0; nap_ma_loi = 0;
  if (nap_bao) nap_bao();
  if (!napCoOta()) {
    nap_tt = 3; nap_ma_loi = -108;
    Serial.println("Không nạp được: bảng phân vùng không có OTA (nạp USB một lần với Minimal SPIFFS (1.9MB APP with OTA)).");
    if (nap_bao) nap_bao();
    return;
  }
  String url = "http://" + WiFi.gatewayIP().toString() + "/fw/cam.bin";
  Serial.printf("Tải firmware mới từ %s ...\n", url.c_str());
  WiFiClient kn;
  httpUpdate.rebootOnUpdate(false);
  httpUpdate.onProgress(napTienDo);
  t_httpUpdate_return kq = httpUpdate.update(kn, url);
  if (kq == HTTP_UPDATE_OK) {
    nap_tt = 2; nap_pt = 100;
    napChuanBiKhoiDong(true);
    Serial.println("Tải xong, khởi động lại vào bản mới.");
    for (int i = 0; i < 3 && nap_bao; i++) { nap_bao(); delay(150); }
    delay(300);
    ESP.restart();
  }
  nap_tt = 3;
  nap_ma_loi = httpUpdate.getLastError();
  if (nap_ma_loi == 0) nap_ma_loi = -1;
  Serial.printf("Tải lỗi %d: %s. Vẫn chạy bản cũ.\n", nap_ma_loi, httpUpdate.getLastErrorString().c_str());
  if (nap_bao) nap_bao();
}

// ---------- 2. Nạp thẳng trên trang của camera ----------
static KiemAnhFw nap_kiem;
static String nap_loi;

inline void napNhanTrucTiep(WebServer &web) {
  HTTPUpload &u = web.upload();
  if (u.status == UPLOAD_FILE_START) {
    nap_loi = "";
    nap_kiem.batDau(CONFIG_IDF_FIRMWARE_CHIP_ID, "aremac");
    if (!napCoOta()) nap_loi = "Bảng phân vùng không có OTA: nạp USB một lần với Minimal SPIFFS (1.9MB APP with OTA).";
    else if (!Update.begin(UPDATE_SIZE_UNKNOWN)) nap_loi = String("Không bắt đầu nạp được: ") + Update.errorString();
  } else if (u.status == UPLOAD_FILE_WRITE) {
    if (nap_loi.length()) return;
    nap_kiem.them(u.buf, u.currentSize);
    int k = nap_kiem.ketQuaSom();
    if (k) { nap_loi = String("Tệp sai: ") + tenKetQuaKiemAnh(k); Update.abort(); return; }
    if (Update.write(u.buf, u.currentSize) != u.currentSize) { nap_loi = String("Ghi lỗi: ") + Update.errorString(); Update.abort(); }
  } else if (u.status == UPLOAD_FILE_END) {
    if (nap_loi.length()) return;
    int k = nap_kiem.ketQua();
    if (k) { nap_loi = String("Tệp sai: ") + tenKetQuaKiemAnh(k); Update.abort(); return; }
    if (!Update.end(true)) nap_loi = String("Tệp không hợp lệ: ") + Update.errorString();
  } else if (u.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    nap_loi = "Tải lên bị ngắt giữa chừng, chưa ghi gì.";
  }
}

inline void napTrucTiepXong(WebServer &web) {
  if (nap_loi.length()) { web.send(400, "text/plain; charset=utf-8", nap_loi); return; }
  web.send(200, "text/plain; charset=utf-8", String("Đã nạp bản ") + nap_kiem.phienBan() +
           ". Camera khởi động lại; bản mới phải tự xác nhận chạy ổn trong 2 phút, không thì tự quay về bản cũ.");
  napChuanBiKhoiDong(WiFi.status() == WL_CONNECTED);
  delay(800);
  ESP.restart();
}
