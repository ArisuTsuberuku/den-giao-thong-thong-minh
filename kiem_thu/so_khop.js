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
                'mat_tin_hieu', 'ut_xanh_an_toan', 'ut_giu', 'ut_toi_da', 'cat_som'];

// ---- sinh kịch bản ----
const r = taoNgauNhien(20261011);
const ng = (a, b) => a + Math.floor(r() * (b - a + 1));
const lenh = [];
function thamSoNgauNhien() {
  const t_min = ng(5, 30) * 1000;
  return [ng(0, 1), t_min, t_min + ng(0, 60) * 1000, ng(0, 10) * 500, ng(1, 20), ng(2, 5) * 1000, ng(0, 4) * 1000,
          ng(10, 40) * 1000, ng(5, 30) * 1000, 3000, ng(0, 8) * 1000, ng(1, 8) * 1000, ng(20, 70) * 1000,
          r() < 0.3 ? 0 : ng(2, 10) * 1000];
}
for (let lan = 0; lan < 250; lan++) {
  lenh.push('TS ' + thamSoNgauNhien().join(' '));
  for (let i = 0; i < 40; i++) {              // bộ lập kế hoạch
    const v = [];
    for (let k = 0; k < 8; k++) v.push(r() < 0.35 ? 0 : ng(0, 6));
    for (let k = 0; k < 4; k++) v.push(r() < 0.9 ? 1 : 0);
    lenh.push('KH ' + v.join(' '));
  }
  let t = lan % 3 === 0 ? 4294900000 : ng(0, 100000);   // một phần kịch bản chạy qua chỗ bộ đếm 32 bit quay vòng
  lenh.push('BD ' + (t >>> 0));
  const emg_toi = [0, 0, 0, 0, 0], im_toi = [0, 0, 0, 0, 0];
  const cs = [0, 0, 0, 0, 0], cl = [0, 0, 0, 0, 0];   // số đếm giữ nguyên một lúc rồi mới đổi, để có lúc hết xe đủ lâu
  for (let i = 0; i < 3000; i++) {
    t += ng(10, 500);
    for (let h = 1; h <= 4; h++) {
      if (r() < 0.004) im_toi[h] = t + ng(1, 15) * 1000;
      if (r() < 0.003) emg_toi[h] = t + ng(1, 80) * 1000;
      if (t < im_toi[h] || r() < 0.3) continue;
      if (r() < 0.04) { cs[h] = ng(0, 5) * (r() < 0.5 ? 1 : 0); cl[h] = ng(0, 5) * (r() < 0.4 ? 1 : 0); }
      lenh.push(['D', h, cs[h], cl[h], t < emg_toi[h] ? ng(1, 2) : 0, t >>> 0, r() < 0.02 ? 1 : 0].join(' '));
    }
    if (r() < 0.002) lenh.push('TS ' + thamSoNgauNhien().join(' '));
    lenh.push('T ' + (t >>> 0));
  }
}

// ---- bản JavaScript chạy kịch bản ----
// Bản JS không có bộ đếm 32 bit, nên với kịch bản qua chỗ quay vòng ta chạy bằng thời gian
// chưa quay vòng rồi in ra phần 32 bit thấp.
const ra_js = [];
{
  const ts = QD.thamSoMacDinh();
  let dk = new DieuKhien(ts);
  let goc = 0, t_truoc = 0;                    // goc: số lần đã quay vòng × 2^32
  const moRong = (t32) => { if (t32 < t_truoc) goc += 4294967296; t_truoc = t32; return goc + t32; };
  for (const dong of lenh) {
    const p = dong.split(' ');
    const v = p.slice(1).map(Number);
    if (p[0] === 'TS') TEN_TS.forEach((ten, i) => { ts[ten] = v[i]; });
    else if (p[0] === 'KH') {
      const s = [0], l = [0], hl = [false];
      for (let h = 1; h <= 4; h++) { s.push(v[(h - 1) * 2]); l.push(v[(h - 1) * 2 + 1]); hl.push(v[7 + h] !== 0); }
      const kh = QD.lapKeHoach(ts, s, l, hl);
      ra_js.push('KH ' + kh.length + kh.map(b => ' ' + b.mat_na + ':' + b.xanh).join(''));
    } else if (p[0] === 'BD') { dk = new DieuKhien(ts); goc = 0; t_truoc = v[0]; dk.khoiDong(v[0]); }
    else if (p[0] === 'D') dk.nhanDem(v[0], v[1], v[2], v[3], moRong(v[4]), v[5] !== 0);
    else if (p[0] === 'T') {
      const t = moRong(v[0]);
      dk.capNhat(t);
      const mau = [];
      for (let g = 0; g < 8; g++) mau.push(dk.mauNhom(g, v[0]));
      ra_js.push(['T', v[0], dk.pha, dk.xanh, dk.vang, dk.i_buoc, dk.chu_ki, dk.ut_huong, dk.loi ? 1 : 0, dk.conLai(t)].concat(mau).join(' '));
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
console.log(khac === 0 ? 'ĐẠT: bản C++ và bản JavaScript giống nhau hoàn toàn.' : 'KHÔNG ĐẠT.');
process.exit(khac === 0 ? 0 : 1);
