// thong_so.h — Chỉnh thông số đèn lúc đang chạy (lệnh "dat ..." ở Serial hoặc trang web) và lưu vào board.
// Mỗi thông số có giới hạn an toàn: vàng không dưới 3 s, đỏ toàn phần không dưới 1 s, xanh ngắn nhất không
// dưới 5 s... Đổi thông số có tác dụng từ bước hoặc lượt sau; bước đang chạy giữ nguyên.
// Phần kiểm tra giới hạn không dùng hàm Arduino nên thử được trên máy tính (kiem_thu/thu_mang.cpp).
#pragma once
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "quyet_dinh.h"

struct MucThamSo {
  const char *khoa;          // tên gõ trong lệnh "dat <khoa> <số giây>"
  const char *ten;           // tên hiện trên trang web
  uint32_t ThamSo::*o;       // ô trong ThamSo (mili giây, trừ xe_quy_doi)
  uint32_t nho_nhat, lon_nhat;
  bool la_ms;                // true: gõ bằng giây, lưu bằng mili giây
};

static const MucThamSo MUC_THAM_SO[] = {
  {"tmin", "Xanh ngắn nhất", &ThamSo::t_min, 5000, 60000, true},
  {"tmax", "Xanh dài nhất", &ThamSo::t_max, 10000, 180000, true},
  {"cong", "Cộng thêm mỗi 10 xe thật", &ThamSo::cong_them, 0, 30000, true},
  {"quydoi", "1 xe mô hình bằng bao nhiêu xe thật", &ThamSo::xe_quy_doi, 1, 20, false},
  {"vang", "Vàng", &ThamSo::t_vang, 3000, 6000, true},
  {"dotp", "Đỏ toàn phần", &ThamSo::t_do, 1000, 5000, true},
  {"cdthang", "Cố định: xanh đi thẳng", &ThamSo::cd_thang, 10000, 120000, true},
  {"cdtrai", "Cố định: xanh rẽ trái", &ThamSo::cd_trai, 5000, 60000, true},
  {"catsom", "Cắt xanh sớm sau (0: tắt)", &ThamSo::cat_som, 0, 60000, true},
  {"utgiu", "Xe ưu tiên qua rồi, chờ trước khi vàng", &ThamSo::ut_giu, 500, 10000, true},
  {"uttoida", "Một lần ưu tiên dài tối đa", &ThamSo::ut_toi_da, 20000, 120000, true},
};
static const int SO_MUC_THAM_SO = sizeof(MUC_THAM_SO) / sizeof(MUC_THAM_SO[0]);

// Đọc số giây (có thể lẻ một chữ số thập phân, dấu chấm hoặc phẩy) thành mili giây. -1 nếu sai.
inline long docGiay(const char *s) {
  while (*s == ' ') s++;
  if (!(*s >= '0' && *s <= '9')) return -1;
  long nguyen = 0, le = 0;
  while (*s >= '0' && *s <= '9') { nguyen = nguyen * 10 + (*s - '0'); if (nguyen > 100000) return -1; s++; }
  if (*s == '.' || *s == ',') {
    s++;
    if (*s >= '0' && *s <= '9') { le = (*s - '0') * 100; s++; }
    while (*s >= '0' && *s <= '9') s++;
  }
  while (*s == ' ') s++;
  if (*s) return -1;
  return nguyen * 1000 + le;
}

// Đặt một thông số. Trả về 0 nếu được; 1 sai tên; 2 sai số; 3 ngoài giới hạn; 4 xanh dài nhất nhỏ hơn ngắn nhất.
inline int datThamSo(ThamSo &ts, const char *khoa, const char *gia_tri) {
  for (int i = 0; i < SO_MUC_THAM_SO; i++) {
    const MucThamSo &m = MUC_THAM_SO[i];
    if (strcmp(khoa, m.khoa) != 0) continue;
    long v;
    if (m.la_ms) v = docGiay(gia_tri);
    else { v = docGiay(gia_tri); v = v < 0 || v % 1000 ? -1 : v / 1000; }
    if (v < 0) return 2;
    if ((uint32_t)v < m.nho_nhat || (uint32_t)v > m.lon_nhat) return 3;
    uint32_t cu = ts.*(m.o);
    ts.*(m.o) = (uint32_t)v;
    if (ts.t_max < ts.t_min) { ts.*(m.o) = cu; return 4; }
    return 0;
  }
  return 1;
}

inline const char *loiThamSo(int k) {
  switch (k) {
    case 0: return "đã đặt";
    case 1: return "không có thông số này";
    case 2: return "số không hợp lệ";
    case 3: return "ngoài giới hạn an toàn";
    default: return "xanh dài nhất phải không nhỏ hơn xanh ngắn nhất";
  }
}

#ifdef ARDUINO
#include <Preferences.h>
#define TS_PHIEN_BAN 1

// Lưu các thông số chỉnh được, chế độ và kiểu chia pha vào bộ nhớ của board.
inline bool thamSoLuu(const ThamSo &ts) {
  Preferences p;
  if (!p.begin("thong_so", false)) return false;
  bool duoc = p.putUChar("pb", TS_PHIEN_BAN) == 1;
  for (int i = 0; i < SO_MUC_THAM_SO; i++) duoc = duoc && p.putULong(MUC_THAM_SO[i].khoa, ts.*(MUC_THAM_SO[i].o)) == 4;
  duoc = duoc && p.putUChar("chedo", ts.che_do) == 1 && p.putULong("kieu", ts.kieu_pha) == 4;
  p.end();
  return duoc;
}

// Nạp lúc bật máy. Trả về true nếu có thông số đã lưu. Giá trị nào ngoài giới hạn thì giữ mặc định.
inline bool thamSoNap(ThamSo &ts) {
  Preferences p;
  if (!p.begin("thong_so", true)) return false;
  bool co = p.getUChar("pb", 0) == TS_PHIEN_BAN;
  if (co) {
    ThamSo moi = ts;
    for (int i = 0; i < SO_MUC_THAM_SO; i++) {
      const MucThamSo &m = MUC_THAM_SO[i];
      uint32_t v = p.getULong(m.khoa, moi.*(m.o));
      if (v >= m.nho_nhat && v <= m.lon_nhat) moi.*(m.o) = v;
    }
    if (moi.t_max >= moi.t_min) ts = moi;
    uint8_t cd = p.getUChar("chedo", ts.che_do);
    if (cd == CHE_DO_CO_DINH || cd == CHE_DO_THICH_UNG) ts.che_do = cd;
    uint32_t k = p.getULong("kieu", ts.kieu_pha);
    if (k <= KIEU_DOI_XUNG) ts.kieu_pha = k;
  }
  p.end();
  return co;
}

inline void thamSoXoa() {
  Preferences p;
  if (p.begin("thong_so", false)) { p.clear(); p.end(); }
}
#endif
