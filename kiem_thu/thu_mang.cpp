// thu_mang.cpp — Kiểm tra trên máy tính các phần mới của trang web trung tâm và phần nạp qua WiFi:
//   - kiem_anh_fw.h: nhận đúng firmware trung tâm / camera, chặn tệp sai, sai chip, firmware của board khác,
//     dù tệp được chia thành đoạn ngẫu nhiên (dấu nằm vắt qua hai đoạn); hai bản trong trung_tam/ và camera/
//     phải giống hệt nhau;
//   - thong_so.h: đọc số giây, giới hạn an toàn của từng thông số;
//   - xe_ao.h: xe ưu tiên ảo được mở đèn rồi biến mất, chạy cùng camera thật hay không có camera; dòng xe giả.
// Chạy:  g++ -std=c++11 -Wall -Wextra -o /tmp/thu_mang kiem_thu/thu_mang.cpp && /tmp/thu_mang [trung_tam.bin camera_esp32.bin camera_s3.bin]
// Không truyền tệp .bin thì chỉ thử với ảnh firmware tự dựng.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include "../firmware/trung_tam/kiem_anh_fw.h"
#include "../firmware/trung_tam/thong_so.h"
#include "../firmware/trung_tam/xe_ao.h"

static int so_loi = 0;
#define KIEM(dk, ...) do { if (!(dk)) { so_loi++; printf("  SAI: "); printf(__VA_ARGS__); printf("\n"); } } while (0)

static std::vector<uint8_t> docTep(const char *duong_dan) {
  std::vector<uint8_t> b;
  FILE *f = fopen(duong_dan, "rb");
  if (!f) return b;
  int c;
  while ((c = fgetc(f)) != EOF) b.push_back((uint8_t)c);
  fclose(f);
  return b;
}

static unsigned hat = 1;
static unsigned ngauNhien() { hat = hat * 1103515245u + 12345u; return hat >> 8; }

// Cho tệp qua bộ kiểm tra theo từng đoạn ngẫu nhiên; trả về kết quả và phiên bản đọc được.
static int kiemTep(const std::vector<uint8_t> &b, uint16_t chip, const char *loai_nguoc, std::string *pb = nullptr, int lan = 0) {
  KiemAnhFw k;
  k.batDau(chip, loai_nguoc);
  size_t i = 0;
  hat = 7 + lan;
  while (i < b.size()) {
    size_t n = lan == 0 ? 1436 : 1 + ngauNhien() % 1500;
    if (n > b.size() - i) n = b.size() - i;
    k.them(b.data() + i, n);
    i += n;
  }
  if (pb) *pb = k.phienBan();
  return k.ketQua();
}

static std::vector<uint8_t> anhGia(uint16_t chip, const char *dau, size_t co) {
  std::vector<uint8_t> b(co, 0x5A);
  b[0] = 0xE9; b[12] = chip & 0xFF; b[13] = chip >> 8;
  size_t vt = co / 2 - 7;                                    // đặt dấu vắt qua ranh giới đoạn 1436 byte
  for (size_t j = 0; dau[j]; j++) b[vt + j] = (uint8_t)dau[j];
  b[vt + strlen(dau)] = 0;
  return b;
}

static void thuKiemAnh(int argc, char **argv) {
  printf("1. Kiểm tra tệp firmware\n");
  // hai bản của kiem_anh_fw.h phải giống hệt nhau
  std::vector<uint8_t> a = docTep("firmware/trung_tam/kiem_anh_fw.h"), c = docTep("firmware/camera/kiem_anh_fw.h");
  KIEM(!a.empty() && a == c, "firmware/trung_tam/kiem_anh_fw.h và firmware/camera/kiem_anh_fw.h phải giống hệt nhau (chạy từ thư mục gốc repo)");

  // ảnh tự dựng
  std::vector<uint8_t> tt = anhGia(CHIP_ESP32S3, "DGTTM-FW|trung_tam|Oct 27 2026 16:40:01", 2 * 1436);
  std::vector<uint8_t> cam = anhGia(CHIP_ESP32, "DGTTM-FW|camera|Oct 27 2026 16:41:02", 3 * 1436);
  std::string pb;
  for (int lan = 0; lan < 30; lan++) {
    KIEM(kiemTep(tt, CHIP_ESP32S3, "mat_gnurt", &pb, lan) == KA_DUNG, "ảnh trung tâm tự dựng phải được nhận (lần %d)", lan);
    KIEM(pb == "Oct 27 2026 16:40:01", "đọc sai phiên bản: %s", pb.c_str());
    KIEM(kiemTep(cam, CHIP_ESP32, "aremac", &pb, lan) == KA_DUNG, "ảnh camera tự dựng phải được nhận (lần %d)", lan);
    KIEM(pb == "Oct 27 2026 16:41:02", "đọc sai phiên bản camera: %s", pb.c_str());
  }
  KIEM(kiemTep(cam, CHIP_BAT_KI, "aremac") == KA_DUNG, "trung tâm giữ hộ firmware camera ESP32");
  KIEM(kiemTep(cam, CHIP_ESP32S3, "aremac") == KA_SAI_CHIP, "firmware ESP32 nạp cho ESP32-S3 phải bị chặn");
  KIEM(kiemTep(cam, CHIP_ESP32, "mat_gnurt") == KA_SAI_CHIP || kiemTep(cam, CHIP_ESP32S3, "mat_gnurt") == KA_SAI_CHIP, "camera vào trung tâm: sai chip");
  std::vector<uint8_t> cam_s3 = anhGia(CHIP_ESP32S3, "DGTTM-FW|camera|x", 2000);
  KIEM(kiemTep(cam_s3, CHIP_ESP32S3, "mat_gnurt") == KA_KHONG_CO_DAU, "firmware camera ESP32-S3 không được nạp vào trung tâm");
  KIEM(kiemTep(tt, CHIP_BAT_KI, "aremac") == KA_KHONG_CO_DAU, "firmware trung tâm không được giữ làm firmware camera");
  std::vector<uint8_t> chu(5000, 'a');
  KIEM(kiemTep(chu, CHIP_ESP32, "aremac") == KA_KHONG_PHAI_FIRMWARE, "tệp văn bản phải bị chặn");
  std::vector<uint8_t> ngan(tt.begin(), tt.begin() + 40);
  KIEM(kiemTep(ngan, CHIP_ESP32S3, "mat_gnurt") == KA_QUA_NGAN, "tệp quá ngắn");
  std::vector<uint8_t> nua(tt.begin(), tt.begin() + 1200);   // tải lên dở, chưa tới dấu
  KIEM(kiemTep(nua, CHIP_ESP32S3, "mat_gnurt") == KA_KHONG_CO_DAU, "tệp tải dở (chưa tới dấu) phải bị chặn");
  {
    KiemAnhFw k;
    k.batDau(CHIP_ESP32S3, "aremac");
    k.them(cam.data(), 20);
    KIEM(k.ketQuaSom() == KA_SAI_CHIP, "sai chip phải bị phát hiện ngay ở đoạn đầu");
  }
  // dấu gần giống (thiếu một kí tự) không được tính
  std::vector<uint8_t> gan = anhGia(CHIP_ESP32, "DGTTM-FW|camer|x", 3000);
  KIEM(kiemTep(gan, CHIP_ESP32, "aremac") == KA_KHONG_CO_DAU, "dấu gần giống không được tính");
  std::vector<uint8_t> lap = anhGia(CHIP_ESP32, "DGTTM-DGTTM-FW|camera|v1", 3000);   // tiền tố lặp: phải tìm được nhờ bảng KMP
  KIEM(kiemTep(lap, CHIP_ESP32, "aremac", &pb) == KA_DUNG && pb == "v1", "dấu đứng sau tiền tố lặp");

  // tệp .bin thật (nếu có)
  if (argc >= 4) {
    std::vector<uint8_t> b_tt = docTep(argv[1]), b_cam = docTep(argv[2]), b_s3 = docTep(argv[3]);
    KIEM(!b_tt.empty() && !b_cam.empty() && !b_s3.empty(), "không đọc được tệp .bin");
    std::string p1, p2, p3;
    for (int lan = 0; lan < 3; lan++) {
      KIEM(kiemTep(b_tt, CHIP_ESP32S3, "mat_gnurt", &p1, lan) == KA_DUNG, "trung_tam.bin thật phải được trung tâm nhận");
      KIEM(kiemTep(b_cam, CHIP_ESP32, "aremac", &p2, lan) == KA_DUNG, "camera.bin (ESP32-CAM) thật phải được ESP32-CAM nhận");
      KIEM(kiemTep(b_s3, CHIP_ESP32S3, "aremac", &p3, lan) == KA_DUNG, "camera.bin (ESP32-S3-CAM) thật phải được ESP32-S3-CAM nhận");
    }
    KIEM(kiemTep(b_cam, CHIP_BAT_KI, "aremac") == KA_DUNG && kiemTep(b_s3, CHIP_BAT_KI, "aremac") == KA_DUNG, "trung tâm giữ hộ cả hai loại firmware camera");
    KIEM(kiemTep(b_tt, CHIP_BAT_KI, "aremac") == KA_KHONG_CO_DAU, "trung_tam.bin không được giữ làm firmware camera");
    KIEM(kiemTep(b_s3, CHIP_ESP32S3, "mat_gnurt") == KA_KHONG_CO_DAU, "camera.bin (S3) không được nạp vào trung tâm");
    KIEM(kiemTep(b_cam, CHIP_ESP32S3, "mat_gnurt") == KA_SAI_CHIP, "camera.bin (ESP32) vào trung tâm: sai chip");
    KIEM(kiemTep(b_cam, CHIP_ESP32S3, "aremac") == KA_SAI_CHIP, "camera ESP32 không nạp được vào ESP32-S3-CAM");
    KIEM(kiemTep(b_s3, CHIP_ESP32, "aremac") == KA_SAI_CHIP, "camera ESP32-S3 không nạp được vào ESP32-CAM");
    KIEM(p1.size() >= 19 && p2.size() >= 19 && p3.size() >= 19, "phải đọc được phiên bản trong tệp thật");
    printf("  Tệp thật: trung tâm %zu KB bản \"%s\"; camera ESP32 %zu KB bản \"%s\"; camera ESP32-S3 %zu KB bản \"%s\"\n",
           b_tt.size() / 1024, p1.c_str(), b_cam.size() / 1024, p2.c_str(), b_s3.size() / 1024, p3.c_str());
  } else printf("  (không có tệp .bin thật, chỉ thử ảnh tự dựng)\n");
}

static void thuThamSo() {
  printf("2. Thông số đèn\n");
  KIEM(docGiay("3") == 3000 && docGiay("3.5") == 3500 && docGiay("3,5") == 3500 && docGiay(" 12 ") == 12000, "docGiay số thường");
  KIEM(docGiay("") == -1 && docGiay("-2") == -1 && docGiay("abc") == -1 && docGiay("3s") == -1, "docGiay số sai");
  ThamSo ts;
  memset(&ts, 0, sizeof(ts));
  ts.t_min = 15000; ts.t_max = 90000; ts.t_vang = 3000; ts.t_do = 2000; ts.xe_quy_doi = 10;
  KIEM(datThamSo(ts, "vang", "4") == 0 && ts.t_vang == 4000, "đặt vàng 4 s");
  KIEM(datThamSo(ts, "vang", "2") == 3 && ts.t_vang == 4000, "vàng dưới 3 s phải bị chặn, giữ giá trị cũ");
  KIEM(datThamSo(ts, "dotp", "0.5") == 3, "đỏ toàn phần dưới 1 s phải bị chặn");
  KIEM(datThamSo(ts, "tmin", "4") == 3, "xanh ngắn nhất dưới 5 s phải bị chặn");
  KIEM(datThamSo(ts, "tmax", "12") == 4 && ts.t_max == 90000, "xanh dài nhất nhỏ hơn ngắn nhất phải bị chặn");
  KIEM(datThamSo(ts, "tmin", "10") == 0 && datThamSo(ts, "tmax", "12") == 0 && ts.t_max == 12000, "đặt Tmin 10, Tmax 12");
  KIEM(datThamSo(ts, "quydoi", "5") == 0 && ts.xe_quy_doi == 5, "quy đổi là số nguyên, không nhân 1000");
  KIEM(datThamSo(ts, "quydoi", "2.5") == 2, "quy đổi không nhận số lẻ");
  KIEM(datThamSo(ts, "khongco", "3") == 1, "tên sai");
  KIEM(datThamSo(ts, "catsom", "0") == 0 && ts.cat_som == 0, "cắt sớm 0 là tắt");
}

static ThamSo tsMacDinh() {
  ThamSo ts;
  memset(&ts, 0, sizeof(ts));
  ts.che_do = CHE_DO_THICH_UNG; ts.t_min = 15000; ts.t_max = 90000; ts.cong_them = 5000; ts.xe_quy_doi = 10;
  ts.t_vang = 3000; ts.t_do = 2000; ts.cd_thang = 30000; ts.cd_trai = 15000; ts.mat_tin_hieu = 3000;
  ts.ut_xanh_an_toan = 5000; ts.ut_giu = 1000; ts.ut_toi_da = 60000; ts.cat_som = 5000; ts.kieu_pha = KIEU_RIENG;
  ts.hoc_cach_thu = 4; ts.hoc_so_xe = 4; ts.hoc_nguong = 10;
  return ts;
}

static int mo_uu_tien[5], het_uu_tien[5];
static void suKien(uint8_t loai, uint8_t dl) {
  if (loai == SK_UU_TIEN && dl <= 4) mo_uu_tien[dl]++;
  if (loai == SK_HET_UU_TIEN && dl <= 4) het_uu_tien[dl]++;
}

// Chạy t_giay giây, bước 40 ms. co_cam: 4 camera thật gửi số (s, l cố định) mỗi giây, không thấy xe ưu tiên nào.
static void chay(DieuKhien &dk, GiaLap &gl, uint32_t &t, int t_giay, bool co_cam, int s = 2, int l = 1) {
  for (int i = 0; i < t_giay * 25; i++) {
    t += 40;
    bool song[5] = {false, co_cam, co_cam, co_cam, co_cam};
    if (co_cam && t % 1000 == 0) {
      for (int h = 1; h <= 4; h++) { int emg = 0, ut = 0; gl.chenVaoBanTin(h, emg, ut); dk.nhanDem(h, s, l, emg, t, false, ut); }
    }
    gl.chay(dk, t, song);
    dk.capNhat(t);
    KIEM(!dk.loi, "bộ điều khiển báo lỗi an toàn lúc %u ms", t);
  }
}

static void thuXeAo() {
  printf("3. Xe ưu tiên ảo, dòng xe giả\n");
  for (int co_cam = 1; co_cam >= 0; co_cam--) {
    ThamSo ts = tsMacDinh();
    DieuKhien dk(&ts);
    dk.khiSuKien = suKien;
    GiaLap gl;
    memset(mo_uu_tien, 0, sizeof(mo_uu_tien)); memset(het_uu_tien, 0, sizeof(het_uu_tien));
    uint32_t t = 0;
    dk.khoiDong(t);
    chay(dk, gl, t, 20, co_cam);
    int h = gl.tha(3, 1, 2, t);
    KIEM(h == 3 && gl.xe[3].ut == 1 && gl.xe[3].lan == 2, "thả xe cứu hỏa làn trong Hướng 3");
    KIEM(gl.tha(3, 2, 1, t) == 0, "Hướng 3 đang có xe ảo thì không thả thêm");
    uint32_t t0 = t, t_mo = 0, t_het = 0;
    for (int i = 0; i < 60 * 25 && !t_het; i++) {
      t += 40;
      bool song[5] = {false, (bool)co_cam, (bool)co_cam, (bool)co_cam, (bool)co_cam};
      if (co_cam && t % 1000 == 0)
        for (int k = 1; k <= 4; k++) { int emg = 0, ut = 0; gl.chenVaoBanTin(k, emg, ut); dk.nhanDem(k, 2, 1, emg, t, false, ut); }
      gl.chay(dk, t, song);
      dk.capNhat(t);
      if (!t_mo && dk.ut_huong == 3 && dk.pha == PHA_XANH) t_mo = t;
      if (t_mo && !t_het && mo_uu_tien[3] && het_uu_tien[3]) t_het = t;
    }
    KIEM(t_mo && t_mo - t0 <= 15000, "%s: Hướng 3 phải được mở xanh ưu tiên trong 15 s (mất %u ms)", co_cam ? "có camera" : "không camera", t_mo - t0);
    KIEM(t_het && t_het - t_mo <= XE_AO_QUA_MS + ts.ut_giu + 500, "%s: xe ảo qua rồi thì hết ưu tiên ngay (sau %u ms)", co_cam ? "có camera" : "không camera", t_het - t_mo);
    KIEM(gl.xe[3].ut == 0 && gl.so_xe_ao_qua == 1, "xe ảo phải biến mất sau khi qua");
    KIEM(dk.dem[3].ut == 0, "bộ điều khiển không còn thấy xe ưu tiên ở Hướng 3");
    printf("  %s: xe cứu hỏa ảo được mở đèn sau %.1f s, hết ưu tiên %.1f s sau khi mở\n", co_cam ? "Có camera   " : "Không camera",
           (t_mo - t0) / 1000.0, (t_het - t_mo) / 1000.0);
    // camera thật thấy xe ưu tiên thì xe thật được ưu tiên, xe ảo không đè lên
    if (co_cam) {
      gl.tha(2, 4, 1, t);
      int emg = 1, ut = 2;
      gl.chenVaoBanTin(2, emg, ut);
      KIEM(emg == 1 && ut == 2, "camera thấy xe quân sự thật: giữ xe thật, không thay bằng xe ảo");
      emg = 0; ut = 0;
      gl.chenVaoBanTin(2, emg, ut);
      KIEM(emg == 1 && ut == 4, "camera không thấy gì: thêm xe cứu thương ảo");
      gl.bo(2);
    }
  }
  // thả ngẫu nhiên: 4 hướng khác nhau, lần thứ 5 không còn chỗ
  {
    GiaLap gl;
    int da[5] = {0, 0, 0, 0, 0};
    for (int i = 0; i < 4; i++) { int h = gl.tha(0, 0, 0, 0); KIEM(h >= 1 && h <= 4 && !da[h], "thả ngẫu nhiên lần %d ra hướng %d", i, h); if (h >= 1 && h <= 4) da[h] = 1; }
    KIEM(gl.tha(0, 0, 0, 0) == 0, "cả 4 hướng có xe ảo thì không thả thêm");
    for (int h = 1; h <= 4; h++) KIEM(gl.xe[h].ut >= 1 && gl.xe[h].ut <= 4 && (gl.xe[h].lan == 1 || gl.xe[h].lan == 2), "loại và làn ngẫu nhiên hợp lệ");
  }
  // hết hạn: xe ảo ở hướng không bao giờ được mở (chế độ lỗi giả: bỏ qua) — ở đây đặt ut_toi_da lớn, xe phải tự hủy sau 90 s
  // dòng xe giả, không có camera: số xe thay đổi, không vượt 4, đèn chạy thích ứng không lỗi, mọi hướng đều được xanh
  {
    ThamSo ts = tsMacDinh();
    DieuKhien dk(&ts);
    GiaLap gl;
    gl.dong_xe = true; gl.mat_do = 55;
    uint32_t t = 0;
    dk.khoiDong(t);
    int lon_nhat = 0, co_xe = 0;
    uint32_t xanh[5] = {0, 0, 0, 0, 0};
    for (int i = 0; i < 600 * 25; i++) {
      t += 40;
      bool song[5] = {false, false, false, false, false};
      gl.chay(dk, t, song);
      dk.capNhat(t);
      for (int h = 1; h <= 4; h++) {
        if (gl.so_s[h] > lon_nhat) lon_nhat = gl.so_s[h];
        if (gl.so_l[h] > lon_nhat) lon_nhat = gl.so_l[h];
        if (dk.conHan(h, t) && dk.dem[h].s + dk.dem[h].l > 0) co_xe++;
        if (dk.mauNhom((h - 1) * 2, t) == MAU_XANH) xanh[h] += 40;
      }
      KIEM(!dk.loi, "dòng xe giả: lỗi an toàn");
    }
    KIEM(lon_nhat <= DONG_GIA_TOI_DA && lon_nhat >= 2, "dòng xe giả: số xe mỗi làn trong khoảng 0..4 (lớn nhất %d)", lon_nhat);
    KIEM(co_xe > 1000, "dòng xe giả: bộ điều khiển phải thấy có xe");
    for (int h = 1; h <= 4; h++) KIEM(xanh[h] > 30000, "dòng xe giả: Hướng %d phải được xanh (được %u ms trong 10 phút)", h, xanh[h]);
    for (int h = 1; h <= 4; h++) KIEM(dk.conHan(h, t), "dòng xe giả: Hướng %d không được mất tín hiệu", h);
    printf("  Dòng xe giả 10 phút, không camera: mỗi làn tối đa %d xe; thời gian xanh đi thẳng H1..H4: %u, %u, %u, %u s\n",
           lon_nhat, xanh[1] / 1000, xanh[2] / 1000, xanh[3] / 1000, xanh[4] / 1000);
    // tắt dòng xe giả: sau 3 s các hướng mất tín hiệu, chạy thời gian cố định như khi thiếu camera
    gl.dong_xe = false;
    for (int i = 0; i < 5 * 25; i++) { t += 40; bool song[5] = {false, false, false, false, false}; gl.chay(dk, t, song); dk.capNhat(t); }
    KIEM(!dk.conHan(1, t), "tắt dòng xe giả: hướng không có camera trở lại mất tín hiệu");
  }
  // tự thả: bật 1 phút, chạy 5 phút có camera → khoảng 5 xe ảo, đều qua được
  {
    ThamSo ts = tsMacDinh();
    DieuKhien dk(&ts);
    GiaLap gl;
    gl.tu_tha_ms = 60000;
    uint32_t t = 0;
    dk.khoiDong(t);
    chay(dk, gl, t, 330, true);
    KIEM(gl.so_xe_ao_qua >= 4 && gl.so_xe_ao_qua <= 5, "tự thả mỗi phút trong 5,5 phút: 4–5 xe ảo qua (được %u)", gl.so_xe_ao_qua);
  }
}

int main(int argc, char **argv) {
  thuKiemAnh(argc, argv);
  thuThamSo();
  thuXeAo();
  printf(so_loi ? "KHÔNG ĐẠT: %d lỗi.\n" : "ĐẠT: không có lỗi.\n", so_loi);
  return so_loi ? 1 : 0;
}
