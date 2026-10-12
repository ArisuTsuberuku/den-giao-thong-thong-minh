// may_anh.h — Khởi tạo OV2640, chụp khung 320 × 240, cắt ROI và co ảnh.
// Ảnh huấn luyện và ảnh lúc đếm đều đi qua đúng hàm catRoi() này.
// ESP32-CAM (chip ESP32): chụp JPEG rồi giải nén. Chip ESP32 chép ảnh thô RGB565 vào PSRAM không kịp,
// nhất là khi bật WiFi, và có thể treo tới mức bị watchdog khởi động lại (TG1WDT_SYS_RESET); thư viện
// esp32-camera cũng khuyên chụp JPEG rồi đổi sang RGB. ESP32-S3-CAM thì chụp thẳng RGB565.
#pragma once
#include "esp_camera.h"
#include "img_converters.h"
#include "cau_hinh.h"
#include "cat_anh.h"

#if LOAI_BOARD == BOARD_ESP32_CAM
#define DINH_DANG_ANH PIXFORMAT_JPEG
#else
#define DINH_DANG_ANH PIXFORMAT_RGB565
#endif

static uint8_t *khung_rgb = NULL;        // khung JPEG đã giải nén, 320 × 240 × 3 byte, trong PSRAM

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
  c.pixel_format = DINH_DANG_ANH;
  c.grab_mode = CAMERA_GRAB_LATEST;        // luôn lấy khung mới nhất
  c.fb_location = CAMERA_FB_IN_PSRAM;
  c.jpeg_quality = 10;                     // chỉ dùng khi chụp JPEG: số nhỏ là nét hơn
  c.fb_count = 2;
  if (esp_camera_init(&c) != ESP_OK) return false;
  sensor_t *cb = esp_camera_sensor_get();
  if (cb) { cb->set_vflip(cb, LAT_DOC); cb->set_hmirror(cb, LAT_NGANG); }
  return true;
}

// Cắt ROI của một khung vừa chụp rồi co về dw × dh (xem cat_anh.h). Trả về false nếu không giải
// nén được ảnh JPEG (khi đó ảnh ra là ảnh đen).
inline bool catRoi(const camera_fb_t *fb, const CauHinhCam &ch, uint8_t *ra, int dw, int dh) {
  if (fb->format != PIXFORMAT_JPEG) {
    catRoiRgb565(fb->buf, (int)fb->width, (int)fb->height, ch, ra, dw, dh);
    return true;
  }
  if (!khung_rgb) khung_rgb = (uint8_t *)ps_malloc((size_t)ANH_RONG * ANH_CAO * 3);
  if (!khung_rgb || fb->width != ANH_RONG || fb->height != ANH_CAO ||
      !fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, khung_rgb)) {
    memset(ra, 0, (size_t)dw * dh * 3);
    return false;
  }
  catRoiRgb888(khung_rgb, (int)fb->width, (int)fb->height, ch, ra, dw, dh);
  return true;
}
