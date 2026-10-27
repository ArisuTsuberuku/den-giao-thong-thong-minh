// cau_hinh_mang.h — WiFi và trang web điều khiển của board trung tâm.
// Trung tâm phát WiFi riêng trên đúng KENH_WIFI. 4 camera nối vào WiFi này (địa chỉ 192.168.4.11 đến .14),
// máy tính hoặc điện thoại cũng nối vào rồi mở http://192.168.4.1 để xem và điều khiển cả 5 board.
// ESP-NOW vẫn chạy song song trên cùng kênh: số đếm xe vẫn đi bằng ESP-NOW như cũ.
#pragma once

#define DUNG_WEB 1                          // 0: tắt WiFi và trang web, chạy như bản cũ (chỉ ESP-NOW)
#define TEN_WIFI_TRUNG_TAM "TRUNG-TAM-SaBan"   // phải trùng với firmware/camera/cau_hinh_mang.h
#define MAT_KHAU_TRUNG_TAM "12345678"       // ít nhất 8 kí tự
#define SO_MAY_TOI_DA 8                     // 4 camera + vài máy tính, điện thoại (mặc định của ESP32 chỉ 4)
#define CAM_IP_BYTE_DAU 10                  // camera số n ở 192.168.4.(10 + n)

// ---------- Nối thêm vào hotspot của máy tính (tùy chọn) ----------
// Điền tên và mật khẩu hotspot thì trung tâm vừa phát WiFi riêng cho camera, vừa nối vào hotspot. Máy tính phát
// hotspot mở trang điều khiển bằng địa chỉ trung tâm in ra ở Serial (Windows thường là 192.168.137.x) hoặc
// http://densaban.local; ảnh và trang chọn vùng của camera đi qua trung tâm (http://<địa chỉ>/c1/ ...).
// Lưu ý: hotspot phải ở băng tần 2,4 GHz (ESP32 không bắt được 5 GHz). Khi nối hotspot, WiFi của trung tâm
// và ESP-NOW chuyển sang kênh của hotspot; camera tự nối lại sau vài giây.
#define TEN_WIFI_NGOAI ""                   // ví dụ "LAPTOP-SaBan"; để "" là không nối
#define MAT_KHAU_WIFI_NGOAI ""
#define CHO_WIFI_NGOAI_MS 20000             // tìm hotspot quá chừng này mà không thấy thì thôi (gõ "wifi" để thử lại),
                                            // vì lúc tìm, radio nhảy kênh làm camera mất tin
#define TEN_MDNS "densaban"                 // http://densaban.local
