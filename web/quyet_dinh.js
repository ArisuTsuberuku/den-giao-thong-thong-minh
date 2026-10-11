// quyet_dinh.js — Bộ lập kế hoạch đèn (mức 1: bộ luật). Chế độ cố định lập cả chu kì 4 bước;
// chế độ thích ứng lập cho từng LƯỢT: mỗi lượt là phần của một trục (dọc hoặc ngang), chốt theo
// số đếm ngay lúc lượt đó bắt đầu, hết lượt thì tới trục kia.
// Bản JavaScript này và bản C++ ở firmware/trung_tam/quyet_dinh.h phải cho cùng kết quả;
// kiem_thu/so_khop.js chạy hai bản trên cùng dữ liệu để kiểm tra điều đó.
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

  // Kiểu chia pha của một trục khi cả hai hướng đều có xe và làn trong có xe:
  //   RIENG        riêng từng hướng: mỗi hướng được xanh cả thẳng lẫn trái, hướng này xong tới hướng kia.
  //                Không bao giờ kẹt vì xe rẽ trái, nhưng hai hướng không được đi cùng lúc.
  //   THANG_TRUOC  hai hướng cùng đi thẳng trước; hai làn ngoài hết xe là chuyển sang mở riêng từng
  //                hướng cho làn trong còn xe. Lợi khi ít xe rẽ trái, thiệt khi nhiều.
  //   DOI_XUNG     hai hướng cùng đi thẳng, rồi cùng rẽ trái. Vì làn trong đi chung thẳng và trái
  //                nên xe hai loại dễ chặn nhau; chỉ hợp khi gần như không có xe rẽ trái.
  //   TU_HOC       mỗi trục tự chọn giữa RIENG và THANG_TRUOC: cứ vài lượt chạy thử kiểu kia một lượt,
  //                đo thời gian đèn cho cùng số xe rồi giữ kiểu nhanh hơn (xem dieu_khien.js)
  var KIEU_TU_HOC = 0, KIEU_RIENG = 1, KIEU_THANG_TRUOC = 2, KIEU_DOI_XUNG = 3;

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
      ut_giu: 1000,            // xe ưu tiên qua rồi (không còn thấy quá chừng này) là chuyển vàng ngay
      ut_toi_da: 60000,        // ưu tiên tự hủy sau thời gian này
      cat_som: 5000,           // chế độ thích ứng: tổ hợp đang xanh không còn xe quá lâu thế này thì cắt xanh sớm (0: tắt)
      kieu_pha: KIEU_TU_HOC,   // kiểu chia pha của chế độ thích ứng: KIEU_TU_HOC, KIEU_RIENG, KIEU_THANG_TRUOC hoặc KIEU_DOI_XUNG
      hoc_cach_thu: 4,         // tự học: đang giữ "riêng" thì cứ bấy nhiêu lượt của một trục lại thử "thẳng trước" một lượt (0: tắt tự học)
      hoc_so_xe: 4,            // tự học: lượt có ít hơn bấy nhiêu xe qua vạch dừng thì không tính
      hoc_nguong: 10           // tự học: "thẳng trước" phải nhanh hơn "riêng" ít nhất bấy nhiêu % mới được chọn
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

  // Một bước của kế hoạch. ngoai = true: bước "cùng đi thẳng" có bước mở riêng theo sau, nên khi xét
  // cắt xanh sớm chỉ tính xe ở hai làn ngoài (xe làn trong còn lại sẽ do bước mở riêng lo).
  function them(kh, matNa, xanh, ngoai) { kh.push({ mat_na: matNa, xanh: xanh, ngoai: !!ngoai }); }

  // Kế hoạch cho một trục (a và b là hai hướng đối diện).
  // s[h]: xe ở làn ngoài (chỉ đi thẳng); l[h]: xe ở làn trong (đi thẳng hoặc rẽ trái).
  function keHoachTruc(ts, kh, a, b, s, l, hop_le, kieu) {
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
    if (kieu === KIEU_DOI_XUNG) {             // cùng đi thẳng, rồi rẽ trái cho hướng có xe ở làn trong
      them(kh, matNaThang(a, b), thoiGianXanh(ts, max2(max2(sa, la), max2(sb, lb))));
      if (la > 0 && lb > 0) them(kh, matNaTrai(a, b), thoiGianXanh(ts, max2(la, lb)));
      else if (la > 0) them(kh, matNaRieng(a), thoiGianXanh(ts, la));
      else them(kh, matNaRieng(b), thoiGianXanh(ts, lb));
    } else if (kieu === KIEU_THANG_TRUOC) {   // cùng đi thẳng trước, rồi mở riêng cho làn trong còn xe
      var x = la >= lb ? a : b, y = la >= lb ? b : a;     // làn trong đông hơn được mở trước
      them(kh, matNaThang(a, b), thoiGianXanh(ts, max2(sa, sb)), true);
      if (l[x] > 0) them(kh, matNaRieng(x), thoiGianXanh(ts, l[x]));
      if (l[y] > 0) them(kh, matNaRieng(y), thoiGianXanh(ts, l[y]));
    } else {                                  // riêng từng hướng, hướng đông đi trước
      var ta = thoiGianXanh(ts, max2(sa, la)), tb = thoiGianXanh(ts, max2(sb, lb));
      if (na >= nb) { them(kh, matNaRieng(a), ta); them(kh, matNaRieng(b), tb); }
      else { them(kh, matNaRieng(b), tb); them(kh, matNaRieng(a), ta); }
    }
  }

  // Trục của một tổ hợp: 0 nếu có nhóm của Hướng 1 hoặc 3 (trục dọc), 1 nếu là trục ngang.
  function trucCuaMatNa(m) { return (m & 0x33) ? 0 : 1; }

  // Đầu vào: s, l, hop_le là mảng 5 phần tử, dùng chỉ số 1..4 theo số hướng.
  //   kieu: [kiểu chia pha trục dọc, kiểu chia pha trục ngang], mỗi cái là KIEU_RIENG, KIEU_THANG_TRUOC hoặc KIEU_DOI_XUNG
  //   truc: trục tới lượt (0 dọc, 1 ngang). Trục đó không có xe thì lập luôn cho trục kia.
  // Đầu ra: mảng các bước {mat_na, xanh, ngoai}; tối đa 4 bước. Chế độ thích ứng: 1 đến 3 bước của một trục.
  function lapKeHoach(ts, s, l, hop_le, kieu, truc) {
    var kh = [];
    var k_doc = kieu ? kieu[0] : KIEU_RIENG, k_ngang = kieu ? kieu[1] : KIEU_RIENG;
    if (ts.che_do === CHE_DO_CO_DINH) {       // thứ tự cố định, dùng để đối chứng
      them(kh, matNaTrai(1, 3), ts.cd_trai);
      them(kh, matNaThang(2, 4), ts.cd_thang);
      them(kh, matNaTrai(2, 4), ts.cd_trai);
      them(kh, matNaThang(1, 3), ts.cd_thang);
      return kh;
    }
    if (truc === 1) {
      keHoachTruc(ts, kh, 2, 4, s, l, hop_le, k_ngang);
      if (kh.length === 0) keHoachTruc(ts, kh, 1, 3, s, l, hop_le, k_doc);
      if (kh.length === 0) them(kh, matNaThang(2, 4), ts.t_min);   // cả ngã tư trống: trục tới lượt xanh Tmin
    } else {
      keHoachTruc(ts, kh, 1, 3, s, l, hop_le, k_doc);
      if (kh.length === 0) keHoachTruc(ts, kh, 2, 4, s, l, hop_le, k_ngang);
      if (kh.length === 0) them(kh, matNaThang(1, 3), ts.t_min);
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
    KIEU_TU_HOC: KIEU_TU_HOC, KIEU_RIENG: KIEU_RIENG, KIEU_THANG_TRUOC: KIEU_THANG_TRUOC, KIEU_DOI_XUNG: KIEU_DOI_XUNG,
    bitThang: bitThang, bitTrai: bitTrai, matNaRieng: matNaRieng,
    matNaThang: matNaThang, matNaTrai: matNaTrai,
    thamSoMacDinh: thamSoMacDinh, hopLeMatNa: hopLeMatNa, trucCuaMatNa: trucCuaMatNa,
    thoiGianXanh: thoiGianXanh, lapKeHoach: lapKeHoach, tenToHop: tenToHop
  };
});
