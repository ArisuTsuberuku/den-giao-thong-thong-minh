// cau_hinh.h — Mọi thứ cần chỉnh của board trung tâm nằm ở đây.
// Board: ESP32-S3 N16R8 (loại 2 cổng USB-C). Cách chọn trong Arduino IDE: xem README.md.
#pragma once
#include <Arduino.h>
#include "quyet_dinh.h"

// ---------- Chân đèn: {đỏ, vàng, xanh} của 8 nhóm ----------
// Thứ tự nhóm: H1 thẳng, H1 trái, H2 thẳng, H2 trái, H3 thẳng, H3 trái, H4 thẳng, H4 trái.
// Tránh các chân: 0, 3, 45, 46 (khởi động), 19, 20 (USB), 35–37 (bộ nhớ PSRAM), 43, 44 (Serial), 48 (LED trên board).
static const uint8_t CHAN_DEN[8][3] = {
  {4, 5, 6},      // Đ1 - H1 thẳng
  {7, 15, 16},    // Đ1 - H1 trái
  {17, 18, 8},    // Đ2 - H2 thẳng
  {9, 10, 11},    // Đ2 - H2 trái
  {12, 13, 14},   // Đ3 - H3 thẳng
  {1, 2, 42},     // Đ3 - H3 trái
  {41, 40, 39},   // Đ4 - H4 thẳng
  {38, 47, 21}    // Đ4 - H4 trái
};
#define DEN_MUC_BAT HIGH        // module đèn sáng khi chân ở mức cao; nếu đèn sáng ngược thì đổi thành LOW

// ---------- LED báo trạng thái trên board (LED RGB, chân 48) ----------
#define DUNG_LED_BAO 1          // 0: không dùng
#define CHAN_LED_BAO 48

// ---------- Truyền tin ----------
#define KENH_WIFI 1             // kênh ESP-NOW, phải trùng với KENH_WIFI của camera
#define TOC_DO_SERIAL 115200

// ---------- Chế độ và thời gian (mili giây), theo mục 6 của TIEN_DO_DU_AN.md ----------
#define CHE_DO_BAN_DAU CHE_DO_THICH_UNG   // hoặc CHE_DO_CO_DINH để đối chứng
#define HE_SO_TUA_NHANH 1       // 1 = thời gian thực; tăng lên khi cần trình diễn nhanh (tối đa 8)
#define T_MIN 15000             // xanh ngắn nhất
#define T_MAX 90000             // xanh dài nhất
#define CONG_THEM 5000          // cộng thêm cho mỗi 10 xe thật đang chờ
#define XE_QUY_DOI 10           // 1 xe mô hình = 10 xe thật
#define T_VANG 3000
#define T_DO_TOAN_PHAN 2000
#define CD_THANG 30000          // chế độ cố định: xanh đi thẳng
#define CD_TRAI 15000           // chế độ cố định: xanh rẽ trái
#define MAT_TIN_HIEU 3000       // camera im quá lâu thì trục đó chạy thời gian cố định từ chu kì sau
#define UT_XANH_AN_TOAN 5000    // bước đang chạy phải xanh ít nhất chừng này mới bị cắt để ưu tiên
#define UT_GIU 5000             // giữ xanh thêm sau khi không còn thấy xe ưu tiên
#define UT_TOI_DA 60000         // ưu tiên tự hủy sau thời gian này
