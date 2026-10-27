// xe_ao.h — Giả lập ngay trên board trung tâm, để trình diễn và thử khi chưa có (hoặc thiếu) camera:
//   1. Xe ưu tiên ảo: bấm nút trên trang web (hướng và loại ngẫu nhiên, hoặc chọn tay) là bộ điều khiển
//      "thấy" một xe ưu tiên ở hướng đó, y như camera báo. Khi hướng đó đã được mở xanh ưu tiên đủ
//      XE_AO_QUA_MS thì coi như xe đã qua vạch: xe biến mất và đèn chuyển vàng như với xe thật.
//      Có thể bật tự thả: cứ vài phút thả một xe ngẫu nhiên.
//   2. Dòng xe giả: hướng nào không có camera gửi tin (và không gõ tay) thì có xe tới ngẫu nhiên ở hai làn,
//      xe rời đi khi đèn của làn đó xanh. Camera của hướng nào gửi tin trở lại thì hướng đó dùng số thật.
// Không đụng tới bộ luật hay máy trạng thái đèn: mọi thứ đi vào bằng đúng hàm nhanDem() như bản tin camera.
// Không dùng hàm Arduino nên thử được trên máy tính (kiem_thu/thu_mang.cpp).
#pragma once
#include "dieu_khien.h"

#define XE_AO_QUA_MS 3000         // xe ưu tiên ảo qua vạch sau khi được xanh ưu tiên chừng này
#define XE_AO_TOI_DA_MS 90000     // chưa được mở đèn sau chừng này thì bỏ
#define DONG_GIA_NHIP_MS 2000     // dòng xe giả: cứ chừng này xét xe tới, xe đi một lần
#define DONG_GIA_TOI_DA 4         // mỗi làn chờ tối đa bấy nhiêu xe (sa bàn chứa 4 xe Hot Wheels mỗi làn)

struct XeAo {
  uint8_t ut;                     // 0: không có; 1 cứu hỏa, 2 quân sự, 3 công an, 4 cứu thương
  uint8_t lan;                    // 1 làn ngoài, 2 làn trong
  uint32_t t_toi;                 // lúc thả (thời gian điều khiển)
};

class GiaLap {
public:
  XeAo xe[5];
  uint8_t ut_that[5];             // loại xe ưu tiên mà camera thật đang báo (0: không có)
  bool dong_xe = false;           // bật dòng xe giả
  uint8_t mat_do = 35;            // dòng xe giả: % khả năng mỗi làn có thêm 1 xe mỗi nhịp
  uint32_t tu_tha_ms = 0;         // tự thả xe ưu tiên ngẫu nhiên mỗi bấy nhiêu ms (0: tắt)
  uint8_t so_s[5], so_l[5];       // số xe của dòng xe giả
  bool dang_gia[5];               // hướng đang dùng dòng xe giả
  uint32_t so_xe_ao_qua = 0;      // số xe ưu tiên ảo đã qua
  uint32_t (*ngau_nhien)() = nullptr;

  GiaLap() { datLai(0); }

  void datLai(uint32_t t) {
    for (int h = 0; h <= 4; h++) { xe[h].ut = 0; xe[h].lan = 0; xe[h].t_toi = 0; ut_that[h] = 0; so_s[h] = 0; so_l[h] = 0; dang_gia[h] = false; }
    t_nhip_ = t; t_tu_tha_ = t; t_nuoi_ = t;
  }

  // Thả một xe ưu tiên ảo. h = 0: hướng ngẫu nhiên trong các hướng chưa có xe ảo; ut = 0: loại ngẫu nhiên;
  // lan = 0: làn ngẫu nhiên. Trả về hướng đã thả, 0 nếu không thả được (hướng đó đã có xe ảo).
  int tha(int h, int ut, int lan, uint32_t t) {
    if (h == 0) {
      int trong[4], n = 0;
      for (int k = 1; k <= 4; k++) if (!xe[k].ut) trong[n++] = k;
      if (n == 0) return 0;
      h = trong[soNgauNhien(n)];
    }
    if (h < 1 || h > 4 || xe[h].ut) return 0;
    if (ut < 1 || ut > 4) ut = 1 + soNgauNhien(4);
    if (lan != 1 && lan != 2) lan = 1 + soNgauNhien(2);
    xe[h].ut = (uint8_t)ut; xe[h].lan = (uint8_t)lan; xe[h].t_toi = t;
    return h;
  }

  void bo(int h) { if (h >= 1 && h <= 4) xe[h].ut = 0; }

  // Gọi với mỗi bản tin của camera hướng h, ngay trước khi đưa vào bộ điều khiển: camera không thấy xe ưu
  // tiên nào mà hướng đó có xe ưu tiên ảo thì thêm xe ảo vào bản tin (để hai nguồn không giành nhau).
  void chenVaoBanTin(int h, int &emg, int &ut) {
    if (h < 1 || h > 4) return;
    int ut_cam = ut ? ut : (emg ? 4 : 0);
    ut_that[h] = (uint8_t)ut_cam;
    if (xe[h].ut && !ut_cam) { emg = xe[h].lan; ut = xe[h].ut; }
  }

  // Gọi đều trong vòng lặp. cam_song[h]: camera hướng h vừa gửi tin trong vài giây gần đây.
  void chay(DieuKhien &dk, uint32_t t, const bool cam_song[5]) {
    // xe ưu tiên ảo: qua vạch hoặc hết hạn
    for (int h = 1; h <= 4; h++) {
      XeAo &x = xe[h];
      if (!x.ut) continue;
      bool dang_mo = dk.ut_huong == h && dk.pha == PHA_XANH && !dk.loi;
      if ((dang_mo && t - dk.t_xanh_tu >= XE_AO_QUA_MS) || t - x.t_toi >= XE_AO_TOI_DA_MS) {
        if (dang_mo) so_xe_ao_qua++;
        x.ut = 0;
        const SoDem &d = dk.dem[h];
        if (!ut_that[h] && d.co) dk.nhanDem(h, d.s, d.l, 0, t, d.giu, 0);   // báo ngay là xe đã đi
      }
    }
    // tự thả
    if (tu_tha_ms && t - t_tu_tha_ >= tu_tha_ms) { t_tu_tha_ = t; tha(0, 0, 0, t); }
    else if (!tu_tha_ms) t_tu_tha_ = t;
    // dòng xe giả: xe tới, xe đi
    bool nhip = t - t_nhip_ >= DONG_GIA_NHIP_MS;
    if (nhip) t_nhip_ = t;
    for (int h = 1; h <= 4; h++) {
      bool gia = dong_xe && !cam_song[h] && !dk.dem[h].giu;
      if (gia && !dang_gia[h]) { so_s[h] = 0; so_l[h] = 0; }
      dang_gia[h] = gia;
      if (!gia || !nhip) continue;
      int g = (h - 1) * 2;
      bool xanh_thang = dk.mauNhom(g, t) == MAU_XANH, xanh_trai = dk.mauNhom(g + 1, t) == MAU_XANH;
      if (xanh_thang && so_s[h]) so_s[h]--;                    // làn ngoài chỉ đi thẳng
      if ((xanh_thang || xanh_trai) && so_l[h]) so_l[h]--;     // làn trong đi thẳng hoặc rẽ trái
      if (so_s[h] < DONG_GIA_TOI_DA && (int)soNgauNhien(100) < mat_do) so_s[h]++;
      if (so_l[h] < DONG_GIA_TOI_DA && (int)soNgauNhien(100) < mat_do * 2 / 3) so_l[h]++;
    }
    // nuôi bộ điều khiển 4 lần mỗi giây (thời gian điều khiển) cho hướng nào đang có xe ảo hoặc dòng xe giả
    if (t - t_nuoi_ < 250) return;
    t_nuoi_ = t;
    for (int h = 1; h <= 4; h++) {
      const SoDem &d = dk.dem[h];
      bool co_xe_ao = xe[h].ut && !ut_that[h];
      if (dang_gia[h]) {
        dk.nhanDem(h, so_s[h], so_l[h], co_xe_ao ? xe[h].lan : 0, t, false, co_xe_ao ? xe[h].ut : 0);
      } else if (co_xe_ao && !cam_song[h]) {
        // không có camera: giữ số xe đang có (gõ tay hoặc 0) và thêm xe ảo
        dk.nhanDem(h, d.co ? d.s : 0, d.co ? d.l : 0, xe[h].lan, t, d.giu, xe[h].ut);
      }
    }
  }

private:
  uint32_t t_nhip_ = 0, t_tu_tha_ = 0, t_nuoi_ = 0;
  uint32_t soNgauNhien(uint32_t n) {
    if (n == 0) return 0;
    if (ngau_nhien) return ngau_nhien() % n;
    hat_ = hat_ * 1103515245u + 12345u;
    return (hat_ >> 8) % n;
  }
  uint32_t hat_ = 12345;
};
