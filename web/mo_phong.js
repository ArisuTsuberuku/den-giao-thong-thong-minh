// mo_phong.js — Mô phỏng xe chạy trên sa bàn Lần 08 (đơn vị mm, tấm 1000 × 1000).
// Không đụng tới trang web nên chạy được cả bằng Node (xem kiem_thu/thu_mo_phong.js).
// Đèn do đúng bộ quyet_dinh.js + dieu_khien.js điều khiển, giống board trung tâm.
// "Camera" ở đây đếm xe có tâm nằm trong vùng chờ của từng làn và gửi {id, seq, s, l, emg, ut}.
(function (goc, tao) {
  if (typeof module === 'object' && module.exports) module.exports = tao(require('./quyet_dinh.js'), require('./dieu_khien.js'));
  else goc.MoPhong = tao(goc.QuyetDinh, goc.DieuKhien);
})(typeof self !== 'undefined' ? self : this, function (QD, DieuKhien) {
  'use strict';

  // ---------- Kích thước sa bàn Lần 08 (theo Decal sa ban/tao_decal_sa_ban.py) ----------
  var C = 500;                 // tâm tấm
  var LAN = 48;                // bề rộng làn
  var MEP = 399;               // mép lòng đường chính / mép ngã tư
  var X_NGOAI = 423;           // tim làn ngoài (chỉ đi thẳng), tính trong khung Hướng 1
  var X_TRONG = 471;           // tim làn trong (đi thẳng hoặc rẽ trái)
  var X_RE_PHAI = 375;         // tim làn rẽ phải
  var TIM_BAO = 32;            // tim làn bao
  var MEP_BAO = 58;            // mép trong làn bao = đầu vùng chờ
  var Y_VACH = 362;            // vạch dừng; vùng chờ dài 304 mm
  var TAM_CUNG = 295;          // tâm cung nhánh rẽ phải (295, 295), bán kính 80
  var R_VAO = 30;              // bán kính rẽ từ làn bao vào nhánh và ngược lại

  var XE_DAI = 68, XE_RONG = 34;   // cỡ xe 100 %: xe Hot Wheels trên sa bàn thật (4 xe mỗi làn chờ)
  var DS = 4;                  // bước lấy mẫu đường đi (mm)
  var NHIN_XA = 150;           // tầm nhìn phía trước của xe (mm)
  var DT_CAMERA = 250;         // camera gửi 4 gói mỗi giây

  // Xe ưu tiên, xếp theo thứ tự ưu tiên "hỏa → sự → công → thương": số nhỏ hơn được đi trước.
  var TEN_UU_TIEN = ['', 'cứu hỏa', 'quân sự', 'công an', 'cứu thương'];
  var MAU_UU_TIEN = ['', '#d8281f', '#4d5f3a', '#f2f4f6', '#ffffff'];

  var MAU_XE = ['#e23d3d', '#f08a24', '#f2c230', '#3d8fe2', '#2bb3a3', '#8d5bd6', '#e85d9a', '#aab4bf',
                '#5a6472', '#2f9e55', '#c0392b', '#1f6fb5'];

  function taoNgauNhien(hat) {
    var a = hat >>> 0;
    return function () {
      a = (a + 0x6D2B79F5) >>> 0;
      var t = a;
      t = Math.imul(t ^ (t >>> 15), t | 1);
      t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
      return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
    };
  }

  // Quay một điểm của khung Hướng 1 sang hướng h (theo chiều kim đồng hồ, quanh tâm tấm).
  function quay(x, y, h) {
    for (var k = 1; k < h; k++) { var x2 = 2 * C - y; y = x; x = x2; }
    return [x, y];
  }
  // Ngược lại: đưa một điểm trên sa bàn về khung Hướng 1.
  function quayNguoc(x, y, h) {
    for (var k = 1; k < h; k++) { var x2 = y; y = 2 * C - x; x = x2; }
    return [x, y];
  }

  // ---------- Dựng đường đi ----------
  function doan(th, x0, y0, x1, y1, he_so) {
    var d = Math.hypot(x1 - x0, y1 - y0), n = Math.max(1, Math.ceil(d / 2));
    for (var i = 0; i <= n; i++) th.push([x0 + (x1 - x0) * i / n, y0 + (y1 - y0) * i / n, he_so]);
  }
  function cung(th, cx, cy, r, a0, a1, he_so) {   // góc tính bằng độ, tăng dần = quay theo chiều kim đồng hồ
    var n = Math.max(2, Math.ceil(Math.abs(a1 - a0) / 3));
    for (var i = 0; i <= n; i++) {
      var a = (a0 + (a1 - a0) * i / n) * Math.PI / 180;
      th.push([cx + r * Math.cos(a), cy + r * Math.sin(a), he_so]);
    }
  }
  function congBezier(th, p0, p1, p2, p3, he_so) {
    for (var i = 0; i <= 40; i++) {
      var t = i / 40, u = 1 - t;
      th.push([u * u * u * p0[0] + 3 * u * u * t * p1[0] + 3 * u * t * t * p2[0] + t * t * t * p3[0],
               u * u * u * p0[1] + 3 * u * u * t * p1[1] + 3 * u * t * t * p2[1] + t * t * t * p3[1], he_so]);
    }
  }
  // Lấy mẫu lại cho đều mỗi DS mm; quay sang hướng h.
  function lamDeu(th, h) {
    var x = [], y = [], f = [], da_di = 0, can = 0;
    for (var i = 0; i + 1 < th.length; i++) {
      var ax = th[i][0], ay = th[i][1], bx = th[i + 1][0], by = th[i + 1][1];
      var d = Math.hypot(bx - ax, by - ay);
      if (d < 1e-9) continue;
      while (can <= da_di + d) {
        var u = (can - da_di) / d;
        var p = quay(ax + (bx - ax) * u, ay + (by - ay) * u, h);
        x.push(p[0]); y.push(p[1]); f.push(Math.min(th[i][2], th[i + 1][2]));
        can += DS;
      }
      da_di += d;
    }
    return { x: x, y: y, f: f, n: x.length, dai: (x.length - 1) * DS };
  }
  function viTri(dd, s, ra) {
    var i = s / DS;
    if (i < 0) i = 0;
    if (i > dd.n - 1.001) i = dd.n - 1.001;
    var k = Math.floor(i), u = i - k;
    ra.x = dd.x[k] + (dd.x[k + 1] - dd.x[k]) * u;
    ra.y = dd.y[k] + (dd.y[k + 1] - dd.y[k]) * u;
    ra.goc = Math.atan2(dd.y[k + 1] - dd.y[k], dd.x[k + 1] - dd.x[k]);
    ra.f = dd.f[k];
    return ra;
  }

  var VAO = 110;       // đoạn chạy trên làn bao trước khi rẽ vào nhánh
  var RA = 115;        // đoạn chạy trên làn bao sau khi ra khỏi nhánh (xe mờ dần rồi biến mất)

  // Các đường đi của Hướng 1 (xe đi từ trên xuống), sau đó quay cho 3 hướng còn lại.
  function duongVao(th, xl) {
    doan(th, xl - R_VAO - VAO, TIM_BAO, xl - R_VAO, TIM_BAO, 1);
    cung(th, xl - R_VAO, TIM_BAO + R_VAO, R_VAO, -90, 0, 0.5);
  }
  function duongThang(xl) {
    var th = [];
    duongVao(th, xl);
    doan(th, xl, TIM_BAO + R_VAO, xl, 1000 - TIM_BAO - R_VAO, 1);
    cung(th, xl - R_VAO, 1000 - TIM_BAO - R_VAO, R_VAO, 0, 90, 0.5);
    doan(th, xl - R_VAO, 1000 - TIM_BAO, xl - R_VAO - RA, 1000 - TIM_BAO, 1);
    return th;
  }
  function duongTrai() {
    var th = [], xl = X_TRONG, ys = 2 * C - X_TRONG;   // ra ở làn trong của nhánh bên phải (y = 529)
    duongVao(th, xl);
    doan(th, xl, TIM_BAO + R_VAO, xl, MEP, 1);
    congBezier(th, [xl, MEP], [xl, MEP + 39], [1000 - MEP - 39, ys], [1000 - MEP, ys], 0.7);
    doan(th, 1000 - MEP, ys, 1000 - TIM_BAO - R_VAO, ys, 1);
    cung(th, 1000 - TIM_BAO - R_VAO, ys + R_VAO, R_VAO, -90, 0, 0.5);
    doan(th, 1000 - TIM_BAO, ys + R_VAO, 1000 - TIM_BAO, ys + R_VAO + RA, 1);
    return th;
  }
  function duongPhai() {
    var th = [];
    duongVao(th, X_NGOAI);
    doan(th, X_NGOAI, TIM_BAO + R_VAO, X_NGOAI, 170, 1);
    congBezier(th, [X_NGOAI, 170], [X_NGOAI, 190], [X_RE_PHAI, 190], [X_RE_PHAI, 210], 0.9);   // vạt mở làn
    doan(th, X_RE_PHAI, 210, X_RE_PHAI, TAM_CUNG, 0.9);
    cung(th, TAM_CUNG, TAM_CUNG, 80, 0, 90, 0.75);
    doan(th, TAM_CUNG, X_RE_PHAI, 252, X_RE_PHAI, 0.9);
    congBezier(th, [252, X_RE_PHAI], [232, X_RE_PHAI], [232, X_NGOAI], [212, X_NGOAI], 0.9);   // vạt nhập làn
    doan(th, 212, X_NGOAI, TIM_BAO + R_VAO, X_NGOAI, 1);
    cung(th, TIM_BAO + R_VAO, X_NGOAI - R_VAO, R_VAO, 90, 180, 0.5);
    doan(th, TIM_BAO, X_NGOAI - R_VAO, TIM_BAO, X_NGOAI - R_VAO - RA, 1);
    return th;
  }

  var S_VAO_XONG = VAO + Math.PI * R_VAO / 2;                    // hết cung rẽ vào, tâm xe ở y = 62
  var S_TACH = S_VAO_XONG + (215 - (TIM_BAO + R_VAO));           // xe rẽ phải đã rời hẳn làn ngoài

  var DUONG = [];   // DUONG[h][tên]
  (function () {
    for (var h = 1; h <= 4; h++) {
      DUONG[h] = {
        thang_ngoai: lamDeu(duongThang(X_NGOAI), h),
        thang_trong: lamDeu(duongThang(X_TRONG), h),
        trai: lamDeu(duongTrai(), h),
        phai: lamDeu(duongPhai(), h)
      };
      for (var ten in DUONG[h]) {
        var dd = DUONG[h][ten];
        dd.s_ra = dd.dai - RA - Math.PI * R_VAO / 2;      // chỗ bắt đầu rẽ ra làn bao
      }
      // chỗ xe rẽ phải dừng nhường trước khi nhập vào làn ra của nhánh bên cạnh: (262, 375) trong khung Hướng 1
      var p = quay(262, X_RE_PHAI, h), d_phai = DUONG[h].phai, gan = 1e9;
      for (var i = 0; i < d_phai.n; i++) {
        var kc = Math.hypot(d_phai.x[i] - p[0], d_phai.y[i] - p[1]);
        if (kc < gan) { gan = kc; d_phai.s_nhap = i * DS; }
      }
    }
  })();

  // Vùng đếm của camera (trong khung Hướng 1): hai hình chữ nhật, mỗi làn một cái.
  var VUNG = {
    ngoai: { x0: MEP, x1: MEP + LAN, y0: MEP_BAO, y1: Y_VACH + 5 },
    trong: { x0: MEP + LAN, x1: MEP + 2 * LAN, y0: MEP_BAO, y1: Y_VACH + 5 }
  };

  function caiDatMacDinh() {
    return {
      luu_luong: [0, 6, 6, 6, 6],      // xe mô hình mỗi phút, theo hướng 1..4
      ti_le_trai: [0, 25, 25, 25, 25], // % xe rẽ trái
      ti_le_phai: 15,                  // % xe rẽ phải (đi làn riêng, không qua đèn)
      toc_do: 120,                     // mm/s
      tre_phan_ung: 1000,              // ms: xe đang đứng yên thấy đi được (đèn xanh, xe trước đã rời) thì chừng này sau mới lăn bánh
      co_xe: 75                        // % so với xe Hot Wheels 68 × 34 mm. 75 % là đúng tỉ lệ ô tô thật so với bề rộng làn
    };
  }

  function thongKeMoi() {
    return { so_xe: 0, tong_cho: 0, cho_max: 0, re_phai: 0, uu_tien: 0, tong_uu_tien: 0 };
  }

  function MoPhong(tuy_chon) {
    tuy_chon = tuy_chon || {};
    this.hat = tuy_chon.hat || 1;
    this.cai = tuy_chon.cai || {};
    var mac_dinh = caiDatMacDinh();
    for (var k in mac_dinh) if (this.cai[k] === undefined) this.cai[k] = mac_dinh[k];
    this.ts = tuy_chon.ts || QD.thamSoMacDinh();
    this.khiSuKien = null;             // hàm(loại, dữ liệu, mô phỏng)
    this.datLai();
  }

  // Mọi kích thước phụ thuộc cỡ xe. Xe được coi như 3 "đốt" tròn xếp dọc thân để xét vướng nhau.
  MoPhong.prototype.datCoXe = function () {
    var k = Math.max(50, Math.min(100, this.cai.co_xe)) / 100;
    this.xe_dai = XE_DAI * k; this.xe_rong = XE_RONG * k;
    this.dot = this.xe_dai / 4;                 // khoảng cách giữa các đốt
    this.r_cham = this.xe_rong + 3;             // hai xe vướng nhau khi hai đốt cách nhau dưới mức này
    this.cach_xe = this.xe_dai + 8.5;           // khoảng cách tâm hai xe khi xếp hàng (hở chừng 8 mm)
    // số xe tối đa mỗi làn trong vùng chờ dài 304 mm: 4 xe ở cỡ 100 %, 5 xe ở 75 %, 6 xe ở 60 %
    this.suc_chua = Math.floor((Y_VACH - MEP_BAO - 2 - this.xe_dai) / this.cach_xe) + 1;
    this.s_dung = S_VAO_XONG + (Y_VACH - 2 - this.xe_dai / 2) - (TIM_BAO + R_VAO);   // tâm xe khi mũi xe sát vạch dừng
  };

  MoPhong.prototype.datLai = function () {
    var toi = this;
    this.datCoXe();
    this.t = 0;
    this.xe = [];
    this.id_ke = 1;
    this.hang_cho = [[], [], [], [], []];   // xe đã tới nhưng chưa có chỗ vào sa bàn
    this.ngau_nhien = taoNgauNhien(this.hat);
    this.ngau_nhien_mau = taoNgauNhien(this.hat + 77);
    this.dem = [null];
    for (var h = 1; h <= 4; h++) this.dem.push({ s: 0, l: 0, emg: 0, ut: 0, seq: 0 });
    this.t_camera = 0;
    this.thong_ke = [thongKeMoi(), thongKeMoi()];   // [cố định, thích ứng]
    this.so_xe_den = 0;
    this.so_lan_chan_nhau = 0;
    this.dk = new DieuKhien(this.ts);
    this.che_do_chay = this.ts.che_do === QD.CHE_DO_CO_DINH ? 0 : 1;   // chế độ của chu kì đang chạy (đổi chế độ chỉ có hiệu lực từ chu kì sau)
    this.dk.khiSuKien = function (loai, dl) {
      if (loai === 'chu_ki') toi.che_do_chay = toi.ts.che_do === QD.CHE_DO_CO_DINH ? 0 : 1;
      if (toi.khiSuKien) toi.khiSuKien(loai, dl, toi);
    };
    this.dk.khoiDong(0);
    this.guiCamera();
  };

  // Số xe đang chiếm chỗ ở một làn của vùng chờ (xe rẽ phải tính vào làn ngoài cho tới khi tách ra).
  MoPhong.prototype.soXeTrongLan = function (h, lan) {
    var n = 0;
    for (var i = 0; i < this.xe.length; i++) {
      var x = this.xe[i];
      if (x.h !== h) continue;
      if (x.huong_di === 'phai') { if (lan === 'ngoai' && x.s < S_TACH) n++; }
      else if (x.lan === lan && !x.da_qua) n++;
    }
    return n;
  };

  // Thả một xe ưu tiên vào hướng h. loai: 1 cứu hỏa, 2 quân sự, 3 công an, 4 cứu thương.
  MoPhong.prototype.thaXeUuTien = function (h, loai) {
    this.hang_cho[h].unshift({ t_sinh: this.t, huong_di: 'thang', uu_tien: loai >= 1 && loai <= 4 ? loai : 4 });
    this.so_xe_den++;
  };
  MoPhong.prototype.thaXeCuuThuong = function (h) { this.thaXeUuTien(h, 4); };

  MoPhong.prototype.sinhXe = function (dt) {
    for (var h = 1; h <= 4; h++) {
      // mỗi nhịp gieo một lần cho mỗi hướng, để hai chế độ đèn nhận cùng một dòng xe
      var r = this.ngau_nhien(), r2 = this.ngau_nhien();
      if (r >= this.cai.luu_luong[h] / 60000 * dt) continue;
      var p_phai = this.cai.ti_le_phai / 100, p_trai = this.cai.ti_le_trai[h] / 100;
      var huong_di = r2 < p_phai ? 'phai' : (r2 < p_phai + (1 - p_phai) * p_trai ? 'trai' : 'thang');
      this.hang_cho[h].push({ t_sinh: this.t, huong_di: huong_di, uu_tien: 0 });
      this.so_xe_den++;
    }
  };

  MoPhong.prototype.thaXeVao = function () {
    for (var h = 1; h <= 4; h++) {
      var q = this.hang_cho[h];
      if (!q.length) continue;
      var dau = q[0], lan, ten;
      var n_ngoai = this.soXeTrongLan(h, 'ngoai'), n_trong = this.soXeTrongLan(h, 'trong');
      if (dau.huong_di === 'trai') { lan = 'trong'; ten = 'trai'; }
      else if (dau.huong_di === 'phai') { lan = 'ngoai'; ten = 'phai'; }
      else if (n_trong < n_ngoai) { lan = 'trong'; ten = 'thang_trong'; }
      else { lan = 'ngoai'; ten = 'thang_ngoai'; }
      if ((lan === 'ngoai' ? n_ngoai : n_trong) >= this.suc_chua) continue;
      var dd = DUONG[h][ten], trong = true;
      for (var i = 0; i < this.xe.length; i++) {
        if (Math.hypot(this.xe[i].x - dd.x[0], this.xe[i].y - dd.y[0]) < this.xe_dai + 27) { trong = false; break; }
      }
      if (!trong) continue;
      q.shift();
      var xe = {
        id: this.id_ke++, h: h, lan: lan, huong_di: dau.huong_di, dd: dd, s: 0, v: this.cai.toc_do * 0.7,
        uu_tien: dau.uu_tien || 0,   // 0: xe thường; 1..4: loại xe ưu tiên
        mau: dau.uu_tien ? MAU_UU_TIEN[dau.uu_tien] : MAU_XE[Math.floor(this.ngau_nhien_mau() * MAU_XE.length)],
        t_sinh: dau.t_sinh, cho: this.t - dau.t_sinh, da_qua: false, di_tiep: false, quyet_vang: false,
        chan: 0, phanh: false, x: dd.x[0], y: dd.y[0], goc: 0, dx: 0, dy: 0,
        he_so_pu: 0.7 + 0.6 * this.ngau_nhien_mau(),   // mỗi người lái nhanh chậm khác nhau: 0,7 đến 1,3 lần mức chung
        t_san_sang: -1,                                // lúc xe đang đứng yên bắt đầu thấy đi được
        nhom: (h - 1) * 2 + (dau.huong_di === 'trai' ? 1 : 0),
        ra: (h - 1 + (dau.huong_di === 'thang' ? 2 : (dau.huong_di === 'trai' ? 1 : 3))) % 4 + 1,   // ra ở nhánh nào
        lan_ra: (ten === 'thang_ngoai' || ten === 'phai') ? 'ngoai' : 'trong'
      };
      this.datViTri(xe);
      this.xe.push(xe);
    }
  };

  var tam = { x: 0, y: 0, goc: 0, f: 1 };
  MoPhong.prototype.datViTri = function (xe) {
    viTri(xe.dd, xe.s, tam);
    xe.x = tam.x; xe.y = tam.y; xe.goc = tam.goc;
    xe.dx = Math.cos(tam.goc); xe.dy = Math.sin(tam.goc);
  };

  // Khoảng trống phía trước xe a (mm) và tốc độ cho phép vì đường cong sắp tới.
  MoPhong.prototype.nhinTruoc = function (a, gan, ket_qua) {
    var dd = a.dd, v_max = this.cai.toc_do, v_cong = v_max, trong = 1e9, chan = 0;
    var dot = this.dot, r2 = this.r_cham * this.r_cham;
    for (var d = 0; d <= NHIN_XA; d += 6) {
      viTri(dd, a.s + dot + d, tam);
      var v_cho_phep = Math.sqrt(Math.pow(tam.f * v_max, 2) + 2 * 200 * d);
      if (v_cho_phep < v_cong) v_cong = v_cho_phep;
      if (chan) continue;
      for (var i = 0; i < gan.length; i++) {
        var b = gan[i];
        for (var k = -1; k <= 1; k++) {
          var ex = b.x + b.dx * dot * k - tam.x, ey = b.y + b.dy * dot * k - tam.y;
          if (ex * ex + ey * ey < r2) { trong = d - 10; chan = b.id; break; }   // dừng cách xe trước chừng 8 mm
        }
        if (chan) break;
      }
    }
    ket_qua.trong = trong; ket_qua.v_cong = v_cong; ket_qua.chan = chan;
  };

  // Xe ở làn ra phía ngoài phải nhường xe ở làn ra phía trong, vì xe làn trong rẽ ra làn bao
  // sẽ cắt ngang đầu nó. Trả về true khi đang có xe làn trong sắp rẽ hoặc đang rẽ ở cùng nhánh.
  MoPhong.prototype.phaiNhuongKhiRa = function (a) {
    for (var i = 0; i < this.xe.length; i++) {
      var b = this.xe[i];
      if (b.ra !== a.ra || b.lan_ra !== 'trong') continue;
      if (b.s > b.dd.s_ra - this.xe_dai - 27 && b.s < b.dd.s_ra + 47 + this.xe_dai + 17) return true;
    }
    return false;
  };

  // Xe rẽ phải nhường xe đang chạy trên làn mà nó sắp nhập vào.
  MoPhong.prototype.phaiNhuongKhiNhap = function (a) {
    for (var i = 0; i < this.xe.length; i++) {
      var b = this.xe[i];
      if (b === a || b.ra !== a.ra || b.lan_ra !== 'ngoai' || b.huong_di === 'phai') continue;
      var p = quayNguoc(b.x, b.y, a.h);          // về khung của hướng xe a: làn nhập là y = 423, xe chạy về phía x giảm
      if (Math.abs(p[1] - X_NGOAI) < 12 && p[0] > 150 && p[0] < 385) return true;
    }
    return false;
  };

  MoPhong.prototype.buoc = function (dt) {
    var i, j, a, dk = this.dk, giay = dt / 1000;
    this.t += dt;
    dk.capNhat(this.t);
    this.sinhXe(dt);
    this.thaXeVao();

    var kq = { trong: 0, v_cong: 0, chan: 0 }, moi = [];
    for (i = 0; i < this.xe.length; i++) {
      a = this.xe[i];
      var gan = [];
      for (j = 0; j < this.xe.length; j++) {
        var b = this.xe[j];
        if (b === a || Math.abs(b.x - a.x) > 260 || Math.abs(b.y - a.y) > 260) continue;
        // hai xe chặn lẫn nhau: xe có số nhỏ hơn được đi trước
        if (a.chan === b.id && b.chan === a.id && a.id < b.id) { this.so_lan_chan_nhau++; continue; }
        gan.push(b);
      }
      this.nhinTruoc(a, gan, kq);
      var trong = kq.trong;
      // đèn: xe đi thẳng theo nhóm thẳng, xe rẽ trái theo nhóm trái; xe rẽ phải không có đèn
      if (a.huong_di !== 'phai' && !a.da_qua) {
        var mau = dk.mauNhom(a.nhom, this.t), toi_vach = this.s_dung - a.s;
        if (mau === DieuKhien.MAU_XANH) { a.di_tiep = false; a.quyet_vang = false; }
        else {
          if (mau === DieuKhien.MAU_VANG && !a.quyet_vang) {      // gặp vàng: dừng nếu kịp, không kịp thì đi tiếp
            a.quyet_vang = true;
            a.di_tiep = toi_vach < a.v * a.v / (2 * 320) - 2;
          }
          if (!a.di_tiep && toi_vach < trong) trong = toi_vach;
        }
      }
      var toi;
      if (a.lan_ra === 'ngoai' && a.s < a.dd.s_ra - 26) {            // chưa tới chỗ rẽ ra làn bao
        toi = a.dd.s_ra - 28 - a.s;
        if (toi < trong && this.phaiNhuongKhiRa(a)) trong = toi;
      }
      if (a.huong_di === 'phai' && a.s < a.dd.s_nhap + 2) {          // chưa nhập làn
        toi = a.dd.s_nhap - a.s;
        if (toi < trong && this.phaiNhuongKhiNhap(a)) trong = toi;
      }
      var v_muc_tieu = trong > 0 ? Math.sqrt(2 * 200 * trong) : 0;
      if (v_muc_tieu > kq.v_cong) v_muc_tieu = kq.v_cong;
      // Trễ phản ứng của người lái: xe đang đứng yên trong hàng chờ chỉ lăn bánh sau khi đã thấy
      // đi được (đèn xanh, hoặc xe trước đã rời ra hơn 6 mm) đủ lâu. Nhờ vậy hàng xe khởi hành
      // lần lượt từ đầu hàng về cuối hàng như ngoài đường, không đồng loạt. Xe đã qua vạch dừng
      // thì không tính trễ nữa, để xe thoát khỏi lòng ngã tư nhanh như cũ.
      if (a.da_qua) a.t_san_sang = -1;
      else if (a.v < 1) {
        if (trong > 6) {
          if (a.t_san_sang < 0) a.t_san_sang = this.t;
          if (this.t - a.t_san_sang < this.cai.tre_phan_ung * a.he_so_pu) v_muc_tieu = 0;
        } else { a.t_san_sang = -1; v_muc_tieu = 0; }
      } else a.t_san_sang = -1;
      var v = a.v;
      if (v_muc_tieu >= v) v = Math.min(v + 140 * giay, v_muc_tieu);
      else v = Math.max(v - 500 * giay, v_muc_tieu);
      var ds = v * giay;
      if (ds > trong) ds = trong > 0 ? trong : 0;
      moi.push(v, ds, kq.chan, v < a.v - 1 || v < 3 ? 1 : 0);
    }
    var con_lai = [], che_do = this.che_do_chay;
    for (i = 0; i < this.xe.length; i++) {
      a = this.xe[i];
      a.v = moi[i * 4]; a.s += moi[i * 4 + 1]; a.chan = moi[i * 4 + 2]; a.phanh = moi[i * 4 + 3] === 1;
      this.datViTri(a);
      if (!a.da_qua && a.v < 15) a.cho += dt;
      if (!a.da_qua && (a.huong_di === 'phai' ? a.s > S_TACH : a.s > this.s_dung + 4)) {
        a.da_qua = true;
        var tk = this.thong_ke[che_do];
        if (a.huong_di === 'phai') tk.re_phai++;
        else {
          tk.so_xe++; tk.tong_cho += a.cho;
          if (a.cho > tk.cho_max) tk.cho_max = a.cho;
          if (a.uu_tien) {
            tk.uu_tien++; tk.tong_uu_tien += this.t - a.t_sinh;
            if (this.khiSuKien) this.khiSuKien('uu_tien_qua', { h: a.h, ms: this.t - a.t_sinh, loai: a.uu_tien }, this);
          }
        }
      }
      if (a.s < a.dd.dai - 1) con_lai.push(a);
    }
    this.xe = con_lai;

    if (this.t - this.t_camera >= DT_CAMERA) this.guiCamera();
  };

  // "Camera" của 4 hướng: đếm xe theo làn rồi gửi cho bộ điều khiển.
  MoPhong.prototype.guiCamera = function () {
    this.t_camera = this.t;
    for (var h = 1; h <= 4; h++) {
      var d = this.dem[h], s = 0, l = 0, emg = 0, ut = 0;
      for (var i = 0; i < this.xe.length; i++) {
        var lan = MoPhong.lanCuaDiem(this.xe[i].x, this.xe[i].y, h);
        if (!lan) continue;
        if (lan === 'ngoai') s++; else l++;
        var loai = this.xe[i].uu_tien;                   // có nhiều xe ưu tiên thì báo xe hạng cao nhất
        if (loai && (ut === 0 || loai < ut)) { ut = loai; emg = lan === 'ngoai' ? 1 : 2; }
      }
      d.s = s; d.l = l; d.emg = emg; d.ut = ut; d.seq++;
      this.dk.nhanDem(h, s, l, emg, this.t, false, ut);
    }
  };

  // Điểm (x, y) trên sa bàn thuộc làn nào trong vùng đếm của camera hướng h? ('ngoai', 'trong' hoặc null)
  MoPhong.lanCuaDiem = function (x, y, h) {
    var p = quayNguoc(x, y, h);
    if (p[1] < VUNG.ngoai.y0 || p[1] > VUNG.ngoai.y1) return null;
    if (p[0] >= VUNG.ngoai.x0 && p[0] < VUNG.ngoai.x1) return 'ngoai';
    if (p[0] >= VUNG.trong.x0 && p[0] < VUNG.trong.x1) return 'trong';
    return null;
  };

  MoPhong.prototype.jsonCamera = function (h) {
    var d = this.dem[h];
    return '{"id":' + h + ',"seq":' + d.seq + ',"s":' + d.s + ',"l":' + d.l + ',"emg":' + d.emg + ',"ut":' + d.ut + '}';
  };

  // Tổng hợp: thời gian chờ trung bình tính trên MỌI xe đã tới (kể cả xe còn kẹt), để chế độ nào
  // bỏ đói một hướng thì không được lợi.
  MoPhong.prototype.tongHop = function () {
    var da_qua = 0, tong = 0, cho_max = 0, i, h;
    for (i = 0; i < 2; i++) { da_qua += this.thong_ke[i].so_xe; tong += this.thong_ke[i].tong_cho; cho_max = Math.max(cho_max, this.thong_ke[i].cho_max); }
    var con_ket = 0;
    for (i = 0; i < this.xe.length; i++) {
      var x = this.xe[i];
      if (x.da_qua || x.huong_di === 'phai') continue;
      con_ket++; tong += x.cho; cho_max = Math.max(cho_max, x.cho);
    }
    for (h = 1; h <= 4; h++) {
      for (i = 0; i < this.hang_cho[h].length; i++) {
        if (this.hang_cho[h][i].huong_di === 'phai') continue;
        var c = this.t - this.hang_cho[h][i].t_sinh;
        con_ket++; tong += c; cho_max = Math.max(cho_max, c);
      }
    }
    var n = da_qua + con_ket;
    return { da_qua: da_qua, con_ket: con_ket, cho_tb: n ? tong / n / 1000 : 0, cho_max: cho_max / 1000, phut: this.t / 60000 };
  };

  // Chạy nhanh không vẽ: cùng một dòng xe cho hai chế độ đèn, trả về số liệu để so sánh.
  // chi_che_do (0 hoặc 1, không bắt buộc): chỉ chạy chế độ đó, ô của chế độ kia để null.
  MoPhong.soSanh = function (cai, ts, phut, hat, chi_che_do) {
    var ket_qua = [];
    for (var che_do = 0; che_do <= 1; che_do++) {
      if (chi_che_do === 0 || chi_che_do === 1) { if (che_do !== chi_che_do) { ket_qua.push(null); continue; } }
      var ts2 = {}, k;
      for (k in ts) ts2[k] = ts[k];
      ts2.che_do = che_do;
      var mp = new MoPhong({ hat: hat || 1, ts: ts2, cai: JSON.parse(JSON.stringify(cai)) });
      var so_buoc = Math.round(phut * 60000 / 40);
      for (var i = 0; i < so_buoc; i++) mp.buoc(40);
      ket_qua.push(mp.tongHop());
    }
    return ket_qua;
  };

  MoPhong.caiDatMacDinh = caiDatMacDinh;
  MoPhong.quay = quay;
  MoPhong.quayNguoc = quayNguoc;
  MoPhong.HINH = {
    C: C, LAN: LAN, MEP: MEP, X_NGOAI: X_NGOAI, X_TRONG: X_TRONG, Y_VACH: Y_VACH, MEP_BAO: MEP_BAO,
    XE_DAI: XE_DAI, XE_RONG: XE_RONG, VUNG: VUNG, DUONG: DUONG
  };
  MoPhong.TEN_UU_TIEN = TEN_UU_TIEN;
  return MoPhong;
});
