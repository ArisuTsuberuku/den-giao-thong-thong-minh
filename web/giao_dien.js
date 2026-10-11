// giao_dien.js — Vẽ sa bàn, 4 ô camera, bảng thông số và số liệu cho web mô phỏng.
// Phần tính toán nằm ở mo_phong.js (xe), quyet_dinh.js và dieu_khien.js (đèn).
(function () {
  'use strict';
  var QD = QuyetDinh, H = MoPhong.HINH;
  var $ = function (id) { return document.getElementById(id); };

  var NHIP = 40;                 // một nhịp mô phỏng = 40 ms
  var KHOA_LUU = 'sa_ban_den_giao_thong_v1';
  var mp = new MoPhong({ hat: 2026 });
  var ts = mp.ts, cai = mp.cai;
  var tt = { chay: true, toc_do: 2, hien_vung: false, du: 0, t_truoc: 0, khung: 0 };
  var nhat_ky = [];

  // ---------- Thông số trên bảng ----------
  var TRUOT_DEN = [
    { id: 't_min', ten: 'Xanh ngắn nhất (Tmin)', min: 5, max: 60, buoc: 1, dv: ' s', he: 1000 },
    { id: 't_max', ten: 'Xanh dài nhất (Tmax)', min: 30, max: 120, buoc: 5, dv: ' s', he: 1000 },
    { id: 'cong_them', ten: 'Cộng thêm cho mỗi 10 xe thật', min: 0, max: 15, buoc: 1, dv: ' s', he: 1000 },
    { id: 'xe_quy_doi', ten: '1 xe mô hình bằng', min: 1, max: 20, buoc: 1, dv: ' xe thật', he: 1 },
    { id: 'cat_som', ten: 'Cắt xanh sớm khi hết xe quá', min: 0, max: 15, buoc: 1, dv: ' s', he: 1000, khong: 'tắt' },
    { id: 't_vang', ten: 'Vàng', min: 2, max: 6, buoc: 1, dv: ' s', he: 1000 },
    { id: 't_do', ten: 'Đỏ toàn phần', min: 0, max: 5, buoc: 1, dv: ' s', he: 1000 },
    { id: 'cd_thang', ten: 'Cố định: xanh đi thẳng', min: 10, max: 60, buoc: 5, dv: ' s', he: 1000 },
    { id: 'cd_trai', ten: 'Cố định: xanh rẽ trái', min: 5, max: 40, buoc: 5, dv: ' s', he: 1000 }
  ];
  var MAU_DONG_XE = [
    { ten: 'Đều', luu_luong: [6, 6, 6, 6], trai: [25, 25, 25, 25] },
    { ten: 'Dọc đông, ngang vắng', luu_luong: [10, 2, 9, 1], trai: [25, 10, 20, 10] },
    { ten: 'Chỉ Hướng 1', luu_luong: [8, 0, 0, 0], trai: [40, 0, 0, 0] },
    { ten: 'Giờ cao điểm', luu_luong: [12, 12, 12, 12], trai: [30, 30, 30, 30] },
    { ten: 'Ít rẽ trái', luu_luong: [8, 8, 8, 8], trai: [5, 5, 5, 5] },
    { ten: 'Vắng', luu_luong: [1.5, 1.5, 1.5, 1.5], trai: [25, 25, 25, 25] }
  ];
  var TEN_HUONG = ['', 'Hướng 1 (từ trên)', 'Hướng 2 (từ phải)', 'Hướng 3 (từ dưới)', 'Hướng 4 (từ trái)'];

  function soVn(x, le) { return x.toFixed(le === undefined ? 0 : le).replace('.', ','); }
  function gioPhut(ms) {
    var s = Math.floor(ms / 1000), p = Math.floor(s / 60);
    return (p < 10 ? '0' : '') + p + ':' + (s % 60 < 10 ? '0' : '') + (s % 60);
  }

  function taoTruot(cha, id, ten, min, max, buoc, layGiaTri, datGiaTri, hienThi) {
    var khoi = document.createElement('div');
    khoi.className = 'truot';
    khoi.innerHTML = '<label for="' + id + '">' + ten + '</label><output for="' + id + '" id="' + id + '_so"></output>' +
      '<input type="range" id="' + id + '" min="' + min + '" max="' + max + '" step="' + buoc + '">';
    cha.appendChild(khoi);
    var o = khoi.querySelector('input'), so = khoi.querySelector('output');
    function capNhat() { o.value = layGiaTri(); so.textContent = hienThi(Number(o.value)); }
    o.addEventListener('input', function () { datGiaTri(Number(o.value)); so.textContent = hienThi(Number(o.value)); luu(); });
    capNhat();
    return capNhat;
  }

  var cap_nhat_truot = [];
  function dungBang() {
    var muc_den = $('muc_den');
    TRUOT_DEN.forEach(function (d) {
      cap_nhat_truot.push(taoTruot(muc_den, 'ts_' + d.id, d.ten, d.min, d.max, d.buoc,
        function () { return ts[d.id] / d.he; },
        function (v) { ts[d.id] = Math.round(v * d.he); capNhatGhiChuDen(); },
        function (v) { return v === 0 && d.khong ? d.khong : soVn(v) + d.dv; }));
    });
    var gc = document.createElement('p');
    gc.className = 'ghi_chu'; gc.id = 'ghi_chu_den';
    muc_den.appendChild(gc);

    var muc_xe = $('muc_xe');
    MAU_DONG_XE.forEach(function (m, i) {
      var b = document.createElement('button');
      b.type = 'button'; b.className = 'nut nho'; b.id = 'mau_' + i; b.textContent = m.ten;
      b.addEventListener('click', function () {
        for (var h = 1; h <= 4; h++) { cai.luu_luong[h] = m.luu_luong[h - 1]; cai.ti_le_trai[h] = m.trai[h - 1]; }
        cap_nhat_truot.forEach(function (f) { f(); });
        luu();
      });
      $('mau_dong_xe').appendChild(b);
    });
    for (var h = 1; h <= 4; h++) (function (h) {
      var khoi = document.createElement('div');
      khoi.className = 'huong';
      khoi.innerHTML = '<div class="huong_dau"><strong>' + TEN_HUONG[h] + '</strong>' +
        '<button type="button" class="nut nho canh_bao" id="cuu_thuong_' + h + '">Thả xe cứu thương</button></div>' +
        '<div class="huong_dem" id="huong_dem_' + h + '"></div>';
      muc_xe.appendChild(khoi);
      cap_nhat_truot.push(taoTruot(khoi, 'luu_luong_' + h, 'Lưu lượng', 0, 20, 0.5,
        function () { return cai.luu_luong[h]; }, function (v) { cai.luu_luong[h] = v; },
        function (v) { return soVn(v, 1) + ' xe/phút'; }));
      cap_nhat_truot.push(taoTruot(khoi, 'ti_le_trai_' + h, 'Rẽ trái', 0, 60, 5,
        function () { return cai.ti_le_trai[h]; }, function (v) { cai.ti_le_trai[h] = v; },
        function (v) { return soVn(v) + ' %'; }));
      khoi.querySelector('button').addEventListener('click', function () { mp.thaXeCuuThuong(h); ghi('Thả xe cứu thương vào Hướng ' + h + '.'); });
    })(h);
    cap_nhat_truot.push(taoTruot(muc_xe, 'ti_le_phai', 'Rẽ phải (làn riêng, không qua đèn)', 0, 40, 5,
      function () { return cai.ti_le_phai; }, function (v) { cai.ti_le_phai = v; }, function (v) { return soVn(v) + ' %'; }));
    cap_nhat_truot.push(taoTruot(muc_xe, 'toc_do_xe', 'Tốc độ xe', 60, 180, 10,
      function () { return cai.toc_do; }, function (v) { cai.toc_do = v; }, function (v) { return soVn(v) + ' mm/s'; }));

    var chon = $('chon_toc_do');
    [1, 2, 4, 8, 16].forEach(function (v) {
      var id = 'toc_do_' + v;
      chon.insertAdjacentHTML('beforeend', '<input type="radio" name="toc_do" id="' + id + '" value="' + v + '"><label for="' + id + '">×' + v + '</label>');
      $(id).addEventListener('change', function () { tt.toc_do = v; luu(); });
    });
    [0, 1].forEach(function (v) {
      $('che_do_' + v).addEventListener('change', function () {
        ts.che_do = v; luu();
        ghi('Chọn chế độ ' + (v ? 'thích ứng' : 'cố định') + ', áp dụng từ chu kì sau.');
      });
    });
    $('nut_chay').addEventListener('click', function () {
      tt.chay = !tt.chay;
      $('nut_chay').textContent = tt.chay ? 'Tạm dừng' : 'Chạy tiếp';
    });
    $('nut_lai').addEventListener('click', function () { nhat_ky = []; mp.datLai(); ghi('Chạy lại từ đầu với cùng dòng xe.'); });
    $('hien_vung').addEventListener('change', function () { tt.hien_vung = $('hien_vung').checked; luu(); });
    $('nut_so_sanh').addEventListener('click', chaySoSanh);
  }

  function capNhatGhiChuDen() {
    var moi_xe = ts.cong_them * ts.xe_quy_doi / 10 / 1000;
    $('ghi_chu_den').textContent = 'Chế độ thích ứng: xanh = ' + soVn(ts.t_min / 1000) + ' s + ' + soVn(moi_xe, 1) +
      ' s cho mỗi xe mô hình ở làn đông nhất, không quá ' + soVn(Math.max(ts.t_max, ts.t_min) / 1000) + ' s.' +
      (ts.cat_som > 0 ? ' Tổ hợp đang xanh hết xe quá ' + soVn(ts.cat_som / 1000) + ' s mà hướng khác có xe chờ thì chuyển vàng ngay; bước kế không còn xe thì bỏ qua.' : '');
  }
  function dongBoNut() {
    $('che_do_' + (ts.che_do === QD.CHE_DO_CO_DINH ? 0 : 1)).checked = true;
    var o = $('toc_do_' + tt.toc_do);
    if (o) o.checked = true;
    $('hien_vung').checked = tt.hien_vung;
    cap_nhat_truot.forEach(function (f) { f(); });
    capNhatGhiChuDen();
  }

  // ---------- Nhớ thông số trên máy người xem ----------
  function luu() {
    try {
      localStorage.setItem(KHOA_LUU, JSON.stringify({ ts: ts, cai: cai, toc_do: tt.toc_do, hien_vung: tt.hien_vung }));
    } catch (e) { /* không lưu được thì thôi */ }
  }
  function docLai() {
    try {
      var d = JSON.parse(localStorage.getItem(KHOA_LUU) || 'null');
      if (!d) return;
      var k;
      for (k in ts) if (typeof d.ts[k] === 'number' && isFinite(d.ts[k])) ts[k] = d.ts[k];
      for (var h = 1; h <= 4; h++) {
        if (isFinite(d.cai.luu_luong[h])) cai.luu_luong[h] = Number(d.cai.luu_luong[h]);
        if (isFinite(d.cai.ti_le_trai[h])) cai.ti_le_trai[h] = Number(d.cai.ti_le_trai[h]);
      }
      if (isFinite(d.cai.ti_le_phai)) cai.ti_le_phai = Number(d.cai.ti_le_phai);
      if (isFinite(d.cai.toc_do)) cai.toc_do = Number(d.cai.toc_do);
      if ([1, 2, 4, 8, 16].indexOf(d.toc_do) >= 0) tt.toc_do = d.toc_do;
      tt.hien_vung = !!d.hien_vung;
    } catch (e) { /* dữ liệu cũ hỏng: dùng mặc định */ }
  }

  // ---------- Nhật ký ----------
  function ghi(noi_dung) {
    nhat_ky.unshift({ t: mp.t, noi_dung: noi_dung });
    if (nhat_ky.length > 7) nhat_ky.length = 7;
  }
  function tomTatKeHoach(kh) {
    return kh.map(function (b) { return QD.tenToHop(b.mat_na) + ' ' + soVn(b.xanh / 1000) + ' s'; }).join(' → ');
  }
  mp.khiSuKien = function (loai, dl) {
    if (loai === 'chu_ki') ghi('Chu kì ' + mp.dk.chu_ki + ': ' + tomTatKeHoach(mp.dk.kh) + '.');
    else if (loai === 'cat_som') ghi('Hết xe ở ' + QD.tenToHop(dl) + ' quá ' + soVn(ts.cat_som / 1000) + ' s: cắt xanh sớm.');
    else if (loai === 'uu_tien') ghi('Camera Hướng ' + dl + ' thấy xe ưu tiên: kết thúc bước đang chạy, mở riêng Hướng ' + dl + '.');
    else if (loai === 'het_uu_tien') ghi('Hết ưu tiên Hướng ' + dl + ', lập kế hoạch chu kì mới.');
    else if (loai === 'cuu_thuong_qua') ghi('Xe cứu thương Hướng ' + dl.h + ' qua vạch dừng sau ' + soVn(dl.ms / 1000, 1) + ' s.');
    else if (loai === 'loi') ghi('LỖI AN TOÀN: tổ hợp đèn xung đột, chuyển sang nháy vàng.');
  };

  // ---------- Nền sa bàn: vẽ tệp decal một lần ra bộ đệm ----------
  var nen = document.createElement('canvas'), nen_xong = false;
  nen.width = nen.height = 2000;
  (function () {
    var anh = new Image();
    anh.onload = function () {
      var c = nen.getContext('2d');
      c.drawImage(anh, 0, 0, 2000, 2000);
      nen_xong = true;
    };
    anh.src = 'data:image/svg+xml;charset=utf-8,' + encodeURIComponent(SA_BAN_SVG);
  })();

  // ---------- Vẽ ----------
  function hinhBoTron(c, x, y, w, h, r) {
    c.beginPath();
    c.moveTo(x + r, y); c.lineTo(x + w - r, y); c.arcTo(x + w, y, x + w, y + r, r);
    c.lineTo(x + w, y + h - r); c.arcTo(x + w, y + h, x + w - r, y + h, r);
    c.lineTo(x + r, y + h); c.arcTo(x, y + h, x, y + h - r, r);
    c.lineTo(x, y + r); c.arcTo(x, y, x + r, y, r);
    c.closePath();
  }

  function veXe(c, xe) {
    var dai = H.XE_DAI, rong = H.XE_RONG, con = xe.dd.dai - xe.s;
    var mo = Math.min(1, xe.s / 55, con / 70);
    if (mo <= 0) return;
    c.save();
    c.globalAlpha = mo;
    c.translate(xe.x, xe.y);
    c.rotate(xe.goc);
    c.fillStyle = 'rgba(0,0,0,.28)';                      // bóng
    hinhBoTron(c, -dai / 2 + 2, -rong / 2 + 3, dai, rong, 9); c.fill();
    c.fillStyle = xe.mau;                                  // thân
    hinhBoTron(c, -dai / 2, -rong / 2, dai, rong, 9); c.fill();
    c.fillStyle = 'rgba(15,22,30,.82)';                    // kính trước, kính sau
    hinhBoTron(c, 4, -rong / 2 + 4, 15, rong - 8, 4); c.fill();
    hinhBoTron(c, -dai / 2 + 9, -rong / 2 + 5, 9, rong - 10, 3); c.fill();
    c.fillStyle = 'rgba(255,255,255,.22)';                 // nóc
    hinhBoTron(c, -dai / 2 + 20, -rong / 2 + 5, 13, rong - 10, 3); c.fill();
    c.fillStyle = '#fff6c9';                               // đèn pha
    c.fillRect(dai / 2 - 4, -rong / 2 + 3, 3, 6); c.fillRect(dai / 2 - 4, rong / 2 - 9, 3, 6);
    c.fillStyle = xe.phanh ? '#ff2a1a' : '#7a1510';        // đèn hậu sáng lên khi phanh
    c.fillRect(-dai / 2 + 1, -rong / 2 + 3, 3, 7); c.fillRect(-dai / 2 + 1, rong / 2 - 10, 3, 7);
    var nhay = Math.floor(mp.t / 350) % 2 === 0;
    if (!xe.da_qua && nhay && xe.huong_di !== 'thang') {   // xi nhan
      var ben = xe.huong_di === 'trai' ? -1 : 1;
      c.fillStyle = '#ffb000';
      c.fillRect(dai / 2 - 8, ben * (rong / 2 - 4) - 2.5, 6, 5);
      c.fillRect(-dai / 2 + 2, ben * (rong / 2 - 4) - 2.5, 6, 5);
    }
    if (xe.uu_tien) {                                      // xe cứu thương: chữ thập đỏ và đèn chớp
      c.fillStyle = '#d5372c';
      c.fillRect(-13, -3, 20, 6); c.fillRect(-6, -10, 6, 20);
      c.fillStyle = nhay ? '#2f7bff' : '#ff3b30';
      c.fillRect(-dai / 2 + 20, -rong / 2 + 2, 6, 6);
      c.fillStyle = nhay ? '#ff3b30' : '#2f7bff';
      c.fillRect(-dai / 2 + 20, rong / 2 - 8, 6, 6);
    }
    c.restore();
  }

  var MAU_DEN = [['#ff3b30', '#4a1512'], ['#ffc400', '#4a3a08'], ['#27e07a', '#0e3d22']];   // [sáng, tắt] cho đỏ, vàng, xanh
  function veCumDen(c, x, y, mau) {
    c.fillStyle = '#15171a';
    hinhBoTron(c, x - 22, y - 8.5, 44, 17, 6); c.fill();
    c.strokeStyle = '#6d747c'; c.lineWidth = 1.2; c.stroke();
    for (var i = 0; i < 3; i++) {
      var sang = (i === 0 && mau === 0) || (i === 1 && mau === 1) || (i === 2 && mau === 2);
      var lx = x - 14 + i * 14;
      if (sang) {
        var g = c.createRadialGradient(lx, y, 1, lx, y, 17);
        g.addColorStop(0, MAU_DEN[i][0]); g.addColorStop(.35, MAU_DEN[i][0] + 'aa'); g.addColorStop(1, MAU_DEN[i][0] + '00');
        c.fillStyle = g;
        c.beginPath(); c.arc(lx, y, 17, 0, 6.3); c.fill();
      }
      c.fillStyle = sang ? MAU_DEN[i][0] : MAU_DEN[i][1];
      c.beginPath(); c.arc(lx, y, 5.6, 0, 6.3); c.fill();
    }
  }
  function veDen(c) {
    for (var h = 1; h <= 4; h++) {
      c.save();
      c.translate(500, 500); c.rotate((h - 1) * Math.PI / 2); c.translate(-500, -500);
      c.strokeStyle = '#8a9099'; c.lineWidth = 3;          // cần vươn của khung đèn, từ cột trên đảo dẫn hướng
      c.beginPath(); c.moveTo(384, 375); c.lineTo(384, 383); c.lineTo(493, 383); c.stroke();
      veCumDen(c, H.X_NGOAI, 383, mp.dk.mauNhom((h - 1) * 2, mp.t));       // cụm mũi tên thẳng trên làn ngoài
      veCumDen(c, H.X_TRONG, 383, mp.dk.mauNhom((h - 1) * 2 + 1, mp.t));   // cụm mũi tên rẽ trái trên làn trong
      c.restore();
    }
  }
  function veVung(c, dam) {
    var v = H.VUNG;
    c.fillStyle = 'rgba(64,196,255,' + dam + ')';
    c.fillRect(v.ngoai.x0, v.ngoai.y0, v.ngoai.x1 - v.ngoai.x0, v.ngoai.y1 - v.ngoai.y0);
    c.fillStyle = 'rgba(255,120,220,' + dam + ')';
    c.fillRect(v.trong.x0, v.trong.y0, v.trong.x1 - v.trong.x0, v.trong.y1 - v.trong.y0);
  }

  var o_sa_ban = $('sa_ban'), c_sa_ban = o_sa_ban.getContext('2d');
  function veSaBan() {
    var c = c_sa_ban, k = o_sa_ban.width / 1000;
    c.setTransform(k, 0, 0, k, 0, 0);
    if (nen_xong) c.drawImage(nen, 0, 0, 1000, 1000);
    else { c.fillStyle = '#3b3e44'; c.fillRect(0, 0, 1000, 1000); }
    if (tt.hien_vung) {
      for (var h = 1; h <= 4; h++) {
        c.save();
        c.translate(500, 500); c.rotate((h - 1) * Math.PI / 2); c.translate(-500, -500);
        veVung(c, 0.3);
        c.restore();
      }
    }
    for (var i = 0; i < mp.xe.length; i++) veXe(c, mp.xe[i]);
    veDen(c);
  }

  // Ô camera: nhìn từ trên vạch dừng ngược về phía xe đang tới; khung 4:3 rộng hơn 2 làn nhiều
  // nên firmware thật cắt ROI quanh 2 làn rồi mới đưa vào mô hình.
  var CAM_X0 = 220, CAM_Y0 = 40, CAM_RONG = 453.33;
  var o_cam = [], c_cam = [];
  function veCamera(h) {
    var o = o_cam[h], c = c_cam[h], k = o.width / CAM_RONG, i;
    c.setTransform(k, 0, 0, k, 0, 0);
    c.translate(-CAM_X0, -CAM_Y0);
    c.save();
    c.translate(500, 500); c.rotate(-(h - 1) * Math.PI / 2); c.translate(-500, -500);
    if (nen_xong) c.drawImage(nen, 0, 0, 1000, 1000);
    for (i = 0; i < mp.xe.length; i++) veXe(c, mp.xe[i]);
    c.restore();
    veVung(c, 0.2);
    c.setLineDash([7, 5]); c.strokeStyle = '#ffffff'; c.lineWidth = 2;     // khung ROI
    c.strokeRect(H.VUNG.ngoai.x0 - 6, H.VUNG.ngoai.y0 - 6, 2 * H.LAN + 12, H.VUNG.ngoai.y1 - H.VUNG.ngoai.y0 + 12);
    c.setLineDash([]);
    for (i = 0; i < mp.xe.length; i++) {                                    // khung quanh xe đếm được
      var xe = mp.xe[i], lan = MoPhong.lanCuaDiem(xe.x, xe.y, h);
      if (!lan) continue;
      var p = MoPhong.quayNguoc(xe.x, xe.y, h), g = xe.goc - (h - 1) * Math.PI / 2;
      var w = Math.abs(Math.cos(g)) * H.XE_DAI + Math.abs(Math.sin(g)) * H.XE_RONG + 8;
      var cao = Math.abs(Math.sin(g)) * H.XE_DAI + Math.abs(Math.cos(g)) * H.XE_RONG + 8;
      c.strokeStyle = xe.uu_tien ? '#ff3b30' : (lan === 'ngoai' ? '#40c4ff' : '#ff78dc');
      c.lineWidth = xe.uu_tien ? 4 : 2.5;
      c.strokeRect(p[0] - w / 2, p[1] - cao / 2, w, cao);
    }
    c.setTransform(1, 0, 0, 1, 0, 0);
    var to = o.width / 230;                                                 // chữ theo cỡ ô
    var vien = c.createRadialGradient(o.width / 2, o.height / 2, o.height * .35, o.width / 2, o.height / 2, o.width * .75);
    vien.addColorStop(0, 'rgba(0,0,0,0)'); vien.addColorStop(1, 'rgba(0,0,0,.45)');
    c.fillStyle = vien; c.fillRect(0, 0, o.width, o.height);
    c.font = '600 ' + Math.round(11 * to) + 'px ' + 'ui-monospace, Consolas, monospace';
    c.textBaseline = 'top';
    var d = mp.dem[h];
    c.fillStyle = 'rgba(0,0,0,.6)'; c.fillRect(0, 0, o.width, 17 * to);
    c.fillStyle = '#fff'; c.fillText('CAM ' + h, 5 * to, 3 * to);
    c.fillStyle = '#40c4ff'; c.fillText('s=' + d.s, 52 * to, 3 * to);
    c.fillStyle = '#ff78dc'; c.fillText('l=' + d.l, 84 * to, 3 * to);
    c.fillStyle = d.emg ? '#ff5a4f' : '#b9c0c7'; c.fillText('emg=' + d.emg, 116 * to, 3 * to);
    if (d.emg) { c.fillStyle = '#ff3b30'; c.fillText('xe_uu_tien', 160 * to, 3 * to); }
    c.fillStyle = 'rgba(0,0,0,.6)'; c.fillRect(0, o.height - 16 * to, o.width, 16 * to);   // bản tin gửi về board trung tâm
    c.font = Math.round(8.6 * to) + 'px ui-monospace, Consolas, monospace';
    c.fillStyle = '#e9edf0'; c.fillText(mp.jsonCamera(h), 5 * to, o.height - 12.5 * to);
  }

  // Sơ đồ 8 nhóm tín hiệu: mỗi hướng một mũi tên thẳng và một mũi tên rẽ trái.
  var o_so_do = $('so_do_pha'), c_so_do = o_so_do.getContext('2d');
  var MAU_MUI_TEN = ['#b3382f', '#ffc400', '#27e07a'];
  function muiTen(c, diem, mau, dam) {
    c.strokeStyle = mau; c.fillStyle = mau; c.lineWidth = dam; c.lineCap = 'round'; c.lineJoin = 'round';
    c.beginPath(); c.moveTo(diem[0][0], diem[0][1]);
    for (var i = 1; i < diem.length; i++) c.lineTo(diem[i][0], diem[i][1]);
    c.stroke();
    var a = diem[diem.length - 2], b = diem[diem.length - 1], g = Math.atan2(b[1] - a[1], b[0] - a[0]);
    c.beginPath();
    c.moveTo(b[0] + 5 * Math.cos(g), b[1] + 5 * Math.sin(g));
    c.lineTo(b[0] + 5.5 * Math.cos(g + 2.4), b[1] + 5.5 * Math.sin(g + 2.4));
    c.lineTo(b[0] + 5.5 * Math.cos(g - 2.4), b[1] + 5.5 * Math.sin(g - 2.4));
    c.closePath(); c.fill();
  }
  function veSoDoPha() {
    var c = c_so_do, k = o_so_do.width / 100;
    c.setTransform(k, 0, 0, k, 0, 0);
    c.clearRect(0, 0, 100, 100);
    c.fillStyle = '#3b3e44';
    hinhBoTron(c, 30, 0, 40, 100, 3); c.fill();
    hinhBoTron(c, 0, 30, 100, 40, 3); c.fill();
    for (var h = 1; h <= 4; h++) {
      c.save();
      c.translate(50, 50); c.rotate((h - 1) * Math.PI / 2); c.translate(-50, -50);
      var m_thang = mp.dk.mauNhom((h - 1) * 2, mp.t), m_trai = mp.dk.mauNhom((h - 1) * 2 + 1, mp.t);
      muiTen(c, [[36, 5], [36, 21]], MAU_MUI_TEN[m_thang], m_thang ? 4.2 : 2.6);
      muiTen(c, [[45, 5], [45, 16], [51, 22]], MAU_MUI_TEN[m_trai], m_trai ? 4.2 : 2.6);
      c.fillStyle = '#ffffff'; c.font = '600 7px sans-serif'; c.textAlign = 'center'; c.textBaseline = 'middle';
      c.translate(58, 9); c.rotate(-(h - 1) * Math.PI / 2);
      c.fillText('H' + h, 0, 0);
      c.restore();
    }
  }

  // ---------- Số liệu bên cạnh sa bàn ----------
  function danhSachBuoc(o, kh, hien_tai) {
    var html = '';
    for (var i = 0; i < kh.length; i++) {
      html += '<li' + (i === hien_tai ? ' class="hien_tai"' : '') + '><span>' + QD.tenToHop(kh[i].mat_na) + '</span><b>' + soVn(kh[i].xanh / 1000) + ' s</b></li>';
    }
    o.innerHTML = html;
  }
  function chuKi(kh) {
    var t = 0;
    for (var i = 0; i < kh.length; i++) t += kh[i].xanh + ts.t_vang + ts.t_do;
    return t / 1000;
  }
  function capNhatSoLieu() {
    var dk = mp.dk, t = mp.t, con = dk.conLai(t), P = DieuKhien;
    var mau, ten;
    if (dk.loi) { mau = ['do', 'Lỗi']; ten = 'Tổ hợp đèn xung đột: nháy vàng tất cả.'; }
    else if (dk.pha === P.PHA_KHOI_DONG) { mau = ['do', 'Khởi động']; ten = 'Đỏ toàn phần trước khi chạy.'; }
    else if (dk.pha === P.PHA_VANG) { mau = ['vang', 'Vàng']; ten = 'Kết thúc: ' + QD.tenToHop(dk.vang) + (dk.xanh ? '. Giữ xanh: ' + QD.tenToHop(dk.xanh) : '') + '.'; }
    else if (dk.pha === P.PHA_DO_DON) { mau = ['do', 'Đỏ toàn phần']; ten = 'Sắp xanh: ' + QD.tenToHop(dk.dich) + '.'; }
    else if (dk.ut_huong) { mau = ['uu_tien', 'Ưu tiên']; ten = 'Mở riêng Hướng ' + dk.ut_huong + ' cho xe cứu thương.'; }
    else { mau = ['xanh', 'Xanh']; ten = QD.tenToHop(dk.xanh) + '.'; }
    if (dk.ut_huong && dk.pha !== P.PHA_XANH) mau = [mau[0], mau[1] + ' · sắp ưu tiên'];
    $('pha_mau').dataset.mau = mau[0]; $('pha_mau').textContent = mau[1];
    $('pha_ten').textContent = ten;
    $('dem_nguoc').innerHTML = con < 0 ? '–' : Math.ceil(con / 1000) + ' <small>s</small>';

    $('tieu_de_ke_hoach').textContent = 'Kế hoạch chu kì ' + dk.chu_ki;
    danhSachBuoc($('ke_hoach'), dk.kh, dk.ut_huong ? -1 : dk.i_buoc);
    $('tong_chu_ki').textContent = dk.kh.length ? 'Cả chu kì ' + soVn(chuKi(dk.kh)) + ' s, tính cả vàng và đỏ toàn phần.' : '';

    var s = [0, 0, 0, 0, 0], l = [0, 0, 0, 0, 0], hl = [false, false, false, false, false], h;
    for (h = 1; h <= 4; h++) { s[h] = dk.dem[h].s; l[h] = dk.dem[h].l; hl[h] = dk.conHan(h, t); }
    var sau = QD.lapKeHoach(ts, s, l, hl);
    danhSachBuoc($('ke_hoach_sau'), sau, -1);
    $('tong_chu_ki_sau').textContent = 'Cả chu kì ' + soVn(chuKi(sau)) + ' s.';

    $('dong_ho').textContent = gioPhut(t) + '  ×' + tt.toc_do;
    var dang = mp.che_do_chay, ngoai = 0, tren = 0, i;
    for (i = 0; i < 2; i++) {
      var tk = mp.thong_ke[i], o = $('hang_tk_' + i).children;
      $('hang_tk_' + i).className = i === dang ? 'dang_dung' : '';
      o[1].textContent = tk.so_xe;
      o[2].textContent = tk.so_xe ? soVn(tk.tong_cho / tk.so_xe / 1000, 1) : '–';
      o[3].textContent = tk.so_xe ? soVn(tk.cho_max / 1000) : '–';
    }
    for (h = 1; h <= 4; h++) {
      ngoai += mp.hang_cho[h].length;
      $('huong_dem_' + h).textContent = 'camera: s ' + mp.dem[h].s + ' · l ' + mp.dem[h].l + ' · emg ' + mp.dem[h].emg + '   chờ ngoài sa bàn: ' + mp.hang_cho[h].length;
    }
    for (i = 0; i < mp.xe.length; i++) if (!mp.xe[i].da_qua) tren++;
    $('dang_cho').textContent = 'Chưa qua vạch dừng: ' + tren + ' xe trên sa bàn, ' + ngoai + ' xe chờ ngoài sa bàn.';

    var html = '';
    for (i = 0; i < nhat_ky.length; i++) html += '<li><time>' + gioPhut(nhat_ky[i].t) + '</time><span>' + nhat_ky[i].noi_dung + '</span></li>';
    $('nhat_ky').innerHTML = html;
  }

  // ---------- So sánh hai chế độ (chạy nhanh, không vẽ) ----------
  var dang_so_sanh = false;
  function chaySoSanh() {
    if (dang_so_sanh) return;
    dang_so_sanh = true;
    var PHUT = 30, so_buoc = PHUT * 60000 / NHIP, o = $('ket_qua_so_sanh'), may = [], da = 0;
    for (var che_do = 0; che_do <= 1; che_do++) {
      var ts2 = JSON.parse(JSON.stringify(ts));
      ts2.che_do = che_do;
      may.push(new MoPhong({ hat: 2026, ts: ts2, cai: JSON.parse(JSON.stringify(cai)) }));
    }
    $('nut_so_sanh').disabled = true;
    function motDot() {
      var het = Date.now() + 30;
      while (da < so_buoc * 2 && Date.now() < het) { may[da < so_buoc ? 0 : 1].buoc(NHIP); da++; }
      if (da < so_buoc * 2) {
        o.innerHTML = '<p class="ghi_chu">Đang chạy… ' + Math.round(da / so_buoc * 50) + ' %</p>';
        setTimeout(motDot, 0);
        return;
      }
      var a = may[0].tongHop(), b = may[1].tongHop();
      var doi = a.cho_tb > 0 ? (b.cho_tb - a.cho_tb) / a.cho_tb * 100 : 0;
      o.innerHTML = '<table class="so_lieu"><thead><tr><th>' + PHUT + ' phút</th><th>Chờ TB</th><th>Lâu nhất</th><th>Đã qua</th><th>Kẹt</th></tr></thead><tbody>' +
        '<tr><td>Cố định</td><td>' + soVn(a.cho_tb, 1) + '</td><td>' + soVn(a.cho_max) + '</td><td>' + a.da_qua + '</td><td>' + a.con_ket + '</td></tr>' +
        '<tr><td>Thích ứng</td><td>' + soVn(b.cho_tb, 1) + '</td><td>' + soVn(b.cho_max) + '</td><td>' + b.da_qua + '</td><td>' + b.con_ket + '</td></tr></tbody></table>' +
        '<p class="ket_luan">' + (Math.abs(doi) < 1 ? 'Hai chế độ gần như bằng nhau.' :
          'Chế độ thích ứng ' + (doi < 0 ? 'giảm ' : 'tăng ') + soVn(Math.abs(doi)) + ' % thời gian chờ trung bình so với cố định.') +
        ' Thời gian chờ tính bằng giây, kể cả của xe còn kẹt lại khi hết giờ.</p>';
      $('nut_so_sanh').disabled = false;
      dang_so_sanh = false;
    }
    motDot();
  }

  // ---------- Kích thước theo màn hình ----------
  function datCo(o, rong, cao) {
    var ti_le = Math.min(window.devicePixelRatio || 1, 2.5);
    o.style.width = rong + 'px'; o.style.height = cao + 'px';
    var w = Math.round(rong * ti_le), h = Math.round(cao * ti_le);
    if (o.width !== w || o.height !== h) { o.width = w; o.height = h; }
  }
  function xepCho() {
    var ung_dung = $('ung_dung'), san = $('san'), dien_thoai = window.innerWidth <= 900, canh, i;
    // ô camera: cao khoảng 1/5 màn hình
    var o_dau = o_cam[1].parentNode;
    var cao_cam = dien_thoai ? 1e9 : Math.max(110, Math.min(240, window.innerHeight * 0.22));
    var rong_cam = Math.floor(Math.min(o_dau.clientWidth, cao_cam * 4 / 3));
    for (i = 1; i <= 4; i++) datCo(o_cam[i], rong_cam, Math.round(rong_cam * 3 / 4));
    if (dien_thoai) {
      ung_dung.classList.add('hep_san');
      canh = san.clientWidth;
    } else {
      ung_dung.classList.remove('hep_san');
      var W = san.clientWidth, Hc = san.clientHeight;
      canh = Math.min(Hc, W - 2 * 212 - 24);       // chừa chỗ cho hai cột số liệu hai bên
      if (canh < Math.min(420, Hc)) {              // quá hẹp: sa bàn ở trên, số liệu xuống dưới
        ung_dung.classList.add('hep_san');
        canh = Math.min(san.clientWidth, Hc);
        san.style.overflowY = 'auto';
      } else san.style.overflowY = '';
      $('cot_trai').style.maxHeight = $('cot_phai').style.maxHeight = ung_dung.classList.contains('hep_san') ? '' : canh + 'px';
    }
    canh = Math.max(240, Math.floor(canh));
    datCo(o_sa_ban, canh, canh);
  }

  // ---------- Vòng lặp ----------
  function khungHinh(bay_gio) {
    var troi = Math.min(100, bay_gio - tt.t_truoc);
    tt.t_truoc = bay_gio;
    if (tt.chay) {
      tt.du += troi * tt.toc_do;
      var n = 0;
      while (tt.du >= NHIP && n < 60) { mp.buoc(NHIP); tt.du -= NHIP; n++; }
      if (n === 60) tt.du = 0;
    }
    tt.khung++;
    veSaBan();
    veSoDoPha();
    for (var h = 1; h <= 4; h++) if ((tt.khung + h) % 2 === 0) veCamera(h);
    if (tt.khung % 6 === 0) capNhatSoLieu();
    requestAnimationFrame(khungHinh);
  }

  // ---------- Khởi động ----------
  (function () {
    var day = $('day_camera');
    for (var h = 1; h <= 4; h++) {
      var khoi = document.createElement('div');
      khoi.className = 'camera';
      khoi.innerHTML = '<canvas id="cam_' + h + '" width="320" height="240" aria-label="Camera hướng ' + h + '"></canvas>';
      day.appendChild(khoi);
      o_cam[h] = khoi.querySelector('canvas');
      c_cam[h] = o_cam[h].getContext('2d');
    }
    docLai();
    dungBang();
    dongBoNut();
    for (var i = 0; i < 45000 / NHIP; i++) mp.buoc(NHIP);   // chạy trước 45 giây để mở trang là thấy xe
    xepCho();
    window.addEventListener('resize', xepCho);
    capNhatSoLieu();
    tt.t_truoc = performance.now();
    requestAnimationFrame(khungHinh);
    window.MO_PHONG = mp;   // để gõ thử trong bảng điều khiển của trình duyệt
  })();
})();
