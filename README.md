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

- Bộ quyết định đèn cho đúng kế hoạch ở các ca của mục 6 trong `TIEN_DO_DU_AN.md` và ở từng kiểu chia pha.
- Bản C++ trong firmware và bản JavaScript trong web cho kết quả giống nhau trên 760 000 dòng trạng thái sinh ngẫu nhiên, kể cả lúc bộ đếm thời gian 32 bit quay vòng, lúc có xe ưu tiên đủ 4 loại và lúc bộ tự học đang chạy (262 lượt chạy thử, 10 lần đổi kiểu).
- Trong 400 lần chạy ngẫu nhiên 15 phút (hơn 800 lần cắt xanh sớm, hơn 6 000 lần ưu tiên): không lần nào hai nhóm xung đột cùng sáng, không nhóm nào từ xanh sang đỏ mà thiếu vàng, đỏ toàn phần luôn đủ.
- Bộ tự học, thử trên một ngã tư thu nhỏ trong `thu_quyet_dinh.js`: không có xe rẽ trái thì cả hai trục sang "thẳng trước" sau 3 phút; một nửa xe làn trong rẽ trái thì giữ "riêng"; đổi dòng xe giữa chừng thì đổi kiểu theo sau 7–9 phút.
- Mô phỏng 30 phút với 6 kiểu dòng xe và 3 cỡ xe: không có hai xe đè lên nhau, không xe nào vượt đèn đỏ, cả 4 loại xe ưu tiên đều qua được, xe ưu tiên qua vạch dừng thì chừng 1,5 s sau đèn chuyển vàng.
- Cả hai firmware biên dịch được cho ESP32-S3 với lõi `esp32` 2.0.9 và 3.3.12. Chế độ đếm của camera mới biên dịch với thư viện Edge Impulse giả (`kiem_thu/gia_ei`).

Chưa kiểm tra, vì cần phần cứng thật: nạp vào board, độ sáng đèn ở 3,3 V, ảnh từ OV2640, đường truyền ESP-NOW, mô hình nhận diện (kể cả việc phân biệt 4 loại xe ưu tiên). Chế độ tự học mới chạy trong mô phỏng. Danh sách việc cần thử nằm ở cuối mỗi phần dưới đây.

## 1. Web mô phỏng

Mở `web/index.html` bằng trình duyệt, không cần mạng và không cần board.

- **Bên trái:** chạy, tạm dừng, tốc độ ×1 đến ×16; chế độ cố định hoặc thích ứng; kiểu chia pha (tự học, riêng, thẳng trước, đối xứng); các thông số đèn; cỡ xe; lưu lượng và tỉ lệ rẽ trái của từng hướng; 4 nút thả xe ưu tiên cho mỗi hướng; trễ phản ứng của người lái; nút so sánh các phương án.
- **Giữa:** sa bàn vẽ từ đúng tệp decal Lần 08. Hai cột hai bên cho biết tổ hợp đang xanh, thời gian còn lại, kế hoạch của lượt đang chạy và lượt sau, số xe đã qua, thời gian chờ, và bộ tự học đang giữ kiểu chia pha nào cho từng trục.
- **Phía dưới:** 4 ô camera. Khung đứt là ROI, vùng xanh dương là làn ngoài (`s`), vùng hồng là làn trong (`l`), dòng cuối là bản tin JSON gửi về trung tâm.

Đèn trong web do đúng hai tệp `quyet_dinh.js` và `dieu_khien.js` điều khiển; hai tệp này là bản JavaScript của `quyet_dinh.h` và `dieu_khien.h` trong firmware. Web hiện là mô phỏng thuần, chưa nối với board.

Nút **Chạy so sánh** chạy nhanh 60 phút cho chế độ cố định và cho từng kiểu chia pha của chế độ thích ứng, với đúng dòng xe đang đặt, rồi cho biết kiểu nào chờ ít nhất.

Gộp thành một tệp để gửi qua Zalo hoặc email:

```
node cong_cu/dong_goi.js
```

Lệnh này tạo `Mo_phong_sa_ban.html` ở thư mục gốc.

### Xe trong mô phỏng chạy thế nào

- Xe chạy 120 mm/s, giảm tốc khi vào cua, giữ khoảng cách với xe trước và xếp hàng hở nhau chừng 8 mm.
- **Cỡ xe** chọn được 60 %, 75 % hoặc 100 %. Cỡ 100 % là xe Hot Wheels trên sa bàn thật (68 × 34 mm, vùng chờ chứa 4 xe mỗi làn). Mặc định là 75 % (51 × 26 mm, 5 xe mỗi làn): xe chiếm khoảng một nửa bề rộng làn như ô tô ngoài đường, nên không che mất mũi tên và chỗ rẽ. Cỡ 60 % chứa 6 xe mỗi làn. Đổi cỡ xe thì mô phỏng chạy lại từ đầu.
- **Trễ phản ứng của người lái:** xe đang đứng trong hàng chờ chỉ lăn bánh sau khi đã thấy đi được (đèn xanh, hoặc xe trước đã rời ra) một khoảng bằng mức trễ chung nhân với hệ số riêng của từng người lái, từ 0,7 đến 1,3. Mức chung mặc định 1 s, chỉnh được trên web; đặt 0 là không trễ. Xe đã qua vạch dừng thì không tính trễ nữa.
- Với mức 1 s và cỡ xe 75 %, hàng xe xả như sau: xe đầu qua vạch dừng 1,4 s sau khi đèn xanh, mỗi xe sau cách xe trước khoảng 2 s (không trễ thì 1 s). Con số 2 s mỗi xe mỗi làn cũng là mức thường gặp ngoài đường.
- Gặp đèn vàng: xe dừng nếu kịp, không kịp thì đi tiếp. Xe rẽ phải đi làn riêng, nhường xe trên làn mà nó nhập vào.
- **Xe ưu tiên** có 4 loại: cứu hỏa, quân sự, công an, cứu thương. Camera chỉ thấy xe ưu tiên khi xe đã vào vùng chờ dài 304 mm; xe còn kẹt ở cuối hàng dài phía ngoài thì chưa được mở đèn.

### Kết quả mô phỏng

Thời gian chờ trung bình trước vạch dừng (giây), 60 phút mô phỏng, trung bình 3 lần chạy, thông số mặc định, cỡ xe 75 % (lấy từ `node kiem_thu/thu_mo_phong.js`). Bốn cột giữa là chế độ thích ứng với từng kiểu chia pha:

| Kiểu dòng xe | Cố định | Riêng | Thẳng trước | Đối xứng | Tự học | Tự học so với cố định |
|---|---|---|---|---|---|---|
| Đều, vừa phải (6 xe/phút mỗi hướng, 25 % rẽ trái) | 55,5 | 25,9 | 34,1 | 245,9 | 26,7 | −52 % |
| Dọc đông, ngang vắng | 153,3 | 17,9 | 17,6 | 128,3 | 17,7 | −88 % |
| Chỉ Hướng 1 có xe | 398,1 | 2,4 | 2,4 | 2,4 | 2,4 | −99 % |
| Giờ cao điểm (12 xe/phút mỗi hướng, 30 % rẽ trái) | 595,6 | 81,1 | 179,2 | 986,8 | 99,1 | −83 % |
| Ít rẽ trái (8 xe/phút mỗi hướng, 5 % rẽ trái) | 39,6 | 39,3 | 26,0 | 77,2 | 33,1 | −16 % |
| Vắng (1,5 xe/phút mỗi hướng) | 33,4 | 9,3 | 9,1 | 9,9 | 9,3 | −72 % |

Đây là số của mô phỏng với trễ phản ứng 1 s, chưa phải số đo trên sa bàn thật.

### Kiểu chia pha nào tốt nhất, khi nào

Khảo sát riêng về kiểu chia pha (120 phút mô phỏng, trung bình 3 lần chạy, `node kiem_thu/khao_sat_kieu_pha.js`). Số xe/phút là của mỗi hướng; thời gian chờ tính bằng giây:

| Dòng xe | Riêng | Thẳng trước | Đối xứng | Tự học | Tự học so với riêng |
|---|---|---|---|---|---|
| 3 xe/phút, 0 % rẽ trái | 13,5 | 9,1 | **9,0** | 9,3 | −31 % |
| 3 xe/phút, 25 % rẽ trái | **15,2** | 15,4 | 31,0 | 15,3 | +1 % |
| 6 xe/phút, 0 % rẽ trái | 25,9 | **10,8** | **10,8** | 11,7 | −55 % |
| 6 xe/phút, 5 % rẽ trái | 25,8 | **17,3** | 51,8 | 19,3 | −25 % |
| 6 xe/phút, 10 % rẽ trái | 25,9 | **22,7** | 93,4 | 24,6 | −5 % |
| 6 xe/phút, 25 % rẽ trái | **25,9** | 35,2 | 322,7 | 27,0 | +4 % |
| 6 xe/phút, 40 % rẽ trái | **27,0** | 41,0 | 642,8 | 28,9 | +7 % |
| 8 xe/phút, 5 % rẽ trái | 38,9 | **26,5** | 79,6 | 31,8 | −18 % |
| 9 xe/phút, 0 % rẽ trái | 48,6 | **15,4** | 17,7 | 17,2 | −65 % |
| 9 xe/phút, 25 % rẽ trái | **47,1** | 61,2 | 1203,9 | 50,9 | +8 % |
| 12 xe/phút, 30 % rẽ trái | **87,0** | 279,5 | 1939,0 | 102,0 | +17 % |
| 6 xe/phút, dọc 0 %, ngang 25 % rẽ trái | 25,9 | 19,1 | 39,2 | **17,7** | −32 % |
| Dọc đông, ngang vắng | 18,3 | 18,0 | 162,3 | **17,8** | −3 % |

Đọc bảng:

- **Đối xứng** ("hai hướng cùng đi thẳng, rồi cùng rẽ trái") chỉ ngang với kiểu tốt nhất khi không có xe rẽ trái. Chỉ cần 5 % xe rẽ trái là thời gian chờ đã gấp đôi kiểu riêng, từ 25 % trở lên thì ngã tư kẹt cứng. Lí do nằm ở sa bàn: làn trong đi chung thẳng và trái, nên trong bước "cùng đi thẳng" một xe rẽ trái đứng đầu làn chặn hết xe đi thẳng phía sau, và trong bước "cùng rẽ trái" một xe đi thẳng đứng đầu lại chặn hết xe rẽ trái.
- **Thẳng trước** ("hai hướng cùng đi thẳng trước, làn ngoài hết xe thì mở riêng từng hướng cho làn trong") tốt nhất khi dưới khoảng 10–12 % xe rẽ trái, và hơn kiểu riêng càng nhiều khi đường càng đông (9 xe/phút, không rẽ trái: 15,4 s so với 48,6 s).
- **Riêng** ("mỗi hướng được xanh cả thẳng lẫn trái, xong hướng này tới hướng đối diện") tốt nhất khi từ khoảng 15 % xe rẽ trái trở lên, và không bao giờ kẹt vì xe rẽ trái.
- **Tự học** không biết trước tỉ lệ rẽ trái, tự tìm ra kiểu hợp cho từng trục. Trung bình 13 dòng trên, nó chờ ít hơn kiểu riêng 15 % và nhiều hơn kiểu tốt nhất của từng dòng 8 % (cái giá của việc phải thử). Khi hai trục khác nhau (dọc không rẽ trái, ngang rẽ 25 %), nó chọn mỗi trục một kiểu và tốt hơn mọi kiểu dùng chung cho cả hai trục.
- Chỗ yếu của tự học: khi ngã tư quá tải mà nhiều xe rẽ trái (12 xe/phút, 30 %), mỗi lượt thử "thẳng trước" làm hàng xe dài thêm và lâu tan, nên nó chờ lâu hơn kiểu riêng 17 %. Gặp tình huống này thì chọn tay kiểu riêng.

**Đổi dòng xe giữa chừng** (6 xe/phút mỗi hướng, đổi tỉ lệ rẽ trái ở phút 30, đo số phút tới khi từng trục đổi kiểu; 3 lần chạy, mỗi lần 2 trục):

| Đổi | Số phút tới khi đổi kiểu |
|---|---|
| 25 % rẽ trái → không ai rẽ trái (phải sang "thẳng trước") | 9, 10, 11, 11, 12, 30 |
| Không ai rẽ trái → 25 % rẽ trái (phải về "riêng") | 9, 14, 17, 18, 19, 20 |

Tức là thường mất 10–20 phút mô phỏng, tương đương vài lượt thử của mỗi trục. Bản tự học đầu tiên (đo thời gian chờ theo từng cửa sổ 3 vòng) trong cùng tình huống mất từ 11 phút tới hơn 90 phút, có khi không đổi.

## 2. Bộ quyết định đèn

8 nhóm tín hiệu: mỗi hướng một nhóm đi thẳng và một nhóm rẽ trái. Hai nhóm chỉ được xanh cùng lúc khi cùng một hướng, hoặc hai hướng đối diện và cùng loại (thẳng với thẳng, trái với trái). Rẽ trái không bao giờ xanh khi hướng ngược chiều đang đi thẳng.

**Chế độ cố định** (đối chứng): dọc rẽ trái 15 s → ngang đi thẳng 30 s → ngang rẽ trái 15 s → dọc đi thẳng 30 s, chu kì 110 s.

**Chế độ thích ứng** chốt kế hoạch theo từng **lượt**. Một lượt là phần của một trục (dọc hoặc ngang), chốt theo số đếm ngay lúc lượt đó bắt đầu; hết lượt thì tới trục kia. Chốt theo lượt dùng số đếm mới hơn so với chốt cả vòng hai trục một lần: trong mô phỏng giờ cao điểm (kiểu riêng), thời gian chờ giảm từ 169 s xuống 81 s chỉ nhờ đổi cách chốt này; khi đường không quá đông thì hai cách gần như nhau (28,7 s và 25,9 s ở dòng xe đều).

Kế hoạch của một lượt:

1. Trục tới lượt không có xe: lập luôn cho trục kia. Cả ngã tư trống: trục tới lượt đi thẳng 15 s.
2. Một hướng của trục trống: mở riêng hướng còn lại (thẳng và trái cùng xanh).
3. Hai hướng đều có xe, làn trong đều trống: thẳng cả hai.
4. Còn lại thì theo **kiểu chia pha** đang dùng cho trục đó:
   - *Riêng:* riêng từng hướng, hướng đông đi trước.
   - *Thẳng trước:* thẳng cả hai; khi hai làn ngoài hết xe 5 s (hoặc hết thời gian đã định) thì mở riêng hướng có làn trong đông hơn, rồi tới hướng kia nếu làn trong còn xe.
   - *Đối xứng:* thẳng cả hai, rồi trái cả hai (hoặc riêng một hướng nếu chỉ hướng đó có xe ở làn trong).
5. Camera của một hướng im quá 3 s: trục đó chạy thời gian cố định (thẳng 30 s, trái 15 s).
6. Cắt xanh sớm (`CAT_SOM`, mặc định 5 s, đặt 0 để tắt): tổ hợp đang xanh không còn xe suốt 5 s mà hướng khác có xe chờ thì chuyển vàng ngay rồi sang bước kế, không đợi hết thời gian đã định. Khi sang bước kế, bước nào không còn xe thì bỏ qua. Không cắt nếu camera của hướng đang xanh mất tín hiệu, hoặc không có xe nào chờ ở hướng khác (khi đó đèn cứ giữ xanh).

Thời gian xanh định trước của mỗi bước = Tmin + 5 s × (số xe ở làn đông nhất mà bước đó phục vụ × quy đổi / 10), trong khoảng Tmin đến Tmax; cắt xanh sớm có thể làm bước kết thúc trước mức đó. Giữa hai bước, nhóm có mặt ở cả hai thì giữ xanh; nhóm rời đi qua vàng 3 s rồi đỏ, sau đó đỏ toàn phần 2 s.

**Tự học kiểu chia pha** (`KIEU_TU_HOC`). Camera không biết xe ở làn trong sẽ đi thẳng hay rẽ trái, nên không tính trước được kiểu nào tốt hơn. Bộ điều khiển học bằng cách đo từng lượt của từng trục, chỉ dùng số đếm của camera:

- **Thời gian đèn của một lượt** = thời gian xanh của từng bước (tính tới lúc các làn của bước đó hết xe) + số bước × (vàng + đỏ toàn phần + thời gian chờ cắt sớm).
- **Số xe đã qua** trong lượt: mỗi lần số đếm của một làn giảm 1 là một xe qua vạch dừng.
- Lượt chạy kiểu "riêng" cho **mốc**: mỗi xe cần bao nhiêu giây xanh. Giữ hai mốc, một cho lượt xả hết xe và một cho lượt quá tải (có bước hết giờ mà vẫn còn xe), vì hai loại lượt này có nhịp xả khác nhau.
- Lượt chạy kiểu "thẳng trước" cho **điểm %**: thời gian đèn của lượt đó so với thời gian mà kiểu riêng cần cho cùng số xe (tính từ mốc cùng loại). Dưới 100 % là thẳng trước nhanh hơn. Trong mô phỏng ở 6 xe/phút mỗi hướng, điểm trung bình là 61 % khi không có xe rẽ trái, 81 % ở 5 % rẽ trái, 100 % ở 10 %, 109 % ở 15 % và 125 % ở 25 %, tức là vượt 100 % ở đúng quãng mà kiểu riêng bắt đầu tốt hơn.

Luật chọn, riêng cho từng trục:

1. Đang giữ "riêng": cứ 4 lượt (`HOC_CACH_THU`) thì chạy thử một lượt "thẳng trước". Lượt thử đạt 90 % trở xuống (100 − `HOC_NGUONG`) thì lượt sau thử tiếp; trung bình hai lượt vẫn đạt thì đổi sang "thẳng trước". Lượt thử kém rõ (từ 110 %) thì lần thử sau giãn ra 8 lượt.
2. Đang giữ "thẳng trước": mỗi lượt cập nhật điểm (trung bình trượt). Điểm lên tới 105 % thì về "riêng". Thỉnh thoảng chạy một lượt "riêng" để làm mới mốc: cách 4 lượt khi điểm còn gần 100 %, cách 16 lượt khi điểm đã dưới 80 %.
3. Lúc quá tải, thử sai rất tốn nên đòi bằng chứng mạnh hơn: lượt thử quá tải phải đạt 80 %, không đạt thì giãn tới 16 lượt; đang giữ "thẳng trước" mà lượt quá tải có điểm từ 100 % là về "riêng".
4. Lượt có xe ưu tiên, thiếu camera hoặc dưới 4 xe qua (`HOC_SO_XE`) thì không tính. Mỗi lần bật máy bắt đầu từ "riêng".

Cách đo này dựa vào số đếm giảm dần khi xe qua vạch. Nếu hàng xe luôn đầy vùng camera và số đếm đứng yên, hoặc camera đếm nhảy lung tung, thì số xe qua bị sai; khi không đếm được xe nào, bộ tự học giữ nguyên kiểu đang có.

Kiểu "đối xứng" không nằm trong hai kiểu được thử, vì một lần thử sai có thể làm kẹt cả ngã tư; muốn dùng thì chọn tay.

Web mặc định dùng tự học. Firmware mặc định dùng kiểu riêng (`KIEU_PHA` trong `cau_hinh.h`): tự học cần xe chạy qua vạch thật và camera đếm ổn định, còn trên sa bàn xe được đặt bằng tay. Đổi được lúc đang chạy bằng lệnh `kieu` ở Serial.

**Xe ưu tiên** (`emg` khác 0). Có 4 loại, ghi ở trường `ut`, số nhỏ hơn được đi trước: 1 cứu hỏa, 2 quân sự, 3 công an, 4 cứu thương ("hỏa → sự → công → thương").

- Bước đang chạy phải xanh ít nhất 5 s, rồi kết thúc đúng trình tự vàng, đỏ toàn phần và mở riêng hướng có xe ưu tiên.
- Nhiều hướng cùng có xe ưu tiên: mở theo thứ tự loại xe; cùng loại thì hướng thấy trước đi trước. Đang mở cho xe hạng thấp mà hướng khác có xe hạng cao hơn thì nhường ngay (sau khi đã xanh đủ 5 s).
- Xe ưu tiên qua khỏi vùng camera 1 s (`UT_GIU`) là đèn chuyển vàng ngay, không giữ xanh thêm, để các hướng khác không phải chờ lâu. Còn xe ưu tiên ở hướng khác thì mở tiếp cho hướng đó; hết thì trả đèn cho trục vừa phải chờ.
- Một lần ưu tiên dài tối đa 60 s; quá mức đó thì khóa hướng ấy cho tới khi `emg` về 0.
- Thứ tự "sự" trước "công" là theo câu thầy trò dùng để nhớ. Điều 27 Luật Trật tự, an toàn giao thông đường bộ 2024 xếp xe quân sự và xe công an cùng một hạng (sau xe chữa cháy, trước xe cứu thương); muốn đúng từng chữ theo luật thì cho hai loại này cùng hạng, sửa ở hàm `huongCanUuTien`.

Sửa luật thì sửa cả `firmware/trung_tam/quyet_dinh.h` và `web/quyet_dinh.js` (máy trạng thái và tự học: `dieu_khien.h` và `dieu_khien.js`), rồi chạy `node kiem_thu/so_khop.js` để chắc hai bản vẫn giống nhau.

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
| `{"id":2,"s":1,"l":0,"emg":1,"ut":1}` | Hướng 2 có xe ưu tiên ở làn ngoài (`emg` 1 làn ngoài, 2 làn trong). `ut`: 1 cứu hỏa, 2 quân sự, 3 công an, 4 cứu thương; không ghi `ut` thì coi là cứu thương |
| `codinh` hoặc `thichung` | Đổi chế độ, có hiệu lực từ lượt sau |
| `kieu rieng`, `kieu thangtruoc`, `kieu doixung`, `kieu tuhoc` | Đổi kiểu chia pha của chế độ thích ứng |
| `tua 4` | Chạy nhanh gấp 4 (1 đến 8) |
| `catsom 5` | Hết xe quá 5 s thì cắt xanh sớm; `catsom 0` là tắt |
| `xem` | In trạng thái, kiểu chia pha đang dùng, số đếm từng hướng, tỉ lệ mất gói |
| `xoa` | Bỏ các số đếm đã gõ tay |
| `thuday` | Lần lượt sáng từng bóng để kiểm tra dây |

LED trên board: xanh lá là đủ 4 camera, vàng là thiếu camera, xanh dương là đang ưu tiên, đỏ là lỗi an toàn.

**Việc cần thử trên board thật:**

1. GĐ1: gõ `codinh`, rồi `thuday` để kiểm tra 24 bóng đúng chân và đủ sáng. Sau đó theo dõi một chu kì 110 s.
2. GĐ2: gõ `thichung`, gõ vài bản tin JSON, xem dòng "Kế hoạch" của lượt sau đổi theo còn lượt đang chạy giữ nguyên. Gõ `kieu thangtruoc` rồi `kieu doixung` để xem kế hoạch đổi theo kiểu chia pha.
3. Thử cắt xanh sớm: gõ `{"id":1,"s":3,"l":0,"emg":0}` và `{"id":2,"s":2,"l":0,"emg":0}`, chờ Hướng 1 xanh rồi gõ `{"id":1,"s":0,"l":0,"emg":0}`. Sau 5 s phải thấy dòng "cắt xanh sớm" và Hướng 1 chuyển vàng.
4. Thử ưu tiên: gõ `{"id":2,"s":1,"l":0,"emg":1,"ut":4}`, chờ Hướng 2 xanh rồi gõ `{"id":2,"s":0,"l":0,"emg":0}`; khoảng 1 s sau đèn Hướng 2 phải chuyển vàng.
5. Thử thứ tự ưu tiên: gõ liền `{"id":1,"s":1,"l":0,"emg":1,"ut":4}` và `{"id":2,"s":1,"l":0,"emg":1,"ut":1}`. Hướng 2 (cứu hỏa) phải được mở trước Hướng 1 (cứu thương). Gõ `xoa` để bỏ.

## 4. Camera (`firmware/camera`)

**Arduino IDE:** Board `ESP32S3 Dev Module`, Flash Size `16MB`, PSRAM `OPI PSRAM` (bắt buộc), Partition Scheme `16M Flash (3MB APP/9.9MB FATFS)`, USB CDC On Boot `Disabled`. Nạp qua đế nạp.

Chân camera trong `cau_hinh.h` theo loại ESP32-S3-CAM phổ biến (trùng kiểu ESP32-S3-EYE). Nếu chế độ thu ảnh báo không khởi tạo được camera thì kiểm tra lại cáp và bảng chân này.

Mỗi board sửa hai dòng trong `cau_hinh.h`: `CAM_ID` (1 đến 4) và `CHE_DO`.

**Trình tự làm việc:**

1. Nạp `CHE_DO_THU_ANH`. Camera phát WiFi `CAM1-SaBan` (mật khẩu `12345678`); nối máy tính vào rồi mở `http://192.168.4.1`.
2. Trên trang đó: vẽ ROI quanh 2 làn chờ, bấm 4 góc làn ngoài, 4 góc làn trong. Chép dòng cấu hình trang in ra, dán đè lên dòng của camera đó trong `cau_hinh.h`, nạp lại.
3. Thu ảnh huấn luyện bằng nút "Lưu 1 ảnh" hoặc "Tự lưu". Ảnh lưu là ảnh ROI, đúng ảnh sẽ đưa vào mô hình.
4. Trên Edge Impulse: tải ảnh lên, gán nhãn `xe` cho xe thường và `xe_cuu_hoa`, `xe_quan_su`, `xe_cong_an`, `xe_cuu_thuong` cho 4 loại xe ưu tiên (đổi tên được ở `NHAN_UU_TIEN` trong `cau_hinh.h`). Tạo impulse ảnh 96 × 96 với Resize mode `Squash`, khối Object Detection (FOMO), huấn luyện, xuất `Arduino library`. Ảnh 96 × 96 khá nhỏ: nên chọn 4 xe ưu tiên khác hẳn nhau về màu, và chụp đủ ảnh cho từng loại. Nếu mô hình không phân biệt nổi 4 loại thì gộp thành một nhãn `xe_uu_tien`; khi đó trung tâm coi mọi xe ưu tiên là xe cứu thương.
5. Cài thư viện `.zip` vào Arduino IDE, sửa `THU_VIEN_EI` trong `cau_hinh.h` cho đúng tên tệp `.h`, đổi `CHE_DO` thành `CHE_DO_DEM`, nạp lại.
6. Camera in bản tin ra Serial và gửi ESP-NOW 4 gói mỗi giây. Ở board trung tâm gõ `xem` để thấy số đếm và tỉ lệ mất gói.

`CHE_DO_GIA_LAP` không cần camera và mô hình: gõ `3 1 0` vào Serial để gửi s = 3, l = 1, emg = 0; gõ `3 1 2 1` để báo có xe cứu hỏa (ut = 1) ở làn trong. Dùng để thử đường truyền ESP-NOW trước khi có mô hình.

Lọc nhiễu trước khi gửi: `s`, `l` lấy trung vị 5 khung gần nhất; `emg` chỉ bật khi ít nhất 3 trong 5 khung thấy xe ưu tiên, và `ut` là loại xuất hiện ở nhiều khung nhất.

Nếu thư viện Edge Impulse báo lỗi khi biên dịch với lõi `esp32` 3.x, cài lõi 2.0.17; code trong repo biên dịch được trên cả hai dòng.

## 5. Kiểm tra trên máy tính

Cần Node.js và g++:

```
sh kiem_thu/chay_tat_ca.sh
```

Bài cuối (mô phỏng xe) mất khoảng 10 phút. Bảng khảo sát kiểu chia pha chạy riêng bằng `node kiem_thu/khao_sat_kieu_pha.js`, mất khoảng nửa giờ.

## 6. Việc để sau

- Trình tự ưu tiên chi tiết theo báo cáo (các mốc T − 15 s, T − 10 s, bù 15 s xanh).
- Tự học: giảm thêm cái giá của việc thử khi ngã tư quá tải; thử trên sa bàn thật khi camera đã đếm ổn định.
- Bộ quyết định mức 3 (học máy).
- Nối web với board qua Firebase Realtime Database.
