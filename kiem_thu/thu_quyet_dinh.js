// thu_quyet_dinh.js — Kiểm tra bản JavaScript của bộ quyết định:
//  1) các ca ở mục 6 của TIEN_DO_DU_AN.md cho đúng thời gian chu kì;
//  2) chạy ngẫu nhiên rất nhiều tình huống và kiểm tra các luật an toàn của đèn.
// Chạy: node kiem_thu/thu_quyet_dinh.js
'use strict';
const QD = require('../web/quyet_dinh.js');
const DieuKhien = require('../web/dieu_khien.js');

let so_loi = 0;
function kiem(dung, ten) { if (!dung) { so_loi++; console.log('  SAI: ' + ten); } }

// ---------- 1) Các ca ở mục 6 ----------
const ok = [false, true, true, true, true];
function chuKi(ts, kh) { return kh.reduce((a, b) => a + b.xanh + ts.t_vang + ts.t_do, 0) / 1000; }
function ca(ten, che_do, s, l, chu_ki_dung, to_hop_dung) {
  const ts = QD.thamSoMacDinh(); ts.che_do = che_do;
  const kh = QD.lapKeHoach(ts, [0].concat(s), [0].concat(l), ok);
  const mo_ta = kh.map(b => QD.tenToHop(b.mat_na) + ' ' + b.xanh / 1000 + ' s').join(' | ');
  console.log(ten + ': chu kì ' + chuKi(ts, kh) + ' s  [' + mo_ta + ']');
  kiem(chuKi(ts, kh) === chu_ki_dung, ten + ' phải có chu kì ' + chu_ki_dung + ' s');
  if (to_hop_dung) kiem(mo_ta === to_hop_dung, ten + ' phải là: ' + to_hop_dung);
  kh.forEach(b => kiem(QD.hopLeMatNa(b.mat_na), ten + ': tổ hợp xung đột'));
}
ca('Không có xe', 1, [0, 0, 0, 0], [0, 0, 0, 0], 40, 'H1 thẳng + H3 thẳng 15 s | H2 thẳng + H4 thẳng 15 s');
ca('Cố định', 0, [2, 2, 2, 2], [1, 1, 1, 1], 110,
   'H1 trái + H3 trái 15 s | H2 thẳng + H4 thẳng 30 s | H2 trái + H4 trái 15 s | H1 thẳng + H3 thẳng 30 s');
ca('Dọc đông, ngang vắng', 1, [3, 1, 2, 0], [2, 0, 1, 0], 90,
   'H1 thẳng và trái 30 s | H3 thẳng và trái 25 s | H2 thẳng và trái 20 s');
ca('Đông nhất (4 xe mọi làn)', 1, [4, 4, 4, 4], [4, 4, 4, 4], 160,
   'H1 thẳng và trái 35 s | H3 thẳng và trái 35 s | H2 thẳng và trái 35 s | H4 thẳng và trái 35 s');
ca('Làn trong thưa, làn ngoài đông', 1, [4, 0, 4, 0], [1, 0, 1, 0], 65, 'H1 thẳng + H3 thẳng 35 s | H1 trái + H3 trái 20 s');
ca('Chỉ Hướng 1 có xe', 1, [3, 0, 0, 0], [2, 0, 0, 0], 35, 'H1 thẳng và trái 30 s');
ca('Hai hướng lệch nhau', 1, [4, 0, 1, 0], [4, 0, 1, 0], 65, 'H1 thẳng và trái 35 s | H3 thẳng và trái 20 s');
ca('Chỉ đi thẳng', 1, [2, 0, 3, 0], [0, 0, 0, 0], 35, 'H1 thẳng + H3 thẳng 30 s');
ca('Một bên rẽ trái', 1, [2, 0, 3, 0], [1, 0, 0, 0], 60, 'H1 thẳng + H3 thẳng 30 s | H1 thẳng và trái 20 s');

// thời gian xanh: mỗi xe mô hình cộng 5 s, chặn trong [Tmin, Tmax]
{
  const ts = QD.thamSoMacDinh();
  kiem([0, 1, 2, 3, 4].map(n => QD.thoiGianXanh(ts, n) / 1000).join() === '15,20,25,30,35', 'bảng thời gian xanh 0..4 xe');
  kiem(QD.thoiGianXanh(ts, 99) === 90000, 'chặn trên Tmax');
}

// bảng xung đột: chỉ 8 cặp nhóm được xanh cùng nhau
{
  let hop = 0;
  for (let i = 0; i < 8; i++) for (let j = i + 1; j < 8; j++) if (QD.hopLeMatNa((1 << i) | (1 << j))) hop++;
  kiem(hop === 8, 'phải có đúng 8 cặp không xung đột, đang có ' + hop);
  kiem(!QD.hopLeMatNa(QD.bitTrai(1) | QD.bitThang(3)), 'trái H1 với thẳng H3 phải là xung đột');
  kiem(!QD.hopLeMatNa(QD.bitThang(1) | QD.bitThang(2)), 'thẳng H1 với thẳng H2 phải là xung đột');
}

// ---------- 1b) Cắt xanh sớm khi tổ hợp đang xanh hết xe ----------
// Chạy bộ điều khiển với số đếm do hàm dem(t) cho; trả về danh sách [giây, tổ hợp vừa xanh].
function chay(ts, dem, den_giay) {
  const dk = new DieuKhien(ts), vet = [];
  let cat = 0;
  dk.khiSuKien = (loai) => { if (loai === 'buoc') vet.push([Math.round(dk.t_xanh_tu / 100) / 10, QD.tenToHop(dk.xanh)]); if (loai === 'cat_som') cat++; };
  dk.khoiDong(0);
  for (let t = 0; t <= den_giay * 1000; t += 100) {
    const d = dem(t);
    for (let h = 1; h <= 4; h++) dk.nhanDem(h, d[h - 1][0], d[h - 1][1], 0, t, false);
    dk.capNhat(t);
  }
  return { vet, cat };
}
{
  // H1 có 3 xe làn ngoài, đi hết ở giây thứ 8; H2 có 2 xe chờ suốt.
  const dem = t => [[t < 8000 ? 3 : 0, 0], [2, 0], [0, 0], [0, 0]];
  let ts = QD.thamSoMacDinh();
  let kq = chay(ts, dem, 40);
  console.log('Cắt sớm 5 s: ' + kq.vet.map(v => v[0] + ' s ' + v[1]).join(' | '));
  kiem(kq.vet[0][0] === 2 && kq.vet[0][1] === 'H1 thẳng và trái', 'bước đầu phải là riêng H1 lúc 2 s');
  // hết xe lúc 8 s → cắt lúc 13 s → vàng 3 s + đỏ 2 s → H2 xanh lúc 18 s (không cắt thì phải tới 37 s)
  kiem(kq.vet[1][0] === 18 && kq.vet[1][1] === 'H2 thẳng và trái', 'H2 phải được xanh lúc 18 s nhờ cắt sớm, đang là ' + kq.vet[1][0]);
  kiem(kq.cat >= 1, 'phải có sự kiện cắt sớm');
  ts = QD.thamSoMacDinh(); ts.cat_som = 0;
  kq = chay(ts, dem, 40);
  kiem(kq.vet[1][0] === 37 && kq.cat === 0, 'tắt cắt sớm: H2 xanh lúc 37 s, đang là ' + kq.vet[1][0]);
  ts = QD.thamSoMacDinh(); ts.che_do = QD.CHE_DO_CO_DINH;
  kq = chay(ts, dem, 60);
  kiem(kq.cat === 0, 'chế độ cố định không bao giờ cắt sớm');
}
{
  // Chỉ H1 có xe rồi đi hết, không ai chờ ở hướng khác: không cắt, đèn H1 cứ giữ xanh.
  const kq = chay(QD.thamSoMacDinh(), t => [[t < 8000 ? 3 : 0, 0], [0, 0], [0, 0], [0, 0]], 30);
  kiem(kq.cat === 0, 'không có xe nào chờ ở hướng khác thì không cắt');
}
{
  // Bước kế không còn xe thì bỏ qua: lúc chốt kế hoạch H1 và H2 có xe; xe H2 biến mất ở giây 5.
  const kq = chay(QD.thamSoMacDinh(), t => [[2, 0], [t < 5000 ? 1 : 0, 0], [0, 0], [t > 20000 ? 1 : 0, 0]], 45);
  console.log('Bỏ bước trống: ' + kq.vet.map(v => v[0] + ' s ' + v[1]).join(' | '));
  kiem(!kq.vet.some(v => v[1] === 'H2 thẳng và trái'), 'bước riêng H2 phải bị bỏ vì H2 không còn xe');
}
{
  // Camera của hướng đang xanh mất tín hiệu: không biết còn xe hay không nên không cắt.
  const ts = QD.thamSoMacDinh();
  const dk = new DieuKhien(ts);
  let cat = 0;
  dk.khiSuKien = (loai) => { if (loai === 'cat_som') cat++; };
  dk.khoiDong(0);
  for (let t = 0; t <= 30000; t += 100) {
    if (t < 6000) dk.nhanDem(1, 3, 0, 0, t, false);     // camera H1 im từ giây thứ 6
    dk.nhanDem(2, 2, 0, 0, t, false);
    dk.capNhat(t);
  }
  kiem(cat === 0, 'camera mất tín hiệu thì không cắt sớm');
}

// ---------- 2) Chạy ngẫu nhiên, kiểm tra luật an toàn ----------
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

let so_lan_uu_tien = 0, so_chu_ki = 0, so_lan_cat_som = 0;
for (let lan = 0; lan < 400; lan++) {
  const r = taoNgauNhien(1000 + lan);
  const ng = (a, b) => a + Math.floor(r() * (b - a + 1));
  const ts = QD.thamSoMacDinh();
  ts.che_do = ng(0, 1); ts.t_min = ng(5, 30) * 1000; ts.t_max = ts.t_min + ng(0, 60) * 1000;
  ts.cong_them = ng(0, 10) * 1000; ts.xe_quy_doi = ng(1, 20); ts.t_vang = ng(2, 5) * 1000; ts.t_do = ng(0, 4) * 1000;
  ts.cat_som = lan % 3 === 0 ? 0 : ng(2, 10) * 1000;
  const dk = new DieuKhien(ts);
  dk.khiSuKien = (loai) => { if (loai === 'uu_tien') so_lan_uu_tien++; if (loai === 'chu_ki') so_chu_ki++; if (loai === 'cat_som') so_lan_cat_som++; };
  dk.khoiDong(0);
  const mau_truoc = new Array(8).fill(0), doi_luc = new Array(8).fill(0);
  let do_tu = new Array(8).fill(0);         // lúc nhóm chuyển sang đỏ
  const emg_toi = [0, 0, 0, 0, 0], im_toi = [0, 0, 0, 0, 0];
  const cs = [0, 0, 0, 0, 0], cl = [0, 0, 0, 0, 0];   // số đếm giữ nguyên một lúc rồi mới đổi, để có lúc hết xe đủ lâu
  let t = 0;
  while (t < 900000) {
    t += ng(20, 400);
    for (let h = 1; h <= 4; h++) {
      if (r() < 0.002) im_toi[h] = t + ng(1, 20) * 1000;           // camera im một lúc
      if (r() < 0.0015) emg_toi[h] = t + ng(1, 90) * 1000;          // xe ưu tiên xuất hiện
      if (t < im_toi[h]) continue;
      if (r() < 0.04) { cs[h] = ng(0, 5) * (r() < 0.5 ? 1 : 0); cl[h] = ng(0, 5) * (r() < 0.4 ? 1 : 0); }
      dk.nhanDem(h, cs[h], cl[h], t < emg_toi[h] ? ng(1, 2) : 0, t, false);
    }
    if (r() < 0.0005) ts.che_do = 1 - ts.che_do;                    // đổi chế độ giữa chừng
    dk.capNhat(t);
    kiem(!dk.loi, 'lần ' + lan + ': bộ điều khiển báo lỗi');
    kiem(QD.hopLeMatNa(dk.xanh | dk.vang), 'lần ' + lan + ' t=' + t + ': đèn xung đột ' + (dk.xanh | dk.vang));
    for (let g = 0; g < 8; g++) {
      const m = dk.mauNhom(g, t);
      if (m === mau_truoc[g]) continue;
      // xanh chỉ được chuyển sang vàng, vàng chỉ sang đỏ, đỏ chỉ sang xanh
      kiem((mau_truoc[g] === 2 && m === 1) || (mau_truoc[g] === 1 && m === 0) || (mau_truoc[g] === 0 && m === 2),
           'lần ' + lan + ' t=' + t + ': nhóm ' + g + ' đổi màu sai thứ tự ' + mau_truoc[g] + '→' + m);
      if (mau_truoc[g] === 1) kiem(t - doi_luc[g] >= ts.t_vang - 400, 'lần ' + lan + ': vàng quá ngắn ở nhóm ' + g);
      if (m === 0) do_tu[g] = t;
      if (m === 2) {
        // nhóm vừa xanh: mọi nhóm xung đột với nó phải đỏ ít nhất t_do (trừ sai số một nhịp)
        for (let k = 0; k < 8; k++) {
          if (k === g || QD.hopLeMatNa((1 << g) | (1 << k))) continue;
          kiem(dk.mauNhom(k, t) === 0, 'lần ' + lan + ': nhóm xung đột chưa đỏ');
          if (do_tu[k] > 0) kiem(t - do_tu[k] >= ts.t_do - 400, 'lần ' + lan + ' t=' + t + ': đỏ toàn phần quá ngắn');
        }
      }
      mau_truoc[g] = m; doi_luc[g] = t;
    }
  }
}
console.log('Chạy ngẫu nhiên: 400 lần × 15 phút, ' + so_chu_ki + ' chu kì, ' + so_lan_uu_tien + ' lần ưu tiên, ' + so_lan_cat_som + ' lần cắt xanh sớm.');
console.log(so_loi === 0 ? 'ĐẠT: không có lỗi.' : 'KHÔNG ĐẠT: ' + so_loi + ' lỗi.');
process.exit(so_loi === 0 ? 0 : 1);
