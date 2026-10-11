// thu_anh.h — Chế độ thu ảnh: camera tự phát WiFi và mở một trang web để
//   - xem toàn khung 320 × 240, vẽ ROI và bấm 4 góc từng làn, lấy dòng cấu hình dán vào cau_hinh.h;
//   - xem và lưu ảnh ROI (đúng ảnh sẽ đưa vào mô hình) làm dữ liệu huấn luyện trên Edge Impulse.
#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include "img_converters.h"
#include "may_anh.h"

static WebServer may_chu(80);
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
<p class="mo">Khung xanh lá là ROI, xanh dương là làn ngoài (s), hồng là làn trong (l). Đường liền là số đang nạp trong board, đường đứt là số vừa vẽ.</p>
<div class="hai">
<div class="o"><h2>Toàn khung 320 × 240</h2>
<canvas id="khung" width="640" height="480"></canvas>
<p><button id="c_roi">Vẽ ROI (kéo chuột)</button><button id="c_ngoai">Bấm 4 góc làn ngoài</button><button id="c_trong">Bấm 4 góc làn trong</button><button id="c_bo">Bỏ vẽ</button></p>
<p>Dán dòng này đè lên dòng <b>CAM <span id="so2"></span></b> trong cau_hinh.h rồi nạp lại:</p>
<pre id="dong"></pre><button id="chep">Chép dòng trên</button></div>
<div class="o"><h2>Ảnh ROI đang nạp trong board</h2>
<img id="roi" alt="Ảnh ROI" style="min-width:160px;min-height:160px;image-rendering:pixelated">
<p><button id="luu">Lưu 1 ảnh</button></p>
<p><label><input type="checkbox" id="tu_dong"> Tự lưu mỗi <input type="number" id="giay" value="2" min="1" max="60"> giây</label></p>
<p>Đã lưu: <b id="dem">0</b> ảnh (vào thư mục Tải xuống của máy tính)</p>
<p class="mo">Chỉ thu ảnh huấn luyện sau khi ROI đã đúng và đã nạp lại. Chụp đủ các trường hợp: 0 đến 4 xe mỗi làn, có xe ưu tiên, và cả lúc đèn đỏ, vàng, xanh.</p></div>
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
function goc(q){return '{'+q.map(function(p){return '{'+p[0]+', '+p[1]+'}'}).join(', ')+'}'}
function inDong(){if(!CH)return;var r=ve.roi||CH.roi,a=ve.ngoai.length==4?ve.ngoai:CH.ngoai,b=ve.trong.length==4?ve.trong:CH.trong;
 $('dong').textContent='  { '+r.join(', ')+', '+goc(a)+', '+goc(b)+' },   // CAM '+CH.id}
function chon(t){cong_cu=t;['roi','ngoai','trong'].forEach(function(k){$('c_'+k).className=k==t?'chon':''});if(t=='ngoai'||t=='trong')ve[t]=[]}
$('c_roi').onclick=function(){chon('roi')};$('c_ngoai').onclick=function(){chon('ngoai')};$('c_trong').onclick=function(){chon('trong')};
$('c_bo').onclick=function(){ve={roi:null,ngoai:[],trong:[]};chon('');inDong();veLai()};
function toaDo(e){var k=o.getBoundingClientRect();return[Math.max(0,Math.min(319,Math.round((e.clientX-k.left)/k.width*320))),Math.max(0,Math.min(239,Math.round((e.clientY-k.top)/k.height*240)))]}
o.onpointerdown=function(e){var p=toaDo(e);if(cong_cu=='roi'){keo=p;ve.roi=[p[0],p[1],1,1]}
 else if(cong_cu=='ngoai'||cong_cu=='trong'){if(ve[cong_cu].length<4)ve[cong_cu].push(p);if(ve[cong_cu].length==4)chon('')}inDong();veLai()};
o.onpointermove=function(e){if(!keo)return;var p=toaDo(e);ve.roi=[Math.min(keo[0],p[0]),Math.min(keo[1],p[1]),Math.max(8,Math.abs(p[0]-keo[0])),Math.max(8,Math.abs(p[1]-keo[1]))];inDong();veLai()};
o.onpointerup=function(){if(keo){keo=null;chon('')}};
$('chep').onclick=function(){var t=$('dong').textContent;if(navigator.clipboard)navigator.clipboard.writeText(t);else{var r=document.createRange();r.selectNode($('dong'));getSelection().removeAllRanges();getSelection().addRange(r)}};
function layKhung(){fetch('/khung.jpg?t='+Date.now()).then(function(r){return r.blob()}).then(function(b){return createImageBitmap(b)}).then(function(a){anh=a;veLai()}).catch(function(){}).then(function(){setTimeout(layKhung,400)})}
function layRoi(){fetch('/roi.jpg?t='+Date.now()).then(function(r){return r.blob()}).then(function(b){var u=URL.createObjectURL(b),i=$('roi');i.onload=function(){URL.revokeObjectURL(u)};i.src=u}).catch(function(){}).then(function(){setTimeout(layRoi,1200)})}
function luu(){return fetch('/roi.jpg?t='+Date.now()).then(function(r){return r.blob()}).then(function(b){dem++;var a=document.createElement('a');a.href=URL.createObjectURL(b);
 a.download='cam'+CH.id+'_'+('000'+dem).slice(-4)+'.jpg';document.body.appendChild(a);a.click();a.remove();$('dem').textContent=dem})}
$('luu').onclick=luu;
(function lap(){var g=Math.max(1,+$('giay').value||2);setTimeout(function(){($('tu_dong').checked&&CH?luu():Promise.resolve()).catch(function(){}).then(lap)},g*1000)})();
fetch('/cau_hinh.json').then(function(r){return r.json()}).then(function(d){CH=d;$('so').textContent=d.id;$('so2').textContent=d.id;inDong();layKhung();layRoi()});
</script></body></html>)HTML";

static void guiJpeg(uint8_t *jpg, size_t dai) {
  may_chu.sendHeader("Cache-Control", "no-store");
  may_chu.send_P(200, "image/jpeg", (const char *)jpg, dai);
}

static void xuLyKhung() {              // toàn khung, để chỉnh ROI và làn
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) { may_chu.send(503, "text/plain", "Khong chup duoc anh"); return; }
  uint8_t *jpg = NULL;
  size_t dai = 0;
  bool duoc = frame2jpg(fb, 80, &jpg, &dai);
  esp_camera_fb_return(fb);
  if (!duoc) { may_chu.send(500, "text/plain", "Khong nen duoc anh"); return; }
  guiJpeg(jpg, dai);
  free(jpg);
}

static void xuLyRoi() {                // ảnh ROI cỡ gốc, để làm dữ liệu huấn luyện
  const CauHinhCam &ch = CAU_HINH_CAM[CAM_ID];
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) { may_chu.send(503, "text/plain", "Khong chup duoc anh"); return; }
  catRoi(fb, ch, anh_roi, ch.roi_w, ch.roi_h);
  esp_camera_fb_return(fb);
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
  const CauHinhCam &ch = CAU_HINH_CAM[CAM_ID];
  char b[360];
  snprintf(b, sizeof(b),
           "{\"id\":%d,\"roi\":[%d,%d,%d,%d],\"ngoai\":[[%d,%d],[%d,%d],[%d,%d],[%d,%d]],\"trong\":[[%d,%d],[%d,%d],[%d,%d],[%d,%d]]}",
           CAM_ID, ch.roi_x, ch.roi_y, ch.roi_w, ch.roi_h,
           ch.lan_ngoai[0].x, ch.lan_ngoai[0].y, ch.lan_ngoai[1].x, ch.lan_ngoai[1].y,
           ch.lan_ngoai[2].x, ch.lan_ngoai[2].y, ch.lan_ngoai[3].x, ch.lan_ngoai[3].y,
           ch.lan_trong[0].x, ch.lan_trong[0].y, ch.lan_trong[1].x, ch.lan_trong[1].y,
           ch.lan_trong[2].x, ch.lan_trong[2].y, ch.lan_trong[3].x, ch.lan_trong[3].y);
  may_chu.send(200, "application/json", b);
}

inline bool thuAnhKhoiTao() {
  anh_roi = (uint8_t *)ps_malloc(ANH_RONG * ANH_CAO * 3);
  if (!anh_roi) return false;
  char ten[24];
  snprintf(ten, sizeof(ten), "CAM%d-SaBan", CAM_ID);
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(ten, MAT_KHAU_WIFI, KENH_WIFI)) return false;
  may_chu.on("/", []() { may_chu.send_P(200, "text/html; charset=utf-8", TRANG_THU_ANH); });
  may_chu.on("/khung.jpg", xuLyKhung);
  may_chu.on("/roi.jpg", xuLyRoi);
  may_chu.on("/cau_hinh.json", xuLyCauHinh);
  may_chu.begin();
  Serial.printf("Đã phát WiFi \"%s\", mật khẩu \"%s\". Nối máy tính vào WiFi đó rồi mở http://%s\n",
                ten, MAT_KHAU_WIFI, WiFi.softAPIP().toString().c_str());
  return true;
}

inline void thuAnhChay() {
  may_chu.handleClient();
}
