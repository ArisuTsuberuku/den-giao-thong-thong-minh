"""Tạo web/sa_ban_svg.js từ tệp SVG decal sa bàn, để web mô phỏng vẽ đúng sa bàn thật.
Dùng khi có bản decal mới:
    python cong_cu/tao_nen_sa_ban.py "duong/dan/Decal_sa_ban_LanNN_1000mm.svg"
Nhớ sửa các kích thước ở đầu web/mo_phong.js nếu làn đường hoặc vạch dừng thay đổi."""
import json, re, sys, pathlib

nguon = pathlib.Path(sys.argv[1]).read_text(encoding="utf-8")
nguon = re.sub(r"<metadata>.*?</metadata>", "", nguon, flags=re.S)       # bỏ phần ghi nguồn gốc tệp, không ảnh hưởng hình
nguon = re.sub(r'\s+xmlns:c2pa="[^"]*"', "", nguon)
nguon = re.sub(r'width="[\d.]+mm" height="[\d.]+mm"', 'width="1000" height="1000"', nguon, count=1)
dich = pathlib.Path(__file__).resolve().parent.parent / "web" / "sa_ban_svg.js"
dich.write_text("// Tệp sinh tự động bằng cong_cu/tao_nen_sa_ban.py từ decal sa bàn. Không sửa tay.\n"
                "var SA_BAN_SVG = " + json.dumps(nguon, ensure_ascii=False) + ";\n", encoding="utf-8")
print("Đã ghi", dich, len(nguon), "kí tự")
