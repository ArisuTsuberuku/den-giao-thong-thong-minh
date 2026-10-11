// dieu_khien.js — Máy trạng thái chạy kế hoạch đèn: xanh → vàng → đỏ toàn phần → bước kế.
// Bản C++ tương ứng: firmware/trung_tam/dieu_khien.h (hai bản phải cho cùng vết đèn).
// Nguyên tắc: kế hoạch chỉ đổi ở ranh giới chu kì; ngoại lệ duy nhất là xe ưu tiên,
// và khi đó vẫn đi đủ vàng rồi đỏ toàn phần.
(function (goc, tao) {
  if (typeof module === 'object' && module.exports) module.exports = tao(require('./quyet_dinh.js'));
  else goc.DieuKhien = tao(goc.QuyetDinh);
})(typeof self !== 'undefined' ? self : this, function (QD) {
  'use strict';

  var PHA_KHOI_DONG = 0, PHA_XANH = 1, PHA_VANG = 2, PHA_DO_DON = 3;
  var MAU_DO = 0, MAU_VANG = 1, MAU_XANH = 2;

  function DieuKhien(ts) {
    this.ts = ts;                 // tham số (xem QuyetDinh.thamSoMacDinh)
    this.dem = [];                // số đếm mới nhất của từng hướng, chỉ số 1..4
    for (var h = 0; h <= 4; h++) {
      this.dem.push({ s: 0, l: 0, emg: 0, t_nhan: 0, co: false, giu: false, emg_tu: 0 });
    }
    this.ut_khoa = [false, false, false, false, false];
    this.pha = PHA_KHOI_DONG;
    this.xanh = 0;                // mặt nạ các nhóm đang xanh
    this.vang = 0;                // mặt nạ các nhóm đang vàng
    this.dich = 0;                // tổ hợp sẽ xanh sau khi chuyển xong
    this.dich_xanh = 0;           // thời gian xanh của tổ hợp đó
    this.t_het = 0;               // thời điểm hết trạng thái hiện tại
    this.t_xanh_tu = 0;           // thời điểm bước hiện tại bắt đầu xanh
    this.kh = [];                 // kế hoạch của chu kì đang chạy
    this.i_buoc = -1;
    this.chu_ki = 0;
    this.ut_huong = 0;            // đang ưu tiên hướng nào (0: không)
    this.ut_bat_dau = 0;
    this.ut_thay = 0;             // lần cuối còn thấy xe ưu tiên
    this.loi = false;
    this.khiSuKien = null;        // hàm(loại, dữ liệu) để ghi nhật ký; không bắt buộc
  }

  DieuKhien.prototype.khoiDong = function (t) {
    this.pha = PHA_KHOI_DONG;     // bật máy: đỏ toàn phần một lúc rồi mới chạy
    this.xanh = 0; this.vang = 0; this.loi = false;
    this.kh = []; this.i_buoc = -1; this.chu_ki = 0; this.ut_huong = 0;
    this.t_het = t + this.ts.t_do;
  };

  // Nhận một bản tin đếm xe. giu = true: tin gõ tay qua Serial, không tính hết hạn.
  DieuKhien.prototype.nhanDem = function (id, s, l, emg, t, giu) {
    if (!(id >= 1 && id <= 4)) return false;
    var d = this.dem[id];
    if (s < 0) s = 0; if (s > 99) s = 99;
    if (l < 0) l = 0; if (l > 99) l = 99;
    if (emg < 0 || emg > 2) emg = 0;
    if (emg !== 0 && (!d.co || d.emg === 0)) d.emg_tu = t;
    if (emg === 0) this.ut_khoa[id] = false;
    d.s = s; d.l = l; d.emg = emg; d.t_nhan = t; d.co = true; d.giu = !!giu;
    return true;
  };

  DieuKhien.prototype.conHan = function (h, t) {
    var d = this.dem[h];
    return d.co && (d.giu || t - d.t_nhan <= this.ts.mat_tin_hieu);
  };

  DieuKhien.prototype.thayXeUuTien = function (h, t) {
    return this.conHan(h, t) && this.dem[h].emg !== 0;
  };

  DieuKhien.prototype.huongCanUuTien = function (t) {
    var chon = 0;
    for (var h = 1; h <= 4; h++) {
      if (!this.thayXeUuTien(h, t) || this.ut_khoa[h]) continue;
      if (chon === 0 || this.dem[h].emg_tu < this.dem[chon].emg_tu) chon = h;
    }
    return chon;
  };

  DieuKhien.prototype.baoSuKien = function (loai, dl) {
    if (this.khiSuKien) this.khiSuKien(loai, dl);
  };

  DieuKhien.prototype.baoLoi = function () {
    this.loi = true; this.xanh = 0; this.vang = 0;
    this.baoSuKien('loi', null);
  };

  DieuKhien.prototype.vaoXanh = function (t) {
    if (!QD.hopLeMatNa(this.dich)) { this.baoLoi(); return; }
    this.xanh = this.dich; this.vang = 0;
    this.pha = PHA_XANH;
    this.t_xanh_tu = t;
    this.t_het = t + this.dich_xanh;
    if (this.ut_huong) this.ut_thay = t;   // xanh ưu tiên luôn giữ ít nhất ut_giu
    this.baoSuKien('buoc', null);
  };

  // Chuyển sang tổ hợp mới: nhóm có mặt ở cả hai thì giữ xanh, nhóm rời đi qua vàng rồi đỏ.
  DieuKhien.prototype.chuyenSang = function (dich, xanh_ms, t) {
    this.dich = dich; this.dich_xanh = xanh_ms;
    var roi = this.xanh & ~dich;
    if (roi === 0) { this.vaoXanh(t); return; }
    this.xanh &= dich; this.vang = roi;
    this.pha = PHA_VANG;
    this.t_het = t + this.ts.t_vang;
  };

  // Chốt kế hoạch cho chu kì mới theo số đếm lúc này.
  DieuKhien.prototype.chuKiMoi = function (t) {
    var s = [0, 0, 0, 0, 0], l = [0, 0, 0, 0, 0], hop_le = [false, false, false, false, false];
    for (var h = 1; h <= 4; h++) {
      s[h] = this.dem[h].s; l[h] = this.dem[h].l; hop_le[h] = this.conHan(h, t);
    }
    this.kh = QD.lapKeHoach(this.ts, s, l, hop_le);
    this.chu_ki++;
    this.i_buoc = 0;
    this.baoSuKien('chu_ki', null);
    this.chuyenSang(this.kh[0].mat_na, this.kh[0].xanh, t);
  };

  DieuKhien.prototype.motBuoc = function (t) {
    switch (this.pha) {
      case PHA_KHOI_DONG:
        if (t < this.t_het) return false;
        this.chuKiMoi(this.t_het);
        return true;
      case PHA_XANH:
        if (this.ut_huong) {                       // đang giữ xanh cho xe ưu tiên
          if (this.thayXeUuTien(this.ut_huong, t)) this.ut_thay = t;
          var qua_lau = t - this.ut_bat_dau >= this.ts.ut_toi_da;
          if (!qua_lau && t - this.ut_thay < this.ts.ut_giu) return false;
          if (qua_lau) this.ut_khoa[this.ut_huong] = true;
          this.baoSuKien('het_uu_tien', this.ut_huong);
          this.ut_huong = 0;
          this.chuKiMoi(t);
          return true;
        }
        if (t - this.t_xanh_tu >= this.ts.ut_xanh_an_toan) {
          var h = this.huongCanUuTien(t);
          if (h) {
            this.ut_huong = h; this.ut_bat_dau = t; this.ut_thay = t;
            this.i_buoc = -1;
            this.baoSuKien('uu_tien', h);
            this.chuyenSang(QD.matNaRieng(h), 0, t);
            return true;
          }
        }
        if (t < this.t_het) return false;
        if (this.i_buoc + 1 < this.kh.length) {
          this.i_buoc++;
          this.chuyenSang(this.kh[this.i_buoc].mat_na, this.kh[this.i_buoc].xanh, this.t_het);
        } else {
          this.chuKiMoi(this.t_het);
        }
        return true;
      case PHA_VANG:
        if (t < this.t_het) return false;
        this.vang = 0;
        this.pha = PHA_DO_DON;
        this.t_het += this.ts.t_do;
        return true;
      case PHA_DO_DON:
        if (t < this.t_het) return false;
        this.vaoXanh(this.t_het);
        return true;
    }
    return false;
  };

  // Gọi thường xuyên với thời gian hiện tại (ms).
  DieuKhien.prototype.capNhat = function (t) {
    for (var i = 0; i < 16 && !this.loi; i++) {
      if (!this.motBuoc(t)) break;
    }
  };

  // Màu của nhóm g (0..7) lúc t. Khi có lỗi an toàn: mọi nhóm nháy vàng.
  DieuKhien.prototype.mauNhom = function (g, t) {
    if (this.loi) return (Math.floor(t / 500) % 2 === 0) ? MAU_VANG : MAU_DO;
    if (this.xanh & (1 << g)) return MAU_XANH;
    if (this.vang & (1 << g)) return MAU_VANG;
    return MAU_DO;
  };

  DieuKhien.prototype.conLai = function (t) {
    if (this.pha === PHA_XANH && this.ut_huong) return -1;
    return this.t_het > t ? this.t_het - t : 0;
  };

  DieuKhien.PHA_KHOI_DONG = PHA_KHOI_DONG; DieuKhien.PHA_XANH = PHA_XANH;
  DieuKhien.PHA_VANG = PHA_VANG; DieuKhien.PHA_DO_DON = PHA_DO_DON;
  DieuKhien.MAU_DO = MAU_DO; DieuKhien.MAU_VANG = MAU_VANG; DieuKhien.MAU_XANH = MAU_XANH;
  return DieuKhien;
});
