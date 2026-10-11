// gia_ei_inferencing.h — Thư viện Edge Impulse GIẢ, không nhận diện được gì (luôn trả về 0 xe).
// Chỉ dùng để kiểm tra firmware camera biên dịch được ở chế độ đếm khi chưa có mô hình thật.
// Cách dùng: chép thư mục gia_ei vào thư mục libraries của Arduino, rồi trong
// firmware/camera/cau_hinh.h đặt   #define THU_VIEN_EI <gia_ei_inferencing.h>
// Khi đã có thư viện xuất từ Edge Impulse thì đổi lại tên thư viện thật.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <functional>

#define EI_CLASSIFIER_INPUT_WIDTH 96
#define EI_CLASSIFIER_INPUT_HEIGHT 96
#define EI_CLASSIFIER_OBJECT_DETECTION 1

namespace ei {
struct signal_t {
  std::function<int(size_t offset, size_t length, float *out_ptr)> get_data;
  size_t total_length;
};
}

typedef enum { EI_IMPULSE_OK = 0 } EI_IMPULSE_ERROR;

typedef struct {
  const char *label;
  uint32_t x, y, width, height;
  float value;
} ei_impulse_result_bounding_box_t;

typedef struct {
  ei_impulse_result_bounding_box_t *bounding_boxes;
  uint32_t bounding_boxes_count;
} ei_impulse_result_t;

inline EI_IMPULSE_ERROR run_classifier(ei::signal_t *tin_hieu, ei_impulse_result_t *ket_qua, bool in_chi_tiet) {
  (void)in_chi_tiet;
  float diem[8];
  tin_hieu->get_data(0, 8, diem);          // đọc thử vài điểm ảnh như thư viện thật
  ket_qua->bounding_boxes = NULL;
  ket_qua->bounding_boxes_count = 0;
  return EI_IMPULSE_OK;
}
