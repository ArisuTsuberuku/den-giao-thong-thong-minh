// giao_dien.js — Vẽ sa bàn, 4 ô camera, bảng thông số và số liệu cho web mô phỏng.
// Phần tính toán nằm ở mo_phong.js (xe), quyet_dinh.js và dieu_khien.js (đèn).
(function () {
  'use strict';
  var QD = QuyetDinh, H = MoPhong.HINH;
  var $ = function (id) { return document.getElementById(id); };

  var NHIP = 40;                 // một nhịp mô phỏng = 40 ms
  var KHOA_LUU = 'sa_ban_den_giao_thong_v4';
  var mp = new MoPhong({ hat: 2026 });
  var ts = mp.ts, cai = mp.cai;
  var tt = { chay: true, toc_do: 2, hien_vung: false, du: 0, t_truoc: 0, khung: 0,
             so_cat_som: 0, ut_loai: 0 };
  var nhat_ky = [];

  // ---------- Thông số trên bảng ----------
  var TRUOT_DEN = [
    { id: 't_min', ten: 'Xanh ngắn nhất (Tmin)', min: 5, max: 60, buoc: 1, dv: ' s', he: 1000 },
    { id: 't_max', ten: 'Xanh dài nhất (Tmax)', min: 30, max: 120, buoc: 5, dv: ' s', he: 1000 },
    { id: 'cong_them', ten: 'Cộng thêm cho mỗi 10 xe thật', min: 0, max: 15, buoc: 1, dv: ' s', he: 1000 },
    { id: 'xe_quy_doi', ten: '1 xe mô hình bằng', min: 1, max: 20, buoc: 1, dv: ' xe thật', he: 1 },
    { id: 'cat_som', ten: 'Cắt xanh sớm khi hết xe quá', min: 0, max: 15, buoc: 1, dv: ' s', he: 1000, khong: 'tắt' },
    { id: 'ut_giu', ten: 'Xe ưu tiên qua rồi, giữ xanh thêm', min: 0, max: 5, buoc: 0.5, dv: ' s', he: 1000, le: 1 },
    { id: 't_vang', ten: 'Vàng', min: 2, max: 6, buoc: 1, dv: ' s', he: 1000 },
    { id: 't_do', ten: 'Đỏ toàn phần', min: 0, max: 5, buoc: 1, dv: ' s', he: 1000 },
    { id: 'cd_thang', ten: 'Cố định: xanh đi thẳng', min: 10, max: 60, buoc: 5, dv: ' s', he: 1000 },
    { id: 'cd_trai', ten: 'Cố định: xanh rẽ trái', min: 5, max: 40, buoc: 5, dv: ' s', he: 1000 }
  ];
  var MAU_DONG_XE = [
    { ten: 'Đều', luu_luong: [6, 6, 6, 6], trai: [25, 25, 25, 25] },
    { ten: 'Dọc đông, ngang vắng', luu_luong: [10, 2, 9, 1], trai: [25, 10, 20, 10] },
    { ten: 'Chỉ Hướng 1', luu_luong: [8, 0, 0, 0], trai: [40, 0, 0, 0] },
    { ten: 'Giờ cao điểm', luu_luong: [12, 12, 12, 12], trai: [30, 30, 30, 30] },
    { ten: 'Ít rẽ trái', luu_luong: [8, 8, 8, 8], trai: [5, 5, 5, 5] },
    { ten: 'Vắng', luu_luong: [1.5, 1.5, 1.5, 1.5], trai: [25, 25, 25, 25] }
  ];
  var TEN_HUONG = ['', 'Hướng 1 (từ trên)', 'Hướng 2 (từ phải)', 'Hướng 3 (từ dưới)', 'Hướng 4 (từ trái)'];
  var TEN_TRUC = ['dọc', 'ngang'];
  // Xe ưu tiên theo thứ tự hỏa → sự → công → thương; nhãn là tên lớp khi huấn luyện mô hình cho camera.
  var UT_TEN = MoPhong.TEN_UU_TIEN, UT_NUT = ['', 'Cứu hỏa', 'Quân sự', 'Công an', 'Cứu thương'];
  var UT_NHAN = ['', 'xe_cuu_hoa', 'xe_quan_su', 'xe_cong_an', 'xe_cuu_thuong'];
  var UT_MAU = ['', '#d8281f', '#4d5f3a', '#1b5fae', '#c0392b'];
  var KIEU = [
    { v: QD.KIEU_TU_HOC, ten: 'Tự học', dai: 'tự học',
      mo_ta: 'Mỗi trục tự chọn giữa "riêng" và "thẳng trước": cứ vài lượt lại chạy thử kiểu kia một lượt, đo thời gian đèn cho cùng số xe rồi giữ kiểu nhanh hơn.' },
    { v: QD.KIEU_RIENG, ten: 'Riêng', dai: 'riêng từng hướng',
      mo_ta: 'Riêng từng hướng: mỗi hướng được xanh cả thẳng lẫn trái, xong hướng này tới hướng đối diện.' },
    { v: QD.KIEU_THANG_TRUOC, ten: 'Thẳng trước', dai: 'thẳng trước',
      mo_ta: 'Hai hướng đối diện cùng đi thẳng trước; hai làn ngoài hết xe thì mở riêng từng hướng cho làn trong còn xe.' },
    { v: QD.KIEU_DOI_XUNG, ten: 'Đối xứng', dai: 'đối xứng',
      mo_ta: 'Hai hướng đối diện cùng đi thẳng, rồi cùng rẽ trái. Làn trong đi chung thẳng và trái nên dễ kẹt khi có xe rẽ trái.' }
  ];
  function tenKieu(v) { for (var i = 0; i < KIEU.length; i++) if (KIEU[i].v === v) return KIEU[i].dai; return 'riêng từng hướng'; }
  var CO_XE = [60, 75, 100];

  function soVn(x, le) { return x.toFixed(le === undefined ? 0 : le).replace('.', ','); }
  function gioPhut(ms) {
    var s = Math.floor(ms / 1000), p = Math.floor(s / 60);
    return (p < 10 ? '0' : '') + p + ':' + (s % 60 < 10 ? '0' : '') + (s % 60);
  }

  function taoTruot(cha, id, ten, min, max, buoc, layGiaTri, datGiaTri, hienThi) {
    var khoi = document.createElement('div');
    khoi.className = 'truot';
    khoi.innerHTML = '<label for="' + id + '">' + ten + '</label><output for="' + id + '" id="' + id + '_so"></output>' +
      '<input type="range" id="' + id + '" min="' + min + '" max="' + max + '" step="' + buoc + '">';
    cha.appendChild(khoi);
    var o = khoi.querySelector('input'), so = khoi.querySelector('output');
    function capNhat() { o.value = layGiaTri(); so.textContent = hienThi(Number(o.value)); }
    o.addEventListener('input', function () { datGiaTri(Number(o.value)); so.textContent = hienThi(Number(o.value)); luu(); });
    capNhat();
    return capNhat;
  }

  var cap_nhat_truot = [];
  function dungBang() {
    var muc_den = $('muc_den');
    TRUOT_DEN.forEach(function (d) {
      cap_nhat_truot.push(taoTruot(muc_den, 'ts_' + d.id, d.ten, d.min, d.max, d.buoc,
        function () { return ts[d.id] / d.he; },
        function (v) { ts[d.id] = Math.round(v * d.he); capNhatGhiChuDen(); },
        function (v) { return v === 0 && d.khong ? d.khong : soVn(v, d.le || 0) + d.dv; }));
    });
    var gc = document.createElement('p');
    gc.className = 'ghi_chu'; gc.id = 'ghi_chu_den';
    muc_den.appendChild(gc);

    // kiểu chia pha và hai thông số của bộ tự học
    KIEU.forEach(function (k) {
      var id = 'kieu_' + k.v;
      $('chon_kieu').insertAdjacentHTML('beforeend', '<input type="radio" name="kieu_pha" id="' + id + '" value="' + k.v + '"><label for="' + id + '">' + k.ten + '</label>');
      $(id).addEventListener('change', function () {
        ts.kieu_pha = k.v; luu(); capNhatGhiChuKieu();
        ghi('Chọn kiểu chia pha "' + k.dai + '", áp dụng từ lượt sau.');
      });
    });
    cap_nhat_truot.push(taoTruot($('muc_hoc'), 'ts_hoc_cach_thu', 'Tự học: thử kiểu kia sau mỗi', 2, 8, 1,
      function () { return ts.hoc_cach_thu; }, function (v) { ts.hoc_cach_thu = v; }, function (v) { return soVn(v) + ' lượt'; }));
    cap_nhat_truot.push(taoTruot($('muc_hoc'), 'ts_hoc_nguong', 'Tự học: phải nhanh hơn ít nhất', 0, 30, 5,
      function () { return ts.hoc_nguong; }, function (v) { ts.hoc_nguong = v; }, function (v) { return soVn(v) + ' %'; }));

    // camera
    [[0, 'Lí tưởng'], [1, 'ESP32-S3-CAM']].forEach(function (k) {
      var id = 'cam_kieu_' + k[0];
      $('chon_cam_kieu').insertAdjacentHTML('beforeend', '<input type="radio" name="cam_kieu" id="' + id + '"><label for="' + id + '">' + k[1] + '</label>');
      $(id).addEventListener('change', function () { cai.camera.kieu = k[0]; doiCamera('Đổi camera sang kiểu "' + k[1] + '".'); });
    });
    [1, 3, 5].forEach(function (n) {
      var id = 'cam_loc_' + n;
      $('chon_cam_loc').insertAdjacentHTML('beforeend', '<input type="radio" name="cam_loc" id="' + id + '"><label for="' + id + '">' + (n === 1 ? 'không lọc' : n + ' khung') + '</label>');
      $(id).addEventListener('change', function () { cai.camera.so_khung_loc = n; doiCamera(''); });
    });
    [96, 160].forEach(function (n) {
      var id = 'cam_dau_vao_' + n;
      $('chon_cam_dau_vao').insertAdjacentHTML('beforeend', '<input type="radio" name="cam_dau_vao" id="' + id + '"><label for="' + id + '">' + n + ' × ' + n + '</label>');
      $(id).addEventListener('change', function () { cai.camera.dau_vao = n; doiCamera('Đổi ảnh đưa vào mô hình thành ' + n + ' × ' + n + ' (lưới ' + n / cai.camera.o + ' × ' + n / cai.camera.o + ' ô).'); });
    });
    [[1, 'dọc theo đường'], [0, 'ngang đường']].forEach(function (k) {
      var id = 'cam_xoay_' + k[0];
      $('chon_cam_xoay').insertAdjacentHTML('beforeend', '<input type="radio" name="cam_xoay" id="' + id + '"><label for="' + id + '">' + k[1] + '</label>');
      $(id).addEventListener('change', function () { cai.camera.xoay = k[0]; doiCamera('Xoay camera: cạnh dài của khung hình nằm ' + k[1] + '.'); });
    });
    var muc_cam = $('muc_cam_ts'), dau_cam = muc_cam.firstChild;
    [taoTruot(muc_cam, 'cam_khung_giay', 'Xử lí được', 0.5, 5, 0.5,
       function () { return cai.camera.khung_giay; }, function (v) { cai.camera.khung_giay = v; doiCamera(''); },
       function (v) { return soVn(v, 1) + ' khung/giây'; }),
     taoTruot(muc_cam, 'cam_nhan_dung', 'Mô hình nhận ra một xe thấy rõ (giả định)', 80, 100, 1,
       function () { return cai.camera.nhan_dung; }, function (v) { cai.camera.nhan_dung = v; doiCamera(''); },
       function (v) { return soVn(v) + ' %'; })
    ].forEach(function (f) { cap_nhat_truot.push(f); });
    // đưa hai thanh trượt vừa tạo lên trước hai nhóm nút chọn
    muc_cam.insertBefore(muc_cam.lastChild.previousSibling, dau_cam);
    muc_cam.insertBefore(muc_cam.lastChild, dau_cam);

    CO_XE.forEach(function (v) {
      var id = 'co_xe_' + v;
      $('chon_co_xe').insertAdjacentHTML('beforeend', '<input type="radio" name="co_xe" id="' + id + '" value="' + v + '"><label for="' + id + '">' + v + ' %</label>');
      $(id).addEventListener('change', function () {
        cai.co_xe = v; luu();
        nhat_ky = []; mp.datLai(); datLaiGiaoDien();
        capNhatGhiChuCoXe();
        ghi('Đổi cỡ xe thành ' + v + ' %: chạy lại từ đầu.');
      });
    });

    var muc_xe = $('muc_xe');
    MAU_DONG_XE.forEach(function (m, i) {
      var b = document.createElement('button');
      b.type = 'button'; b.className = 'nut nho'; b.id = 'mau_' + i; b.textContent = m.ten;
      b.addEventListener('click', function () {
        for (var h = 1; h <= 4; h++) { cai.luu_luong[h] = m.luu_luong[h - 1]; cai.ti_le_trai[h] = m.trai[h - 1]; }
        cap_nhat_truot.forEach(function (f) { f(); });
        luu();
      });
      $('mau_dong_xe').appendChild(b);
    });
    for (var h = 1; h <= 4; h++) (function (h) {
      var khoi = document.createElement('div');
      khoi.className = 'huong';
      var nut = '';
      for (var k = 1; k <= 4; k++) nut += '<button type="button" class="nut nho ut ut_' + k + '" id="ut_' + h + '_' + k + '">' + UT_NUT[k] + '</button>';
      khoi.innerHTML = '<div class="huong_dau"><strong>' + TEN_HUONG[h] + '</strong></div>' +
        '<div class="hang_ut"><span>Thả xe ưu tiên:</span>' + nut + '</div>' +
        '<div class="huong_dem" id="huong_dem_' + h + '"></div>';
      muc_xe.appendChild(khoi);
      cap_nhat_truot.push(taoTruot(khoi, 'luu_luong_' + h, 'Lưu lượng', 0, 20, 0.5,
        function () { return cai.luu_luong[h]; }, function (v) { cai.luu_luong[h] = v; },
        function (v) { return soVn(v, 1) + ' xe/phút'; }));
      cap_nhat_truot.push(taoTruot(khoi, 'ti_le_trai_' + h, 'Rẽ trái', 0, 60, 5,
        function () { return cai.ti_le_trai[h]; }, function (v) { cai.ti_le_trai[h] = v; },
        function (v) { return soVn(v) + ' %'; }));
      [1, 2, 3, 4].forEach(function (loai) {
        $('ut_' + h + '_' + loai).addEventListener('click', function () { mp.thaXeUuTien(h, loai); ghi('Thả xe ' + UT_TEN[loai] + ' vào Hướng ' + h + '.'); });
      });
    })(h);
    cap_nhat_truot.push(taoTruot(muc_xe, 'ti_le_phai', 'Rẽ phải (làn riêng, không qua đèn)', 0, 40, 5,
      function () { return cai.ti_le_phai; }, function (v) { cai.ti_le_phai = v; }, function (v) { return soVn(v) + ' %'; }));
    cap_nhat_truot.push(taoTruot(muc_xe, 'tre_phan_ung', 'Trễ phản ứng của người lái', 0, 2, 0.1,
      function () { return cai.tre_phan_ung / 1000; }, function (v) { cai.tre_phan_ung = Math.round(v * 1000); },
      function (v) { return v === 0 ? 'không trễ' : soVn(v, 1) + ' s'; }));
    cap_nhat_truot.push(taoTruot(muc_xe, 'toc_do_xe', 'Tốc độ xe', 60, 180, 10,
      function () { return cai.toc_do; }, function (v) { cai.toc_do = v; }, function (v) { return soVn(v) + ' mm/s'; }));

    var chon = $('chon_toc_do');
    [1, 2, 4, 8, 16].forEach(function (v) {
      var id = 'toc_do_' + v;
      chon.insertAdjacentHTML('beforeend', '<input type="radio" name="toc_do" id="' + id + '" value="' + v + '"><label for="' + id + '">×' + v + '</label>');
      $(id).addEventListener('change', function () { tt.toc_do = v; luu(); });
    });
    [0, 1].forEach(function (v) {
      $('che_do_' + v).addEventListener('change', function () {
        ts.che_do = v; luu();
        ghi('Chọn chế độ ' + (v ? 'thích ứng' : 'cố định') + ', áp dụng từ chu kì sau.');
      });
    });
    $('nut_chay').addEventListener('click', function () {
      tt.chay = !tt.chay;
      $('nut_chay').textContent = tt.chay ? 'Tạm dừng' : 'Chạy tiếp';
    });
    $('nut_lai').addEventListener('click', function () { nhat_ky = []; mp.datLai(); datLaiGiaoDien(); ghi('Chạy lại từ đầu với cùng dòng xe.'); });
    $('hien_vung').addEventListener('change', function () { tt.hien_vung = $('hien_vung').checked; luu(); });
    $('nut_so_sanh').addEventListener('click', chaySoSanh);
  }

  function capNhatGhiChuDen() {
    var moi_xe = ts.cong_them * ts.xe_quy_doi / 10 / 1000;
    $('ghi_chu_den').textContent = 'Chế độ thích ứng: xanh = ' + soVn(ts.t_min / 1000) + ' s + ' + soVn(moi_xe, 1) +
      ' s cho mỗi xe mô hình ở làn đông nhất, không quá ' + soVn(Math.max(ts.t_max, ts.t_min) / 1000) + ' s.' +
      (ts.cat_som > 0 ? ' Tổ hợp đang xanh hết xe quá ' + soVn(ts.cat_som / 1000) + ' s mà hướng khác có xe chờ thì chuyển vàng ngay; bước kế không còn xe thì bỏ qua.' : '');
  }
  function datLaiGiaoDien() { tt.so_cat_som = 0; tt.ut_loai = 0; }
  function capNhatGhiChuKieu() {
    for (var i = 0; i < KIEU.length; i++) if (KIEU[i].v === ts.kieu_pha) $('ghi_chu_kieu').textContent = KIEU[i].mo_ta;
  }
  function capNhatGhiChuCoXe() {
    $('ghi_chu_co_xe').textContent = 'Xe dài ' + soVn(mp.xe_dai) + ' mm, mỗi làn chờ chứa ' + mp.suc_chua + ' xe. ' +
      (cai.co_xe === 100 ? 'Đây là cỡ xe Hot Wheels trên sa bàn thật.' : 'Cỡ 100 % là xe Hot Wheels trên sa bàn thật (4 xe mỗi làn); 75 % là đúng tỉ lệ ô tô thật so với bề rộng làn.');
  }
  // Đổi thông số camera: dựng lại 4 camera, không chạy lại mô phỏng.
  function doiCamera(loi_ghi) {
    mp.datCamera();
    luu();
    capNhatGhiChuCamera();
    if (loi_ghi) ghi(loi_ghi);
  }
  function capNhatGhiChuCamera() {
    var c = cai.camera;
    $('muc_cam_ts').style.display = c.kieu ? '' : 'none';
    $('ghi_chu_cam').textContent = c.kieu ?
      'Mỗi camera chụp khung 320 × 240 từ trên khung đèn, cắt vùng 2 làn chờ, co về ' + c.dau_vao + ' × ' + c.dau_vao + ' rồi nhận diện theo lưới ' +
        c.dau_vao / c.o + ' × ' + c.dau_vao / c.o + ' ô. Kết quả tới trung tâm trễ chừng ' +
        soVn((1000 / c.khung_giay) * (1.5 + (c.so_khung_loc - 1) / 2) / 1000, 1) + ' s.' :
      'Đếm đúng tuyệt đối số xe trong vùng chờ, 4 lần mỗi giây. Dùng để so sánh; board thật không làm được như vậy.';
  }
  function dongBoNut() {
    $('cam_kieu_' + (cai.camera.kieu ? 1 : 0)).checked = true;
    ($('cam_loc_' + cai.camera.so_khung_loc) || $('cam_loc_3')).checked = true;
    $('cam_xoay_' + (cai.camera.xoay ? 1 : 0)).checked = true;
    ($('cam_dau_vao_' + cai.camera.dau_vao) || $('cam_dau_vao_96')).checked = true;
    capNhatGhiChuCamera();
    var o_kieu = $('kieu_' + ts.kieu_pha) || $('kieu_' + QD.KIEU_RIENG);
    o_kieu.checked = true;
    capNhatGhiChuKieu();
    ($('co_xe_' + cai.co_xe) || $('co_xe_75')).checked = true;
    capNhatGhiChuCoXe();
    $('che_do_' + (ts.che_do === QD.CHE_DO_CO_DINH ? 0 : 1)).checked = true;
    var o = $('toc_do_' + tt.toc_do);
    if (o) o.checked = true;
    $('hien_vung').checked = tt.hien_vung;
    cap_nhat_truot.forEach(function (f) { f(); });
    capNhatGhiChuDen();
  }

  // ---------- Nhớ thông số trên máy người xem ----------
  function luu() {
    try {
      localStorage.setItem(KHOA_LUU, JSON.stringify({ ts: ts, cai: cai, toc_do: tt.toc_do, hien_vung: tt.hien_vung }));
    } catch (e) { /* không lưu được thì thôi */ }
  }
  function docLai() {
    try {
      var d = JSON.parse(localStorage.getItem(KHOA_LUU) || 'null');
      if (!d) return;
      var k;
      for (k in ts) if (typeof d.ts[k] === 'number' && isFinite(d.ts[k])) ts[k] = d.ts[k];
      for (var h = 1; h <= 4; h++) {
        if (isFinite(d.cai.luu_luong[h])) cai.luu_luong[h] = Number(d.cai.luu_luong[h]);
        if (isFinite(d.cai.ti_le_trai[h])) cai.ti_le_trai[h] = Number(d.cai.ti_le_trai[h]);
      }
      if (isFinite(d.cai.ti_le_phai)) cai.ti_le_phai = Number(d.cai.ti_le_phai);
      if (isFinite(d.cai.toc_do)) cai.toc_do = Number(d.cai.toc_do);
      if (isFinite(d.cai.tre_phan_ung)) cai.tre_phan_ung = Number(d.cai.tre_phan_ung);
      if (CO_XE.indexOf(d.cai.co_xe) >= 0) cai.co_xe = d.cai.co_xe;
      var dc = d.cai.camera || {};
      if (dc.kieu === 0 || dc.kieu === 1) cai.camera.kieu = dc.kieu;
      if (dc.xoay === 0 || dc.xoay === 1) cai.camera.xoay = dc.xoay;
      if ([1, 3, 5].indexOf(dc.so_khung_loc) >= 0) cai.camera.so_khung_loc = dc.so_khung_loc;
      if (dc.dau_vao === 96 || dc.dau_vao === 160) cai.camera.dau_vao = dc.dau_vao;
      if (dc.khung_giay >= 0.5 && dc.khung_giay <= 5) cai.camera.khung_giay = Number(dc.khung_giay);
      if (dc.nhan_dung >= 80 && dc.nhan_dung <= 100) cai.camera.nhan_dung = Number(dc.nhan_dung);
      if ([1, 2, 4, 8, 16].indexOf(d.toc_do) >= 0) tt.toc_do = d.toc_do;
      tt.hien_vung = !!d.hien_vung;
    } catch (e) { /* dữ liệu cũ hỏng: dùng mặc định */ }
  }

  // ---------- Nhật ký ----------
  function ghi(noi_dung) {
    nhat_ky.unshift({ t: mp.t, noi_dung: noi_dung });
    if (nhat_ky.length > 8) nhat_ky.length = 8;
  }
  // Nhật ký chỉ ghi việc đáng chú ý (xe ưu tiên, tự học, đổi chế độ). Kế hoạch từng lượt đã có ở cột bên
  // trái; số lần cắt xanh sớm được đếm ở ô "Xe qua đèn".
  mp.khiSuKien = function (loai, dl) {
    var hc = mp.dk.hoc, truc, kieu;
    if (loai === 'cat_som') tt.so_cat_som++;
    else if (loai === 'uu_tien') {
      tt.ut_loai = mp.dk.dem[dl].ut;
      ghi('Camera Hướng ' + dl + ' thấy xe ' + UT_TEN[tt.ut_loai] + ': kết thúc bước đang chạy, mở riêng Hướng ' + dl + '.');
    }
    else if (loai === 'uu_tien_qua') ghi('Xe ' + UT_TEN[dl.loai] + ' Hướng ' + dl.h + ' qua vạch dừng sau ' + soVn(dl.ms / 1000, 1) + ' s.');
    else if (loai === 'het_uu_tien') ghi('Hết ưu tiên Hướng ' + dl + ': chuyển vàng.');
    else if (loai === 'hoc_nhan') {
      // chỉ ghi lúc đổi kiểu; các lượt thử định kì xem ở ô "Kiểu chia pha"
      truc = dl >> 2; kieu = dl & 3;
      ghi('Tự học: trục ' + TEN_TRUC[truc] + ' đổi sang "' + tenKieu(kieu) + '" (thẳng trước đang cần ' + soVn(hc.q[truc]) +
          ' % thời gian đèn của kiểu riêng).');
    }
    else if (loai === 'loi') ghi('LỖI AN TOÀN: tổ hợp đèn xung đột, chuyển sang nháy vàng.');
  };

  // ---------- Nền sa bàn: vẽ tệp decal một lần ra bộ đệm ----------
  var nen = document.createElement('canvas'), nen_xong = false;
  nen.width = nen.height = 2000;
  (function () {
    var anh = new Image();
    anh.onload = function () {
      var c = nen.getContext('2d');
      c.drawImage(anh, 0, 0, 2000, 2000);
      nen_xong = true;
    };
    anh.src = 'data:image/svg+xml;charset=utf-8,' + encodeURIComponent(SA_BAN_SVG);
  })();

  // ---------- Vẽ ----------
  function hinhBoTron(c, x, y, w, h, r) {
    c.beginPath();
    c.moveTo(x + r, y); c.lineTo(x + w - r, y); c.arcTo(x + w, y, x + w, y + r, r);
    c.lineTo(x + w, y + h - r); c.arcTo(x + w, y + h, x + w - r, y + h, r);
    c.lineTo(x + r, y + h); c.arcTo(x, y + h, x, y + h - r, r);
    c.lineTo(x, y + r); c.arcTo(x, y, x + r, y, r);
    c.closePath();
  }

  // Xe vẽ theo cỡ gốc 68 × 34 mm rồi thu nhỏ theo cỡ xe đang chọn.
  function veXe(c, xe) {
    var dai = H.XE_DAI, rong = H.XE_RONG, con = xe.dd.dai - xe.s, ti_le = mp.xe_dai / H.XE_DAI;
    var mo = Math.min(1, xe.s / 55, con / 70);
    if (mo <= 0) return;
    c.save();
    c.globalAlpha = mo;
    c.translate(xe.x, xe.y);
    c.rotate(xe.goc);
    c.scale(ti_le, ti_le);
    c.fillStyle = 'rgba(0,0,0,.28)';                      // bóng
    hinhBoTron(c, -dai / 2 + 2, -rong / 2 + 3, dai, rong, 9); c.fill();
    c.fillStyle = xe.mau;                                  // thân
    hinhBoTron(c, -dai / 2, -rong / 2, dai, rong, 9); c.fill();
    c.fillStyle = 'rgba(15,22,30,.82)';                    // kính trước, kính sau
    hinhBoTron(c, 4, -rong / 2 + 4, 15, rong - 8, 4); c.fill();
    hinhBoTron(c, -dai / 2 + 9, -rong / 2 + 5, 9, rong - 10, 3); c.fill();
    c.fillStyle = 'rgba(255,255,255,.22)';                 // nóc
    hinhBoTron(c, -dai / 2 + 20, -rong / 2 + 5, 13, rong - 10, 3); c.fill();
    c.fillStyle = '#fff6c9';                               // đèn pha
    c.fillRect(dai / 2 - 4, -rong / 2 + 3, 3, 6); c.fillRect(dai / 2 - 4, rong / 2 - 9, 3, 6);
    c.fillStyle = xe.phanh ? '#ff2a1a' : '#7a1510';        // đèn hậu sáng lên khi phanh
    c.fillRect(-dai / 2 + 1, -rong / 2 + 3, 3, 7); c.fillRect(-dai / 2 + 1, rong / 2 - 10, 3, 7);
    var nhay = Math.floor(mp.t / 350) % 2 === 0;
    if (!xe.da_qua && nhay && xe.huong_di !== 'thang') {   // xi nhan
      var ben = xe.huong_di === 'trai' ? -1 : 1;
      c.fillStyle = '#ffb000';
      c.fillRect(dai / 2 - 8, ben * (rong / 2 - 4) - 2.5, 6, 5);
      c.fillRect(-dai / 2 + 2, ben * (rong / 2 - 4) - 2.5, 6, 5);
    }
    if (xe.uu_tien === 1) {                                // cứu hỏa: thân đỏ, thang nhôm trên nóc
      c.fillStyle = '#cfd4d9'; c.fillRect(-29, -5, 30, 10);
      c.fillStyle = '#7b848c';
      for (var i = 0; i < 5; i++) c.fillRect(-26 + i * 6, -5, 1.6, 10);
    } else if (xe.uu_tien === 2) {                         // quân sự: thân xanh rêu, thùng phủ bạt
      c.fillStyle = '#39472b'; hinhBoTron(c, -31, -14, 30, 28, 4); c.fill();
      c.fillStyle = '#5d7345'; c.fillRect(-22, -14, 1.6, 28); c.fillRect(-12, -14, 1.6, 28);
      c.fillStyle = '#f2c230'; c.beginPath(); c.arc(26, 0, 3.2, 0, 6.3); c.fill();
    } else if (xe.uu_tien === 3) {                         // công an: thân trắng, hai dải xanh dọc sườn
      c.fillStyle = '#1b5fae'; c.fillRect(-28, -rong / 2 + 1, 52, 5); c.fillRect(-28, rong / 2 - 6, 52, 5);
    } else if (xe.uu_tien === 4) {                         // cứu thương: thân trắng, chữ thập đỏ
      c.fillStyle = '#d5372c';
      c.fillRect(-19, -3, 18, 6); c.fillRect(-13, -9, 6, 18);
    }
    if (xe.uu_tien) {                                      // đèn chớp trên nóc
      c.fillStyle = nhay ? '#2f7bff' : '#ff3b30'; c.fillRect(0, -9, 5, 8);
      c.fillStyle = nhay ? '#ff3b30' : '#2f7bff'; c.fillRect(0, 1, 5, 8);
    }
    c.restore();
  }
  // Nhãn chữ cạnh xe ưu tiên, để nhận ra loại xe khi xe vẽ nhỏ.
  function veNhanUuTien(c) {
    c.font = '700 15px sans-serif'; c.textAlign = 'center'; c.textBaseline = 'middle';
    for (var i = 0; i < mp.xe.length; i++) {
      var xe = mp.xe[i];
      if (!xe.uu_tien || xe.s < 55 || xe.dd.dai - xe.s < 70) continue;
      var chu = UT_NUT[xe.uu_tien].toUpperCase(), w = c.measureText(chu).width + 12;
      var y = xe.y - mp.xe_dai / 2 - 13;
      c.fillStyle = UT_MAU[xe.uu_tien];
      hinhBoTron(c, xe.x - w / 2, y - 10, w, 20, 5); c.fill();
      c.fillStyle = '#ffffff'; c.fillText(chu, xe.x, y + 1);
    }
    c.textAlign = 'start'; c.textBaseline = 'alphabetic';
  }

  var MAU_DEN = [['#ff3b30', '#4a1512'], ['#ffc400', '#4a3a08'], ['#27e07a', '#0e3d22']];   // [sáng, tắt] cho đỏ, vàng, xanh
  function veCumDen(c, x, y, mau) {
    c.fillStyle = '#15171a';
    hinhBoTron(c, x - 22, y - 8.5, 44, 17, 6); c.fill();
    c.strokeStyle = '#6d747c'; c.lineWidth = 1.2; c.stroke();
    for (var i = 0; i < 3; i++) {
      var sang = (i === 0 && mau === 0) || (i === 1 && mau === 1) || (i === 2 && mau === 2);
      var lx = x - 14 + i * 14;
      if (sang) {
        var g = c.createRadialGradient(lx, y, 1, lx, y, 17);
        g.addColorStop(0, MAU_DEN[i][0]); g.addColorStop(.35, MAU_DEN[i][0] + 'aa'); g.addColorStop(1, MAU_DEN[i][0] + '00');
        c.fillStyle = g;
        c.beginPath(); c.arc(lx, y, 17, 0, 6.3); c.fill();
      }
      c.fillStyle = sang ? MAU_DEN[i][0] : MAU_DEN[i][1];
      c.beginPath(); c.arc(lx, y, 5.6, 0, 6.3); c.fill();
    }
  }
  function veDen(c) {
    for (var h = 1; h <= 4; h++) {
      c.save();
      c.translate(500, 500); c.rotate((h - 1) * Math.PI / 2); c.translate(-500, -500);
      c.strokeStyle = '#8a9099'; c.lineWidth = 3;          // cần vươn của khung đèn, từ cột trên đảo dẫn hướng
      c.beginPath(); c.moveTo(384, 375); c.lineTo(384, 383); c.lineTo(493, 383); c.stroke();
      veCumDen(c, H.X_NGOAI, 383, mp.dk.mauNhom((h - 1) * 2, mp.t));       // cụm mũi tên thẳng trên làn ngoài
      veCumDen(c, H.X_TRONG, 383, mp.dk.mauNhom((h - 1) * 2 + 1, mp.t));   // cụm mũi tên rẽ trái trên làn trong
      c.restore();
    }
  }
  function veVung(c, dam) {
    var v = H.VUNG;
    c.fillStyle = 'rgba(64,196,255,' + dam + ')';
    c.fillRect(v.ngoai.x0, v.ngoai.y0, v.ngoai.x1 - v.ngoai.x0, v.ngoai.y1 - v.ngoai.y0);
    c.fillStyle = 'rgba(255,120,220,' + dam + ')';
    c.fillRect(v.trong.x0, v.trong.y0, v.trong.x1 - v.trong.x0, v.trong.y1 - v.trong.y0);
  }

  var o_sa_ban = $('sa_ban'), c_sa_ban = o_sa_ban.getContext('2d');
  function veSaBan() {
    var c = c_sa_ban, k = o_sa_ban.width / 1000;
    c.setTransform(k, 0, 0, k, 0, 0);
    if (nen_xong) c.drawImage(nen, 0, 0, 1000, 1000);
    else { c.fillStyle = '#3b3e44'; c.fillRect(0, 0, 1000, 1000); }
    if (tt.hien_vung) {
      for (var h = 1; h <= 4; h++) {
        c.save();
        c.translate(500, 500); c.rotate((h - 1) * Math.PI / 2); c.translate(-500, -500);
        veVung(c, 0.3);
        c.restore();
      }
    }
    for (var i = 0; i < mp.xe.length; i++) veXe(c, mp.xe[i]);
    veDen(c);
    veNhanUuTien(c);
  }

  // Ô camera: nhìn từ trên vạch dừng ngược về phía xe đang tới; khung 4:3 rộng hơn 2 làn nhiều
  // nên firmware thật cắt ROI quanh 2 làn rồi mới đưa vào mô hình.
  var CAM_X0 = 220, CAM_Y0 = 40, CAM_RONG = 453.33;
  var o_cam = [], c_cam = [];
  // Ô camera gồm khung hình 320 × 240 ở trên và một dải thông tin ở dưới.
  var KH_RONG = 320, KH_CAO = 240, DAI_CAO = 104, TI_LE_CAM = (KH_CAO + DAI_CAO) / KH_RONG;
  var MAU_LOAI = ['#ffe45c', '#ff5a4f', '#9be36b', '#6fb1ff', '#ff9ad5'];   // màu đánh dấu: xe thường, rồi 4 loại xe ưu tiên
  var TEN_LOP = ['xe'].concat(UT_NHAN.slice(1));
  var o_khung = [], o_dau_vao = [], khung_da_ve = [], nen_cam = [], nen_goc = null, nen_goc_hong = false;

  function veCamera(h) {
    if (cai.camera.kieu) veCameraThat(h); else veCameraLiTuong(h);
  }

  // dòng trên cùng của ô camera: số đếm đang gửi về trung tâm
  function veDauCamera(c, o, h, to) {
    var d = mp.dem[h];
    c.font = '600 ' + Math.round(11 * to) + 'px ' + 'ui-monospace, Consolas, monospace';
    c.textBaseline = 'top';
    c.fillStyle = 'rgba(0,0,0,.6)'; c.fillRect(0, 0, o.width, 17 * to);
    c.fillStyle = '#fff'; c.fillText('CAM ' + h, 5 * to, 3 * to);
    c.fillStyle = '#40c4ff'; c.fillText('s=' + d.s, 52 * to, 3 * to);
    c.fillStyle = '#ff78dc'; c.fillText('l=' + d.l, 84 * to, 3 * to);
    c.fillStyle = d.emg ? '#ff5a4f' : '#b9c0c7'; c.fillText('emg=' + d.emg, 116 * to, 3 * to);
    c.fillStyle = d.ut ? '#ff5a4f' : '#b9c0c7'; c.fillText('ut=' + d.ut, 166 * to, 3 * to);
  }

  // Camera lí tưởng: nhìn thẳng từ trên xuống, đếm xe có tâm trong vùng chờ.
  function veCameraLiTuong(h) {
    var o = o_cam[h], c = c_cam[h], k = o.width / CAM_RONG, cao_khung = o.width * KH_CAO / KH_RONG, i;
    c.setTransform(1, 0, 0, 1, 0, 0);
    c.fillStyle = '#1d2024'; c.fillRect(0, 0, o.width, o.height);
    c.save();
    c.beginPath(); c.rect(0, 0, o.width, cao_khung); c.clip();
    c.setTransform(k, 0, 0, k, 0, 0);
    c.translate(-CAM_X0, -CAM_Y0);
    c.save();
    c.translate(500, 500); c.rotate(-(h - 1) * Math.PI / 2); c.translate(-500, -500);
    if (nen_xong) c.drawImage(nen, 0, 0, 1000, 1000);
    for (i = 0; i < mp.xe.length; i++) veXe(c, mp.xe[i]);
    c.restore();
    veVung(c, 0.2);
    c.setLineDash([7, 5]); c.strokeStyle = '#ffffff'; c.lineWidth = 2;     // khung ROI
    c.strokeRect(H.VUNG.ngoai.x0 - 6, H.VUNG.ngoai.y0 - 6, 2 * H.LAN + 12, H.VUNG.ngoai.y1 - H.VUNG.ngoai.y0 + 12);
    c.setLineDash([]);
    for (i = 0; i < mp.xe.length; i++) {                                    // khung quanh xe đếm được
      var xe = mp.xe[i], lan = MoPhong.lanCuaDiem(xe.x, xe.y, h);
      if (!lan) continue;
      var p = MoPhong.quayNguoc(xe.x, xe.y, h), g = xe.goc - (h - 1) * Math.PI / 2;
      var w = Math.abs(Math.cos(g)) * mp.xe_dai + Math.abs(Math.sin(g)) * mp.xe_rong + 8;
      var cao = Math.abs(Math.sin(g)) * mp.xe_dai + Math.abs(Math.cos(g)) * mp.xe_rong + 8;
      c.strokeStyle = xe.uu_tien ? '#ff3b30' : (lan === 'ngoai' ? '#40c4ff' : '#ff78dc');
      c.lineWidth = xe.uu_tien ? 4 : 2.5;
      c.strokeRect(p[0] - w / 2, p[1] - cao / 2, w, cao);
      if (xe.uu_tien) {                                                     // tên lớp mô hình nhận ra
        c.font = '600 15px ui-monospace, Consolas, monospace'; c.textBaseline = 'bottom';
        var ten_lop = UT_NHAN[xe.uu_tien], rong_chu = c.measureText(ten_lop).width + 8;
        c.fillStyle = '#ff3b30'; c.fillRect(p[0] - w / 2 - 2, p[1] - cao / 2 - 19, rong_chu, 18);
        c.fillStyle = '#ffffff'; c.fillText(ten_lop, p[0] - w / 2 + 2, p[1] - cao / 2 - 2);
      }
    }
    c.restore();
    c.setTransform(1, 0, 0, 1, 0, 0);
    var to = o.width / 230;                                                 // chữ theo cỡ ô
    veDauCamera(c, o, h, to);
    c.font = Math.round(8.6 * to) + 'px ui-monospace, Consolas, monospace';
    c.fillStyle = '#b9c0c7';
    c.fillText('Camera lí tưởng: nhìn thẳng từ trên xuống,', 6 * to, cao_khung + 8 * to);
    c.fillText('đếm đúng mọi xe có tâm trong vùng chờ,', 6 * to, cao_khung + 20 * to);
    c.fillText('gửi 4 lần mỗi giây, không trễ.', 6 * to, cao_khung + 32 * to);
    c.fillStyle = '#e9edf0'; c.font = Math.round(7.4 * to) + 'px ui-monospace, Consolas, monospace';
    c.fillText(mp.jsonCamera(h), 6 * to, o.height - 12 * to);
  }

  // ---------- Camera kiểu ESP32-S3-CAM ----------
  // Nền sa bàn nhìn qua ống kính: với từng điểm ảnh, tìm điểm trên mặt sa bàn mà tia nhìn đi tới rồi lấy màu ở đó.
  function nenQuaOngKinh(h) {
    var cam = mp.cam[h].mo_hinh, ts = cai.camera;
    var khoa = [ts.xoay, ts.nghieng, ts.cao_mm, ts.lui_mm, ts.goc_cheo, nen_xong && !nen_goc_hong ? 1 : 0].join();
    if (nen_cam[h] && nen_cam[h].khoa === khoa) return nen_cam[h].o;
    var o = document.createElement('canvas'), u, v, p;
    o.width = KH_RONG; o.height = KH_CAO;
    var c = o.getContext('2d');
    if (nen_xong && !nen_goc && !nen_goc_hong) {
      try {
        var nho = document.createElement('canvas');
        nho.width = nho.height = 1000;
        nho.getContext('2d').drawImage(nen, 0, 0, 1000, 1000);
        nen_goc = nho.getContext('2d').getImageData(0, 0, 1000, 1000).data;
      } catch (e) { nen_goc_hong = true; }      // trình duyệt không cho đọc điểm ảnh: vẽ nền đơn giản
    }
    if (nen_goc) {
      var anh = c.createImageData(KH_RONG, KH_CAO), d = anh.data, i = 0;
      for (v = 0; v < KH_CAO; v++) {
        for (u = 0; u < KH_RONG; u++, i += 4) {
          p = cam.nguoc(u + 0.5, v + 0.5);
          var x = p ? Math.floor(p[0]) : -1, y = p ? Math.floor(p[1]) : -1;
          if (x >= 0 && x < 1000 && y >= 0 && y < 1000) {
            var j = (y * 1000 + x) * 4;
            d[i] = nen_goc[j]; d[i + 1] = nen_goc[j + 1]; d[i + 2] = nen_goc[j + 2];
          } else { d[i] = 70; d[i + 1] = 66; d[i + 2] = 60; }       // ngoài mép tấm sa bàn: mặt bàn
          d[i + 3] = 255;
        }
      }
      c.putImageData(anh, 0, 0);
    } else {
      c.fillStyle = '#3b3e44'; c.fillRect(0, 0, KH_RONG, KH_CAO);
    }
    nen_cam[h] = { khoa: khoa, o: o };
    khung_da_ve[h] = -1;
    return o;
  }

  function pha(mau, k) {       // làm màu tối đi (k < 1) hoặc sáng lên (k > 1)
    var m = /^#?([0-9a-f]{2})([0-9a-f]{2})([0-9a-f]{2})$/i.exec(mau);
    if (!m) return mau;
    var f = function (s) { return Math.max(0, Math.min(255, Math.round(parseInt(s, 16) * k))); };
    return 'rgb(' + f(m[1]) + ',' + f(m[2]) + ',' + f(m[3]) + ')';
  }
  function toDaGiac(c, ds, mau) {
    c.fillStyle = mau; c.beginPath(); c.moveTo(ds[0].u, ds[0].v);
    for (var i = 1; i < ds.length; i++) c.lineTo(ds[i].u, ds[i].v);
    c.closePath(); c.fill();
  }
  // Vẽ một xe (hộp có nóc, kính, dấu hiệu xe ưu tiên) lên khung hình theo phép chiếu của camera.
  function veXeQuaOngKinh(c, cam, xe) {
    var g = MayAnh.chieuXe(cam, xe);
    if (!g) return;
    var cs = Math.cos(xe.goc), sn = Math.sin(xe.goc), dx = cam.goc_x - xe.x, dy = cam.goc_y - xe.y;
    var doc = dx * cs + dy * sn, ngang = -dx * sn + dy * cs;       // camera nằm phía nào của xe
    var mat = [];
    if (doc > xe.dai / 2) mat.push([0, 1, 5, 4]); else if (doc < -xe.dai / 2) mat.push([2, 3, 7, 6]);
    if (ngang > xe.rong / 2) mat.push([1, 2, 6, 5]); else if (ngang < -xe.rong / 2) mat.push([3, 0, 4, 7]);
    // bóng đổ nhẹ dưới gầm
    toDaGiac(c, [g[0], g[1], g[2], g[3]], 'rgba(0,0,0,.35)');
    for (var i = 0; i < mat.length; i++) toDaGiac(c, mat[i].map(function (k) { return g[k]; }), pha(xe.mau, i === 0 ? 0.62 : 0.74));
    toDaGiac(c, [g[4], g[5], g[6], g[7]], xe.mau);
    // điểm trên nóc theo toạ độ thân xe (a dọc thân từ -1 đuôi tới 1 mũi, b ngang thân từ -1 tới 1)
    function noc(a, b) {
      var lx = a * xe.dai / 2, ly = b * xe.rong / 2;
      return cam.chieu(xe.x + lx * cs - ly * sn, xe.y + lx * sn + ly * cs, xe.cao);
    }
    function mang(a0, a1, b0, b1, mau) { toDaGiac(c, [noc(a0, b0), noc(a1, b0), noc(a1, b1), noc(a0, b1)], mau); }
    mang(0.15, 0.55, -0.72, 0.72, 'rgba(15,22,30,.82)');          // kính trước
    mang(-0.72, -0.48, -0.66, 0.66, 'rgba(15,22,30,.82)');        // kính sau
    if (xe.loai === 1) mang(-0.8, 0.05, -0.3, 0.3, '#cfd4d9');                    // cứu hỏa: thang nhôm
    else if (xe.loai === 2) mang(-0.9, 0.05, -0.85, 0.85, '#39472b');             // quân sự: thùng phủ bạt
    else if (xe.loai === 3) { mang(-0.85, 0.75, -1, -0.72, '#1b5fae'); mang(-0.85, 0.75, 0.72, 1, '#1b5fae'); }   // công an: hai dải xanh
    else if (xe.loai === 4) { mang(-0.45, 0.0, -0.18, 0.18, '#d5372c'); mang(-0.3, -0.15, -0.55, 0.55, '#d5372c'); }   // cứu thương: chữ thập đỏ
    if (xe.loai) { mang(0.02, 0.14, -0.5, 0, '#2f7bff'); mang(0.02, 0.14, 0, 0.5, '#ff3b30'); }                  // đèn chớp
  }

  // Vẽ lại khung hình cảm biến và ảnh đầu vào mô hình của camera h theo khung vừa xử lí xong.
  function veKhungCamBien(h) {
    var cm = mp.cam[h], cam = cm.mo_hinh, nen_h = nenQuaOngKinh(h), i;
    if (!o_khung[h]) {
      o_khung[h] = document.createElement('canvas'); o_khung[h].width = KH_RONG; o_khung[h].height = KH_CAO;
      o_dau_vao[h] = document.createElement('canvas');
    }
    var c = o_khung[h].getContext('2d');
    c.drawImage(nen_h, 0, 0);
    var ds = cm.khung && cm.khung.xe ? cm.khung.xe.slice() : [];
    ds.sort(function (a, b) {          // xe xa camera vẽ trước
      return Math.hypot(b.x - cam.goc_x, b.y - cam.goc_y) - Math.hypot(a.x - cam.goc_x, a.y - cam.goc_y);
    });
    for (i = 0; i < ds.length; i++) veXeQuaOngKinh(c, cam, ds[i]);
    // ảnh đầu vào mô hình: cắt ROI rồi co về N × N
    var N = cai.camera.dau_vao, dv = o_dau_vao[h];
    if (dv.width !== N) { dv.width = N; dv.height = N; }
    var cd = dv.getContext('2d');
    cd.imageSmoothingEnabled = true;
    cd.drawImage(o_khung[h], cam.roi.x, cam.roi.y, cam.roi.w, cam.roi.h, 0, 0, N, N);
    khung_da_ve[h] = cm.so_khung;
  }

  function veCameraThat(h) {
    var o = o_cam[h], c = c_cam[h], cm = mp.cam[h], cam = cm.mo_hinh, ts = cai.camera, i;
    var nen_h = nenQuaOngKinh(h);
    if (khung_da_ve[h] !== cm.so_khung || !o_khung[h]) veKhungCamBien(h);
    var k = o.width / KH_RONG, to = o.width / 230, kh = cm.khung, roi = cam.roi;
    c.setTransform(1, 0, 0, 1, 0, 0);
    c.fillStyle = '#1d2024'; c.fillRect(0, 0, o.width, o.height);
    c.setTransform(k, 0, 0, k, 0, 0);
    c.imageSmoothingEnabled = true;
    c.drawImage(o_khung[h] || nen_h, 0, 0);
    // hai tứ giác làn
    function tuGiac(q, mau_to, mau_vien) {
      c.beginPath(); c.moveTo(q[0].x, q[0].y);
      for (var j = 1; j < 4; j++) c.lineTo(q[j].x, q[j].y);
      c.closePath(); c.fillStyle = mau_to; c.fill(); c.strokeStyle = mau_vien; c.lineWidth = 1.2; c.stroke();
    }
    c.save();
    c.beginPath(); c.rect(0, 0, KH_RONG, KH_CAO); c.clip();
    tuGiac(cam.lan_ngoai, 'rgba(64,196,255,.13)', 'rgba(64,196,255,.9)');
    tuGiac(cam.lan_trong, 'rgba(255,120,220,.13)', 'rgba(255,120,220,.9)');
    // ROI và lưới ô của mô hình
    var luoi = Math.floor(ts.dau_vao / ts.o), ow = roi.w / luoi, oh = roi.h / luoi;
    c.strokeStyle = 'rgba(255,255,255,.16)'; c.lineWidth = 0.6;
    c.beginPath();
    for (i = 1; i < luoi; i++) {
      c.moveTo(roi.x + i * ow, roi.y); c.lineTo(roi.x + i * ow, roi.y + roi.h);
      c.moveTo(roi.x, roi.y + i * oh); c.lineTo(roi.x + roi.w, roi.y + i * oh);
    }
    c.stroke();
    c.setLineDash([6, 4]); c.strokeStyle = '#ffffff'; c.lineWidth = 1.4;
    c.strokeRect(roi.x + 0.7, roi.y + 0.7, roi.w - 1.4, roi.h - 1.4);
    c.setLineDash([]);
    if (kh) {
      for (i = 0; i < kh.o.length; i++) {                          // ô sáng
        c.fillStyle = 'rgba(255,228,92,.28)';
        c.fillRect(roi.x + kh.o[i].cx * ow, roi.y + kh.o[i].cy * oh, ow, oh);
      }
      for (i = 0; i < kh.vat.length; i++) {                        // tâm từng vật mô hình trả về
        var vt = kh.vat[i], mau = MAU_LOAI[vt.loai] || MAU_LOAI[0];
        c.strokeStyle = '#000'; c.lineWidth = 3; c.beginPath(); c.arc(vt.x, vt.y, 5, 0, 6.3); c.stroke();
        c.strokeStyle = mau; c.lineWidth = 1.8; c.beginPath(); c.arc(vt.x, vt.y, 5, 0, 6.3); c.stroke();
        c.fillStyle = mau; c.beginPath(); c.arc(vt.x, vt.y, 1.6, 0, 6.3); c.fill();
        if (vt.loai) {
          c.font = '600 9px ui-monospace, Consolas, monospace'; c.textBaseline = 'bottom';
          var ten_lop = TEN_LOP[vt.loai], rc = c.measureText(ten_lop).width + 6;
          c.fillStyle = 'rgba(0,0,0,.75)'; c.fillRect(vt.x - rc / 2, vt.y - 19, rc, 11);
          c.fillStyle = mau; c.fillText(ten_lop, vt.x - rc / 2 + 3, vt.y - 9);
        }
      }
    }
    c.restore();
    // dải dưới: ảnh đầu vào mô hình và các con số của khung này
    var y0 = KH_CAO + 6, canh = DAI_CAO - 12;
    c.imageSmoothingEnabled = false;
    if (o_dau_vao[h]) c.drawImage(o_dau_vao[h], 6, y0, canh, canh);
    c.imageSmoothingEnabled = true;
    c.strokeStyle = 'rgba(255,255,255,.14)'; c.lineWidth = 0.5; c.beginPath();
    for (i = 1; i < luoi; i++) {
      c.moveTo(6 + i * canh / luoi, y0); c.lineTo(6 + i * canh / luoi, y0 + canh);
      c.moveTo(6, y0 + i * canh / luoi); c.lineTo(6 + canh, y0 + i * canh / luoi);
    }
    c.stroke();
    c.strokeStyle = '#8b939b'; c.lineWidth = 1; c.strokeRect(6, y0, canh, canh);
    if (kh) {
      for (i = 0; i < kh.vat.length; i++) {
        var kb = kh.vat[i].khoi;
        c.fillStyle = MAU_LOAI[kh.vat[i].loai] || MAU_LOAI[0];
        c.beginPath(); c.arc(6 + (kb.x + kb.w / 2) * canh / luoi, y0 + (kb.y + kb.h / 2) * canh / luoi, 2.6, 0, 6.3); c.fill();
      }
    }
    var x_chu = 6 + canh + 9, dong = 0;
    function chu(mau_chu, noi_dung) {
      c.fillStyle = mau_chu; c.fillText(noi_dung, x_chu, y0 + dong * 13.2); dong++;
    }
    c.font = '10px ui-monospace, Consolas, monospace'; c.textBaseline = 'top';
    chu('#b9c0c7', 'ROI ' + roi.w + '×' + roi.h + ' → ' + ts.dau_vao + '×' + ts.dau_vao + ', lưới ' + luoi + '×' + luoi);
    if (kh) {
      chu('#e9edf0', 'khung ' + cm.so_khung + ', chụp ' + soVn((mp.t - kh.t) / 1000, 1) + ' s trước');
      chu('#ffe45c', 'mô hình thấy   s=' + kh.tho.s + ' l=' + kh.tho.l + (kh.tho.ut ? ' ut=' + kh.tho.ut : ''));
      chu(kh.tho.s === kh.that.s && kh.tho.l === kh.that.l ? '#9be36b' : '#ff8a80', 'thật lúc chụp  s=' + kh.that.s + ' l=' + kh.that.l);
      chu('#b9c0c7', ts.so_khung_loc > 1 ? 'lọc ' + ts.so_khung_loc + ' khung → s=' + cm.kq.s + ' l=' + cm.kq.l : 'không lọc');
    } else chu('#e9edf0', 'đang chờ khung đầu tiên…');
    c.font = '7.2px ui-monospace, Consolas, monospace';
    c.fillStyle = '#e9edf0'; c.fillText(mp.jsonCamera(h), x_chu, y0 + canh - 9);
    c.setTransform(1, 0, 0, 1, 0, 0);
    veDauCamera(c, o, h, to);
  }

  // Sơ đồ 8 nhóm tín hiệu: mỗi hướng một mũi tên thẳng và một mũi tên rẽ trái.
  var o_so_do = $('so_do_pha'), c_so_do = o_so_do.getContext('2d');
  var MAU_MUI_TEN = ['#b3382f', '#ffc400', '#27e07a'];
  function muiTen(c, diem, mau, dam) {
    c.strokeStyle = mau; c.fillStyle = mau; c.lineWidth = dam; c.lineCap = 'round'; c.lineJoin = 'round';
    c.beginPath(); c.moveTo(diem[0][0], diem[0][1]);
    for (var i = 1; i < diem.length; i++) c.lineTo(diem[i][0], diem[i][1]);
    c.stroke();
    var a = diem[diem.length - 2], b = diem[diem.length - 1], g = Math.atan2(b[1] - a[1], b[0] - a[0]);
    c.beginPath();
    c.moveTo(b[0] + 5 * Math.cos(g), b[1] + 5 * Math.sin(g));
    c.lineTo(b[0] + 5.5 * Math.cos(g + 2.4), b[1] + 5.5 * Math.sin(g + 2.4));
    c.lineTo(b[0] + 5.5 * Math.cos(g - 2.4), b[1] + 5.5 * Math.sin(g - 2.4));
    c.closePath(); c.fill();
  }
  function veSoDoPha() {
    var c = c_so_do, k = o_so_do.width / 100;
    c.setTransform(k, 0, 0, k, 0, 0);
    c.clearRect(0, 0, 100, 100);
    c.fillStyle = '#3b3e44';
    hinhBoTron(c, 30, 0, 40, 100, 3); c.fill();
    hinhBoTron(c, 0, 30, 100, 40, 3); c.fill();
    for (var h = 1; h <= 4; h++) {
      c.save();
      c.translate(50, 50); c.rotate((h - 1) * Math.PI / 2); c.translate(-50, -50);
      var m_thang = mp.dk.mauNhom((h - 1) * 2, mp.t), m_trai = mp.dk.mauNhom((h - 1) * 2 + 1, mp.t);
      muiTen(c, [[36, 5], [36, 21]], MAU_MUI_TEN[m_thang], m_thang ? 4.2 : 2.6);
      muiTen(c, [[45, 5], [45, 16], [51, 22]], MAU_MUI_TEN[m_trai], m_trai ? 4.2 : 2.6);
      c.fillStyle = '#ffffff'; c.font = '600 7px sans-serif'; c.textAlign = 'center'; c.textBaseline = 'middle';
      c.translate(58, 9); c.rotate(-(h - 1) * Math.PI / 2);
      c.fillText('H' + h, 0, 0);
      c.restore();
    }
  }

  // ---------- Số liệu bên cạnh sa bàn ----------
  function danhSachBuoc(o, kh, hien_tai) {
    var html = '';
    for (var i = 0; i < kh.length; i++) {
      html += '<li' + (i === hien_tai ? ' class="hien_tai"' : '') + '><span>' + QD.tenToHop(kh[i].mat_na) + '</span><b>' + soVn(kh[i].xanh / 1000) + ' s</b></li>';
    }
    o.innerHTML = html;
  }
  function chuKi(kh) {
    var t = 0;
    for (var i = 0; i < kh.length; i++) t += kh[i].xanh + ts.t_vang + ts.t_do;
    return t / 1000;
  }
  function capNhatSoLieu() {
    var dk = mp.dk, t = mp.t, con = dk.conLai(t), P = DieuKhien;
    var mau, ten;
    if (dk.loi) { mau = ['do', 'Lỗi']; ten = 'Tổ hợp đèn xung đột: nháy vàng tất cả.'; }
    else if (dk.pha === P.PHA_KHOI_DONG) { mau = ['do', 'Khởi động']; ten = 'Đỏ toàn phần trước khi chạy.'; }
    else if (dk.pha === P.PHA_VANG) { mau = ['vang', 'Vàng']; ten = 'Kết thúc: ' + QD.tenToHop(dk.vang) + (dk.xanh ? '. Giữ xanh: ' + QD.tenToHop(dk.xanh) : '') + '.'; }
    else if (dk.pha === P.PHA_DO_DON) { mau = ['do', 'Đỏ toàn phần']; ten = 'Sắp xanh: ' + QD.tenToHop(dk.dich) + '.'; }
    else if (dk.ut_huong) { mau = ['uu_tien', 'Ưu tiên']; ten = 'Mở riêng Hướng ' + dk.ut_huong + ' cho xe ' + (UT_TEN[tt.ut_loai] || 'ưu tiên') + '.'; }
    else { mau = ['xanh', 'Xanh']; ten = QD.tenToHop(dk.xanh) + '.'; }
    if (dk.ut_huong && dk.pha !== P.PHA_XANH) mau = [mau[0], mau[1] + ' · sắp ưu tiên'];
    $('pha_mau').dataset.mau = mau[0]; $('pha_mau').textContent = mau[1];
    $('pha_ten').textContent = ten;
    $('dem_nguoc').innerHTML = con < 0 ? '–' : Math.ceil(con / 1000) + ' <small>s</small>';

    // Chế độ thích ứng chốt kế hoạch theo từng lượt (mỗi lượt một trục); chế độ cố định theo cả chu kì.
    var thich_ung = dk.che_do_kh !== QD.CHE_DO_CO_DINH, don_vi = thich_ung ? 'lượt' : 'chu kì';
    $('tieu_de_ke_hoach').textContent = 'Kế hoạch ' + don_vi + ' ' + dk.chu_ki +
      (thich_ung && dk.kh.length ? ' (trục ' + TEN_TRUC[QD.trucCuaMatNa(dk.kh[0].mat_na)] + ')' : '');
    danhSachBuoc($('ke_hoach'), dk.kh, dk.ut_huong ? -1 : dk.i_buoc);
    $('tong_chu_ki').textContent = dk.kh.length ? 'Cả ' + don_vi + ' ' + soVn(chuKi(dk.kh)) + ' s nếu không cắt sớm, tính cả vàng và đỏ toàn phần.' : '';

    var s = [0, 0, 0, 0, 0], l = [0, 0, 0, 0, 0], hl = [false, false, false, false, false], h;
    for (h = 1; h <= 4; h++) { s[h] = dk.dem[h].s; l[h] = dk.dem[h].l; hl[h] = dk.conHan(h, t); }
    var sau_thich_ung = ts.che_do !== QD.CHE_DO_CO_DINH, don_vi_sau = sau_thich_ung ? 'lượt' : 'chu kì';
    var sau = QD.lapKeHoach(ts, s, l, hl, dk.kieuChoChuKi(), dk.truc_ke);
    $('tieu_de_ke_hoach_sau').textContent = (sau_thich_ung ? 'Lượt' : 'Chu kì') + ' sau' +
      (sau_thich_ung ? ' (trục ' + TEN_TRUC[QD.trucCuaMatNa(sau[0].mat_na)] + ')' : '');
    danhSachBuoc($('ke_hoach_sau'), sau, -1);
    $('tong_chu_ki_sau').textContent = 'Nếu chốt theo số xe lúc này. Cả ' + don_vi_sau + ' ' + soVn(chuKi(sau)) + ' s.';

    $('dong_ho').textContent = gioPhut(t) + '  ×' + tt.toc_do;
    var dang = mp.che_do_chay, ngoai = 0, tren = 0, i;
    for (i = 0; i < 2; i++) {
      var tk = mp.thong_ke[i], o = $('hang_tk_' + i).children;
      $('hang_tk_' + i).className = i === dang ? 'dang_dung' : '';
      o[1].textContent = tk.so_xe;
      o[2].textContent = tk.so_xe ? soVn(tk.tong_cho / tk.so_xe / 1000, 1) : '–';
      o[3].textContent = tk.so_xe ? soVn(tk.cho_max / 1000) : '–';
    }
    for (h = 1; h <= 4; h++) {
      ngoai += mp.hang_cho[h].length;
      $('huong_dem_' + h).textContent = 'camera: s ' + mp.dem[h].s + ' · l ' + mp.dem[h].l + ' · ut ' + mp.dem[h].ut + '   chờ ngoài sa bàn: ' + mp.hang_cho[h].length;
    }
    for (i = 0; i < mp.xe.length; i++) if (!mp.xe[i].da_qua) tren++;
    $('dang_cho').textContent = 'Chưa qua vạch dừng: ' + tren + ' xe trên sa bàn, ' + ngoai + ' xe chờ ngoài sa bàn. Đã cắt xanh sớm ' + tt.so_cat_som + ' lần.';
    var n_ut = mp.thong_ke[0].uu_tien + mp.thong_ke[1].uu_tien, t_ut = mp.thong_ke[0].tong_uu_tien + mp.thong_ke[1].tong_uu_tien;
    var tkc = mp.thongKeCamera();
    $('cam_thong_ke').textContent = cai.camera.kieu && tkc.khung ?
      'Từ lúc dựng camera: ' + soVn(tkc.tho_dung / tkc.khung * 100) + ' % số khung đếm đúng cả hai làn lúc chụp; ' +
        soVn(tkc.loc_dung / tkc.khung * 100) + ' % số lần kết quả gửi đi còn đúng với số xe ngay lúc đó (đã tính trễ).' : '';
    $('uu_tien_tk').textContent = n_ut ? 'Xe ưu tiên: ' + n_ut + ' xe, từ lúc tới đến lúc qua vạch dừng trung bình ' + soVn(t_ut / n_ut / 1000, 1) + ' s.' : '';
    capNhatTheHoc();

    var html = '';
    for (i = 0; i < nhat_ky.length; i++) html += '<li><time>' + gioPhut(nhat_ky[i].t) + '</time><span>' + nhat_ky[i].noi_dung + '</span></li>';
    $('nhat_ky').innerHTML = html;
  }

  // Ô "Kiểu chia pha": kiểu đang dùng của từng trục và việc bộ tự học đang làm.
  function capNhatTheHoc() {
    var dk = mp.dk, hc = dk.hoc, html = '', ghi_chu = '';
    function dong(a, b) { html += '<dt>' + a + '</dt><dd>' + b + '</dd>'; }
    if (ts.che_do === QD.CHE_DO_CO_DINH) dong('Chế độ', 'Cố định: thứ tự và thời gian không đổi theo xe.');
    else if (ts.kieu_pha !== QD.KIEU_TU_HOC) dong('Cả hai trục', '<b>' + tenKieu(ts.kieu_pha) + '</b> (chọn tay)');
    else {
      for (var i = 0; i < 2; i++) {
        var dang_thu = hc.l_thu && hc.l_truc === i, them;
        if (hc.kieu[i] === QD.KIEU_RIENG) {
          them = dang_thu ? 'lượt này đang thử "thẳng trước"' :
            (hc.b[0] < 0 && hc.b[1] < 0 ? 'đang đo mốc của kiểu riêng' :
              (hc.dem[i] >= hc.cach[i] ? 'lượt tới thử "thẳng trước"' : 'thử "thẳng trước" sau ' + (hc.cach[i] - hc.dem[i]) + ' lượt'));
          if (hc.q[i] >= 0) them += '; lần gần nhất thẳng trước cần ' + soVn(hc.q[i]) + ' %';
        } else {
          them = dang_thu ? 'lượt này chạy "riêng" để lấy mốc so sánh' : (hc.q[i] >= 0 ? 'đang cần ' + soVn(hc.q[i]) + ' %' : 'đang đo');
        }
        dong('Trục ' + TEN_TRUC[i], '<b>' + tenKieu(hc.kieu[i]) + '</b><br>' + them);
      }
      ghi_chu = 'Số % là thời gian đèn mà "thẳng trước" cần so với "riêng" cho cùng số xe. Hai lần thử liền dưới ' + (100 - ts.hoc_nguong) +
        ' % thì đổi sang thẳng trước; lên tới ' + soVn(100 + ts.hoc_nguong / 2) + ' % thì về riêng.';
    }
    $('hoc_ghi_chu').textContent = ghi_chu;
    $('hoc_noi_dung').innerHTML = html;
  }

  // ---------- So sánh các phương án đèn (chạy nhanh, không vẽ) ----------
  // Cùng một dòng xe và thông số đang đặt, chạy lần lượt: cố định, rồi thích ứng với từng kiểu chia pha.
  var dang_so_sanh = false;
  function chaySoSanh() {
    if (dang_so_sanh) return;
    dang_so_sanh = true;
    var PHUT = 60, so_buoc = PHUT * 60000 / NHIP, o = $('ket_qua_so_sanh'), may = [], da = 0;
    var PA = [
      { ten: 'Cố định', che_do: QD.CHE_DO_CO_DINH, kieu: ts.kieu_pha },
      { ten: 'Riêng từng hướng', che_do: QD.CHE_DO_THICH_UNG, kieu: QD.KIEU_RIENG },
      { ten: 'Thẳng trước', che_do: QD.CHE_DO_THICH_UNG, kieu: QD.KIEU_THANG_TRUOC },
      { ten: 'Đối xứng', che_do: QD.CHE_DO_THICH_UNG, kieu: QD.KIEU_DOI_XUNG },
      { ten: 'Tự học', che_do: QD.CHE_DO_THICH_UNG, kieu: QD.KIEU_TU_HOC }
    ];
    PA.forEach(function (pa) {
      var ts2 = JSON.parse(JSON.stringify(ts));
      ts2.che_do = pa.che_do; ts2.kieu_pha = pa.kieu;
      may.push(new MoPhong({ hat: 2026, ts: ts2, cai: JSON.parse(JSON.stringify(cai)) }));
    });
    var tong = so_buoc * may.length;
    $('nut_so_sanh').disabled = true;
    function motDot() {
      var het = Date.now() + 30;
      while (da < tong && Date.now() < het) { may[Math.floor(da / so_buoc)].buoc(NHIP); da++; }
      if (da < tong) {
        o.innerHTML = '<p class="ghi_chu">Đang chạy "' + PA[Math.floor(da / so_buoc)].ten.toLowerCase() + '"… ' + Math.round(da / tong * 100) + ' %</p>';
        setTimeout(motDot, 0);
        return;
      }
      var kq = may.map(function (m) { return m.tongHop(); }), tot = 1, i;
      for (i = 2; i < kq.length; i++) if (kq[i].cho_tb < kq[tot].cho_tb) tot = i;
      var html = '<table class="so_lieu"><thead><tr><th>' + PHUT + ' phút</th><th>Chờ TB</th><th>Lâu nhất</th><th>Đã qua</th><th>Kẹt</th></tr></thead><tbody>';
      for (i = 0; i < kq.length; i++) {
        html += '<tr' + (i === tot ? ' class="dang_dung"' : '') + '><td>' + PA[i].ten + '</td><td>' + soVn(kq[i].cho_tb, 1) + '</td><td>' + soVn(kq[i].cho_max) +
          '</td><td>' + kq[i].da_qua + '</td><td>' + kq[i].con_ket + '</td></tr>';
      }
      var co_dinh = kq[0].cho_tb, hoc = kq[4].cho_tb, rieng = kq[1].cho_tb;
      var so_cd = co_dinh > 0 ? (kq[tot].cho_tb - co_dinh) / co_dinh * 100 : 0;
      var so_rieng = rieng > 0 ? (hoc - rieng) / rieng * 100 : 0;
      html += '</tbody></table><p class="ket_luan">Với dòng xe đang đặt, chờ ít nhất là kiểu "' + PA[tot].ten.toLowerCase() + '": ' +
        (Math.abs(so_cd) < 1 ? 'gần bằng' : (so_cd < 0 ? 'giảm ' : 'tăng ') + soVn(Math.abs(so_cd)) + ' % so với') + ' cố định. ' +
        (tot === 4 ? '' : 'Tự học ' + (Math.abs(so_rieng) < 1 ? 'gần bằng' : (so_rieng < 0 ? 'chờ ít hơn ' : 'chờ lâu hơn ') + soVn(Math.abs(so_rieng)) + ' % so với') +
          ' kiểu riêng từng hướng. ') +
        'Thời gian tính bằng giây, kể cả của xe còn kẹt lại khi hết giờ.</p>';
      o.innerHTML = html;
      $('nut_so_sanh').disabled = false;
      dang_so_sanh = false;
    }
    motDot();
  }

  // ---------- Kích thước theo màn hình ----------
  function datCo(o, rong, cao) {
    var ti_le = Math.min(window.devicePixelRatio || 1, 2.5);
    o.style.width = rong + 'px'; o.style.height = cao + 'px';
    var w = Math.round(rong * ti_le), h = Math.round(cao * ti_le);
    if (o.width !== w || o.height !== h) { o.width = w; o.height = h; }
  }
  function xepCho() {
    var ung_dung = $('ung_dung'), san = $('san'), dien_thoai = window.innerWidth <= 900, canh, i;
    // ô camera: cao khoảng 1/5 màn hình
    var o_dau = o_cam[1].parentNode;
    var cao_cam = dien_thoai ? 1e9 : Math.max(150, Math.min(344, window.innerHeight * 0.30));
    var rong_cam = Math.floor(Math.min(o_dau.clientWidth, cao_cam / TI_LE_CAM));
    for (i = 1; i <= 4; i++) datCo(o_cam[i], rong_cam, Math.round(rong_cam * TI_LE_CAM));
    if (dien_thoai) {
      ung_dung.classList.add('hep_san');
      canh = san.clientWidth;
    } else {
      ung_dung.classList.remove('hep_san');
      var W = san.clientWidth, Hc = san.clientHeight;
      canh = Math.min(Hc, W - 2 * 212 - 24);       // chừa chỗ cho hai cột số liệu hai bên
      if (canh < Math.min(420, Hc)) {              // quá hẹp: sa bàn ở trên, số liệu xuống dưới
        ung_dung.classList.add('hep_san');
        canh = Math.min(san.clientWidth, Hc);
        san.style.overflowY = 'auto';
      } else san.style.overflowY = '';
      $('cot_trai').style.maxHeight = $('cot_phai').style.maxHeight = ung_dung.classList.contains('hep_san') ? '' : canh + 'px';
    }
    canh = Math.max(240, Math.floor(canh));
    datCo(o_sa_ban, canh, canh);
  }

  // ---------- Vòng lặp ----------
  function khungHinh(bay_gio) {
    var troi = Math.min(100, bay_gio - tt.t_truoc);
    tt.t_truoc = bay_gio;
    if (tt.chay) {
      tt.du += troi * tt.toc_do;
      var n = 0;
      while (tt.du >= NHIP && n < 60) { mp.buoc(NHIP); tt.du -= NHIP; n++; }
      if (n === 60) tt.du = 0;
    }
    tt.khung++;
    veSaBan();
    veSoDoPha();
    for (var h = 1; h <= 4; h++) if ((tt.khung + h) % 2 === 0) veCamera(h);
    if (tt.khung % 6 === 0) capNhatSoLieu();
    requestAnimationFrame(khungHinh);
  }

  // ---------- Khởi động ----------
  (function () {
    var day = $('day_camera');
    for (var h = 1; h <= 4; h++) {
      var khoi = document.createElement('div');
      khoi.className = 'camera';
      khoi.innerHTML = '<canvas id="cam_' + h + '" width="320" height="344" aria-label="Camera hướng ' + h + '"></canvas>';
      day.appendChild(khoi);
      o_cam[h] = khoi.querySelector('canvas');
      c_cam[h] = o_cam[h].getContext('2d');
    }
    docLai();
    mp.luu_khung = true;        // giữ vị trí xe của từng khung camera để vẽ lại
    mp.datCamera();
    dungBang();
    dongBoNut();
    for (var i = 0; i < 45000 / NHIP; i++) mp.buoc(NHIP);   // chạy trước 45 giây để mở trang là thấy xe
    xepCho();
    window.addEventListener('resize', xepCho);
    capNhatSoLieu();
    tt.t_truoc = performance.now();
    requestAnimationFrame(khungHinh);
    window.MO_PHONG = mp;   // để gõ thử trong bảng điều khiển của trình duyệt
  })();
})();
