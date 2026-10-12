#!/bin/sh
# Chạy toàn bộ kiểm tra không cần phần cứng. Cần Node.js và g++.
#   sh kiem_thu/chay_tat_ca.sh
set -e
cd "$(dirname "$0")/.."
echo "== 1. Bộ quyết định (JavaScript): các ca mục 6 và luật an toàn của đèn"
node kiem_thu/thu_quyet_dinh.js
echo
echo "== 2. So bản C++ của firmware với bản JavaScript của web"
node kiem_thu/so_khop.js
echo
echo "== 3. Firmware camera: cắt ROI, gán làn, lọc nhiễu"
g++ -std=c++11 -Wall -Wextra -o "${TMPDIR:-/tmp}/thu_camera" kiem_thu/thu_camera.cpp
"${TMPDIR:-/tmp}/thu_camera"
echo
echo "== 4. Mô hình camera ESP32-S3-CAM của mô phỏng: phép chiếu, ROI, gộp ô, nhịp 1 khung/giây"
node kiem_thu/thu_may_anh.js
echo
echo "== 5. Mô phỏng xe: va chạm, vượt đèn đỏ, xe ưu tiên, tự học, so sánh các chế độ (mất khoảng 20 phút)"
node kiem_thu/thu_mo_phong.js
