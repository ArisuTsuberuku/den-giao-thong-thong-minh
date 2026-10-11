// lan_duong.h — Gán từng xe vào làn bằng 2 tứ giác, rồi lọc nhiễu qua 5 khung gần nhất.
// Không dùng hàm nào của Arduino nên thử được trên máy tính (kiem_thu/thu_camera.cpp).
#pragma once
#include <stdint.h>

struct Diem { int16_t x, y; };

// Vùng quan sát của một camera, tính bằng điểm ảnh trên khung 320 × 240.
struct CauHinhCam {
  int16_t roi_x, roi_y, roi_w, roi_h;   // hình chữ nhật cắt ra để đưa vào mô hình (chỉ chứa 2 làn chờ)
  Diem lan_ngoai[4];                    // 4 góc làn ngoài (chỉ đi thẳng)   → đếm vào "s"
  Diem lan_trong[4];                    // 4 góc làn trong (thẳng hoặc trái) → đếm vào "l"
};

// Loại xe: 0 xe thường; 1 cứu hỏa, 2 quân sự, 3 công an, 4 cứu thương. Số của xe ưu tiên cũng là
// thứ tự được mở đèn ("hỏa → sự → công → thương"): số nhỏ hơn đi trước.
struct VatThe {
  float x, y;        // tâm xe trên khung 320 × 240
  uint8_t loai;
};

struct KetQuaKhung {
  uint8_t s;         // số xe ở làn ngoài
  uint8_t l;         // số xe ở làn trong
  uint8_t emg;       // 0 không có xe ưu tiên, 1 ở làn ngoài, 2 ở làn trong
  uint8_t ut;        // loại xe ưu tiên đó (1..4), 0 nếu không có
};

// Điểm (x, y) có nằm trong tứ giác q không (đếm số cạnh mà tia ngang cắt qua).
inline bool trongTuGiac(const Diem q[4], float x, float y) {
  bool trong = false;
  for (int i = 0, j = 3; i < 4; j = i++) {
    float xi = q[i].x, yi = q[i].y, xj = q[j].x, yj = q[j].y;
    if (((yi > y) != (yj > y)) && (x < (xj - xi) * (y - yi) / (yj - yi) + xi)) trong = !trong;
  }
  return trong;
}

// Xe có tâm nằm ngoài cả hai tứ giác thì bỏ qua (làn rẽ phải, làn ra, đường bao).
// Có nhiều xe ưu tiên trong vùng thì báo xe hạng cao nhất (số loại nhỏ nhất).
inline KetQuaKhung phanLan(const VatThe *v, int n, const CauHinhCam &ch) {
  KetQuaKhung kq = {0, 0, 0, 0};
  for (int i = 0; i < n; i++) {
    uint8_t lan = 0;
    if (trongTuGiac(ch.lan_ngoai, v[i].x, v[i].y)) { kq.s++; lan = 1; }
    else if (trongTuGiac(ch.lan_trong, v[i].x, v[i].y)) { kq.l++; lan = 2; }
    uint8_t loai = v[i].loai <= 4 ? v[i].loai : 0;
    if (lan && loai && (kq.ut == 0 || loai < kq.ut)) { kq.ut = loai; kq.emg = lan; }
  }
  return kq;
}

// Lọc nhiễu: s và l lấy trung vị 5 khung gần nhất; emg chỉ bật khi ít nhất 3/5 khung thấy xe ưu tiên.
// Loại xe ưu tiên là loại xuất hiện ở nhiều khung nhất trong 5 khung đó (bằng nhau thì lấy hạng cao hơn).
class BoLoc {
public:
  BoLoc() : so_khung(0), vi_tri(0) {}

  void them(const KetQuaKhung &k) {
    khung[vi_tri] = k;
    vi_tri = (vi_tri + 1) % 5;
    if (so_khung < 5) so_khung++;
  }

  KetQuaKhung layKetQua() const {
    KetQuaKhung kq = {0, 0, 0, 0};
    if (so_khung == 0) return kq;
    uint8_t a[5], b[5];
    int n1 = 0, n2 = 0;
    int so_loai[5] = {0, 0, 0, 0, 0};
    for (int i = 0; i < so_khung; i++) {
      a[i] = khung[i].s; b[i] = khung[i].l;
      if (khung[i].emg == 1) n1++;
      if (khung[i].emg == 2) n2++;
      if (khung[i].emg != 0) so_loai[(khung[i].ut >= 1 && khung[i].ut <= 4) ? khung[i].ut : 4]++;   // không ghi loại: cứu thương
    }
    kq.s = trungVi(a, so_khung);
    kq.l = trungVi(b, so_khung);
    if (n1 + n2 >= 3) {
      kq.emg = (n2 > n1) ? 2 : 1;
      kq.ut = 1;
      for (uint8_t k = 2; k <= 4; k++) if (so_loai[k] > so_loai[kq.ut]) kq.ut = k;
    }
    return kq;
  }

private:
  KetQuaKhung khung[5];
  int so_khung, vi_tri;

  static uint8_t trungVi(uint8_t *a, int n) {
    for (int i = 1; i < n; i++) {                 // sắp xếp chèn
      uint8_t v = a[i];
      int j = i - 1;
      while (j >= 0 && a[j] > v) { a[j + 1] = a[j]; j--; }
      a[j + 1] = v;
    }
    return a[n / 2];
  }
};
