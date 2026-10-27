// trung_tam.ino — Board trung tâm ESP32-S3 N16R8 của sa bàn đèn giao thông thông minh.
// Việc của board: nhận số xe từng làn do 4 camera gửi về (ESP-NOW) hoặc gõ tay (Serial),
// lập kế hoạch đèn cho từng lượt và điều khiển 24 bóng đèn của 8 cụm.
//
//   quyet_dinh.h  bộ luật lập kế hoạch (sửa luật ở đây, nhớ sửa cả web/quyet_dinh.js)
//   dieu_khien.h  máy trạng thái xanh → vàng → đỏ toàn phần, xe ưu tiên, tự học kiểu chia pha
//   den.h         xuất chân đèn, có kiểm tra xung đột
//   nhan_tin.h    nhận ESP-NOW, giải mã JSON, gửi lệnh xuống camera
//   web.h         phát WiFi "TRUNG-TAM-SaBan", trang điều khiển http://192.168.4.1
//   nap_tu_xa.h   nạp firmware trung tâm và camera qua WiFi, tự quay về bản cũ nếu bản mới không chạy ổn
//   xe_ao.h       xe ưu tiên ảo, dòng xe giả (trình diễn, thử khi thiếu camera)
//   thong_so.h    chỉnh và lưu thông số đèn       nhat_ky.h  nhật ký sự kiện cho trang web
//   cau_hinh.h    chân, kênh WiFi, thời gian      cau_hinh_mang.h  tên và mật khẩu WiFi, bật tắt trang web
//
// Thư viện cần cài: ArduinoJson (Library Manager). Cách nạp và cách thử: xem README.md.
// Không dùng delay() trong loop(); mọi thứ chạy theo millis().
#include "cau_hinh.h"
#include "dieu_khien.h"
#include "den.h"
#include "nhan_tin.h"
#include "thong_so.h"
#include "xe_ao.h"
#include "nhat_ky.h"
#if DUNG_WEB
#include "web.h"
#endif

#define TS_MAC_DINH { \
  CHE_DO_BAN_DAU, T_MIN, T_MAX, CONG_THEM, XE_QUY_DOI, T_VANG, T_DO_TOAN_PHAN, \
  CD_THANG, CD_TRAI, MAT_TIN_HIEU, UT_XANH_AN_TOAN, UT_GIU, UT_TOI_DA, CAT_SOM, \
  KIEU_PHA, HOC_CACH_THU, HOC_SO_XE, HOC_NGUONG }
ThamSo ts = TS_MAC_DINH;
DieuKhien dk(&ts);
GiaLap gl;                        // xe ưu tiên ảo, dòng xe giả
uint32_t t_tin_cam[5];            // millis() lúc nhận bản tin đếm gần nhất của từng camera
bool co_espnow = false;

uint32_t he_so_tua = HE_SO_TUA_NHANH;
uint32_t t_dieu_khien = 0;        // đồng hồ của bộ điều khiển (ms); chạy nhanh hơn millis() khi tua
uint32_t t_millis_truoc = 0;

// thống kê gói tin của từng camera, để đo tỉ lệ mất gói
struct ThongKeGoi { long seq_cuoi; uint32_t so_goi, so_mat; };
ThongKeGoi goi[5];

char dong_serial[160];
uint8_t dai_dong = 0;

// ---------- In ra Serial ----------
const char *tenKieu(uint32_t k) {
  switch (k) {
    case KIEU_TU_HOC: return "tự học";
    case KIEU_THANG_TRUOC: return "thẳng trước";
    case KIEU_DOI_XUNG: return "đối xứng";
    default: return "riêng từng hướng";
  }
}

const char *tenXeUuTien(uint8_t ut) {
  switch (ut) {
    case UT_CUU_HOA: return "xe cứu hỏa";
    case UT_QUAN_SU: return "xe quân sự";
    case UT_CONG_AN: return "xe công an";
    case UT_CUU_THUONG: return "xe cứu thương";
    default: return "xe ưu tiên";
  }
}

const char *tenTruc(uint8_t truc) {
  return truc ? "trục ngang" : "trục dọc";
}

void inThoiGian(Print &ra = Serial) {
  ra.printf("[%7.1f s] ", t_dieu_khien / 1000.0);
}

void inToHop(uint8_t m, Print &ra = Serial) {
  bool dau = true;
  for (int h = 1; h <= 4; h++) {
    bool thang = m & bitThang(h), trai = m & bitTrai(h);
    if (!thang && !trai) continue;
    if (!dau) ra.print(" + ");
    dau = false;
    ra.printf("H%d %s", h, (thang && trai) ? "thẳng và trái" : (thang ? "thẳng" : "trái"));
  }
  if (dau) ra.print("đỏ toàn phần");
}

void inSoDem(Print &ra = Serial) {
  for (int h = 1; h <= 4; h++) {
    if (dk.conHan(h, t_dieu_khien)) ra.printf(" H%d %u/%u", h, dk.dem[h].s, dk.dem[h].l);
    else ra.printf(" H%d mất tín hiệu", h);
  }
}

void khiSuKien(uint8_t loai, uint8_t dl) {
  inThoiGian(nk);
  switch (loai) {
    case SK_CHU_KI:
      if (dk.che_do_kh == CHE_DO_CO_DINH) nk.printf("Chu kì %lu (cố định).", (unsigned long)dk.chu_ki);
      else nk.printf("Lượt %lu (thích ứng, %s).", (unsigned long)dk.chu_ki, tenTruc(trucCuaMatNa(dk.kh.buoc[0].mat_na)));
      nk.print(" Số xe làn ngoài/làn trong:");
      inSoDem(nk);
      nk.print(". Kế hoạch: ");
      for (int i = 0; i < dk.kh.so_buoc; i++) {
        if (i) nk.print(" -> ");
        inToHop(dk.kh.buoc[i].mat_na, nk);
        nk.printf(" %lu s", (unsigned long)(dk.kh.buoc[i].xanh / 1000));
      }
      nk.println();
      break;
    case SK_BUOC:
      nk.print("Xanh: ");
      inToHop(dk.xanh, nk);
      if (dk.ut_huong) nk.printf(" (ưu tiên %s, xe qua là chuyển vàng)\n", tenXeUuTien(dk.dem[dk.ut_huong].ut));
      else nk.printf(" trong %lu s\n", (unsigned long)(dk.dich_xanh / 1000));
      break;
    case SK_UU_TIEN:
      nk.printf("%s Hướng %u thấy %s: kết thúc bước đang chạy rồi mở riêng Hướng %u\n", gl.xe[dl].ut ? "Xe ảo:" : "Camera", dl,
                tenXeUuTien(dk.dem[dl].ut), dl);
      break;
    case SK_HET_UU_TIEN:
      nk.printf("Hết ưu tiên Hướng %u: chuyển vàng ngay, trả đèn cho hướng đang chờ\n", dl);
      break;
    case SK_CAT_SOM:
      nk.print("Hết xe ở ");
      inToHop(dl, nk);
      nk.printf(" quá %lu s: cắt xanh sớm\n", (unsigned long)(ts.cat_som / 1000));
      break;
    case SK_HOC_THU:
      nk.printf("Tự học: lượt này %s chạy thử kiểu \"%s\"\n", tenTruc(dl / 4), tenKieu(dl % 4));
      break;
    case SK_HOC_NHAN:
      nk.printf("Tự học: %s đổi sang kiểu \"%s\" (thẳng trước đang cần %ld %% thời gian đèn của kiểu riêng)\n", tenTruc(dl / 4),
                    tenKieu(dl % 4), (long)dk.hoc.q[dl / 4]);
      break;
    case SK_HOC_TRA:
      nk.printf("Tự học: lượt thử \"thẳng trước\" ở %s cần %ld %% thời gian đèn của kiểu riêng, vẫn giữ \"riêng\"\n", tenTruc(dl / 4),
                    (long)dk.hoc.q[dl / 4]);
      break;
    case SK_HOC_HUY:
      nk.printf("Tự học: bỏ lượt thử ở %s vì có xe ưu tiên hoặc thiếu camera\n", tenTruc(dl));
      break;
    case SK_LOI:
      nk.println("LỖI AN TOÀN: tổ hợp đèn xung đột. Tất cả nháy vàng. Bấm RST để chạy lại.");
      break;
  }
}

void inTrangThai() {
  inThoiGian();
  Serial.printf("Chế độ %s, tua x%lu, cắt sớm %lu s, %s %lu, bước %d/%u. ", ts.che_do == CHE_DO_CO_DINH ? "cố định" : "thích ứng",
                (unsigned long)he_so_tua, (unsigned long)(ts.cat_som / 1000), dk.che_do_kh == CHE_DO_CO_DINH ? "chu kì" : "lượt",
                (unsigned long)dk.chu_ki, dk.i_buoc + 1, dk.kh.so_buoc);
  const char *ten_pha[] = {"khởi động", "xanh", "vàng", "đỏ toàn phần"};
  Serial.printf("Đang %s: ", dk.loi ? "LỖI" : ten_pha[dk.pha]);
  inToHop(dk.pha == PHA_VANG ? dk.vang : dk.xanh);
  int32_t con = dk.conLai(t_dieu_khien);
  if (con >= 0) Serial.printf(", còn %ld s", (long)((con + 999) / 1000));
  Serial.println();
  if (ts.che_do == CHE_DO_THICH_UNG) {
    Serial.printf("  Kiểu chia pha: %s", tenKieu(ts.kieu_pha));
    if (ts.kieu_pha == KIEU_TU_HOC) {
      Serial.printf(" (đang giữ: dọc %s, ngang %s", tenKieu(dk.hoc.kieu[0]), tenKieu(dk.hoc.kieu[1]));
      for (int k = 0; k < 2; k++)
        if (dk.hoc.q[k] >= 0) Serial.printf("; thẳng trước ở %s cần %ld %% thời gian đèn của kiểu riêng", tenTruc(k), (long)dk.hoc.q[k]);
      if (dk.hoc.b[0] >= 0) Serial.printf("; kiểu riêng: %.1f s xanh mỗi xe", dk.hoc.b[0] / 1000.0);
      Serial.print(")");
    }
    Serial.println();
  }
  for (int h = 1; h <= 4; h++) {
    const SoDem &d = dk.dem[h];
    Serial.printf("  Hướng %d: ", h);
    if (!d.co) Serial.print("chưa có tin");
    else {
      Serial.printf("làn ngoài %u, làn trong %u", d.s, d.l);
      if (d.ut) Serial.printf(", có %s ở làn %s", tenXeUuTien(d.ut), d.emg == 2 ? "trong" : "ngoài");
      Serial.printf("%s%s", d.giu ? " (gõ tay)" : "", dk.conHan(h, t_dieu_khien) ? "" : " - MẤT TÍN HIỆU");
    }
    uint32_t tong = goi[h].so_goi + goi[h].so_mat;
    if (tong) Serial.printf(" | ESP-NOW: nhận %lu gói, mất %lu (%.1f %%)", (unsigned long)goi[h].so_goi,
                            (unsigned long)goi[h].so_mat, 100.0 * goi[h].so_mat / tong);
    Serial.println();
  }
  for (int h = 1; h <= 4; h++) {
    const TrangThaiCam &c = tt_cam[h];
    if (!c.co) { Serial.printf("  CAM %d: chưa có tin trạng thái\n", h); continue; }
    Serial.printf("  CAM %d: chế độ %d, %.1f khung/s, %s, sóng %d dBm, vùng %s, tin cách đây %lu s\n", h, c.cd, c.kh,
                  c.ip[0] ? c.ip : "chưa nối WiFi", c.rs, c.vl ? "đã lưu" : "mặc định", (unsigned long)((millis() - c.t) / 1000));
  }
}

// Chuỗi JSON cho trang web (web.h). Gọi 2 lần mỗi giây.
void taoTrangThaiWeb(char *b, size_t co) {
  size_t n = 0;
#define THEM(...) do { if (n < co) n += snprintf(b + n, co - n, __VA_ARGS__); } while (0)
  int32_t con = dk.conLai(t_dieu_khien);
  THEM("{\"t\":%lu,\"cd\":%d,\"kieu\":%lu,\"tua\":%lu,\"cs\":%lu,\"pha\":%d,\"loi\":%d,\"con\":%ld,\"ck\":%lu,\"buoc\":%d,\"sb\":%u,"
       "\"uh\":%u,\"xanh\":%u,\"vang\":%u,\"den\":[",
       (unsigned long)(t_dieu_khien / 1000), dk.che_do_kh == CHE_DO_CO_DINH ? 0 : 1, (unsigned long)ts.kieu_pha,
       (unsigned long)he_so_tua, (unsigned long)(ts.cat_som / 1000), (int)dk.pha, dk.loi ? 1 : 0,
       (long)(con >= 0 ? (con + 999) / 1000 : -1), (unsigned long)dk.chu_ki, dk.i_buoc, dk.kh.so_buoc, dk.ut_huong, dk.xanh, dk.vang);
  for (int g = 0; g < 8; g++) THEM("%s%u", g ? "," : "", dk.mauNhom(g, t_dieu_khien));
  THEM("],\"h\":[");
  for (int h = 1; h <= 4; h++) {
    const SoDem &d = dk.dem[h];
    THEM("%s{\"co\":%d,\"s\":%u,\"l\":%u,\"emg\":%u,\"ut\":%u,\"song\":%d,\"tay\":%d,\"nhan\":%lu,\"mat\":%lu,\"ao\":%u,\"gia\":%d}",
         h > 1 ? "," : "", d.co ? 1 : 0, d.s, d.l, d.emg, d.ut, dk.conHan(h, t_dieu_khien) ? 1 : 0, d.giu ? 1 : 0,
         (unsigned long)goi[h].so_goi, (unsigned long)goi[h].so_mat, gl.xe[h].ut, gl.dang_gia[h] ? 1 : 0);
  }
  THEM("],\"c\":[");
  for (int h = 1; h <= 4; h++) {
    const TrangThaiCam &c = tt_cam[h];
    THEM("%s{\"co\":%d,\"ip\":\"%s\",\"cd\":%d,\"kh\":%.1f,\"rs\":%d,\"vl\":%d,\"lat\":%d,\"up\":%lu,\"cach\":%lu,"
         "\"fw\":\"%s\",\"ch\":%d,\"mh\":%d,\"nap\":%d,\"np\":%d,\"xn\":%d,\"rb\":%d}", h > 1 ? "," : "",
         c.co ? 1 : 0, c.ip, c.cd, c.kh, c.rs, c.vl, c.lat, (unsigned long)c.up, (unsigned long)(c.co ? (millis() - c.t) / 1000 : 0),
         c.fw, c.ch, c.mh, c.nap, c.np, c.xn, c.rb);
  }
  THEM("],\"tutha\":%lu,\"dong\":%d,\"md\":%u,\"aoqua\":%lu,\"ts\":[", (unsigned long)(gl.tu_tha_ms / 60000), gl.dong_xe ? 1 : 0,
       gl.mat_do, (unsigned long)gl.so_xe_ao_qua);
  for (int i = 0; i < SO_MUC_THAM_SO; i++) THEM("%s%lu", i ? "," : "", (unsigned long)(ts.*(MUC_THAM_SO[i].o)));
#if DUNG_WEB
  uint8_t kenh = 0;
  wifi_second_chan_t phu;
  esp_wifi_get_channel(&kenh, &phu);
  THEM("],\"wn\":%u,\"wcf\":%d,\"wip\":\"%s\",\"kenh\":%u,\"nap\":", wn_tt, TEN_WIFI_NGOAI[0] ? 1 : 0,
       wn_tt == 2 ? WiFi.localIP().toString().c_str() : "", kenh);
  if (n < co) n += napJson(b + n, co - n);
  THEM("}");
#else
  THEM("]}");
#endif
#undef THEM
}

void inHuongDan() {
  Serial.println();
  Serial.println("=== Trung tâm đèn giao thông thông minh - NCKHKT 2026 ===");
  Serial.println("Gõ vào ô gửi của Serial Monitor (115200, kết thúc dòng bằng Newline):");
  Serial.println("  {\"id\":1,\"s\":3,\"l\":1,\"emg\":0}   giả làm camera Hướng 1: 3 xe làn ngoài, 1 xe làn trong");
  Serial.println("  {\"id\":2,\"s\":1,\"l\":0,\"emg\":1,\"ut\":1}   Hướng 2 có xe ưu tiên ở làn ngoài (emg 1 ngoài, 2 trong);");
  Serial.println("                                 ut: 1 cứu hỏa, 2 quân sự, 3 công an, 4 cứu thương (số nhỏ đi trước)");
  Serial.println("  codinh | thichung              đổi chế độ, có hiệu lực từ lượt sau");
  Serial.println("  kieu rieng | thangtruoc | doixung | tuhoc   kiểu chia pha của chế độ thích ứng");
  Serial.println("  tua 4                          chạy nhanh gấp 4 (1 đến 8)");
  Serial.println("  catsom 5                       hết xe quá 5 s thì cắt xanh sớm (0 là tắt)");
  Serial.println("  xem                            in trạng thái và tỉ lệ mất gói");
  Serial.println("  xoa                            bỏ các số đếm đã gõ tay");
  Serial.println("  thuday                         lần lượt sáng từng bóng để kiểm tra dây");
  Serial.println("  uutien [hướng] [loại] [làn]    thả xe ưu tiên ảo (không ghi gì: ngẫu nhiên); boxeao: bỏ hết");
  Serial.println("  tutha 3                        cứ 3 phút tự thả một xe ưu tiên ảo (0 là tắt)");
  Serial.println("  donggia 1 35                   dòng xe giả cho hướng không có camera, mật độ 35 % (donggia 0 là tắt)");
  Serial.println("  dat vang 3.5                   đặt thông số (giây): tmin tmax cong quydoi vang dotp cdthang cdtrai catsom utgiu uttoida");
  Serial.println("  thongso | luuts | tsmacdinh    xem, lưu vào board, về mặc định các thông số");
#if DUNG_WEB
  Serial.println("  wifi                           nối lại hotspot máy tính (TEN_WIFI_NGOAI trong cau_hinh_mang.h)");
  Serial.println("  napcam 0 | napcam 2 | napdung  nạp firmware camera đang giữ cho mọi camera / CAM 2; hủy lịch");
  Serial.println("  quayve                         khởi động lại bằng bản firmware trước");
#endif
  Serial.println();
}

// ---------- Nhận tin ----------
void nhanBanTin(const char *chuoi, bool tu_serial) {
  if (!tu_serial && nhanTrangThaiCam(chuoi, millis())) return;   // tin trạng thái của camera, không phải số đếm
  BanTin b;
  if (!giaiMaBanTin(chuoi, b)) {
    if (tu_serial) Serial.println("Không hiểu bản tin. Ví dụ đúng: {\"id\":1,\"s\":3,\"l\":1,\"emg\":0}");
    return;
  }
  if (!tu_serial) { t_tin_cam[b.id] = millis(); gl.chenVaoBanTin(b.id, b.emg, b.ut); }
  dk.nhanDem(b.id, b.s, b.l, b.emg, t_dieu_khien, tu_serial, b.ut);
  if (tu_serial) {
    const SoDem &d = dk.dem[b.id];
    Serial.printf("Đã nhận Hướng %d: làn ngoài %u, làn trong %u", b.id, d.s, d.l);
    if (d.ut) Serial.printf(", có %s ở làn %s", tenXeUuTien(d.ut), d.emg == 2 ? "trong" : "ngoài");
    Serial.println(". Dùng cho lượt sau.");
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
  else if (!strcmp(dong, "thichung")) { ts.che_do = CHE_DO_THICH_UNG; Serial.println("Chế độ thích ứng, từ lượt sau."); }
  else if (!strncmp(dong, "kieu", 4)) {
    const char *k = dong + 4;
    while (*k == ' ') k++;
    if (!strcmp(k, "rieng")) ts.kieu_pha = KIEU_RIENG;
    else if (!strcmp(k, "thangtruoc")) ts.kieu_pha = KIEU_THANG_TRUOC;
    else if (!strcmp(k, "doixung")) ts.kieu_pha = KIEU_DOI_XUNG;
    else if (!strcmp(k, "tuhoc")) { ts.kieu_pha = KIEU_TU_HOC; dk.hocMoi(); }
    else { Serial.println("Gõ: kieu rieng, kieu thangtruoc, kieu doixung hoặc kieu tuhoc."); return; }
    Serial.printf("Kiểu chia pha: %s, từ lượt sau.", tenKieu(ts.kieu_pha));
    if (ts.kieu_pha == KIEU_TU_HOC)
      Serial.printf(" Bắt đầu từ kiểu riêng từng hướng; cứ %lu lượt của một trục thì thử \"thẳng trước\" một lượt.",
                    (unsigned long)ts.hoc_cach_thu);
    if (ts.che_do != CHE_DO_THICH_UNG) Serial.print(" (Chỉ có tác dụng ở chế độ thích ứng.)");
    Serial.println();
  }
  else if (!strncmp(dong, "tua", 3)) {
    int n = atoi(dong + 3);
    if (n < 1) n = 1;
    if (n > 8) n = 8;
    he_so_tua = (uint32_t)n;
    ts.mat_tin_hieu = MAT_TIN_HIEU * he_so_tua;      // vẫn là 3 giây thật
    Serial.printf("Tua nhanh x%d.\n", n);
  }
  else if (!strncmp(dong, "catsom", 6)) {
    int n = atoi(dong + 6);
    if (n < 0) n = 0;
    if (n > 60) n = 60;
    ts.cat_som = (uint32_t)n * 1000;
    if (n) Serial.printf("Cắt xanh sớm khi tổ hợp đang xanh hết xe quá %d s.\n", n);
    else Serial.println("Tắt cắt xanh sớm.");
  }
  else if (!strcmp(dong, "xem")) inTrangThai();
  else if (!strncmp(dong, "uutien", 6)) {
    int h = 0, loai = 0, lan = 0;
    sscanf(dong + 6, "%d %d %d", &h, &loai, &lan);
    int da = gl.tha(h, loai, lan, t_dieu_khien);
    inThoiGian(nk);
    if (da) nk.printf("Thả %s ảo vào Hướng %d, làn %s.\n", tenXeUuTien(gl.xe[da].ut), da, gl.xe[da].lan == 2 ? "trong" : "ngoài");
    else nk.println(h ? "Hướng đó đang có xe ưu tiên ảo chờ." : "Cả 4 hướng đang có xe ưu tiên ảo chờ.");
  }
  else if (!strcmp(dong, "boxeao")) { for (int h = 1; h <= 4; h++) gl.bo(h); inThoiGian(nk); nk.println("Đã bỏ mọi xe ưu tiên ảo."); }
  else if (!strncmp(dong, "tutha", 5)) {
    int p = atoi(dong + 5);
    if (p < 0) p = 0;
    if (p > 30) p = 30;
    gl.tu_tha_ms = (uint32_t)p * 60000;
    inThoiGian(nk);
    if (p) nk.printf("Cứ %d phút tự thả một xe ưu tiên ảo.\n", p); else nk.println("Tắt tự thả xe ưu tiên ảo.");
  }
  else if (!strncmp(dong, "donggia", 7)) {
    int bat = 0, md = gl.mat_do;
    sscanf(dong + 7, "%d %d", &bat, &md);
    gl.dong_xe = bat != 0;
    if (md >= 5 && md <= 95) gl.mat_do = (uint8_t)md;
    inThoiGian(nk);
    if (gl.dong_xe) nk.printf("Bật dòng xe giả (mật độ %u %%) cho hướng không có camera.\n", gl.mat_do); else nk.println("Tắt dòng xe giả.");
  }
  else if (!strncmp(dong, "dat ", 4)) {
    char khoa[16] = "", gia_tri[16] = "";
    sscanf(dong + 4, "%15s %15s", khoa, gia_tri);
    int k = datThamSo(ts, khoa, gia_tri);
    inThoiGian(nk);
    nk.printf("Thông số %s = %s: %s%s\n", khoa, gia_tri, loiThamSo(k), k == 0 ? " (từ bước sau; gõ luuts để lưu vào board)" : "");
  }
  else if (!strcmp(dong, "thongso")) {
    for (int i = 0; i < SO_MUC_THAM_SO; i++) {
      const MucThamSo &m = MUC_THAM_SO[i];
      uint32_t v = ts.*(m.o);
      if (m.la_ms) Serial.printf("  %-8s %6.1f s   %s\n", m.khoa, v / 1000.0, m.ten);
      else Serial.printf("  %-8s %6lu     %s\n", m.khoa, (unsigned long)v, m.ten);
    }
  }
  else if (!strcmp(dong, "luuts")) { inThoiGian(nk); nk.println(thamSoLuu(ts) ? "Đã lưu thông số, chế độ và kiểu chia pha vào board." : "KHÔNG lưu được thông số."); }
  else if (!strcmp(dong, "tsmacdinh")) {
    thamSoXoa();
    ThamSo md = TS_MAC_DINH;
    md.mat_tin_hieu = ts.mat_tin_hieu;
    ts = md;
    inThoiGian(nk);
    nk.println("Về thông số mặc định trong cau_hinh.h (đã xóa bản lưu trong board).");
  }
#if DUNG_WEB
  else if (!strncmp(dong, "napcam", 6)) {
    String ly_do;
    int so = napXepLich(atoi(dong + 6), ly_do);
    inThoiGian(nk);
    if (so > 0) nk.printf("Xếp lịch nạp firmware camera bản %s cho %d camera.%s%s\n", anh_cam.pb, so, ly_do.length() ? " Bỏ qua: " : "", ly_do.c_str());
    else nk.printf("Không nạp được: %s\n", ly_do.c_str());
  }
  else if (!strcmp(dong, "napdung")) { napHuyLich(); inThoiGian(nk); nk.println("Đã hủy các camera chưa tới lượt nạp."); }
  else if (!strcmp(dong, "quayve")) {
    const char *ly_do = napQuayVe();
    if (ly_do) Serial.println(ly_do);
    else { Serial.println("Khởi động lại bằng bản trước..."); delay(300); ESP.restart(); }
  }
#endif
#if DUNG_WEB
  else if (!strcmp(dong, "wifi")) wifiNgoaiThu();
#endif
  else if (!strcmp(dong, "xoa")) {
    for (int h = 1; h <= 4; h++) if (dk.dem[h].giu) { dk.dem[h].co = false; dk.dem[h].giu = false; dk.dem[h].emg = 0; dk.dem[h].ut = 0; }
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

#if DUNG_WEB
void baoNap(int h, const char *cau) {
  inThoiGian(nk);
  nk.printf("Nạp CAM %d: %s\n", h, cau);
}
#endif

void setup() {
  denKhoiTao();                      // việc đầu tiên: đưa tất cả về đỏ
  Serial.begin(TOC_DO_SERIAL);
  for (int h = 0; h <= 4; h++) { goi[h].seq_cuoi = -1; goi[h].so_goi = 0; goi[h].so_mat = 0; t_tin_cam[h] = 0; }
#if DUNG_WEB
  napKhoiDong();
#endif
  bool co_luu = thamSoNap(ts);
  ts.mat_tin_hieu = MAT_TIN_HIEU * he_so_tua;
  gl.ngau_nhien = esp_random;
  co_espnow = nhanTinKhoiTao();
  inHuongDan();
  if (co_luu) Serial.println("Dùng thông số đèn đã lưu trong board (gõ thongso để xem, tsmacdinh để về mặc định).");
#if DUNG_WEB
  Serial.printf("Firmware bản %s.%s%s\n", phienBanFw(), nap_cho_xn ? " BẢN MỚI NẠP QUA WIFI: tự kiểm tra trong 2 phút, không ổn thì quay về bản cũ." : "",
                nap_co_ban_loi ? " (Một bản nạp gần đây bị lỗi, board đã quay về bản này.)" : "");
#endif
  Serial.printf("ESP-NOW kênh %d: %s. Địa chỉ MAC: %s\n", KENH_WIFI, co_espnow ? "sẵn sàng" : "KHÔNG khởi tạo được",
                WiFi.macAddress().c_str());
#if DUNG_WEB
  webKhoiTao();
  Serial.printf("WiFi \"%s\", mật khẩu \"%s\". Nối điện thoại hoặc máy tính vào rồi mở http://%s\n", TEN_WIFI_TRUNG_TAM,
                MAT_KHAU_TRUNG_TAM, WiFi.softAPIP().toString().c_str());
#endif
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
#if DUNG_WEB
  char dong_web[160];
  while (webLayLenh(dong_web, sizeof(dong_web))) xuLyLenh(dong_web);
#endif

  bool cam_song[5] = {false, false, false, false, false};
  for (int h = 1; h <= 4; h++) cam_song[h] = t_tin_cam[h] && bay_gio - t_tin_cam[h] < 3000;
  gl.chay(dk, t_dieu_khien, cam_song);

  dk.capNhat(t_dieu_khien);

  uint8_t mau[8];
  for (int i = 0; i < 8; i++) mau[i] = dk.mauNhom(i, t_dieu_khien);
  if (!denXuat(mau) && !dk.loi) dk.baoLoi();   // lớp kiểm tra cuối cùng từ chối: chuyển sang nháy vàng

  ledBao();
#if DUNG_WEB
  wifiNgoaiChay();
  napLichChay(baoNap);
  {
    bool thay_cam = nap_so_cam_truoc == 0;
    for (int h = 1; h <= 4; h++) if (tt_cam[h].co) thay_cam = true;
    bool khoe = co_espnow && !dk.loi && WiFi.softAPIP() != IPAddress(0, 0, 0, 0) && thay_cam;
    if (napXacNhanChay(khoe)) { inThoiGian(nk); nk.printf("Bản firmware mới %s chạy ổn: đã xác nhận, không quay về bản cũ nữa.\n", phienBanFw()); }
  }
  static uint32_t t_web = 0;
  if (bay_gio - t_web >= 500) {
    t_web = bay_gio;
    static char b[sizeof(trang_thai_web)];
    taoTrangThaiWeb(b, sizeof(b));
    webDatTrangThai(b);
  }
#endif
}
