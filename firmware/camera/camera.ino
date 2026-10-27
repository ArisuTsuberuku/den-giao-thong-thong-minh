// camera.ino — Camera ESP32-CAM (AI-Thinker; hoặc ESP32-S3-CAM, xem LOAI_BOARD) của sa bàn đèn giao thông thông minh.
// Một firmware cho cả 4 camera; chọn CAM_ID và CHE_DO trong cau_hinh.h:
//   CHE_DO_THU_ANH  xem ảnh, chỉnh ROI và 2 tứ giác làn, lưu ảnh huấn luyện
//   CHE_DO_DEM      đếm xe từng làn bằng mô hình Edge Impulse rồi gửi ESP-NOW về board trung tâm
//   CHE_DO_GIA_LAP  không dùng camera: gửi số đếm gõ từ Serial, để thử đường truyền
//
//   may_anh.h    khởi tạo OV2640, cắt ROI      lan_duong.h  gán xe vào làn, lọc nhiễu 5 khung
//   nhan_dien.h  chạy mô hình Edge Impulse      gui_tin.h    gửi JSON bằng ESP-NOW
//   thu_anh.h    trang web thu ảnh, chọn vùng   cau_hinh.h   mọi thứ cần chỉnh
//   mang.h       nối WiFi trung tâm, nhận lệnh  vung_dem.h   vùng đếm lưu trong board
//   cau_hinh_mang.h  tên WiFi trung tâm, địa chỉ 192.168.4.(10 + CAM_ID)
//
// Cách nạp và trình tự làm việc: xem README.md.
#include "cau_hinh.h"
#include "cau_hinh_mang.h"
#include "vung_dem.h"

#if CAM_ID < 1 || CAM_ID > 4
#error "CAM_ID phải là 1, 2, 3 hoặc 4"
#endif

#if CHE_DO == CHE_DO_THU_ANH
// ============================ Chế độ thu ảnh ============================
#include "thu_anh.h"

void setup() {
  Serial.begin(TOC_DO_SERIAL);
  delay(300);
  Serial.printf("\n=== CAM %d - chế độ THU ẢNH ===\n", CAM_ID);
  vungNap();
  Serial.printf("Vùng đếm: %s\n", vung_da_luu ? "đã lưu trong board" : "số trong cau_hinh.h");
  if (!psramFound()) Serial.println(LOI_PSRAM);
  Serial.printf("PSRAM: %u byte trống. Đang khởi tạo camera...\n", (unsigned)ESP.getFreePsram());
  if (!mayAnhKhoiTao()) {
    Serial.println("KHÔNG khởi tạo được camera. Kiểm tra cáp camera và các chân trong cau_hinh.h.");
    while (true) delay(1000);
  }
  Serial.println("Camera đã chạy. Đang bật WiFi...");
  if (!thuAnhKhoiTao()) {
    Serial.println("KHÔNG nối được WiFi trung tâm, không tự phát được WiFi, hoặc thiếu bộ nhớ.");
    while (true) delay(1000);
  }
}

void loop() {
  thuAnhChay();
}

#elif CHE_DO == CHE_DO_DEM
// ============================== Chế độ đếm ==============================
#include "nhan_dien.h"
#include "gui_tin.h"
#if WEB_KHI_DEM
#include "thu_anh.h"
#endif

BoLoc bo_loc(SO_KHUNG_LOC);
VatThe xe[SO_XE_TOI_DA];
uint32_t t_gui = 0, t_in = 0, so_khung = 0, t_khung = 0;
KetQuaKhung da_in = {255, 255, 255, 255};

void setup() {
  Serial.begin(TOC_DO_SERIAL);
  delay(300);
  Serial.printf("\n=== CAM %d - chế độ ĐẾM, mô hình %d x %d ===\n", CAM_ID, EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT);
  vungNap();
  Serial.printf("Vùng đếm: %s\n", vung_da_luu ? "đã lưu trong board" : "số trong cau_hinh.h");
  if (!psramFound()) Serial.println(LOI_PSRAM);
  if (!mayAnhKhoiTao() || !nhanDienKhoiTao()) {
    Serial.println("KHÔNG khởi tạo được camera hoặc thiếu bộ nhớ.");
    while (true) delay(1000);
  }
  if (!guiTinKhoiTao()) Serial.println("KHÔNG khởi tạo được ESP-NOW; vẫn đếm và in ra Serial.");
  Serial.printf("ESP-NOW kênh %d, gửi sau mỗi khung, tối đa %d gói mỗi giây.\n", KENH_WIFI, 1000 / CHU_KI_GUI_MS);
#if WEB_KHI_DEM
  if (WiFi.isConnected() && trangWebKhoiTao())
    Serial.printf("Trang chọn vùng: http://%s (mở từ máy nối WiFi trung tâm)\n", WiFi.localIP().toString().c_str());
#endif
}

void loop() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (fb) {
    int n = nhanDien(fb, ch_cam, xe, SO_XE_TOI_DA);
    esp_camera_fb_return(fb);
    if (n >= 0) { bo_loc.them(phanLan(xe, n, ch_cam)); so_khung++; }
    else Serial.println("Mô hình báo lỗi khi chạy.");
  }
  uint32_t bay_gio = millis();
  if (bay_gio - t_gui >= CHU_KI_GUI_MS) {
    t_gui = bay_gio;
    KetQuaKhung kq = bo_loc.layKetQua();
    char goi[96];
    guiBanTin(kq.s, kq.l, kq.emg, kq.ut, goi, sizeof(goi));
    bool doi = kq.s != da_in.s || kq.l != da_in.l || kq.emg != da_in.emg || kq.ut != da_in.ut;
    if (doi || bay_gio - t_in >= 2000) {              // in khi số đếm đổi, hoặc 2 giây một lần
      float khung_giay = so_khung * 1000.0f / (bay_gio - t_khung + 1);
      mang_khung_giay = khung_giay;
      Serial.printf("%s   (%.1f khung/giây)\n", goi, khung_giay);
      da_in = kq; t_in = bay_gio; t_khung = bay_gio; so_khung = 0;
    }
  }
  mangChay();                                        // trang web, lệnh từ trung tâm, tin trạng thái
}

#elif CHE_DO == CHE_DO_GIA_LAP
// ============================ Chế độ giả lập ============================
#include "gui_tin.h"

uint8_t gia_s = 0, gia_l = 0, gia_emg = 0, gia_ut = 0;
uint32_t t_gui = 0, t_in = 0;
char dong[32];
uint8_t dai_dong = 0;

void datGiaLap(int s, int l, int emg, int ut) {     // dùng chung cho Serial và lệnh "gia" từ trang web trung tâm
  if (s < 0 || s > 9 || l < 0 || l > 9 || emg < 0 || emg > 2 || ut < 0 || ut > 4) return;
  if (emg == 0) ut = 0;
  else if (ut == 0) ut = 4;
  gia_s = (uint8_t)s; gia_l = (uint8_t)l; gia_emg = (uint8_t)emg; gia_ut = (uint8_t)ut;
  Serial.printf("Từ giờ gửi s=%d l=%d emg=%d ut=%d\n", s, l, emg, ut);
}

void setup() {
  Serial.begin(TOC_DO_SERIAL);
  delay(300);
  Serial.printf("\n=== CAM %d - chế độ GIẢ LẬP ===\n", CAM_ID);
  khiLenhGia = datGiaLap;
  if (!guiTinKhoiTao()) Serial.println("KHÔNG khởi tạo được ESP-NOW.");
  Serial.println("Gõ 3 số \"s l emg\" rồi Enter, ví dụ: 3 1 0  (3 xe làn ngoài, 1 xe làn trong, không có xe ưu tiên)");
  Serial.println("Có xe ưu tiên thì thêm số thứ tư là loại xe, ví dụ: 3 1 2 1  (xe cứu hỏa ở làn trong).");
  Serial.println("Loại xe: 1 cứu hỏa, 2 quân sự, 3 công an, 4 cứu thương. Không ghi thì là xe cứu thương.");
}

void loop() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      dong[dai_dong] = 0;
      dai_dong = 0;
      int s, l, emg, ut = 0;
      int so = sscanf(dong, "%d %d %d %d", &s, &l, &emg, &ut);
      if (so >= 3 && s >= 0 && s <= 9 && l >= 0 && l <= 9 && emg >= 0 && emg <= 2 && ut >= 0 && ut <= 4) datGiaLap(s, l, emg, ut);
      else Serial.println("Cần 3 hoặc 4 số: s (0-9), l (0-9), emg (0, 1 hoặc 2), ut (1 đến 4).");
    } else if (dai_dong < sizeof(dong) - 1) dong[dai_dong++] = c;
  }
  uint32_t bay_gio = millis();
  if (bay_gio - t_gui >= CHU_KI_GUI_MS) {
    t_gui = bay_gio;
    char goi[96];
    bool duoc = guiBanTin(gia_s, gia_l, gia_emg, gia_ut, goi, sizeof(goi));
    if (bay_gio - t_in >= 2000) { t_in = bay_gio; Serial.printf("%s  %s\n", goi, duoc ? "" : "(gửi lỗi)"); }
  }
  mangChay();
}

#else
#error "CHE_DO trong cau_hinh.h phải là CHE_DO_THU_ANH, CHE_DO_DEM hoặc CHE_DO_GIA_LAP"
#endif
