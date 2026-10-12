// may_anh.js — Mô hình camera ESP32-S3-CAM (cảm biến OV2640) gắn trên khung đèn, dùng cho mô phỏng.
// Làm đúng các bước mà board thật làm, theo đúng thứ tự:
//   1. Chụp một khung 320 × 240: phép chiếu phối cảnh từ vị trí ống kính trên khung đèn.
//   2. Cắt ROI (hình chữ nhật chứa 2 làn chờ) rồi co về ảnh vuông đưa vào mô hình (96 × 96, kiểu Squash).
//   3. Mô hình kiểu FOMO chia ảnh thành ô 8 × 8 điểm ảnh, ô nào chứa tâm một xe thì "sáng"; các ô sáng
//      cùng nhãn nằm sát nhau bị gộp thành MỘT vật (đúng cách thư viện Edge Impulse gộp), nên hai xe
//      đứng quá gần nhau trên ảnh sẽ bị đếm thành một.
//   4. Tâm từng vật được đổi về toạ độ khung 320 × 240, rồi gán làn bằng 2 tứ giác (như lan_duong.h).
//   5. Lọc nhiễu qua vài khung gần nhất (như BoLoc trong lan_duong.h).
// Phần hình học (góc nhìn, méo phối cảnh, xe bị cắt ở mép khung, xe trước che xe sau, gộp ô) là tính
// thật. Phần "mô hình nhận đúng bao nhiêu phần trăm" là giả định đặt bằng tham số, vì chưa có mô hình
// huấn luyện thật để đo.
// Không đụng tới trang web nên chạy được bằng Node.
(function (goc, tao) {
  if (typeof module === 'object' && module.exports) module.exports = tao();
  else goc.MayAnh = tao();
})(typeof self !== 'undefined' ? self : this, function () {
  'use strict';

  var RAD = Math.PI / 180;

  function thamSoMacDinh() {
    return {
      kieu: 1,             // 0: camera lí tưởng (đếm đúng tuyệt đối, 4 lần mỗi giây); 1: ESP32-S3-CAM
      rong: 320, cao: 240, // khung QVGA
      goc_cheo: 66,        // góc nhìn theo đường chéo của ống kính thường đi kèm OV2640 (độ)
      cao_mm: 285,         // độ cao ống kính so với mặt sa bàn (khung đèn Lần 08)
      nghieng: 26,         // trục ống kính lệch khỏi phương thẳng đứng, chúc về phía xe đang tới (độ)
      lui_mm: 14,          // ống kính nằm trước vạch dừng chừng này, tính về phía xe đang tới
      xoay: 1,             // 1: cạnh dài của khung nằm dọc theo đường; 0: cạnh dài nằm ngang đường
      dau_vao: 96,         // cạnh ảnh đưa vào mô hình
      o: 8,                // FOMO: mỗi ô 8 × 8 điểm ảnh, ảnh 96 × 96 thành lưới 12 × 12
      khung_giay: 1,       // số khung xử lí được mỗi giây (chụp + cắt + chạy mô hình)
      so_khung_loc: 3,     // lọc nhiễu qua bấy nhiêu khung gần nhất (1, 3 hoặc 5)
      nhan_dung: 95,       // giả định: mô hình nhận ra một xe nhìn thấy trọn vẹn với xác suất này (%)
      gui_ms: 250          // sau mỗi khung, gửi kết quả qua ESP-NOW nếu đã quá chừng này kể từ lần gửi trước (CHU_KI_GUI_MS)
    };
  }

  // ---------- Hình học ----------
  // hinh: { MEP, LAN, Y_VACH, MEP_BAO, quay(x, y, h), quayNguoc(x, y, h) } lấy từ mo_phong.js.
  // Mọi toạ độ sa bàn tính bằng mm; z là độ cao so với mặt sa bàn.
  function tao(h, ts, hinh) {
    var cam = { h: h, ts: ts, hinh: hinh };
    var W = ts.rong, H = ts.cao;
    cam.f = Math.hypot(W, H) / 2 / Math.tan(ts.goc_cheo / 2 * RAD);      // tiêu cự tính bằng điểm ảnh
    // Trong khung Hướng 1: xe đi theo chiều +y, vạch dừng ở y = Y_VACH, ống kính nhìn ngược về phía -y.
    var x0 = hinh.MEP + hinh.LAN, y0 = hinh.Y_VACH - ts.lui_mm;
    cam.vi_tri = [x0, y0, ts.cao_mm];
    var g0 = hinh.quay(x0, y0, h);
    cam.goc_x = g0[0]; cam.goc_y = g0[1];     // chân ống kính trên mặt sa bàn
    var s = Math.sin(ts.nghieng * RAD), c = Math.cos(ts.nghieng * RAD);
    var truc = [0, -s, -c];            // trục ống kính
    var len = [0, -c, s];              // hướng "phía xa" trên ảnh khi cạnh dài nằm ngang đường
    var phai = [1, 0, 0];              // bên phải ảnh: phía dải phân cách (làn trong)
    if (ts.xoay) {                     // xoay board 90° quanh trục ống kính: vạch dừng ở mép phải ảnh, làn trong ở phía trên
      cam.phai = [-len[0], -len[1], -len[2]]; cam.len = phai;
    } else { cam.phai = phai; cam.len = len; }
    cam.truc = truc;

    // điểm (x, y, z) của sa bàn → điểm ảnh (u, v); sau = true nếu điểm nằm sau lưng camera
    cam.chieu = function (x, y, z) {
      var p = hinh.quayNguoc(x, y, h);
      var dx = p[0] - cam.vi_tri[0], dy = p[1] - cam.vi_tri[1], dz = (z || 0) - cam.vi_tri[2];
      var sau = dx * cam.truc[0] + dy * cam.truc[1] + dz * cam.truc[2];
      if (sau <= 1e-6) return { u: 0, v: 0, sau: true };
      return {
        u: W / 2 + cam.f * (dx * cam.phai[0] + dy * cam.phai[1] + dz * cam.phai[2]) / sau,
        v: H / 2 - cam.f * (dx * cam.len[0] + dy * cam.len[1] + dz * cam.len[2]) / sau,
        sau: false
      };
    };
    // điểm ảnh (u, v) → điểm trên mặt sa bàn (z = 0) mà tia nhìn đi tới; null nếu tia không chạm mặt sa bàn
    cam.nguoc = function (u, v) {
      var a = (u - W / 2) / cam.f, b = -(v - H / 2) / cam.f;
      var tx = cam.truc[0] + a * cam.phai[0] + b * cam.len[0];
      var ty = cam.truc[1] + a * cam.phai[1] + b * cam.len[1];
      var tz = cam.truc[2] + a * cam.phai[2] + b * cam.len[2];
      if (tz >= -1e-6) return null;
      var k = -cam.vi_tri[2] / tz;
      return hinh.quay(cam.vi_tri[0] + k * tx, cam.vi_tri[1] + k * ty, h);
    };

    // Hai tứ giác làn trên ảnh: 4 góc của từng làn chờ trên mặt đường (đây là 4 điểm mà người dùng bấm
    // trên trang thu ảnh của camera). Thứ tự góc: xa-ngoài, xa-trong, gần-trong, gần-ngoài.
    function tuGiac(xa, xb) {
      var y_xa = hinh.MEP_BAO, y_gan = hinh.Y_VACH + 5, q = [];
      [[xa, y_xa], [xb, y_xa], [xb, y_gan], [xa, y_gan]].forEach(function (g) {
        var p = hinh.quay(g[0], g[1], h), a = cam.chieu(p[0], p[1], 0);
        q.push({ x: Math.round(a.u), y: Math.round(a.v) });
      });
      return q;
    }
    cam.lan_ngoai = tuGiac(hinh.MEP, hinh.MEP + hinh.LAN);
    cam.lan_trong = tuGiac(hinh.MEP + hinh.LAN, hinh.MEP + 2 * hinh.LAN);

    // ROI: hình chữ nhật bao hai tứ giác, nới thêm cho thân xe nhô khỏi mép làn, cắt gọn trong khung.
    var u0 = 1e9, u1 = -1e9, v0 = 1e9, v1 = -1e9;
    cam.lan_ngoai.concat(cam.lan_trong).forEach(function (g) {
      u0 = Math.min(u0, g.x); u1 = Math.max(u1, g.x); v0 = Math.min(v0, g.y); v1 = Math.max(v1, g.y);
    });
    var NOI = 8;
    u0 = Math.max(0, Math.floor(u0 - NOI)); v0 = Math.max(0, Math.floor(v0 - NOI));
    u1 = Math.min(W, Math.ceil(u1 + NOI)); v1 = Math.min(H, Math.ceil(v1 + NOI));
    cam.roi = { x: u0, y: v0, w: Math.max(8, u1 - u0), h: Math.max(8, v1 - v0) };
    return cam;
  }

  // 8 đỉnh của hộp xe (tâm x, y; hướng goc; dài, rộng, cao) chiếu lên ảnh.
  // Thứ tự: 4 đỉnh đáy (trước-trái, trước-phải, sau-phải, sau-trái) rồi 4 đỉnh nóc tương ứng.
  function chieuXe(cam, xe) {
    var cs = Math.cos(xe.goc), sn = Math.sin(xe.goc), d = xe.dai / 2, r = xe.rong / 2, kq = [], sau = false;
    var goc = [[d, -r], [d, r], [-d, r], [-d, -r]];
    for (var tang = 0; tang < 2; tang++) {
      for (var i = 0; i < 4; i++) {
        var a = cam.chieu(xe.x + goc[i][0] * cs - goc[i][1] * sn, xe.y + goc[i][0] * sn + goc[i][1] * cs, tang ? xe.cao : 0);
        if (a.sau) sau = true;
        kq.push(a);
      }
    }
    return sau ? null : kq;
  }

  // ---------- Nhận diện kiểu FOMO ----------
  // ds_xe: [{ x, y, goc, dai, rong, cao, loai }] (loai: 0 xe thường, 1..4 xe ưu tiên).
  // ngau_nhien: hàm trả số trong [0, 1).
  // Trả về { vat: [{ x, y, loai }], o: [...] } với x, y là tâm vật trên khung 320 × 240 (như nhan_dien.h trả về),
  // o là danh sách ô sáng trên lưới (để vẽ).
  function nhanDien(cam, ds_xe, ngau_nhien) {
    var ts = cam.ts, roi = cam.roi, N = ts.dau_vao, O = ts.o, luoi = Math.floor(N / O);
    var p_dung = Math.max(0, Math.min(100, ts.nhan_dung)) / 100;
    var ung = [], i, j;
    for (i = 0; i < ds_xe.length; i++) {
      var xe = ds_xe[i], g = chieuXe(cam, xe);
      if (!g) continue;
      var u0 = 1e9, u1 = -1e9, v0 = 1e9, v1 = -1e9;
      for (j = 0; j < 8; j++) { u0 = Math.min(u0, g[j].u); u1 = Math.max(u1, g[j].u); v0 = Math.min(v0, g[j].v); v1 = Math.max(v1, g[j].v); }
      var dt = (u1 - u0) * (v1 - v0);
      if (dt <= 0) continue;
      // phần của xe nằm trong ROI
      var a0 = Math.max(u0, roi.x), a1 = Math.min(u1, roi.x + roi.w), b0 = Math.max(v0, roi.y), b1 = Math.min(v1, roi.y + roi.h);
      if (a1 <= a0 || b1 <= b0) continue;
      var thay = (a1 - a0) * (b1 - b0) / dt;                 // tỉ lệ thân xe còn nằm trong ROI
      ung.push({ xe: xe, u: (a0 + a1) / 2, v: (b0 + b1) / 2, rong: a1 - a0, cao: b1 - b0, thay: thay,
                 xa: Math.hypot(xe.x - cam.goc_x, xe.y - cam.goc_y) });
    }
    // xe gần camera hơn che bớt xe ở xa hơn: tính phần hộp bao của xe xa bị hộp bao xe gần đè lên
    for (i = 0; i < ung.length; i++) {
      var che = 0;
      for (j = 0; j < ung.length; j++) {
        if (i === j || ung[j].xa >= ung[i].xa) continue;
        var w = Math.min(ung[i].u + ung[i].rong / 2, ung[j].u + ung[j].rong / 2) - Math.max(ung[i].u - ung[i].rong / 2, ung[j].u - ung[j].rong / 2);
        var hh = Math.min(ung[i].v + ung[i].cao / 2, ung[j].v + ung[j].cao / 2) - Math.max(ung[i].v - ung[i].cao / 2, ung[j].v - ung[j].cao / 2);
        if (w > 0 && hh > 0) che += w * hh / (ung[i].rong * ung[i].cao);
      }
      ung[i].che = Math.min(1, che);
    }
    // từng xe: có được nhận ra không, sáng ô nào
    var o_sang = [];      // { cx, cy, loai }
    for (i = 0; i < ung.length; i++) {
      var q = ung[i];
      // xác suất nhận ra: giảm theo phần thân bị cắt khỏi ROI và phần bị xe khác che
      var p = p_dung * Math.min(1, q.thay / 0.8) * (1 - 0.6 * q.che);
      if (q.thay < 0.35) p = 0;
      var r = ngau_nhien(), r2 = ngau_nhien(), r3 = ngau_nhien();
      if (r >= p) continue;
      var loai = q.xe.loai || 0;
      // nhầm loại xe ưu tiên: thành loại ưu tiên khác hoặc thành xe thường
      if (loai && r2 < (1 - p_dung) * 2) loai = r3 < 0.5 ? 0 : 1 + Math.floor(r3 * 8) % 4;
      // hiếm hơn nhiều: nhìn một xe thường thành xe ưu tiên (với nhan_dung 95 % là 1 lần trong 1000 lượt nhìn)
      else if (!loai && r2 < (1 - p_dung) * 0.02) loai = 1 + Math.floor(r3 * 4) % 4;
      // tâm trên ảnh đầu vào mô hình (ROI co về N × N), rồi ô chứa tâm
      var mx = (q.u - roi.x) * N / roi.w, my = (q.v - roi.y) * N / roi.h;
      var cx = Math.max(0, Math.min(luoi - 1, Math.floor(mx / O))), cy = Math.max(0, Math.min(luoi - 1, Math.floor(my / O)));
      o_sang.push({ cx: cx, cy: cy, loai: loai });
      // xe chiếm từ 3 ô trở lên theo một chiều thì đôi khi làm sáng thêm ô kề bên theo chiều đó
      var dai_o = q.rong * N / roi.w / O, cao_o = q.cao * N / roi.h / O;
      if (dai_o >= 3 && r2 > 0.6) o_sang.push({ cx: Math.max(0, Math.min(luoi - 1, cx + (mx / O - cx > 0.5 ? 1 : -1))), cy: cy, loai: loai });
      else if (cao_o >= 3 && r2 > 0.6) o_sang.push({ cx: cx, cy: Math.max(0, Math.min(luoi - 1, cy + (my / O - cy > 0.5 ? 1 : -1))), loai: loai });
    }
    // nhận nhầm: hiếm khi một ô trống trong vùng hai làn sáng lên
    if (ngau_nhien() < (1 - p_dung) * 0.2) {
      o_sang.push({ cx: Math.floor(ngau_nhien() * luoi), cy: Math.floor(ngau_nhien() * luoi), loai: 0 });
    }
    // Gộp ô như thư viện Edge Impulse (ei_handle_cube): duyệt lưới theo hàng, ô sáng nào chạm (kể cả
    // chéo góc) một khối cùng nhãn đã có thì nhập vào khối đó.
    o_sang.sort(function (a, b) { return a.cy - b.cy || a.cx - b.cx; });
    var khoi = [];
    for (i = 0; i < o_sang.length; i++) {
      var c = o_sang[i], co = false;
      for (j = 0; j < khoi.length && !co; j++) {
        var k = khoi[j];
        if (k.loai !== c.loai) continue;
        if (!(k.x + k.w < c.cx || k.y + k.h < c.cy || k.x > c.cx + 1 || k.y > c.cy + 1)) {
          var x1 = Math.max(k.x + k.w, c.cx + 1), y1 = Math.max(k.y + k.h, c.cy + 1);
          k.x = Math.min(k.x, c.cx); k.y = Math.min(k.y, c.cy); k.w = x1 - k.x; k.h = y1 - k.y;
          co = true;
        }
      }
      if (!co) khoi.push({ x: c.cx, y: c.cy, w: 1, h: 1, loai: c.loai });
    }
    // tâm khối → toạ độ khung 320 × 240 (như nhan_dien.h)
    var vat = [];
    for (i = 0; i < khoi.length; i++) {
      vat.push({
        x: roi.x + (khoi[i].x + khoi[i].w / 2) * O * roi.w / N,
        y: roi.y + (khoi[i].y + khoi[i].h / 2) * O * roi.h / N,
        loai: khoi[i].loai, khoi: khoi[i]
      });
    }
    return { vat: vat, o: o_sang };
  }

  // ---------- Gán làn và lọc nhiễu: bản JavaScript của firmware/camera/lan_duong.h ----------
  function trongTuGiac(q, x, y) {
    var trong = false;
    for (var i = 0, j = 3; i < 4; j = i++) {
      var xi = q[i].x, yi = q[i].y, xj = q[j].x, yj = q[j].y;
      if (((yi > y) !== (yj > y)) && (x < (xj - xi) * (y - yi) / (yj - yi) + xi)) trong = !trong;
    }
    return trong;
  }

  // vat: [{ x, y, loai }]; trả về { s, l, emg, ut }
  function phanLan(vat, lan_ngoai, lan_trong) {
    var kq = { s: 0, l: 0, emg: 0, ut: 0 };
    for (var i = 0; i < vat.length; i++) {
      var lan = 0;
      if (trongTuGiac(lan_ngoai, vat[i].x, vat[i].y)) { kq.s++; lan = 1; }
      else if (trongTuGiac(lan_trong, vat[i].x, vat[i].y)) { kq.l++; lan = 2; }
      var loai = vat[i].loai >= 0 && vat[i].loai <= 4 ? vat[i].loai : 0;
      if (lan && loai && (kq.ut === 0 || loai < kq.ut)) { kq.ut = loai; kq.emg = lan; }
    }
    return kq;
  }

  // Lọc qua n khung gần nhất (n = 1, 3 hoặc 5): s và l lấy trung vị; emg bật khi quá nửa số khung thấy
  // xe ưu tiên; loại xe là loại xuất hiện ở nhiều khung nhất.
  function BoLoc(n) {
    this.n = n >= 5 ? 5 : (n >= 3 ? 3 : 1);
    this.khung = []; this.so_khung = 0; this.vi_tri = 0;
  }
  BoLoc.prototype.them = function (k) {
    this.khung[this.vi_tri] = { s: k.s, l: k.l, emg: k.emg, ut: k.ut };
    this.vi_tri = (this.vi_tri + 1) % this.n;
    if (this.so_khung < this.n) this.so_khung++;
  };
  BoLoc.prototype.layKetQua = function () {
    var kq = { s: 0, l: 0, emg: 0, ut: 0 };
    if (this.so_khung === 0) return kq;
    var a = [], b = [], n1 = 0, n2 = 0, so_loai = [0, 0, 0, 0, 0], i;
    for (i = 0; i < this.so_khung; i++) {
      var k = this.khung[i];
      a.push(k.s); b.push(k.l);
      if (k.emg === 1) n1++;
      if (k.emg === 2) n2++;
      if (k.emg !== 0) so_loai[(k.ut >= 1 && k.ut <= 4) ? k.ut : 4]++;
    }
    var giua = function (m) { m.sort(function (x, y) { return x - y; }); return m[Math.floor(m.length / 2)]; };
    kq.s = giua(a); kq.l = giua(b);
    if (n1 + n2 >= Math.floor(this.n / 2) + 1) {
      kq.emg = n2 > n1 ? 2 : 1;
      kq.ut = 1;
      for (i = 2; i <= 4; i++) if (so_loai[i] > so_loai[kq.ut]) kq.ut = i;
    }
    return kq;
  };

  return {
    thamSoMacDinh: thamSoMacDinh, tao: tao, chieuXe: chieuXe, nhanDien: nhanDien,
    trongTuGiac: trongTuGiac, phanLan: phanLan, BoLoc: BoLoc
  };
});
