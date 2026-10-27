// cau_hinh_mang.h — Nối camera vào WiFi do board trung tâm phát, để điều khiển cả 4 camera từ
// trang web của trung tâm (http://192.168.4.1) và chọn vùng đếm mà không phải nạp lại code.
// Tách khỏi cau_hinh.h để không đụng tới ROI và tứ giác làn đã đo cho từng camera.
#pragma once
#include "cau_hinh.h"
#include "danh_tinh.h"

// ---------- WiFi của board trung tâm ----------
#define NOI_WIFI_TRUNG_TAM 1                // 0: không nối, chỉ dùng ESP-NOW như bản cũ
#define TEN_WIFI_TRUNG_TAM "TRUNG-TAM-SaBan"   // phải trùng với firmware/trung_tam/cau_hinh_mang.h
#define MAT_KHAU_TRUNG_TAM "12345678"
#define CHO_NOI_MS 8000                     // lúc bật máy chờ nối tối đa chừng này; trung tâm chưa bật thì vẫn chạy tiếp
// Địa chỉ cố định cho dễ nhớ: CAM 1 là 192.168.4.11, CAM 2 là 192.168.4.12, ...
#define IP_CAM_BYTE_CUOI (10 + cam_id)        // cam_id: số CAM đang lưu trong board (danh_tinh.h)

// ---------- Trang web trên camera ----------
// Chế độ thu ảnh luôn có trang web. Ở chế độ đếm, bật trang web để vẫn xem ảnh và chỉnh vùng đếm
// từ xa; mỗi lần trình duyệt lấy ảnh thì khung đếm kế tiếp chậm đi chừng 0,1–0,3 s.
#define WEB_KHI_DEM 1
// Không thấy WiFi trung tâm thì ở chế độ thu ảnh camera tự phát WiFi "CAM<số>-SaBan" như bản cũ.
#define TU_PHAT_KHI_KHONG_THAY 1

// ---------- Tin trạng thái gửi về trung tâm ----------
#define CHU_KI_TRANG_THAI_MS 3000

// ---------- LED đỏ nhỏ trên lưng board, dùng cho lệnh "nháy" để biết board nào là CAM mấy ----------
#if LOAI_BOARD == BOARD_ESP32_CAM
#define CHAN_LED_DO 33                      // AI-Thinker: LED đỏ ở chân 33, sáng khi chân ở mức thấp
#define LED_DO_SANG LOW
#else
#define CHAN_LED_DO -1
#define LED_DO_SANG LOW
#endif
