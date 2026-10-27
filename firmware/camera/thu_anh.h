// thu_anh.h — Trang web của camera, mở từ http://192.168.4.(10 + CAM_ID) khi camera đã nối WiFi trung tâm
// (hoặc http://192.168.4.1 khi camera tự phát WiFi "CAM<số>-SaBan"), để
//   - xem toàn khung 320 × 240, vẽ ROI và bấm 4 góc từng làn, rồi bấm "Lưu vào board" (dùng ngay,
//     không phải nạp lại) hoặc lấy dòng cấu hình dán vào cau_hinh.h;
//   - xem và lưu ảnh ROI (đúng ảnh sẽ đưa vào mô hình) làm dữ liệu huấn luyện trên Edge Impulse.
// Trang dùng đường dẫn tương đối nên cũng mở được qua trung tâm: http://<địa chỉ trung tâm>/c<CAM_ID>/
// (trung tâm chuyển tiếp, dùng khi máy tính nối trung tâm qua hotspot). Chế độ thu ảnh luôn có trang này; chế độ đếm có khi WEB_KHI_DEM là 1 (cau_hinh_mang.h).
#pragma once
#include "img_converters.h"
#include "may_anh.h"
#include "mang.h"

static uint8_t *anh_roi = NULL;      // ROI đúng cỡ gốc, thứ tự B, G, R

static const char TRANG_THU_ANH[] PROGMEM = R"HTML(<!doctype html>
<html lang="vi"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Camera sa bàn</title>
<style>
body{font:15px/1.45 system-ui,Segoe UI,sans-serif;margin:16px;background:#f4f6f2;color:#1c221e}
h1{font-size:20px;margin:0 0 4px}h2{font-size:15px;margin:0 0 6px}p{margin:6px 0}
.hai{display:flex;flex-wrap:wrap;gap:18px;margin-top:12px}.o{background:#fff;border:1px solid #c5ccc1;border-radius:6px;padding:12px;max-width:100%}
canvas,img{display:block;max-width:100%;background:#222;border-radius:4px}
button{font:inherit;padding:6px 10px;border:1px solid #9aa59d;border-radius:4px;background:#fff;cursor:pointer;margin:2px 2px 2px 0}
button.chon{background:#1b5fae;color:#fff;border-color:#1b5fae}
pre{background:#eef1ec;padding:8px;border-radius:4px;white-space:pre-wrap;word-break:break-all;font:12.5px Consolas,monospace;margin:6px 0}
input[type=number]{width:4em;font:inherit}.mo{color:#58635b;font-size:13px}
</style></head><body>
<h1>Camera Hướng <span id="so"></span>: thu ảnh và chỉnh vùng đếm</h1>
<p class="mo" id="bo"></p>
<p class="mo">Khung xanh lá là ROI, xanh dương là làn ngoài (s), hồng là làn trong (l). Đường liền là số đang nạp trong board, đường đứt là số vừa vẽ.</p>
<div class="hai">
<div class="o"><h2>Toàn khung 320 × 240</h2>
<canvas id="khung" width="640" height="480"></canvas>
<p><button id="c_roi">Vẽ ROI (kéo chuột)</button><button id="c_ngoai">Bấm 4 góc làn ngoài</button><button id="c_trong">Bấm 4 góc làn trong</button><button id="c_bo">Bỏ vẽ</button></p>
<p>Ảnh: <label><input type="checkbox" id="ld"> Lật dọc</label> <label><input type="checkbox" id="ln"> Lật ngang</label>
<button id="xoay">Xoay 180°</button> <span class="mo">(lưu vào board ngay; đổi lật xong phải vẽ lại vùng)</span></p>
<p><button id="luu_vung" class="chon">Lưu vào board</button><button id="ve_md">Về số mặc định</button> <span id="tb_vung" class="mo"></span></p>
<p class="mo">Đang dùng: <b id="nguon"></b>. Lưu vào board là có tác dụng ngay, tắt máy không mất. Muốn ghi hẳn vào code thì dán dòng này đè lên dòng <b>CAM <span id="so2"></span></b> trong cau_hinh.h:</p>
<pre id="dong"></pre><button id="chep">Chép dòng trên</button></div>
<div class="o"><h2>Ảnh ROI đang nạp trong board</h2>
<img id="roi" alt="Ảnh ROI" style="min-width:160px;min-height:160px;image-rendering:pixelated">
<p><button id="luu">Lưu 1 ảnh</button></p>
<p><label><input type="checkbox" id="tu_dong"> Tự lưu mỗi <input type="number" id="giay" value="2" min="1" max="60"> giây</label></p>
<p>Đã lưu: <b id="dem">0</b> ảnh (vào thư mục Tải xuống của máy tính)</p>
<p class="mo">Chỉ thu ảnh huấn luyện sau khi ROI đã đúng và đã nạp lại. Chụp đủ các trường hợp: 0 đến 4 xe mỗi làn, có xe ưu tiên, và cả lúc đèn đỏ, vàng, xanh.</p></div>
<div class="o"><h2>Board</h2>
<p>Chế độ: <button data-cd="1">Thu ảnh</button><button data-cd="2">Đếm</button><button data-cd="3">Giả lập</button> <button id="doi_id">Đổi số CAM</button></p>
<p class="mo">Đổi chế độ hay số CAM thì board lưu lại rồi khởi động lại (khoảng 10 giây).</p>
<p id="nap_khoi">Nạp firmware: <input type="file" id="f_nap" accept=".bin"> <button id="nut_nap" class="chon">Nạp</button> <span id="tb_nap" class="mo"></span></p>
<p class="mo">Tệp camera.ino.bin xuất từ Arduino IDE (Sketch &gt; Export Compiled Binary). Camera kiểm tra tệp trước khi ghi; bản mới không chạy ổn trong 2 phút thì tự quay về bản cũ. Có trung tâm thì nên nạp trên trang trung tâm.</p></div>
</div>
<script>
var CH=null,ve={roi:null,ngoai:[],trong:[]},cong_cu='',keo=null,dem=0;
var o=document.getElementById('khung'),c=o.getContext('2d'),anh=null;
function $(i){return document.getElementById(i)}
function tuGiac(q,mau,dut){if(!q||!q.length)return;c.strokeStyle=mau;c.lineWidth=2;c.setLineDash(dut?[6,4]:[]);c.beginPath();
 q.forEach(function(p,i){i?c.lineTo(p[0]*2,p[1]*2):c.moveTo(p[0]*2,p[1]*2)});if(q.length==4)c.closePath();c.stroke();
 c.fillStyle=mau;q.forEach(function(p){c.fillRect(p[0]*2-3,p[1]*2-3,6,6)})}
function veLai(){if(anh)c.drawImage(anh,0,0,640,480);if(!CH)return;
 c.setLineDash([]);c.strokeStyle='#39d353';c.lineWidth=2;c.strokeRect(CH.roi[0]*2,CH.roi[1]*2,CH.roi[2]*2,CH.roi[3]*2);
 tuGiac(CH.ngoai,'#40c4ff',false);tuGiac(CH.trong,'#ff78dc',false);
 if(ve.roi){c.setLineDash([6,4]);c.strokeStyle='#39d353';c.strokeRect(ve.roi[0]*2,ve.roi[1]*2,ve.roi[2]*2,ve.roi[3]*2)}
 tuGiac(ve.ngoai,'#40c4ff',true);tuGiac(ve.trong,'#ff78dc',true);c.setLineDash([])}
function so20(){var r=ve.roi||CH.roi,a=ve.ngoai.length==4?ve.ngoai:CH.ngoai,b=ve.trong.length==4?ve.trong:CH.trong;
 return r.concat([].concat.apply([],a),[].concat.apply([],b)).join(',')}
var CD=['','thu ảnh','đếm','giả lập'];
function napCH(){return fetch('cau_hinh.json?t='+Date.now()).then(function(r){return r.json()}).then(function(d){CH=d;$('so').textContent=d.id;$('so2').textContent=d.id;
 $('bo').innerHTML='CAM '+d.id+' · chế độ '+CD[d.cd]+(d.bl?' (đã chọn chế độ đếm nhưng firmware chưa có mô hình)':'')+' · firmware bản '+d.fw+(d.mh?'':' · chưa có mô hình')+(d.xn?'':' · <b>bản mới, đang tự kiểm tra</b>')+(d.rb?' · <b>lần nạp gần nhất lỗi, đã quay về bản này</b>':'');
 $('nguon').textContent=d.luu?'vùng đã lưu trong board':'số trong cau_hinh.h';$('ld').checked=!!d.ld;$('ln').checked=!!d.ln;inDong();veLai()})}
function lat(doc,ngang){$('tb_vung').textContent='Đang lật ảnh...';fetch('lat',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'doc='+(doc?1:0)+'&ngang='+(ngang?1:0)})
 .then(function(r){return r.text()}).then(function(t){$('tb_vung').textContent=t;return napCH()})}
$('ld').onchange=$('ln').onchange=function(){lat($('ld').checked,$('ln').checked)};
$('xoay').onclick=function(){lat(!$('ld').checked,!$('ln').checked)};
$('luu_vung').onclick=function(){if(!CH)return;$('tb_vung').textContent='Đang lưu...';
 fetch('luu_vung',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'v='+so20()}).then(function(r){return r.text().then(function(t){if(!r.ok)throw t;return t})})
 .then(function(t){ve={roi:null,ngoai:[],trong:[]};$('tb_vung').textContent=t;return napCH()}).catch(function(e){$('tb_vung').textContent='Không lưu được: '+e})};
$('ve_md').onclick=function(){if(!confirm('Bỏ vùng và kiểu lật đã lưu trong board, quay về số trong cau_hinh.h?'))return;
 fetch('xoa_vung',{method:'POST'}).then(function(r){return r.text()}).then(function(t){$('tb_vung').textContent=t;ve={roi:null,ngoai:[],trong:[]};return napCH()})};
function goc(q){return '{'+q.map(function(p){return '{'+p[0]+', '+p[1]+'}'}).join(', ')+'}'}
function inDong(){if(!CH)return;var r=ve.roi||CH.roi,a=ve.ngoai.length==4?ve.ngoai:CH.ngoai,b=ve.trong.length==4?ve.trong:CH.trong;
 $('dong').textContent='  { '+r.join(', ')+', '+goc(a)+', '+goc(b)+' },   // CAM '+CH.id}
function chon(t){cong_cu=t;['roi','ngoai','trong'].forEach(function(k){$('c_'+k).className=k==t?'chon':''});if(t=='ngoai'||t=='trong')ve[t]=[]}
$('c_roi').onclick=function(){chon('roi')};$('c_ngoai').onclick=function(){chon('ngoai')};$('c_trong').onclick=function(){chon('trong')};
$('c_bo').onclick=function(){ve={roi:null,ngoai:[],trong:[]};chon('');inDong();veLai()};
function toaDo(e){var k=o.getBoundingClientRect();return[Math.max(0,Math.min(319,Math.round((e.clientX-k.left)/k.width*320))),Math.max(0,Math.min(239,Math.round((e.clientY-k.top)/k.height*240)))]}
o.onpointerdown=function(e){var p=toaDo(e);if(cong_cu=='roi'){keo=p;ve.roi=[p[0],p[1],1,1]}
 else if(cong_cu=='ngoai'||cong_cu=='trong'){if(ve[cong_cu].length<4)ve[cong_cu].push(p);if(ve[cong_cu].length==4)chon('')}inDong();veLai()};
o.onpointermove=function(e){if(!keo)return;var p=toaDo(e);ve.roi=[Math.min(keo[0],p[0]),Math.min(keo[1],p[1]),Math.max(16,Math.abs(p[0]-keo[0])),Math.max(16,Math.abs(p[1]-keo[1]))];inDong();veLai()};
o.onpointerup=function(){if(keo){keo=null;chon('')}};
$('chep').onclick=function(){var t=$('dong').textContent;if(navigator.clipboard)navigator.clipboard.writeText(t);else{var r=document.createRange();r.selectNode($('dong'));getSelection().removeAllRanges();getSelection().addRange(r)}};
function layKhung(){fetch('khung.jpg?t='+Date.now()).then(function(r){return r.blob()}).then(function(b){return createImageBitmap(b)}).then(function(a){anh=a;veLai()}).catch(function(){}).then(function(){setTimeout(layKhung,400)})}
function layRoi(){fetch('roi.jpg?t='+Date.now()).then(function(r){return r.blob()}).then(function(b){var u=URL.createObjectURL(b),i=$('roi');i.onload=function(){URL.revokeObjectURL(u)};i.src=u}).catch(function(){}).then(function(){setTimeout(layRoi,1200)})}
function luu(){return fetch('roi.jpg?t='+Date.now()).then(function(r){return r.blob()}).then(function(b){dem++;var a=document.createElement('a');a.href=URL.createObjectURL(b);
 a.download='cam'+CH.id+'_'+('000'+dem).slice(-4)+'.jpg';document.body.appendChild(a);a.click();a.remove();$('dem').textContent=dem})}
$('luu').onclick=luu;
(function lap(){var g=Math.max(1,+$('giay').value||2);setTimeout(function(){($('tu_dong').checked&&CH?luu():Promise.resolve()).catch(function(){}).then(lap)},g*1000)})();
document.addEventListener('click',function(e){var b=e.target.closest('button');if(!b||!b.dataset.cd)return;
 if(!confirm('Đổi sang chế độ '+CD[b.dataset.cd]+'? Camera sẽ khởi động lại.'))return;
 fetch('chedo',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'cd='+b.dataset.cd}).then(function(r){return r.text()}).then(function(t){$('bo').textContent=t})});
$('doi_id').onclick=function(){var m=prompt('Số CAM mới (1 đến 4):');if(!m||!(+m>=1&&+m<=4))return;
 fetch('doiid',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'moi='+(+m)}).then(function(r){return r.text()}).then(function(t){$('bo').textContent=t})};
if(location.pathname.indexOf('/c')==0)$('nap_khoi').innerHTML='Nạp firmware: mở qua trung tâm thì nạp ở mục "Nạp firmware qua WiFi" trên trang trung tâm.';
else $('nut_nap').onclick=function(){var f=$('f_nap').files[0];if(!f){$('tb_nap').textContent='Chọn tệp .bin trước';return}
 if(!confirm('Nạp '+f.name+' cho camera này?'))return;var x=new XMLHttpRequest();x.open('POST','nap');
 x.upload.onprogress=function(e){if(e.lengthComputable)$('tb_nap').textContent='Đang tải lên '+Math.round(e.loaded*100/e.total)+' %'};
 x.onload=function(){$('tb_nap').textContent=x.responseText};x.onerror=function(){$('tb_nap').textContent='Mất kết nối (nếu đã tải đủ thì camera đang khởi động lại)'};
 var fd=new FormData();fd.append('f',f,f.name);x.send(fd)};
napCH().then(function(){layKhung();layRoi()});
</script></body></html>)HTML";

static void guiJpeg(uint8_t *jpg, size_t dai) {
  may_chu.sendHeader("Cache-Control", "no-store");
  mangChoPhepTrungTam();
  may_chu.send_P(200, "image/jpeg", (const char *)jpg, dai);
}

static void xuLyKhung() {              // toàn khung, để chỉnh ROI và làn
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) { may_chu.send(503, "text/plain", "Khong chup duoc anh"); return; }
  if (fb->format == PIXFORMAT_JPEG) {  // ESP32-CAM: khung đã là JPEG, gửi thẳng
    guiJpeg(fb->buf, fb->len);
    esp_camera_fb_return(fb);
    return;
  }
  uint8_t *jpg = NULL;
  size_t dai = 0;
  bool duoc = frame2jpg(fb, 80, &jpg, &dai);
  esp_camera_fb_return(fb);
  if (!duoc) { may_chu.send(500, "text/plain", "Khong nen duoc anh"); return; }
  guiJpeg(jpg, dai);
  free(jpg);
}

static void xuLyRoi() {                // ảnh ROI cỡ gốc, để làm dữ liệu huấn luyện
  const CauHinhCam &ch = ch_cam;
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) { may_chu.send(503, "text/plain", "Khong chup duoc anh"); return; }
  bool giai_nen = catRoi(fb, ch, anh_roi, ch.roi_w, ch.roi_h);
  esp_camera_fb_return(fb);
  if (!giai_nen) { may_chu.send(500, "text/plain", "Khong giai nen duoc anh JPEG"); return; }
  uint8_t *jpg = NULL;
  size_t dai = 0;
  if (!fmt2jpg(anh_roi, (size_t)ch.roi_w * ch.roi_h * 3, ch.roi_w, ch.roi_h, PIXFORMAT_RGB888, 92, &jpg, &dai)) {
    may_chu.send(500, "text/plain", "Khong nen duoc anh");
    return;
  }
  guiJpeg(jpg, dai);
  free(jpg);
}

static void xuLyCauHinh() {
  const CauHinhCam &ch = ch_cam;
  char b[520];
  snprintf(b, sizeof(b),
           "{\"id\":%d,\"cd\":%d,\"bl\":%d,\"fw\":\"%s\",\"mh\":%d,\"xn\":%d,\"rb\":%d,\"luu\":%d,\"ld\":%d,\"ln\":%d,\"roi\":[%d,%d,%d,%d],\"ngoai\":[[%d,%d],[%d,%d],[%d,%d],[%d,%d]],\"trong\":[[%d,%d],[%d,%d],[%d,%d],[%d,%d]]}",
           cam_id, che_do, che_do_bi_doi ? 1 : 0, phienBanFw(), CO_MO_HINH, nap_cho_xn ? 0 : 1, nap_co_ban_loi ? 1 : 0,
           vung_da_luu ? 1 : 0, lat_doc, lat_ngang, ch.roi_x, ch.roi_y, ch.roi_w, ch.roi_h,
           ch.lan_ngoai[0].x, ch.lan_ngoai[0].y, ch.lan_ngoai[1].x, ch.lan_ngoai[1].y,
           ch.lan_ngoai[2].x, ch.lan_ngoai[2].y, ch.lan_ngoai[3].x, ch.lan_ngoai[3].y,
           ch.lan_trong[0].x, ch.lan_trong[0].y, ch.lan_trong[1].x, ch.lan_trong[1].y,
           ch.lan_trong[2].x, ch.lan_trong[2].y, ch.lan_trong[3].x, ch.lan_trong[3].y);
  mangChoPhepTrungTam();
  may_chu.send(200, "application/json", b);
}

static void xuLyLuuVung() {            // v = 20 số: ROI, 4 góc làn ngoài, 4 góc làn trong
  CauHinhCam c;
  if (!vungDocChuoi(may_chu.arg("v").c_str(), c)) {
    may_chu.send(400, "text/plain; charset=utf-8", "Số không hợp lệ: ROI phải nằm trong khung 320 × 240, rộng và cao ít nhất 16, đủ 4 góc mỗi làn.");
    return;
  }
  if (!vungLuu(c)) { may_chu.send(500, "text/plain; charset=utf-8", "Không ghi được vào bộ nhớ của board."); return; }
  Serial.printf("Đã lưu vùng đếm mới: ROI %d,%d,%d,%d\n", c.roi_x, c.roi_y, c.roi_w, c.roi_h);
  mangGuiTrangThai();
  may_chu.send(200, "text/plain; charset=utf-8", "Đã lưu vào board, đang dùng vùng mới.");
}

static void xuLyLat() {
  bool duoc = latDat(may_chu.arg("doc").toInt() ? 1 : 0, may_chu.arg("ngang").toInt() ? 1 : 0);
  Serial.printf("Lật ảnh: dọc %d, ngang %d\n", lat_doc, lat_ngang);
  mangGuiTrangThai();
  may_chu.send(duoc ? 200 : 500, "text/plain; charset=utf-8",
               duoc ? "Đã lật ảnh và lưu vào board. Nhớ vẽ lại ROI và 2 làn." : "Đã lật ảnh nhưng không lưu được vào board.");
}

static void xuLyCheDo() {
  int cd = may_chu.arg("cd").toInt();
  if (!cheDoHopLe(cd)) { may_chu.send(400, "text/plain; charset=utf-8", "Chế độ sai."); return; }
  if (cd == CHE_DO_DEM && !CO_MO_HINH) {
    may_chu.send(400, "text/plain; charset=utf-8", "Firmware này chưa có mô hình: cài thư viện Edge Impulse, biên dịch rồi nạp lại.");
    return;
  }
  danhTinhDat("cd", (uint8_t)cd);
  may_chu.send(200, "text/plain; charset=utf-8", "Đã lưu, camera đang khởi động lại...");
  delay(500);
  ESP.restart();
}

static void xuLyDoiId() {
  int moi = may_chu.arg("moi").toInt();
  if (moi < 1 || moi > 4) { may_chu.send(400, "text/plain; charset=utf-8", "Số CAM từ 1 đến 4."); return; }
  danhTinhDat("id", (uint8_t)moi);
  may_chu.send(200, "text/plain; charset=utf-8", "Đã lưu, camera khởi động lại thành CAM " + String(moi) + ". Địa chỉ mới: 192.168.4." + String(10 + moi));
  delay(500);
  ESP.restart();
}

static void xuLyXoaVung() {
  vungXoa();
  mangGuiTrangThai();
  may_chu.send(200, "text/plain; charset=utf-8", "Đã về số trong cau_hinh.h.");
}

// Mở trang web trên camera. Gọi sau mangKhoiTao().
inline bool trangWebKhoiTao() {
  if (!anh_roi) anh_roi = (uint8_t *)ps_malloc(ANH_RONG * ANH_CAO * 3);
  if (!anh_roi) return false;
  may_chu.on("/", []() { may_chu.send_P(200, "text/html; charset=utf-8", TRANG_THU_ANH); });
  may_chu.on("/khung.jpg", xuLyKhung);
  may_chu.on("/roi.jpg", xuLyRoi);
  may_chu.on("/cau_hinh.json", xuLyCauHinh);
  may_chu.on("/luu_vung", HTTP_POST, xuLyLuuVung);
  may_chu.on("/xoa_vung", HTTP_POST, xuLyXoaVung);
  may_chu.on("/lat", HTTP_POST, xuLyLat);
  may_chu.on("/chedo", HTTP_POST, xuLyCheDo);
  may_chu.on("/doiid", HTTP_POST, xuLyDoiId);
  may_chu.on("/nap", HTTP_POST, []() { napTrucTiepXong(may_chu); }, []() { napNhanTrucTiep(may_chu); });
  may_chu.begin();
  mang_co_web = true;
  return true;
}

// Chế độ thu ảnh: nối WiFi trung tâm (không thấy thì tự phát WiFi riêng) rồi mở trang web.
inline bool thuAnhKhoiTao() {
  mangKhoiTao(true);
  mangGanBaoNap();
  if (!WiFi.isConnected() && !mang_tu_phat) return false;
  if (!trangWebKhoiTao()) return false;
  Serial.printf("Mở http://%s để xem ảnh và chỉnh vùng đếm.\n",
                (mang_tu_phat ? WiFi.softAPIP() : WiFi.localIP()).toString().c_str());
  return true;
}

inline void thuAnhChay() {
  mangChay();
}
