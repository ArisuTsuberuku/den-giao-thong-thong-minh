// may_anh.h — Khởi tạo OV2640, chụp khung 320 × 240 dạng RGB565, cắt ROI và co ảnh.
// Ảnh huấn luyện và ảnh lúc đếm đều đi qua đúng hàm catRoi() này.
#pragma once
#include "esp_camera.h"
#include "cau_hinh.h"
#include "cat_anh.h"

inline bool mayAnhKhoiTao() {
#if CHAN_DEN_FLASH >= 0
  pinMode(CHAN_DEN_FLASH, OUTPUT);
  digitalWrite(CHAN_DEN_FLASH, LOW);
#endif
  camera_config_t c;
  memset(&c, 0, sizeof(c));
  c.ledc_channel = LEDC_CHANNEL_0;
  c.ledc_timer = LEDC_TIMER_0;
  c.pin_d0 = CHAN_Y2; c.pin_d1 = CHAN_Y3; c.pin_d2 = CHAN_Y4; c.pin_d3 = CHAN_Y5;
  c.pin_d4 = CHAN_Y6; c.pin_d5 = CHAN_Y7; c.pin_d6 = CHAN_Y8; c.pin_d7 = CHAN_Y9;
  c.pin_xclk = CHAN_XCLK; c.pin_pclk = CHAN_PCLK; c.pin_vsync = CHAN_VSYNC; c.pin_href = CHAN_HREF;
  c.pin_sccb_sda = CHAN_SIOD; c.pin_sccb_scl = CHAN_SIOC;
  c.pin_pwdn = CHAN_PWDN; c.pin_reset = CHAN_RESET;
  c.xclk_freq_hz = TAN_SO_XCLK;
  c.frame_size = FRAMESIZE_QVGA;           // 320 × 240
  c.pixel_format = PIXFORMAT_RGB565;       // lấy ảnh thô, không qua nén JPEG
  c.grab_mode = CAMERA_GRAB_LATEST;        // luôn lấy khung mới nhất
  c.fb_location = CAMERA_FB_IN_PSRAM;
  c.jpeg_quality = 12;
  c.fb_count = 2;
  if (esp_camera_init(&c) != ESP_OK) return false;
  sensor_t *cb = esp_camera_sensor_get();
  if (cb) { cb->set_vflip(cb, LAT_DOC); cb->set_hmirror(cb, LAT_NGANG); }
  return true;
}

// Cắt ROI của một khung vừa chụp rồi co về dw × dh (xem cat_anh.h).
inline void catRoi(const camera_fb_t *fb, const CauHinhCam &ch, uint8_t *ra, int dw, int dh) {
  catRoiRgb565(fb->buf, (int)fb->width, (int)fb->height, ch, ra, dw, dh);
}
