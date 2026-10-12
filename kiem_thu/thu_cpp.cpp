// thu_cpp.cpp — Chạy bản C++ của bộ quyết định trên máy tính.
// Đọc lệnh từ stdin, in kết quả ra stdout; so_khop.js so kết quả này với bản JavaScript.
//   TS <18 số>                 đặt tham số
//   KH s1 l1 s2 l2 s3 l3 s4 l4 h1 h2 h3 h4 k_doc k_ngang truc   lập kế hoạch, in ra
//   BD t                       khởi động bộ điều khiển
//   D id s l emg t giu ut      nhận một bản tin đếm xe
//   T t                        cập nhật tới thời điểm t, in trạng thái
//   LM n                       (firmware camera) bộ lọc mới qua n khung
//   LK s l emg ut              thêm một khung vào bộ lọc, in kết quả đã lọc
//   PL <16 số góc 2 tứ giác> n rồi n bộ (x y loai)   gán xe vào làn, in kết quả
#include <cstdio>
#include <cstring>
#include "../firmware/trung_tam/dieu_khien.h"
#include "../firmware/camera/lan_duong.h"

int main() {
  ThamSo ts;
  memset(&ts, 0, sizeof(ts));
  DieuKhien dk(&ts);
  BoLoc loc(5);
  char lenh[8];
  while (scanf("%7s", lenh) == 1) {
    if (!strcmp(lenh, "TS")) {
      unsigned v[18];
      for (int i = 0; i < 18; i++) if (scanf("%u", &v[i]) != 1) return 1;
      ts.che_do = (uint8_t)v[0]; ts.t_min = v[1]; ts.t_max = v[2]; ts.cong_them = v[3];
      ts.xe_quy_doi = v[4]; ts.t_vang = v[5]; ts.t_do = v[6]; ts.cd_thang = v[7]; ts.cd_trai = v[8];
      ts.mat_tin_hieu = v[9]; ts.ut_xanh_an_toan = v[10]; ts.ut_giu = v[11]; ts.ut_toi_da = v[12]; ts.cat_som = v[13];
      ts.kieu_pha = v[14]; ts.hoc_cach_thu = v[15]; ts.hoc_so_xe = v[16]; ts.hoc_nguong = v[17];
    } else if (!strcmp(lenh, "KH")) {
      unsigned v[15];
      for (int i = 0; i < 15; i++) if (scanf("%u", &v[i]) != 1) return 1;
      uint8_t s[5] = {0}, l[5] = {0};
      bool hl[5] = {false};
      for (int h = 1; h <= 4; h++) { s[h] = (uint8_t)v[(h - 1) * 2]; l[h] = (uint8_t)v[(h - 1) * 2 + 1]; hl[h] = v[7 + h] != 0; }
      uint8_t kieu[2] = {(uint8_t)v[12], (uint8_t)v[13]};
      KeHoach kh = lapKeHoach(ts, s, l, hl, kieu, (uint8_t)v[14]);
      printf("KH %u", (unsigned)kh.so_buoc);
      for (int i = 0; i < kh.so_buoc; i++) printf(" %u:%u:%u", (unsigned)kh.buoc[i].mat_na, (unsigned)kh.buoc[i].xanh, (unsigned)kh.buoc[i].ngoai);
      printf("\n");
    } else if (!strcmp(lenh, "BD")) {
      unsigned t;
      if (scanf("%u", &t) != 1) return 1;
      dk = DieuKhien(&ts);   // bộ điều khiển mới cho mỗi kịch bản
      dk.khoiDong(t);
    } else if (!strcmp(lenh, "D")) {
      int id, s, l, emg, giu, ut;
      unsigned t;
      if (scanf("%d %d %d %d %u %d %d", &id, &s, &l, &emg, &t, &giu, &ut) != 7) return 1;
      dk.nhanDem(id, s, l, emg, t, giu != 0, ut);
    } else if (!strcmp(lenh, "LM")) {
      int n;
      if (scanf("%d", &n) != 1) return 1;
      loc = BoLoc(n);
    } else if (!strcmp(lenh, "LK")) {
      unsigned v[4];
      for (int i = 0; i < 4; i++) if (scanf("%u", &v[i]) != 1) return 1;
      KetQuaKhung k = {(uint8_t)v[0], (uint8_t)v[1], (uint8_t)v[2], (uint8_t)v[3]};
      loc.them(k);
      KetQuaKhung kq = loc.layKetQua();
      printf("L %u %u %u %u\n", (unsigned)kq.s, (unsigned)kq.l, (unsigned)kq.emg, (unsigned)kq.ut);
    } else if (!strcmp(lenh, "PL")) {
      CauHinhCam ch;
      ch.roi_x = 0; ch.roi_y = 0; ch.roi_w = 320; ch.roi_h = 240;
      int g[16], n;
      for (int i = 0; i < 16; i++) if (scanf("%d", &g[i]) != 1) return 1;
      for (int i = 0; i < 4; i++) {
        ch.lan_ngoai[i].x = (int16_t)g[i * 2]; ch.lan_ngoai[i].y = (int16_t)g[i * 2 + 1];
        ch.lan_trong[i].x = (int16_t)g[8 + i * 2]; ch.lan_trong[i].y = (int16_t)g[8 + i * 2 + 1];
      }
      if (scanf("%d", &n) != 1 || n < 0 || n > 32) return 1;
      VatThe vat[32];
      for (int i = 0; i < n; i++) {
        int x, y, loai;
        if (scanf("%d %d %d", &x, &y, &loai) != 3) return 1;
        vat[i].x = (float)x; vat[i].y = (float)y; vat[i].loai = (uint8_t)loai;
      }
      KetQuaKhung kq = phanLan(vat, n, ch);
      printf("P %u %u %u %u\n", (unsigned)kq.s, (unsigned)kq.l, (unsigned)kq.emg, (unsigned)kq.ut);
    } else if (!strcmp(lenh, "T")) {
      unsigned t;
      if (scanf("%u", &t) != 1) return 1;
      dk.capNhat(t);
      printf("T %u %u %u %u %d %u %u %d %d", t, (unsigned)dk.pha, (unsigned)dk.xanh, (unsigned)dk.vang,
             dk.i_buoc, (unsigned)dk.chu_ki, (unsigned)dk.ut_huong, dk.loi ? 1 : 0, (int)dk.conLai(t));
      for (int g = 0; g < 8; g++) printf(" %u", (unsigned)dk.mauNhom(g, t));
      // trạng thái của bộ tự học và trục tới lượt
      const Hoc &hc = dk.hoc;
      printf(" | %u", (unsigned)dk.truc_ke);
      for (int k = 0; k < 2; k++)
        printf(" %u %lld %lld %u %u %u %u %u", (unsigned)hc.kieu[k], (long long)hc.b[k], (long long)hc.q[k], (unsigned)hc.dem[k],
               (unsigned)hc.cach[k], (unsigned)hc.lien[k], (unsigned)hc.tuoi[k], (unsigned)hc.ra[k]);
      printf(" %d %u %d %d %d %d %llu %u\n", (int)hc.l_truc, (unsigned)hc.l_kieu, hc.l_do ? 1 : 0, hc.l_thu ? 1 : 0, hc.l_hong ? 1 : 0,
             hc.l_qt ? 1 : 0, (unsigned long long)hc.l_g, (unsigned)hc.l_n);
    }
  }
  return 0;
}
