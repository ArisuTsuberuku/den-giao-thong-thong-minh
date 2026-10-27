// nhat_ky.h — Nhật ký sự kiện cho trang web: những dòng khiSuKien() in ra Serial (lượt mới, xanh, ưu tiên,
// cắt sớm, tự học...) cũng được giữ lại 40 dòng gần nhất để trang web hiện.
// Dùng như Serial: nk.printf(...). Mỗi dòng (kết thúc bằng \n) vừa ra Serial vừa vào nhật ký.
// Vòng loop() ghi (lõi 1), tác vụ web đọc (lõi 0), nên có khóa.
#pragma once
#include <Arduino.h>

#define NK_SO_DONG 40
#define NK_DAI_DONG 150

class NhatKy : public Print {
public:
  size_t write(uint8_t c) override {
    Serial.write(c);
    if (c == '\r') return 1;
    if (c == '\n') { dayDong(); return 1; }
    if (dai_ < NK_DAI_DONG - 1) dong_[dai_++] = (char)c;
    return 1;
  }
  size_t write(const uint8_t *b, size_t n) override { for (size_t i = 0; i < n; i++) write(b[i]); return n; }

  // JSON: {"so":<số thứ tự dòng mới nhất>,"d":["dòng", ...]} các dòng có số thứ tự lớn hơn tu (cũ trước).
  size_t json(char *ra, size_t co, uint32_t tu) {
    size_t n = 0;
    if (co < 16) return 0;
    portENTER_CRITICAL(&khoa_);
    uint32_t moi = so_;
    n += snprintf(ra + n, co - n, "{\"so\":%lu,\"d\":[", (unsigned long)moi);
    uint32_t dau = moi > NK_SO_DONG ? moi - NK_SO_DONG : 0;
    if (tu > dau) dau = tu;
    bool phay = false;
    for (uint32_t i = dau; i < moi && n + NK_DAI_DONG * 2 + 8 < co; i++) {
      const char *d = vong_[i % NK_SO_DONG];
      if (phay) ra[n++] = ',';
      phay = true;
      ra[n++] = '"';
      for (const char *p = d; *p && n + 8 < co; p++) {
        unsigned char c = (unsigned char)*p;
        if (c == '"' || c == '\\') { ra[n++] = '\\'; ra[n++] = (char)c; }
        else if (c < 0x20) ra[n++] = ' ';
        else ra[n++] = (char)c;
      }
      ra[n++] = '"';
    }
    portEXIT_CRITICAL(&khoa_);
    n += snprintf(ra + n, co - n, "]}");
    return n;
  }

private:
  void dayDong() {
    dong_[dai_] = 0;
    portENTER_CRITICAL(&khoa_);
    memcpy(vong_[so_ % NK_SO_DONG], dong_, dai_ + 1);
    so_++;
    portEXIT_CRITICAL(&khoa_);
    dai_ = 0;
  }
  char dong_[NK_DAI_DONG];
  size_t dai_ = 0;
  char vong_[NK_SO_DONG][NK_DAI_DONG];
  uint32_t so_ = 0;
  portMUX_TYPE khoa_ = portMUX_INITIALIZER_UNLOCKED;
};

static NhatKy nk;
