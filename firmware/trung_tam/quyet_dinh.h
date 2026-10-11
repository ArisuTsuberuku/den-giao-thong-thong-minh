// quyet_dinh.h — Bộ lập kế hoạch đèn (mức 1: bộ luật). Chế độ cố định lập cả chu kì 4 bước;
// chế độ thích ứng lập cho từng LƯỢT: mỗi lượt là phần của một trục (dọc hoặc ngang), chốt theo
// số đếm ngay lúc lượt đó bắt đầu, hết lượt thì tới trục kia.
// Không dùng hàm nào của Arduino nên biên dịch được cả trên máy tính (xem kiem_thu/).
// Bản JavaScript tương ứng: web/quyet_dinh.js. Sửa luật ở bản này thì sửa cả bản kia.
// Mọi thời gian tính bằng mili giây.
#pragma once
#include <stdint.h>

#define CHE_DO_CO_DINH 0
#define CHE_DO_THICH_UNG 1
#define SO_BUOC_TOI_DA 4

// Kiểu chia pha của một trục khi cả hai hướng đều có xe và làn trong có xe:
//   RIENG        riêng từng hướng: mỗi hướng được xanh cả thẳng lẫn trái, hướng này xong tới hướng kia.
//                Không bao giờ kẹt vì xe rẽ trái, nhưng hai hướng không được đi cùng lúc.
//   THANG_TRUOC  hai hướng cùng đi thẳng trước; hai làn ngoài hết xe là chuyển sang mở riêng từng
//                hướng cho làn trong còn xe. Lợi khi ít xe rẽ trái, thiệt khi nhiều.
//   DOI_XUNG     hai hướng cùng đi thẳng, rồi cùng rẽ trái. Vì làn trong đi chung thẳng và trái
//                nên xe hai loại dễ chặn nhau; chỉ hợp khi gần như không có xe rẽ trái.
//   TU_HOC       mỗi trục tự chọn giữa RIENG và THANG_TRUOC: cứ vài lượt chạy thử kiểu kia một lượt,
//                đo thời gian đèn cho cùng số xe rồi giữ kiểu nhanh hơn (xem dieu_khien.h)
#define KIEU_TU_HOC 0
#define KIEU_RIENG 1
#define KIEU_THANG_TRUOC 2
#define KIEU_DOI_XUNG 3

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
  uint32_t ut_giu;           // xe ưu tiên qua rồi (không còn thấy quá chừng này) là chuyển vàng ngay
  uint32_t ut_toi_da;        // ưu tiên tự hủy sau thời gian này
  uint32_t cat_som;          // chế độ thích ứng: tổ hợp đang xanh không còn xe quá lâu thế này thì cắt xanh sớm (0: tắt)
  uint32_t kieu_pha;         // kiểu chia pha của chế độ thích ứng: KIEU_TU_HOC, KIEU_RIENG, KIEU_THANG_TRUOC hoặc KIEU_DOI_XUNG
  uint32_t hoc_cach_thu;     // tự học: đang giữ "riêng" thì cứ bấy nhiêu lượt của một trục lại thử "thẳng trước" một lượt (0: tắt tự học)
  uint32_t hoc_so_xe;        // tự học: lượt có ít hơn bấy nhiêu xe qua vạch dừng thì không tính
  uint32_t hoc_nguong;       // tự học: "thẳng trước" phải nhanh hơn "riêng" ít nhất bấy nhiêu % mới được chọn
};

struct Buoc {
  uint8_t mat_na;            // các nhóm được xanh trong bước
  uint8_t ngoai;             // 1: bước "cùng đi thẳng" có bước mở riêng theo sau, nên khi xét cắt xanh sớm
                             //    chỉ tính xe ở hai làn ngoài (xe làn trong còn lại do bước mở riêng lo)
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

inline void themBuoc(KeHoach &kh, uint8_t mat_na, uint32_t xanh, bool ngoai = false) {
  if (kh.so_buoc >= SO_BUOC_TOI_DA) return;
  kh.buoc[kh.so_buoc].mat_na = mat_na;
  kh.buoc[kh.so_buoc].ngoai = ngoai ? 1 : 0;
  kh.buoc[kh.so_buoc].xanh = xanh;
  kh.so_buoc++;
}

// Kế hoạch cho một trục (a và b là hai hướng đối diện).
// s[h]: xe ở làn ngoài (chỉ đi thẳng); l[h]: xe ở làn trong (đi thẳng hoặc rẽ trái).
inline void keHoachTruc(const ThamSo &ts, KeHoach &kh, int a, int b,
                        const uint8_t s[5], const uint8_t l[5], const bool hop_le[5], uint8_t kieu) {
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
  if (kieu == KIEU_DOI_XUNG) {               // cùng đi thẳng, rồi rẽ trái cho hướng có xe ở làn trong
    themBuoc(kh, matNaThang(a, b), thoiGianXanh(ts, max2(max2(sa, la), max2(sb, lb))));
    if (la > 0 && lb > 0) themBuoc(kh, matNaTrai(a, b), thoiGianXanh(ts, max2(la, lb)));
    else if (la > 0) themBuoc(kh, matNaRieng(a), thoiGianXanh(ts, la));
    else themBuoc(kh, matNaRieng(b), thoiGianXanh(ts, lb));
  } else if (kieu == KIEU_THANG_TRUOC) {     // cùng đi thẳng trước, rồi mở riêng cho làn trong còn xe
    int x = la >= lb ? a : b, y = la >= lb ? b : a;      // làn trong đông hơn được mở trước
    themBuoc(kh, matNaThang(a, b), thoiGianXanh(ts, max2(sa, sb)), true);
    if (l[x] > 0) themBuoc(kh, matNaRieng(x), thoiGianXanh(ts, l[x]));
    if (l[y] > 0) themBuoc(kh, matNaRieng(y), thoiGianXanh(ts, l[y]));
  } else {                                   // riêng từng hướng, hướng đông đi trước
    uint32_t ta = thoiGianXanh(ts, max2(sa, la)), tb = thoiGianXanh(ts, max2(sb, lb));
    if (na >= nb) { themBuoc(kh, matNaRieng(a), ta); themBuoc(kh, matNaRieng(b), tb); }
    else { themBuoc(kh, matNaRieng(b), tb); themBuoc(kh, matNaRieng(a), ta); }
  }
}

// Trục của một tổ hợp: 0 nếu có nhóm của Hướng 1 hoặc 3 (trục dọc), 1 nếu là trục ngang.
inline uint8_t trucCuaMatNa(uint8_t m) { return (m & 0x33) ? 0 : 1; }

// Đầu vào: s, l, hop_le là mảng 5 phần tử, dùng chỉ số 1..4 theo số hướng.
//   kieu: {kiểu chia pha trục dọc, kiểu chia pha trục ngang}, mỗi cái là KIEU_RIENG, KIEU_THANG_TRUOC hoặc KIEU_DOI_XUNG
//   truc: trục tới lượt (0 dọc, 1 ngang). Trục đó không có xe thì lập luôn cho trục kia.
// Đầu ra: tối đa 4 bước. Chế độ thích ứng: 1 đến 3 bước của một trục.
inline KeHoach lapKeHoach(const ThamSo &ts, const uint8_t s[5], const uint8_t l[5], const bool hop_le[5],
                          const uint8_t kieu[2], uint8_t truc) {
  KeHoach kh;
  kh.so_buoc = 0;
  if (ts.che_do == CHE_DO_CO_DINH) {        // thứ tự cố định, dùng để đối chứng
    themBuoc(kh, matNaTrai(1, 3), ts.cd_trai);
    themBuoc(kh, matNaThang(2, 4), ts.cd_thang);
    themBuoc(kh, matNaTrai(2, 4), ts.cd_trai);
    themBuoc(kh, matNaThang(1, 3), ts.cd_thang);
    return kh;
  }
  if (truc == 1) {
    keHoachTruc(ts, kh, 2, 4, s, l, hop_le, kieu[1]);
    if (kh.so_buoc == 0) keHoachTruc(ts, kh, 1, 3, s, l, hop_le, kieu[0]);
    if (kh.so_buoc == 0) themBuoc(kh, matNaThang(2, 4), ts.t_min);   // cả ngã tư trống: trục tới lượt xanh Tmin
  } else {
    keHoachTruc(ts, kh, 1, 3, s, l, hop_le, kieu[0]);
    if (kh.so_buoc == 0) keHoachTruc(ts, kh, 2, 4, s, l, hop_le, kieu[1]);
    if (kh.so_buoc == 0) themBuoc(kh, matNaThang(1, 3), ts.t_min);
  }
  return kh;
}
