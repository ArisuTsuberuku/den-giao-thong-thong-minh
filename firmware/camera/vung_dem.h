// vung_dem.h — Vùng đếm (ROI và 2 tứ giác làn) đang dùng của camera này.
// Bật máy: nếu trong bộ nhớ của board (NVS) có vùng đã lưu từ trang web thì dùng vùng đó, không thì dùng
// dòng CAM <CAM_ID> trong cau_hinh.h. Nhờ vậy chỉnh vùng trên web là có tác dụng ngay, không phải nạp lại.
#pragma once
#include <Preferences.h>
#include "esp_camera.h"
#include "cau_hinh.h"
#include "danh_tinh.h"

static CauHinhCam ch_cam = CAU_HINH_CAM[CAM_ID];       // vungNap() đặt lại theo cam_id
static bool vung_da_luu = false;              // true: đang dùng vùng lưu trong board
// Lật ảnh: mặc định theo LAT_DOC, LAT_NGANG của cau_hinh.h; bấm trên trang web thì lưu vào board.
// Ảnh lộn ngược 180° (trần nhà ở dưới, chữ bị ngược) thì bật cả hai.
static uint8_t lat_doc = LAT_DOC, lat_ngang = LAT_NGANG;
static bool lat_da_luu = false;

#define VUNG_PHIEN_BAN 1

inline void vungNap() {
  ch_cam = CAU_HINH_CAM[cam_id];
  vung_da_luu = false;
  lat_doc = LAT_DOC; lat_ngang = LAT_NGANG; lat_da_luu = false;
  Preferences p;
  if (!p.begin("vung_dem", true)) return;      // chưa từng lưu: chưa có vùng nhớ này
  uint8_t lat = p.getUChar("lat", 0xFF);       // bit 0: lật dọc, bit 1: lật ngang
  if (lat != 0xFF) { lat_doc = lat & 1; lat_ngang = (lat >> 1) & 1; lat_da_luu = true; }
  CauHinhCam c;
  if (p.getUChar("pb", 0) == VUNG_PHIEN_BAN && p.getUChar("cam", 0) == cam_id &&
      p.getBytes("ch", &c, sizeof(c)) == sizeof(c) && vungHopLe(c, ANH_RONG, ANH_CAO)) {
    ch_cam = c;
    vung_da_luu = true;
  }
  p.end();
}

// Lưu và dùng ngay. Trả về false nếu vùng sai hoặc không ghi được.
inline bool vungLuu(const CauHinhCam &c) {
  if (!vungHopLe(c, ANH_RONG, ANH_CAO)) return false;
  Preferences p;
  if (!p.begin("vung_dem", false)) return false;
  bool duoc = p.putBytes("ch", &c, sizeof(c)) == sizeof(c);
  duoc = duoc && p.putUChar("cam", cam_id) && p.putUChar("pb", VUNG_PHIEN_BAN);
  p.end();
  if (duoc) { ch_cam = c; vung_da_luu = true; }
  return duoc;
}

// Lật ảnh ngay trên cảm biến (khung chụp sau đó đã lật) và lưu vào board.
// Đổi lật thì toạ độ ROI và làn cũ không còn đúng: phải vẽ lại vùng đếm.
inline bool latDat(uint8_t doc, uint8_t ngang) {
  lat_doc = doc ? 1 : 0; lat_ngang = ngang ? 1 : 0;
  sensor_t *cb = esp_camera_sensor_get();
  if (cb) { cb->set_vflip(cb, lat_doc); cb->set_hmirror(cb, lat_ngang); }
  Preferences p;
  if (!p.begin("vung_dem", false)) return false;
  bool duoc = p.putUChar("lat", (uint8_t)(lat_doc | (lat_ngang << 1))) == 1;
  p.end();
  lat_da_luu = duoc;
  return duoc;
}

// Xoá vùng và kiểu lật đã lưu, quay về số trong cau_hinh.h.
inline void vungXoa() {
  Preferences p;
  if (p.begin("vung_dem", false)) { p.clear(); p.end(); }
  ch_cam = CAU_HINH_CAM[cam_id];
  vung_da_luu = false;
  lat_da_luu = false;
  lat_doc = LAT_DOC; lat_ngang = LAT_NGANG;
  sensor_t *cb = esp_camera_sensor_get();
  if (cb) { cb->set_vflip(cb, lat_doc); cb->set_hmirror(cb, lat_ngang); }
}
