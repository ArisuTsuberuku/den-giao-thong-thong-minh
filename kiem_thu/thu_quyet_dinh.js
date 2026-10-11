// thu_quyet_dinh.js — Kiểm tra bản JavaScript của bộ quyết định:
//  1) kế hoạch đúng với các ca ở mục 6 của TIEN_DO_DU_AN.md và với từng kiểu chia pha;
//  2) cắt xanh sớm; 3) xe ưu tiên theo thứ tự hỏa → sự → công → thương;
//  4) chạy ngẫu nhiên rất nhiều tình huống và kiểm tra các luật an toàn của đèn.
// Chạy: node kiem_thu/thu_quyet_dinh.js
'use strict';
const QD = require('../web/quyet_dinh.js');
const DieuKhien = require('../web/dieu_khien.js');

let so_loi = 0;
function kiem(dung, ten) { if (!dung) { so_loi++; console.log('  SAI: ' + ten); } }

// ---------- 1) Kế hoạch ----------
const ok = [false, true, true, true, true];
const moTa = kh => kh.map(b => QD.tenToHop(b.mat_na) + ' ' + b.xanh / 1000 + ' s' + (b.ngoai ? '*' : '')).join(' | ');
// Một vòng = lượt của trục dọc rồi lượt của trục ngang, cùng một bộ số đếm.
function motVong(ts, s, l, kieu) {
  if (ts.che_do === QD.CHE_DO_CO_DINH) return QD.lapKeHoach(ts, [0].concat(s), [0].concat(l), ok, kieu, 0);
  let kh = [];
  for (const truc of [0, 1]) {
    const luot = QD.lapKeHoach(ts, [0].concat(s), [0].concat(l), ok, kieu, truc);
    if (QD.trucCuaMatNa(luot[0].mat_na) === truc) kh = kh.concat(luot);   // trục trống thì bộ lập kế hoạch nhảy sang trục kia: bỏ qua
  }
  return kh;
}
function ca(ten, che_do, kieu, s, l, vong_dung, to_hop_dung) {
  const ts = QD.thamSoMacDinh(); ts.che_do = che_do;
  const kh = motVong(ts, s, l, [kieu, kieu]);
  const vong = kh.reduce((a, b) => a + b.xanh + ts.t_vang + ts.t_do, 0) / 1000;
  console.log(ten + ': một vòng ' + vong + ' s  [' + moTa(kh) + ']');
  kiem(vong === vong_dung, ten + ' phải có một vòng dài ' + vong_dung + ' s');
  if (to_hop_dung) kiem(moTa(kh) === to_hop_dung, ten + ' phải là: ' + to_hop_dung);
  kh.forEach(b => kiem(QD.hopLeMatNa(b.mat_na), ten + ': tổ hợp xung đột'));
}
const R = QD.KIEU_RIENG, T = QD.KIEU_THANG_TRUOC, D = QD.KIEU_DOI_XUNG;
ca('Không có xe', 1, R, [0, 0, 0, 0], [0, 0, 0, 0], 40, 'H1 thẳng + H3 thẳng 15 s | H2 thẳng + H4 thẳng 15 s');
ca('Cố định', 0, R, [2, 2, 2, 2], [1, 1, 1, 1], 110,
   'H1 trái + H3 trái 15 s | H2 thẳng + H4 thẳng 30 s | H2 trái + H4 trái 15 s | H1 thẳng + H3 thẳng 30 s');
ca('Dọc đông, ngang vắng (riêng)', 1, R, [3, 1, 2, 0], [2, 0, 1, 0], 90,
   'H1 thẳng và trái 30 s | H3 thẳng và trái 25 s | H2 thẳng và trái 20 s');
ca('Đông nhất, 4 xe mọi làn (riêng)', 1, R, [4, 4, 4, 4], [4, 4, 4, 4], 160,
   'H1 thẳng và trái 35 s | H3 thẳng và trái 35 s | H2 thẳng và trái 35 s | H4 thẳng và trái 35 s');
ca('Chỉ Hướng 1 có xe', 1, R, [3, 0, 0, 0], [2, 0, 0, 0], 35, 'H1 thẳng và trái 30 s');
ca('Hướng đông đi trước (riêng)', 1, R, [1, 0, 4, 0], [1, 0, 4, 0], 65, 'H3 thẳng và trái 35 s | H1 thẳng và trái 20 s');
ca('Chỉ đi thẳng', 1, R, [2, 0, 3, 0], [0, 0, 0, 0], 35, 'H1 thẳng + H3 thẳng 30 s');
// "*" đánh dấu bước cùng đi thẳng chỉ tính xe làn ngoài khi xét cắt sớm
ca('Thẳng trước', 1, T, [4, 0, 2, 0], [1, 0, 3, 0], 100, 'H1 thẳng + H3 thẳng 35 s* | H3 thẳng và trái 30 s | H1 thẳng và trái 20 s');
ca('Thẳng trước, một làn trong trống', 1, T, [2, 0, 3, 0], [1, 0, 0, 0], 60, 'H1 thẳng + H3 thẳng 30 s* | H1 thẳng và trái 20 s');
ca('Đối xứng', 1, D, [4, 0, 2, 0], [1, 0, 3, 0], 75, 'H1 thẳng + H3 thẳng 35 s | H1 trái + H3 trái 30 s');
ca('Đối xứng, một làn trong trống', 1, D, [2, 0, 3, 0], [1, 0, 0, 0], 60, 'H1 thẳng + H3 thẳng 30 s | H1 thẳng và trái 20 s');
{
  // tới lượt trục dọc nhưng trục dọc trống: lập luôn cho trục ngang
  const ts = QD.thamSoMacDinh();
  const kh = QD.lapKeHoach(ts, [0, 0, 2, 0, 0], [0, 0, 1, 0, 0], ok, [R, R], 0);
  kiem(moTa(kh) === 'H2 thẳng và trái 25 s', 'trục tới lượt trống thì lập cho trục kia, đang là: ' + moTa(kh));
  // camera Hướng 3 mất tín hiệu: trục dọc chạy thời gian cố định
  const kh2 = QD.lapKeHoach(ts, [0, 3, 0, 0, 0], [0, 1, 0, 0, 0], [false, true, true, false, true], [R, R], 0);
  kiem(moTa(kh2) === 'H1 thẳng + H3 thẳng 30 s | H1 trái + H3 trái 15 s', 'mất camera thì trục đó chạy cố định, đang là: ' + moTa(kh2));
}

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

// ---------- 2) Cắt xanh sớm ----------
// Chạy bộ điều khiển với số đếm do hàm dem(t) cho (mỗi hướng [s, l] hoặc [s, l, ut]).
// Trả về: vet = [giây, tổ hợp vừa xanh]; sk = [giây, loại sự kiện, dữ liệu].
function chay(ts, dem, den_giay) {
  const dk = new DieuKhien(ts), vet = [], sk = [];
  dk.khiSuKien = (loai, dl) => {
    if (loai === 'buoc') vet.push([Math.round(dk.t_xanh_tu / 100) / 10, QD.tenToHop(dk.xanh)]);
    else sk.push([Math.round(t_nay / 100) / 10, loai, dl]);
  };
  let t_nay = 0;
  dk.khoiDong(0);
  for (let t = 0; t <= den_giay * 1000; t += 100) {
    t_nay = t;
    const d = dem(t);
    for (let h = 1; h <= 4; h++) dk.nhanDem(h, d[h - 1][0], d[h - 1][1], 0, t, false, d[h - 1][2] || 0);
    dk.capNhat(t);
  }
  return { vet, sk, cat: sk.filter(x => x[1] === 'cat_som').length, dk };
}
const tsRieng = () => { const ts = QD.thamSoMacDinh(); ts.kieu_pha = QD.KIEU_RIENG; return ts; };
{
  // H1 có 3 xe làn ngoài, đi hết ở giây thứ 8; H2 có 2 xe chờ suốt.
  const dem = t => [[t < 8000 ? 3 : 0, 0], [2, 0], [0, 0], [0, 0]];
  let kq = chay(tsRieng(), dem, 40);
  console.log('Cắt sớm 5 s: ' + kq.vet.map(v => v[0] + ' s ' + v[1]).join(' | '));
  kiem(kq.vet[0][0] === 2 && kq.vet[0][1] === 'H1 thẳng và trái', 'bước đầu phải là riêng H1 lúc 2 s');
  // hết xe lúc 8 s → cắt lúc 13 s → vàng 3 s + đỏ 2 s → H2 xanh lúc 18 s (không cắt thì phải tới 37 s)
  kiem(kq.vet[1][0] === 18 && kq.vet[1][1] === 'H2 thẳng và trái', 'H2 phải được xanh lúc 18 s nhờ cắt sớm, đang là ' + kq.vet[1][0]);
  kiem(kq.cat >= 1, 'phải có sự kiện cắt sớm');
  let ts = tsRieng(); ts.cat_som = 0;
  kq = chay(ts, dem, 40);
  kiem(kq.vet[1][0] === 37 && kq.cat === 0, 'tắt cắt sớm: H2 xanh lúc 37 s, đang là ' + kq.vet[1][0]);
  ts = tsRieng(); ts.che_do = QD.CHE_DO_CO_DINH;
  kq = chay(ts, dem, 60);
  kiem(kq.cat === 0, 'chế độ cố định không bao giờ cắt sớm');
}
{
  // Chỉ H1 có xe rồi đi hết, không ai chờ ở hướng khác: không cắt, đèn H1 cứ giữ xanh.
  const kq = chay(tsRieng(), t => [[t < 8000 ? 3 : 0, 0], [0, 0], [0, 0], [0, 0]], 30);
  kiem(kq.cat === 0, 'không có xe nào chờ ở hướng khác thì không cắt');
}
{
  // Kế hoạch chốt theo từng lượt: lượt dọc xong mới chốt lượt ngang, lúc đó H2 đã có thêm xe.
  const kq = chay(tsRieng(), t => [[t < 8000 ? 3 : 0, 0], [t < 10000 ? 1 : 4, 0], [0, 0], [0, 0]], 60);
  const k = kq.sk.filter(x => x[1] === 'chu_ki').length;
  kiem(kq.dk.kh.length === 1 && kq.dk.kh[0].xanh === 35000, 'lượt ngang phải chốt theo 4 xe lúc bắt đầu lượt (35 s), đang là ' + moTa(kq.dk.kh));
  kiem(k >= 2, 'phải có ít nhất 2 lần chốt kế hoạch');
}
{
  // Thẳng trước: hai làn ngoài hết xe là cắt bước cùng đi thẳng, dù làn trong H1 còn xe (bị xe rẽ trái chặn).
  const ts = QD.thamSoMacDinh(); ts.kieu_pha = QD.KIEU_THANG_TRUOC;
  const kq = chay(ts, t => [[t < 8000 ? 2 : 0, t < 25000 ? 2 : 0], [0, 0], [t < 8000 ? 2 : 0, 0], [0, 0]], 50);
  console.log('Thẳng trước: ' + kq.vet.map(v => v[0] + ' s ' + v[1]).join(' | '));
  kiem(kq.vet[0][1] === 'H1 thẳng + H3 thẳng', 'bước đầu phải là cùng đi thẳng');
  // làn ngoài hết lúc 8 s → cắt lúc 13 s → H3 thẳng qua vàng, H1 thẳng giữ xanh → H1 trái xanh lúc 18 s
  kiem(kq.vet[1][0] === 18 && kq.vet[1][1] === 'H1 thẳng và trái', 'phải mở riêng H1 lúc 18 s, đang là ' + kq.vet[1].join(' '));
  kiem(kq.dk.mauNhom(0, 15000) !== undefined, 'đọc màu được');
}
{
  // Đối xứng: bước rẽ trái không còn xe ở làn trong thì bỏ qua.
  const ts = QD.thamSoMacDinh(); ts.kieu_pha = QD.KIEU_DOI_XUNG;
  const kq = chay(ts, t => [[t < 6000 ? 2 : 0, t < 6000 ? 1 : 0], [1, 0], [t < 6000 ? 1 : 0, 0], [0, 0]], 40);
  console.log('Bỏ bước trống: ' + kq.vet.map(v => v[0] + ' s ' + v[1]).join(' | '));
  kiem(kq.vet[0][1] === 'H1 thẳng + H3 thẳng' && kq.vet[1][1] === 'H2 thẳng và trái', 'bước mở riêng H1 phải bị bỏ vì làn trong đã hết xe');
  kiem(kq.vet[1][0] === 16, 'H2 phải xanh lúc 16 s (hết xe 6 s, cắt 11 s, vàng và đỏ 5 s), đang là ' + kq.vet[1][0]);
}
{
  // Camera của hướng đang xanh mất tín hiệu: không biết còn xe hay không nên không cắt.
  const dk = new DieuKhien(tsRieng());
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

// ---------- 3) Xe ưu tiên ----------
const ut = (kq, loai) => kq.sk.filter(x => x[1] === loai);
{
  // Xe cứu thương ở H1 từ giây 20 tới giây 28. Xe qua rồi (hết thấy 1 s) là chuyển vàng ngay.
  const kq = chay(tsRieng(), t => [[2, 0, t >= 20000 && t < 28000 ? 4 : 0], [2, 0], [0, 0], [0, 0]], 60);
  const bd = ut(kq, 'uu_tien'), het = ut(kq, 'het_uu_tien');
  console.log('Ưu tiên: bắt đầu ' + bd.map(x => x[0] + ' s H' + x[2]).join(', ') + '; hết ' + het.map(x => x[0] + ' s').join(', '));
  kiem(bd.length === 1 && bd[0][2] === 1, 'phải ưu tiên Hướng 1 đúng một lần');
  kiem(het.length === 1 && het[0][0] >= 28.8 && het[0][0] <= 29.1, 'hết thấy xe lúc 28 s thì khoảng 29 s phải kết thúc ưu tiên, đang là ' + (het[0] && het[0][0]));
  // sau ưu tiên Hướng 1 (trục dọc), tới lượt trục ngang vì trục này đã phải chờ
  const sau = kq.vet.find(v => v[0] > 29);
  kiem(sau && sau[1] === 'H2 thẳng và trái', 'sau ưu tiên trục dọc phải tới lượt trục ngang, đang là ' + (sau && sau[1]));
}
{
  // Bản tin kiểu cũ chỉ có emg, không có ut: coi là xe cứu thương.
  const dk = new DieuKhien(tsRieng());
  dk.nhanDem(2, 1, 0, 1, 0, false);
  kiem(dk.dem[2].ut === 4, 'emg khác 0 mà không có ut thì ut = 4 (cứu thương)');
  dk.nhanDem(2, 1, 0, 0, 0, false, 1);
  kiem(dk.dem[2].ut === 1 && dk.dem[2].emg === 1, 'có ut mà emg = 0 thì vẫn nhận, emg = 1');
  dk.nhanDem(2, 1, 0, 0, 0, false, 9);
  kiem(dk.dem[2].ut === 0, 'ut ngoài 1..4 thì bỏ');
}
{
  // Hỏa → sự → công → thương: 4 xe ưu tiên xuất hiện cùng lúc ở 4 hướng, mỗi xe đứng chờ tới khi
  // hướng của nó được mở riêng 3 giây rồi mới đi mất.
  const loai = [4, 2, 1, 3];              // H1 cứu thương, H2 quân sự, H3 cứu hỏa, H4 công an
  const ts = tsRieng(), dk = new DieuKhien(ts), thu_tu = [];
  dk.khiSuKien = (l, dl) => { if (l === 'uu_tien') thu_tu.push(dl); };
  dk.khoiDong(0);
  const con = [true, true, true, true], mo_tu = [0, 0, 0, 0];
  let nhom_xung_dot = false;
  for (let t = 0; t <= 120000; t += 100) {
    for (let h = 1; h <= 4; h++) {
      const dang_mo = dk.pha === DieuKhien.PHA_XANH && dk.xanh === QD.matNaRieng(h);
      if (dang_mo && !mo_tu[h - 1]) mo_tu[h - 1] = t;
      if (dang_mo && t - mo_tu[h - 1] >= 3000) con[h - 1] = false;
      dk.nhanDem(h, 1, 0, 0, t, false, t >= 20000 && con[h - 1] ? loai[h - 1] : 0);
    }
    dk.capNhat(t);
    if (!QD.hopLeMatNa(dk.xanh | dk.vang)) nhom_xung_dot = true;
  }
  console.log('Thứ tự mở đèn ưu tiên: ' + thu_tu.map(h => 'H' + h + ' (' + ['', 'cứu hỏa', 'quân sự', 'công an', 'cứu thương'][loai[h - 1]] + ')').join(' → '));
  kiem(thu_tu.join() === '3,2,4,1', 'phải mở theo thứ tự cứu hỏa, quân sự, công an, cứu thương (H3, H2, H4, H1), đang là ' + thu_tu.join());
  kiem(!nhom_xung_dot, 'trong lúc ưu tiên không được có tổ hợp xung đột');
}
{
  // Đang mở cho xe cứu thương ở H1 (xe bị kẹt, chưa đi được) thì xe cứu hỏa tới H2: nhường xe cứu hỏa,
  // xong lại mở tiếp cho xe cứu thương.
  const kq = chay(tsRieng(), t => [[1, 0, t >= 20000 && t < 70000 ? 4 : 0], [1, 0, t >= 35000 && t < 50000 ? 1 : 0], [0, 0], [0, 0]], 90);
  const bd = ut(kq, 'uu_tien').map(x => x[2]);
  console.log('Nhường xe hạng cao hơn: ' + ut(kq, 'uu_tien').map(x => x[0] + ' s H' + x[2]).join(' → '));
  kiem(bd.join() === '1,2,1', 'phải là H1, rồi H2 (cứu hỏa), rồi lại H1; đang là ' + bd.join());
}
{
  // Xe hạng thấp hơn tới sau thì không được chen: cứu hỏa ở H1 đang đi, cứu thương tới H2 phải chờ.
  const kq = chay(tsRieng(), t => [[1, 0, t >= 20000 && t < 50000 ? 1 : 0], [1, 0, t >= 30000 && t < 65000 ? 4 : 0], [0, 0], [0, 0]], 90);
  const bd = ut(kq, 'uu_tien');
  kiem(bd.map(x => x[2]).join() === '1,2' && bd[1][0] >= 50, 'cứu thương ở H2 chỉ được mở sau khi xe cứu hỏa đi (50 s), đang là ' + bd.map(x => x[0] + ' s H' + x[2]).join(', '));
}

// ---------- 4) Kiểu chia pha và bộ tự học ----------
{
  const ts = QD.thamSoMacDinh(), dk = new DieuKhien(ts);
  ts.kieu_pha = QD.KIEU_DOI_XUNG; kiem(dk.kieuChoChuKi().join() === '3,3', 'chọn tay đối xứng');
  ts.kieu_pha = 99; kiem(dk.kieuChoChuKi().join() === '1,1', 'giá trị lạ thì dùng riêng từng hướng');
  ts.kieu_pha = QD.KIEU_TU_HOC; kiem(dk.kieuChoChuKi().join() === '1,1', 'tự học bắt đầu từ riêng từng hướng');
  dk.hoc.dang_thu = true; dk.hoc.truc_thu = 1;
  kiem(dk.kieuChoChuKi().join() === '1,2', 'đang thử trục ngang thì trục ngang dùng kiểu thẳng trước');
}
{
  // Xe đứng yên (số đếm không đổi) thì không có xe nào qua: bộ tự học không được kết luận gì.
  const ts = QD.thamSoMacDinh();
  const kq = chay(ts, () => [[2, 1], [2, 1], [2, 1], [2, 1]], 900);
  kiem(kq.sk.filter(x => x[1].indexOf('hoc') === 0).length === 0 && kq.dk.hoc.diem_giu === -1, 'không có xe qua thì không thử, không có điểm');
}
{
  // Có xe ưu tiên trong lúc đo: cửa sổ đó bị bỏ.
  const ts = QD.thamSoMacDinh(), dk = new DieuKhien(ts);
  dk.khoiDong(0);
  for (let t = 0; t <= 60000; t += 100) {
    for (let h = 1; h <= 4; h++) dk.nhanDem(h, (t / 1000 | 0) % 3, 0, 0, t, false, h === 1 && t > 30000 && t < 55000 ? 4 : 0);
    dk.capNhat(t);
    if (t === 50000) kiem(dk.ut_huong === 1 && dk.hoc.hong, 'đang có xe ưu tiên thì cửa sổ đo phải bị đánh dấu hỏng');
  }
}

// ---------- 5) Chạy ngẫu nhiên, kiểm tra luật an toàn ----------
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

const dem_sk = {};
for (let lan = 0; lan < 400; lan++) {
  const r = taoNgauNhien(1000 + lan);
  const ng = (a, b) => a + Math.floor(r() * (b - a + 1));
  const ts = QD.thamSoMacDinh();
  ts.che_do = ng(0, 1); ts.t_min = ng(5, 30) * 1000; ts.t_max = ts.t_min + ng(0, 60) * 1000;
  ts.cong_them = ng(0, 10) * 1000; ts.xe_quy_doi = ng(1, 20); ts.t_vang = ng(2, 5) * 1000; ts.t_do = ng(0, 4) * 1000;
  ts.cat_som = lan % 3 === 0 ? 0 : ng(2, 10) * 1000;
  ts.kieu_pha = ng(0, 3); ts.hoc_so_chu_ki = ng(1, 2); ts.hoc_so_xe = ng(1, 6); ts.hoc_nguong = ng(0, 30);
  ts.ut_giu = ng(0, 6) * 500;
  const co_ut = lan % 4 !== 1;                // một phần tư số lần không có xe ưu tiên, để bộ tự học được chạy
  const dk = new DieuKhien(ts);
  dk.khiSuKien = (loai) => { dem_sk[loai] = (dem_sk[loai] || 0) + 1; };
  dk.khoiDong(0);
  const mau_truoc = new Array(8).fill(0), doi_luc = new Array(8).fill(0);
  let do_tu = new Array(8).fill(0);         // lúc nhóm chuyển sang đỏ
  const emg_toi = [0, 0, 0, 0, 0], im_toi = [0, 0, 0, 0, 0], loai_ut = [0, 1, 1, 1, 1];
  const cs = [0, 0, 0, 0, 0], cl = [0, 0, 0, 0, 0];   // số đếm giữ nguyên một lúc rồi mới đổi, để có lúc hết xe đủ lâu
  let t = 0;
  while (t < 900000) {
    t += ng(20, 400);
    for (let h = 1; h <= 4; h++) {
      if (co_ut && r() < 0.002) im_toi[h] = t + ng(1, 20) * 1000;  // camera im một lúc
      if (co_ut && r() < 0.0015) { emg_toi[h] = t + ng(1, 90) * 1000; loai_ut[h] = ng(1, 4); }   // xe ưu tiên xuất hiện
      if (t < im_toi[h]) continue;
      if (r() < 0.04) { cs[h] = ng(0, 5) * (r() < 0.5 ? 1 : 0); cl[h] = ng(0, 5) * (r() < 0.4 ? 1 : 0); }
      dk.nhanDem(h, cs[h], cl[h], 0, t, false, t < emg_toi[h] ? loai_ut[h] : 0);
    }
    if (r() < 0.0005) ts.che_do = 1 - ts.che_do;                    // đổi chế độ giữa chừng
    if (r() < 0.0005) ts.kieu_pha = ng(0, 3);                       // đổi kiểu chia pha giữa chừng
    dk.capNhat(t);
    kiem(!dk.loi, 'lần ' + lan + ': bộ điều khiển báo lỗi');
    kiem(QD.hopLeMatNa(dk.xanh | dk.vang), 'lần ' + lan + ' t=' + t + ': đèn xung đột ' + (dk.xanh | dk.vang));
    for (let g = 0; g < 8; g++) {
      const m = dk.mauNhom(g, t);
      if (m === mau_truoc[g]) continue;
      // xanh chỉ được chuyển sang vàng, vàng chỉ sang đỏ, đỏ chỉ sang xanh. Riêng khi đặt đỏ toàn phần
      // bằng 0 thì một nhóm vừa vàng xong có thể xanh lại ngay (ưu tiên bị hủy giữa chừng), vì pha đỏ dài 0 giây.
      kiem((mau_truoc[g] === 2 && m === 1) || (mau_truoc[g] === 1 && m === 0) || (mau_truoc[g] === 0 && m === 2) ||
           (ts.t_do === 0 && mau_truoc[g] === 1 && m === 2),
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
console.log('Chạy ngẫu nhiên: 400 lần × 15 phút, ' + (dem_sk.chu_ki || 0) + ' lần chốt kế hoạch, ' + (dem_sk.uu_tien || 0) + ' lần ưu tiên, ' +
  (dem_sk.cat_som || 0) + ' lần cắt xanh sớm, ' + (dem_sk.hoc_thu || 0) + ' lần tự học thử kiểu khác (' + (dem_sk.hoc_nhan || 0) + ' lần giữ).');
console.log(so_loi === 0 ? 'ĐẠT: không có lỗi.' : 'KHÔNG ĐẠT: ' + so_loi + ' lỗi.');
process.exit(so_loi === 0 ? 0 : 1);
