// nap_tu_xa.h — Nạp firmware (kể cả firmware camera có mô hình nhận diện mới) qua WiFi, có đường lui.
//
// Trung tâm: trên trang web chọn tệp .bin của trung tâm → trung tâm kiểm tra tệp trong lúc nhận (đúng
// firmware ESP32-S3, đúng firmware trung tâm của dự án), ghi vào phân vùng OTA còn trống rồi khởi động lại.
// Camera: chọn tệp .bin của camera → trung tâm kiểm tra rồi cất vào bộ nhớ FFat của nó. Bấm "Nạp" thì trung
// tâm lần lượt ra lệnh cho từng camera (ESP-NOW); camera tự tải tệp từ http://192.168.4.1/fw/cam.bin (kèm
// mã MD5 để camera kiểm tra), ghi vào phân vùng OTA của nó rồi khởi động lại. Xong camera này (camera báo
// đã chạy bản mới và tự xác nhận chạy ổn) mới tới camera sau; một camera lỗi thì dừng, không nạp tiếp.
//
// Đường lui (fallback), từ trong ra ngoài:
//   1. Tệp sai (không phải firmware, sai chip, firmware của board khác, tải lên dở, sai MD5): không ghi gì,
//      bản đang chạy giữ nguyên.
//   2. Bản mới khởi động được nhưng không chạy ổn: bản mới phải tự xác nhận trong 120 giây sau khi bật
//      (trung tâm: WiFi và ESP-NOW chạy, thấy lại camera nếu trước đó có camera; camera: camera chụp được,
//      nối lại được trung tâm, ở chế độ đếm thì mô hình chạy được một khung). Quá hạn là tự quay về bản cũ.
//   3. Bản mới treo hoặc khởi động lại liên tục: bootloader thấy bản mới chưa được xác nhận nên tự quay về
//      bản cũ ở lần khởi động sau (tính năng rollback của ESP-IDF, bật sẵn trong lõi esp32 của Arduino).
//   4. Nút "Quay về bản trước" trên trang web, khi bản mới chạy ổn nhưng thầy trò muốn dùng lại bản cũ.
//   5. Cuối cùng vẫn nạp được bằng cáp USB như mọi khi.
// Cần bảng phân vùng có hai phân vùng ứng dụng (OTA). Trung tâm: "16M Flash (3MB APP/9.9MB FATFS)" (đang
// dùng) có sẵn. Camera ESP32-CAM: phải nạp bằng USB một lần với "Minimal SPIFFS (1.9MB APP with OTA)".
#pragma once
#include <WebServer.h>
#include <Update.h>
#include <FFat.h>
#include <MD5Builder.h>
#include <Preferences.h>
#include <esp_ota_ops.h>
#include <esp_timer.h>
#include "kiem_anh_fw.h"
#include "nhan_tin.h"

// Dấu nhận diện firmware trung tâm, kèm ngày giờ biên dịch làm phiên bản (xem kiem_anh_fw.h).
#define DAU_FW_TRUNG_TAM "DGTTM-FW|trung_tam|"
__attribute__((used)) static const char DAU_FW[] = DAU_FW_TRUNG_TAM __DATE__ " " __TIME__;
inline const char *phienBanFw() { return DAU_FW + sizeof(DAU_FW_TRUNG_TAM) - 1; }

#define NAP_HAN_XAC_NHAN_MS 120000     // bản mới phải tự xác nhận chạy ổn trong chừng này
#define NAP_CHAY_IT_NHAT_MS 30000      // ... và phải chạy được ít nhất chừng này mới được xác nhận

// Arduino tự xác nhận bản mới ngay khi khởi động nếu không có hàm này; ở đây để firmware tự kiểm tra rồi
// mới xác nhận (napXacNhanChay).
extern "C" bool verifyRollbackLater() { return true; }

// ---------- Xác nhận bản mới, quay về bản cũ ----------
static bool nap_cho_xn = false;           // đang chạy bản vừa nạp, chưa xác nhận
static bool nap_co_ban_loi = false;       // có một bản nạp gần đây bị lỗi (board đã quay về bản đang chạy)
static uint8_t nap_so_cam_truoc = 0;      // số camera trung tâm thấy ngay trước khi nạp
static esp_timer_handle_t nap_hen = nullptr;

static void napHetHan(void *) {
  esp_ota_mark_app_invalid_rollback_and_reboot();   // chỉ trả về nếu không có bản cũ để quay về
}

// Gọi sớm trong setup().
inline void napKhoiDong() {
  const esp_partition_t *chay = esp_ota_get_running_partition();
  esp_ota_img_states_t st;
  if (chay && esp_ota_get_state_partition(chay, &st) == ESP_OK && st == ESP_OTA_IMG_PENDING_VERIFY) {
    nap_cho_xn = true;
    Preferences p;
    if (p.begin("nap", true)) { nap_so_cam_truoc = p.getUChar("cam", 0); p.end(); }
    esp_timer_create_args_t a = {};
    a.callback = napHetHan;
    a.name = "nap_han";
    if (esp_timer_create(&a, &nap_hen) == ESP_OK) esp_timer_start_once(nap_hen, (uint64_t)NAP_HAN_XAC_NHAN_MS * 1000ULL);
  }
  nap_co_ban_loi = esp_ota_get_last_invalid_partition() != NULL;
}

// Gọi trong loop(). khoe: lúc này board đang chạy ổn. Trả về true đúng lần vừa xác nhận.
inline bool napXacNhanChay(bool khoe) {
  if (!nap_cho_xn || !khoe || millis() < NAP_CHAY_IT_NHAT_MS) return false;
  if (esp_ota_mark_app_valid_cancel_rollback() != ESP_OK) return false;
  if (nap_hen) esp_timer_stop(nap_hen);
  nap_cho_xn = false;
  return true;
}

// Chọn bản ở phân vùng kia để khởi động lần sau. Trả về NULL nếu được, không thì lí do.
inline const char *napQuayVe() {
  const esp_partition_t *kia = esp_ota_get_next_update_partition(NULL);
  esp_app_desc_t mo_ta;
  if (!kia || esp_ota_get_partition_description(kia, &mo_ta) != ESP_OK) return "Không có bản trước để quay về.";
  esp_ota_img_states_t st;
  if (esp_ota_get_state_partition(kia, &st) == ESP_OK && (st == ESP_OTA_IMG_INVALID || st == ESP_OTA_IMG_ABORTED))
    return "Bản trước đã bị đánh dấu lỗi, không quay về được.";
  if (esp_ota_set_boot_partition(kia) != ESP_OK) return "Bản trước bị hỏng.";
  return NULL;
}

inline bool napCoOta() { return esp_ota_get_next_update_partition(NULL) != NULL; }

// ---------- Nhận firmware qua trang web ----------
static KiemAnhFw nap_kiem;
static String nap_loi;
static bool nap_dang_nhan = false;

// Thông tin tệp firmware camera đang cất trong FFat
struct AnhCam { bool co; uint32_t kt; char md5[33]; int chip; char pb[28]; };
static AnhCam anh_cam = {false, 0, "", -1, ""};
static File nap_tep;
static MD5Builder nap_md5;
static bool nap_fat_ok = false, nap_fat_thu = false;

inline bool napFat() {
  if (!nap_fat_thu) {
    nap_fat_thu = true;
    nap_fat_ok = FFat.begin(true);         // lần đầu: định dạng phân vùng FFat (mất vài giây)
    if (nap_fat_ok && FFat.exists("/cam.txt")) {
      File f = FFat.open("/cam.txt", "r");
      if (f) {
        String s = f.readString();
        f.close();
        AnhCam a = {false, 0, "", -1, ""};
        if (sscanf(s.c_str(), "%lu %32s %d %27[^\n]", (unsigned long *)&a.kt, a.md5, &a.chip, a.pb) >= 3 && FFat.exists("/cam.bin")) {
          a.co = true;
          anh_cam = a;
        }
      }
    }
  }
  return nap_fat_ok;
}

// Tệp firmware trung tâm: ghi thẳng vào phân vùng OTA.
static void napNhanTrungTam(WebServer &web) {
  HTTPUpload &u = web.upload();
  if (u.status == UPLOAD_FILE_START) {
    nap_loi = "";
    nap_dang_nhan = true;
    nap_kiem.batDau(CONFIG_IDF_FIRMWARE_CHIP_ID, "mat_gnurt");
    if (!napCoOta()) nap_loi = "Bảng phân vùng không có chỗ nạp qua WiFi: nạp bằng USB với Partition Scheme có OTA.";
    else if (!Update.begin(UPDATE_SIZE_UNKNOWN)) nap_loi = String("Không bắt đầu nạp được: ") + Update.errorString();
  } else if (u.status == UPLOAD_FILE_WRITE) {
    if (nap_loi.length()) return;
    nap_kiem.them(u.buf, u.currentSize);
    int k = nap_kiem.ketQuaSom();
    if (k) { nap_loi = String("Tệp sai: ") + tenKetQuaKiemAnh(k); Update.abort(); return; }
    if (Update.write(u.buf, u.currentSize) != u.currentSize) { nap_loi = String("Ghi lỗi: ") + Update.errorString(); Update.abort(); }
  } else if (u.status == UPLOAD_FILE_END) {
    nap_dang_nhan = false;
    if (nap_loi.length()) return;
    int k = nap_kiem.ketQua();
    if (k) { nap_loi = String("Tệp sai: ") + tenKetQuaKiemAnh(k); Update.abort(); return; }
    if (!Update.end(true)) { nap_loi = String("Tệp không hợp lệ: ") + Update.errorString(); return; }
    uint8_t so_cam = 0;
    for (int h = 1; h <= 4; h++) if (tt_cam[h].co && millis() - tt_cam[h].t < 10000) so_cam++;
    Preferences p;
    if (p.begin("nap", false)) { p.putUChar("cam", so_cam); p.end(); }
  } else if (u.status == UPLOAD_FILE_ABORTED) {
    nap_dang_nhan = false;
    Update.abort();
    nap_loi = "Tải lên bị ngắt giữa chừng, chưa ghi gì.";
  }
}

// Tệp firmware camera: kiểm tra rồi cất vào FFat (/cam.bin), kèm kích thước, MD5, loại chip, phiên bản.
static void napNhanAnhCam(WebServer &web) {
  HTTPUpload &u = web.upload();
  if (u.status == UPLOAD_FILE_START) {
    nap_loi = "";
    nap_dang_nhan = true;
    nap_kiem.batDau(CHIP_BAT_KI, "aremac");
    nap_md5.begin();
    if (!napFat()) { nap_loi = "Trung tâm không có phân vùng FFat (cần Partition Scheme 16M Flash (3MB APP/9.9MB FATFS)). Hãy nạp thẳng trên trang của từng camera."; return; }
    anh_cam.co = false;
    FFat.remove("/cam.txt");
    nap_tep = FFat.open("/cam.tam", "w");
    if (!nap_tep) nap_loi = "Không ghi được vào FFat.";
  } else if (u.status == UPLOAD_FILE_WRITE) {
    if (nap_loi.length()) return;
    nap_kiem.them(u.buf, u.currentSize);
    int k = nap_kiem.ketQuaSom();
    if (k) { nap_loi = String("Tệp sai: ") + tenKetQuaKiemAnh(k); nap_tep.close(); FFat.remove("/cam.tam"); return; }
    nap_md5.add(u.buf, u.currentSize);
    if (nap_tep.write(u.buf, u.currentSize) != u.currentSize) { nap_loi = "FFat đầy hoặc ghi lỗi."; nap_tep.close(); FFat.remove("/cam.tam"); }
  } else if (u.status == UPLOAD_FILE_END) {
    nap_dang_nhan = false;
    if (nap_loi.length()) return;
    nap_tep.close();
    int k = nap_kiem.ketQua();
    if (k) { nap_loi = String("Tệp sai: ") + tenKetQuaKiemAnh(k); FFat.remove("/cam.tam"); return; }
    nap_md5.calculate();
    AnhCam a;
    a.co = true; a.kt = (uint32_t)nap_kiem.daNhan(); a.chip = nap_kiem.chip();
    snprintf(a.md5, sizeof(a.md5), "%s", nap_md5.toString().c_str());
    snprintf(a.pb, sizeof(a.pb), "%s", nap_kiem.phienBan());
    FFat.remove("/cam.bin");
    if (!FFat.rename("/cam.tam", "/cam.bin")) { nap_loi = "Không đổi tên được tệp trong FFat."; return; }
    File f = FFat.open("/cam.txt", "w");
    if (f) { f.printf("%lu %s %d %s\n", (unsigned long)a.kt, a.md5, a.chip, a.pb); f.close(); }
    anh_cam = a;
  } else if (u.status == UPLOAD_FILE_ABORTED) {
    nap_dang_nhan = false;
    if (nap_tep) nap_tep.close();
    FFat.remove("/cam.tam");
    nap_loi = "Tải lên bị ngắt giữa chừng.";
  }
}

// GET /fw/cam.bin: camera tải về. Gửi kèm x-MD5 để thư viện HTTPUpdate trên camera kiểm tra.
static void napGuiAnhCam(WebServer &web) {
  if (!napFat() || !anh_cam.co) { web.send(404, "text/plain", "chua co firmware camera"); return; }
  File f = FFat.open("/cam.bin", "r");
  if (!f) { web.send(404, "text/plain", "khong mo duoc"); return; }
  web.sendHeader("x-MD5", anh_cam.md5);
  web.streamFile(f, "application/octet-stream");
  f.close();
}

// ---------- Nạp lần lượt cho các camera ----------
#define NC_KHONG 0
#define NC_CHO 1          // chờ tới lượt
#define NC_DA_LENH 2      // đã ra lệnh, chờ camera bắt đầu tải
#define NC_DANG_TAI 3
#define NC_KHOI_DONG 4    // tải xong, chờ camera chạy bản mới và tự xác nhận
#define NC_XONG 5
#define NC_LOI 6
#define NC_DUNG 7         // không nạp vì camera trước lỗi
static const char *const TEN_NC[] = {"", "chờ lượt", "đã ra lệnh", "đang tải", "đang khởi động bản mới", "xong", "lỗi", "đã dừng"};

struct NapCam { uint8_t tt; uint32_t t; uint8_t lan_lenh; uint32_t up0; char ghi[96]; };
static NapCam nap_cam[5];

inline const char *loiTaiCam(int ma) {
  switch (ma) {
    case -100: return "camera không đủ chỗ: phải nạp USB một lần với Minimal SPIFFS (1.9MB APP with OTA)";
    case -101: return "trung tâm không báo kích thước tệp";
    case -102: return "trung tâm chưa có tệp firmware camera";
    case -104: return "trung tâm trả lời sai";
    case -105: return "sai MD5";
    case -106: return "tệp không phải firmware";
    case -107: return "tệp cho bộ nhớ flash khác";
    case -108: return "camera không có phân vùng OTA: nạp USB một lần với Minimal SPIFFS (1.9MB APP with OTA)";
    case -1: return "không nối được trung tâm";
    case -11: return "hết giờ khi tải";
    default: return "tải lỗi (xem Serial của camera)";
  }
}

// Xếp lịch nạp. den = 0: mọi camera đang nối WiFi trung tâm. Trả về số camera được xếp, hoặc -1 kèm lí do.
inline int napXepLich(int den, String &ly_do) {
  if (!anh_cam.co) { ly_do = "Chưa tải firmware camera lên trung tâm."; return -1; }
  for (int h = 1; h <= 4; h++)
    if (nap_cam[h].tt >= NC_CHO && nap_cam[h].tt <= NC_KHOI_DONG) { ly_do = "Đang nạp dở, chờ xong đã."; return -1; }
  int so = 0;
  ly_do = "";
  for (int h = 1; h <= 4; h++) {
    if (den != 0 && den != h) continue;
    const TrangThaiCam &c = tt_cam[h];
    nap_cam[h].ghi[0] = 0;
    if (!c.co || millis() - c.t > 10000 || c.rs == 0) {
      if (den) ly_do = "CAM " + String(h) + " chưa nối WiFi trung tâm.";
      continue;
    }
    if (c.ch >= 0 && c.ch != anh_cam.chip) { ly_do += "CAM " + String(h) + " khác loại chip với tệp. "; continue; }
    nap_cam[h].tt = NC_CHO; nap_cam[h].lan_lenh = 0;
    so++;
  }
  if (!so && !ly_do.length()) ly_do = "Không có camera nào nối WiFi trung tâm.";
  return so ? so : -1;
}

inline void napHuyLich() {
  for (int h = 1; h <= 4; h++) if (nap_cam[h].tt == NC_CHO) nap_cam[h].tt = NC_KHONG;
}

// Gọi trong loop(). bao(h, câu): báo một dòng ra nhật ký.
inline void napLichChay(void (*bao)(int, const char *)) {
  int dang = 0;
  for (int h = 1; h <= 4; h++) if (nap_cam[h].tt >= NC_DA_LENH && nap_cam[h].tt <= NC_KHOI_DONG) dang = h;
  uint32_t bay_gio = millis();
  if (!dang) {                                    // tới lượt camera kế tiếp
    for (int h = 1; h <= 4; h++) {
      if (nap_cam[h].tt != NC_CHO) continue;
      char them[40];
      snprintf(them, sizeof(them), "\"kt\":%lu", (unsigned long)anh_cam.kt);
      guiLenhCam(h, "nap", them);
      nap_cam[h].tt = NC_DA_LENH; nap_cam[h].t = bay_gio; nap_cam[h].lan_lenh = 1;
      bao(h, "trung tâm ra lệnh nạp firmware mới");
      break;
    }
    return;
  }
  NapCam &n = nap_cam[dang];
  const TrangThaiCam &c = tt_cam[dang];
  bool loi = false;
  switch (n.tt) {
    case NC_DA_LENH:
      if (c.nap == 1 && c.t > n.t) { n.tt = NC_DANG_TAI; n.t = bay_gio; bao(dang, "camera bắt đầu tải"); }
      else if (c.nap == 3 && c.t > n.t) { snprintf(n.ghi, sizeof(n.ghi), "%s", loiTaiCam(c.nl)); loi = true; }
      else if (bay_gio - n.t > 15000 && n.lan_lenh < 3) {           // gói phát chung có thể lạc: gửi lại
        char them[40];
        snprintf(them, sizeof(them), "\"kt\":%lu", (unsigned long)anh_cam.kt);
        guiLenhCam(dang, "nap", them);
        n.lan_lenh++; n.t = bay_gio;
      } else if (bay_gio - n.t > 15000) { snprintf(n.ghi, sizeof(n.ghi), "camera không nhận lệnh nạp"); loi = true; }
      break;
    case NC_DANG_TAI:
      if (c.nap == 2) { n.tt = NC_KHOI_DONG; n.t = bay_gio; n.up0 = c.up; bao(dang, "camera tải xong, đang khởi động bản mới"); }
      else if (c.nap == 3) { snprintf(n.ghi, sizeof(n.ghi), "%s", loiTaiCam(c.nl)); loi = true; }
      else if (bay_gio - n.t > 180000) { snprintf(n.ghi, sizeof(n.ghi), "tải quá 3 phút"); loi = true; }
      break;
    case NC_KHOI_DONG:
      if (c.t > n.t && c.up < n.up0 && c.nap != 2) {              // tin từ lần bật máy mới (đồng hồ chạy lại từ 0)
        n.up0 = 0xFFFFFFFFu;                                       // đã thấy camera khởi động lại: từ giờ không xét đồng hồ nữa
        bool ban_moi = strcmp(c.fw, anh_cam.pb) == 0;
        if (ban_moi && c.xn) { n.tt = NC_XONG; snprintf(n.ghi, sizeof(n.ghi), "chạy bản %s", anh_cam.pb); bao(dang, "camera đã chạy ổn bản mới"); }
        else if (!ban_moi && c.up > 5) { snprintf(n.ghi, sizeof(n.ghi), "bản mới không chạy ổn, camera đã quay về bản %s", c.fw); loi = true; }
      }
      if (n.tt == NC_KHOI_DONG && bay_gio - n.t > 240000) { snprintf(n.ghi, sizeof(n.ghi), "không thấy camera chạy lại sau 4 phút"); loi = true; }
      break;
  }
  if (loi) {
    n.tt = NC_LOI;
    bao(dang, n.ghi);
    for (int h = 1; h <= 4; h++) if (nap_cam[h].tt == NC_CHO) { nap_cam[h].tt = NC_DUNG; snprintf(nap_cam[h].ghi, sizeof(nap_cam[h].ghi), "dừng vì CAM %d lỗi", dang); }
  }
}

// JSON cho trang web.
inline size_t napJson(char *b, size_t co) {
  size_t n = 0;
#define THEM_N(...) do { if (n < co) n += snprintf(b + n, co - n, __VA_ARGS__); } while (0)
  const esp_partition_t *chay = esp_ota_get_running_partition();
  THEM_N("{\"fw\":\"%s\",\"pv\":\"%s\",\"ota\":%d,\"xn\":%d,\"loi\":%d,\"cam\":{\"co\":%d,\"kt\":%lu,\"chip\":%d,\"pb\":\"%s\"},\"c\":[",
         phienBanFw(), chay ? chay->label : "", napCoOta() ? 1 : 0, nap_cho_xn ? 0 : 1, nap_co_ban_loi ? 1 : 0,
         anh_cam.co ? 1 : 0, (unsigned long)anh_cam.kt, anh_cam.chip, anh_cam.pb);
  for (int h = 1; h <= 4; h++) {
    const NapCam &x = nap_cam[h];
    const TrangThaiCam &c = tt_cam[h];
    THEM_N("%s{\"tt\":%u,\"ten\":\"%s\",\"ghi\":\"%s\",\"np\":%d}", h > 1 ? "," : "", x.tt, TEN_NC[x.tt], x.ghi,
           x.tt == NC_DANG_TAI ? c.np : 0);
  }
  THEM_N("]}");
#undef THEM_N
  return n;
}
