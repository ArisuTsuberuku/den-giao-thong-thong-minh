// quyet_dinh.h — Bộ lập kế hoạch đèn cho MỘT chu kì (mức 1: bộ luật).
// Không dùng hàm nào của Arduino nên biên dịch được cả trên máy tính (xem kiem_thu/).
// Bản JavaScript tương ứng: web/quyet_dinh.js. Sửa luật ở bản này thì sửa cả bản kia.
// Mọi thời gian tính bằng mili giây.
#pragma once
#include <stdint.h>

#define CHE_DO_CO_DINH 0
#define CHE_DO_THICH_UNG 1
#define SO_BUOC_TOI_DA 4

struct ThamSo {
  uint8_t che_do;
  uint32_t t_min;            // xanh ngắn nhất
  uint32_t t_max;            // xanh dài nhất
  uint32_t cong_them;        // cộng thêm cho mỗi 10 xe thật đang chờ
  uint32_t xe_quy_doi;       // 1 xe mô hình = bao nhiêu xe thật
  uint32_t t_vang;
  uint32_t t_do;             // đỏ toàn phần giữa hai bước
  uint32_t cd_thang;         // chế độ cố định: xanh đi thẳng
  uint32_t cd_trai;          // chế độ cố định: xanh rẽ trái
  uint32_t mat_tin_hieu;     // quá lâu không có tin thì coi camera mất tín hiệu
  uint32_t ut_xanh_an_toan;  // bước đang chạy phải xanh ít nhất chừng này mới bị cắt để ưu tiên
  uint32_t ut_giu;           // giữ xanh thêm sau khi không còn thấy xe ưu tiên
  uint32_t ut_toi_da;        // ưu tiên tự hủy sau thời gian này
};

struct Buoc {
  uint8_t mat_na;            // các nhóm được xanh trong bước
  uint32_t xanh;             // thời gian xanh
};

struct KeHoach {
  uint8_t so_buoc;
  Buoc buoc[SO_BUOC_TOI_DA];
};

// ----- 8 nhóm tín hiệu: bit = (hướng-1)*2 + (0: đi thẳng, 1: rẽ trái) -----
inline uint8_t bitThang(int h) { return (uint8_t)(1u << ((h - 1) * 2)); }
inline uint8_t bitTrai(int h) { return (uint8_t)(1u << ((h - 1) * 2 + 1)); }
inline uint8_t matNaRieng(int h) { return bitThang(h) | bitTrai(h); }
inline uint8_t matNaThang(int a, int b) { return bitThang(a) | bitThang(b); }
inline uint8_t matNaTrai(int a, int b) { return bitTrai(a) | bitTrai(b); }

// Hai nhóm được xanh cùng lúc khi: cùng một hướng, hoặc hai hướng đối diện và cùng loại
// (thẳng với thẳng, trái với trái). Mọi cặp khác là xung đột.
inline bool hopLeMatNa(uint8_t m) {
  for (int i = 0; i < 8; i++) {
    if (!(m & (1u << i))) continue;
    for (int j = i + 1; j < 8; j++) {
      if (!(m & (1u << j))) continue;
      int hi = i >> 1, hj = j >> 1;
      if (hi == hj) continue;
      if (((hi ^ hj) & 1) == 0 && (i & 1) == (j & 1)) continue;
      return false;
    }
  }
  return true;
}

inline uint32_t max2(uint32_t a, uint32_t b) { return a > b ? a : b; }

// Thời gian xanh cho bước có làn đông nhất là n xe mô hình.
inline uint32_t thoiGianXanh(const ThamSo &ts, uint32_t n) {
  uint32_t t = ts.t_min + ts.cong_them * n * ts.xe_quy_doi / 10;
  if (t > ts.t_max) t = ts.t_max;
  if (t < ts.t_min) t = ts.t_min;
  return t;
}

inline void themBuoc(KeHoach &kh, uint8_t mat_na, uint32_t xanh) {
  if (kh.so_buoc >= SO_BUOC_TOI_DA) return;
  kh.buoc[kh.so_buoc].mat_na = mat_na;
  kh.buoc[kh.so_buoc].xanh = xanh;
  kh.so_buoc++;
}

// Kế hoạch cho một trục (a và b là hai hướng đối diện).
// s[h]: xe ở làn ngoài (chỉ đi thẳng); l[h]: xe ở làn trong (đi thẳng hoặc rẽ trái).
inline void keHoachTruc(const ThamSo &ts, KeHoach &kh, int a, int b,
                        const uint8_t s[5], const uint8_t l[5], const bool hop_le[5]) {
  if (!hop_le[a] || !hop_le[b]) {           // camera mất tín hiệu: trục này chạy thời gian cố định
    themBuoc(kh, matNaThang(a, b), ts.cd_thang);
    themBuoc(kh, matNaTrai(a, b), ts.cd_trai);
    return;
  }
  uint32_t sa = s[a], la = l[a], sb = s[b], lb = l[b];
  uint32_t na = sa + la, nb = sb + lb;
  if (na == 0 && nb == 0) return;           // cả hai hướng trống: bỏ qua trục
  if (nb == 0) { themBuoc(kh, matNaRieng(a), thoiGianXanh(ts, max2(sa, la))); return; }
  if (na == 0) { themBuoc(kh, matNaRieng(b), thoiGianXanh(ts, max2(sb, lb))); return; }
  if (la == 0 && lb == 0) {                 // không ai ở làn trong: chỉ cần đi thẳng
    themBuoc(kh, matNaThang(a, b), thoiGianXanh(ts, max2(sa, sb)));
    return;
  }
  // Phương án 1: thẳng cả hai, rồi rẽ trái có bảo vệ cho hướng nào có xe ở làn trong
  uint32_t t1 = thoiGianXanh(ts, max2(max2(sa, la), max2(sb, lb)));
  uint8_t m2;
  uint32_t t2;
  if (la > 0 && lb > 0) { m2 = matNaTrai(a, b); t2 = thoiGianXanh(ts, max2(la, lb)); }
  else if (la > 0) { m2 = matNaRieng(a); t2 = thoiGianXanh(ts, la); }
  else { m2 = matNaRieng(b); t2 = thoiGianXanh(ts, lb); }
  // Phương án 2: riêng từng hướng, hướng đông đi trước
  uint32_t ta = thoiGianXanh(ts, max2(sa, la)), tb = thoiGianXanh(ts, max2(sb, lb));
  // Chọn phương án tốn ít thời gian hơn. Bằng nhau mà cả hai làn trong đều có xe thì chọn
  // riêng từng hướng: làn trong đi chung thẳng và trái, xe đi thẳng và xe rẽ trái dễ chặn nhau
  // nếu hai loại không được xanh cùng lúc.
  bool chon_rieng = ta + tb < t1 + t2 || (ta + tb == t1 + t2 && la > 0 && lb > 0);
  if (chon_rieng) {
    if (na >= nb) { themBuoc(kh, matNaRieng(a), ta); themBuoc(kh, matNaRieng(b), tb); }
    else { themBuoc(kh, matNaRieng(b), tb); themBuoc(kh, matNaRieng(a), ta); }
  } else {
    themBuoc(kh, matNaThang(a, b), t1);
    themBuoc(kh, m2, t2);
  }
}

// Đầu vào: s, l, hop_le là mảng 5 phần tử, dùng chỉ số 1..4 theo số hướng.
inline KeHoach lapKeHoach(const ThamSo &ts, const uint8_t s[5], const uint8_t l[5], const bool hop_le[5]) {
  KeHoach kh;
  kh.so_buoc = 0;
  if (ts.che_do == CHE_DO_CO_DINH) {        // thứ tự cố định, dùng để đối chứng
    themBuoc(kh, matNaTrai(1, 3), ts.cd_trai);
    themBuoc(kh, matNaThang(2, 4), ts.cd_thang);
    themBuoc(kh, matNaTrai(2, 4), ts.cd_trai);
    themBuoc(kh, matNaThang(1, 3), ts.cd_thang);
    return kh;
  }
  keHoachTruc(ts, kh, 1, 3, s, l, hop_le);  // trục dọc
  keHoachTruc(ts, kh, 2, 4, s, l, hop_le);  // trục ngang
  if (kh.so_buoc == 0) {                    // cả ngã tư trống: luân phiên hai trục ở Tmin
    themBuoc(kh, matNaThang(1, 3), ts.t_min);
    themBuoc(kh, matNaThang(2, 4), ts.t_min);
  }
  return kh;
}
