// dieu_khien.h — Máy trạng thái chạy kế hoạch đèn: xanh → vàng → đỏ toàn phần → bước kế.
// Không dùng hàm nào của Arduino nên biên dịch được cả trên máy tính (xem kiem_thu/).
// Bản JavaScript tương ứng: web/dieu_khien.js (hai bản phải cho cùng vết đèn).
// Nguyên tắc: tổ hợp và thứ tự các bước chốt ở đầu mỗi lượt (chế độ thích ứng: lượt của một trục;
// chế độ cố định: cả chu kì). Hai ngoại lệ, đều vẫn đi đủ vàng rồi đỏ toàn phần: xe ưu tiên, và
// cắt xanh sớm khi tổ hợp đang xanh hết xe (chế độ thích ứng).
// Ngoài ra bộ điều khiển còn tự học kiểu chia pha: xem phần "Tự học" ở dưới.
#pragma once
#include "quyet_dinh.h"

#define PHA_KHOI_DONG 0
#define PHA_XANH 1
#define PHA_VANG 2
#define PHA_DO_DON 3

#define MAU_DO 0
#define MAU_VANG 1
#define MAU_XANH 2

#define SK_CHU_KI 1        // vừa chốt kế hoạch mới (thích ứng: một lượt; cố định: một chu kì)
#define SK_BUOC 2          // một tổ hợp vừa xanh
#define SK_UU_TIEN 3       // bắt đầu ưu tiên (dl = hướng)
#define SK_HET_UU_TIEN 4   // hết ưu tiên (dl = hướng)
#define SK_LOI 5           // lỗi an toàn
#define SK_CAT_SOM 6       // cắt xanh sớm vì tổ hợp đang xanh hết xe (dl = mặt nạ tổ hợp bị cắt)
#define SK_HOC_THU 7       // tự học: một lượt chạy kiểu khác kiểu đang giữ (dl = trục * 4 + kiểu của lượt đó)
#define SK_HOC_NHAN 8      // tự học: trục đổi sang kiểu mới (dl = trục * 4 + kiểu mới)
#define SK_HOC_TRA 9       // tự học: lượt thử "thẳng trước" không đạt, vẫn giữ "riêng" (dl như trên)
#define SK_HOC_HUY 10      // tự học: bỏ lượt thử vì có xe ưu tiên hoặc thiếu camera (dl = trục)

// Loại xe ưu tiên, cũng là thứ tự ưu tiên "hỏa → sự → công → thương": số nhỏ hơn đi trước.
#define UT_KHONG 0
#define UT_CUU_HOA 1
#define UT_QUAN_SU 2
#define UT_CONG_AN 3
#define UT_CUU_THUONG 4

struct SoDem {
  uint8_t s, l, emg;
  uint8_t ut;              // loại xe ưu tiên đang thấy (UT_KHONG nếu không có)
  uint32_t t_nhan;         // lúc nhận tin gần nhất
  bool co;                 // đã từng nhận tin
  bool giu;                // tin gõ tay qua Serial: không tính hết hạn
  uint32_t emg_tu;         // lúc bắt đầu thấy xe ưu tiên
};

// Trạng thái của bộ tự học kiểu chia pha (giải thích ở phần "Tự học" cuối file)
struct Hoc {
  uint8_t kieu[2];         // kiểu đang giữ của trục dọc, trục ngang
  int64_t b[2];            // mốc của kiểu riêng (ms xanh mỗi xe): [lượt xả hết, lượt quá tải]; -1 = chưa đo
  int64_t q[2];            // điểm của "thẳng trước" so với "riêng" ở từng trục (%); -1 = chưa đo
  uint32_t dem[2];         // số lượt đã đo kể từ lần thử gần nhất
  uint32_t cach[2];        // cách mấy lượt thì thử "thẳng trước" một lần
  uint8_t lien[2];         // số lần thử liên tiếp đạt
  uint32_t tuoi[2];        // số lượt "thẳng trước" kể từ lần gần nhất có lượt "riêng" để lấy mốc
  // lượt đang chạy
  int8_t l_truc;           // trục của lượt (-1: không theo dõi)
  uint8_t l_kieu;          // kiểu của lượt
  bool l_do;               // lượt đo được: cả hai hướng có xe và làn trong có xe, tức kế hoạch có phụ thuộc kiểu
  bool l_thu;              // lượt chạy kiểu khác kiểu đang giữ
  bool l_hong;             // có xe ưu tiên hoặc thiếu camera
  bool l_qt;               // quá tải: có bước hết giờ mà vẫn còn xe
  uint64_t l_g;            // tổng thời gian xanh tới lúc hết xe (ms)
  uint32_t l_n;            // số bước đã chạy
  uint32_t ra[2];          // số xe đã qua vạch dừng của trục dọc, trục ngang trong lượt
  bool b_trong;            // bước đang xanh hiện không còn xe...
  uint32_t b_trong_tu;     // ... kể từ lúc này
};

// a đã tới hoặc qua b (so sánh an toàn khi bộ đếm thời gian quay vòng)
inline bool daToi(uint32_t a, uint32_t b) { return (int32_t)(a - b) >= 0; }

// Hai kiểu chia pha mà bộ tự học chọn giữa: "riêng từng hướng" và "cùng đi thẳng trước".
inline uint8_t kieuNguoc(uint8_t k) { return k == KIEU_RIENG ? KIEU_THANG_TRUOC : KIEU_RIENG; }

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
  KeHoach kh;              // kế hoạch đang chạy (thích ứng: của một lượt; cố định: của cả chu kì)
  uint8_t che_do_kh;       // chế độ lúc chốt kế hoạch đó
  int i_buoc;
  uint32_t chu_ki;         // số lần đã chốt kế hoạch
  bool dang_trong;         // tổ hợp đang xanh hiện không còn xe
  uint32_t t_trong_tu;     // ... kể từ lúc này
  uint8_t ut_huong;        // đang ưu tiên hướng nào (0: không)
  uint32_t ut_bat_dau;
  uint32_t ut_thay;        // lần cuối còn thấy xe ưu tiên
  uint8_t truc_ke;         // lượt kế là của trục nào (0 dọc, 1 ngang)
  Hoc hoc;
  bool loi;
  void (*khiSuKien)(uint8_t loai, uint8_t dl);

  explicit DieuKhien(ThamSo *tham_so) : ts(tham_so) {
    for (int h = 0; h <= 4; h++) {
      dem[h].s = 0; dem[h].l = 0; dem[h].emg = 0; dem[h].ut = 0; dem[h].t_nhan = 0;
      dem[h].co = false; dem[h].giu = false; dem[h].emg_tu = 0;
      ut_khoa[h] = false;
    }
    pha = PHA_KHOI_DONG; xanh = 0; vang = 0; dich = 0; dich_xanh = 0;
    t_het = 0; t_xanh_tu = 0; kh.so_buoc = 0; che_do_kh = CHE_DO_CO_DINH; i_buoc = -1; chu_ki = 0;
    dang_trong = false; t_trong_tu = 0;
    ut_huong = 0; ut_bat_dau = 0; ut_thay = 0; truc_ke = 0; loi = false; khiSuKien = 0;
    hocMoi();
  }

  void khoiDong(uint32_t t) {
    pha = PHA_KHOI_DONG;   // bật máy: đỏ toàn phần một lúc rồi mới chạy
    xanh = 0; vang = 0; loi = false;
    kh.so_buoc = 0; i_buoc = -1; chu_ki = 0; ut_huong = 0;
    dang_trong = false;
    truc_ke = 0;
    hocMoi();
    t_het = t + ts->t_do;
  }

  // Nhận một bản tin đếm xe. giu = true: tin gõ tay qua Serial, không tính hết hạn.
  // ut: loại xe ưu tiên (1..4); bản tin cũ chỉ có emg thì coi là xe cứu thương.
  bool nhanDem(int id, int s, int l, int emg, uint32_t t, bool giu, int ut = 0) {
    if (!(id >= 1 && id <= 4)) return false;
    SoDem &d = dem[id];
    if (s < 0) s = 0;
    if (s > 99) s = 99;
    if (l < 0) l = 0;
    if (l > 99) l = 99;
    if (emg < 0 || emg > 2) emg = 0;
    if (!(ut >= 1 && ut <= 4)) ut = 0;
    if (emg != 0 && ut == 0) ut = UT_CUU_THUONG;
    if (ut != 0 && emg == 0) emg = 1;
    if (ut != 0 && (!d.co || d.ut == 0)) d.emg_tu = t;
    if (ut == 0) ut_khoa[id] = false;
    // tự học: số đếm của một làn giảm 1 là một xe vừa qua vạch dừng; cộng cho trục của hướng đó
    if (d.co) hoc.ra[(id - 1) & 1] += (uint32_t)((d.s > s ? d.s - s : 0) + (d.l > l ? d.l - l : 0));
    d.s = (uint8_t)s; d.l = (uint8_t)l; d.emg = (uint8_t)emg; d.ut = (uint8_t)ut;
    d.t_nhan = t; d.co = true; d.giu = giu;
    return true;
  }

  bool conHan(int h, uint32_t t) const {
    const SoDem &d = dem[h];
    // so sánh có dấu: tin đến sau mốc t (t là mốc danh định của chu kì) vẫn là tin còn hạn
    return d.co && (d.giu || (int32_t)(t - d.t_nhan) <= (int32_t)ts->mat_tin_hieu);
  }

  bool thayXeUuTien(int h, uint32_t t) const {
    return conHan(h, t) && dem[h].ut != 0;
  }

  // Hướng có xe ưu tiên cần được mở đèn. Nhiều hướng cùng có xe ưu tiên thì theo thứ tự
  // hỏa → sự → công → thương (số ut nhỏ hơn đi trước); cùng hạng thì xe thấy trước đi trước.
  int huongCanUuTien(uint32_t t, int bo_qua) const {
    int chon = 0;
    for (int h = 1; h <= 4; h++) {
      if (h == bo_qua || !thayXeUuTien(h, t) || ut_khoa[h]) continue;
      const SoDem &d = dem[h], &c = dem[chon];
      if (chon == 0 || d.ut < c.ut || (d.ut == c.ut && (int32_t)(d.emg_tu - c.emg_tu) < 0)) chon = h;
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
    dang_trong = false;
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

  // Kiểu chia pha của {trục dọc, trục ngang} cho lượt sắp chốt.
  void kieuChoChuKi(uint8_t ra[2]) const {
    uint32_t k = ts->kieu_pha;
    if (k != KIEU_TU_HOC) { ra[0] = ra[1] = (uint8_t)((k == KIEU_THANG_TRUOC || k == KIEU_DOI_XUNG) ? k : KIEU_RIENG); return; }
    ra[0] = hoc.kieu[0]; ra[1] = hoc.kieu[1];
    for (int i = 0; i < 2; i++) if (hocSapThu(i)) ra[i] = kieuNguoc(ra[i]);
  }

  // Chốt kế hoạch mới theo số đếm lúc này. Số thứ tự chu_ki đếm số lần chốt (thích ứng: số lượt).
  void chuKiMoi(uint32_t t) {
    hocCuoiLuot(t);
    uint8_t s[5] = {0, 0, 0, 0, 0}, l[5] = {0, 0, 0, 0, 0};
    bool hop_le[5] = {false, false, false, false, false};
    for (int h = 1; h <= 4; h++) {
      s[h] = dem[h].s; l[h] = dem[h].l; hop_le[h] = conHan(h, t);
    }
    uint8_t kieu[2];
    kieuChoChuKi(kieu);
    kh = lapKeHoach(*ts, s, l, hop_le, kieu, truc_ke);
    che_do_kh = ts->che_do;
    if (che_do_kh == CHE_DO_THICH_UNG) {
      uint8_t truc = trucCuaMatNa(kh.buoc[0].mat_na);
      truc_ke = (uint8_t)(1 - truc);
      hocDauLuot(truc, kieu[truc], s, l, hop_le);
    }
    chu_ki++;
    i_buoc = 0;
    baoSuKien(SK_CHU_KI, 0);
    chuyenSang(kh.buoc[0].mat_na, kh.buoc[0].xanh, t);
  }

  // Số xe mà tổ hợp m phục vụ: nhóm thẳng phục vụ cả hai làn, nhóm trái chỉ phục vụ làn trong.
  // Trả về -1 nếu camera của một hướng trong tổ hợp mất tín hiệu (không biết thì không cắt, không bỏ).
  int nhuCau(uint8_t m, uint32_t t) const {
    int n = 0;
    for (int h = 1; h <= 4; h++) {
      bool thang = (m & bitThang(h)) != 0, trai = (m & bitTrai(h)) != 0;
      if (!thang && !trai) continue;
      if (!conHan(h, t)) return -1;
      n += thang ? dem[h].s + dem[h].l : dem[h].l;
    }
    return n;
  }

  // Có xe đang chờ mà tổ hợp m không phục vụ không? Camera mất tín hiệu thì coi như có.
  bool coXeChoNgoai(uint8_t m, uint32_t t) const {
    for (int h = 1; h <= 4; h++) {
      if (m & bitThang(h)) continue;
      if (!conHan(h, t)) return true;
      if (m & bitTrai(h)) { if (dem[h].s > 0) return true; }
      else if (dem[h].s + dem[h].l > 0) return true;
    }
    return false;
  }

  // Như nhuCau và coXeChoNgoai nhưng cho bước thứ i của kế hoạch. Bước "cùng đi thẳng" có đánh dấu
  // ngoai chỉ tính xe ở làn ngoài; xe làn trong của chính hai hướng đó được coi là đang chờ bước sau.
  int nhuCauBuoc(int i, uint32_t t) const {
    const Buoc &b = kh.buoc[i];
    if (!b.ngoai) return nhuCau(b.mat_na, t);
    int n = 0;
    for (int h = 1; h <= 4; h++) {
      if (!(b.mat_na & bitThang(h))) continue;
      if (!conHan(h, t)) return -1;
      n += dem[h].s;
    }
    return n;
  }

  bool coXeChoBuoc(int i, uint32_t t) const {
    const Buoc &b = kh.buoc[i];
    if (b.ngoai) {
      for (int h = 1; h <= 4; h++) {
        if ((b.mat_na & bitThang(h)) && conHan(h, t) && dem[h].l > 0) return true;
      }
    }
    return coXeChoNgoai(b.mat_na, t);
  }

  bool duocCatSom() const {
    return che_do_kh == CHE_DO_THICH_UNG && ts->cat_som > 0;
  }

  // Sang bước kế của kế hoạch. Khi bật cắt sớm, bước nào không còn xe thì bỏ qua.
  void buocKe(uint32_t t) {
    hocCuoiBuoc(t);
    int i = i_buoc + 1;
    if (duocCatSom()) {
      while (i < (int)kh.so_buoc && nhuCauBuoc(i, t) == 0) i++;
    }
    if (i < (int)kh.so_buoc) {
      i_buoc = i;
      chuyenSang(kh.buoc[i].mat_na, kh.buoc[i].xanh, t);
    } else {
      chuKiMoi(t);
    }
  }

  // Mở riêng hướng h cho xe ưu tiên: các nhóm khác đang xanh đi qua vàng rồi đỏ toàn phần.
  void batDauUuTien(int h, uint32_t t) {
    ut_huong = (uint8_t)h; ut_bat_dau = t; ut_thay = t;
    i_buoc = -1;
    hoc.l_hong = true;     // lượt bị xe ưu tiên cắt ngang: tự học không tính lượt này
    baoSuKien(SK_UU_TIEN, (uint8_t)h);
    chuyenSang(matNaRieng(h), 0, t);
  }

  // Hết ưu tiên cho hướng đang mở: còn xe ưu tiên ở hướng khác thì mở tiếp cho nó, không thì quay
  // lại chạy bình thường, bắt đầu từ trục đã phải chờ.
  void ketThucUuTien(uint32_t t) {
    uint8_t cu = ut_huong;
    baoSuKien(SK_HET_UU_TIEN, cu);
    ut_huong = 0;
    int h = huongCanUuTien(t, 0);
    if (h) { batDauUuTien(h, t); return; }
    truc_ke = (cu == 1 || cu == 3) ? 1 : 0;
    chuKiMoi(t);
  }

  bool motBuoc(uint32_t t) {
    int h;
    // trong lúc đèn đang chuyển (vàng, đỏ toàn phần) vẫn ghi nhận lần cuối còn thấy xe ưu tiên
    if (ut_huong && pha != PHA_XANH && thayXeUuTien(ut_huong, t)) ut_thay = t;
    switch (pha) {
      case PHA_KHOI_DONG:
        if (!daToi(t, t_het)) return false;
        chuKiMoi(t_het);
        return true;
      case PHA_XANH:
        if (ut_huong) {                      // đang giữ xanh cho xe ưu tiên
          bool thay = thayXeUuTien(ut_huong, t);
          if (thay) ut_thay = t;
          if ((uint32_t)(t - t_xanh_tu) >= ts->ut_xanh_an_toan) {
            // hướng khác có xe ưu tiên hạng cao hơn (hoặc xe của hướng này đã qua): nhường ngay
            h = huongCanUuTien(t, ut_huong);
            if (h && (!thay || dem[h].ut < dem[ut_huong].ut)) {
              baoSuKien(SK_HET_UU_TIEN, ut_huong);
              batDauUuTien(h, t);
              return true;
            }
          }
          // xe ưu tiên vừa qua (không còn thấy quá ut_giu) là chuyển vàng ngay, không giữ xanh thêm
          bool qua_lau = (uint32_t)(t - ut_bat_dau) >= ts->ut_toi_da;
          if (!qua_lau && (thay || (uint32_t)(t - ut_thay) < ts->ut_giu)) return false;
          if (qua_lau) ut_khoa[ut_huong] = true;
          ketThucUuTien(t);
          return true;
        }
        if ((uint32_t)(t - t_xanh_tu) >= ts->ut_xanh_an_toan) {
          h = huongCanUuTien(t, 0);
          if (h) { batDauUuTien(h, t); return true; }
        }
        if (duocCatSom() && i_buoc >= 0) {
          // Cắt xanh sớm: tổ hợp đang xanh không còn xe suốt cat_som, và có xe khác đang chờ
          if (nhuCauBuoc(i_buoc, t) == 0) {
            if (!dang_trong) { dang_trong = true; t_trong_tu = t; }
          } else dang_trong = false;
          if (dang_trong && (uint32_t)(t - t_trong_tu) >= ts->cat_som && coXeChoBuoc(i_buoc, t)) {
            baoSuKien(SK_CAT_SOM, xanh);
            buocKe(t);
            return true;
          }
        }
        if (!daToi(t, t_het)) return false;
        buocKe(t_het);
        return true;
      case PHA_VANG:
        if (!daToi(t, t_het)) return false;
        vang = 0;
        pha = PHA_DO_DON;
        t_het += ts->t_do;
        return true;
      case PHA_DO_DON:
        if (!daToi(t, t_het)) return false;
        if (ut_huong && !thayXeUuTien(ut_huong, t) && (uint32_t)(t - ut_thay) >= ts->ut_giu) {
          ketThucUuTien(t);          // xe ưu tiên đã đi mất trong lúc chuyển đèn: không mở riêng nữa
          return true;
        }
        vaoXanh(t_het);
        return true;
    }
    return false;
  }

  // Gọi thường xuyên với thời gian hiện tại (ms).
  void capNhat(uint32_t t) {
    hocTheoDoi(t);
    for (int i = 0; i < 16 && !loi; i++) {
      if (!motBuoc(t)) break;
    }
  }

  // ============================== Tự học kiểu chia pha ==============================
  // Camera không biết xe ở làn trong sẽ đi thẳng hay rẽ trái, nên không tính trước được kiểu chia
  // pha nào tốt hơn. Bộ điều khiển học bằng cách đo từng LƯỢT của từng trục, chỉ dùng số đếm camera:
  //   - Thời gian đèn có ích của một lượt: E = tổng (thời gian xanh của mỗi bước, tính tới lúc các làn
  //     của bước đó hết xe) + số bước × (vàng + đỏ toàn phần + thời gian chờ cắt sớm).
  //   - Số xe đã qua vạch dừng trong lượt: mỗi lần số đếm của một làn giảm 1 là một xe.
  //   - Lượt chạy kiểu "riêng" cho mốc so sánh b = (thời gian xanh) / (số xe), ms mỗi xe. Giữ hai mốc:
  //     b[0] cho lượt xả hết xe, b[1] cho lượt quá tải (có bước hết giờ mà vẫn còn xe), vì hai loại
  //     lượt này chạy với nhịp xả khác hẳn nhau và chỉ nên so với lượt cùng loại.
  //   - Lượt chạy kiểu "thẳng trước" cho điểm q = E × 100 / (thời gian mà kiểu riêng cần cho cùng
  //     số xe) = E × 100 / (2 bước × thời gian chuyển + b × số xe). q dưới 100 là thẳng trước nhanh hơn.
  // Luật chọn, riêng cho từng trục:
  //   - Đang giữ "riêng": cứ hoc_cach_thu lượt thì chạy thử một lượt "thẳng trước". Lần thử có
  //     q ≤ 100 − hoc_nguong thì lượt sau thử tiếp; trung bình hai lần vẫn đạt thì đổi sang "thẳng
  //     trước". Lần thử kém rõ (q ≥ 100 + hoc_nguong) thì giãn khoảng cách thử ra gấp đôi, tối đa 2 lần
  //     mức đầu; lần thử không đạt mà không kém rõ thì về mức đầu.
  //   - Đang giữ "thẳng trước": mỗi lượt cập nhật điểm Q (trung bình trượt của q). Q ≥ 100 + hoc_nguong / 2
  //     thì trả về "riêng". Thỉnh thoảng chạy một lượt "riêng" để làm mới mốc b: cách hoc_cach_thu
  //     lượt khi Q còn gần 100, cách 4 lần mức đó khi Q đã thấp rõ.
  //   - Lúc quá tải, thử sai rất tốn (hàng xe dài thêm và lâu tan) nên đòi bằng chứng mạnh hơn: lượt thử
  //     quá tải phải đạt q ≤ 100 − 2 × hoc_nguong, không đạt là giãn khoảng cách thử (tối đa 4 lần mức
  //     đầu); đang giữ "thẳng trước" mà lượt quá tải có Q ≥ 100 là trả về "riêng".
  //   - Lượt có xe ưu tiên, thiếu camera, hoặc quá ít xe thì không tính. Khi hàng xe luôn đầy vùng
  //     camera và số đếm không nhúc nhích thì không đếm được xe qua, bộ tự học giữ nguyên kiểu đang có.
  // Kiểu "đối xứng" không được thử: làn trong đi chung thẳng và trái nên thử sai là kẹt cả ngã tư.
  void hocMoi() {
    uint32_t cach = ts->hoc_cach_thu > 0 ? ts->hoc_cach_thu : 1;
    hoc.kieu[0] = KIEU_RIENG; hoc.kieu[1] = KIEU_RIENG;
    hoc.b[0] = -1; hoc.b[1] = -1; hoc.q[0] = -1; hoc.q[1] = -1;
    hoc.dem[0] = cach > 1 ? cach - 1 : 0; hoc.dem[1] = cach > 2 ? cach - 2 : 0;   // hai trục lệch nhau một lượt
    hoc.cach[0] = cach; hoc.cach[1] = cach;
    hoc.lien[0] = 0; hoc.lien[1] = 0; hoc.tuoi[0] = 0; hoc.tuoi[1] = 0;
    hoc.l_truc = -1; hoc.l_kieu = 0;
    hoc.l_do = false; hoc.l_thu = false; hoc.l_hong = false; hoc.l_qt = false;
    hoc.l_g = 0; hoc.l_n = 0; hoc.ra[0] = 0; hoc.ra[1] = 0;
    hoc.b_trong = false; hoc.b_trong_tu = 0;
  }

  bool hocDangBat() const {
    return ts->che_do == CHE_DO_THICH_UNG && ts->kieu_pha == KIEU_TU_HOC && ts->hoc_cach_thu > 0;
  }

  // Lượt sắp tới của trục k có chạy kiểu khác kiểu đang giữ không (thử "thẳng trước", hoặc làm mới mốc "riêng").
  bool hocSapThu(int k) const {
    uint32_t cach = ts->hoc_cach_thu;
    if (!hocDangBat()) return false;
    if (hoc.kieu[k] == KIEU_RIENG) return hoc.dem[k] >= hoc.cach[k] && (hoc.b[0] >= 0 || hoc.b[1] >= 0);
    bool thap_ro = hoc.q[k] >= 0 && hoc.q[k] < 100 - 2 * (int64_t)ts->hoc_nguong;
    return hoc.tuoi[k] >= (thap_ro ? 4 * cach : cach);
  }

  // Gọi mỗi nhịp, trước khi xét chuyển đèn: đánh dấu lượt hỏng, và ghi lúc bước đang xanh hết xe.
  void hocTheoDoi(uint32_t t) {
    Hoc &hc = hoc;
    if (hc.l_truc < 0) return;
    if (!hocDangBat() || ut_huong) { hc.l_hong = true; return; }
    for (int h = 1; h <= 4; h++) if (!conHan(h, t)) hc.l_hong = true;
    if (pha == PHA_XANH && i_buoc >= 0) {
      if (nhuCauBuoc(i_buoc, t) == 0) {
        if (!hc.b_trong) { hc.b_trong = true; hc.b_trong_tu = t; }
      } else hc.b_trong = false;
    }
  }

  // Bắt đầu theo dõi một lượt của trục truc, chạy kiểu kieu.
  void hocDauLuot(uint8_t truc, uint8_t kieu, const uint8_t s[5], const uint8_t l[5], const bool hop_le[5]) {
    Hoc &hc = hoc;
    int a = truc ? 2 : 1, b = a + 2;
    hc.l_truc = (int8_t)truc; hc.l_kieu = kieu;
    hc.l_hong = false; hc.l_qt = false; hc.l_g = 0; hc.l_n = 0; hc.ra[0] = 0; hc.ra[1] = 0; hc.b_trong = false;
    hc.l_do = hocDangBat() && hop_le[a] && hop_le[b] && s[a] + l[a] > 0 && s[b] + l[b] > 0 && (l[a] > 0 || l[b] > 0);
    hc.l_thu = hc.l_do && kieu != hc.kieu[truc];
    if (hc.l_thu) baoSuKien(SK_HOC_THU, (uint8_t)(truc * 4 + kieu));
  }

  // Hết một bước (do cắt sớm hoặc hết giờ): cộng thời gian xanh của bước, tính tới lúc hết xe.
  void hocCuoiBuoc(uint32_t t) {
    Hoc &hc = hoc;
    if (hc.l_truc >= 0 && i_buoc >= 0) {
      bool trong = hc.b_trong && daToi(hc.b_trong_tu, t_xanh_tu);
      if (!trong) hc.l_qt = true;
      hc.l_g += (uint32_t)((trong ? hc.b_trong_tu : t) - t_xanh_tu);
      hc.l_n++;
    }
    hc.b_trong = false;
  }

  void hocDoiKieu(int k, uint8_t kieu) {
    hoc.kieu[k] = kieu; hoc.lien[k] = 0; hoc.dem[k] = 0; hoc.tuoi[k] = 0;
    hoc.cach[k] = ts->hoc_cach_thu;
    baoSuKien(SK_HOC_NHAN, (uint8_t)(k * 4 + kieu));
  }

  // Gọi mỗi lần sắp chốt kế hoạch mới: chấm lượt vừa chạy xong.
  void hocCuoiLuot(uint32_t t) {
    (void)t;
    Hoc &hc = hoc;
    if (hc.l_truc >= 0 && hc.l_do) {
      int k = hc.l_truc;
      int64_t ra = hc.ra[k], ng = ts->hoc_nguong;
      uint32_t cach = ts->hoc_cach_thu;
      if (!hocDangBat() || hc.l_hong) {
        if (hc.l_thu) baoSuKien(SK_HOC_HUY, (uint8_t)k);
      } else if (ra > 0 && ra >= (int64_t)ts->hoc_so_xe && hc.l_n > 0) {
        int64_t c = (int64_t)ts->t_vang + ts->t_do + ts->cat_som;
        int i = hc.l_qt ? 1 : 0;
        if (hc.l_kieu == KIEU_RIENG) {
          int64_t bx = (int64_t)hc.l_g / ra;
          hc.b[i] = hc.b[i] < 0 ? bx : (hc.b[i] + bx) / 2;
          hc.tuoi[0] = 0; hc.tuoi[1] = 0;
          if (hc.kieu[k] == KIEU_RIENG) hc.dem[k]++;
        } else {
          int64_t mau = hc.b[i] < 0 ? 0 : 2 * c + hc.b[i] * ra;   // thời gian kiểu riêng cần cho cùng số xe
          hc.tuoi[k]++;
          if (mau <= 0) {
            // chưa có mốc "riêng" cùng loại để so: đang giữ "thẳng trước" thì lượt sau chạy "riêng" để lấy mốc
            if (hc.kieu[k] != KIEU_RIENG) hc.tuoi[k] = 4 * cach;
          } else {
            int64_t q = ((int64_t)hc.l_g + (int64_t)hc.l_n * c) * 100 / mau;
            if (hc.kieu[k] != KIEU_RIENG) {              // đang giữ "thẳng trước"
              hc.q[k] = hc.q[k] < 0 ? q : (hc.q[k] * 3 + q) / 4;
              if (hc.l_qt ? hc.q[k] >= 100 : hc.q[k] * 2 >= 200 + ng) hocDoiKieu(k, KIEU_RIENG);
            } else {                                     // lượt thử khi đang giữ "riêng"
              hc.q[k] = hc.lien[k] == 0 ? q : (hc.q[k] + q) / 2;
              if (hc.q[k] <= (hc.l_qt ? 100 - 2 * ng : 100 - ng)) {
                hc.lien[k]++;
                if (hc.lien[k] >= 2) hocDoiKieu(k, KIEU_THANG_TRUOC);
                else hc.dem[k] = hc.cach[k];             // đạt lần đầu: lượt sau của trục này thử tiếp
              } else {
                hc.lien[k] = 0; hc.dem[k] = 0;
                if (hc.l_qt || q >= 100 + ng) {
                  uint32_t tran = (hc.l_qt ? 4u : 2u) * cach;
                  hc.cach[k] = hc.cach[k] * 2 > tran ? tran : hc.cach[k] * 2;
                } else hc.cach[k] = cach;
                baoSuKien(SK_HOC_TRA, (uint8_t)(k * 4 + KIEU_THANG_TRUOC));
              }
            }
          }
        }
      }
    }
    hc.l_truc = -1; hc.l_do = false; hc.l_thu = false;
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
