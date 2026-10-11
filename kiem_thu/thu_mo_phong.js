// thu_mo_phong.js — Chạy mô phỏng không vẽ để kiểm tra:
//  - không có hai xe đè lên nhau, không xe nào vượt đèn đỏ, không xe nào đứng yên quá 300 giây liền,
//    xe ưu tiên luôn qua được; thử ở cả ba cỡ xe;
//  - trễ phản ứng của người lái; bộ tự học chọn đúng kiểu chia pha;
//  - in bảng so sánh chế độ cố định với các kiểu chia pha của chế độ thích ứng.
// Chạy: node kiem_thu/thu_mo_phong.js   (mất khoảng 6 phút)
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
const TEN_KIEU_PHA = ['tự học', 'riêng', 'thẳng trước', 'đối xứng'];

// ---------- 1) Kiểm tra an toàn trong lúc chạy ----------
// Mỗi lần chạy 30 phút; thả 4 xe ưu tiên đủ 4 loại ở các thời điểm khác nhau.
function chayAnToan(ten, cai, che_do, kieu_pha, co_xe) {
  const ts = QD.thamSoMacDinh(); ts.che_do = che_do; ts.kieu_pha = kieu_pha;
  const cai2 = JSON.parse(JSON.stringify(cai)); cai2.co_xe = co_xe;
  const mp = new MoPhong({ hat: 7, ts, cai: cai2 });
  const nhan = ten + ' / ' + (che_do ? TEN_KIEU_PHA[kieu_pha] : 'cố định') + ' / xe ' + co_xe + ' %';
  let gan_nhat = 1e9, vuot_do = 0, ut = 0, dung_lau_nhat = 0;
  const truoc = new Map(), dung = new Map();   // dung: id → [vị trí, lúc bắt đầu đứng yên]
  const dot = mp.dot;
  for (let i = 0; i < 30 * 60 * 25; i++) {
    if (i === 5000 || i === 13000 || i === 21000 || i === 29000) { ut++; mp.thaXeUuTien(1 + (i / 1000 | 0) % 4, ut); }
    mp.buoc(40);
    kiem(!mp.dk.loi, nhan + ': bộ điều khiển báo lỗi');
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
          const d = Math.hypot(x.x + x.dx * dot * k - y.x - y.dx * dot * m, x.y + x.dy * dot * k - y.y - y.dy * dot * m);
          if (d < gan_nhat) gan_nhat = d;
        }
      }
    }
  }
  const th = mp.tongHop(), tk = mp.thong_ke[che_do];
  // hai xe coi là đè lên nhau khi hai "đốt" gần hơn bề rộng xe
  kiem(gan_nhat >= mp.xe_rong, nhan + ': hai xe đè lên nhau, hai đốt cách ' + gan_nhat.toFixed(1) + ' mm');
  kiem(vuot_do === 0, nhan + ': ' + vuot_do + ' xe vượt đèn đỏ');
  kiem(dung_lau_nhat < 300000, nhan + ': có xe đứng yên ' + (dung_lau_nhat / 1000).toFixed(0) + ' s liền');
  kiem(mp.so_lan_chan_nhau === 0, nhan + ': có xe chặn lẫn nhau');
  kiem(tk.uu_tien === ut, nhan + ': xe ưu tiên chưa qua hết (' + tk.uu_tien + '/' + ut + ')');
  console.log(nhan.padEnd(48) + ' gần nhất ' + gan_nhat.toFixed(1).padStart(5) + ' mm | qua ' + String(th.da_qua).padStart(4) +
    ' | kẹt ' + String(th.con_ket).padStart(3) + ' | chờ TB ' + th.cho_tb.toFixed(1).padStart(5) + ' s | lâu nhất ' + th.cho_max.toFixed(0).padStart(3) +
    ' s | xe ưu tiên TB ' + (tk.tong_uu_tien / Math.max(1, tk.uu_tien) / 1000).toFixed(1).padStart(4) + ' s | đứng yên lâu nhất ' + (dung_lau_nhat / 1000).toFixed(0) + ' s');
}
for (const [ten, cai] of Object.entries(KIEU)) {
  chayAnToan(ten, cai, 0, QD.KIEU_RIENG, 75);
  chayAnToan(ten, cai, 1, QD.KIEU_TU_HOC, 75);
}
for (const co_xe of [60, 100]) {
  chayAnToan('Giờ cao điểm', KIEU['Giờ cao điểm'], 1, QD.KIEU_TU_HOC, co_xe);
  chayAnToan('Đều, vừa phải', KIEU['Đều, vừa phải'], 1, QD.KIEU_THANG_TRUOC, co_xe);
}
chayAnToan('Đều, vừa phải', KIEU['Đều, vừa phải'], 1, QD.KIEU_DOI_XUNG, 75);
{
  const sc = [60, 75, 100].map(c => new MoPhong({ cai: { co_xe: c } }).suc_chua);
  kiem(sc.join() === '6,5,4', 'sức chứa mỗi làn ở cỡ xe 60, 75, 100 % phải là 6, 5, 4 xe; đang là ' + sc.join());
}

// ---------- 2) Trễ phản ứng của người lái ----------
// Chế độ cố định; thả 4 xe rẽ trái vào làn trong Hướng 1 ngay sau khi pha rẽ trái vừa hết, để cả 4
// xếp hàng chờ gần trọn một chu kì. Khi đèn rẽ trái xanh lại, đo lúc từng xe qua vạch dừng.
function xaHang(tre) {
  const cai = { luu_luong: [0, 0, 0, 0, 0], ti_le_trai: [0, 0, 0, 0, 0], ti_le_phai: 0, toc_do: 120, tre_phan_ung: tre };
  const ts = QD.thamSoMacDinh(); ts.che_do = QD.CHE_DO_CO_DINH;
  const mp = new MoPhong({ hat: 3, cai, ts });
  let t_xanh = -1;
  const qua = [], da = new Set();
  for (let i = 0; i < 200 * 25 && qua.length < 4; i++) {
    if (i === 20 * 25) for (let k = 0; k < 4; k++) mp.hang_cho[1].push({ t_sinh: mp.t, huong_di: 'trai', uu_tien: 0 });
    mp.buoc(40);
    if (t_xanh < 0 && mp.t > 40000 && mp.dk.mauNhom(1, mp.t) === DieuKhien.MAU_XANH) {
      t_xanh = mp.t;
      kiem(mp.xe.length === 4 && mp.xe.every(x => x.v < 10 && !x.da_qua), 'cả 4 xe phải đang đứng chờ lúc đèn rẽ trái xanh');
    }
    for (const x of mp.xe) if (x.da_qua && !da.has(x.id)) { da.add(x.id); qua.push((mp.t - t_xanh) / 1000); }
  }
  return qua;
}
{
  const khong = xaHang(0), co = xaHang(1000);
  const cach = q => ((q[3] - q[0]) / 3);
  console.log('\nXả hàng 4 xe sau khi đèn xanh (giây, tính từ lúc xanh), cỡ xe 75 %:');
  console.log('  không trễ phản ứng: ' + khong.map(v => v.toFixed(2)).join(', ') + '   cách nhau TB ' + cach(khong).toFixed(2) + ' s');
  console.log('  trễ phản ứng 1 s  : ' + co.map(v => v.toFixed(2)).join(', ') + '   cách nhau TB ' + cach(co).toFixed(2) + ' s');
  kiem(khong.length === 4 && co.length === 4, 'cả 4 xe phải qua được vạch dừng');
  kiem(co[0] - khong[0] > 0.6 && co[0] - khong[0] < 1.4, 'xe đầu hàng phải trễ khoảng 1 s so với khi không có trễ phản ứng');
  kiem(cach(co) - cach(khong) > 0.5, 'các xe sau phải khởi hành lần lượt, cách nhau thêm ít nhất 0,5 s');
}

// ---------- 3) Xe ưu tiên: qua rồi là chuyển vàng ngay ----------
{
  const ts = QD.thamSoMacDinh(); ts.kieu_pha = QD.KIEU_RIENG;
  const mp = new MoPhong({ hat: 5, ts, cai: JSON.parse(JSON.stringify(KIEU['Đều, vừa phải'])) });
  let t_qua = -1, t_het = -1, t_xanh_uu_tien = 0, dang = false;
  mp.khiSuKien = (loai, dl) => { if (loai === 'uu_tien_qua') t_qua = mp.t; if (loai === 'het_uu_tien' && t_het < 0) t_het = mp.t; };
  for (let i = 0; i < 6 * 60 * 25; i++) {
    if (i === 3000) mp.thaXeUuTien(2, 1);
    mp.buoc(40);
    const mo = mp.dk.ut_huong === 2 && mp.dk.pha === DieuKhien.PHA_XANH;
    if (mo) t_xanh_uu_tien += 40;
    dang = dang || mo;
  }
  console.log('\nXe cứu hỏa ở Hướng 2: qua vạch dừng lúc ' + (t_qua / 1000).toFixed(1) + ' s, hết ưu tiên lúc ' + (t_het / 1000).toFixed(1) +
    ' s, đèn mở riêng tổng cộng ' + (t_xanh_uu_tien / 1000).toFixed(1) + ' s');
  kiem(dang && t_qua > 0 && t_het > 0, 'phải có lần mở riêng cho xe cứu hỏa và xe phải qua được');
  kiem(t_het - t_qua <= 1600, 'xe ưu tiên qua vạch dừng rồi thì trong khoảng 1,5 s phải hết ưu tiên; đang là ' + ((t_het - t_qua) / 1000).toFixed(1) + ' s');
}

// ---------- 4) Bộ tự học kiểu chia pha ----------
// Chạy 90 phút, đo phần thời gian mỗi trục dùng kiểu "thẳng trước" trong 45 phút cuối.
function tiLeThangTruoc(p_trai, hat) {
  const cai = { luu_luong: [0, 6, 6, 6, 6], ti_le_trai: [0, p_trai[0], p_trai[1], p_trai[0], p_trai[1]], ti_le_phai: 15, toc_do: 120 };
  const mp = new MoPhong({ hat, ts: QD.thamSoMacDinh(), cai });
  let n = 0, tt = [0, 0], thu = 0;
  mp.khiSuKien = (loai) => { if (loai === 'hoc_thu') thu++; };
  for (let i = 0; i < 90 * 1500; i++) {
    mp.buoc(40);
    if (i >= 45 * 1500 && i % 250 === 0) { const k = mp.dk.kieuChoChuKi(); n++; if (k[0] === QD.KIEU_THANG_TRUOC) tt[0]++; if (k[1] === QD.KIEU_THANG_TRUOC) tt[1]++; }
  }
  return { doc: tt[0] / n, ngang: tt[1] / n, thu, cho: mp.tongHop().cho_tb };
}
{
  console.log('\nTự học, 6 xe/phút mỗi hướng: phần thời gian dùng kiểu "thẳng trước" trong 45 phút cuối');
  const ca = [['không ai rẽ trái', [0, 0]], ['25 % rẽ trái', [25, 25]], ['dọc 0 %, ngang 30 %', [0, 30]]];
  const kq = ca.map(([ten, p]) => {
    let doc = 0, ngang = 0, thu = 0;
    for (const hat of [1, 2, 3]) { const r = tiLeThangTruoc(p, hat); doc += r.doc / 3; ngang += r.ngang / 3; thu += r.thu; }
    console.log('  ' + ten.padEnd(22) + 'trục dọc ' + (doc * 100).toFixed(0).padStart(3) + ' %, trục ngang ' + (ngang * 100).toFixed(0).padStart(3) + ' %   (' + thu + ' lần thử trong 3 lần chạy)');
    return { doc, ngang, thu };
  });
  kiem(kq[0].doc > 0.6 && kq[0].ngang > 0.6, 'không ai rẽ trái: cả hai trục phải dùng "thẳng trước" phần lớn thời gian');
  kiem(kq[1].doc < 0.25 && kq[1].ngang < 0.25, '25 % rẽ trái: cả hai trục phải ở "riêng từng hướng" phần lớn thời gian');
  kiem(kq[2].doc > 0.5 && kq[2].ngang < 0.3, 'dọc không rẽ trái, ngang rẽ nhiều: dọc "thẳng trước", ngang "riêng"');
  kiem(kq.every(k => k.thu >= 3), 'bộ tự học phải có chạy thử');
}

// ---------- 5) Bảng so sánh (60 phút mô phỏng, 3 hạt giống) ----------
console.log('\nSo sánh thời gian chờ trung bình (giây), 60 phút mô phỏng, trung bình 3 lần chạy, cỡ xe 75 %:');
console.log('Kiểu dòng xe'.padEnd(24) + 'Cố định'.padStart(9) + 'Riêng'.padStart(9) + 'Thẳng trước'.padStart(13) + 'Đối xứng'.padStart(10) + 'Tự học'.padStart(9) + '   Tự học so với cố định');
for (const [ten, cai] of Object.entries(KIEU)) {
  const o = [0, 0, 0, 0, 0];
  for (const hat of [1, 2, 3]) {
    [QD.KIEU_RIENG, QD.KIEU_THANG_TRUOC, QD.KIEU_DOI_XUNG, QD.KIEU_TU_HOC].forEach((k, i) => {
      const ts = QD.thamSoMacDinh(); ts.kieu_pha = k;
      const kq = MoPhong.soSanh(cai, ts, 60, hat, i === 0 ? null : 1);
      if (i === 0) o[0] += kq[0].cho_tb / 3;
      o[i + 1] += kq[1].cho_tb / 3;
    });
  }
  console.log(ten.padEnd(24) + o[0].toFixed(1).padStart(9) + o[1].toFixed(1).padStart(9) + o[2].toFixed(1).padStart(13) + o[3].toFixed(1).padStart(10) +
    o[4].toFixed(1).padStart(9) + '   ' + ((o[4] - o[0]) / o[0] * 100).toFixed(0).padStart(4) + ' %');
}
console.log(so_loi === 0 ? '\nĐẠT: không có lỗi.' : '\nKHÔNG ĐẠT: ' + so_loi + ' lỗi.');
process.exit(so_loi === 0 ? 0 : 1);
