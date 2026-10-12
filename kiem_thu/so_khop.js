// so_khop.js — So bản C++ (firmware) với bản JavaScript (web) của bộ quyết định.
// Sinh ngẫu nhiên tham số, số đếm, xe ưu tiên, camera mất tín hiệu; cho cả hai bản chạy
// cùng một kịch bản rồi so từng dòng trạng thái. Hai bản phải giống nhau hoàn toàn.
// Chạy: node kiem_thu/so_khop.js   (cần g++)
'use strict';
const { execFileSync } = require('child_process');
const fs = require('fs');
const os = require('os');
const path = require('path');
const QD = require('../web/quyet_dinh.js');
const DieuKhien = require('../web/dieu_khien.js');
const MayAnh = require('../web/may_anh.js');

const thu_muc = fs.mkdtempSync(path.join(os.tmpdir(), 'so_khop_'));
const tep_chay = path.join(thu_muc, 'thu_cpp');
execFileSync('g++', ['-std=c++11', '-O1', '-Wall', '-Wextra', '-o', tep_chay, path.join(__dirname, 'thu_cpp.cpp')], { stdio: 'inherit' });

function taoNgauNhien(hat) {
  let a = hat >>> 0;
  return function () {
    a = (a + 0x6D2B79F5) >>> 0;
    let t = a;
    t = Math.imul(t ^ (t >>> 15), t | 1);
    t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

const TEN_TS = ['che_do', 't_min', 't_max', 'cong_them', 'xe_quy_doi', 't_vang', 't_do', 'cd_thang', 'cd_trai',
                'mat_tin_hieu', 'ut_xanh_an_toan', 'ut_giu', 'ut_toi_da', 'cat_som',
                'kieu_pha', 'hoc_cach_thu', 'hoc_so_xe', 'hoc_nguong'];

// ---- sinh kịch bản ----
const r = taoNgauNhien(20261011);
const ng = (a, b) => a + Math.floor(r() * (b - a + 1));
const lenh = [];
function thamSoNgauNhien() {
  const t_min = ng(5, 30) * 1000;
  return [ng(0, 1), t_min, t_min + ng(0, 60) * 1000, ng(0, 10) * 500, ng(1, 20), ng(2, 5) * 1000, ng(0, 4) * 1000,
          ng(10, 40) * 1000, ng(5, 30) * 1000, 3000, ng(0, 8) * 1000, ng(0, 8) * 500, ng(20, 70) * 1000,
          r() < 0.3 ? 0 : ng(2, 10) * 1000,
          // kiểu chia pha (một nửa số lần là tự học), và tham số tự học nhỏ để bộ học quyết định thường xuyên
          r() < 0.5 ? 0 : ng(1, 4), ng(0, 4), ng(0, 6), ng(0, 30)];
}
for (let lan = 0; lan < 250; lan++) {
  // Cứ 3 kịch bản thì 1 cái dành cho bộ tự học: chế độ thích ứng, kiểu tự học, số đếm đổi thường xuyên,
  // xe ưu tiên và camera mất tín hiệu thì hiếm (một nửa số kịch bản này không có). Các kịch bản còn lại trộn đủ thứ.
  const de_hoc = lan % 3 === 1, hiem = de_hoc ? (lan % 2 === 0 ? 0.1 : 0) : 1;
  const tsMoi = () => { const v = thamSoNgauNhien(); if (de_hoc) { v[0] = 1; v[14] = 0; v[15] = ng(1, 3); v[16] = ng(1, 3); } return v; };
  lenh.push('TS ' + tsMoi().join(' '));
  for (let i = 0; i < 40; i++) {              // bộ lập kế hoạch
    const v = [];
    for (let k = 0; k < 8; k++) v.push(r() < 0.35 ? 0 : ng(0, 6));
    for (let k = 0; k < 4; k++) v.push(r() < 0.9 ? 1 : 0);
    v.push(ng(0, 4), ng(0, 4), ng(0, 1));       // kiểu chia pha của hai trục, trục tới lượt
    lenh.push('KH ' + v.join(' '));
  }
  let t = lan % 3 === 0 ? 4294900000 : ng(0, 100000);   // một phần kịch bản chạy qua chỗ bộ đếm 32 bit quay vòng
  lenh.push('BD ' + (t >>> 0));
  const emg_toi = [0, 0, 0, 0, 0], im_toi = [0, 0, 0, 0, 0];
  const cs = [0, 0, 0, 0, 0], cl = [0, 0, 0, 0, 0];   // số đếm giữ nguyên một lúc rồi mới đổi, để có lúc hết xe đủ lâu
  const loai_ut = [0, 1, 1, 1, 1];
  for (let i = 0; i < 3000; i++) {
    t += ng(10, 500);
    for (let h = 1; h <= 4; h++) {
      if (r() < 0.004 * hiem) im_toi[h] = t + ng(1, 15) * 1000;
      if (r() < 0.003 * hiem) emg_toi[h] = t + ng(1, 80) * 1000;
      if (t < im_toi[h] || r() < 0.3) continue;
      if (r() < (de_hoc ? 0.15 : 0.04)) { cs[h] = ng(0, 5) * (r() < 0.5 ? 1 : 0); cl[h] = ng(0, 5) * (r() < 0.4 ? 1 : 0); }
      // xe ưu tiên: loại xe giữ nguyên trong một đợt; đôi khi gửi kiểu cũ (chỉ có emg) hoặc chỉ có ut
      const co_ut = t < emg_toi[h];
      if (!co_ut) loai_ut[h] = ng(1, 4);
      const kieu_tin = r();
      lenh.push(['D', h, cs[h], cl[h], co_ut && kieu_tin < 0.9 ? ng(1, 2) : 0, t >>> 0, r() < 0.02 ? 1 : 0,
                 co_ut && kieu_tin > 0.1 ? loai_ut[h] : 0].join(' '));
    }
    if (r() < 0.002) lenh.push('TS ' + tsMoi().join(' '));
    lenh.push('T ' + (t >>> 0));
  }
}

// ---- firmware camera: bộ lọc khung và gán làn (lan_duong.h so với may_anh.js) ----
let so_lenh_cam = 0;
for (let lan = 0; lan < 300; lan++) {
  lenh.push('LM ' + [1, 3, 5, 5, 3, 4, 0, 7][lan % 8]);
  let co_ut = false;
  for (let i = 0; i < 60; i++) {
    if (r() < 0.1) co_ut = !co_ut;
    const emg = co_ut && r() < 0.8 ? ng(1, 2) : (r() < 0.05 ? ng(1, 2) : 0);
    lenh.push(['LK', r() < 0.8 ? 3 : ng(0, 9), r() < 0.8 ? 2 : ng(0, 9), emg, emg && r() < 0.9 ? ng(1, 4) : (r() < 0.1 ? ng(0, 5) : 0)].join(' '));
    so_lenh_cam++;
  }
}
for (let lan = 0; lan < 3000; lan++) {
  // hai tứ giác kề nhau, méo ngẫu nhiên như làn nhìn phối cảnh; xe rải khắp khung 320 × 240
  const x0 = ng(20, 120), x1 = x0 + ng(30, 90), x2 = x1 + ng(30, 90), y0 = ng(0, 60), y1 = ng(160, 239), lech = ng(-40, 40);
  const g = [x0 + lech, y0, x1 + lech, y0, x1, y1, x0, y1, x1 + lech, y0, x2 + lech, y0, x2, y1, x1, y1];
  const n = ng(0, 12), v = [];
  for (let i = 0; i < n; i++) v.push(ng(0, 319), ng(0, 239), r() < 0.8 ? 0 : ng(0, 5));
  lenh.push(['PL'].concat(g, [n], v).join(' '));
  so_lenh_cam++;
}

// ---- bản JavaScript chạy kịch bản ----
// Bản JS không có bộ đếm 32 bit, nên với kịch bản qua chỗ quay vòng ta chạy bằng thời gian
// chưa quay vòng rồi in ra phần 32 bit thấp.
const ra_js = [], dem_sk = {};   // dem_sk: số lần mỗi loại sự kiện xảy ra, để biết kịch bản đã chạm tới những gì
{
  const ts = QD.thamSoMacDinh();
  let dk = new DieuKhien(ts);
  let loc = new MayAnh.BoLoc(5);
  let goc = 0, t_truoc = 0;                    // goc: số lần đã quay vòng × 2^32
  const moRong = (t32) => { if (t32 < t_truoc) goc += 4294967296; t_truoc = t32; return goc + t32; };
  for (const dong of lenh) {
    const p = dong.split(' ');
    const v = p.slice(1).map(Number);
    if (p[0] === 'TS') TEN_TS.forEach((ten, i) => { ts[ten] = v[i]; });
    else if (p[0] === 'KH') {
      const s = [0], l = [0], hl = [false];
      for (let h = 1; h <= 4; h++) { s.push(v[(h - 1) * 2]); l.push(v[(h - 1) * 2 + 1]); hl.push(v[7 + h] !== 0); }
      const kh = QD.lapKeHoach(ts, s, l, hl, [v[12], v[13]], v[14]);
      ra_js.push('KH ' + kh.length + kh.map(b => ' ' + b.mat_na + ':' + b.xanh + ':' + (b.ngoai ? 1 : 0)).join(''));
    } else if (p[0] === 'BD') {
      dk = new DieuKhien(ts); goc = 0; t_truoc = v[0];
      dk.khiSuKien = (loai) => { dem_sk[loai] = (dem_sk[loai] || 0) + 1; };
      dk.khoiDong(v[0]);
    }
    else if (p[0] === 'LM') loc = new MayAnh.BoLoc(v[0]);
    else if (p[0] === 'LK') {
      loc.them({ s: v[0], l: v[1], emg: v[2], ut: v[3] });
      const kq = loc.layKetQua();
      ra_js.push(['L', kq.s, kq.l, kq.emg, kq.ut].join(' '));
    } else if (p[0] === 'PL') {
      const q = (i) => [0, 1, 2, 3].map(k => ({ x: v[i + k * 2], y: v[i + k * 2 + 1] }));
      const vat = [];
      for (let i = 0; i < v[16]; i++) vat.push({ x: v[17 + i * 3], y: v[18 + i * 3], loai: v[19 + i * 3] });
      const kq = MayAnh.phanLan(vat, q(0), q(8));
      ra_js.push(['P', kq.s, kq.l, kq.emg, kq.ut].join(' '));
    }
    else if (p[0] === 'D') dk.nhanDem(v[0], v[1], v[2], v[3], moRong(v[4]), v[5] !== 0, v[6]);
    else if (p[0] === 'T') {
      const t = moRong(v[0]);
      dk.capNhat(t);
      const mau = [];
      for (let g = 0; g < 8; g++) mau.push(dk.mauNhom(g, v[0]));
      const hc = dk.hoc;
      ra_js.push(['T', v[0], dk.pha, dk.xanh, dk.vang, dk.i_buoc, dk.chu_ki, dk.ut_huong, dk.loi ? 1 : 0, dk.conLai(t)].concat(mau).join(' ') +
        ' | ' + [dk.truc_ke].concat(
          [0, 1].map(k => [hc.kieu[k], hc.b[k], hc.q[k], hc.dem[k], hc.cach[k], hc.lien[k], hc.tuoi[k], hc.ra[k]].join(' ')),
          [hc.l_truc, hc.l_kieu, hc.l_do ? 1 : 0, hc.l_thu ? 1 : 0, hc.l_hong ? 1 : 0, hc.l_qt ? 1 : 0, hc.l_g, hc.l_n]).join(' '));
    }
  }
}

// ---- bản C++ chạy cùng kịch bản ----
const ra_cpp = execFileSync(tep_chay, { input: lenh.join('\n') + '\n', maxBuffer: 1 << 30 }).toString().trim().split('\n');
fs.rmSync(thu_muc, { recursive: true, force: true });

let khac = 0;
const n = Math.max(ra_js.length, ra_cpp.length);
for (let i = 0; i < n && khac < 10; i++) {
  if (ra_js[i] !== ra_cpp[i]) {
    khac++;
    console.log('Khác nhau ở dòng ' + (i + 1) + ':\n  JS : ' + ra_js[i] + '\n  C++: ' + ra_cpp[i]);
  }
}
console.log('Đã so ' + n + ' dòng kết quả (' + lenh.length + ' lệnh).');
console.log('Kịch bản có: ' + (dem_sk.chu_ki || 0) + ' lần chốt kế hoạch, ' + (dem_sk.cat_som || 0) + ' lần cắt xanh sớm, ' +
  (dem_sk.uu_tien || 0) + ' lần ưu tiên; tự học: ' + (dem_sk.hoc_thu || 0) + ' lượt chạy kiểu khác, ' + (dem_sk.hoc_nhan || 0) + ' lần đổi kiểu, ' +
  (dem_sk.hoc_tra || 0) + ' lần thử không đạt, ' + (dem_sk.hoc_huy || 0) + ' lần hủy.');
console.log('Firmware camera: ' + so_lenh_cam + ' lệnh lọc khung và gán làn.');
console.log(khac === 0 ? 'ĐẠT: bản C++ và bản JavaScript giống nhau hoàn toàn.' : 'KHÔNG ĐẠT.');
process.exit(khac === 0 ? 0 : 1);
