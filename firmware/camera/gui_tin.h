// gui_tin.h — Gửi bản tin {"id":1,"seq":1042,"s":3,"l":1,"emg":0,"ut":0} bằng ESP-NOW.
// emg: xe ưu tiên ở làn nào (0 không có, 1 làn ngoài, 2 làn trong); ut: loại xe ưu tiên
// (1 cứu hỏa, 2 quân sự, 3 công an, 4 cứu thương).
// Gửi kiểu phát chung (địa chỉ FF:FF:FF:FF:FF:FF) trên kênh cố định, nên không cần biết
// địa chỉ MAC của board trung tâm và thay board không phải sửa code. Kênh do mang.h giữ.
#pragma once
#include "mang.h"

static uint32_t so_thu_tu_goi = 0;

// Nối WiFi trung tâm (nếu có) và bật ESP-NOW; xem mang.h.
inline bool guiTinKhoiTao() {
  return mangKhoiTao(false);
}

// Gửi một gói; chuỗi JSON vừa gửi được chép ra da_gui (nếu khác NULL) để in Serial.
inline bool guiBanTin(uint8_t s, uint8_t l, uint8_t emg, uint8_t ut, char *da_gui, size_t co) {
  char goi[96];
  snprintf(goi, sizeof(goi), "{\"id\":%d,\"seq\":%lu,\"s\":%u,\"l\":%u,\"emg\":%u,\"ut\":%u}",
           CAM_ID, (unsigned long)so_thu_tu_goi++, s, l, emg, ut);
  if (da_gui) snprintf(da_gui, co, "%s", goi);
  return mangGui(goi);
}
