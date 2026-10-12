// thu_may_anh.js — Kiểm tra mô hình camera ESP32-S3-CAM của mô phỏng (web/may_anh.js).
//   node kiem_thu/thu_may_anh.js
// Kiểm phần hình học và phần gộp ô (những thứ tính được chắc chắn), và kiểm rằng cấu hình ban đầu
// trong firmware/camera/cau_hinh.h đúng là cấu hình mà mô hình này tính ra.
// KHÔNG kiểm được độ chính xác của mô hình nhận diện thật, vì chưa có mô hình huấn luyện thật.
'use strict';
const fs = require('fs');
const path = require('path');
const MayAnh = require('../web/may_anh.js');
const MoPhong = require('../web/mo_phong.js');

const H = MoPhong.HINH, HC = MoPhong.HINH_CAM;
let loi = 0;
function kiem(dung, ten, them) {
  if (!dung) { loi++; console.log('  SAI: ' + ten + (them ? ' — ' + them : '')); }
}
const ts0 = () => MayAnh.thamSoMacDinh();
const chac = () => 0;        // "ngẫu nhiên" luôn trả 0: xe nào nhìn thấy đủ thì chắc chắn được nhận ra
const xeTai = (x, y, h, k, loai) => {      // xe đứng trong khung Hướng 1 tại (x, y), đầu quay về vạch dừng
  const p = MoPhong.quay(x, y, h);
  return { x: p[0], y: p[1], goc: Math.PI / 2 * h, dai: H.XE_DAI * k, rong: H.XE_RONG * k, cao: H.XE_CAO * k, loai: loai || 0 };
};

// ---------- 1. Phép chiếu ----------
console.log('1. Phép chiếu phối cảnh');
for (const xoay of [1, 0]) {
  for (let h = 1; h <= 4; h++) {
    const ts = ts0(); ts.xoay = xoay;
    const cam = MayAnh.tao(h, ts, HC);
    // chiếu một điểm trên mặt sa bàn lên ảnh rồi lần ngược lại phải về đúng điểm cũ
    let lech = 0;
    for (let y = 80; y <= 340; y += 20) for (let x = 405; x <= 490; x += 17) {
      const p = MoPhong.quay(x, y, h), a = cam.chieu(p[0], p[1], 0), b = cam.nguoc(a.u, a.v);
      lech = Math.max(lech, Math.hypot(b[0] - p[0], b[1] - p[1]));
    }
    kiem(lech < 1e-6, 'chiếu rồi lần ngược lệch ' + lech + ' mm (hướng ' + h + ', xoay ' + xoay + ')');
    // 4 camera là cùng một hình quay đi: tứ giác làn và ROI phải y hệt nhau
    const c1 = MayAnh.tao(1, ts, HC);
    kiem(JSON.stringify([cam.lan_ngoai, cam.lan_trong, cam.roi]) === JSON.stringify([c1.lan_ngoai, c1.lan_trong, c1.roi]), 'camera hướng ' + h + ' khác camera hướng 1');
  }
}
{
  const cam = MayAnh.tao(1, ts0(), HC);
  // điểm ngay dưới trục ống kính rơi vào giữa khung
  const d = Math.tan(26 * Math.PI / 180) * 285, a = cam.chieu(HC.MEP + HC.LAN, HC.Y_VACH - 14 - d, 0);
  kiem(Math.abs(a.u - 160) < 1e-6 && Math.abs(a.v - 120) < 1e-6, 'điểm trên trục ống kính không rơi vào giữa khung', JSON.stringify(a));
  kiem(Math.abs(cam.f - 308) < 0.5, 'tiêu cự tính ra ' + cam.f.toFixed(1) + ' px, phải gần 308');
  // vật cao hơn mặt đường thì gần ống kính hơn nên to hơn trên ảnh
  const g = MayAnh.chieuXe(cam, xeTai(H.X_NGOAI, 200, 1, 1));
  const day = Math.hypot(g[0].u - g[2].u, g[0].v - g[2].v), noc = Math.hypot(g[4].u - g[6].u, g[4].v - g[6].v);
  kiem(noc > day * 1.05, 'nóc xe phải to hơn đáy xe trên ảnh', day.toFixed(1) + ' / ' + noc.toFixed(1));
}

// ---------- 2. Khung hình trùm được bao nhiêu phần vùng chờ ----------
console.log('2. Tầm phủ của khung hình');
function tamPhu(xoay) {
  const ts = ts0(); ts.xoay = xoay;
  const cam = MayAnh.tao(1, ts, HC);
  // dọc theo vạch chia hai làn: đoạn y nào nằm trong khung
  let y0 = null, y1 = null;
  for (let y = -100; y <= 420; y += 0.5) {
    let trong = true;
    for (const x of [HC.MEP, HC.MEP + 2 * HC.LAN]) { const a = cam.chieu(x, y, 0); if (a.sau || a.u < 0 || a.u > 320 || a.v < 0 || a.v > 240) trong = false; }
    if (trong) { if (y0 === null) y0 = y; y1 = y; }
  }
  return { y0, y1, cam };
}
const doc = tamPhu(1), ngang = tamPhu(0);
const ts_chac = ts0(); ts_chac.nhan_dung = 100;          // mô hình không bao giờ nhìn sót hay nhìn nhầm
const cam_chac = MayAnh.tao(1, ts_chac, HC);
console.log('  Cạnh dài dọc theo đường: thấy trọn bề rộng hai làn từ y = ' + doc.y0 + ' tới ' + doc.y1 + ' mm (vùng chờ: ' + HC.MEP_BAO + ' tới ' + HC.Y_VACH + ')');
console.log('  Cạnh dài ngang đường:    thấy trọn bề rộng hai làn từ y = ' + ngang.y0 + ' tới ' + ngang.y1 + ' mm');
kiem(doc.y0 <= HC.MEP_BAO && doc.y1 >= HC.Y_VACH - 10, 'gắn cạnh dài dọc đường phải thấy gần hết vùng chờ');
kiem(ngang.y1 < HC.Y_VACH - 30, 'gắn cạnh dài ngang đường thì không thấy tới vạch dừng (nếu ca này sai tức là hình học đã đổi, xem lại hướng dẫn gắn camera)');
kiem(doc.cam.roi.x >= 0 && doc.cam.roi.y >= 0 && doc.cam.roi.x + doc.cam.roi.w <= 320 && doc.cam.roi.y + doc.cam.roi.h <= 240, 'ROI vượt khỏi khung');

// ---------- 3. Cấu hình ban đầu của firmware camera phải khớp với mô hình này ----------
console.log('3. Cấu hình ban đầu trong firmware/camera/cau_hinh.h');
{
  const tep = fs.readFileSync(path.join(__dirname, '..', 'firmware', 'camera', 'cau_hinh.h'), 'utf8');
  const cam = doc.cam;
  const mong = [cam.roi.x, cam.roi.y, cam.roi.w, cam.roi.h];
  cam.lan_ngoai.concat(cam.lan_trong).forEach(g => mong.push(g.x, g.y));
  const dong = tep.split('\n').filter(d => /\/\/ CAM [1-4]\s*$/.test(d));
  kiem(dong.length === 4, 'không tìm thấy 4 dòng cấu hình CAM 1..4');
  dong.forEach((d, i) => {
    const so = (d.split('//')[0].match(/-?\d+/g) || []).map(Number);
    kiem(JSON.stringify(so) === JSON.stringify(mong), 'CAM ' + (i + 1) + ' trong cau_hinh.h khác mô hình', 'cau_hinh.h: ' + so.join(' ') + ' | mô hình: ' + mong.join(' '));
  });
  kiem(new RegExp('#define\\s+SO_KHUNG_LOC\\s+' + ts0().so_khung_loc + '\\b').test(tep), 'SO_KHUNG_LOC trong cau_hinh.h khác so_khung_loc mặc định của mô phỏng');
  console.log('  ROI ' + mong.slice(0, 4).join(', ') + '; ảnh ' + cam.roi.w + ' × ' + cam.roi.h + ' co về ' + ts0().dau_vao + ' × ' + ts0().dau_vao +
    ' (ngang còn ' + (ts0().dau_vao / cam.roi.w * 100).toFixed(0) + ' %, dọc còn ' + (ts0().dau_vao / cam.roi.h * 100).toFixed(0) + ' %)');
}

// ---------- 4. Nhận diện một xe, gán làn ----------
console.log('4. Một xe: nhận ra, đúng làn, đúng loại');
for (const xoay of [1, 0]) {
  const ts = ts0(); ts.xoay = xoay; ts.nhan_dung = 100;
  for (let h = 1; h <= 4; h++) {
    const cam = MayAnh.tao(h, ts, HC);
    for (const k of [1, 0.75, 0.6]) {
      for (let y = 110; y <= 300; y += 10) {
        for (const [x, lan] of [[H.X_NGOAI, 's'], [H.X_TRONG, 'l']]) {
          const nd = MayAnh.nhanDien(cam, [xeTai(x, y, h, k, 3)], chac);
          const kq = MayAnh.phanLan(nd.vat, cam.lan_ngoai, cam.lan_trong);
          const dung = kq[lan] === 1 && kq.s + kq.l === 1 && kq.ut === 3 && kq.emg === (lan === 's' ? 1 : 2);
          kiem(dung, 'xe ở (' + x + ', ' + y + ') hướng ' + h + ' cỡ ' + k + ' xoay ' + xoay, JSON.stringify(kq));
        }
      }
    }
  }
}
{
  // xe chạy ngang qua trong ngã tư (không nằm trong làn chờ) không được đếm
  const cam = cam_chac;
  const nd = MayAnh.nhanDien(cam, [{ x: 450, y: 420, goc: 0, dai: 51, rong: 25.5, cao: 19.5, loai: 0 }], chac);
  const kq = MayAnh.phanLan(nd.vat, cam.lan_ngoai, cam.lan_trong);
  kiem(kq.s + kq.l === 0, 'xe đang ở giữa ngã tư bị đếm vào làn chờ', JSON.stringify(kq));
  // xe ở làn rẽ phải (ngoài hai làn chờ) không được đếm
  const nd2 = MayAnh.nhanDien(cam, [xeTai(375, 200, 1, 0.75)], chac);
  const kq2 = MayAnh.phanLan(nd2.vat, cam.lan_ngoai, cam.lan_trong);
  kiem(kq2.s + kq2.l === 0, 'xe ở làn rẽ phải bị đếm vào làn chờ', JSON.stringify(kq2));
}

// ---------- 5. Gộp ô ----------
console.log('5. Gộp các ô sáng nằm sát nhau');
{
  const cam = cam_chac, ts = cam.ts, N = ts.dau_vao, O = ts.o;
  const oCua = (xe) => {        // ô mà tâm xe rơi vào (tính lại độc lập với may_anh.js)
    const g = MayAnh.chieuXe(cam, xe); let u0 = 1e9, u1 = -1e9, v0 = 1e9, v1 = -1e9;
    g.forEach(p => { u0 = Math.min(u0, p.u); u1 = Math.max(u1, p.u); v0 = Math.min(v0, p.v); v1 = Math.max(v1, p.v); });
    u0 = Math.max(u0, cam.roi.x); u1 = Math.min(u1, cam.roi.x + cam.roi.w); v0 = Math.max(v0, cam.roi.y); v1 = Math.min(v1, cam.roi.y + cam.roi.h);
    return [Math.floor(((u0 + u1) / 2 - cam.roi.x) * N / cam.roi.w / O), Math.floor(((v0 + v1) / 2 - cam.roi.y) * N / cam.roi.h / O)];
  };
  let gop = 0, tach = 0;
  for (let y = 100; y <= 220; y += 4) for (let cach = 30; cach <= 120; cach += 2) {
    const a = xeTai(H.X_NGOAI, y, 1, 0.6), b = xeTai(H.X_NGOAI, y + cach, 1, 0.6);
    const oa = oCua(a), ob = oCua(b), sat = Math.abs(oa[0] - ob[0]) <= 1 && Math.abs(oa[1] - ob[1]) <= 1;
    const n = MayAnh.nhanDien(cam, [a, b], chac).vat.length;
    if (sat) { gop++; kiem(n === 1, 'hai xe ở hai ô sát nhau phải gộp thành một vật', 'y ' + y + ', cách ' + cach + ': ' + n); }
    else { tach++; kiem(n === 2, 'hai xe ở hai ô cách nhau phải là hai vật', 'y ' + y + ', cách ' + cach + ': ' + n); }
  }
  kiem(gop > 20 && tach > 20, 'ca thử gộp ô không đủ cả hai trường hợp', gop + ' / ' + tach);
  // khác nhãn thì không gộp
  const a = xeTai(H.X_NGOAI, 100, 1, 0.6, 0), b = xeTai(H.X_NGOAI, 136, 1, 0.6, 1), oa = oCua(a), ob = oCua(b);
  kiem(Math.abs(oa[0] - ob[0]) <= 1 && Math.abs(oa[1] - ob[1]) <= 1, 'ca thử khác nhãn: hai xe phải rơi vào hai ô sát nhau', oa + ' / ' + ob);
  kiem(MayAnh.nhanDien(cam, [a, b], chac).vat.length === 2, 'hai ô sát nhau nhưng khác nhãn không được gộp');
  console.log('  ' + gop + ' ca hai xe rơi vào ô sát nhau (gộp), ' + tach + ' ca rơi vào ô cách nhau (tách).');
}

// ---------- 6. Xe đứng sát vạch dừng, và hàng xe chờ kín làn ----------
console.log('6. Xe sát vạch dừng và hàng xe chờ kín làn (mô hình nhận đúng 100 % xe nhìn thấy đủ)');
{
  // Một xe duy nhất đứng sát vạch dừng: đây là xe quan trọng nhất, camera không thấy thì hướng đó không được xanh.
  for (const co of [100, 75, 60]) {
    const k = co / 100, y = HC.Y_VACH - 2 - H.XE_DAI * k / 2;
    for (const xoay of [1, 0]) {
      const ts = ts0(); ts.xoay = xoay; ts.nhan_dung = 100;
      const cam = MayAnh.tao(1, ts, HC);
      const kq = MayAnh.phanLan(MayAnh.nhanDien(cam, [xeTai(H.X_TRONG, y, 1, k)], chac).vat, cam.lan_ngoai, cam.lan_trong);
      if (xoay) kiem(kq.l === 1 && kq.s === 0, 'cỡ ' + co + ' %, cạnh dài dọc đường: phải thấy xe sát vạch dừng', JSON.stringify(kq));
      else if (co <= 75) kiem(kq.l === 0, 'cỡ ' + co + ' %, cạnh dài ngang đường: xe sát vạch dừng nằm gần hết ngoài khung nên không nhận ra được (ca này sai tức là hình học đã đổi)', JSON.stringify(kq));
    }
  }
  function demHang(co, xoay, dau_vao) {
    const k = co / 100, dai = H.XE_DAI * k, cach = dai + 8.5;
    const suc_chua = Math.floor((HC.Y_VACH - HC.MEP_BAO - 2 - dai) / cach) + 1;
    const ts = ts0(); ts.xoay = xoay; ts.nhan_dung = 100; ts.dau_vao = dau_vao;
    const cam = MayAnh.tao(1, ts, HC), ds = [];
    for (let i = 0; i < suc_chua; i++) for (const x of [H.X_NGOAI, H.X_TRONG]) ds.push(xeTai(x, HC.Y_VACH - 2 - dai / 2 - i * cach, 1, k));
    // 200 khung với số ngẫu nhiên thật để tính cả phần xe trước che xe sau
    const rng = (() => { let a = 12345; return () => { a = (a * 1103515245 + 12345) >>> 0; return (a >>> 8) / 16777216; }; })();
    let s = 0, l = 0;
    for (let i = 0; i < 200; i++) { const kq = MayAnh.phanLan(MayAnh.nhanDien(cam, ds, rng).vat, cam.lan_ngoai, cam.lan_trong); s += kq.s; l += kq.l; }
    return { suc_chua, s: s / 200, l: l / 200 };
  }
  const so = (o) => o.s.toFixed(1) + ' + ' + o.l.toFixed(1);
  for (const co of [100, 75, 60]) {
    const a = demHang(co, 1, 96), b = demHang(co, 1, 160), c = demHang(co, 0, 96);
    console.log('  Cỡ xe ' + String(co).padStart(3) + ' %, mỗi làn ' + a.suc_chua + ' xe. Camera đếm trung bình (làn ngoài + làn trong): ảnh 96 × 96 ' + so(a) +
      '; ảnh 160 × 160 ' + so(b) + '; gắn cạnh dài ngang đường ' + so(c));
    // ảnh 160 × 160: đếm gần đủ ở mọi cỡ xe
    kiem(b.s >= b.suc_chua - 0.4 && b.l >= b.suc_chua - 0.4 && b.s <= b.suc_chua + 0.2 && b.l <= b.suc_chua + 0.2, 'cỡ ' + co + ' %, ảnh 160 × 160: phải đếm gần đủ', so(b));
    // ảnh 96 × 96: cỡ 100 % đếm gần đủ; cỡ 75 % và 60 % thì hai xe cuối hàng rơi vào hai ô sát nhau nên bị gộp, thiếu đúng một xe
    if (co === 100) kiem(a.s >= 3.6 && a.l >= 3.6, 'cỡ 100 %, ảnh 96 × 96: phải đếm gần đủ', so(a));
    else kiem(Math.abs(a.s - (a.suc_chua - 1)) <= 0.4 && Math.abs(a.l - (a.suc_chua - 1)) <= 0.4, 'cỡ ' + co + ' %, ảnh 96 × 96: giới hạn đã biết là thiếu một xe mỗi làn khi làn kín (ca này sai tức là mô hình đã đổi, phải sửa lại tài liệu)', so(a));
    kiem(a.s <= a.suc_chua + 0.2 && a.l <= a.suc_chua + 0.2, 'cỡ ' + co + ' %: đếm thừa', so(a));
  }
}

// ---------- 7. Nhịp xử lí: chụp → một chu kì sau mới có kết quả, 4 camera chụp lệch nhau ----------
console.log('7. Nhịp 1 khung mỗi giây và độ trễ');
{
  const cai = MoPhong.caiDatMacDinh();
  cai.luu_luong = [0, 0, 0, 0, 0]; cai.camera.nhan_dung = 100; cai.camera.so_khung_loc = 1;
  const mp = new MoPhong({ hat: 1, cai });
  const T = 1000 / cai.camera.khung_giay;
  const chup = [[], [], [], [], []], xong = [[], [], [], [], []], truoc = [0, 0, 0, 0, 0], cho_truoc = [null, null, null, null, null];
  for (let i = 0; i < 400; i++) {
    for (let h = 1; h <= 4; h++) {
      const cm = mp.cam[h];
      if (cm.cho && cm.cho !== cho_truoc[h]) chup[h].push(cm.cho.t);
      cho_truoc[h] = cm.cho;
      if (cm.so_khung !== truoc[h]) { truoc[h] = cm.so_khung; xong[h].push([cm.khung.t, mp.t]); }
    }
    mp.buoc(40);
  }
  for (let h = 1; h <= 4; h++) {
    kiem(Math.abs(chup[h][0] - (h - 1) * T / 4) <= 40, 'camera ' + h + ' chụp khung đầu ở ' + chup[h][0] + ' ms, phải quanh ' + (h - 1) * T / 4);
    for (let i = 1; i < chup[h].length; i++) kiem(Math.abs(chup[h][i] - chup[h][i - 1] - T) <= 40, 'camera ' + h + ': hai khung cách nhau ' + (chup[h][i] - chup[h][i - 1]) + ' ms');
    for (const [t_chup, t_xong] of xong[h]) kiem(t_xong - t_chup >= T && t_xong - t_chup <= T + 40, 'camera ' + h + ': kết quả có sau ' + (t_xong - t_chup) + ' ms');
    kiem(xong[h].length >= 13 && xong[h].length <= 16, 'camera ' + h + ': 16 giây xử lí ' + xong[h].length + ' khung');
  }
  console.log('  Chu kì ' + T + ' ms; 4 camera chụp khung đầu ở ' + [1, 2, 3, 4].map(h => chup[h][0]).join(', ') + ' ms; kết quả có sau đúng một chu kì.');

  // Một xe đặt đứng yên trong vùng chờ: bộ điều khiển biết sau bao lâu, với từng cỡ bộ lọc
  for (const loc of [1, 3, 5]) {
    const c2 = MoPhong.caiDatMacDinh(); c2.luu_luong = [0, 0, 0, 0, 0]; c2.camera.nhan_dung = 100; c2.camera.so_khung_loc = loc;
    const m2 = new MoPhong({ hat: 1, cai: c2 });
    for (let i = 0; i < 250; i++) m2.buoc(40);          // 10 giây cho bộ lọc đầy khung trống
    const t0 = m2.t;
    m2.xeChoCamera = () => [xeTai(H.X_NGOAI, 250, 1, 0.75)];   // từ lúc này camera hướng nào cũng "thấy" một xe ở chỗ đó của Hướng 1
    let t_biet = -1;
    for (let i = 0; i < 400 && t_biet < 0; i++) { m2.buoc(40); if (m2.dem[1].s === 1) t_biet = m2.t - t0; }
    const it = (Math.floor(loc / 2) + 1) * T, nhieu = (Math.floor(loc / 2) + 2) * T + c2.camera.gui_ms + 40;
    kiem(t_biet >= it && t_biet <= nhieu, 'lọc ' + loc + ' khung: biết có xe sau ' + t_biet + ' ms, phải trong khoảng ' + it + ' tới ' + nhieu);
    console.log('  Lọc ' + loc + ' khung: xe xuất hiện → board trung tâm biết sau ' + t_biet + ' ms.');
  }
}

console.log(loi ? 'KHÔNG ĐẠT: ' + loi + ' lỗi.' : 'ĐẠT: không có lỗi.');
process.exit(loi ? 1 : 0);
