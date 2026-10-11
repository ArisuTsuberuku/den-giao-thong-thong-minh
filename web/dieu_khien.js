// dieu_khien.js — Máy trạng thái chạy kế hoạch đèn: xanh → vàng → đỏ toàn phần → bước kế.
// Bản C++ tương ứng: firmware/trung_tam/dieu_khien.h (hai bản phải cho cùng vết đèn).
// Nguyên tắc: tổ hợp và thứ tự các bước chốt ở đầu mỗi lượt (chế độ thích ứng: lượt của một trục;
// chế độ cố định: cả chu kì). Hai ngoại lệ, đều vẫn đi đủ vàng rồi đỏ toàn phần: xe ưu tiên, và
// cắt xanh sớm khi tổ hợp đang xanh hết xe (chế độ thích ứng).
// Ngoài ra bộ điều khiển còn tự học kiểu chia pha: xem phần "Tự học" ở dưới.
(function (goc, tao) {
  if (typeof module === 'object' && module.exports) module.exports = tao(require('./quyet_dinh.js'));
  else goc.DieuKhien = tao(goc.QuyetDinh);
})(typeof self !== 'undefined' ? self : this, function (QD) {
  'use strict';

  var PHA_KHOI_DONG = 0, PHA_XANH = 1, PHA_VANG = 2, PHA_DO_DON = 3;
  var MAU_DO = 0, MAU_VANG = 1, MAU_XANH = 2;
  var LUI_DAU = 2, LUI_TOI_DA = 16;     // tự học: số cửa sổ đo phải chờ giữa hai lần thử cùng một trục

  // Hai kiểu chia pha mà bộ tự học chọn giữa: "riêng từng hướng" và "cùng đi thẳng trước".
  function kieuNguoc(k) { return k === QD.KIEU_RIENG ? QD.KIEU_THANG_TRUOC : QD.KIEU_RIENG; }

  // Chia lấy phần nguyên, cho kết quả đúng như phép chia số nguyên của C++.
  function chiaNguyen(a, b) {
    var q = Math.floor(a / b);
    if (q * b > a) q--;
    else if ((q + 1) * b <= a) q++;
    return q;
  }

  function DieuKhien(ts) {
    this.ts = ts;                 // tham số (xem QuyetDinh.thamSoMacDinh)
    this.dem = [];                // số đếm mới nhất của từng hướng, chỉ số 1..4
    for (var h = 0; h <= 4; h++) {
      // ut: loại xe ưu tiên đang thấy (0 không có, 1 cứu hỏa, 2 quân sự, 3 công an, 4 cứu thương)
      this.dem.push({ s: 0, l: 0, emg: 0, ut: 0, t_nhan: 0, co: false, giu: false, emg_tu: 0 });
    }
    this.ut_khoa = [false, false, false, false, false];
    this.pha = PHA_KHOI_DONG;
    this.xanh = 0;                // mặt nạ các nhóm đang xanh
    this.vang = 0;                // mặt nạ các nhóm đang vàng
    this.dich = 0;                // tổ hợp sẽ xanh sau khi chuyển xong
    this.dich_xanh = 0;           // thời gian xanh của tổ hợp đó
    this.t_het = 0;               // thời điểm hết trạng thái hiện tại
    this.t_xanh_tu = 0;           // thời điểm bước hiện tại bắt đầu xanh
    this.kh = [];                 // kế hoạch đang chạy (thích ứng: của một lượt; cố định: của cả chu kì)
    this.che_do_kh = QD.CHE_DO_CO_DINH;   // chế độ lúc chốt kế hoạch đó
    this.i_buoc = -1;
    this.chu_ki = 0;
    this.dang_trong = false;      // tổ hợp đang xanh hiện không còn xe
    this.t_trong_tu = 0;          // ... kể từ lúc này
    this.ut_huong = 0;            // đang ưu tiên hướng nào (0: không)
    this.ut_bat_dau = 0;
    this.ut_thay = 0;             // lần cuối còn thấy xe ưu tiên
    this.truc_ke = 0;             // lượt kế là của trục nào (0 dọc, 1 ngang)
    this.t_tich = 0;              // tự học: đã cộng dồn hàng chờ tới thời điểm này
    this.hoc = this.hocMoi();
    this.loi = false;
    this.khiSuKien = null;        // hàm(loại, dữ liệu) để ghi nhật ký; không bắt buộc
  }

  DieuKhien.prototype.khoiDong = function (t) {
    this.pha = PHA_KHOI_DONG;     // bật máy: đỏ toàn phần một lúc rồi mới chạy
    this.xanh = 0; this.vang = 0; this.loi = false;
    this.kh = []; this.i_buoc = -1; this.chu_ki = 0; this.ut_huong = 0;
    this.dang_trong = false;
    this.truc_ke = 0;
    this.t_tich = t;
    this.hoc = this.hocMoi();
    this.t_het = t + this.ts.t_do;
  };

  // Nhận một bản tin đếm xe. giu = true: tin gõ tay qua Serial, không tính hết hạn.
  // ut: loại xe ưu tiên (1..4); bản tin cũ chỉ có emg thì coi là xe cứu thương.
  DieuKhien.prototype.nhanDem = function (id, s, l, emg, t, giu, ut) {
    if (!(id >= 1 && id <= 4)) return false;
    var d = this.dem[id];
    if (s < 0) s = 0; if (s > 99) s = 99;
    if (l < 0) l = 0; if (l > 99) l = 99;
    if (emg < 0 || emg > 2) emg = 0;
    if (!(ut >= 1 && ut <= 4)) ut = 0;
    if (emg !== 0 && ut === 0) ut = 4;
    if (ut !== 0 && emg === 0) emg = 1;
    if (ut !== 0 && (!d.co || d.ut === 0)) d.emg_tu = t;
    if (ut === 0) this.ut_khoa[id] = false;
    // tự học: mỗi lần số đếm của một làn tăng hay giảm 1 là một lượt xe vào hoặc ra vùng camera
    if (d.co) this.hoc.luot += (s > d.s ? s - d.s : d.s - s) + (l > d.l ? l - d.l : d.l - l);
    d.s = s; d.l = l; d.emg = emg; d.ut = ut; d.t_nhan = t; d.co = true; d.giu = !!giu;
    return true;
  };

  DieuKhien.prototype.conHan = function (h, t) {
    var d = this.dem[h];
    return d.co && (d.giu || t - d.t_nhan <= this.ts.mat_tin_hieu);
  };

  DieuKhien.prototype.thayXeUuTien = function (h, t) {
    return this.conHan(h, t) && this.dem[h].ut !== 0;
  };

  // Hướng có xe ưu tiên cần được mở đèn. Nhiều hướng cùng có xe ưu tiên thì theo thứ tự
  // hỏa → sự → công → thương (số ut nhỏ hơn đi trước); cùng hạng thì xe thấy trước đi trước.
  DieuKhien.prototype.huongCanUuTien = function (t, bo_qua) {
    var chon = 0;
    for (var h = 1; h <= 4; h++) {
      if (h === bo_qua || !this.thayXeUuTien(h, t) || this.ut_khoa[h]) continue;
      var d = this.dem[h], c = this.dem[chon];
      if (chon === 0 || d.ut < c.ut || (d.ut === c.ut && d.emg_tu < c.emg_tu)) chon = h;
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
    this.dang_trong = false;
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

  // Kiểu chia pha của [trục dọc, trục ngang] cho lượt sắp chốt.
  DieuKhien.prototype.kieuChoChuKi = function () {
    var k = this.ts.kieu_pha;
    if (k !== QD.KIEU_TU_HOC) {
      if (k !== QD.KIEU_THANG_TRUOC && k !== QD.KIEU_DOI_XUNG) k = QD.KIEU_RIENG;
      return [k, k];
    }
    var hc = this.hoc, kieu = [hc.kieu[0], hc.kieu[1]];
    if (hc.dang_thu) kieu[hc.truc_thu] = kieuNguoc(kieu[hc.truc_thu]);
    return kieu;
  };

  // Chốt kế hoạch mới theo số đếm lúc này. Số thứ tự chu_ki đếm số lần chốt (thích ứng: số lượt).
  DieuKhien.prototype.chuKiMoi = function (t) {
    this.hocCuoiChuKi();
    var s = [0, 0, 0, 0, 0], l = [0, 0, 0, 0, 0], hop_le = [false, false, false, false, false];
    for (var h = 1; h <= 4; h++) {
      s[h] = this.dem[h].s; l[h] = this.dem[h].l; hop_le[h] = this.conHan(h, t);
    }
    this.kh = QD.lapKeHoach(this.ts, s, l, hop_le, this.kieuChoChuKi(), this.truc_ke);
    this.che_do_kh = this.ts.che_do;
    if (this.che_do_kh === QD.CHE_DO_THICH_UNG) this.truc_ke = 1 - QD.trucCuaMatNa(this.kh[0].mat_na);
    this.chu_ki++;
    this.i_buoc = 0;
    this.baoSuKien('chu_ki', null);
    this.chuyenSang(this.kh[0].mat_na, this.kh[0].xanh, t);
  };

  // Số xe mà tổ hợp m phục vụ: nhóm thẳng phục vụ cả hai làn, nhóm trái chỉ phục vụ làn trong.
  // Trả về -1 nếu camera của một hướng trong tổ hợp mất tín hiệu (không biết thì không cắt, không bỏ).
  DieuKhien.prototype.nhuCau = function (m, t) {
    var n = 0;
    for (var h = 1; h <= 4; h++) {
      var thang = (m & QD.bitThang(h)) !== 0, trai = (m & QD.bitTrai(h)) !== 0;
      if (!thang && !trai) continue;
      if (!this.conHan(h, t)) return -1;
      n += thang ? this.dem[h].s + this.dem[h].l : this.dem[h].l;
    }
    return n;
  };

  // Có xe đang chờ mà tổ hợp m không phục vụ không? Camera mất tín hiệu thì coi như có.
  DieuKhien.prototype.coXeChoNgoai = function (m, t) {
    for (var h = 1; h <= 4; h++) {
      if (m & QD.bitThang(h)) continue;
      if (!this.conHan(h, t)) return true;
      if (m & QD.bitTrai(h)) { if (this.dem[h].s > 0) return true; }
      else if (this.dem[h].s + this.dem[h].l > 0) return true;
    }
    return false;
  };

  // Như nhuCau và coXeChoNgoai nhưng cho bước thứ i của kế hoạch. Bước "cùng đi thẳng" có đánh dấu
  // ngoai chỉ tính xe ở làn ngoài; xe làn trong của chính hai hướng đó được coi là đang chờ bước sau.
  DieuKhien.prototype.nhuCauBuoc = function (i, t) {
    var b = this.kh[i];
    if (!b.ngoai) return this.nhuCau(b.mat_na, t);
    var n = 0;
    for (var h = 1; h <= 4; h++) {
      if (!(b.mat_na & QD.bitThang(h))) continue;
      if (!this.conHan(h, t)) return -1;
      n += this.dem[h].s;
    }
    return n;
  };

  DieuKhien.prototype.coXeChoBuoc = function (i, t) {
    var b = this.kh[i];
    if (b.ngoai) {
      for (var h = 1; h <= 4; h++) {
        if ((b.mat_na & QD.bitThang(h)) && this.conHan(h, t) && this.dem[h].l > 0) return true;
      }
    }
    return this.coXeChoNgoai(b.mat_na, t);
  };

  DieuKhien.prototype.duocCatSom = function () {
    return this.che_do_kh === QD.CHE_DO_THICH_UNG && this.ts.cat_som > 0;
  };

  // Sang bước kế của kế hoạch. Khi bật cắt sớm, bước nào không còn xe thì bỏ qua.
  DieuKhien.prototype.buocKe = function (t) {
    var i = this.i_buoc + 1;
    if (this.duocCatSom()) {
      while (i < this.kh.length && this.nhuCauBuoc(i, t) === 0) i++;
    }
    if (i < this.kh.length) {
      this.i_buoc = i;
      this.chuyenSang(this.kh[i].mat_na, this.kh[i].xanh, t);
    } else {
      this.chuKiMoi(t);
    }
  };

  // Mở riêng hướng h cho xe ưu tiên: các nhóm khác đang xanh đi qua vàng rồi đỏ toàn phần.
  DieuKhien.prototype.batDauUuTien = function (h, t) {
    this.ut_huong = h; this.ut_bat_dau = t; this.ut_thay = t;
    this.i_buoc = -1;
    this.baoSuKien('uu_tien', h);
    this.chuyenSang(QD.matNaRieng(h), 0, t);
  };

  // Hết ưu tiên cho hướng đang mở: còn xe ưu tiên ở hướng khác thì mở tiếp cho nó, không thì quay
  // lại chạy bình thường, bắt đầu từ trục đã phải chờ.
  DieuKhien.prototype.ketThucUuTien = function (t) {
    var cu = this.ut_huong;
    this.baoSuKien('het_uu_tien', cu);
    this.ut_huong = 0;
    var h = this.huongCanUuTien(t, 0);
    if (h) { this.batDauUuTien(h, t); return; }
    this.truc_ke = (cu === 1 || cu === 3) ? 1 : 0;
    this.chuKiMoi(t);
  };

  DieuKhien.prototype.motBuoc = function (t) {
    var h;
    // trong lúc đèn đang chuyển (vàng, đỏ toàn phần) vẫn ghi nhận lần cuối còn thấy xe ưu tiên
    if (this.ut_huong && this.pha !== PHA_XANH && this.thayXeUuTien(this.ut_huong, t)) this.ut_thay = t;
    switch (this.pha) {
      case PHA_KHOI_DONG:
        if (t < this.t_het) return false;
        this.chuKiMoi(this.t_het);
        return true;
      case PHA_XANH:
        if (this.ut_huong) {                       // đang giữ xanh cho xe ưu tiên
          var thay = this.thayXeUuTien(this.ut_huong, t);
          if (thay) this.ut_thay = t;
          if (t - this.t_xanh_tu >= this.ts.ut_xanh_an_toan) {
            // hướng khác có xe ưu tiên hạng cao hơn (hoặc xe của hướng này đã qua): nhường ngay
            h = this.huongCanUuTien(t, this.ut_huong);
            if (h && (!thay || this.dem[h].ut < this.dem[this.ut_huong].ut)) {
              this.baoSuKien('het_uu_tien', this.ut_huong);
              this.batDauUuTien(h, t);
              return true;
            }
          }
          // xe ưu tiên vừa qua (không còn thấy quá ut_giu) là chuyển vàng ngay, không giữ xanh thêm
          var qua_lau = t - this.ut_bat_dau >= this.ts.ut_toi_da;
          if (!qua_lau && (thay || t - this.ut_thay < this.ts.ut_giu)) return false;
          if (qua_lau) this.ut_khoa[this.ut_huong] = true;
          this.ketThucUuTien(t);
          return true;
        }
        if (t - this.t_xanh_tu >= this.ts.ut_xanh_an_toan) {
          h = this.huongCanUuTien(t, 0);
          if (h) { this.batDauUuTien(h, t); return true; }
        }
        if (this.duocCatSom() && this.i_buoc >= 0) {
          // Cắt xanh sớm: tổ hợp đang xanh không còn xe suốt cat_som, và có xe khác đang chờ
          if (this.nhuCauBuoc(this.i_buoc, t) === 0) {
            if (!this.dang_trong) { this.dang_trong = true; this.t_trong_tu = t; }
          } else this.dang_trong = false;
          if (this.dang_trong && t - this.t_trong_tu >= this.ts.cat_som && this.coXeChoBuoc(this.i_buoc, t)) {
            this.baoSuKien('cat_som', this.xanh);
            this.buocKe(t);
            return true;
          }
        }
        if (t < this.t_het) return false;
        this.buocKe(this.t_het);
        return true;
      case PHA_VANG:
        if (t < this.t_het) return false;
        this.vang = 0;
        this.pha = PHA_DO_DON;
        this.t_het += this.ts.t_do;
        return true;
      case PHA_DO_DON:
        if (t < this.t_het) return false;
        if (this.ut_huong && !this.thayXeUuTien(this.ut_huong, t) && t - this.ut_thay >= this.ts.ut_giu) {
          this.ketThucUuTien(t);     // xe ưu tiên đã đi mất trong lúc chuyển đèn: không mở riêng nữa
          return true;
        }
        this.vaoXanh(this.t_het);
        return true;
    }
    return false;
  };

  // Gọi thường xuyên với thời gian hiện tại (ms).
  DieuKhien.prototype.capNhat = function (t) {
    this.hocTichLuy(t);
    for (var i = 0; i < 16 && !this.loi; i++) {
      if (!this.motBuoc(t)) break;
    }
  };

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
  DieuKhien.prototype.hocMoi = function () {
    return {
      kieu: [QD.KIEU_RIENG, QD.KIEU_RIENG],   // kiểu đang giữ của trục dọc, trục ngang
      dang_thu: false, truc_thu: 0,           // đang chạy thử: trục truc_thu dùng kiểu ngược với kiểu đang giữ
      thu_ke: 0,                              // trục được ưu tiên thử ở lần tới
      cho: [1, 2],                            // còn mấy cửa sổ đo nữa thì tới lượt thử trục đó
      lui: [LUI_DAU, LUI_DAU],
      diem_giu: -1, diem_thu: -1,             // điểm của kiểu đang giữ và của lần thử gần nhất
      tich: 0, tg: 0, luot: 0, so_ck: 0, hong: false   // cửa sổ đang đo: xe·ms, ms, số lượt xe vào/ra, số lần chốt kế hoạch; hỏng = phải đo lại
    };
  };

  DieuKhien.prototype.hocDangBat = function () {
    return this.ts.che_do === QD.CHE_DO_THICH_UNG && this.ts.kieu_pha === QD.KIEU_TU_HOC && this.ts.hoc_so_chu_ki > 0;
  };

  // Cộng dồn hàng chờ từ lần gọi trước tới lúc t. Cửa sổ có xe ưu tiên hoặc thiếu camera thì bỏ.
  DieuKhien.prototype.hocTichLuy = function (t) {
    var dt = t - this.t_tich, hc = this.hoc;
    this.t_tich = t;
    if (!this.hocDangBat()) { hc.hong = true; return; }
    if (dt <= 0) return;
    if (this.ut_huong) hc.hong = true;
    var q = 0;
    for (var h = 1; h <= 4; h++) {
      if (!this.conHan(h, t)) hc.hong = true;
      else q += this.dem[h].s + this.dem[h].l;
    }
    hc.tich += q * dt; hc.tg += dt;
  };

  DieuKhien.prototype.hocCuaSoMoi = function () {
    var hc = this.hoc;
    hc.tich = 0; hc.tg = 0; hc.luot = 0; hc.so_ck = 0; hc.hong = false;
  };

  // Gọi mỗi lần sắp chốt kế hoạch mới.
  DieuKhien.prototype.hocCuoiChuKi = function () {
    var hc = this.hoc, n = this.ts.hoc_so_chu_ki, truc, moi;
    if (!this.hocDangBat()) return;
    if (hc.hong || hc.tg === 0) {              // cửa sổ hỏng: bỏ, đang thử thì quay về kiểu đang giữ
      if (hc.dang_thu) { hc.dang_thu = false; this.baoSuKien('hoc_huy', hc.truc_thu); }
      this.hocCuaSoMoi();
      return;
    }
    hc.so_ck++;
    var so_xe = this.ts.hoc_so_xe > 0 ? this.ts.hoc_so_xe : 1, ng = this.ts.hoc_nguong;
    var du = hc.so_ck >= 2 * n && hc.luot >= 2 * so_xe;   // một vòng = 2 lượt; một xe = 2 lượt vào/ra
    // Thời gian chờ trung bình mỗi xe (ms) = 2 × xe·ms / số lượt vào ra. Khi mới có ít xe qua thì chia
    // cho so_xe, tức là tính như đã có ít nhất nửa số xe cần có: điểm lúc đó chỉ có thể thấp hơn thực tế.
    var diem = chiaNguyen(hc.tich * 2, hc.luot > so_xe ? hc.luot : so_xe);
    if (hc.dang_thu) {
      truc = hc.truc_thu; moi = kieuNguoc(hc.kieu[truc]);
      // Dừng thử sớm, không chờ đủ cửa sổ, khi kiểu thử đang kém rõ: chờ lâu hơn kiểu đang giữ quá
      // hoc_nguong % (thử "thẳng trước") hoặc quá 2 lần mức đó (thử "riêng"). Nhờ vậy một lần thử
      // hỏng chỉ tốn chừng một vòng.
      var kem = diem * 100 > hc.diem_giu * (moi === QD.KIEU_THANG_TRUOC ? 100 + ng : 100 + 2 * ng);
      if (!du && (!kem || hc.so_ck % 2 !== 0)) return;   // chỉ xét dừng sớm khi vừa xong trọn một vòng
      var nhan = !kem && diem * 100 <= hc.diem_giu * (moi === QD.KIEU_THANG_TRUOC ? 100 - ng : 100 + ng);
      hc.diem_thu = diem;
      hc.dang_thu = false;
      if (nhan) {
        hc.kieu[truc] = moi; hc.diem_giu = diem; hc.lui[truc] = LUI_DAU;
      } else {
        hc.lui[truc] = hc.lui[truc] * 2 > LUI_TOI_DA ? LUI_TOI_DA : hc.lui[truc] * 2;
      }
      hc.cho[truc] = hc.lui[truc];
      this.hocCuaSoMoi();
      this.baoSuKien(nhan ? 'hoc_nhan' : 'hoc_tra', truc * 4 + moi);
      return;
    }
    if (!du) return;
    // Sắp tới lượt thử trục nào? Lần thử phải bắt đầu đúng vào lượt của trục đó, nên nếu lượt sắp
    // chốt là của trục kia thì đo thêm một lượt nữa rồi mới đóng cửa sổ.
    truc = -1;
    if (hc.cho[hc.thu_ke] <= 1) truc = hc.thu_ke;
    else if (hc.cho[1 - hc.thu_ke] <= 1) truc = 1 - hc.thu_ke;
    if (truc >= 0 && this.truc_ke !== truc) return;
    // xong một cửa sổ đo kiểu đang giữ: điểm mới lấy trung bình với điểm cũ cho đỡ dao động
    hc.diem_giu = hc.diem_giu < 0 ? diem : chiaNguyen(hc.diem_giu + diem, 2);
    this.hocCuaSoMoi();
    for (var i = 0; i < 2; i++) if (hc.cho[i] > 0) hc.cho[i]--;
    if (truc < 0) return;
    hc.dang_thu = true; hc.truc_thu = truc; hc.thu_ke = 1 - truc;
    this.baoSuKien('hoc_thu', truc * 4 + kieuNguoc(hc.kieu[truc]));
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
