// den.h — Xuất 24 chân đèn. Trong lúc chạy, mọi lệnh bật đèn đều đi qua hàm denXuat(); hàm này
// kiểm tra xung đột lần cuối trước khi đụng tới chân: nếu yêu cầu có hai nhóm xung đột mà một
// nhóm đang xanh thì từ chối và giữ nguyên đèn. Chỉ có denKhoiTao() (đỏ tất cả) và denThuDay()
// (mỗi lúc sáng đúng một bóng) là ghi thẳng ra chân.
#pragma once
#include "cau_hinh.h"
#include "dieu_khien.h"

static uint8_t den_dang_xuat[8] = {255, 255, 255, 255, 255, 255, 255, 255};

inline void denGhiNhom(int g, uint8_t mau) {
  digitalWrite(CHAN_DEN[g][0], mau == MAU_DO ? DEN_MUC_BAT : !DEN_MUC_BAT);
  digitalWrite(CHAN_DEN[g][1], mau == MAU_VANG ? DEN_MUC_BAT : !DEN_MUC_BAT);
  digitalWrite(CHAN_DEN[g][2], mau == MAU_XANH ? DEN_MUC_BAT : !DEN_MUC_BAT);
  den_dang_xuat[g] = mau;
}

// mau[g] là MAU_DO, MAU_VANG hoặc MAU_XANH cho từng nhóm. Trả về false nếu bị từ chối.
inline bool denXuat(const uint8_t mau[8]) {
  for (int i = 0; i < 8; i++) {
    for (int j = i + 1; j < 8; j++) {
      if (mau[i] == MAU_DO || mau[j] == MAU_DO) continue;
      if (mau[i] != MAU_XANH && mau[j] != MAU_XANH) continue;        // cùng vàng (nháy báo lỗi) thì được
      if (!hopLeMatNa((uint8_t)((1u << i) | (1u << j)))) return false;
    }
  }
  for (int g = 0; g < 8; g++) {
    if (mau[g] != den_dang_xuat[g]) denGhiNhom(g, mau[g]);
  }
  return true;
}

inline void denKhoiTao() {
  for (int g = 0; g < 8; g++) {
    for (int k = 0; k < 3; k++) pinMode(CHAN_DEN[g][k], OUTPUT);
    denGhiNhom(g, MAU_DO);       // bật máy: đỏ tất cả
  }
}

// Thử dây lúc lắp: lần lượt sáng đỏ, vàng, xanh của từng nhóm (mỗi lúc chỉ một bóng sáng).
inline void denThuDay(uint16_t ms_moi_bong) {
  for (int g = 0; g < 8; g++) {
    for (int k = 0; k < 3; k++) {
      for (int a = 0; a < 8; a++) for (int b = 0; b < 3; b++) digitalWrite(CHAN_DEN[a][b], !DEN_MUC_BAT);
      digitalWrite(CHAN_DEN[g][k], DEN_MUC_BAT);
      delay(ms_moi_bong);
    }
  }
  for (int g = 0; g < 8; g++) denGhiNhom(g, MAU_DO);
}
