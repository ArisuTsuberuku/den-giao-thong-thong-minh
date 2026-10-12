# Tiến độ dự án: Đèn giao thông thông minh (NCKHKT 2026)

Cập nhật lần cuối: 2026-10-12 (camera trong mô phỏng làm đúng như ESP32-S3-CAM thật: góc nhìn OV2640 từ khung đèn, cắt ROI, nhận diện kiểu FOMO 96 × 96, 1 khung mỗi giây; ngày 2026-10-11: viết lại chế độ tự học, chốt kế hoạch theo lượt, ba kiểu chia pha, bốn loại xe ưu tiên, cỡ xe trong mô phỏng; code ở repo GitHub `ArisuTsuberuku/den-giao-thong-thong-minh`; chưa thử trên board thật)

## Cách dùng file này

File này là "trí nhớ" của dự án. Khi đổi máy hoặc mở phiên làm việc mới, đưa file này cho Claude trước để tiếp tục đúng chỗ đang dừng. Mỗi lần chốt một quyết định hoặc xong một việc: sửa mục tương ứng, tích ô ở mục 10 và ghi một dòng vào mục 12.

Có ba bản giống nhau: một trong tài liệu của Project "Dự án NCKHKT 2026" trên Claude, một ở gốc thư mục dự án trên OneDrive, một trong thư mục code `den-giao-thong-thong-minh` nằm trong thư mục dự án (cũng là tên repo GitHub).

Kí hiệu trạng thái: `[ ]` chưa làm, `[~]` đang làm, `[x]` xong. Mục ghi "(đề xuất)" là ý của Claude, chưa được thầy xác nhận.

## 1. Tổng quan

Mô hình ngã tư 4 hướng trên sa bàn. Camera đếm và phân loại xe theo từng làn bằng TinyML, board trung tâm quyết định kế hoạch đèn theo lưu lượng và mở đèn cho xe ưu tiên (cứu hỏa, quân sự, công an, cứu thương).

- AI nhận diện, đếm xe chạy trên 4 camera.
- Board trung tâm ESP32-S3 N16R8 nhận số đếm, quyết định kế hoạch đèn cho từng lượt và điều khiển 24 đèn.
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

1. Truyền tin camera → trung tâm bằng ESP-NOW, nội dung JSON, kênh WiFi cố định. Camera gửi ngay sau mỗi khung xử lí xong, không dày hơn 4 gói mỗi giây; ở 1 khung/giây là 1 gói mỗi giây.
2. Một firmware camera dùng chung cho 4 board, chỉ khác `CAM_ID` và thông số ROI.
3. Firmware trung tâm viết kiểu máy trạng thái với `millis()`, không dùng `delay()`.
4. Trung tâm nhận cùng định dạng JSON qua Serial để thử thuật toán khi chưa có camera.
5. Mọi lệnh bật đèn đi qua một hàm duy nhất có kiểm tra xung đột.
6. Xe ưu tiên nhận diện bằng hình ảnh, không dùng cảm biến âm thanh. Bốn lớp: `xe_cuu_hoa`, `xe_quan_su`, `xe_cong_an`, `xe_cuu_thuong` (mục 24); nếu mô hình không phân biệt nổi thì gộp thành một lớp `xe_uu_tien`.
7. Môi trường lập trình dự kiến: Arduino IDE, thư viện ArduinoJson và thư viện xuất từ Edge Impulse.
8. Làn rẽ phải cắt góc chạy tự do, không có đèn, camera không đếm.
9. Đèn điều khiển theo hướng đi, không theo làn: cụm mũi tên thẳng cho mọi xe đi thẳng ở cả hai làn; cụm mũi tên rẽ trái cho xe rẽ trái (chỉ từ làn trong). Decal sa bàn và khung đèn hiện tại đã đúng với cách này, không cần sửa.
10. Thời gian đèn dùng giá trị thực tế theo báo cáo dự án: Tmin 15 s, Tmax 90 s, cộng 5 s cho mỗi 10 xe chờ.
11. Kế hoạch đèn chỉ đổi ở ranh giới giữa hai lần chốt, không đổi giữa chừng. Chế độ cố định chốt cả chu kì; chế độ thích ứng chốt từng lượt, mỗi lượt là phần của một trục (mục 22). Ngoại lệ: xe ưu tiên, lỗi an toàn và cắt xanh sớm (mục 21); mọi ngoại lệ vẫn phải qua vàng rồi đỏ toàn phần.
12. Board trung tâm ESP32-S3 N16R8 đảm nhận cả việc quyết định kế hoạch đèn; không cần thêm board.
13. (Đề xuất, đã cài thành tham số) Quy đổi trên sa bàn: 1 xe mô hình đại diện cho 10 xe thật, hằng số `XE_QUY_DOI`.
14. (Đề xuất, đã cài thành tham số) Hằng số `HE_SO_TUA_NHANH`, mặc định 1 = thời gian thực; tăng lên khi cần trình diễn nhanh.
15. Chế độ thích ứng chọn cả tổ hợp pha lẫn thời gian theo lưu lượng (mục 5.2), không gắn cứng với làn nào; chế độ cố định giữ thứ tự ở mục 5.1 để đối chứng. Thầy xác nhận 2026-10-11.
16. Rẽ trái không bao giờ được đi khi hướng ngược chiều đang chạy. Thầy chốt 2026-10-11; phương án vàng nhấp nháy nhường đường bị bỏ.
17. (Đề xuất) Bộ quyết định làm theo 3 mức, dùng chung một giao diện: vào là số đếm 8 làn, ra là kế hoạch một lượt (mục 5.4). Mức 1 đã viết xong; mức 2 làm theo cách tự học ở mục 23.
19. Web mô phỏng là mô phỏng thuần, chưa nối với board. Sau này nếu nối thì đi qua Firebase Realtime Database.
20. Camera gửi ESP-NOW kiểu phát chung (địa chỉ FF:FF:FF:FF:FF:FF) trên kênh cố định, nên không cần ghi địa chỉ MAC của board trung tâm vào code.
21. Cắt xanh sớm ở chế độ thích ứng (thầy yêu cầu 2026-10-11): tổ hợp đang xanh không thấy xe quá 5 s thì chuyển vàng, đỏ rồi cho xanh bước kế, không đợi hết thời gian đã định. Tham số `CAT_SOM`, đặt 0 để tắt. Chi tiết ở mục 5.2.
18. Sa bàn giữ tấm 1000 × 1000 mm và giữ đường bao; kích thước làn tối ưu theo xe Hot Wheels rộng 40 mm (Lần 06).
22. (Đề xuất, đã cài; đổi so với mục 11 cũ là "chỉ đổi ở cuối chu kì", chờ thầy xác nhận) Chế độ thích ứng chốt kế hoạch theo từng lượt (2026-10-11): một lượt là phần của một trục, chốt theo số đếm ngay lúc lượt bắt đầu, hết lượt thì tới trục kia. Mô phỏng giờ cao điểm: thời gian chờ 169 s (chốt cả vòng) xuống 81 s (chốt theo lượt).
23. Chế độ thích ứng có ba kiểu chia pha cho trường hợp cả hai hướng của trục đều có xe và làn trong có xe: "riêng từng hướng", "thẳng trước", "đối xứng", và chế độ "tự học" tự chọn giữa hai kiểu đầu cho từng trục bằng cách chạy thử rồi đo (thầy yêu cầu 2026-10-11). Chi tiết ở mục 5.2 và 5.6. Web mặc định tự học; firmware mặc định "riêng", đổi bằng lệnh `kieu`.
24. Bốn loại xe ưu tiên, mở đèn theo thứ tự cứu hỏa → quân sự → công an → cứu thương (thầy yêu cầu 2026-10-11). Bản tin JSON thêm trường `ut`. Lưu ý: Điều 27 Luật Trật tự, an toàn giao thông đường bộ 2024 xếp xe quân sự và xe công an cùng một hạng; thứ tự "sự" trước "công" là theo câu dễ nhớ, chờ thầy quyết giữ hay cho cùng hạng.
25. Xe ưu tiên qua khỏi vùng camera 1 s là đèn chuyển vàng ngay, không giữ xanh thêm, để các hướng khác không phải chờ lâu (thầy yêu cầu 2026-10-11). Tham số `UT_GIU`, trước đó là 5 s.
26. Web mô phỏng vẽ xe ở cỡ 75 % (51 × 26 mm, 5 xe mỗi làn chờ) cho giống tỉ lệ ô tô với làn đường thật và không che chỗ rẽ; chọn được 60 %, 75 %, 100 %. Cỡ 100 % là xe Hot Wheels trên sa bàn thật (4 xe mỗi làn).
27. (Đề xuất, đã cài 2026-10-12) Camera trong mô phỏng làm đúng như ESP32-S3-CAM thật (thầy yêu cầu): khung 320 × 240 chiếu phối cảnh từ ống kính trên khung đèn, cắt ROI rồi co về 96 × 96, nhận diện kiểu FOMO lưới 12 × 12 có gộp ô, 1 khung mỗi giây, lọc 3 khung. Chi tiết và kết quả ở mục 8. Camera lí tưởng giữ lại để so sánh.
28. (Đề xuất) Gắn board camera sao cho cạnh dài của khung hình nằm dọc theo đường; gắn ngược lại thì không thấy xe đứng sát vạch dừng (mục 8).
29. (Đề xuất) Lọc nhiễu ở camera qua 3 khung (trước là 5): ở 1 khung/giây, 5 khung làm trễ thêm 1 s mà không bớt nhầm được bao nhiêu; không lọc thì hay mở ưu tiên nhầm. Tham số `SO_KHUNG_LOC`.

## 4. Giao thức JSON

```json
{"id":1,"seq":1042,"s":3,"l":1,"emg":1,"ut":1}
```

| Trường | Ý nghĩa |
|---|---|
| `id` | Số hướng 1–4, trùng với số cột Đ1–Đ4 |
| `seq` | Số thứ tự gói, để phát hiện mất gói |
| `s` | Số xe chờ ở làn ngoài (chỉ đi thẳng) |
| `l` | Số xe chờ ở làn trong (đi thẳng hoặc rẽ trái) |
| `emg` | Xe ưu tiên: 0 không có, 1 ở làn ngoài, 2 ở làn trong |
| `ut` | Loại xe ưu tiên: 0 không có, 1 cứu hỏa, 2 quân sự, 3 công an, 4 cứu thương. Số nhỏ hơn được mở đèn trước. Bản tin kiểu cũ không có `ut` mà `emg` khác 0 thì trung tâm coi là xe cứu thương |

Camera gửi số xe đếm được trên sa bàn; việc nhân với `XE_QUY_DOI` làm ở board trung tâm. Trong vùng có nhiều xe ưu tiên thì camera báo xe hạng cao nhất.

Lọc nhiễu tại camera trước khi gửi (`SO_KHUNG_LOC`, mặc định 3): `s`, `l` lấy trung vị 3 khung gần nhất; `emg` chỉ bật khi 2/3 khung thấy xe ưu tiên; `ut` là loại xuất hiện ở nhiều khung nhất trong các khung đó.

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

Chế độ thích ứng chạy theo từng lượt: lượt trục dọc, rồi lượt trục ngang, cứ thế luân phiên. Kế hoạch của một lượt chốt theo số đếm ngay lúc lượt đó bắt đầu:

- Trục tới lượt không có xe: lập luôn cho trục kia. Cả ngã tư trống: trục tới lượt chạy "thẳng cả hai" ở Tmin.
- Một hướng trống: chạy riêng hướng còn lại.
- Hai hướng đều có xe, làn trong đều trống: "thẳng cả hai".
- Còn lại thì theo kiểu chia pha của trục đó:

| Kiểu | Các bước của một lượt | Hợp khi (theo mô phỏng) |
|---|---|---|
| Riêng từng hướng (`KIEU_RIENG`) | Riêng hướng đông hơn, rồi riêng hướng kia | Từ khoảng 15 % xe rẽ trái trở lên. Không bao giờ kẹt vì xe rẽ trái |
| Thẳng trước (`KIEU_THANG_TRUOC`) | "Thẳng cả hai"; hai làn ngoài hết xe thì riêng hướng có làn trong đông hơn, rồi riêng hướng kia nếu làn trong còn xe | Dưới khoảng 10–12 % xe rẽ trái; lợi càng nhiều khi đường càng đông |
| Đối xứng (`KIEU_DOI_XUNG`) | "Thẳng cả hai", rồi "trái cả hai" (hoặc riêng một hướng nếu chỉ hướng đó có xe ở làn trong) | Chỉ khi gần như không có xe rẽ trái. Từ 5 % rẽ trái đã chờ gấp đôi kiểu riêng, từ 25 % thì kẹt cứng |
| Tự học (`KIEU_TU_HOC`) | Tự chọn "riêng" hoặc "thẳng trước" cho từng trục (mục 5.6) | Khi không biết trước tỉ lệ rẽ trái, hoặc hai trục khác nhau |

- Lí do kiểu đối xứng kém: làn trong đi chung thẳng và trái, camera không biết xe nào rẽ. Trong bước "thẳng cả hai", xe rẽ trái đứng đầu làn chặn xe đi thẳng phía sau; trong bước "trái cả hai", xe đi thẳng đứng đầu lại chặn xe rẽ trái.
- Camera của một hướng mất tín hiệu: trục đó chạy "thẳng cả hai" 30 s rồi "trái cả hai" 15 s.

Cắt xanh sớm trong lúc lượt đang chạy (mục 21 của phần 3):

- Tổ hợp đang xanh không còn xe suốt `CAT_SOM` (5 s) và có xe đang chờ ở hướng khác: chuyển vàng ngay, sang bước kế. "Không còn xe" tính theo làn mà tổ hợp phục vụ: nhóm thẳng phục vụ cả hai làn, nhóm trái chỉ phục vụ làn trong. Riêng bước "thẳng cả hai" của kiểu thẳng trước chỉ tính hai làn ngoài, vì xe làn trong còn lại đã có bước mở riêng lo.
- Khi sang bước kế (do cắt sớm hay do hết giờ), bước nào không còn xe thì bỏ qua; hết bước thì chốt lượt mới cho trục kia. Hai trục luân phiên nên không hướng nào bị bỏ đói.
- Không cắt khi camera của hướng đang xanh mất tín hiệu, hoặc khi không có xe nào chờ ở hướng khác (đèn giữ xanh, xe tới sau không phải dừng).
- Chế độ cố định không cắt sớm.

Thời gian xanh mỗi bước = Tmin + 5 s × (số xe làn đông nhất mà bước phục vụ × `XE_QUY_DOI` / 10), giới hạn trong [Tmin, Tmax]. Bước riêng một hướng tính cả hai làn của hướng đó; bước "thẳng cả hai" của kiểu thẳng trước tính hai làn ngoài; bước mở riêng sau nó tính làn trong.

Khi chuyển giữa hai tổ hợp, nhóm đèn có mặt ở cả hai thì giữ xanh; chỉ nhóm rời đi mới chuyển vàng rồi đỏ.

### 5.3 Rẽ trái nhường đường (đã bỏ)

Phương án cho rẽ trái nhường đường bằng vàng nhấp nháy đã bị bỏ ngày 2026-10-11: thầy chốt rẽ trái không được đi khi hướng ngược chiều đang chạy. Đèn rẽ trái chỉ có hai trường hợp: xanh trong "trái cả hai" hoặc "riêng một hướng", đỏ trong mọi trường hợp còn lại.

### 5.4 Bộ quyết định ở board trung tâm (đề xuất)

| Mức | Cách quyết định | Ghi chú |
|---|---|---|
| 1 | Bộ luật ở mục 5.2 | Làm trước, ở GĐ2. Là hệ luật, chưa phải học máy |
| 2 | Tự học kiểu chia pha (mục 5.6): chạy thử từng phương án trên dòng xe thật, đo thời gian chờ, giữ phương án thấp nhất | Đã viết xong, mới thử trong mô phỏng. Đúng nghĩa "thuật toán tối ưu hóa" trong tên đề tài. Không ước tính trước được vì camera không biết xe làn trong đi thẳng hay rẽ |
| 3 | Mô hình học máy nhỏ huấn luyện từ dữ liệu mô phỏng trên máy tính | Tùy chọn, nếu còn thời gian |

Ở mọi mức, kết quả phải đi qua lớp an toàn: giới hạn Tmin/Tmax, kiểm tra xung đột, vàng và đỏ toàn phần.

### 5.5 Xe ưu tiên

Bốn loại, ghi ở trường `ut`: 1 cứu hỏa, 2 quân sự, 3 công an, 4 cứu thương. Số nhỏ hơn được mở đèn trước.

- Khi camera của một hướng báo có xe ưu tiên: bước đang chạy phải xanh ít nhất 5 s, rồi kết thúc đúng trình tự vàng, đỏ toàn phần và bật "riêng hướng đó".
- Nhiều hướng cùng có xe ưu tiên: mở theo thứ tự loại xe; cùng loại thì hướng thấy trước đi trước. Đang mở cho xe hạng thấp mà hướng khác có xe hạng cao hơn thì nhường (sau khi đã xanh đủ 5 s), xong lại quay về.
- Xe ưu tiên qua khỏi vùng camera 1 s (`UT_GIU`) là đèn chuyển vàng ngay. Hướng khác còn xe ưu tiên thì mở tiếp cho hướng đó; hết thì trả đèn cho trục vừa phải chờ.
- Xe ưu tiên biến mất ngay trong lúc đèn đang chuyển (vàng, đỏ toàn phần): hủy, không mở riêng nữa.
- Một lần ưu tiên dài tối đa 60 s; quá mức đó thì khóa hướng ấy cho tới khi `emg` về 0.
- Camera chỉ thấy xe trong vùng chờ 304 mm. Xe ưu tiên kẹt ở cuối hàng dài phía ngoài vùng đó chưa được mở đèn cho tới khi nhích vào vùng camera.

Trình tự chi tiết theo báo cáo (mốc T − 15 s, T − 10 s, T, T + 5 s, bù 15 s xanh) làm ở GĐ5.

Hạn chế đã biết của làn chung: xe rẽ trái đứng đầu làn trong chặn xe đi thẳng phía sau cho tới khi được rẽ; tổ hợp "riêng một hướng" (thẳng và trái cùng xanh) tránh được việc này.

Các ý khác trong báo cáo để dành cho GĐ6: "xả cân bằng" chia thời gian theo tỉ lệ hàng đợi; giảm xanh cho làn có xe chết máy.

### 5.6 Tự học kiểu chia pha (đã cài ở `dieu_khien.h` và `dieu_khien.js`, mới thử trong mô phỏng)

Bộ điều khiển chọn giữa "riêng" và "thẳng trước" cho từng trục bằng cách đo từng lượt, chỉ dùng số đếm của camera (bản thứ hai, viết lại ngày 2026-10-11; bản đầu đo thời gian chờ theo cửa sổ 3 vòng và đổi kiểu quá chậm):

- Thời gian đèn của một lượt = thời gian xanh của từng bước (tính tới lúc các làn của bước đó hết xe) + số bước × (vàng + đỏ toàn phần + thời gian chờ cắt sớm). Số xe đã qua = số lần số đếm của một làn giảm 1.
- Lượt chạy "riêng" cho mốc: mỗi xe cần bao nhiêu giây xanh. Có hai mốc: lượt xả hết xe và lượt quá tải (có bước hết giờ mà vẫn còn xe).
- Lượt chạy "thẳng trước" cho điểm %: thời gian đèn của lượt đó so với thời gian kiểu riêng cần cho cùng số xe. Dưới 100 % là thẳng trước nhanh hơn. Mô phỏng ở 6 xe/phút: 61 % khi không có xe rẽ trái, 100 % ở 10 % rẽ trái, 125 % ở 25 %.

Luật chọn cho từng trục:

1. Đang giữ "riêng": cứ `HOC_CACH_THU` lượt (4) thì thử một lượt "thẳng trước". Đạt 100 − `HOC_NGUONG` (90 %) thì lượt sau thử tiếp; trung bình hai lượt vẫn đạt thì đổi. Kém rõ (từ 110 %) thì lần thử sau giãn ra 8 lượt.
2. Đang giữ "thẳng trước": điểm trung bình trượt lên tới 105 % thì về "riêng". Thỉnh thoảng chạy một lượt "riêng" để làm mới mốc (cách 4 lượt khi điểm gần 100 %, cách 16 lượt khi điểm dưới 80 %).
3. Lúc quá tải đòi bằng chứng mạnh hơn: lượt thử phải đạt 80 %, không đạt thì giãn tới 16 lượt; đang giữ "thẳng trước" mà điểm từ 100 % là về "riêng".
4. Lượt có xe ưu tiên, thiếu camera hoặc dưới `HOC_SO_XE` xe qua (4) thì không tính. Lượt thử bị bỏ vì lí do đó thì chờ đủ `HOC_CACH_THU` lượt mới thử lại (thêm 2026-10-12: camera thật thỉnh thoảng nhìn nhầm ra xe ưu tiên, và lúc đông xe hai lượt thử sát nhau làm có xe đứng chờ hơn 5 phút). Lượt thử mà chưa có mốc "riêng" cùng loại để so thì bỏ qua và chạy "riêng" để lấy mốc (sửa 2026-10-12: trước đó có thể thử mãi không dừng). Mỗi lần bật máy bắt đầu từ kiểu "riêng".

Kết quả mô phỏng 120 phút với camera lí tưởng (`node kiem_thu/khao_sat_kieu_pha.js`, bảng đầy đủ ở `README.md`): trung bình 13 kiểu dòng xe, tự học chờ ít hơn kiểu riêng 14 % và nhiều hơn kiểu tốt nhất của từng dòng 9 %; ít hơn 53–65 % khi không có xe rẽ trái ở đường đông; nhiều hơn 1–8 % khi từ 25 % xe rẽ trái. Chỗ yếu: ngã tư quá tải mà nhiều xe rẽ trái (12 xe/phút mỗi hướng, 30 % rẽ trái) thì chờ lâu hơn kiểu riêng 17 %.

Với camera ESP32-S3-CAM (1 khung/giây, lọc 3 khung): tự học vẫn chọn đúng kiểu ở 8 dòng xe đã thử; so với kiểu riêng: −58 % (6 xe/phút, không rẽ trái), −27 % (5 % rẽ trái), +5 % (25 % rẽ trái), −68 % (9 xe/phút, không rẽ trái), +16 % (quá tải, 30 % rẽ trái).

Đổi dòng xe giữa chừng (6 xe/phút mỗi hướng): camera lí tưởng: từ 25 % rẽ trái về 0 % thì sang "thẳng trước" sau 11–15 phút, một lần 30 phút; từ 0 % lên 25 % thì về "riêng" sau 9–20 phút. Camera ESP32-S3-CAM: 2–21 phút và 8–26 phút; có một lần đo hai trục giữ "thẳng trước" quá 40 phút vì điểm % lơ lửng 95–100 % (chưa tới 105 % để trả về; hạ mức trả về thì các dòng 5 % rẽ trái lại kém đi, nên chưa đổi).

Điều kiện để dùng trên sa bàn thật: xe phải chạy qua vạch thật (số đếm giảm dần) và camera đếm ổn định. Số đếm nhảy lung tung, hoặc hàng xe luôn đầy vùng camera làm số đếm đứng yên, thì số xe qua bị sai. Xe đặt bằng tay thì nên chọn kiểu cố định bằng lệnh `kieu`.

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
| Camera mất tín hiệu sau | 3 s không nhận gói → từ lượt sau trục đó dùng thời gian cố định | Đề xuất |
| Cắt xanh sớm `CAT_SOM` | hết xe quá 5 s | Thầy yêu cầu 2026-10-11 |
| Ưu tiên tự hủy sau | 60 s | Đề xuất |
| Xanh ít nhất trước khi bị cắt để ưu tiên | 5 s | Đề xuất |
| Xe ưu tiên qua rồi thì chuyển vàng sau `UT_GIU` | 1 s không còn thấy xe | Thầy yêu cầu 2026-10-11 (trước đó 5 s) |
| Kiểu chia pha `KIEU_PHA` | web: tự học; firmware: riêng từng hướng | Đề xuất |
| Tự học: khoảng cách giữa hai lượt thử `HOC_CACH_THU` | 4 lượt của một trục | Đề xuất, chọn qua mô phỏng |
| Tự học: số xe ít nhất của một lượt được tính `HOC_SO_XE` | 4 xe | Đề xuất |
| Tự học: ngưỡng đổi kiểu `HOC_NGUONG` | 10 % | Đề xuất, chọn qua mô phỏng |
| Camera: số khung xử lí mỗi giây | khoảng 1 (chụp + cắt + co + chạy mô hình) | Thầy đặt 2026-10-12; FOMO 96 × 96 ảnh xám trên ESP32-S3 riêng phần mô hình mất 115–143 ms |
| Camera: lọc nhiễu `SO_KHUNG_LOC` | 3 khung | Đề xuất, chọn qua mô phỏng |
| Camera: gửi gói `CHU_KI_GUI_MS` | sau mỗi khung, cách nhau ít nhất 250 ms | Có từ bản đầu; ghi rõ 2026-10-12 |
| Camera: ảnh đưa vào mô hình | 96 × 96 (nên thử 160 × 160) | Đề xuất, xem mục 8 |

Với quy đổi trên, mỗi xe mô hình cộng 5 s: 0 xe → 15 s, 1 xe → 20 s, 2 xe → 25 s, 3 xe → 30 s, 4 xe (đầy làn từ Lần 08) → 35 s. Mức Tmax 90 s chỉ đạt khi có 150 xe thật, tức không xảy ra trên sa bàn.

Bảng dưới lập khi mỗi làn tối đa 3 xe. Với sa bàn Lần 08 (4 xe mỗi làn), trường hợp đông nhất là xanh 35 s mỗi pha, chu kì 4 × (35 + 3 + 2) = 160 s.

Thời gian từng nhóm đèn theo thứ tự cố định ở 5.1 (xanh / vàng / đỏ, tính bằng giây). Đỏ = chu kì − xanh − vàng. Chế độ thích ứng đã cài dùng tổ hợp ở 5.2 và chốt theo lượt nên thứ tự bước khác bảng này; với kiểu riêng từng hướng, một vòng (lượt dọc + lượt ngang) của các ví dụ vẫn dài 40, 90 và 160 s.

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

Vị trí theo khung đèn Lần 08: trụ camera nằm giữa 2 cụm đèn, đúng trên vạch chia 2 làn; sau khi bẻ tấm gá, ống kính nằm trước vạch dừng khoảng 14 mm (về phía xe tới), cao khoảng 285 mm, trục ống kính chếch 26° so với phương thẳng đứng, nhìn ngược về phía xe đang tới. (Số lấy từ thiết kế ghi ở mục 2; script khung đèn chưa đọc lại được trong phiên 2026-10-12 vì máy đang dùng chưa cấp quyền thư mục dự án.)

Hình học tính trong `web/may_anh.js` (2026-10-12), với ống kính thường đi kèm OV2640, góc nhìn 66° theo đường chéo (55° theo cạnh dài, 43° theo cạnh ngắn), khung 320 × 240:

- **Cạnh dài khung hình dọc theo đường (đề xuất, mục 3 phần 28):** thấy trọn bề rộng hai làn từ trước mép làn bao (vùng chờ bắt đầu ở 58 mm) tới cách vạch dừng 7 mm. Xe đứng sát vạch dừng vẫn thấy 89 % thân ở cỡ xe 75 %. ROI tính ra: 277 × 136 điểm ảnh bắt đầu ở (43, 52), vạch dừng ở mép phải ảnh, làn trong ở phía trên. Đây là dòng cấu hình ban đầu đã ghi vào `firmware/camera/cau_hinh.h` cho cả 4 camera; lắp xong vẫn phải bấm lại trên trang thu ảnh.
- **Cạnh ngắn dọc theo đường:** khung hình dừng cách vạch dừng 38 mm, xe sát vạch chỉ còn 15–48 % thân trong ảnh và gần như không được nhận ra. Mô phỏng ở cỡ xe 100 %: số khung đếm đúng còn 48 % (so với gần 80 %).
- Bề ngang khung hình rộng hơn nhiều so với 2 làn, nên ROI bỏ phần lớn ảnh; ảnh ROI co về 96 × 96 chỉ còn 35 % độ phân giải theo chiều dọc đường.

Xử lí ảnh (firmware đã viết theo cách này; mô phỏng làm đúng thứ tự đó):

- Chụp ở 320 × 240, cắt ROI chỉ chứa 2 làn chờ, rồi co về kích thước đầu vào mô hình kiểu Squash.
- Mô hình FOMO chia ảnh 96 × 96 thành lưới 12 × 12 ô; ô sáng cùng nhãn nằm sát nhau bị gộp thành một vật. Hệ quả tính được: hàng xe xếp kín làn ở cỡ 75 % (5 xe) bị đếm thành 4, cỡ 60 % (6 xe) thành 5, vì hai xe cuối hàng rơi vào hai ô sát nhau; cỡ 100 % đếm đủ. Ảnh 160 × 160 (lưới 20 × 20) đếm đủ ở cả ba cỡ, giờ cao điểm chờ ít hơn chừng 3 s; tính nhiều gấp khoảng 2,8 lần, ước 0,3–0,4 s mỗi lần trên ESP32-S3, cần đo trên board thật.
- Ảnh huấn luyện phải đi qua đúng bước cắt này, chụp bằng chính camera đã gắn cố định trên sa bàn (chế độ thu ảnh lưu ảnh ROI ở kích thước gốc, nên dùng được cho cả mô hình 96 và 160).
- Sau khi mô hình trả về tâm từng xe, gán xe vào làn bằng 2 tứ giác (làn ngoài, làn trong). Tâm nằm ngoài cả hai tứ giác thì bỏ qua: làn rẽ phải, 2 làn ra bên kia dải phân cách, đường bao ngoài.
- Lọc 3 khung, gửi ngay sau mỗi khung. Ở 1 khung/giây, xe vừa vào vùng chờ thì trung tâm biết sau 2–3 s; xe ưu tiên qua vạch rồi thì 3–4 s sau đèn chuyển vàng (camera lí tưởng: 1,5 s).
- Ánh sáng đèn tín hiệu ngay dưới camera hắt lên xe ở vạch dừng và đổi màu theo pha, nên ảnh huấn luyện phải có đủ cả lúc đèn đỏ, vàng, xanh.
- Thông số ROI và 2 tứ giác lưu riêng cho từng camera.

Ảnh hưởng của camera thật trong mô phỏng (kiểu riêng, cỡ xe 75 %, 60 phút × 3 lần): thời gian chờ tăng 3–6 s ở các dòng xe không quá tải (25,9 → 31,9 s ở dòng xe đều) và khoảng 16 % ở giờ cao điểm (81,1 → 94,2 s). Không lọc thì giờ cao điểm lên 188,7 s và trong 6 giờ mô phỏng có 129 lần mở ưu tiên nhầm (lọc 3 khung: 14 lần). Xử lí 3 khung/giây thì dòng xe đều còn 27,6 s. Phần "mô hình nhận đúng 95 % xe thấy rõ" là giả định; cần đo trên mô hình thật rồi chỉnh lại.

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
- [ ] GĐ2. Bộ quyết định mức 1 với JSON gõ qua Serial. Xong khi: đổi `s`, `l` thì kế hoạch lượt sau đổi theo, lượt đang chạy không bị ảnh hưởng
- [ ] GĐ3. Một camera: cắt ROI, thu ảnh, huấn luyện, đếm riêng 2 làn, in ra Serial. Xong khi: đếm đúng 0–4 xe mỗi làn trên 90 % lần thử
- [ ] GĐ4. Camera gửi ESP-NOW; ghép 1 rồi 4 camera. Xong khi: trung tâm nhận đủ 4 hướng, mất gói dưới 5 %
- [x] Thêm chốt kế hoạch theo lượt, ba kiểu chia pha, chế độ tự học, bốn loại xe ưu tiên, chuyển vàng ngay sau khi xe ưu tiên qua, cỡ xe trong mô phỏng; đã kiểm tra trên máy tính, chưa thử trên board (2026-10-11)
- [x] Camera trong mô phỏng làm đúng như ESP32-S3-CAM thật (góc nhìn OV2640, cắt ROI, FOMO 96 × 96, 1 khung/giây, lọc 3 khung); firmware camera lọc 3 khung, có cấu hình ROI ban đầu tính từ hình học; đã kiểm tra trên máy tính (2026-10-12)
- [ ] GĐ5. Xe ưu tiên. Xong khi: đặt xe ưu tiên vào thì hướng đó được xanh, xe qua thì đèn chuyển vàng và hệ thống tự trở lại bình thường; đặt hai loại xe ở hai hướng thì đèn mở đúng thứ tự
- [ ] GĐ6. Cơ chế an toàn và đo đạc. Xong khi: rút nguồn 1 camera hệ thống vẫn chạy; có số liệu so sánh chế độ cố định và thích ứng trên sa bàn
- [ ] GĐ7 (tùy chọn). Thử chế độ tự học (mức 2) trên sa bàn khi camera đã đếm ổn định; mức 3 nếu còn thời gian

GĐ1–2 và GĐ3 có thể làm song song bởi hai nhóm học sinh.

## 11. Việc đang mở

1. Xác nhận các đề xuất còn lại ở mục 3: quy đổi xe (13), tua nhanh (14), ba mức của bộ quyết định (17). Mục 15 và 16 đã chốt.
2. Học sinh đo lại 3–4 xe Hot Wheels sẽ dùng (rộng nhất kể cả bánh, dài từ đầu tới đuôi): vùng chờ 304 mm đủ 4 xe nếu xe dài ≤ 70–72 mm; xe 75 mm thì xe thứ 4 lấn ~2 cm ra làn bao.
3. Ống kính camera là loại thường hay góc rộng; đo board thật để chỉnh ô ống kính trên tấm gá. Mô phỏng tính với ống kính thường 66° theo đường chéo; nếu là ống kính góc rộng (120–160°) thì khung hình khác hẳn, ảnh méo cong, có thể hạ camera xuống khoảng 150–170 mm. Khi gắn: cạnh dài của khung hình dọc theo đường (mục 8). (Loại board đã rõ: ESP32-S3-CAM có đế nạp.)
4. Thử độ sáng module đèn ở 3,3 V.
5. Mua mạch hạ áp 5 V 5 A.
6. Báo cáo ghi mỗi hướng có "1 làn đi thẳng/rẽ phải, 1 làn rẽ trái" và dùng cảm biến âm thanh; sa bàn thực tế là làn ngoài đi thẳng, làn trong thẳng hoặc rẽ trái, rẽ phải có làn riêng, không có cảm biến âm thanh. Cần sửa báo cáo cho khớp.
7. Thư mục `Decal sa ban/Khung den` còn tệp cũ `Duong_cat_khung_den_A4.pdf` (Lần 05); dưỡng cắt mới là `Duong_cat_khung_den_A3.pdf`. Có thể xóa tệp A4 cũ (bản lưu vẫn còn trong `Nhật ký thiết kế/Lần 05`).
8. Thử code trên board thật theo danh sách trong `README.md` của thư mục code: 24 bóng đèn (lệnh `thuday`), chu kì cố định, bản tin gõ qua Serial, camera ở chế độ thu ảnh.
9. Mô phỏng (trễ phản ứng 1 s, cỡ xe 75 %): chế độ thích ứng tự học chờ ít hơn chế độ cố định ở cả 6 kiểu dòng xe (18–99 %). Kiểu "ít rẽ trái" trước đây thua 24 %, nay hơn 18 % nhờ kiểu thẳng trước. Cần kiểm lại trên sa bàn thật, nhất là việc camera đếm có đủ ổn định để cắt sớm và để tự học không.
10. Thầy quyết: giữ thứ tự quân sự trước công an, hay cho hai loại cùng hạng như Điều 27 Luật Trật tự, an toàn giao thông đường bộ 2024 (mục 24 phần 3).
11. Chuẩn bị 4 xe ưu tiên mô hình khác hẳn nhau về màu để mô hình 96 × 96 phân biệt được; chụp đủ ảnh từng loại. Nếu không phân biệt nổi thì gộp một lớp `xe_uu_tien`.
13. Khi có mô hình thật: đo thời gian mỗi khung (camera in "khung/giây" ra Serial) cho bản 96 × 96 và 160 × 160; dưới khoảng 0,6 s thì dùng 160 × 160. Đo tỉ lệ nhận đúng thật để thay giả định 95 % trong mô phỏng.
14. Kiểm lại hình học camera với script khung đèn Lần 08 (độ cao ống kính, góc chúc, vị trí so với vạch dừng) khi mở phiên trên máy có thư mục dự án; nếu khác thì sửa `cao_mm`, `nghieng`, `lui_mm` trong `web/may_anh.js` và dòng cấu hình trong `cau_hinh.h`.
12. Tự học còn yếu khi ngã tư quá tải mà nhiều xe rẽ trái (chờ lâu hơn kiểu riêng 16–17 % trong mô phỏng), và có lần mất 30 phút (camera thật: hơn 40 phút) mới đổi kiểu. Trên sa bàn thật còn phải xem camera đếm có đủ ổn định để đếm được xe qua vạch không.

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
| 2026-10-11 | Chế độ thích ứng chốt kế hoạch theo từng lượt (mỗi lượt một trục) thay vì cả vòng. Thêm ba kiểu chia pha: riêng từng hướng, thẳng trước, đối xứng. Khảo sát bằng mô phỏng 120 phút: đối xứng chỉ tốt khi không có xe rẽ trái (từ 5 % đã chờ gấp đôi kiểu riêng); thẳng trước tốt nhất khi dưới 10–12 % rẽ trái; riêng tốt nhất khi từ 15 % trở lên |
| 2026-10-11 | Thêm chế độ tự học theo yêu cầu của thầy: mỗi trục tự chạy thử "riêng" và "thẳng trước", đo thời gian chờ mỗi xe từ số đếm camera, giữ kiểu tốt hơn. Trung bình 13 kiểu dòng xe chờ ít hơn kiểu riêng 12 %; kém kiểu riêng 29 % khi quá tải và nhiều rẽ trái. So khớp C++ với JavaScript trên 760 000 dòng, có 297 lần thử của bộ tự học |
| 2026-10-11 | Theo góp ý thầy chuyển tới: web vẽ xe nhỏ lại (mặc định 75 %, chọn được 60/75/100 %); thêm xe cứu hỏa, quân sự, công an bên cạnh xe cứu thương, mở đèn theo thứ tự hỏa → sự → công → thương; JSON thêm trường `ut`; firmware camera nhận 4 nhãn xe ưu tiên |
| 2026-10-11 | Theo yêu cầu của thầy: xe ưu tiên qua khỏi vùng camera 1 s là chuyển vàng ngay (trước đó giữ xanh thêm 5 s); sau ưu tiên, đèn trả cho trục vừa phải chờ |
| 2026-10-11 | Thầy thấy trên web: giảm xe rẽ trái mà đèn vẫn chỉ chạy kiểu "thẳng và trái". Kiểm tra lại: bản tự học đầu đổi kiểu quá chậm (11 đến hơn 90 phút mô phỏng, có khi không đổi). Viết lại tự học: đo thời gian đèn của từng lượt so với kiểu riêng cho cùng số xe, cứ 4 lượt thử một lượt. Nay đổi kiểu sau 9–20 phút (một lần 30 phút); trung bình 13 kiểu dòng xe chờ ít hơn kiểu riêng 15 % (trước 12 %); quá tải nhiều rẽ trái kém kiểu riêng 17 % (trước 29 %). Thêm bài kiểm tra đổi dòng xe giữa chừng |
| 2026-10-12 | Theo yêu cầu của thầy, camera trong mô phỏng làm đúng như ESP32-S3-CAM: khung 320 × 240 chiếu phối cảnh từ ống kính OV2640 66° trên khung đèn (cao 285 mm, chúc 26°), cắt ROI 277 × 136 rồi co về 96 × 96, nhận diện kiểu FOMO lưới 12 × 12 có gộp ô như Edge Impulse, 1 khung mỗi giây, lọc 3 khung, gửi ngay sau mỗi khung. Phát hiện: phải gắn cạnh dài khung hình dọc theo đường; ảnh 96 × 96 đếm thiếu một xe khi làn xếp kín (160 × 160 thì đủ). Camera thật làm chờ lâu hơn 3–6 s, giờ cao điểm 16 %; xe ưu tiên qua rồi 3–4 s sau đèn mới vàng. Firmware camera: lọc 3 khung (trước 5), cấu hình ROI ban đầu tính từ hình học. Tự học: sửa lỗi thử mãi khi chưa có mốc; lượt thử bị hủy thì chờ đủ 4 lượt. Thêm bài kiểm tra `thu_may_anh.js` |
