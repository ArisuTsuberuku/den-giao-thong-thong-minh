// khao_sat_kieu_pha.js — Khảo sát: kiểu chia pha nào chờ ít nhất ở từng kiểu dòng xe, và chế độ
// tự học bám được kiểu tốt nhất tới đâu. Không phải bài kiểm tra đạt hay không đạt; chỉ in bảng số.
//   node kiem_thu/khao_sat_kieu_pha.js [số phút mỗi lần chạy, mặc định 120] [camera: lt hoặc s3, mặc định lt]
// Mặc định dùng camera lí tưởng, để so riêng các kiểu chia pha mà không lẫn sai số của camera;
// "s3" là camera kiểu ESP32-S3-CAM (1 khung/giây, lọc 3 khung) như mặc định của mô phỏng.
// Mỗi dòng chạy 6 phương án × 3 hạt giống, nên bảng 120 phút mất khoảng nửa giờ (camera s3 lâu hơn chừng 2 lần).
// Bảng ở README.md lấy từ lệnh này với 120 phút.
'use strict';
const QD = require('../web/quyet_dinh.js');
const MoPhong = require('../web/mo_phong.js');

const PHUT = +(process.argv[2] || 120);
const CAM = process.argv[3] === 's3' ? {} : { kieu: 0 };
const HAT = [1, 2, 3];
const R = QD.KIEU_RIENG, T = QD.KIEU_THANG_TRUOC, D = QD.KIEU_DOI_XUNG;

// kieu: một kiểu cho cả hai trục, hoặc [kiểu trục dọc, kiểu trục ngang] để ép riêng từng trục
function chay(cai, kieu, hat) {
  const ts = QD.thamSoMacDinh();
  ts.kieu_pha = Array.isArray(kieu) ? R : kieu;
  const c = JSON.parse(JSON.stringify(cai)); c.camera = Object.assign({}, CAM);
  const mp = new MoPhong({ hat, ts, cai: c });
  if (Array.isArray(kieu)) mp.dk.kieuChoChuKi = () => kieu.slice();
  let dem = 0, thang_truoc = [0, 0];
  for (let i = 0; i < PHUT * 1500; i++) {
    mp.buoc(40);
    if (i % 250 === 0) { const k = mp.dk.hoc.kieu; dem++; if (k[0] === T) thang_truoc[0]++; if (k[1] === T) thang_truoc[1]++; }
  }
  return { cho: mp.tongHop().cho_tb, doc: thang_truoc[0] / dem, ngang: thang_truoc[1] / dem };
}

function trungBinh(cai, kieu) {
  const o = { cho: 0, doc: 0, ngang: 0 };
  for (const hat of HAT) { const r = chay(cai, kieu, hat); o.cho += r.cho / HAT.length; o.doc += r.doc / HAT.length; o.ngang += r.ngang / HAT.length; }
  return o;
}

const dong = (q, p) => ({ luu_luong: [0].concat(q), ti_le_trai: [0].concat(p), ti_le_phai: 15, toc_do: 120 });
const CA = [
  ['3 xe/phút, 0 % rẽ trái', dong([3, 3, 3, 3], [0, 0, 0, 0])],
  ['3 xe/phút, 25 % rẽ trái', dong([3, 3, 3, 3], [25, 25, 25, 25])],
  ['6 xe/phút, 0 % rẽ trái', dong([6, 6, 6, 6], [0, 0, 0, 0])],
  ['6 xe/phút, 5 % rẽ trái', dong([6, 6, 6, 6], [5, 5, 5, 5])],
  ['6 xe/phút, 10 % rẽ trái', dong([6, 6, 6, 6], [10, 10, 10, 10])],
  ['6 xe/phút, 25 % rẽ trái', dong([6, 6, 6, 6], [25, 25, 25, 25])],
  ['6 xe/phút, 40 % rẽ trái', dong([6, 6, 6, 6], [40, 40, 40, 40])],
  ['8 xe/phút, 5 % rẽ trái', dong([8, 8, 8, 8], [5, 5, 5, 5])],
  ['9 xe/phút, 0 % rẽ trái', dong([9, 9, 9, 9], [0, 0, 0, 0])],
  ['9 xe/phút, 25 % rẽ trái', dong([9, 9, 9, 9], [25, 25, 25, 25])],
  ['12 xe/phút, 30 % rẽ trái', dong([12, 12, 12, 12], [30, 30, 30, 30])],
  ['6 xe/phút, dọc 0 %, ngang 25 %', dong([6, 6, 6, 6], [0, 25, 0, 25])],
  ['Dọc đông, ngang vắng', dong([10, 2, 9, 1], [25, 10, 20, 10])]
];

const so = (x) => x.toFixed(1).padStart(9);
console.log('Thời gian chờ trung bình (giây), ' + PHUT + ' phút mô phỏng, trung bình ' + HAT.length + ' lần chạy, thông số mặc định, ' +
  (CAM.kieu === 0 ? 'camera lí tưởng.' : 'camera ESP32-S3-CAM.'));
console.log('"Dọc T" là trục dọc thẳng trước, trục ngang riêng; "Ngang T" thì ngược lại. Hai cột cuối: phần thời gian tự học giữ kiểu thẳng trước.');
console.log('Dòng xe (mỗi hướng)'.padEnd(34) + 'Riêng'.padStart(9) + 'Th.trước'.padStart(9) + 'Đối xứng'.padStart(9) + 'Dọc T'.padStart(9) + 'Ngang T'.padStart(9) +
  'Tự học'.padStart(9) + '  so với riêng  so với tốt nhất   dọc   ngang');
let tong_rieng = 0, tong_tot = 0;
for (const [ten, cai] of CA) {
  const nen = [R, T, D, [T, R], [R, T]].map(k => trungBinh(cai, k).cho);
  const hoc = trungBinh(cai, QD.KIEU_TU_HOC), tot = Math.min.apply(null, nen);
  tong_rieng += hoc.cho / nen[0] / CA.length; tong_tot += hoc.cho / tot / CA.length;
  const pt = (x) => ((x - 1) * 100).toFixed(0).padStart(5) + ' %';
  console.log(ten.padEnd(34) + nen.map(so).join('') + so(hoc.cho) + '    ' + pt(hoc.cho / nen[0]) + '         ' + pt(hoc.cho / tot) +
    '       ' + (hoc.doc * 100).toFixed(0).padStart(3) + ' %  ' + (hoc.ngang * 100).toFixed(0).padStart(3) + ' %');
}
console.log('Trung bình các dòng: tự học so với riêng ' + ((tong_rieng - 1) * 100).toFixed(1) + ' %, so với kiểu tốt nhất của từng dòng ' + ((tong_tot - 1) * 100).toFixed(1) + ' %.');
