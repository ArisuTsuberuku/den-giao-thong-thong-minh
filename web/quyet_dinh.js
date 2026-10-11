// quyet_dinh.js — Bộ lập kế hoạch đèn cho MỘT chu kì (mức 1: bộ luật).
// Bản JavaScript này và bản C++ ở firmware/trung_tam/quyet_dinh.h phải cho cùng kết quả;
// kiem_thu/so_khop.mjs chạy hai bản trên cùng dữ liệu để kiểm tra điều đó.
// Mọi thời gian tính bằng mili giây, số nguyên.
(function (goc, tao) {
  if (typeof module === 'object' && module.exports) module.exports = tao();
  else goc.QuyetDinh = tao();
})(typeof self !== 'undefined' ? self : this, function () {
  'use strict';

  // ----- 8 nhóm tín hiệu: bit = (hướng-1)*2 + (0: đi thẳng, 1: rẽ trái) -----
  function bitThang(h) { return 1 << ((h - 1) * 2); }
  function bitTrai(h) { return 1 << ((h - 1) * 2 + 1); }
  function matNaRieng(h) { return bitThang(h) | bitTrai(h); }
  function matNaThang(a, b) { return bitThang(a) | bitThang(b); }
  function matNaTrai(a, b) { return bitTrai(a) | bitTrai(b); }

  var CHE_DO_CO_DINH = 0, CHE_DO_THICH_UNG = 1;

  function thamSoMacDinh() {
    return {
      che_do: CHE_DO_THICH_UNG,
      t_min: 15000,            // xanh ngắn nhất
      t_max: 90000,            // xanh dài nhất
      cong_them: 5000,         // cộng thêm cho mỗi 10 xe thật đang chờ
      xe_quy_doi: 10,          // 1 xe mô hình = bao nhiêu xe thật
      t_vang: 3000,
      t_do: 2000,              // đỏ toàn phần giữa hai bước
      cd_thang: 30000,         // chế độ cố định: xanh đi thẳng
      cd_trai: 15000,          // chế độ cố định: xanh rẽ trái
      mat_tin_hieu: 3000,      // quá lâu không có tin thì coi camera mất tín hiệu
      ut_xanh_an_toan: 5000,   // bước đang chạy phải xanh ít nhất chừng này mới bị cắt để ưu tiên
      ut_giu: 5000,            // giữ xanh thêm sau khi không còn thấy xe ưu tiên
      ut_toi_da: 60000,        // ưu tiên tự hủy sau thời gian này
      cat_som: 5000            // chế độ thích ứng: tổ hợp đang xanh không còn xe quá lâu thế này thì cắt xanh sớm (0: tắt)
    };
  }

  // Hai nhóm được xanh cùng lúc khi: cùng một hướng, hoặc hai hướng đối diện và cùng loại
  // (thẳng với thẳng, trái với trái). Mọi cặp khác là xung đột.
  function hopLeMatNa(m) {
    for (var i = 0; i < 8; i++) {
      if (!(m & (1 << i))) continue;
      for (var j = i + 1; j < 8; j++) {
        if (!(m & (1 << j))) continue;
        var hi = i >> 1, hj = j >> 1;
        if (hi === hj) continue;
        if (((hi ^ hj) & 1) === 0 && (i & 1) === (j & 1)) continue;
        return false;
      }
    }
    return true;
  }

  function max2(a, b) { return a > b ? a : b; }

  // Thời gian xanh cho bước có làn đông nhất là n xe mô hình.
  function thoiGianXanh(ts, n) {
    var t = ts.t_min + Math.floor(ts.cong_them * n * ts.xe_quy_doi / 10);
    if (t > ts.t_max) t = ts.t_max;
    if (t < ts.t_min) t = ts.t_min;
    return t;
  }

  function them(kh, matNa, xanh) { kh.push({ mat_na: matNa, xanh: xanh }); }

  // Kế hoạch cho một trục (a và b là hai hướng đối diện).
  // s[h]: xe ở làn ngoài (chỉ đi thẳng); l[h]: xe ở làn trong (đi thẳng hoặc rẽ trái).
  function keHoachTruc(ts, kh, a, b, s, l, hop_le) {
    if (!hop_le[a] || !hop_le[b]) {           // camera mất tín hiệu: trục này chạy thời gian cố định
      them(kh, matNaThang(a, b), ts.cd_thang);
      them(kh, matNaTrai(a, b), ts.cd_trai);
      return;
    }
    var sa = s[a], la = l[a], sb = s[b], lb = l[b];
    var na = sa + la, nb = sb + lb;
    if (na === 0 && nb === 0) return;         // cả hai hướng trống: bỏ qua trục
    if (nb === 0) { them(kh, matNaRieng(a), thoiGianXanh(ts, max2(sa, la))); return; }
    if (na === 0) { them(kh, matNaRieng(b), thoiGianXanh(ts, max2(sb, lb))); return; }
    if (la === 0 && lb === 0) {               // không ai ở làn trong: chỉ cần đi thẳng
      them(kh, matNaThang(a, b), thoiGianXanh(ts, max2(sa, sb)));
      return;
    }
    // Phương án 1: thẳng cả hai, rồi rẽ trái có bảo vệ cho hướng nào có xe ở làn trong
    var t1 = thoiGianXanh(ts, max2(max2(sa, la), max2(sb, lb)));
    var m2, t2;
    if (la > 0 && lb > 0) { m2 = matNaTrai(a, b); t2 = thoiGianXanh(ts, max2(la, lb)); }
    else if (la > 0) { m2 = matNaRieng(a); t2 = thoiGianXanh(ts, la); }
    else { m2 = matNaRieng(b); t2 = thoiGianXanh(ts, lb); }
    // Phương án 2: riêng từng hướng, hướng đông đi trước
    var ta = thoiGianXanh(ts, max2(sa, la)), tb = thoiGianXanh(ts, max2(sb, lb));
    // Chọn phương án tốn ít thời gian hơn. Bằng nhau mà cả hai làn trong đều có xe thì chọn
    // riêng từng hướng: làn trong đi chung thẳng và trái, xe đi thẳng và xe rẽ trái dễ chặn nhau
    // nếu hai loại không được xanh cùng lúc.
    var chon_rieng = ta + tb < t1 + t2 || (ta + tb === t1 + t2 && la > 0 && lb > 0);
    if (chon_rieng) {
      if (na >= nb) { them(kh, matNaRieng(a), ta); them(kh, matNaRieng(b), tb); }
      else { them(kh, matNaRieng(b), tb); them(kh, matNaRieng(a), ta); }
    } else {
      them(kh, matNaThang(a, b), t1);
      them(kh, m2, t2);
    }
  }

  // Đầu vào: s, l, hop_le là mảng 5 phần tử, dùng chỉ số 1..4 theo số hướng.
  // Đầu ra: mảng các bước {mat_na, xanh}; tối đa 4 bước.
  function lapKeHoach(ts, s, l, hop_le) {
    var kh = [];
    if (ts.che_do === CHE_DO_CO_DINH) {       // thứ tự cố định, dùng để đối chứng
      them(kh, matNaTrai(1, 3), ts.cd_trai);
      them(kh, matNaThang(2, 4), ts.cd_thang);
      them(kh, matNaTrai(2, 4), ts.cd_trai);
      them(kh, matNaThang(1, 3), ts.cd_thang);
      return kh;
    }
    keHoachTruc(ts, kh, 1, 3, s, l, hop_le);  // trục dọc
    keHoachTruc(ts, kh, 2, 4, s, l, hop_le);  // trục ngang
    if (kh.length === 0) {                    // cả ngã tư trống: luân phiên hai trục ở Tmin
      them(kh, matNaThang(1, 3), ts.t_min);
      them(kh, matNaThang(2, 4), ts.t_min);
    }
    return kh;
  }

  // Tên tổ hợp để hiển thị, ví dụ "H1 thẳng + H3 thẳng".
  function tenToHop(m) {
    var ten = [];
    for (var h = 1; h <= 4; h++) {
      var t = (m & bitThang(h)) !== 0, r = (m & bitTrai(h)) !== 0;
      if (t && r) ten.push('H' + h + ' thẳng và trái');
      else if (t) ten.push('H' + h + ' thẳng');
      else if (r) ten.push('H' + h + ' trái');
    }
    return ten.length ? ten.join(' + ') : 'đỏ toàn phần';
  }

  return {
    CHE_DO_CO_DINH: CHE_DO_CO_DINH, CHE_DO_THICH_UNG: CHE_DO_THICH_UNG,
    bitThang: bitThang, bitTrai: bitTrai, matNaRieng: matNaRieng,
    matNaThang: matNaThang, matNaTrai: matNaTrai,
    thamSoMacDinh: thamSoMacDinh, hopLeMatNa: hopLeMatNa,
    thoiGianXanh: thoiGianXanh, lapKeHoach: lapKeHoach, tenToHop: tenToHop
  };
});
