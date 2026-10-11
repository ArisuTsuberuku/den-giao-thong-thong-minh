// trung_tam.ino — Board trung tâm ESP32-S3 N16R8 của sa bàn đèn giao thông thông minh.
// Việc của board: nhận số xe từng làn do 4 camera gửi về (ESP-NOW) hoặc gõ tay (Serial),
// lập kế hoạch đèn cho từng chu kì và điều khiển 24 bóng đèn của 8 cụm.
//
//   quyet_dinh.h  bộ luật lập kế hoạch một chu kì (sửa luật ở đây, nhớ sửa cả web/quyet_dinh.js)
//   dieu_khien.h  máy trạng thái xanh → vàng → đỏ toàn phần, xe ưu tiên
//   den.h         xuất chân đèn, có kiểm tra xung đột
//   nhan_tin.h    nhận ESP-NOW, giải mã JSON
//   cau_hinh.h    chân, kênh WiFi, thời gian
//
// Thư viện cần cài: ArduinoJson (Library Manager). Cách nạp và cách thử: xem README.md.
// Không dùng delay() trong loop(); mọi thứ chạy theo millis().
#include "cau_hinh.h"
#include "dieu_khien.h"
#include "den.h"
#include "nhan_tin.h"

ThamSo ts = {
  CHE_DO_BAN_DAU, T_MIN, T_MAX, CONG_THEM, XE_QUY_DOI, T_VANG, T_DO_TOAN_PHAN,
  CD_THANG, CD_TRAI, MAT_TIN_HIEU, UT_XANH_AN_TOAN, UT_GIU, UT_TOI_DA
};
DieuKhien dk(&ts);

uint32_t he_so_tua = HE_SO_TUA_NHANH;
uint32_t t_dieu_khien = 0;        // đồng hồ của bộ điều khiển (ms); chạy nhanh hơn millis() khi tua
uint32_t t_millis_truoc = 0;

// thống kê gói tin của từng camera, để đo tỉ lệ mất gói
struct ThongKeGoi { long seq_cuoi; uint32_t so_goi, so_mat; };
ThongKeGoi goi[5];

char dong_serial[160];
uint8_t dai_dong = 0;

// ---------- In ra Serial ----------
void inThoiGian() {
  Serial.printf("[%7.1f s] ", t_dieu_khien / 1000.0);
}

void inToHop(uint8_t m) {
  bool dau = true;
  for (int h = 1; h <= 4; h++) {
    bool thang = m & bitThang(h), trai = m & bitTrai(h);
    if (!thang && !trai) continue;
    if (!dau) Serial.print(" + ");
    dau = false;
    Serial.printf("H%d %s", h, (thang && trai) ? "thẳng và trái" : (thang ? "thẳng" : "trái"));
  }
  if (dau) Serial.print("đỏ toàn phần");
}

void inSoDem() {
  for (int h = 1; h <= 4; h++) {
    if (dk.conHan(h, t_dieu_khien)) Serial.printf(" H%d %u/%u", h, dk.dem[h].s, dk.dem[h].l);
    else Serial.printf(" H%d mất tín hiệu", h);
  }
}

void khiSuKien(uint8_t loai, uint8_t dl) {
  inThoiGian();
  switch (loai) {
    case SK_CHU_KI:
      Serial.printf("Chu kì %lu (%s). Số xe làn ngoài/làn trong:", (unsigned long)dk.chu_ki,
                    ts.che_do == CHE_DO_CO_DINH ? "cố định" : "thích ứng");
      inSoDem();
      Serial.print(". Kế hoạch: ");
      for (int i = 0; i < dk.kh.so_buoc; i++) {
        if (i) Serial.print(" -> ");
        inToHop(dk.kh.buoc[i].mat_na);
        Serial.printf(" %lu s", (unsigned long)(dk.kh.buoc[i].xanh / 1000));
      }
      Serial.println();
      break;
    case SK_BUOC:
      Serial.print("Xanh: ");
      inToHop(dk.xanh);
      if (dk.ut_huong) Serial.println(" (ưu tiên)");
      else Serial.printf(" trong %lu s\n", (unsigned long)(dk.dich_xanh / 1000));
      break;
    case SK_UU_TIEN:
      Serial.printf("Camera Hướng %u thấy xe ưu tiên: kết thúc bước đang chạy rồi mở riêng Hướng %u\n", dl, dl);
      break;
    case SK_HET_UU_TIEN:
      Serial.printf("Hết ưu tiên Hướng %u, lập kế hoạch chu kì mới\n", dl);
      break;
    case SK_LOI:
      Serial.println("LỖI AN TOÀN: tổ hợp đèn xung đột. Tất cả nháy vàng. Bấm RST để chạy lại.");
      break;
  }
}

void inTrangThai() {
  inThoiGian();
  Serial.printf("Chế độ %s, tua x%lu, chu kì %lu, bước %d/%u. ", ts.che_do == CHE_DO_CO_DINH ? "cố định" : "thích ứng",
                (unsigned long)he_so_tua, (unsigned long)dk.chu_ki, dk.i_buoc + 1, dk.kh.so_buoc);
  const char *ten_pha[] = {"khởi động", "xanh", "vàng", "đỏ toàn phần"};
  Serial.printf("Đang %s: ", dk.loi ? "LỖI" : ten_pha[dk.pha]);
  inToHop(dk.pha == PHA_VANG ? dk.vang : dk.xanh);
  int32_t con = dk.conLai(t_dieu_khien);
  if (con >= 0) Serial.printf(", còn %ld s", (long)((con + 999) / 1000));
  Serial.println();
  for (int h = 1; h <= 4; h++) {
    const SoDem &d = dk.dem[h];
    Serial.printf("  Hướng %d: ", h);
    if (!d.co) Serial.print("chưa có tin");
    else Serial.printf("làn ngoài %u, làn trong %u, emg %u%s%s", d.s, d.l, d.emg, d.giu ? " (gõ tay)" : "",
                       dk.conHan(h, t_dieu_khien) ? "" : " - MẤT TÍN HIỆU");
    uint32_t tong = goi[h].so_goi + goi[h].so_mat;
    if (tong) Serial.printf(" | ESP-NOW: nhận %lu gói, mất %lu (%.1f %%)", (unsigned long)goi[h].so_goi,
                            (unsigned long)goi[h].so_mat, 100.0 * goi[h].so_mat / tong);
    Serial.println();
  }
}

void inHuongDan() {
  Serial.println();
  Serial.println("=== Trung tâm đèn giao thông thông minh - NCKHKT 2026 ===");
  Serial.println("Gõ vào ô gửi của Serial Monitor (115200, kết thúc dòng bằng Newline):");
  Serial.println("  {\"id\":1,\"s\":3,\"l\":1,\"emg\":0}   giả làm camera Hướng 1: 3 xe làn ngoài, 1 xe làn trong");
  Serial.println("  codinh | thichung              đổi chế độ, có hiệu lực từ chu kì sau");
  Serial.println("  tua 4                          chạy nhanh gấp 4 (1 đến 8)");
  Serial.println("  xem                            in trạng thái và tỉ lệ mất gói");
  Serial.println("  xoa                            bỏ các số đếm đã gõ tay");
  Serial.println("  thuday                         lần lượt sáng từng bóng để kiểm tra dây");
  Serial.println();
}

// ---------- Nhận tin ----------
void nhanBanTin(const char *chuoi, bool tu_serial) {
  BanTin b;
  if (!giaiMaBanTin(chuoi, b)) {
    if (tu_serial) Serial.println("Không hiểu bản tin. Ví dụ đúng: {\"id\":1,\"s\":3,\"l\":1,\"emg\":0}");
    return;
  }
  dk.nhanDem(b.id, b.s, b.l, b.emg, t_dieu_khien, tu_serial);
  if (tu_serial) {
    Serial.printf("Đã nhận Hướng %d: làn ngoài %d, làn trong %d, emg %d. Dùng cho chu kì sau.\n", b.id, b.s, b.l, b.emg);
    return;
  }
  ThongKeGoi &g = goi[b.id];
  if (b.seq >= 0) {
    if (g.seq_cuoi >= 0 && b.seq > g.seq_cuoi + 1 && b.seq - g.seq_cuoi < 1000) g.so_mat += (uint32_t)(b.seq - g.seq_cuoi - 1);
    g.seq_cuoi = b.seq;       // seq nhỏ hơn trước nghĩa là camera vừa khởi động lại
  }
  g.so_goi++;
}

void xuLyLenh(char *dong) {
  while (*dong == ' ') dong++;
  if (*dong == 0) return;
  if (*dong == '{') { nhanBanTin(dong, true); return; }
  if (!strcmp(dong, "codinh")) { ts.che_do = CHE_DO_CO_DINH; Serial.println("Chế độ cố định, từ chu kì sau."); }
  else if (!strcmp(dong, "thichung")) { ts.che_do = CHE_DO_THICH_UNG; Serial.println("Chế độ thích ứng, từ chu kì sau."); }
  else if (!strncmp(dong, "tua", 3)) {
    int n = atoi(dong + 3);
    if (n < 1) n = 1;
    if (n > 8) n = 8;
    he_so_tua = (uint32_t)n;
    ts.mat_tin_hieu = MAT_TIN_HIEU * he_so_tua;      // vẫn là 3 giây thật
    Serial.printf("Tua nhanh x%d.\n", n);
  }
  else if (!strcmp(dong, "xem")) inTrangThai();
  else if (!strcmp(dong, "xoa")) {
    for (int h = 1; h <= 4; h++) if (dk.dem[h].giu) { dk.dem[h].co = false; dk.dem[h].giu = false; dk.dem[h].emg = 0; }
    Serial.println("Đã bỏ các số đếm gõ tay.");
  }
  else if (!strcmp(dong, "thuday")) {
    Serial.println("Thử dây: mỗi bóng sáng 0,4 s theo thứ tự đỏ, vàng, xanh của H1 thẳng, H1 trái, H2 thẳng...");
    denThuDay(400);
    dk.khoiDong(t_dieu_khien);
    Serial.println("Xong. Chạy lại từ đầu.");
  }
  else inHuongDan();
}

void docSerial() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') { dong_serial[dai_dong] = 0; xuLyLenh(dong_serial); dai_dong = 0; }
    else if (dai_dong < sizeof(dong_serial) - 1) dong_serial[dai_dong++] = c;
  }
}

// ---------- LED báo trên board ----------
void ledBao() {
#if DUNG_LED_BAO
  static uint8_t mau_truoc = 255;
  uint8_t mau;                       // 0 xanh lá: đủ 4 camera; 1 vàng: thiếu camera; 2 xanh dương: đang ưu tiên; 3 đỏ: lỗi
  if (dk.loi) mau = 3;
  else if (dk.ut_huong) mau = 2;
  else {
    mau = 0;
    for (int h = 1; h <= 4; h++) if (!dk.conHan(h, t_dieu_khien)) mau = 1;
  }
  if (mau == mau_truoc) return;
  mau_truoc = mau;
  static const uint8_t bang[4][3] = {{0, 20, 0}, {20, 14, 0}, {0, 0, 24}, {30, 0, 0}};
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  rgbLedWrite(CHAN_LED_BAO, bang[mau][0], bang[mau][1], bang[mau][2]);
#else
  neopixelWrite(CHAN_LED_BAO, bang[mau][0], bang[mau][1], bang[mau][2]);
#endif
#endif
}

void setup() {
  denKhoiTao();                      // việc đầu tiên: đưa tất cả về đỏ
  Serial.begin(TOC_DO_SERIAL);
  for (int h = 0; h <= 4; h++) { goi[h].seq_cuoi = -1; goi[h].so_goi = 0; goi[h].so_mat = 0; }
  ts.mat_tin_hieu = MAT_TIN_HIEU * he_so_tua;
  bool co_espnow = nhanTinKhoiTao();
  inHuongDan();
  Serial.printf("ESP-NOW kênh %d: %s. Địa chỉ MAC: %s\n", KENH_WIFI, co_espnow ? "sẵn sàng" : "KHÔNG khởi tạo được",
                WiFi.macAddress().c_str());
  dk.khiSuKien = khiSuKien;
  t_millis_truoc = millis();
  dk.khoiDong(t_dieu_khien);
}

void loop() {
  uint32_t bay_gio = millis();
  t_dieu_khien += (bay_gio - t_millis_truoc) * he_so_tua;
  t_millis_truoc = bay_gio;

  GoiTin g;
  while (nhanTinLay(g)) nhanBanTin(g.noi_dung, false);
  docSerial();

  dk.capNhat(t_dieu_khien);

  uint8_t mau[8];
  for (int i = 0; i < 8; i++) mau[i] = dk.mauNhom(i, t_dieu_khien);
  if (!denXuat(mau) && !dk.loi) dk.baoLoi();   // lớp kiểm tra cuối cùng từ chối: chuyển sang nháy vàng

  ledBao();
}
