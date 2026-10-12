// cau_hinh.h — Mọi thứ cần chỉnh của camera nằm ở đây. Một firmware dùng chung cho 4 board,
// mỗi board chỉ khác CAM_ID và các số ROI, tứ giác làn của nó.
// Board: ESP32-CAM loại AI-Thinker (chip ESP32, cảm biến OV2640, đế nạp ESP32-CAM-MB). Vẫn dùng được
// với ESP32-S3-CAM nếu đổi LOAI_BOARD ở mục 4. Cách chọn trong Arduino IDE: xem README.md.
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
#define CHU_KI_GUI_MS 250        // gửi kết quả ngay sau mỗi khung, nhưng không dày hơn 4 gói mỗi giây. Mô hình chạy
                                 // chừng 1 khung/giây nên thực tế là 1 gói mỗi giây (trung tâm báo mất camera sau 3 s)
#define SO_KHUNG_LOC 3           // lọc nhiễu qua 3 khung gần nhất (1: không lọc, 3 hoặc 5). Ở 1 khung/giây, mỗi 2 khung
                                 // lọc thêm là trễ thêm 1 giây; mô phỏng cho thấy 3 khung là hợp
#define TOC_DO_SERIAL 115200
#define MAT_KHAU_WIFI "12345678" // chế độ thu ảnh: camera phát WiFi tên "CAM<số>-SaBan", vào http://192.168.4.1

// ---------- 4) Loại board và chân camera ----------
#define BOARD_ESP32_CAM 1        // ESP32-CAM AI-Thinker (chip ESP32): board của dự án
#define BOARD_ESP32S3_CAM 2      // ESP32-S3-CAM (chân trùng kiểu ESP32-S3-EYE): chạy mô hình nhanh hơn khoảng 6 lần
#ifndef LOAI_BOARD
#define LOAI_BOARD BOARD_ESP32_CAM
#endif

#if LOAI_BOARD == BOARD_ESP32_CAM
#define CHAN_PWDN 32
#define CHAN_RESET -1
#define CHAN_XCLK 0
#define CHAN_SIOD 26
#define CHAN_SIOC 27
#define CHAN_Y9 35
#define CHAN_Y8 34
#define CHAN_Y7 39
#define CHAN_Y6 36
#define CHAN_Y5 21
#define CHAN_Y4 19
#define CHAN_Y3 18
#define CHAN_Y2 5
#define CHAN_VSYNC 25
#define CHAN_HREF 23
#define CHAN_PCLK 22
#define CHAN_DEN_FLASH 4         // đèn LED trắng rất sáng trên board: giữ tắt để không hắt sáng lên sa bàn
#define LOI_PSRAM "KHÔNG THẤY PSRAM: kiểm tra đã chọn Board \"AI Thinker ESP32-CAM\" rồi nạp lại."
#else
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
#define CHAN_DEN_FLASH -1
#define LOI_PSRAM "CHƯA BẬT PSRAM: vào Tools > PSRAM chọn \"OPI PSRAM\" rồi nạp lại."
#endif
#define TAN_SO_XCLK 20000000
#define LAT_DOC 0                // 1 nếu ảnh bị lộn ngược
#define LAT_NGANG 0              // 1 nếu ảnh bị ngược trái phải

// ---------- 5) Vùng quan sát: ROI và 2 tứ giác làn, tính bằng điểm ảnh trên khung 320 × 240 ----------
#define ANH_RONG 320
#define ANH_CAO 240

// Mỗi dòng: { roi_x, roi_y, roi_w, roi_h, {4 góc làn ngoài}, {4 góc làn trong} }  (kiểu CauHinhCam trong lan_duong.h)
//
// CÁCH GẮN BOARD: cạnh dài của khung hình (320 điểm ảnh) phải nằm DỌC theo đường. Ống kính thường của
// OV2640 nhìn được 55° theo cạnh dài nhưng chỉ 43° theo cạnh ngắn; từ độ cao 285 mm, chúc 26°, cạnh dài
// dọc đường thì thấy trọn vùng chờ 304 mm, còn cạnh ngắn dọc đường thì mất hẳn chiếc xe đứng sát vạch dừng
// (tính trong web/may_anh.js, xem README.md).
//
// Các số dưới đây tính từ hình học thiết kế (ống kính cao 285 mm, chúc 26°, nằm trước vạch dừng 14 mm,
// ngay trên vạch chia hai làn chờ), với vạch dừng ở mép phải ảnh và làn trong ở phía trên. Chúng chỉ là
// điểm xuất phát: lắp xong phải chạy chế độ thu ảnh, vẽ ROI và bấm 4 góc từng làn trên trang web của
// camera, rồi dán dòng nó in ra vào đúng ô của camera đó. Ảnh bị lộn 180° thì đặt LAT_DOC và LAT_NGANG thành 1.
static const CauHinhCam CAU_HINH_CAM[5] = {
  { 0, 0, 0, 0, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}} },                          // không dùng
  { 43, 52, 277, 136, {{51, 159}, {51, 120}, {336, 120}, {336, 180}}, {{51, 120}, {51, 81}, {336, 60}, {336, 120}} },   // CAM 1
  { 43, 52, 277, 136, {{51, 159}, {51, 120}, {336, 120}, {336, 180}}, {{51, 120}, {51, 81}, {336, 60}, {336, 120}} },   // CAM 2
  { 43, 52, 277, 136, {{51, 159}, {51, 120}, {336, 120}, {336, 180}}, {{51, 120}, {51, 81}, {336, 60}, {336, 120}} },   // CAM 3
  { 43, 52, 277, 136, {{51, 159}, {51, 120}, {336, 120}, {336, 180}}, {{51, 120}, {51, 81}, {336, 60}, {336, 120}} }    // CAM 4
};

// ---------- 6) Mô hình Edge Impulse (chỉ dùng ở chế độ đếm) ----------
// Sau khi huấn luyện, xuất "Arduino library", cài tệp .zip vào Arduino IDE rồi sửa tên dưới đây
// cho đúng tên tệp .h của thư viện (xem File > Examples, tên có đuôi _inferencing).
#ifndef THU_VIEN_EI
#define THU_VIEN_EI <den_giao_thong_inferencing.h>
#endif
#define NGUONG_TIN_CAY 0.5f          // bỏ các ô mô hình không chắc chắn
// Tên lớp của 4 loại xe ưu tiên lúc gán nhãn trên Edge Impulse, theo đúng thứ tự được mở đèn
// "hỏa → sự → công → thương" (ut = 1, 2, 3, 4). Mọi lớp khác đều tính là xe thường.
// Mô hình chỉ có một lớp xe ưu tiên chung thì đặt tên lớp đó là "xe_uu_tien": trung tâm coi là xe cứu thương.
static const char *const NHAN_UU_TIEN[4] = {"xe_cuu_hoa", "xe_quan_su", "xe_cong_an", "xe_cuu_thuong"};
#define NHAN_UU_TIEN_CHUNG "xe_uu_tien"
#define SO_XE_TOI_DA 24
