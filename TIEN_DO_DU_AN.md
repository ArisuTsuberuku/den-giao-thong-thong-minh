# Tiến độ dự án: Đèn giao thông thông minh (NCKHKT 2026)

Cập nhật lần cuối: 2026-10-11 (bản đầu của web mô phỏng và hai firmware, thêm cắt xanh sớm; code ở repo GitHub `ArisuTsuberuku/den-giao-thong-thong-minh`; chưa thử trên board thật)

## Cách dùng file này

File này là "trí nhớ" của dự án. Khi đổi máy hoặc mở phiên làm việc mới, đưa file này cho Claude trước để tiếp tục đúng chỗ đang dừng. Mỗi lần chốt một quyết định hoặc xong một việc: sửa mục tương ứng, tích ô ở mục 10 và ghi một dòng vào mục 12.

Có ba bản giống nhau: một trong tài liệu của Project "Dự án NCKHKT 2026" trên Claude, một ở gốc thư mục dự án trên OneDrive, một trong thư mục code `den-giao-thong-thong-minh` nằm trong thư mục dự án (cũng là tên repo GitHub).

Kí hiệu trạng thái: `[ ]` chưa làm, `[~]` đang làm, `[x]` xong. Mục ghi "(đề xuất)" là ý của Claude, chưa được thầy xác nhận.

## 1. Tổng quan

Mô hình ngã tư 4 hướng trên sa bàn. Camera đếm và phân loại xe theo từng làn bằng TinyML, board trung tâm quyết định kế hoạch đèn theo lưu lượng và ưu tiên xe cứu thương.

- AI nhận diện, đếm xe chạy trên 4 camera.
- Board trung tâm ESP32-S3 N16R8 nhận số đếm, quyết định kế hoạch đèn cho từng chu kì và điều khiển 24 đèn.
- Sa bàn là mô hình thu nhỏ của ngã tư thật: thời gian đèn và thuật toán dùng giá trị thực tế, không rút gọn cho sa bàn.

## 2. Phần cứng và sa bàn

| Thành phần | Số lượng | Ghi chú |
|---|---|---|
| ESP32-S3-CAM có đế nạp, cảm biến OV2640 | 4 | Loại phổ biến, chân camera trùng kiểu ESP32-S3-EYE, LED RGB ở chân 48. Gắn trên trụ camera của khung đèn, mỗi camera đếm riêng 2 làn của 1 hướng |
| ESP32-S3 N16R8, loại 2 cổng USB-C | 1 | Trung tâm: nhận tin, quyết định kế hoạch, xuất đèn. Hàng chân có in số 38 và 48; LED RGB trên board ở chân 48 |
| Module đèn 3 màu (PCB 56 × 13 mm, LED 10 mm) | 8 | Hàng chân theo ảnh người bán: G, Y, R, GND. GND nối chung |
| Nguồn tổ ong 12 V 5 A | 1 | Cần thêm mạch hạ áp về 5 V (xem mục 9) |
| Xe thử nghiệm Hot Wheels | – | Bề rộng 40 mm do học sinh đo (chưa chắc; xe tỉ lệ ~1:64 thường rộng 28–35 mm); dài thường 65–75 mm, cần đo lại |

Sa bàn (bản mới nhất: Lần 08, file `Decal sa ban/tao_decal_sa_ban.py`), tấm 1000 × 1000 mm, tọa độ tính từ góc trên bên trái của bản in:

| Thông số | Giá trị |
|---|---|
| Bề rộng làn đường chính | 48 mm (xe 40 mm + 4 mm mỗi bên) |
| Dải phân cách cây xanh giữa | 10 mm |
| Lòng ngã tư | 202 × 202 mm |
| Mỗi hướng | 2 làn vào có đèn + 1 làn rẽ phải cắt góc (không đèn) + 2 làn ra |
| Làn ngoài | chỉ đi thẳng |
| Làn trong (sát dải phân cách) | đi thẳng hoặc rẽ trái |
| Vùng xe chờ | dài 304 mm (từ mép làn bao tới vạch dừng), rộng 96 mm (2 làn); đủ 4 xe mỗi làn nếu xe dài ≤ 70–72 mm |
| Đường bao ngoài | 1 làn 1 chiều rộng 52 mm, lề ngoài 6 mm, chạy theo chiều kim đồng hồ (xe ra và vào các nhánh đều chỉ rẽ phải) |

Khung đèn (bản mới nhất: Lần 08, file `Decal sa ban/Khung den/tao_khung_den.py`): cột đứng trên đảo dẫn hướng, bên phải người lái, ngay sau vạch dừng. Cần vươn cao 130 mm, dài 105 mm, mang 2 cụm đèn dựng đứng (đỏ trên cùng, vàng giữa, xanh dưới cùng), vỏ 28 × 64 mm, đặt đúng tim từng làn (cách nhau 48 mm), mũi tên in trên vỏ dưới đèn xanh: cụm mũi tên thẳng trên làn ngoài, cụm mũi tên rẽ trái trên làn trong. Khoảng hở dưới đèn 55 mm. Camera nằm trên trụ riêng chính giữa 2 cụm (đúng trên vạch chia 2 làn), có thanh giằng nối đỉnh cột; đường gấp tấm gá ở độ cao 271 mm (ống kính ~285 mm). Decal 2 trang A3 dọc, dưỡng cắt A3 dọc.

Cách gọi tên thống nhất với sa bàn: Hướng 1 (từ trên xuống) – cột Đ1, rồi theo chiều kim đồng hồ: Hướng 2 (từ phải) – Đ2, Hướng 3 (từ dưới) – Đ3, Hướng 4 (từ trái) – Đ4. "Dọc" = Hướng 1 và 3, "ngang" = Hướng 2 và 4.

## 3. Quyết định thiết kế

1. Truyền tin camera → trung tâm bằng ESP-NOW, nội dung JSON, kênh WiFi cố định, 3–5 gói mỗi giây.
2. Một firmware camera dùng chung cho 4 board, chỉ khác `CAM_ID` và thông số ROI.
3. Firmware trung tâm viết kiểu máy trạng thái với `millis()`, không dùng `delay()`.
4. Trung tâm nhận cùng định dạng JSON qua Serial để thử thuật toán khi chưa có camera.
5. Mọi lệnh bật đèn đi qua một hàm duy nhất có kiểm tra xung đột.
6. Xe ưu tiên nhận diện bằng hình ảnh (lớp `xe_uu_tien`), không dùng cảm biến âm thanh.
7. Môi trường lập trình dự kiến: Arduino IDE, thư viện ArduinoJson và thư viện xuất từ Edge Impulse.
8. Làn rẽ phải cắt góc chạy tự do, không có đèn, camera không đếm.
9. Đèn điều khiển theo hướng đi, không theo làn: cụm mũi tên thẳng cho mọi xe đi thẳng ở cả hai làn; cụm mũi tên rẽ trái cho xe rẽ trái (chỉ từ làn trong). Decal sa bàn và khung đèn hiện tại đã đúng với cách này, không cần sửa.
10. Thời gian đèn dùng giá trị thực tế theo báo cáo dự án: Tmin 15 s, Tmax 90 s, cộng 5 s cho mỗi 10 xe chờ.
11. Kế hoạch đèn chỉ đổi ở ranh giới chu kì. Chu kì đang chạy giữ nguyên tới hết (xanh, vàng, đỏ đủ như đã định); kế hoạch mới được tính trong lúc đó và áp dụng từ chu kì sau. Ngoại lệ: xe ưu tiên, lỗi an toàn và cắt xanh sớm (mục 21); mọi ngoại lệ vẫn phải qua vàng rồi đỏ toàn phần.
12. Board trung tâm ESP32-S3 N16R8 đảm nhận cả việc quyết định kế hoạch đèn; không cần thêm board.
13. (Đề xuất, đã cài thành tham số) Quy đổi trên sa bàn: 1 xe mô hình đại diện cho 10 xe thật, hằng số `XE_QUY_DOI`.
14. (Đề xuất, đã cài thành tham số) Hằng số `HE_SO_TUA_NHANH`, mặc định 1 = thời gian thực; tăng lên khi cần trình diễn nhanh.
15. Chế độ thích ứng chọn cả tổ hợp pha lẫn thời gian theo lưu lượng (mục 5.2), không gắn cứng với làn nào; chế độ cố định giữ thứ tự ở mục 5.1 để đối chứng. Thầy xác nhận 2026-10-11.
16. Rẽ trái không bao giờ được đi khi hướng ngược chiều đang chạy. Thầy chốt 2026-10-11; phương án vàng nhấp nháy nhường đường bị bỏ.
17. (Đề xuất) Bộ quyết định làm theo 3 mức, dùng chung một giao diện: vào là số đếm 8 làn, ra là kế hoạch một chu kì (mục 5.4). Mức 1 đã viết xong.
19. Web mô phỏng là mô phỏng thuần, chưa nối với board. Sau này nếu nối thì đi qua Firebase Realtime Database.
20. Camera gửi ESP-NOW kiểu phát chung (địa chỉ FF:FF:FF:FF:FF:FF) trên kênh cố định, nên không cần ghi địa chỉ MAC của board trung tâm vào code.
21. Cắt xanh sớm ở chế độ thích ứng (thầy yêu cầu 2026-10-11): tổ hợp đang xanh không thấy xe quá 5 s thì chuyển vàng, đỏ rồi cho xanh bước kế, không đợi hết thời gian đã định. Tham số `CAT_SOM`, đặt 0 để tắt. Chi tiết ở mục 5.2.
18. Sa bàn giữ tấm 1000 × 1000 mm và giữ đường bao; kích thước làn tối ưu theo xe Hot Wheels rộng 40 mm (Lần 06).

## 4. Giao thức JSON

```json
{"id":1,"seq":1042,"s":3,"l":1,"emg":0}
```

| Trường | Ý nghĩa |
|---|---|
| `id` | Số hướng 1–4, trùng với số cột Đ1–Đ4 |
| `seq` | Số thứ tự gói, để phát hiện mất gói |
| `s` | Số xe chờ ở làn ngoài (chỉ đi thẳng) |
| `l` | Số xe chờ ở làn trong (đi thẳng hoặc rẽ trái) |
| `emg` | Xe ưu tiên: 0 không có, 1 ở làn ngoài, 2 ở làn trong |

Camera gửi số xe đếm được trên sa bàn; việc nhân với `XE_QUY_DOI` làm ở board trung tâm.

Lọc nhiễu tại camera trước khi gửi: `s`, `l` lấy trung vị 5 khung gần nhất; `emg` chỉ bật khi 3/5 khung thấy xe ưu tiên.

## 5. Nhóm tín hiệu, pha và cách quyết định

8 nhóm tín hiệu: H1 thẳng, H1 trái, H2 thẳng, H2 trái, H3 thẳng, H3 trái, H4 thẳng, H4 trái.

### 5.1 Thứ tự cố định (đã chốt, dùng cho chế độ đối chứng)

| Thứ tự | Pha | Nhóm được xanh |
|---|---|---|
| 1 | Dọc rẽ trái | H1 trái + H3 trái |
| 2 | Ngang đi thẳng | H2 thẳng + H4 thẳng |
| 3 | Ngang rẽ trái | H2 trái + H4 trái |
| 4 | Dọc đi thẳng | H1 thẳng + H3 thẳng |

Chuyển pha: Xanh → Vàng → Đỏ toàn phần → pha kế tiếp.

### 5.2 Tổ hợp pha cho chế độ thích ứng (đã cài ở `quyet_dinh.h` và `quyet_dinh.js`)

Trong một trục có 4 tổ hợp an toàn (ví dụ trục dọc):

| Tổ hợp | Đèn xanh | Hợp khi |
|---|---|---|
| Thẳng cả hai | H1 thẳng + H3 thẳng | Hai hướng đều có xe đi thẳng |
| Trái cả hai | H1 trái + H3 trái | Cả hai hướng có xe chờ rẽ trái |
| Riêng Hướng 1 | H1 thẳng + H1 trái | Hướng 3 trống, hoặc chỉ Hướng 1 cần rẽ trái |
| Riêng Hướng 3 | H3 thẳng + H3 trái | Ngược lại |

Cấm tuyệt đối: trái xanh của một hướng cùng lúc với thẳng xanh của hướng đối diện; mọi tổ hợp xanh giữa hai trục.

Một chu kì thích ứng gồm khối trục dọc rồi khối trục ngang (cùng vòng lặp với thứ tự ở 5.1). Kế hoạch từng khối, chốt ở đầu chu kì theo số đếm lúc đó:

- Cả hai hướng trống: bỏ qua khối. Cả ngã tư trống thì luân phiên hai tổ hợp "thẳng cả hai" ở Tmin.
- Một hướng trống: chạy riêng hướng còn lại.
- Hai hướng đều có xe, làn trong đều trống: "thẳng cả hai".
- Còn lại, so hai phương án và chọn phương án tốn ít thời gian hơn:
  - "thẳng cả hai", sau đó rẽ trái có bảo vệ cho hướng nào có xe ở làn trong: cả hai hướng thì "trái cả hai", một hướng thì "riêng hướng đó" (đèn thẳng của hướng đó giữ xanh);
  - "riêng từng hướng", hướng đông đi trước.
- Hai phương án bằng nhau mà cả hai làn trong đều có xe: chọn "riêng từng hướng". Lí do: làn trong đi chung thẳng và trái, camera không biết xe nào rẽ; nếu thẳng và trái không xanh cùng lúc thì xe đầu làn chặn các xe phía sau. Mô phỏng cho thấy cách này giảm mạnh thời gian chờ khi đông xe.
- Camera của một hướng mất tín hiệu: trục đó chạy "thẳng cả hai" 30 s rồi "trái cả hai" 15 s.

Cắt xanh sớm trong lúc chu kì đang chạy (mục 21 của phần 3):

- Tổ hợp đang xanh không còn xe suốt `CAT_SOM` (5 s) và có xe đang chờ ở hướng khác: chuyển vàng ngay, sang bước kế. "Không còn xe" tính theo làn mà tổ hợp phục vụ: nhóm thẳng phục vụ cả hai làn, nhóm trái chỉ phục vụ làn trong.
- Khi sang bước kế (do cắt sớm hay do hết giờ), bước nào không còn xe thì bỏ qua; hết bước thì lập kế hoạch chu kì mới. Thứ tự các bước trong chu kì không đổi, nên không hướng nào bị bỏ đói.
- Không cắt khi camera của hướng đang xanh mất tín hiệu, hoặc khi không có xe nào chờ ở hướng khác (đèn giữ xanh, xe tới sau không phải dừng). Vì vậy khi cả ngã tư trống, trục dọc giữ xanh thay vì luân phiên.
- Chế độ cố định không cắt sớm.

Thời gian xanh mỗi bước = Tmin + 5 s × (số xe làn đông nhất của bước × `XE_QUY_DOI` / 10), giới hạn trong [Tmin, Tmax]. Bước đi thẳng tính cả làn trong vì xe làn trong cũng có thể đi thẳng.

Khi chuyển giữa hai tổ hợp, nhóm đèn có mặt ở cả hai thì giữ xanh; chỉ nhóm rời đi mới chuyển vàng rồi đỏ.

Vì kế hoạch chốt theo chu kì, xe rẽ trái còn sót lại sau chu kì này được tính vào số đếm của chu kì sau.

### 5.3 Rẽ trái nhường đường (đã bỏ)

Phương án cho rẽ trái nhường đường bằng vàng nhấp nháy đã bị bỏ ngày 2026-10-11: thầy chốt rẽ trái không được đi khi hướng ngược chiều đang chạy. Đèn rẽ trái chỉ có hai trường hợp: xanh trong "trái cả hai" hoặc "riêng một hướng", đỏ trong mọi trường hợp còn lại.

### 5.4 Bộ quyết định ở board trung tâm (đề xuất)

| Mức | Cách quyết định | Ghi chú |
|---|---|---|
| 1 | Bộ luật ở mục 5.2 | Làm trước, ở GĐ2. Là hệ luật, chưa phải học máy |
| 2 | Chấm điểm phương án: liệt kê các kế hoạch chu kì hợp lệ, ước tính tổng thời gian chờ của từng kế hoạch, chọn kế hoạch thấp nhất | Đúng nghĩa "thuật toán tối ưu hóa" trong tên đề tài |
| 3 | Mô hình học máy nhỏ huấn luyện từ dữ liệu mô phỏng trên máy tính | Tùy chọn, nếu còn thời gian |

Ở mọi mức, kết quả phải đi qua lớp an toàn: giới hạn Tmin/Tmax, kiểm tra xung đột, vàng và đỏ toàn phần.

### 5.5 Ưu tiên xe cứu thương

Khi `emg` khác 0: kết thúc bước hiện tại đúng trình tự vàng, đỏ toàn phần, rồi bật "riêng hướng đó"; xe đi qua thì lập kế hoạch chu kì mới. Bản đã cài: bước đang chạy phải xanh ít nhất 5 s mới bị cắt; giữ xanh ưu tiên tới khi không còn thấy xe 5 s, tối đa 60 s; quá 60 s thì khóa hướng đó cho tới khi `emg` về 0. Trình tự chi tiết theo báo cáo (mốc T − 15 s, T − 10 s, T, T + 5 s, bù 15 s xanh) làm ở GĐ5.

Hạn chế đã biết của làn chung: xe rẽ trái đứng đầu làn trong chặn xe đi thẳng phía sau cho tới khi được rẽ; tổ hợp "riêng một hướng" (thẳng và trái cùng xanh) tránh được việc này.

Các ý khác trong báo cáo để dành cho GĐ6: "xả cân bằng" chia thời gian theo tỉ lệ hàng đợi; giảm xanh cho làn có xe chết máy.

## 6. Tham số thời gian (giá trị thực tế)

| Tham số | Giá trị | Nguồn |
|---|---|---|
| Tmin | 15 s | Báo cáo dự án |
| Tmax | 90 s | Báo cáo dự án |
| Cộng thêm | 5 s mỗi 10 xe chờ | Báo cáo dự án |
| Quy đổi `XE_QUY_DOI` | 1 xe mô hình = 10 xe thật | Đề xuất |
| Vàng | 3 s | Đề xuất, theo mức thường dùng |
| Đỏ toàn phần | 2 s | Đề xuất |
| Chế độ cố định (đối chứng) | đi thẳng 30 s, rẽ trái 15 s, chu kì 110 s | Đề xuất |
| `HE_SO_TUA_NHANH` | 1 | Đề xuất |
| Camera mất tín hiệu sau | 3 s không nhận gói → từ chu kì sau hướng đó dùng thời gian cố định | Đề xuất |
| Cắt xanh sớm `CAT_SOM` | hết xe quá 5 s | Thầy yêu cầu 2026-10-11 |
| Ưu tiên tự hủy sau | 60 s | Đề xuất |
| Xanh ít nhất trước khi bị cắt để ưu tiên | 5 s | Đề xuất |
| Giữ xanh sau khi hết thấy xe ưu tiên | 5 s | Đề xuất |

Với quy đổi trên, mỗi xe mô hình cộng 5 s: 0 xe → 15 s, 1 xe → 20 s, 2 xe → 25 s, 3 xe → 30 s, 4 xe (đầy làn từ Lần 08) → 35 s. Mức Tmax 90 s chỉ đạt khi có 150 xe thật, tức không xảy ra trên sa bàn.

Bảng dưới lập khi mỗi làn tối đa 3 xe. Với sa bàn Lần 08 (4 xe mỗi làn), trường hợp đông nhất là xanh 35 s mỗi pha, chu kì 4 × (35 + 3 + 2) = 160 s.

Thời gian từng nhóm đèn theo thứ tự cố định ở 5.1 (xanh / vàng / đỏ, tính bằng giây). Đỏ = chu kì − xanh − vàng. Chế độ thích ứng đã cài dùng tổ hợp ở 5.2 nên thứ tự bước khác bảng này, nhưng độ dài chu kì của các ví dụ vẫn đúng: 40, 90 và 160 s.

| Nhóm đèn | Không có xe | Cố định (đối chứng) | Ví dụ dọc đông, ngang vắng | Đông nhất (3 xe mọi làn) |
|---|---|---|---|---|
| Dọc rẽ trái | đỏ suốt | 15 / 3 / 92 | 25 / 3 / 62 | 30 / 3 / 107 |
| Ngang đi thẳng | 15 / 3 / 22 | 30 / 3 / 77 | 20 / 3 / 67 | 30 / 3 / 107 |
| Ngang rẽ trái | đỏ suốt | 15 / 3 / 92 | bỏ qua, đỏ suốt | 30 / 3 / 107 |
| Dọc đi thẳng | 15 / 3 / 22 | 30 / 3 / 77 | 30 / 3 / 57 | 30 / 3 / 107 |
| Chu kì | 40 | 110 | 90 | 140 |

Ví dụ "dọc đông, ngang vắng": Hướng 1 có 3 xe làn ngoài, 2 xe làn trong chờ rẽ trái; Hướng 3 có 2 và 1; Hướng 2 có 1 xe làn ngoài; Hướng 4 không có xe.

Ví dụ lợi ích của "riêng một hướng": Hướng 1 có 3 xe làn ngoài và 2 xe rẽ trái, Hướng 3 trống. Thứ tự cố định chiếm 65 s của trục dọc; "riêng Hướng 1" chỉ chiếm 35 s.

Cần kiểm chứng: hệ số 5 s mỗi 10 xe là giả định trong báo cáo mô phỏng. Một ô tô cần khoảng 2 s để qua vạch dừng trên một làn, nên hệ số này hợp với dòng xe máy hơn dòng ô tô. Đã để thành tham số để chỉnh sau.

## 7. Gán chân GPIO board trung tâm (đã đưa vào `cau_hinh.h`, chưa kiểm trên board thật)

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

Chân phải tránh: 0, 3, 45, 46 (strapping), 19, 20 (USB), 35–37 (PSRAM của bản R8), 43, 44 (UART0). Theo ảnh board thầy gửi (hàng chân có cả 38 và 48), LED RGB ở chân 48 và firmware dùng nó làm đèn báo trạng thái; chân 38 dùng cho H4 trái. Nếu khi thử thấy LED RGB sáng theo đèn H4 trái thì board dùng 38 cho LED: đổi H4 trái sang chân khác và tắt `DUNG_LED_BAO`.

Đã dùng hết 24 chân an toàn. Nếu cần thêm nút bấm, còi, màn hình thì chuyển sang 74HC595 hoặc MCP23017.

Dây cho mỗi cột: 8 sợi (6 tín hiệu đèn + 5 V cho camera + GND).

Cần thử ở GĐ1: module đèn thiết kế cho 5 V, chân ESP32 chỉ ra 3,3 V nên đèn (nhất là xanh) có thể mờ. Nếu mờ, thêm 3 con 74HCT245 cấp 5 V làm tầng đệm.

## 8. Camera: vị trí, vùng quan sát và chia làn

Vị trí theo khung đèn Lần 06: trụ camera nằm giữa 2 cụm đèn, đúng trên vạch chia 2 làn; khung đèn đứng khoảng 15 mm sau vạch dừng (phía ngã tư). Sau khi bẻ tấm gá, ống kính chồm về phía xe tới khoảng 29 mm nên nằm gần như ngay trên vạch dừng, cao khoảng 285 mm, nhìn ngược về phía xe đang tới.

Góc bẻ tấm gá: khoảng 64° so với phương đứng (trục ống kính chếch khoảng 26° so với phương thẳng đứng). Tính với ống kính thường, góc nhìn khoảng 55° × 43°: vùng nhìn dọc đường từ khoảng 20 mm sau vạch dừng tới hết vùng chờ 304 mm; bề ngang khung hình rộng hơn nhiều so với 2 làn (96 mm). Cần kiểm lại bằng ảnh thật; nếu dùng ống kính góc rộng thì bẻ ít hơn.

Xử lí ảnh:

- Chụp ở 320 × 240, cắt một hình chữ nhật ROI chỉ chứa 2 làn chờ, rồi co về kích thước đầu vào mô hình (bắt đầu với 96 × 96).
- Ảnh huấn luyện phải đi qua đúng bước cắt này, chụp bằng chính camera đã gắn cố định trên sa bàn.
- Sau khi mô hình trả về tâm từng xe, gán xe vào làn bằng 2 tứ giác (làn ngoài, làn trong) khai báo trong cấu hình. Tâm nằm ngoài cả hai tứ giác thì bỏ qua: làn rẽ phải, 2 làn ra bên kia dải phân cách, đường bao ngoài.
- Ánh sáng đèn tín hiệu ngay dưới camera hắt lên xe ở vạch dừng và đổi màu theo pha, nên ảnh huấn luyện phải có đủ cả lúc đèn đỏ, vàng, xanh.
- Thông số ROI và 2 tứ giác lưu riêng cho từng camera.

## 9. Nguồn

- Núm chỉnh trên nguồn tổ ong 12 V chỉ chỉnh được khoảng ±10 %, không xuống 5 V được.
- Dùng mạch hạ áp DC-DC 12 V → 5 V, loại chịu dòng 5 A (ví dụ XL4015). Chỉnh và đo đúng 5,0–5,1 V bằng đồng hồ trước khi nối vào board.
- Cấp vào chân 5V của từng board, đi dây hình sao từ mạch hạ áp. Thêm tụ 470–1000 µF gần mỗi camera.
- GND của mạch hạ áp, 5 board và 8 module đèn nối chung.
- Cọc 220 V của nguồn tổ ong phải che kín; phần đấu điện lưới do giáo viên làm.

## 10. Lộ trình và trạng thái

- [x] Lập phương án tổng thể (2026-10-09)
- [x] Đối chiếu phương án với sa bàn Lần 04 và khung đèn Lần 05 (2026-10-09)
- [x] Chốt cách chia pha, thứ tự pha cố định (2026-10-09)
- [x] Đổi tham số thời gian sang giá trị thực tế (2026-10-09)
- [x] Chốt nguyên tắc chỉ đổi kế hoạch ở cuối chu kì (2026-10-09)
- [x] Tối ưu sa bàn và khung đèn theo xe Hot Wheels rộng 40 mm – Lần 06 (2026-10-09)
- [x] Module đèn dựng đứng, đỏ trên – xanh dưới – Lần 07 (2026-10-09)
- [x] Thu hẹp làn 48 mm, vùng chờ 4 xe – Lần 08 (2026-10-09)
- [x] Viết bản đầu của web mô phỏng, firmware trung tâm (GĐ1, GĐ2, ưu tiên dạng đơn giản) và firmware camera 3 chế độ; đã kiểm tra trên máy tính, chưa thử trên board (2026-10-11)
- [ ] GĐ0. Chuẩn bị: mạch hạ áp 5 V, lắp 1 khung đèn mẫu có camera, bẻ tấm gá ~64° và kiểm tra vùng nhìn bằng ảnh thật
- [ ] GĐ1. Trung tâm chạy thứ tự cố định, chu kì 110 s. Xong khi: 24 đèn đúng thứ tự, không xung đột, đèn đủ sáng
- [ ] GĐ2. Bộ quyết định mức 1 với JSON gõ qua Serial. Xong khi: đổi `s`, `l` thì kế hoạch chu kì sau đổi theo, chu kì đang chạy không bị ảnh hưởng
- [ ] GĐ3. Một camera: cắt ROI, thu ảnh, huấn luyện, đếm riêng 2 làn, in ra Serial. Xong khi: đếm đúng 0–4 xe mỗi làn trên 90 % lần thử
- [ ] GĐ4. Camera gửi ESP-NOW; ghép 1 rồi 4 camera. Xong khi: trung tâm nhận đủ 4 hướng, mất gói dưới 5 %
- [ ] GĐ5. Xe ưu tiên. Xong khi: đặt xe cứu thương vào thì hướng đó được xanh và hệ thống tự trở lại bình thường
- [ ] GĐ6. Cơ chế an toàn và đo đạc. Xong khi: rút nguồn 1 camera hệ thống vẫn chạy; có số liệu so sánh chế độ cố định và thích ứng trên sa bàn
- [ ] GĐ7 (tùy chọn). Bộ quyết định mức 2, rồi mức 3 nếu còn thời gian

GĐ1–2 và GĐ3 có thể làm song song bởi hai nhóm học sinh.

## 11. Việc đang mở

1. Xác nhận các đề xuất còn lại ở mục 3: quy đổi xe (13), tua nhanh (14), ba mức của bộ quyết định (17). Mục 15 và 16 đã chốt.
2. Học sinh đo lại 3–4 xe Hot Wheels sẽ dùng (rộng nhất kể cả bánh, dài từ đầu tới đuôi): vùng chờ 304 mm đủ 4 xe nếu xe dài ≤ 70–72 mm; xe 75 mm thì xe thứ 4 lấn ~2 cm ra làn bao.
3. Ống kính camera là loại thường hay góc rộng; đo board thật để chỉnh ô ống kính trên tấm gá. Nếu là ống kính góc rộng (~120°) thì có thể hạ camera xuống khoảng 150–170 mm. (Loại board đã rõ: ESP32-S3-CAM có đế nạp.)
4. Thử độ sáng module đèn ở 3,3 V.
5. Mua mạch hạ áp 5 V 5 A.
6. Báo cáo ghi mỗi hướng có "1 làn đi thẳng/rẽ phải, 1 làn rẽ trái" và dùng cảm biến âm thanh; sa bàn thực tế là làn ngoài đi thẳng, làn trong thẳng hoặc rẽ trái, rẽ phải có làn riêng, không có cảm biến âm thanh. Cần sửa báo cáo cho khớp.
7. Thư mục `Decal sa ban/Khung den` còn tệp cũ `Duong_cat_khung_den_A4.pdf` (Lần 05); dưỡng cắt mới là `Duong_cat_khung_den_A3.pdf`. Có thể xóa tệp A4 cũ (bản lưu vẫn còn trong `Nhật ký thiết kế/Lần 05`).
8. Thử code trên board thật theo danh sách trong `README.md` của thư mục code: 24 bóng đèn (lệnh `thuday`), chu kì cố định, bản tin gõ qua Serial, camera ở chế độ thu ảnh.
9. Mô phỏng (có trễ phản ứng của người lái 1 s): chế độ thích ứng có cắt sớm hơn chế độ cố định ở 5 trong 6 kiểu dòng xe, nhưng thua 24 % khi rất ít xe rẽ trái, vì camera không biết xe làn trong đi thẳng hay rẽ nên bộ quyết định mở riêng từng hướng. Hướng cải tiến: bộ quyết định mức 2, hoặc cho "thẳng cả hai" khi làn trong thưa. Cần kiểm lại trên sa bàn thật, nhất là việc camera đếm về 0 có đủ ổn định để cắt sớm không.

## 12. Nhật ký thay đổi

| Ngày | Nội dung |
|---|---|
| 2026-10-09 | Chốt kiến trúc: 4 camera ESP32-S3 + 1 ESP32-S3 N16R8 trung tâm, ESP-NOW + JSON |
| 2026-10-09 | Xác nhận mỗi hướng 2 làn (thẳng, rẽ trái) → 8 nhóm tín hiệu độc lập, 4 pha, JSON thêm `s`, `l` |
| 2026-10-09 | Nguồn: dùng nguồn tổ ong 12 V 5 A qua mạch hạ áp 5 V |
| 2026-10-09 | Đối chiếu sa bàn Lần 04 và khung đèn Lần 05: đổi tên hướng thành Hướng 1–4 / Đ1–Đ4; vùng chờ 203 × 124 mm, khoảng 3 xe mỗi làn; đề xuất bẻ gá camera 55–60° |
| 2026-10-09 | Chốt: đèn theo hướng đi (làn trong đi thẳng hoặc rẽ trái), decal và khung đèn giữ nguyên; thứ tự pha dọc trái → ngang thẳng → ngang trái → dọc thẳng |
| 2026-10-09 | Đổi thời gian đèn sang giá trị thực tế theo báo cáo (Tmin 15 s, Tmax 90 s, +5 s mỗi 10 xe), vàng 3 s, đỏ toàn phần 2 s; thêm quy đổi xe và hằng số tua nhanh |
| 2026-10-09 | Thầy nêu thêm phương án: chạy riêng từng hướng, mở đồng thời thẳng và trái, cho cắt ngang như thực tế → Claude đề xuất 4 tổ hợp pha an toàn và rẽ trái nhường đường bằng vàng nhấp nháy (chờ xác nhận) |
| 2026-10-09 | Chốt: board trung tâm quyết định kế hoạch đèn; kế hoạch chỉ đổi ở cuối chu kì, không đổi đột ngột. Đề xuất bộ quyết định 3 mức |
| 2026-10-09 | Lần 06: tối ưu theo xe Hot Wheels rộng 40 mm, giữ tấm 1 m và đường bao. Làn 56 mm, dải cây xanh 16 mm, đường bao thành 1 làn 1 chiều 60 mm (chiều kim đồng hồ), lòng ngã tư 240 mm, vùng chờ 265 × 112 mm (từ 203). Khung đèn: 2 cụm cách 62 mm, camera trên trụ riêng có thanh giằng, ống kính cao ~240 mm, bẻ ~64° (thay đề xuất 55–60° cũ) |
| 2026-10-09 | Lần 07: module đèn dựng đứng (đỏ trên – xanh dưới), vỏ 28 × 64 mm đặt đúng tim làn 56 mm, cần vươn 117 mm. Đánh giá: thu hẹp làn xuống 48 mm chỉ thêm 16 mm vùng chờ, không đủ thêm 1 xe → giữ 56 mm. Camera giữ ~240 mm vì ống kính thường chỉ nhìn dọc ~43° |
| 2026-10-09 | Lần 08: thu hẹp để vùng chờ chứa 4 xe: làn 48 mm, dải cây xanh 10 mm, làn bao 52 mm, lòng ngã tư 202 mm, vùng chờ 304 × 96 mm. Khung đèn: cụm đứng cách 48 mm, cần vươn 105 mm, ống kính ~285 mm. Thông tin xe: Hot Wheels ~1:64 dài 65–75 mm; bề rộng 40 mm học sinh đo cần kiểm lại. Số xe tối đa mỗi làn đổi từ 3 lên 4 |
| 2026-10-11 | Thầy chốt: tổ hợp pha thay đổi theo lưu lượng; rẽ trái không được đi khi hướng ngược chiều đang chạy (bỏ vàng nhấp nháy); web là mô phỏng thuần, sau này nối board qua Firebase RTDB. Board: ESP32-S3-CAM có đế nạp và ESP32-S3 N16R8 loại 2 cổng USB-C |
| 2026-10-11 | Viết bản đầu: web mô phỏng (nền là decal Lần 08), firmware trung tâm, firmware camera 3 chế độ (thu ảnh, đếm, giả lập). Bộ quyết định viết hai bản C++ và JavaScript, so khớp nhau trên 760 000 dòng trạng thái. Biên dịch được với lõi esp32 2.0.9 và 3.3.12. Chưa thử trên board thật |
| 2026-10-11 | Đổi luật 5.2: so thời gian hai phương án; bằng nhau và cả hai làn trong có xe thì chọn riêng từng hướng. Mô phỏng 60 phút: thích ứng giảm thời gian chờ 25–95 % ở 4 kiểu dòng xe, ngang bằng khi dòng xe đều, tăng 57 % khi rất ít xe rẽ trái |
| 2026-10-11 | Thầy tạo repo GitHub `ArisuTsuberuku/den-giao-thong-thong-minh`; đã đẩy code lên nhánh `main` |
| 2026-10-11 | Thêm cắt xanh sớm theo yêu cầu của thầy: tổ hợp đang xanh hết xe quá 5 s mà hướng khác có xe chờ thì chuyển vàng ngay; bước kế không còn xe thì bỏ qua. Mô phỏng 60 phút: thời gian chờ trung bình của chế độ thích ứng giảm thêm 17–70 % so với khi chưa cắt sớm, và thấp hơn chế độ cố định 10–98 % ở cả 6 kiểu dòng xe |
| 2026-10-11 | Web mô phỏng thêm trễ phản ứng của người lái (mặc định 1 s, mỗi người lái lệch 0,7–1,3 lần): xe trong hàng chờ khởi hành lần lượt, xả hàng khoảng 2,1 s mỗi xe. Chạy lại bảng so sánh 60 phút: thích ứng có cắt sớm thấp hơn cố định 45–99 % ở 5 kiểu dòng xe, cao hơn 24 % ở kiểu rất ít rẽ trái (số ở dòng trên là khi chưa có trễ phản ứng) |
