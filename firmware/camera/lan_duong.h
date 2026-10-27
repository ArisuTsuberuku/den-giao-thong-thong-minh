// lan_duong.h — Gán từng xe vào làn bằng 2 tứ giác, rồi lọc nhiễu qua vài khung gần nhất.
// Bản JavaScript tương ứng nằm trong web/may_anh.js (kiem_thu/so_khop.js so hai bản với nhau).
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

// Kiểm tra một vùng đếm trước khi dùng hoặc lưu vào board: ROI phải nằm trọn trong khung 320 × 240
// và đủ lớn; góc làn được phép ra ngoài khung một chút (vạch dừng có thể nằm ngoài mép ảnh).
inline bool vungHopLe(const CauHinhCam &c, int rong = 320, int cao = 240) {
  if (c.roi_x < 0 || c.roi_y < 0 || c.roi_w < 16 || c.roi_h < 16) return false;
  if (c.roi_x + c.roi_w > rong || c.roi_y + c.roi_h > cao) return false;
  for (int i = 0; i < 4; i++) {
    const Diem *p[2] = {&c.lan_ngoai[i], &c.lan_trong[i]};
    for (int k = 0; k < 2; k++)
      if (p[k]->x < -rong / 2 || p[k]->x > rong + rong / 2 || p[k]->y < -cao / 2 || p[k]->y > cao + cao / 2) return false;
  }
  return true;
}

// Đọc 20 số "roi_x,roi_y,roi_w,roi_h, 4 góc làn ngoài (x,y), 4 góc làn trong (x,y)" (cách nhau bởi
// dấu phẩy hoặc khoảng trắng) do trang web gửi lên. Trả về true nếu đủ 20 số và vùng hợp lệ.
inline bool vungDocChuoi(const char *s, CauHinhCam &ra) {
  int v[20], n = 0;
  while (*s && n < 20) {
    while (*s && !(*s == '-' || (*s >= '0' && *s <= '9'))) s++;
    if (!*s) break;
    bool am = false;
    if (*s == '-') { am = true; s++; }
    if (!(*s >= '0' && *s <= '9')) continue;
    long x = 0;
    while (*s >= '0' && *s <= '9') { x = x * 10 + (*s - '0'); if (x > 30000) x = 30000; s++; }
    v[n++] = (int)(am ? -x : x);
  }
  if (n != 20) return false;
  CauHinhCam c;
  c.roi_x = (int16_t)v[0]; c.roi_y = (int16_t)v[1]; c.roi_w = (int16_t)v[2]; c.roi_h = (int16_t)v[3];
  for (int i = 0; i < 4; i++) {
    c.lan_ngoai[i].x = (int16_t)v[4 + 2 * i]; c.lan_ngoai[i].y = (int16_t)v[5 + 2 * i];
    c.lan_trong[i].x = (int16_t)v[12 + 2 * i]; c.lan_trong[i].y = (int16_t)v[13 + 2 * i];
  }
  if (!vungHopLe(c)) return false;
  ra = c;
  return true;
}

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

// Lọc nhiễu qua n khung gần nhất (n = 1, 3 hoặc 5): s và l lấy trung vị; emg chỉ bật khi quá nửa số
// khung thấy xe ưu tiên (2/3 hoặc 3/5). Loại xe ưu tiên là loại xuất hiện ở nhiều khung nhất (bằng nhau
// thì lấy hạng cao hơn). Camera chỉ xử lí được chừng 1 khung mỗi giây, nên lọc n khung là trễ thêm
// khoảng (n − 1) / 2 giây: 3 khung là mức cân bằng giữa chống nhiễu và phản ứng nhanh.
class BoLoc {
public:
  explicit BoLoc(int so = 5) : n(so >= 5 ? 5 : (so >= 3 ? 3 : 1)), so_khung(0), vi_tri(0) {}

  void them(const KetQuaKhung &k) {
    khung[vi_tri] = k;
    vi_tri = (vi_tri + 1) % n;
    if (so_khung < n) so_khung++;
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
    if (n1 + n2 >= n / 2 + 1) {
      kq.emg = (n2 > n1) ? 2 : 1;
      kq.ut = 1;
      for (uint8_t k = 2; k <= 4; k++) if (so_loai[k] > so_loai[kq.ut]) kq.ut = k;
    }
    return kq;
  }

private:
  KetQuaKhung khung[5];
  int n, so_khung, vi_tri;

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
