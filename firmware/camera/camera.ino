// camera.ino — Camera ESP32-CAM (AI-Thinker; hoặc ESP32-S3-CAM, xem LOAI_BOARD) của sa bàn đèn giao thông thông minh.
// Một tệp firmware cho cả 4 camera. Số CAM và chế độ lưu trong bộ nhớ của board (danh_tinh.h): lần nạp USB
// đầu lấy CAM_ID, CHE_DO trong cau_hinh.h; sau đó đổi được trên trang web mà không phải nạp lại.
//   CHE_DO_THU_ANH  xem ảnh, chỉnh ROI và 2 tứ giác làn, lưu ảnh huấn luyện
//   CHE_DO_DEM      đếm xe từng làn bằng mô hình Edge Impulse rồi gửi ESP-NOW về board trung tâm
//                   (cần cài thư viện mô hình; chưa cài thì firmware vẫn biên dịch được, chỉ thiếu chế độ này)
//   CHE_DO_GIA_LAP  không dùng camera: gửi số đếm gõ từ Serial, để thử đường truyền
//
//   may_anh.h    khởi tạo OV2640, cắt ROI      lan_duong.h  gán xe vào làn, lọc nhiễu
//   nhan_dien.h  chạy mô hình Edge Impulse      gui_tin.h    gửi JSON bằng ESP-NOW
//   thu_anh.h    trang web thu ảnh, chọn vùng   cau_hinh.h   mọi thứ cần chỉnh
//   mang.h       nối WiFi trung tâm, nhận lệnh  vung_dem.h   vùng đếm lưu trong board
//   danh_tinh.h  số CAM, chế độ lưu trong board nap_tu_xa.h  nạp firmware qua WiFi, tự quay về bản cũ
//   cau_hinh_mang.h  tên WiFi trung tâm, địa chỉ 192.168.4.(10 + số CAM)
//
// Cách nạp và trình tự làm việc: xem README.md.
#include "cau_hinh.h"
#include "cau_hinh_mang.h"
#include "danh_tinh.h"
#include "vung_dem.h"
#include "gui_tin.h"
#include "thu_anh.h"
#if CO_MO_HINH
#include "nhan_dien.h"
#endif

#if CAM_ID < 1 || CAM_ID > 4
#error "CAM_ID phải là 1, 2, 3 hoặc 4"
#endif
#if CHE_DO != CHE_DO_THU_ANH && CHE_DO != CHE_DO_DEM && CHE_DO != CHE_DO_GIA_LAP
#error "CHE_DO trong cau_hinh.h phải là CHE_DO_THU_ANH, CHE_DO_DEM hoặc CHE_DO_GIA_LAP"
#endif

void dungLai(const char *cau) {
  // Không chạy tiếp được. Bản vừa nạp qua WiFi thì tự quay về bản cũ sau 2 phút (nap_tu_xa.h).
  Serial.println(cau);
  while (true) delay(1000);
}

// ============================ Chế độ thu ảnh ============================
void batThuAnh() {
  Serial.printf("PSRAM: %u byte trống. Đang khởi tạo camera...\n", (unsigned)ESP.getFreePsram());
  if (!mayAnhKhoiTao()) dungLai("KHÔNG khởi tạo được camera. Kiểm tra cáp camera và các chân trong cau_hinh.h.");
  khoe_may_anh = true;
  Serial.println("Camera đã chạy. Đang bật WiFi...");
  if (!thuAnhKhoiTao()) dungLai("KHÔNG nối được WiFi trung tâm, không tự phát được WiFi, hoặc thiếu bộ nhớ.");
}

void chayThuAnh() {
  thuAnhChay();
}

// ============================== Chế độ đếm ==============================
#if CO_MO_HINH
BoLoc bo_loc(SO_KHUNG_LOC);
VatThe xe[SO_XE_TOI_DA];
uint32_t t_gui = 0, t_in = 0, so_khung = 0, t_khung = 0;
KetQuaKhung da_in = {255, 255, 255, 255};

void batDem() {
  Serial.printf("Mô hình %d x %d\n", EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT);
  if (!mayAnhKhoiTao() || !nhanDienKhoiTao()) dungLai("KHÔNG khởi tạo được camera hoặc thiếu bộ nhớ.");
  khoe_may_anh = true;
  if (!guiTinKhoiTao()) Serial.println("KHÔNG khởi tạo được ESP-NOW; vẫn đếm và in ra Serial.");
  Serial.printf("ESP-NOW kênh %d, gửi sau mỗi khung, tối đa %d gói mỗi giây.\n", KENH_WIFI, 1000 / CHU_KI_GUI_MS);
#if WEB_KHI_DEM
  if (WiFi.isConnected() && trangWebKhoiTao())
    Serial.printf("Trang chọn vùng: http://%s (mở từ máy nối WiFi trung tâm)\n", WiFi.localIP().toString().c_str());
#endif
}

void chayDem() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (fb) {
    int n = nhanDien(fb, ch_cam, xe, SO_XE_TOI_DA);
    esp_camera_fb_return(fb);
    if (n >= 0) { bo_loc.them(phanLan(xe, n, ch_cam)); so_khung++; khoe_mo_hinh = true; }
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
#endif

// ============================ Chế độ giả lập ============================
uint8_t gia_s = 0, gia_l = 0, gia_emg = 0, gia_ut = 0;
uint32_t gl_t_gui = 0, gl_t_in = 0;
char dong[32];
uint8_t dai_dong = 0;

void datGiaLap(int s, int l, int emg, int ut) {     // dùng chung cho Serial và lệnh "gia" từ trang web trung tâm
  if (s < 0 || s > 9 || l < 0 || l > 9 || emg < 0 || emg > 2 || ut < 0 || ut > 4) return;
  if (emg == 0) ut = 0;
  else if (ut == 0) ut = 4;
  gia_s = (uint8_t)s; gia_l = (uint8_t)l; gia_emg = (uint8_t)emg; gia_ut = (uint8_t)ut;
  Serial.printf("Từ giờ gửi s=%d l=%d emg=%d ut=%d\n", s, l, emg, ut);
}

void batGiaLap() {
  khiLenhGia = datGiaLap;
  if (!guiTinKhoiTao()) Serial.println("KHÔNG khởi tạo được ESP-NOW.");
  Serial.println("Gõ 3 số \"s l emg\" rồi Enter, ví dụ: 3 1 0  (3 xe làn ngoài, 1 xe làn trong, không có xe ưu tiên)");
  Serial.println("Có xe ưu tiên thì thêm số thứ tư là loại xe, ví dụ: 3 1 2 1  (xe cứu hỏa ở làn trong).");
  Serial.println("Loại xe: 1 cứu hỏa, 2 quân sự, 3 công an, 4 cứu thương. Không ghi thì là xe cứu thương.");
}

void chayGiaLap() {
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
  if (bay_gio - gl_t_gui >= CHU_KI_GUI_MS) {
    gl_t_gui = bay_gio;
    char goi[96];
    bool duoc = guiBanTin(gia_s, gia_l, gia_emg, gia_ut, goi, sizeof(goi));
    if (bay_gio - gl_t_in >= 2000) { gl_t_in = bay_gio; Serial.printf("%s  %s\n", goi, duoc ? "" : "(gửi lỗi)"); }
  }
  mangChay();
}

// ================================ Chung ================================
static const char *const TEN_CHE_DO[] = {"", "THU ẢNH", "ĐẾM", "GIẢ LẬP"};

void setup() {
  Serial.begin(TOC_DO_SERIAL);
  delay(300);
  napKhoiDong();
  danhTinhNap();
  Serial.printf("\n=== CAM %d - chế độ %s - firmware bản %s ===\n", cam_id, TEN_CHE_DO[che_do], phienBanFw());
  if (cam_id != CAM_ID) Serial.printf("(Số CAM lưu trong board là %d, khác CAM_ID trong code là %d.)\n", cam_id, CAM_ID);
  if (che_do_bi_doi) Serial.println("Đã chọn chế độ ĐẾM nhưng firmware chưa có mô hình: chạy chế độ THU ẢNH.");
  if (nap_cho_xn) Serial.println("BẢN MỚI NẠP QUA WIFI: tự kiểm tra trong 2 phút, không ổn thì quay về bản cũ.");
  if (nap_co_ban_loi) Serial.println("(Một bản nạp gần đây bị lỗi, board đã quay về bản này.)");
  vungNap();
  Serial.printf("Vùng đếm: %s\n", vung_da_luu ? "đã lưu trong board" : "số trong cau_hinh.h");
  if (che_do != CHE_DO_GIA_LAP && !psramFound()) Serial.println(LOI_PSRAM);
  if (che_do == CHE_DO_GIA_LAP) batGiaLap();
#if CO_MO_HINH
  else if (che_do == CHE_DO_DEM) batDem();
#endif
  else batThuAnh();
}

void loop() {
  if (che_do == CHE_DO_GIA_LAP) chayGiaLap();
#if CO_MO_HINH
  else if (che_do == CHE_DO_DEM) chayDem();
#endif
  else chayThuAnh();
}
