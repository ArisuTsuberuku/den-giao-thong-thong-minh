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
#define SK_HOC_THU 7       // tự học: bắt đầu thử (dl = trục * 4 + kiểu được thử)
#define SK_HOC_NHAN 8      // tự học: kiểu thử tốt hơn, giữ lại (dl như trên)
#define SK_HOC_TRA 9       // tự học: kiểu thử không hơn, trả về kiểu cũ (dl như trên)
#define SK_HOC_HUY 10      // tự học: bỏ lần thử vì có xe ưu tiên hoặc thiếu camera (dl = trục)

// Loại xe ưu tiên, cũng là thứ tự ưu tiên "hỏa → sự → công → thương": số nhỏ hơn đi trước.
#define UT_KHONG 0
#define UT_CUU_HOA 1
#define UT_QUAN_SU 2
#define UT_CONG_AN 3
#define UT_CUU_THUONG 4

#define HOC_LUI_DAU 2      // tự học: số cửa sổ đo phải chờ giữa hai lần thử cùng một trục...
#define HOC_LUI_TOI_DA 16  // ... tăng gấp đôi sau mỗi lần thử hỏng, tối đa chừng này

struct SoDem {
  uint8_t s, l, emg;
  uint8_t ut;              // loại xe ưu tiên đang thấy (UT_KHONG nếu không có)
  uint32_t t_nhan;         // lúc nhận tin gần nhất
  bool co;                 // đã từng nhận tin
  bool giu;                // tin gõ tay qua Serial: không tính hết hạn
  uint32_t emg_tu;         // lúc bắt đầu thấy xe ưu tiên
};

// Trạng thái của bộ tự học kiểu chia pha
struct Hoc {
  uint8_t kieu[2];         // kiểu đang giữ của trục dọc, trục ngang
  bool dang_thu;           // đang chạy thử: trục truc_thu dùng kiểu ngược với kiểu đang giữ
  uint8_t truc_thu;
  uint8_t thu_ke;          // trục được ưu tiên thử ở lần tới
  uint8_t cho[2];          // còn mấy cửa sổ đo nữa thì tới lượt thử trục đó
  uint8_t lui[2];
  int64_t diem_giu;        // thời gian chờ trung bình mỗi xe (ms) của kiểu đang giữ; -1 nếu chưa đo
  int64_t diem_thu;        // ... của lần thử gần nhất
  uint64_t tich;           // cửa sổ đang đo: tổng (số xe đang chờ × thời gian), xe·ms
  uint64_t tg;             //   thời gian đã đo, ms
  uint32_t luot;           //   số lượt xe vào hoặc ra vùng camera
  uint32_t so_ck;          //   số lần chốt kế hoạch
  bool hong;               //   cửa sổ hỏng, phải đo lại
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
  uint32_t t_tich;         // tự học: đã cộng dồn hàng chờ tới thời điểm này
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
    ut_huong = 0; ut_bat_dau = 0; ut_thay = 0; truc_ke = 0; t_tich = 0; loi = false; khiSuKien = 0;
    hocMoi();
  }

  void khoiDong(uint32_t t) {
    pha = PHA_KHOI_DONG;   // bật máy: đỏ toàn phần một lúc rồi mới chạy
    xanh = 0; vang = 0; loi = false;
    kh.so_buoc = 0; i_buoc = -1; chu_ki = 0; ut_huong = 0;
    dang_trong = false;
    truc_ke = 0;
    t_tich = t;
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
    // tự học: mỗi lần số đếm của một làn tăng hay giảm 1 là một lượt xe vào hoặc ra vùng camera
    if (d.co) hoc.luot += (uint32_t)((s > d.s ? s - d.s : d.s - s) + (l > d.l ? l - d.l : d.l - l));
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
    if (hoc.dang_thu) ra[hoc.truc_thu] = kieuNguoc(ra[hoc.truc_thu]);
  }

  // Chốt kế hoạch mới theo số đếm lúc này. Số thứ tự chu_ki đếm số lần chốt (thích ứng: số lượt).
  void chuKiMoi(uint32_t t) {
    hocCuoiChuKi();
    uint8_t s[5] = {0, 0, 0, 0, 0}, l[5] = {0, 0, 0, 0, 0};
    bool hop_le[5] = {false, false, false, false, false};
    for (int h = 1; h <= 4; h++) {
      s[h] = dem[h].s; l[h] = dem[h].l; hop_le[h] = conHan(h, t);
    }
    uint8_t kieu[2];
    kieuChoChuKi(kieu);
    kh = lapKeHoach(*ts, s, l, hop_le, kieu, truc_ke);
    che_do_kh = ts->che_do;
    if (che_do_kh == CHE_DO_THICH_UNG) truc_ke = (uint8_t)(1 - trucCuaMatNa(kh.buoc[0].mat_na));
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
    hocTichLuy(t);
    for (int i = 0; i < 16 && !loi; i++) {
      if (!motBuoc(t)) break;
    }
  }

  // ============================== Tự học kiểu chia pha ==============================
  // Camera không biết xe ở làn trong sẽ đi thẳng hay rẽ trái, nên không tính trước được kiểu chia
  // pha nào tốt hơn. Bộ điều khiển vì vậy học bằng cách thử:
  //   1. Đo "thời gian chờ trung bình mỗi xe" của kiểu đang giữ trong một cửa sổ. Cửa sổ dài ít
  //      nhất hoc_so_chu_ki vòng (mỗi vòng gồm lượt dọc và lượt ngang) và phải có ít nhất hoc_so_xe xe đi qua.
  //      Cách đo chỉ dùng số đếm của camera: cộng dồn (tổng số xe 4 camera đang thấy × thời gian)
  //      được số xe·giây chờ; mỗi lần số đếm của một làn tăng hay giảm 1 là một lượt xe vào hoặc ra,
  //      nên số xe đi qua = số lượt / 2. Chia hai số cho nhau ra thời gian mỗi xe nằm trong vùng camera.
  //   2. Đổi kiểu chia pha của MỘT trục, chạy thử một cửa sổ, đo lại.
  //   3. Kiểu thử tốt hơn thì giữ, không thì trả về kiểu cũ; lần thử sau của trục đó lùi xa dần
  //      (2, 4, 8, 16 cửa sổ) để không tốn thời gian vào kiểu đã biết là kém.
  // Hai kiểu được thử là "riêng từng hướng" và "cùng đi thẳng trước". Kiểu "riêng từng hướng" chạy
  // ổn dù nhiều hay ít xe rẽ trái nên được ưu ái: muốn sang "thẳng trước" thì thời gian chờ phải
  // giảm ít nhất hoc_nguong %, còn muốn về "riêng" chỉ cần không tăng quá hoc_nguong %.
  // Điểm là thời gian chờ trung bình mỗi xe, tính bằng mili giây.
  void hocMoi() {
    hoc.kieu[0] = KIEU_RIENG; hoc.kieu[1] = KIEU_RIENG;
    hoc.dang_thu = false; hoc.truc_thu = 0; hoc.thu_ke = 0;
    hoc.cho[0] = 1; hoc.cho[1] = 2;
    hoc.lui[0] = HOC_LUI_DAU; hoc.lui[1] = HOC_LUI_DAU;
    hoc.diem_giu = -1; hoc.diem_thu = -1;
    hocCuaSoMoi();
  }

  bool hocDangBat() const {
    return ts->che_do == CHE_DO_THICH_UNG && ts->kieu_pha == KIEU_TU_HOC && ts->hoc_so_chu_ki > 0;
  }

  // Cộng dồn hàng chờ từ lần gọi trước tới lúc t. Cửa sổ có xe ưu tiên hoặc thiếu camera thì bỏ.
  void hocTichLuy(uint32_t t) {
    uint32_t dt = t - t_tich;
    t_tich = t;
    if (!hocDangBat()) { hoc.hong = true; return; }
    if ((int32_t)dt <= 0) return;
    if (ut_huong) hoc.hong = true;
    uint32_t q = 0;
    for (int h = 1; h <= 4; h++) {
      if (!conHan(h, t)) hoc.hong = true;
      else q += (uint32_t)dem[h].s + dem[h].l;
    }
    hoc.tich += (uint64_t)q * dt; hoc.tg += dt;
  }

  void hocCuaSoMoi() {
    hoc.tich = 0; hoc.tg = 0; hoc.luot = 0; hoc.so_ck = 0; hoc.hong = false;
  }

  // Gọi mỗi lần sắp chốt kế hoạch mới.
  void hocCuoiChuKi() {
    Hoc &hc = hoc;
    int truc;
    if (!hocDangBat()) return;
    if (hc.hong || hc.tg == 0) {             // cửa sổ hỏng: bỏ, đang thử thì quay về kiểu đang giữ
      if (hc.dang_thu) { hc.dang_thu = false; baoSuKien(SK_HOC_HUY, hc.truc_thu); }
      hocCuaSoMoi();
      return;
    }
    hc.so_ck++;
    uint32_t n = ts->hoc_so_chu_ki, so_xe = ts->hoc_so_xe > 0 ? ts->hoc_so_xe : 1;
    int64_t ng = (int64_t)ts->hoc_nguong;
    bool du = hc.so_ck >= 2 * n && hc.luot >= 2 * so_xe;   // một vòng = 2 lượt; một xe = 2 lượt vào/ra
    // Thời gian chờ trung bình mỗi xe (ms) = 2 × xe·ms / số lượt vào ra. Khi mới có ít xe qua thì chia
    // cho so_xe, tức là tính như đã có ít nhất nửa số xe cần có: điểm lúc đó chỉ có thể thấp hơn thực tế.
    int64_t diem = (int64_t)(hc.tich * 2 / (hc.luot > so_xe ? hc.luot : so_xe));
    if (hc.dang_thu) {
      truc = hc.truc_thu;
      uint8_t moi = kieuNguoc(hc.kieu[truc]);
      // Dừng thử sớm, không chờ đủ cửa sổ, khi kiểu thử đang kém rõ: chờ lâu hơn kiểu đang giữ quá
      // hoc_nguong % (thử "thẳng trước") hoặc quá 2 lần mức đó (thử "riêng"). Nhờ vậy một lần thử
      // hỏng chỉ tốn chừng một vòng.
      bool kem = diem * 100 > hc.diem_giu * (moi == KIEU_THANG_TRUOC ? 100 + ng : 100 + 2 * ng);
      if (!du && (!kem || hc.so_ck % 2 != 0)) return;   // chỉ xét dừng sớm khi vừa xong trọn một vòng
      bool nhan = !kem && diem * 100 <= hc.diem_giu * (moi == KIEU_THANG_TRUOC ? 100 - ng : 100 + ng);
      hc.diem_thu = diem;
      hc.dang_thu = false;
      if (nhan) {
        hc.kieu[truc] = moi; hc.diem_giu = diem; hc.lui[truc] = HOC_LUI_DAU;
      } else {
        hc.lui[truc] = (uint8_t)(hc.lui[truc] * 2 > HOC_LUI_TOI_DA ? HOC_LUI_TOI_DA : hc.lui[truc] * 2);
      }
      hc.cho[truc] = hc.lui[truc];
      hocCuaSoMoi();
      baoSuKien(nhan ? SK_HOC_NHAN : SK_HOC_TRA, (uint8_t)(truc * 4 + moi));
      return;
    }
    if (!du) return;
    // Sắp tới lượt thử trục nào? Lần thử phải bắt đầu đúng vào lượt của trục đó, nên nếu lượt sắp
    // chốt là của trục kia thì đo thêm một lượt nữa rồi mới đóng cửa sổ.
    truc = -1;
    if (hc.cho[hc.thu_ke] <= 1) truc = hc.thu_ke;
    else if (hc.cho[1 - hc.thu_ke] <= 1) truc = 1 - hc.thu_ke;
    if (truc >= 0 && truc_ke != truc) return;
    // xong một cửa sổ đo kiểu đang giữ: điểm mới lấy trung bình với điểm cũ cho đỡ dao động
    hc.diem_giu = hc.diem_giu < 0 ? diem : (hc.diem_giu + diem) / 2;
    hocCuaSoMoi();
    for (int i = 0; i < 2; i++) if (hc.cho[i] > 0) hc.cho[i]--;
    if (truc < 0) return;
    hc.dang_thu = true; hc.truc_thu = (uint8_t)truc; hc.thu_ke = (uint8_t)(1 - truc);
    baoSuKien(SK_HOC_THU, (uint8_t)(truc * 4 + kieuNguoc(hc.kieu[truc])));
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
