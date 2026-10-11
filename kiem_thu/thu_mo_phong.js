// thu_mo_phong.js — Chạy mô phỏng không vẽ để kiểm tra:
//  - không có hai xe đè lên nhau, không xe nào vượt đèn đỏ, không xe nào đứng yên quá 300 giây liền;
//  - in bảng so sánh chế độ cố định và thích ứng với vài kiểu dòng xe.
// Chạy: node kiem_thu/thu_mo_phong.js
'use strict';
const QD = require('../web/quyet_dinh.js');
const DieuKhien = require('../web/dieu_khien.js');
const MoPhong = require('../web/mo_phong.js');

let so_loi = 0;
function kiem(dung, ten) { if (!dung) { so_loi++; if (so_loi <= 30) console.log('  SAI: ' + ten); } }

const KIEU = {
  'Đều, vừa phải': { luu_luong: [0, 6, 6, 6, 6], ti_le_trai: [0, 25, 25, 25, 25], ti_le_phai: 15, toc_do: 120 },
  'Dọc đông, ngang vắng': { luu_luong: [0, 10, 2, 9, 1], ti_le_trai: [0, 25, 10, 20, 10], ti_le_phai: 15, toc_do: 120 },
  'Chỉ Hướng 1 có xe': { luu_luong: [0, 8, 0, 0, 0], ti_le_trai: [0, 40, 0, 0, 0], ti_le_phai: 10, toc_do: 120 },
  'Giờ cao điểm': { luu_luong: [0, 12, 12, 12, 12], ti_le_trai: [0, 30, 30, 30, 30], ti_le_phai: 15, toc_do: 120 },
  'Ít rẽ trái': { luu_luong: [0, 8, 8, 8, 8], ti_le_trai: [0, 5, 5, 5, 5], ti_le_phai: 15, toc_do: 120 },
  'Vắng': { luu_luong: [0, 1.5, 1.5, 1.5, 1.5], ti_le_trai: [0, 25, 25, 25, 25], ti_le_phai: 15, toc_do: 120 }
};

// ---------- 1) Kiểm tra an toàn trong lúc chạy ----------
for (const [ten, cai] of Object.entries(KIEU)) {
  for (let che_do = 0; che_do <= 1; che_do++) {
    const ts = QD.thamSoMacDinh(); ts.che_do = che_do;
    const mp = new MoPhong({ hat: 7, ts, cai: JSON.parse(JSON.stringify(cai)) });
    let gan_nhat = 1e9, vuot_do = 0, ut = 0, dung_lau_nhat = 0;
    const truoc = new Map(), dung = new Map();   // dung: id → [vị trí, lúc bắt đầu đứng yên]
    for (let i = 0; i < 30 * 60 * 25; i++) {
      if (i === 5000 || i === 21000) { mp.thaXeCuuThuong(1 + (i % 4)); ut++; }
      mp.buoc(40);
      kiem(!mp.dk.loi, ten + ': bộ điều khiển báo lỗi');
      for (let a = 0; a < mp.xe.length; a++) {
        const x = mp.xe[a];
        // xe vừa qua vạch dừng: đèn của nó phải đang xanh hoặc vàng (hoặc vừa đỏ sau khi xe đã quyết định đi tiếp)
        if (x.da_qua && truoc.get(x.id) === false && x.huong_di !== 'phai') {
          if (mp.dk.mauNhom(x.nhom, mp.t) === DieuKhien.MAU_DO && !x.di_tiep) vuot_do++;
        }
        truoc.set(x.id, x.da_qua);
        const d0 = dung.get(x.id);
        if (!d0 || x.s - d0[0] > 1) dung.set(x.id, [x.s, mp.t]);
        else if (mp.t - d0[1] > dung_lau_nhat) dung_lau_nhat = mp.t - d0[1];
        for (let b = a + 1; b < mp.xe.length; b++) {
          const y = mp.xe[b];
          if (Math.abs(x.x - y.x) > 110 || Math.abs(x.y - y.y) > 110) continue;
          for (let k = -1; k <= 1; k++) for (let m = -1; m <= 1; m++) {
            const d = Math.hypot(x.x + x.dx * 17 * k - y.x - y.dx * 17 * m, x.y + x.dy * 17 * k - y.y - y.dy * 17 * m);
            if (d < gan_nhat) gan_nhat = d;
          }
        }
      }
    }
    const th = mp.tongHop();
    kiem(gan_nhat >= 30, ten + ' (chế độ ' + che_do + '): hai xe đè lên nhau, khoảng cách đốt ' + gan_nhat.toFixed(1));
    kiem(vuot_do === 0, ten + ' (chế độ ' + che_do + '): ' + vuot_do + ' xe vượt đèn đỏ');
    kiem(dung_lau_nhat < 300000, ten + ' (chế độ ' + che_do + '): có xe đứng yên ' + (dung_lau_nhat / 1000).toFixed(0) + ' s liền');
    kiem(mp.so_lan_chan_nhau === 0, ten + ' (chế độ ' + che_do + '): có xe chặn lẫn nhau');
    kiem(mp.thong_ke[che_do].cuu_thuong === ut, ten + ' (chế độ ' + che_do + '): xe cứu thương chưa qua hết');
    console.log((ten + ' / ' + (che_do ? 'thích ứng' : 'cố định')).padEnd(34) + ' xe gần nhau nhất ' + gan_nhat.toFixed(1).padStart(5) +
      ' mm | đã qua ' + String(th.da_qua).padStart(4) + ' | còn kẹt ' + String(th.con_ket).padStart(3) + ' | chờ TB ' + th.cho_tb.toFixed(1).padStart(5) +
      ' s | chờ lâu nhất ' + th.cho_max.toFixed(0).padStart(3) + ' s | cứu thương TB ' +
      (mp.thong_ke[che_do].tong_cuu_thuong / Math.max(1, mp.thong_ke[che_do].cuu_thuong) / 1000).toFixed(1) + ' s | đứng yên lâu nhất ' + (dung_lau_nhat / 1000).toFixed(0) + ' s');
  }
}

// ---------- 2) Bảng so sánh hai chế độ (60 phút mô phỏng, 3 hạt giống) ----------
console.log('\nSo sánh thời gian chờ trung bình (giây), 60 phút mô phỏng, trung bình 3 lần chạy:');
console.log('Kiểu dòng xe'.padEnd(24) + 'Cố định'.padStart(9) + 'Thích ứng'.padStart(11) + '   Thay đổi');
for (const [ten, cai] of Object.entries(KIEU)) {
  let cd = 0, tu = 0;
  for (const hat of [1, 2, 3]) {
    const kq = MoPhong.soSanh(cai, QD.thamSoMacDinh(), 60, hat);
    cd += kq[0].cho_tb / 3; tu += kq[1].cho_tb / 3;
  }
  console.log(ten.padEnd(24) + cd.toFixed(1).padStart(9) + tu.toFixed(1).padStart(11) + '   ' + ((tu - cd) / cd * 100).toFixed(0).padStart(4) + ' %');
}
console.log(so_loi === 0 ? '\nĐẠT: không có lỗi.' : '\nKHÔNG ĐẠT: ' + so_loi + ' lỗi.');
process.exit(so_loi === 0 ? 0 : 1);
