// danh_tinh.h — Số CAM và chế độ chạy của board này, lưu trong bộ nhớ của board (NVS).
// Nhờ vậy một tệp firmware dùng chung cho cả 4 camera khi nạp qua WiFi.
//
// Luật chọn lúc bật máy:
//   - Vừa nạp qua WiFi (cờ "ota_moi"): giữ số CAM và chế độ đang lưu, không lấy số trong code.
//   - Nạp bằng USB với CAM_ID hoặc CHE_DO trong cau_hinh.h khác lần nạp USB trước: lấy số trong code
//     (thầy trò sửa code là muốn đổi). Không đổi gì trong code thì giữ số đang lưu.
//   - Đổi trên trang web (lệnh "doiid", "chedo"): lưu số mới rồi khởi động lại.
#pragma once
#include <Preferences.h>
#include "cau_hinh.h"

static uint8_t cam_id = CAM_ID;              // số CAM đang dùng (1..4)
static uint8_t che_do = CHE_DO;              // chế độ đang dùng (CHE_DO_THU_ANH, CHE_DO_DEM, CHE_DO_GIA_LAP)
static bool che_do_bi_doi = false;           // chọn chế độ đếm mà firmware chưa có mô hình: chạy thu ảnh

inline bool cheDoHopLe(int cd) { return cd == CHE_DO_THU_ANH || cd == CHE_DO_DEM || cd == CHE_DO_GIA_LAP; }

inline void danhTinhNap() {
  Preferences p;
  if (p.begin("danh_tinh", false)) {
    bool ota = p.getUChar("ota_moi", 0) != 0;
    uint8_t id = p.getUChar("id", 0), id_nap = p.getUChar("id_nap", 0);
    uint8_t cd = p.getUChar("cd", 0), cd_nap = p.getUChar("cd_nap", 0);
    if (ota) {
      p.putUChar("ota_moi", 0);
      p.putUChar("id_nap", CAM_ID);              // lần nạp USB sau mới so với số này
      p.putUChar("cd_nap", CHE_DO);
    } else {
      if (id_nap != CAM_ID) { id = CAM_ID; p.putUChar("id_nap", CAM_ID); }
      if (cd_nap != CHE_DO) { cd = CHE_DO; p.putUChar("cd_nap", CHE_DO); }
    }
    if (id < 1 || id > 4) id = CAM_ID;
    if (!cheDoHopLe(cd)) cd = CHE_DO;
    p.putUChar("id", id);
    p.putUChar("cd", cd);
    p.end();
    cam_id = id;
    che_do = cd;
  }
#if !CO_MO_HINH
  if (che_do == CHE_DO_DEM) { che_do = CHE_DO_THU_ANH; che_do_bi_doi = true; }
#endif
}

inline bool danhTinhDat(const char *khoa, uint8_t gia_tri) {
  Preferences p;
  if (!p.begin("danh_tinh", false)) return false;
  bool duoc = p.putUChar(khoa, gia_tri) == 1;
  p.end();
  return duoc;
}

// Gọi ngay trước khi khởi động lại vào bản vừa nạp qua WiFi.
inline void danhTinhDanhDauNapXa() { danhTinhDat("ota_moi", 1); }
