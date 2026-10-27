// kiem_anh_fw.h — Kiểm tra một tệp firmware (.bin) trong lúc nhận từng đoạn, trước khi cho nạp:
//   - byte đầu là 0xE9 (ảnh ứng dụng ESP32);
//   - mã chip ở byte 12–13 đúng loại chip (ESP32 là 0, ESP32-S3 là 9);
//   - có dấu "DGTTM-FW|<loại>|<ngày giờ biên dịch>" mà firmware của dự án tự mang theo (xem DAU_FW trong
//     trung_tam.ino và camera.ino), để không nạp nhầm firmware camera vào trung tâm hay ngược lại.
// Dấu cần tìm được viết ngược trong code ("|aremac|WF-MTTGD") rồi lật lại lúc chạy, nên firmware trung tâm
// không vô tình chứa dấu của firmware camera.
// Không dùng hàm nào của Arduino nên thử được trên máy tính (kiem_thu/thu_mang.cpp).
// Bản này có ở cả firmware/trung_tam và firmware/camera; hai bản phải giống hệt nhau (bài thử kiểm tra).
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define CHIP_ESP32 0
#define CHIP_ESP32S3 9
#define CHIP_BAT_KI 0xFFFF

enum KetQuaKiemAnh {
  KA_DUNG = 0,
  KA_KHONG_PHAI_FIRMWARE = 1,   // byte đầu không phải 0xE9
  KA_SAI_CHIP = 2,              // firmware cho loại chip khác
  KA_KHONG_CO_DAU = 3,          // không phải firmware của dự án, hoặc của board khác (camera / trung tâm)
  KA_QUA_NGAN = 4
};

inline const char *tenKetQuaKiemAnh(int k) {
  switch (k) {
    case KA_DUNG: return "đúng";
    case KA_KHONG_PHAI_FIRMWARE: return "không phải tệp firmware ESP32 (.bin)";
    case KA_SAI_CHIP: return "firmware cho loại chip khác (ESP32 / ESP32-S3)";
    case KA_KHONG_CO_DAU: return "không phải firmware của dự án cho đúng loại board (camera / trung tâm)";
    default: return "tệp quá ngắn";
  }
}

class KiemAnhFw {
public:
  // loai_nguoc: tên loại viết ngược, ví dụ "aremac" cho "camera", "mat_gnurt" cho "trung_tam".
  void batDau(uint16_t chip_mong, const char *loai_nguoc) {
    chip_mong_ = chip_mong;
    // dấu = "DGTTM-FW|" + loại + "|", dựng từ hai chuỗi viết ngược
    static const char DAU_NGUOC[] = "|WF-MTTGD";
    n_ = 0;
    for (int i = (int)sizeof(DAU_NGUOC) - 2; i >= 0 && n_ < (int)sizeof(can_) - 2; i--) can_[n_++] = DAU_NGUOC[i];
    for (int i = (int)strlen(loai_nguoc) - 1; i >= 0 && n_ < (int)sizeof(can_) - 2; i--) can_[n_++] = loai_nguoc[i];
    can_[n_++] = '|';
    can_[n_] = 0;
    // bảng nhảy KMP để tìm dấu xuyên qua ranh giới các đoạn
    lui_[0] = 0;
    for (int i = 1, k = 0; i < n_; i++) {
      while (k > 0 && can_[i] != can_[k]) k = lui_[k - 1];
      if (can_[i] == can_[k]) k++;
      lui_[i] = k;
    }
    khop_ = 0; da_nhan_ = 0; thay_ = false; chip_ = 0xFFFF; dau0_ = 0;
    pb_dai_ = 0; doc_pb_ = false; phien_ban_[0] = 0;
  }

  void them(const uint8_t *b, size_t n) {
    for (size_t i = 0; i < n; i++) {
      uint8_t c = b[i];
      size_t vt = da_nhan_ + i;
      if (vt == 0) dau0_ = c;
      else if (vt == 12) chip_ = c;
      else if (vt == 13) chip_ |= (uint16_t)c << 8;
      if (doc_pb_) {                       // chép phiên bản (ngày giờ biên dịch) đứng ngay sau dấu
        if (c == 0 || pb_dai_ >= (int)sizeof(phien_ban_) - 1) doc_pb_ = false;
        else { phien_ban_[pb_dai_++] = (char)c; phien_ban_[pb_dai_] = 0; }
        continue;
      }
      if (thay_) continue;
      while (khop_ > 0 && (char)c != can_[khop_]) khop_ = lui_[khop_ - 1];
      if ((char)c == can_[khop_]) khop_++;
      if (khop_ == n_) { thay_ = true; doc_pb_ = true; }
    }
    da_nhan_ += n;
  }

  int ketQua() const {
    if (da_nhan_ < 64) return KA_QUA_NGAN;
    if (dau0_ != 0xE9) return KA_KHONG_PHAI_FIRMWARE;
    if (!chipDung()) return KA_SAI_CHIP;
    if (!thay_) return KA_KHONG_CO_DAU;
    return KA_DUNG;
  }
  // Kiểm tra sớm, sau đoạn đầu: sai byte đầu hoặc sai chip thì dừng nhận ngay.
  int ketQuaSom() const {
    if (da_nhan_ < 16) return KA_DUNG;
    if (dau0_ != 0xE9) return KA_KHONG_PHAI_FIRMWARE;
    if (!chipDung()) return KA_SAI_CHIP;
    return KA_DUNG;
  }
  uint16_t chip() const { return chip_; }
  size_t daNhan() const { return da_nhan_; }
  const char *phienBan() const { return phien_ban_; }

private:
  // chip_mong = CHIP_BAT_KI: nhận cả ESP32 lẫn ESP32-S3 (trung tâm giữ firmware camera hộ, chưa biết camera loại gì)
  bool chipDung() const { return chip_mong_ == CHIP_BAT_KI ? (chip_ == CHIP_ESP32 || chip_ == CHIP_ESP32S3) : chip_ == chip_mong_; }
  char can_[40];
  int lui_[40];
  int n_ = 0, khop_ = 0, pb_dai_ = 0;
  size_t da_nhan_ = 0;
  bool thay_ = false, doc_pb_ = false;
  uint16_t chip_mong_ = 0, chip_ = 0xFFFF;
  uint8_t dau0_ = 0;
  char phien_ban_[28];
};
