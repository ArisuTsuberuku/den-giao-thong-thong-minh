// cat_anh.h — Cắt ROI từ khung RGB565 (ESP32-S3-CAM) hoặc RGB888 giải nén từ JPEG (ESP32-CAM) rồi co ảnh. Không dùng hàm nào của Arduino nên thử được
// trên máy tính (kiem_thu/thu_camera.cpp).
#pragma once
#include <stdint.h>
#include "lan_duong.h"

// Cắt ROI của khung RGB565 (rong × cao) rồi co về dw × dh bằng cách lấy trung bình các điểm ảnh
// rơi vào mỗi ô. Kết quả 3 byte mỗi điểm theo thứ tự B, G, R (thứ tự mà thư viện esp32-camera
// gọi là RGB888).
inline void catRoiRgb565(const uint8_t *khung, int rong, int cao, const CauHinhCam &ch, uint8_t *ra, int dw, int dh) {
  for (int dy = 0; dy < dh; dy++) {
    int y0 = ch.roi_y + dy * ch.roi_h / dh, y1 = ch.roi_y + (dy + 1) * ch.roi_h / dh;
    if (y1 <= y0) y1 = y0 + 1;
    for (int dx = 0; dx < dw; dx++) {
      int x0 = ch.roi_x + dx * ch.roi_w / dw, x1 = ch.roi_x + (dx + 1) * ch.roi_w / dw;
      if (x1 <= x0) x1 = x0 + 1;
      uint32_t r = 0, g = 0, b = 0, n = 0;
      for (int y = y0; y < y1; y++) {
        if (y < 0 || y >= cao) continue;
        for (int x = x0; x < x1; x++) {
          if (x < 0 || x >= rong) continue;
          const uint8_t *p = khung + 2 * (y * rong + x);
          uint16_t v = (uint16_t)(((uint16_t)p[0] << 8) | p[1]);   // RGB565, byte cao đi trước
          r += (uint32_t)(v >> 11) << 3; g += (uint32_t)((v >> 5) & 0x3F) << 2; b += (uint32_t)(v & 0x1F) << 3;
          n++;
        }
      }
      if (n == 0) n = 1;
      *ra++ = (uint8_t)(b / n); *ra++ = (uint8_t)(g / n); *ra++ = (uint8_t)(r / n);
    }
  }
}

// Như catRoiRgb565 nhưng khung vào là 3 byte mỗi điểm theo thứ tự B, G, R (ảnh JPEG đã giải nén
// bằng fmt2rgb888 của thư viện esp32-camera). Kết quả cùng thứ tự B, G, R.
inline void catRoiRgb888(const uint8_t *khung, int rong, int cao, const CauHinhCam &ch, uint8_t *ra, int dw, int dh) {
  for (int dy = 0; dy < dh; dy++) {
    int y0 = ch.roi_y + dy * ch.roi_h / dh, y1 = ch.roi_y + (dy + 1) * ch.roi_h / dh;
    if (y1 <= y0) y1 = y0 + 1;
    for (int dx = 0; dx < dw; dx++) {
      int x0 = ch.roi_x + dx * ch.roi_w / dw, x1 = ch.roi_x + (dx + 1) * ch.roi_w / dw;
      if (x1 <= x0) x1 = x0 + 1;
      uint32_t r = 0, g = 0, b = 0, n = 0;
      for (int y = y0; y < y1; y++) {
        if (y < 0 || y >= cao) continue;
        for (int x = x0; x < x1; x++) {
          if (x < 0 || x >= rong) continue;
          const uint8_t *p = khung + 3 * (y * rong + x);
          b += p[0]; g += p[1]; r += p[2];
          n++;
        }
      }
      if (n == 0) n = 1;
      *ra++ = (uint8_t)(b / n); *ra++ = (uint8_t)(g / n); *ra++ = (uint8_t)(r / n);
    }
  }
}
