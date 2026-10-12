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
    // tự học: số đếm của một làn giảm 1 là một xe vừa qua vạch dừng; cộng cho trục của hướng đó
    if (d.co) this.hoc.ra[(id - 1) & 1] += (d.s > s ? d.s - s : 0) + (d.l > l ? d.l - l : 0);
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
    var kieu = [this.hoc.kieu[0], this.hoc.kieu[1]];
    for (var i = 0; i < 2; i++) if (this.hocSapThu(i)) kieu[i] = kieuNguoc(kieu[i]);
    return kieu;
  };

  // Chốt kế hoạch mới theo số đếm lúc này. Số thứ tự chu_ki đếm số lần chốt (thích ứng: số lượt).
  DieuKhien.prototype.chuKiMoi = function (t) {
    this.hocCuoiLuot(t);
    var s = [0, 0, 0, 0, 0], l = [0, 0, 0, 0, 0], hop_le = [false, false, false, false, false];
    for (var h = 1; h <= 4; h++) {
      s[h] = this.dem[h].s; l[h] = this.dem[h].l; hop_le[h] = this.conHan(h, t);
    }
    var kieu = this.kieuChoChuKi();
    this.kh = QD.lapKeHoach(this.ts, s, l, hop_le, kieu, this.truc_ke);
    this.che_do_kh = this.ts.che_do;
    if (this.che_do_kh === QD.CHE_DO_THICH_UNG) {
      var truc = QD.trucCuaMatNa(this.kh[0].mat_na);
      this.truc_ke = 1 - truc;
      this.hocDauLuot(truc, kieu[truc], s, l, hop_le);
    }
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
    this.hocCuoiBuoc(t);
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
    this.hoc.l_hong = true;       // lượt bị xe ưu tiên cắt ngang: tự học không tính lượt này
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
    this.hocTheoDoi(t);
    for (var i = 0; i < 16 && !this.loi; i++) {
      if (!this.motBuoc(t)) break;
    }
  };

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
  DieuKhien.prototype.hocMoi = function () {
    var cach = this.ts.hoc_cach_thu > 0 ? this.ts.hoc_cach_thu : 1;
    return {
      kieu: [QD.KIEU_RIENG, QD.KIEU_RIENG],   // kiểu đang giữ của trục dọc, trục ngang
      b: [-1, -1],                            // mốc của kiểu riêng (ms xanh mỗi xe): [lượt xả hết, lượt quá tải]; -1 = chưa đo
      q: [-1, -1],                            // điểm của "thẳng trước" so với "riêng" ở từng trục (%); -1 = chưa đo
      dem: [cach > 1 ? cach - 1 : 0, cach > 2 ? cach - 2 : 0],   // số lượt đã đo kể từ lần thử gần nhất (hai trục lệch nhau một lượt)
      cach: [cach, cach],                     // cách mấy lượt thì thử "thẳng trước" một lần
      lien: [0, 0],                           // số lần thử liên tiếp đạt
      tuoi: [0, 0],                           // số lượt "thẳng trước" kể từ lần gần nhất có lượt "riêng" để lấy mốc
      // lượt đang chạy
      l_truc: -1, l_kieu: 0,                  // trục và kiểu của lượt (l_truc = -1: không theo dõi)
      l_do: false,                            // lượt đo được: cả hai hướng có xe và làn trong có xe, tức kế hoạch có phụ thuộc kiểu
      l_thu: false,                           // lượt chạy kiểu khác kiểu đang giữ
      l_hong: false,                          // có xe ưu tiên hoặc thiếu camera
      l_qt: false,                            // quá tải: có bước hết giờ mà vẫn còn xe
      l_g: 0, l_n: 0,                         // tổng thời gian xanh tới lúc hết xe (ms), số bước đã chạy
      ra: [0, 0],                             // số xe đã qua vạch dừng của trục dọc, trục ngang trong lượt
      b_trong: false, b_trong_tu: 0           // bước đang xanh hiện không còn xe, kể từ lúc nào
    };
  };

  DieuKhien.prototype.hocDangBat = function () {
    return this.ts.che_do === QD.CHE_DO_THICH_UNG && this.ts.kieu_pha === QD.KIEU_TU_HOC && this.ts.hoc_cach_thu > 0;
  };

  // Lượt sắp tới của trục k có chạy kiểu khác kiểu đang giữ không (thử "thẳng trước", hoặc làm mới mốc "riêng").
  DieuKhien.prototype.hocSapThu = function (k) {
    var hc = this.hoc, cach = this.ts.hoc_cach_thu;
    if (!this.hocDangBat()) return false;
    if (hc.kieu[k] === QD.KIEU_RIENG) return hc.dem[k] >= hc.cach[k] && (hc.b[0] >= 0 || hc.b[1] >= 0);
    return hc.tuoi[k] >= (hc.q[k] >= 0 && hc.q[k] < 100 - 2 * this.ts.hoc_nguong ? 4 * cach : cach);
  };

  // Gọi mỗi nhịp, trước khi xét chuyển đèn: đánh dấu lượt hỏng, và ghi lúc bước đang xanh hết xe.
  DieuKhien.prototype.hocTheoDoi = function (t) {
    var hc = this.hoc;
    if (hc.l_truc < 0) return;
    if (!this.hocDangBat() || this.ut_huong) { hc.l_hong = true; return; }
    for (var h = 1; h <= 4; h++) if (!this.conHan(h, t)) hc.l_hong = true;
    if (this.pha === PHA_XANH && this.i_buoc >= 0) {
      if (this.nhuCauBuoc(this.i_buoc, t) === 0) {
        if (!hc.b_trong) { hc.b_trong = true; hc.b_trong_tu = t; }
      } else hc.b_trong = false;
    }
  };

  // Bắt đầu theo dõi một lượt của trục truc, chạy kiểu kieu.
  DieuKhien.prototype.hocDauLuot = function (truc, kieu, s, l, hop_le) {
    var hc = this.hoc, a = truc ? 2 : 1, b = a + 2;
    hc.l_truc = truc; hc.l_kieu = kieu;
    hc.l_hong = false; hc.l_qt = false; hc.l_g = 0; hc.l_n = 0; hc.ra[0] = 0; hc.ra[1] = 0; hc.b_trong = false;
    hc.l_do = this.hocDangBat() && hop_le[a] && hop_le[b] && s[a] + l[a] > 0 && s[b] + l[b] > 0 && (l[a] > 0 || l[b] > 0);
    hc.l_thu = hc.l_do && kieu !== hc.kieu[truc];
    if (hc.l_thu) this.baoSuKien('hoc_thu', truc * 4 + kieu);
  };

  // Hết một bước (do cắt sớm hoặc hết giờ): cộng thời gian xanh của bước, tính tới lúc hết xe.
  DieuKhien.prototype.hocCuoiBuoc = function (t) {
    var hc = this.hoc;
    if (hc.l_truc >= 0 && this.i_buoc >= 0) {
      var trong = hc.b_trong && hc.b_trong_tu >= this.t_xanh_tu;
      if (!trong) hc.l_qt = true;
      hc.l_g += (trong ? hc.b_trong_tu : t) - this.t_xanh_tu;
      hc.l_n++;
    }
    hc.b_trong = false;
  };

  DieuKhien.prototype.hocDoiKieu = function (k, kieu) {
    var hc = this.hoc;
    hc.kieu[k] = kieu; hc.lien[k] = 0; hc.dem[k] = 0; hc.tuoi[k] = 0;
    hc.cach[k] = this.ts.hoc_cach_thu;
    this.baoSuKien('hoc_nhan', k * 4 + kieu);
  };

  // Gọi mỗi lần sắp chốt kế hoạch mới: chấm lượt vừa chạy xong.
  DieuKhien.prototype.hocCuoiLuot = function (t) {
    var hc = this.hoc, ts = this.ts;
    if (hc.l_truc >= 0 && hc.l_do) {
      var k = hc.l_truc, ra = hc.ra[k], ng = ts.hoc_nguong, cach = ts.hoc_cach_thu;
      if (!this.hocDangBat() || hc.l_hong) {
        if (hc.l_thu) {
          // lượt thử bị hỏng (xe ưu tiên, mất camera): không thử lại ngay lượt sau mà chờ đủ một khoảng
          if (hc.kieu[k] === QD.KIEU_RIENG) { hc.dem[k] = 0; hc.lien[k] = 0; }
          this.baoSuKien('hoc_huy', k);
        }
      } else if (ra > 0 && ra >= ts.hoc_so_xe && hc.l_n > 0) {
        var c = ts.t_vang + ts.t_do + ts.cat_som, i = hc.l_qt ? 1 : 0;
        if (hc.l_kieu === QD.KIEU_RIENG) {
          var bx = chiaNguyen(hc.l_g, ra);
          hc.b[i] = hc.b[i] < 0 ? bx : chiaNguyen(hc.b[i] + bx, 2);
          hc.tuoi[0] = 0; hc.tuoi[1] = 0;
          if (hc.kieu[k] === QD.KIEU_RIENG) hc.dem[k]++;
        } else {
          var mau = hc.b[i] < 0 ? 0 : 2 * c + hc.b[i] * ra;   // thời gian kiểu riêng cần cho cùng số xe
          hc.tuoi[k]++;
          if (mau <= 0) {
            // Chưa có mốc "riêng" cùng loại để so nên không chấm được lượt này. Đang giữ "thẳng trước" thì
            // lượt sau chạy "riêng" để lấy mốc; đang giữ "riêng" thì bỏ lần thử, chạy "riêng" tiếp để có mốc.
            if (hc.kieu[k] !== QD.KIEU_RIENG) hc.tuoi[k] = 4 * cach;
            else { hc.dem[k] = 0; hc.lien[k] = 0; }
          } else {
            var q = chiaNguyen((hc.l_g + hc.l_n * c) * 100, mau);
            if (hc.kieu[k] !== QD.KIEU_RIENG) {            // đang giữ "thẳng trước"
              hc.q[k] = hc.q[k] < 0 ? q : chiaNguyen(hc.q[k] * 3 + q, 4);
              if (hc.l_qt ? hc.q[k] >= 100 : hc.q[k] * 2 >= 200 + ng) this.hocDoiKieu(k, QD.KIEU_RIENG);
            } else {                                       // lượt thử khi đang giữ "riêng"
              hc.q[k] = hc.lien[k] === 0 ? q : chiaNguyen(hc.q[k] + q, 2);
              if (hc.q[k] <= (hc.l_qt ? 100 - 2 * ng : 100 - ng)) {
                hc.lien[k]++;
                if (hc.lien[k] >= 2) this.hocDoiKieu(k, QD.KIEU_THANG_TRUOC);
                else hc.dem[k] = hc.cach[k];               // đạt lần đầu: lượt sau của trục này thử tiếp
              } else {
                hc.lien[k] = 0; hc.dem[k] = 0;
                if (hc.l_qt || q >= 100 + ng) {
                  var tran = (hc.l_qt ? 4 : 2) * cach;
                  hc.cach[k] = hc.cach[k] * 2 > tran ? tran : hc.cach[k] * 2;
                } else hc.cach[k] = cach;
                this.baoSuKien('hoc_tra', k * 4 + QD.KIEU_THANG_TRUOC);
              }
            }
          }
        }
      }
    }
    hc.l_truc = -1; hc.l_do = false; hc.l_thu = false;
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
