// cau_hinh.h — Mọi thứ cần chỉnh của camera nằm ở đây. Một firmware dùng chung cho 4 board,
// mỗi board chỉ khác CAM_ID và các số ROI, tứ giác làn của nó.
// Board: ESP32-S3-CAM (cảm biến OV2640, có đế nạp). Cách chọn trong Arduino IDE: xem README.md.
#pragma once
#include <Arduino.h>
#include "lan_duong.h"

// ---------- 1) Board này gắn ở hướng nào ----------
#ifndef CAM_ID
#define CAM_ID 1                 // 1..4, trùng với số hướng và số cột Đ1..Đ4
#endif

// ---------- 2) Chế độ chạy ----------
#define CHE_DO_THU_ANH 1         // phát WiFi, xem ảnh trên trình duyệt, lưu ảnh huấn luyện, chỉnh ROI và làn
#define CHE_DO_DEM 2             // đếm xe theo làn bằng mô hình Edge Impulse, gửi ESP-NOW (cần mục 6)
#define CHE_DO_GIA_LAP 3         // không cần camera: gửi số đếm gõ từ Serial, để thử đường truyền
#ifndef CHE_DO
#define CHE_DO CHE_DO_THU_ANH
#endif

// ---------- 3) Truyền tin ----------
#define KENH_WIFI 1              // kênh ESP-NOW, phải trùng với KENH_WIFI của board trung tâm
#define CHU_KI_GUI_MS 250        // 4 gói mỗi giây
#define TOC_DO_SERIAL 115200
#define MAT_KHAU_WIFI "12345678" // chế độ thu ảnh: camera phát WiFi tên "CAM<số>-SaBan", vào http://192.168.4.1

// ---------- 4) Chân camera của board ESP32-S3-CAM (trùng với kiểu ESP32-S3-EYE) ----------
#define CHAN_XCLK 15
#define CHAN_SIOD 4
#define CHAN_SIOC 5
#define CHAN_Y9 16
#define CHAN_Y8 17
#define CHAN_Y7 18
#define CHAN_Y6 12
#define CHAN_Y5 10
#define CHAN_Y4 8
#define CHAN_Y3 9
#define CHAN_Y2 11
#define CHAN_VSYNC 6
#define CHAN_HREF 7
#define CHAN_PCLK 13
#define CHAN_PWDN -1
#define CHAN_RESET -1
#define TAN_SO_XCLK 20000000
#define LAT_DOC 0                // 1 nếu ảnh bị lộn ngược
#define LAT_NGANG 0              // 1 nếu ảnh bị ngược trái phải

// ---------- 5) Vùng quan sát: ROI và 2 tứ giác làn, tính bằng điểm ảnh trên khung 320 × 240 ----------
#define ANH_RONG 320
#define ANH_CAO 240

// Mỗi dòng: { roi_x, roi_y, roi_w, roi_h, {4 góc làn ngoài}, {4 góc làn trong} }  (kiểu CauHinhCam trong lan_duong.h)
// Các số dưới đây chỉ là giá trị tạm. Chạy chế độ thu ảnh, vẽ ROI và bấm 4 góc từng làn trên
// trang web của camera, rồi dán dòng nó in ra vào đúng ô của camera đó.
static const CauHinhCam CAU_HINH_CAM[5] = {
  { 0, 0, 0, 0, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}} },                          // không dùng
  { 100, 0, 120, 240, {{100, 0}, {160, 0}, {160, 239}, {100, 239}}, {{160, 0}, {220, 0}, {220, 239}, {160, 239}} },   // CAM 1
  { 100, 0, 120, 240, {{100, 0}, {160, 0}, {160, 239}, {100, 239}}, {{160, 0}, {220, 0}, {220, 239}, {160, 239}} },   // CAM 2
  { 100, 0, 120, 240, {{100, 0}, {160, 0}, {160, 239}, {100, 239}}, {{160, 0}, {220, 0}, {220, 239}, {160, 239}} },   // CAM 3
  { 100, 0, 120, 240, {{100, 0}, {160, 0}, {160, 239}, {100, 239}}, {{160, 0}, {220, 0}, {220, 239}, {160, 239}} }    // CAM 4
};

// ---------- 6) Mô hình Edge Impulse (chỉ dùng ở chế độ đếm) ----------
// Sau khi huấn luyện, xuất "Arduino library", cài tệp .zip vào Arduino IDE rồi sửa tên dưới đây
// cho đúng tên tệp .h của thư viện (xem File > Examples, tên có đuôi _inferencing).
#ifndef THU_VIEN_EI
#define THU_VIEN_EI <den_giao_thong_inferencing.h>
#endif
#define NGUONG_TIN_CAY 0.5f          // bỏ các ô mô hình không chắc chắn
#define NHAN_XE_UU_TIEN "xe_uu_tien" // tên lớp xe ưu tiên lúc gán nhãn; các lớp khác đều tính là xe thường
#define SO_XE_TOI_DA 24
