// thu_camera.cpp — Kiểm tra trên máy tính các phần của firmware camera không cần phần cứng:
// cắt ROI và co ảnh (cat_anh.h), gán xe vào làn và lọc nhiễu 5 khung (lan_duong.h).
// Chạy: g++ -std=c++11 -Wall -Wextra -o /tmp/thu_camera kiem_thu/thu_camera.cpp && /tmp/thu_camera
#include <cstdio>
#include <vector>
#include "../firmware/camera/cat_anh.h"

static int so_loi = 0;
#define KIEM(dk, ten) do { if (!(dk)) { so_loi++; printf("  SAI: %s\n", ten); } } while (0)

static void datDiem(std::vector<uint8_t> &khung, int rong, int x, int y, int r, int g, int b) {
  uint16_t v = (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
  khung[2 * (y * rong + x)] = (uint8_t)(v >> 8);
  khung[2 * (y * rong + x) + 1] = (uint8_t)(v & 0xFF);
}

int main() {
  // ---------- cắt ROI ----------
  const int RONG = 320, CAO = 240;
  std::vector<uint8_t> khung(RONG * CAO * 2, 0);
  CauHinhCam ch = {100, 40, 120, 160, {{100, 40}, {160, 40}, {160, 200}, {100, 200}}, {{160, 40}, {220, 40}, {220, 200}, {160, 200}}};
  // nửa trái ROI màu đỏ, nửa phải màu xanh dương, ngoài ROI màu xanh lá
  for (int y = 0; y < CAO; y++) for (int x = 0; x < RONG; x++) {
    bool trong_roi = x >= 100 && x < 220 && y >= 40 && y < 200;
    if (!trong_roi) datDiem(khung, RONG, x, y, 0, 255, 0);
    else if (x < 160) datDiem(khung, RONG, x, y, 255, 0, 0);
    else datDiem(khung, RONG, x, y, 0, 0, 255);
  }
  std::vector<uint8_t> nho(96 * 96 * 3);
  catRoiRgb565(khung.data(), RONG, CAO, ch, nho.data(), 96, 96);
  const uint8_t *trai = &nho[(48 * 96 + 10) * 3], *phai = &nho[(48 * 96 + 85) * 3];
  KIEM(trai[2] >= 248 && trai[1] == 0 && trai[0] == 0, "nua trai ROI phai do (thu tu B, G, R)");
  KIEM(phai[0] >= 248 && phai[1] == 0 && phai[2] == 0, "nua phai ROI phai xanh duong");
  bool co_xanh_la = false;
  for (size_t i = 0; i < nho.size(); i += 3) if (nho[i + 1] > 8) co_xanh_la = true;
  KIEM(!co_xanh_la, "anh cat khong duoc lan diem anh ngoai ROI");
  std::vector<uint8_t> goc(120 * 160 * 3);
  catRoiRgb565(khung.data(), RONG, CAO, ch, goc.data(), 120, 160);   // cỡ gốc: mỗi ô đúng một điểm ảnh
  KIEM(goc[(0 * 120 + 59) * 3 + 2] >= 248 && goc[(0 * 120 + 60) * 3 + 0] >= 248, "cat co goc: ranh gioi do/xanh dung cot 60");

  // ---------- điểm trong tứ giác, gán làn ----------
  Diem thang[4] = {{90, 20}, {150, 20}, {170, 230}, {60, 230}};   // hình thang như làn nhìn phối cảnh
  KIEM(trongTuGiac(thang, 120, 100), "tam hinh thang nam trong");
  KIEM(!trongTuGiac(thang, 70, 30), "goc tren trai nam ngoai hinh thang");
  KIEM(trongTuGiac(thang, 70, 220), "gan day duoi nam trong");
  KIEM(!trongTuGiac(thang, 200, 100), "ben phai nam ngoai");
  VatThe xe[5] = {{120, 80, 0}, {130, 150, 0}, {180, 100, 4}, {250, 100, 1}, {190, 180, 0}};
  KetQuaKhung kq = phanLan(xe, 5, ch);
  KIEM(kq.s == 2 && kq.l == 2 && kq.emg == 2 && kq.ut == 4,
       "phan lan: 2 xe lan ngoai, 2 xe lan trong, xe cuu thuong o lan trong, xe cuu hoa ngoai vung bi bo");
  VatThe hai_ut[3] = {{180, 100, 4}, {120, 80, 2}, {130, 150, 3}};
  kq = phanLan(hai_ut, 3, ch);
  KIEM(kq.emg == 1 && kq.ut == 2, "nhieu xe uu tien trong vung: bao xe hang cao nhat (quan su truoc cong an, cuu thuong)");

  // ---------- lọc nhiễu, 5 khung ----------
  BoLoc loc(5);
  KetQuaKhung day[] = {{3, 1, 0, 0}, {3, 1, 0, 0}, {9, 1, 1, 1}, {3, 0, 0, 0}, {3, 1, 0, 0}};   // khung thứ 3 nhiễu
  for (int i = 0; i < 5; i++) loc.them(day[i]);
  kq = loc.layKetQua();
  KIEM(kq.s == 3 && kq.l == 1 && kq.emg == 0 && kq.ut == 0, "trung vi bo duoc 1 khung nhieu; emg 1/5 khung chua bat");
  KetQuaKhung sach = {3, 1, 0, 0}, co_ut = {3, 1, 2, 1};
  for (int i = 0; i < 3; i++) loc.them(sach);        // đẩy khung nhiễu ra khỏi 5 khung gần nhất
  loc.them(co_ut); loc.them(co_ut);
  KIEM(loc.layKetQua().emg == 0, "emg 2/5 khung chua bat");
  loc.them(co_ut);
  KIEM(loc.layKetQua().emg == 2 && loc.layKetQua().ut == 1, "emg 3/5 khung thi bat, dung lan trong, dung loai xe cuu hoa");
  KetQuaKhung nham = {3, 1, 2, 3};                    // mô hình nhìn nhầm thành xe công an ở 2 khung
  loc.them(nham); loc.them(nham);
  KIEM(loc.layKetQua().emg == 2 && loc.layKetQua().ut == 1, "loai xe lay theo da so: 3 khung cuu hoa, 2 khung cong an -> cuu hoa");
  loc.them(nham);
  KIEM(loc.layKetQua().ut == 3, "3 khung cong an, 2 khung cuu hoa -> cong an");
  BoLoc cu(5);
  KetQuaKhung kieu_cu = {1, 0, 1, 0};                 // có emg mà không ghi loại
  for (int i = 0; i < 3; i++) cu.them(kieu_cu);
  KIEM(cu.layKetQua().emg == 1 && cu.layKetQua().ut == 4, "co emg ma khong ghi loai: coi la xe cuu thuong");
  BoLoc moi(5);
  KetQuaKhung mot = {2, 0, 0, 0};
  moi.them(mot);
  KIEM(moi.layKetQua().s == 2, "moi co 1 khung van tra ve duoc");

  // ---------- lọc nhiễu, 3 khung (mức mặc định khi camera chỉ xử lí được chừng 1 khung mỗi giây) ----------
  BoLoc ba(3);
  KetQuaKhung a1 = {2, 1, 0, 0}, a2 = {5, 1, 1, 3}, a3 = {2, 0, 0, 0};
  ba.them(a1); ba.them(a2); ba.them(a3);
  kq = ba.layKetQua();
  KIEM(kq.s == 2 && kq.l == 1 && kq.emg == 0, "loc 3 khung: trung vi bo 1 khung nhieu; emg 1/3 khung chua bat");
  ba.them(a2);                                         // 3 khung gần nhất: a2, a3, a2
  kq = ba.layKetQua();
  KIEM(kq.s == 5 && kq.emg == 1 && kq.ut == 3, "loc 3 khung: emg 2/3 khung thi bat, dung loai xe cong an");
  ba.them(a1); ba.them(a1);                            // 3 khung gần nhất: a2, a1, a1
  KIEM(ba.layKetQua().emg == 0 && ba.layKetQua().s == 2, "loc 3 khung: het xe uu tien 2 khung la tat");
  BoLoc khong(1);                                      // không lọc: khung nào ra khung đó
  khong.them(a1); khong.them(a2);
  kq = khong.layKetQua();
  KIEM(kq.s == 5 && kq.l == 1 && kq.emg == 1 && kq.ut == 3, "khong loc: lay dung khung moi nhat");
  BoLoc sai(4);
  sai.them(a1); sai.them(a2); sai.them(a2); sai.them(a2);
  KIEM(sai.layKetQua().s == 5, "so khung la khong hop le (4) thi dung 3");

  printf(so_loi == 0 ? "DAT: khong co loi.\n" : "KHONG DAT: %d loi.\n", so_loi);
  return so_loi == 0 ? 0 : 1;
}
