// nhan_dien.h — Chạy mô hình Edge Impulse (loại FOMO, nhận diện vật thể) trên ảnh ROI
// và trả về tâm từng xe theo toạ độ của khung 320 × 240.
// Chỉ biên dịch ở chế độ đếm, sau khi đã cài thư viện xuất từ Edge Impulse.
#pragma once
#include THU_VIEN_EI
#include "may_anh.h"

#if !defined(EI_CLASSIFIER_OBJECT_DETECTION) || EI_CLASSIFIER_OBJECT_DETECTION != 1
#error "Mô hình phải là loại nhận diện vật thể (Object Detection - FOMO), không phải phân loại ảnh."
#endif

static uint8_t *anh_mo_hinh = NULL;      // ảnh ROI đã co về đúng cỡ đầu vào của mô hình, thứ tự B, G, R

// Edge Impulse gọi hàm này để lấy từng đoạn điểm ảnh, mỗi điểm gói thành 0xRRGGBB.
static int layDiemAnh(size_t vi_tri, size_t so_luong, float *ra) {
  size_t i = vi_tri * 3;
  for (size_t k = 0; k < so_luong; k++, i += 3) {
    ra[k] = (float)(((uint32_t)anh_mo_hinh[i + 2] << 16) | ((uint32_t)anh_mo_hinh[i + 1] << 8) | anh_mo_hinh[i]);
  }
  return 0;
}

inline bool nhanDienKhoiTao() {
  size_t co = (size_t)EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT * 3;
  anh_mo_hinh = (uint8_t *)ps_malloc(co);
  if (!anh_mo_hinh) anh_mo_hinh = (uint8_t *)malloc(co);     // chưa bật PSRAM thì thử bộ nhớ thường
  return anh_mo_hinh != NULL;
}

// Tên lớp của mô hình → loại xe (0 xe thường, 1 cứu hỏa, 2 quân sự, 3 công an, 4 cứu thương).
inline uint8_t loaiTheoNhan(const char *nhan) {
  for (uint8_t k = 0; k < 4; k++) if (strcmp(nhan, NHAN_UU_TIEN[k]) == 0) return (uint8_t)(k + 1);
  return strcmp(nhan, NHAN_UU_TIEN_CHUNG) == 0 ? 4 : 0;
}

// Trả về số xe tìm được (ghi vào ra[]), hoặc -1 nếu mô hình báo lỗi.
inline int nhanDien(const camera_fb_t *fb, const CauHinhCam &ch, VatThe *ra, int toi_da) {
  if (!catRoi(fb, ch, anh_mo_hinh, EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT)) return -1;
  ei::signal_t tin_hieu;
  tin_hieu.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
  tin_hieu.get_data = &layDiemAnh;
  ei_impulse_result_t kq;
  memset(&kq, 0, sizeof(kq));
  if (run_classifier(&tin_hieu, &kq, false) != EI_IMPULSE_OK) return -1;
  int n = 0;
  for (uint32_t i = 0; i < kq.bounding_boxes_count && n < toi_da; i++) {
    const ei_impulse_result_bounding_box_t &o = kq.bounding_boxes[i];
    if (o.value < NGUONG_TIN_CAY) continue;
    // tâm ô trên ảnh mô hình → toạ độ trên khung 320 × 240
    ra[n].x = ch.roi_x + (o.x + o.width / 2.0f) * ch.roi_w / EI_CLASSIFIER_INPUT_WIDTH;
    ra[n].y = ch.roi_y + (o.y + o.height / 2.0f) * ch.roi_h / EI_CLASSIFIER_INPUT_HEIGHT;
    ra[n].loai = loaiTheoNhan(o.label);
    n++;
  }
  return n;
}
