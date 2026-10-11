// dieu_khien.h — Máy trạng thái chạy kế hoạch đèn: xanh → vàng → đỏ toàn phần → bước kế.
// Không dùng hàm nào của Arduino nên biên dịch được cả trên máy tính (xem kiem_thu/).
// Bản JavaScript tương ứng: web/dieu_khien.js (hai bản phải cho cùng vết đèn).
// Nguyên tắc: kế hoạch chỉ đổi ở ranh giới chu kì; ngoại lệ duy nhất là xe ưu tiên,
// và khi đó vẫn đi đủ vàng rồi đỏ toàn phần.
#pragma once
#include "quyet_dinh.h"

#define PHA_KHOI_DONG 0
#define PHA_XANH 1
#define PHA_VANG 2
#define PHA_DO_DON 3

#define MAU_DO 0
#define MAU_VANG 1
#define MAU_XANH 2

#define SK_CHU_KI 1        // vừa chốt kế hoạch chu kì mới
#define SK_BUOC 2          // một tổ hợp vừa xanh
#define SK_UU_TIEN 3       // bắt đầu ưu tiên (dl = hướng)
#define SK_HET_UU_TIEN 4   // hết ưu tiên (dl = hướng)
#define SK_LOI 5           // lỗi an toàn

struct SoDem {
  uint8_t s, l, emg;
  uint32_t t_nhan;         // lúc nhận tin gần nhất
  bool co;                 // đã từng nhận tin
  bool giu;                // tin gõ tay qua Serial: không tính hết hạn
  uint32_t emg_tu;         // lúc bắt đầu thấy xe ưu tiên
};

// a đã tới hoặc qua b (so sánh an toàn khi bộ đếm thời gian quay vòng)
inline bool daToi(uint32_t a, uint32_t b) { return (int32_t)(a - b) >= 0; }

class DieuKhien {
public:
  ThamSo *ts;
  SoDem dem[5];            // chỉ số 1..4 theo số hướng
  bool ut_khoa[5];
  uint8_t pha;
  uint8_t xanh;            // mặt nạ các nhóm đang xanh
  uint8_t vang;            // mặt nạ các nhóm đang vàng
  uint8_t dich;            // tổ hợp sẽ xanh sau khi chuyển xong
  uint32_t dich_xanh;      // thời gian xanh của tổ hợp đó
  uint32_t t_het;          // thời điểm hết trạng thái hiện tại
  uint32_t t_xanh_tu;      // thời điểm bước hiện tại bắt đầu xanh
  KeHoach kh;              // kế hoạch của chu kì đang chạy
  int i_buoc;
  uint32_t chu_ki;
  uint8_t ut_huong;        // đang ưu tiên hướng nào (0: không)
  uint32_t ut_bat_dau;
  uint32_t ut_thay;        // lần cuối còn thấy xe ưu tiên
  bool loi;
  void (*khiSuKien)(uint8_t loai, uint8_t dl);

  explicit DieuKhien(ThamSo *tham_so) : ts(tham_so) {
    for (int h = 0; h <= 4; h++) {
      dem[h].s = 0; dem[h].l = 0; dem[h].emg = 0; dem[h].t_nhan = 0;
      dem[h].co = false; dem[h].giu = false; dem[h].emg_tu = 0;
      ut_khoa[h] = false;
    }
    pha = PHA_KHOI_DONG; xanh = 0; vang = 0; dich = 0; dich_xanh = 0;
    t_het = 0; t_xanh_tu = 0; kh.so_buoc = 0; i_buoc = -1; chu_ki = 0;
    ut_huong = 0; ut_bat_dau = 0; ut_thay = 0; loi = false; khiSuKien = 0;
  }

  void khoiDong(uint32_t t) {
    pha = PHA_KHOI_DONG;   // bật máy: đỏ toàn phần một lúc rồi mới chạy
    xanh = 0; vang = 0; loi = false;
    kh.so_buoc = 0; i_buoc = -1; chu_ki = 0; ut_huong = 0;
    t_het = t + ts->t_do;
  }

  // Nhận một bản tin đếm xe. giu = true: tin gõ tay qua Serial, không tính hết hạn.
  bool nhanDem(int id, int s, int l, int emg, uint32_t t, bool giu) {
    if (!(id >= 1 && id <= 4)) return false;
    SoDem &d = dem[id];
    if (s < 0) s = 0;
    if (s > 99) s = 99;
    if (l < 0) l = 0;
    if (l > 99) l = 99;
    if (emg < 0 || emg > 2) emg = 0;
    if (emg != 0 && (!d.co || d.emg == 0)) d.emg_tu = t;
    if (emg == 0) ut_khoa[id] = false;
    d.s = (uint8_t)s; d.l = (uint8_t)l; d.emg = (uint8_t)emg; d.t_nhan = t; d.co = true; d.giu = giu;
    return true;
  }

  bool conHan(int h, uint32_t t) const {
    const SoDem &d = dem[h];
    // so sánh có dấu: tin đến sau mốc t (t là mốc danh định của chu kì) vẫn là tin còn hạn
    return d.co && (d.giu || (int32_t)(t - d.t_nhan) <= (int32_t)ts->mat_tin_hieu);
  }

  bool thayXeUuTien(int h, uint32_t t) const {
    return conHan(h, t) && dem[h].emg != 0;
  }

  int huongCanUuTien(uint32_t t) const {
    int chon = 0;
    for (int h = 1; h <= 4; h++) {
      if (!thayXeUuTien(h, t) || ut_khoa[h]) continue;
      if (chon == 0 || (int32_t)(dem[h].emg_tu - dem[chon].emg_tu) < 0) chon = h;
    }
    return chon;
  }

  void baoSuKien(uint8_t loai, uint8_t dl) {
    if (khiSuKien) khiSuKien(loai, dl);
  }

  void baoLoi() {
    loi = true; xanh = 0; vang = 0;
    baoSuKien(SK_LOI, 0);
  }

  void vaoXanh(uint32_t t) {
    if (!hopLeMatNa(dich)) { baoLoi(); return; }
    xanh = dich; vang = 0;
    pha = PHA_XANH;
    t_xanh_tu = t;
    t_het = t + dich_xanh;
    if (ut_huong) ut_thay = t;   // xanh ưu tiên luôn giữ ít nhất ut_giu
    baoSuKien(SK_BUOC, 0);
  }

  // Chuyển sang tổ hợp mới: nhóm có mặt ở cả hai thì giữ xanh, nhóm rời đi qua vàng rồi đỏ.
  void chuyenSang(uint8_t dich_moi, uint32_t xanh_ms, uint32_t t) {
    dich = dich_moi; dich_xanh = xanh_ms;
    uint8_t roi = xanh & (uint8_t)~dich_moi;
    if (roi == 0) { vaoXanh(t); return; }
    xanh &= dich_moi; vang = roi;
    pha = PHA_VANG;
    t_het = t + ts->t_vang;
  }

  // Chốt kế hoạch cho chu kì mới theo số đếm lúc này.
  void chuKiMoi(uint32_t t) {
    uint8_t s[5] = {0, 0, 0, 0, 0}, l[5] = {0, 0, 0, 0, 0};
    bool hop_le[5] = {false, false, false, false, false};
    for (int h = 1; h <= 4; h++) {
      s[h] = dem[h].s; l[h] = dem[h].l; hop_le[h] = conHan(h, t);
    }
    kh = lapKeHoach(*ts, s, l, hop_le);
    chu_ki++;
    i_buoc = 0;
    baoSuKien(SK_CHU_KI, 0);
    chuyenSang(kh.buoc[0].mat_na, kh.buoc[0].xanh, t);
  }

  bool motBuoc(uint32_t t) {
    switch (pha) {
      case PHA_KHOI_DONG:
        if (!daToi(t, t_het)) return false;
        chuKiMoi(t_het);
        return true;
      case PHA_XANH:
        if (ut_huong) {                      // đang giữ xanh cho xe ưu tiên
          if (thayXeUuTien(ut_huong, t)) ut_thay = t;
          bool qua_lau = (uint32_t)(t - ut_bat_dau) >= ts->ut_toi_da;
          if (!qua_lau && (uint32_t)(t - ut_thay) < ts->ut_giu) return false;
          if (qua_lau) ut_khoa[ut_huong] = true;
          baoSuKien(SK_HET_UU_TIEN, ut_huong);
          ut_huong = 0;
          chuKiMoi(t);
          return true;
        }
        if ((uint32_t)(t - t_xanh_tu) >= ts->ut_xanh_an_toan) {
          int h = huongCanUuTien(t);
          if (h) {
            ut_huong = (uint8_t)h; ut_bat_dau = t; ut_thay = t;
            i_buoc = -1;
            baoSuKien(SK_UU_TIEN, (uint8_t)h);
            chuyenSang(matNaRieng(h), 0, t);
            return true;
          }
        }
        if (!daToi(t, t_het)) return false;
        if (i_buoc + 1 < (int)kh.so_buoc) {
          i_buoc++;
          chuyenSang(kh.buoc[i_buoc].mat_na, kh.buoc[i_buoc].xanh, t_het);
        } else {
          chuKiMoi(t_het);
        }
        return true;
      case PHA_VANG:
        if (!daToi(t, t_het)) return false;
        vang = 0;
        pha = PHA_DO_DON;
        t_het += ts->t_do;
        return true;
      case PHA_DO_DON:
        if (!daToi(t, t_het)) return false;
        vaoXanh(t_het);
        return true;
    }
    return false;
  }

  // Gọi thường xuyên với thời gian hiện tại (ms).
  void capNhat(uint32_t t) {
    for (int i = 0; i < 16 && !loi; i++) {
      if (!motBuoc(t)) break;
    }
  }

  // Màu của nhóm g (0..7) lúc t. Khi có lỗi an toàn: mọi nhóm nháy vàng.
  uint8_t mauNhom(int g, uint32_t t) const {
    if (loi) return ((t / 500) % 2 == 0) ? MAU_VANG : MAU_DO;
    if (xanh & (1u << g)) return MAU_XANH;
    if (vang & (1u << g)) return MAU_VANG;
    return MAU_DO;
  }

  // Thời gian còn lại của trạng thái hiện tại; -1 khi đang giữ xanh cho xe ưu tiên.
  int32_t conLai(uint32_t t) const {
    if (pha == PHA_XANH && ut_huong) return -1;
    return daToi(t, t_het) ? 0 : (int32_t)(t_het - t);
  }
};
