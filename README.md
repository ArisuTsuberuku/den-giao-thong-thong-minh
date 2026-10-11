# Đèn giao thông thông minh — NCKHKT 2026

Code cho mô hình ngã tư trên sa bàn Lần 08: 4 camera ESP32-S3-CAM đếm xe theo làn, 1 board ESP32-S3 N16R8 quyết định kế hoạch đèn và điều khiển 24 bóng đèn, cùng một trang web mô phỏng chạy trên máy tính.

| Thư mục | Nội dung |
|---|---|
| `web/` | Web mô phỏng: sa bàn nhìn từ trên xuống, bảng thông số, 4 ô camera |
| `firmware/trung_tam/` | Code board trung tâm ESP32-S3 N16R8 |
| `firmware/camera/` | Code camera ESP32-S3-CAM (một code cho cả 4 board) |
| `kiem_thu/` | Các bài kiểm tra chạy trên máy tính, không cần phần cứng |
| `cong_cu/` | Gộp web thành một tệp; tạo nền sa bàn từ tệp decal |
| `TIEN_DO_DU_AN.md` | Tiến độ và các quyết định thiết kế của dự án |

## Tình trạng kiểm tra

Đã kiểm tra trên máy tính:

- Bộ quyết định đèn cho đúng thời gian chu kì ở các ca của mục 6 trong `TIEN_DO_DU_AN.md` (40 s, 110 s, 90 s, 160 s).
- Bản C++ trong firmware và bản JavaScript trong web cho kết quả giống nhau trên 760 000 dòng trạng thái sinh ngẫu nhiên, kể cả lúc bộ đếm thời gian 32 bit quay vòng.
- Trong 400 lần chạy ngẫu nhiên 15 phút (có gần 600 lần cắt xanh sớm): không lần nào hai nhóm xung đột cùng sáng, không nhóm nào từ xanh sang đỏ mà thiếu vàng, đỏ toàn phần luôn đủ.
- Mô phỏng 30 phút với 6 kiểu dòng xe: không có hai xe đè lên nhau, không xe nào vượt đèn đỏ, xe cứu thương luôn qua được.
- Cả hai firmware biên dịch được cho ESP32-S3 với lõi `esp32` 2.0.9 và 3.3.12. Chế độ đếm của camera mới biên dịch với thư viện Edge Impulse giả (`kiem_thu/gia_ei`).

Chưa kiểm tra, vì cần phần cứng thật: nạp vào board, độ sáng đèn ở 3,3 V, ảnh từ OV2640, đường truyền ESP-NOW, mô hình nhận diện. Danh sách việc cần thử nằm ở cuối mỗi phần dưới đây.

## 1. Web mô phỏng

Mở `web/index.html` bằng trình duyệt, không cần mạng và không cần board.

- **Bên trái:** chạy, tạm dừng, tốc độ ×1 đến ×16; chế độ cố định hoặc thích ứng; các thông số đèn; lưu lượng và tỉ lệ rẽ trái của từng hướng; trễ phản ứng của người lái; nút thả xe cứu thương; nút so sánh hai chế độ.
- **Giữa:** sa bàn vẽ từ đúng tệp decal Lần 08. Hai cột hai bên cho biết tổ hợp đang xanh, thời gian còn lại, kế hoạch chu kì, số xe đã qua và thời gian chờ.
- **Phía dưới:** 4 ô camera. Khung đứt là ROI, vùng xanh dương là làn ngoài (`s`), vùng hồng là làn trong (`l`), dòng cuối là bản tin JSON gửi về trung tâm.

Đèn trong web do đúng hai tệp `quyet_dinh.js` và `dieu_khien.js` điều khiển; hai tệp này là bản JavaScript của `quyet_dinh.h` và `dieu_khien.h` trong firmware. Web hiện là mô phỏng thuần, chưa nối với board.

Gộp thành một tệp để gửi qua Zalo hoặc email:

```
node cong_cu/dong_goi.js
```

Lệnh này tạo `Mo_phong_sa_ban.html` ở thư mục gốc.

### Xe trong mô phỏng chạy thế nào

- Xe chạy 120 mm/s, giảm tốc khi vào cua, giữ khoảng cách với xe trước và xếp hàng cách nhau 76 mm (vùng chờ chứa đúng 4 xe mỗi làn).
- **Trễ phản ứng của người lái:** xe đang đứng trong hàng chờ chỉ lăn bánh sau khi đã thấy đi được (đèn xanh, hoặc xe trước đã rời ra) một khoảng bằng mức trễ chung nhân với hệ số riêng của từng người lái, từ 0,7 đến 1,3. Mức chung mặc định 1 s, chỉnh được trên web; đặt 0 là không trễ. Xe đã qua vạch dừng thì không tính trễ nữa.
- Với mức 1 s, hàng 4 xe xả như sau: xe đầu qua vạch dừng 1,4 s sau khi đèn xanh, mỗi xe sau cách xe trước khoảng 2,1 s (không trễ thì 1,2 s). Con số 2 s mỗi xe mỗi làn cũng là mức thường gặp ngoài đường.
- Gặp đèn vàng: xe dừng nếu kịp, không kịp thì đi tiếp. Xe rẽ phải đi làn riêng, nhường xe trên làn mà nó nhập vào.

### Kết quả mô phỏng

Thời gian chờ trung bình trước vạch dừng (giây), 60 phút mô phỏng, trung bình 3 lần chạy, thông số mặc định (lấy từ `node kiem_thu/thu_mo_phong.js`):

| Kiểu dòng xe | Cố định | Thích ứng, không cắt sớm | Thích ứng, cắt sớm 5 s | So với cố định |
|---|---|---|---|---|
| Đều, vừa phải (6 xe/phút mỗi hướng) | 58,9 | 52,2 | 32,5 | −45 % |
| Dọc đông, ngang vắng | 176,8 | 34,5 | 19,4 | −89 % |
| Chỉ Hướng 1 có xe | 413,0 | 8,4 | 2,5 | −99 % |
| Giờ cao điểm (12 xe/phút mỗi hướng) | 674,3 | 296,3 | 277,9 | −59 % |
| Ít rẽ trái (5 %) | 38,7 | 66,5 | 48,2 | +24 % |
| Vắng (1,5 xe/phút mỗi hướng) | 33,4 | 25,1 | 11,2 | −66 % |

Đây là số của mô phỏng với trễ phản ứng 1 s, chưa phải số đo trên sa bàn thật. Cột cuối so chế độ thích ứng có cắt sớm (mặc định) với chế độ cố định.

Chế độ thích ứng vẫn thua ở kiểu "ít rẽ trái". Camera không biết xe ở làn trong sẽ đi thẳng hay rẽ trái, nên bộ quyết định mở riêng từng hướng; còn chế độ cố định cho hai hướng đối diện đi thẳng cùng lúc, lợi hơn khi gần như không ai rẽ trái. Cắt sớm thu lại được một phần thời gian thừa (66,5 s xuống 48,2 s) nhưng chưa đủ. Đây là chỗ để cải tiến ở bộ quyết định mức 2.

## 2. Bộ quyết định đèn

8 nhóm tín hiệu: mỗi hướng một nhóm đi thẳng và một nhóm rẽ trái. Hai nhóm chỉ được xanh cùng lúc khi cùng một hướng, hoặc hai hướng đối diện và cùng loại (thẳng với thẳng, trái với trái). Rẽ trái không bao giờ xanh khi hướng ngược chiều đang đi thẳng.

**Chế độ cố định** (đối chứng): dọc rẽ trái 15 s → ngang đi thẳng 30 s → ngang rẽ trái 15 s → dọc đi thẳng 30 s, chu kì 110 s.

**Chế độ thích ứng**, chốt kế hoạch ở đầu mỗi chu kì theo số đếm lúc đó, lần lượt cho trục dọc rồi trục ngang:

1. Cả hai hướng của trục trống: bỏ qua trục. Cả ngã tư trống: hai trục luân phiên đi thẳng 15 s.
2. Một hướng trống: mở riêng hướng còn lại (thẳng và trái cùng xanh).
3. Hai hướng đều có xe, làn trong đều trống: thẳng cả hai.
4. Còn lại, so hai phương án và chọn phương án tốn ít thời gian hơn:
   - thẳng cả hai, rồi rẽ trái cho hướng có xe ở làn trong;
   - riêng từng hướng, hướng đông đi trước.

   Bằng nhau mà cả hai làn trong đều có xe thì chọn riêng từng hướng, vì làn trong đi chung thẳng và trái nên xe hai loại dễ chặn nhau.
5. Camera của một hướng im quá 3 s: từ chu kì sau, trục đó chạy thời gian cố định.
6. Cắt xanh sớm (`CAT_SOM`, mặc định 5 s, đặt 0 để tắt): tổ hợp đang xanh không còn xe suốt 5 s mà hướng khác có xe chờ thì chuyển vàng ngay rồi sang bước kế, không đợi hết thời gian đã định. Khi sang bước kế, bước nào không còn xe thì bỏ qua. Không cắt nếu camera của hướng đang xanh mất tín hiệu, hoặc không có xe nào chờ ở hướng khác (khi đó đèn cứ giữ xanh).

Thời gian xanh định trước của mỗi bước = Tmin + 5 s × (số xe ở làn đông nhất × quy đổi / 10), trong khoảng Tmin đến Tmax; cắt xanh sớm có thể làm bước kết thúc trước mức đó. Giữa hai bước, nhóm có mặt ở cả hai thì giữ xanh; nhóm rời đi qua vàng 3 s rồi đỏ, sau đó đỏ toàn phần 2 s.

**Xe ưu tiên** (`emg` khác 0): bước đang chạy phải xanh ít nhất 5 s, rồi kết thúc đúng trình tự vàng, đỏ toàn phần và mở riêng hướng có xe ưu tiên. Giữ xanh tới khi không còn thấy xe 5 s, tối đa 60 s, rồi lập kế hoạch chu kì mới.

Sửa luật thì sửa cả `firmware/trung_tam/quyet_dinh.h` và `web/quyet_dinh.js`, rồi chạy `node kiem_thu/so_khop.js` để chắc hai bản vẫn giống nhau.

## 3. Board trung tâm (`firmware/trung_tam`)

**Arduino IDE:** cài lõi `esp32` của Espressif và thư viện `ArduinoJson`. Chọn Board `ESP32S3 Dev Module`, Flash Size `16MB`, PSRAM `OPI PSRAM`, USB CDC On Boot `Disabled`. Cắm cáp vào cổng ghi `COM` trên board. Serial Monitor 115200, kết thúc dòng `Newline`.

**Nối dây** (sửa được trong `cau_hinh.h`):

| Cột | Nhóm | Đỏ | Vàng | Xanh |
|---|---|---|---|---|
| Đ1 | H1 thẳng | 4 | 5 | 6 |
| Đ1 | H1 trái | 7 | 15 | 16 |
| Đ2 | H2 thẳng | 17 | 18 | 8 |
| Đ2 | H2 trái | 9 | 10 | 11 |
| Đ3 | H3 thẳng | 12 | 13 | 14 |
| Đ3 | H3 trái | 1 | 2 | 42 |
| Đ4 | H4 thẳng | 41 | 40 | 39 |
| Đ4 | H4 trái | 38 | 47 | 21 |

GND của nguồn 5 V, board và 8 module đèn nối chung.

**Lệnh gõ vào Serial Monitor:**

| Lệnh | Tác dụng |
|---|---|
| `{"id":1,"s":3,"l":1,"emg":0}` | Giả làm camera Hướng 1: 3 xe làn ngoài, 1 xe làn trong |
| `codinh` hoặc `thichung` | Đổi chế độ, có hiệu lực từ chu kì sau |
| `tua 4` | Chạy nhanh gấp 4 (1 đến 8) |
| `catsom 5` | Hết xe quá 5 s thì cắt xanh sớm; `catsom 0` là tắt |
| `xem` | In trạng thái, số đếm từng hướng, tỉ lệ mất gói |
| `xoa` | Bỏ các số đếm đã gõ tay |
| `thuday` | Lần lượt sáng từng bóng để kiểm tra dây |

LED trên board: xanh lá là đủ 4 camera, vàng là thiếu camera, xanh dương là đang ưu tiên, đỏ là lỗi an toàn.

**Việc cần thử trên board thật:**

1. GĐ1: gõ `codinh`, rồi `thuday` để kiểm tra 24 bóng đúng chân và đủ sáng. Sau đó theo dõi một chu kì 110 s.
2. GĐ2: gõ `thichung`, gõ vài bản tin JSON, xem dòng "Kế hoạch" của chu kì sau đổi theo còn chu kì đang chạy giữ nguyên.
3. Thử cắt xanh sớm: gõ `{"id":1,"s":3,"l":0,"emg":0}` và `{"id":2,"s":2,"l":0,"emg":0}`, chờ Hướng 1 xanh rồi gõ `{"id":1,"s":0,"l":0,"emg":0}`. Sau 5 s phải thấy dòng "cắt xanh sớm" và Hướng 1 chuyển vàng.
4. Gõ `{"id":2,"s":1,"l":0,"emg":1}` để thử ưu tiên, rồi gõ lại với `"emg":0`.

## 4. Camera (`firmware/camera`)

**Arduino IDE:** Board `ESP32S3 Dev Module`, Flash Size `16MB`, PSRAM `OPI PSRAM` (bắt buộc), Partition Scheme `16M Flash (3MB APP/9.9MB FATFS)`, USB CDC On Boot `Disabled`. Nạp qua đế nạp.

Chân camera trong `cau_hinh.h` theo loại ESP32-S3-CAM phổ biến (trùng kiểu ESP32-S3-EYE). Nếu chế độ thu ảnh báo không khởi tạo được camera thì kiểm tra lại cáp và bảng chân này.

Mỗi board sửa hai dòng trong `cau_hinh.h`: `CAM_ID` (1 đến 4) và `CHE_DO`.

**Trình tự làm việc:**

1. Nạp `CHE_DO_THU_ANH`. Camera phát WiFi `CAM1-SaBan` (mật khẩu `12345678`); nối máy tính vào rồi mở `http://192.168.4.1`.
2. Trên trang đó: vẽ ROI quanh 2 làn chờ, bấm 4 góc làn ngoài, 4 góc làn trong. Chép dòng cấu hình trang in ra, dán đè lên dòng của camera đó trong `cau_hinh.h`, nạp lại.
3. Thu ảnh huấn luyện bằng nút "Lưu 1 ảnh" hoặc "Tự lưu". Ảnh lưu là ảnh ROI, đúng ảnh sẽ đưa vào mô hình.
4. Trên Edge Impulse: tải ảnh lên, gán nhãn `xe` và `xe_uu_tien`, tạo impulse ảnh 96 × 96 với Resize mode `Squash`, khối Object Detection (FOMO), huấn luyện, xuất `Arduino library`.
5. Cài thư viện `.zip` vào Arduino IDE, sửa `THU_VIEN_EI` trong `cau_hinh.h` cho đúng tên tệp `.h`, đổi `CHE_DO` thành `CHE_DO_DEM`, nạp lại.
6. Camera in bản tin ra Serial và gửi ESP-NOW 4 gói mỗi giây. Ở board trung tâm gõ `xem` để thấy số đếm và tỉ lệ mất gói.

`CHE_DO_GIA_LAP` không cần camera và mô hình: gõ `3 1 0` vào Serial để gửi s = 3, l = 1, emg = 0. Dùng để thử đường truyền ESP-NOW trước khi có mô hình.

Nếu thư viện Edge Impulse báo lỗi khi biên dịch với lõi `esp32` 3.x, cài lõi 2.0.17; code trong repo biên dịch được trên cả hai dòng.

## 5. Kiểm tra trên máy tính

Cần Node.js và g++:

```
sh kiem_thu/chay_tat_ca.sh
```

## 6. Việc để sau

- Trình tự ưu tiên chi tiết theo báo cáo (các mốc T − 15 s, T − 10 s, bù 15 s xanh).
- Bộ quyết định mức 2 (chấm điểm phương án) và mức 3 (học máy).
- Nối web với board qua Firebase Realtime Database.
