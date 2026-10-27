// web.h — Trang web điều khiển của board trung tâm: http://192.168.4.1 (nối vào WiFi "TRUNG-TAM-SaBan").
// Trên một trang: xem đèn và số xe của 4 hướng, đổi chế độ và kiểu chia pha, gõ tay số xe, và gửi lệnh
// cho từng camera hoặc cả 4 camera cùng lúc (nháy LED, lật ảnh, khởi động lại, đặt số đếm giả), mở trang
// chọn vùng đếm của từng camera, xem ảnh của cả 4 camera.
//
// Máy chủ web chạy ở một tác vụ riêng trên lõi 0, vòng loop() điều khiển đèn chạy trên lõi 1 nên trang web
// không làm chậm đèn. Hai bên chỉ trao đổi qua:
//   - hàng đợi lệnh: trang web gửi một dòng lệnh y như gõ ở Serial ("codinh", "kieu tuhoc", "{\"id\":1,...}"),
//     loop() lấy ra và chạy bằng đúng hàm xuLyLenh();
//   - bản chụp trạng thái: loop() viết chuỗi JSON 2 lần mỗi giây, trang web chỉ chép ra.
// Lệnh cho camera đi thẳng từ tác vụ web bằng ESP-NOW phát chung (guiLenhCam trong nhan_tin.h).
//
// Nạp firmware qua WiFi (nap_tu_xa.h): /nap/tt (firmware trung tâm), /nap/cam (firmware camera, cất trong FFat),
// /fw/cam.bin (camera tải về), /nap/quayve. Nhật ký sự kiện: /nhat_ky.json. Danh sách thông số đèn: /muc_ts.json.
//
// Chuyển tiếp camera: http://<trung tâm>/c2/khung.jpg được trung tâm lấy từ http://192.168.4.12/khung.jpg rồi
// trả lại, nên máy nối trung tâm qua hotspot (không thấy được mạng 192.168.4.x) vẫn xem ảnh và chọn vùng được.
#pragma once
#include <WebServer.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include "cau_hinh_mang.h"
#include "nhan_tin.h"
#include "nap_tu_xa.h"
#include "thong_so.h"
#include "nhat_ky.h"

static WebServer may_chu_web(80);
static QueueHandle_t hang_doi_lenh_web = NULL;
static SemaphoreHandle_t khoa_trang_thai = NULL;
static char trang_thai_web[4096] = "{}";
struct LenhWeb { char dong[160]; };

// ---------- Hotspot của máy tính ----------
static uint8_t wn_tt = 0;          // 0: không nối (chưa điền tên, hoặc tìm không thấy nên thôi), 1: đang tìm, 2: đã nối
static uint32_t wn_t = 0;

inline void wifiNgoaiThu() {
  if (!TEN_WIFI_NGOAI[0]) { Serial.println("Chưa điền TEN_WIFI_NGOAI trong cau_hinh_mang.h."); return; }
  WiFi.setAutoReconnect(true);
  WiFi.begin(TEN_WIFI_NGOAI, MAT_KHAU_WIFI_NGOAI);
  wn_tt = 1; wn_t = millis();
  Serial.printf("Đang tìm hotspot \"%s\"...\n", TEN_WIFI_NGOAI);
}

// Gọi trong loop().
inline void wifiNgoaiChay() {
  if (wn_tt == 0) return;
  if (WiFi.status() == WL_CONNECTED) {
    if (wn_tt != 2) {
      wn_tt = 2;
      Serial.printf("Đã nối hotspot \"%s\" (kênh %d). Trên máy tính mở http://%s hoặc http://%s.local\n", TEN_WIFI_NGOAI,
                    (int)WiFi.channel(), WiFi.localIP().toString().c_str(), TEN_MDNS);
    }
    wn_t = millis();
    return;
  }
  if (wn_tt == 2) { wn_tt = 1; wn_t = millis(); Serial.println("Mất hotspot, đang tìm lại..."); }
  if (millis() - wn_t >= CHO_WIFI_NGOAI_MS) {
    WiFi.setAutoReconnect(false);
    WiFi.disconnect();                // chỉ ngắt phần nối hotspot, WiFi riêng của trung tâm vẫn phát
    wn_tt = 0;
    Serial.println("Không thấy hotspot (đã bật chưa? có ở băng 2,4 GHz không?). Thôi tìm để camera không mất tin; gõ \"wifi\" để thử lại.");
  }
}

static const char TRANG_TRUNG_TAM[] PROGMEM = R"HTML(<!doctype html>
<html lang="vi"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Trung tâm sa bàn</title>
<style>
:root{--nen:#f4f6f2;--the:#fff;--vien:#c9d0c5;--chu:#1c221e;--mo:#5c665f;--nhan:#1b5fae;--do:#e0362c;--vang:#f2b705;--xanh:#18a957;--tat:#3a3f3b}
*{box-sizing:border-box}body{font:15px/1.45 system-ui,Segoe UI,sans-serif;margin:0;background:var(--nen);color:var(--chu)}
header{background:#1f2a24;color:#fff;padding:12px 16px}h1{font-size:18px;margin:0}#tom{font-size:13px;color:#cfd8cf;margin-top:2px}
main{padding:12px 16px;max-width:1100px;margin:auto}h2{font-size:15px;margin:18px 0 8px}
.luoi{display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));gap:12px}
.the{background:var(--the);border:1px solid var(--vien);border-radius:8px;padding:12px}
.the h3{font-size:15px;margin:0 0 8px;display:flex;justify-content:space-between;gap:8px}
.cum{display:flex;gap:16px;align-items:center;margin-bottom:6px}.den{display:flex;flex-direction:column;gap:3px;background:var(--tat);padding:4px;border-radius:6px}
.bong{width:14px;height:14px;border-radius:50%;background:#555}.ten{font-size:12px;color:var(--mo);text-align:center}
.so{font-size:13px}.so b{font-size:17px}.ut{background:#fde3e1;color:#9c1c14;border-radius:4px;padding:1px 6px;font-size:12px}
.cam{font-size:12.5px;color:var(--mo);margin:6px 0;border-top:1px dashed var(--vien);padding-top:6px}.cam .tot{color:#127a40}.cam .xau{color:#b3261e}
button{font:inherit;font-size:13.5px;padding:5px 9px;border:1px solid #9aa59d;border-radius:5px;background:#fff;cursor:pointer;margin:2px 2px 2px 0}
button.chon{background:var(--nhan);color:#fff;border-color:var(--nhan)}button:active{transform:translateY(1px)}
a.nut{display:inline-block;font-size:13.5px;padding:5px 9px;border:1px solid var(--nhan);border-radius:5px;color:var(--nhan);text-decoration:none;margin:2px 2px 2px 0}
input[type=number]{width:3.4em;font:inherit;padding:3px}select{font:inherit;padding:3px}
img.anh{display:block;width:100%;aspect-ratio:4/3;background:#222;border-radius:4px;margin-top:6px;object-fit:cover}
#bao{position:fixed;left:50%;bottom:16px;transform:translateX(-50%);background:#1f2a24;color:#fff;padding:8px 14px;border-radius:6px;font-size:13.5px;opacity:0;transition:opacity .25s;pointer-events:none;max-width:90vw}
.hang{margin:6px 0}.mo{color:var(--mo);font-size:13px}
.ao{background:#e3ecfb;color:#163e75;border-radius:4px;padding:1px 6px;font-size:12px}.gia{background:#efe6c8;color:#5a4500;border-radius:4px;padding:1px 6px;font-size:12px}
table.ts{border-collapse:collapse;font-size:13.5px}table.ts td{padding:3px 8px 3px 0}table.ts input{width:4.5em}
table.nc{border-collapse:collapse;font-size:13px;width:100%}table.nc td,table.nc th{text-align:left;padding:4px 6px;border-top:1px solid var(--vien)}
#nk{font:12.5px/1.5 ui-monospace,Consolas,monospace;background:#1f2a24;color:#dfe8df;border-radius:6px;padding:8px 10px;max-height:260px;overflow:auto;white-space:pre-wrap}
.canh{color:#9c1c14;font-weight:600}
</style></head><body>
<header><h1>Trung tâm đèn giao thông thông minh</h1><div id="tom">Đang kết nối...</div><div id="wn" style="font-size:12.5px;color:#aab8ad"></div></header>
<main>
<div class="luoi" id="huong"></div>
<h2>Cả 4 camera cùng lúc</h2>
<div class="the">
<button data-c="0" data-l="xem">Hỏi trạng thái</button><button data-c="0" data-l="nhay">Nháy LED đỏ</button>
<button data-c="0" data-l="lat" data-x="doc=1&ngang=1">Lật 180°</button><button data-c="0" data-l="lat" data-x="doc=0&ngang=0">Bỏ lật</button>
<button data-c="0" data-l="khoidong">Khởi động lại 4 camera</button><button id="kd5">Khởi động lại cả 5 board</button>
<p class="hang"><label><input type="checkbox" id="xem_anh"> Xem ảnh 4 camera</label> <span class="mo">(camera đang đếm sẽ chậm đi một chút khi bật)</span></p>
<p class="mo">Lệnh gửi một gói ESP-NOW phát chung nên cả 4 camera nhận cùng lúc. "Lật" đổi hướng ảnh và lưu vào board; lật xong phải vẽ lại vùng đếm.</p>
</div>
<h2>Đèn</h2>
<div class="the">
<div class="hang">Chế độ: <button data-d="codinh" id="cd0">Cố định</button><button data-d="thichung" id="cd1">Thích ứng</button></div>
<div class="hang">Kiểu chia pha: <button data-d="kieu rieng" id="k1">Riêng từng hướng</button><button data-d="kieu thangtruoc" id="k2">Thẳng trước</button><button data-d="kieu doixung" id="k3">Đối xứng</button><button data-d="kieu tuhoc" id="k0">Tự học</button></div>
<div class="hang">Tua nhanh <select id="tua"><option>1</option><option>2</option><option>4</option><option>8</option></select>
 &nbsp;Cắt xanh sớm sau <input type="number" id="cs" min="0" max="60"> s <button id="dat_cs">Đặt</button></div>
<div class="hang"><button data-d="xoa">Bỏ số gõ tay</button><button data-d="thuday">Thử dây 24 bóng</button><button data-d="wifi">Nối lại hotspot</button></div>
<p class="mo">Đổi chế độ, kiểu chia pha có tác dụng từ lượt sau. Thử dây mất khoảng 10 giây, xong đèn chạy lại từ đầu.</p>
</div>
<h2>Giả lập (trình diễn, thử khi thiếu camera)</h2>
<div class="the">
<div class="hang"><button id="tha_nn" class="chon">Thả xe ưu tiên ngẫu nhiên</button><button data-d="boxeao">Bỏ mọi xe ưu tiên ảo</button>
 &nbsp;Tự thả mỗi <select id="tutha"><option value="0">tắt</option><option value="1">1 phút</option><option value="2">2 phút</option><option value="3">3 phút</option><option value="5">5 phút</option></select></div>
<div class="hang"><label><input type="checkbox" id="dong"> Dòng xe giả cho hướng không có camera</label>, mật độ <select id="md"><option value="20">thưa</option><option value="35">vừa</option><option value="55">đông</option><option value="75">rất đông</option></select></div>
<p class="mo">Xe ưu tiên ảo: hướng và loại ngẫu nhiên (hoặc chọn ở thẻ từng hướng), bộ điều khiển thấy y như camera báo; được xanh ưu tiên 3 giây là coi như xe đã qua và đèn chuyển vàng. Dòng xe giả: hướng nào camera không gửi tin thì có xe tới ngẫu nhiên, xe đi khi đèn của làn đó xanh; camera gửi tin lại là dùng số thật. <span id="ao_tk"></span></p>
</div>
<h2>Thông số đèn</h2>
<div class="the">
<table class="ts" id="ts"></table>
<div class="hang"><button id="ap_ts" class="chon">Áp dụng</button><button data-d="luuts">Lưu vào board</button><button data-d="tsmacdinh">Về mặc định</button></div>
<p class="mo">Tính bằng giây. Áp dụng có tác dụng từ bước sau; "Lưu vào board" giữ cả chế độ, kiểu chia pha qua lần tắt máy. Có giới hạn an toàn: vàng 3–6 s, đỏ toàn phần 1–5 s, xanh ngắn nhất 5–60 s.</p>
</div>
<h2>Nạp firmware qua WiFi</h2>
<div class="the">
<div class="hang"><b>Trung tâm</b>: <span id="nap_tt"></span></div>
<div class="hang"><input type="file" id="f_tt" accept=".bin"> <button id="nap_tt_nut" class="chon">Nạp cho trung tâm</button><button id="quayve">Quay về bản trước</button> <span id="nap_tt_tb" class="mo"></span></div>
<div class="hang" style="margin-top:12px"><b>Camera</b>: <span id="nap_cam"></span></div>
<div class="hang"><input type="file" id="f_cam" accept=".bin"> <button id="nap_cam_nut" class="chon">Tải lên trung tâm</button> <span id="nap_cam_tb" class="mo"></span></div>
<table class="nc" id="nc"></table>
<div class="hang"><button id="nap_ca4" class="chon">Nạp cả 4 camera lần lượt</button><button data-d="napdung">Hủy các camera chưa tới lượt</button></div>
<p class="mo">Tệp .bin lấy ở Arduino IDE: Sketch &gt; Export Compiled Binary (tệp <i>.ino.bin</i>, không lấy tệp có chữ bootloader, partitions hay merged). Một tệp camera dùng cho cả 4 camera: số CAM, chế độ, vùng đếm nằm trong bộ nhớ từng board. Trung tâm kiểm tra tệp trước khi ghi; bản mới phải tự xác nhận chạy ổn trong 2 phút, không thì board tự quay về bản cũ. Nạp lần lượt: camera này xong mới tới camera sau, lỗi thì dừng. Lần đầu camera phải được nạp bằng USB với Partition Scheme "Minimal SPIFFS (1.9MB APP with OTA)".</p>
</div>
<h2>Nhật ký</h2>
<div id="nk"></div>
</main><div id="bao"></div>
<script>
var TEN=['','Hướng 1 (từ trên xuống)','Hướng 2 (từ phải)','Hướng 3 (từ dưới)','Hướng 4 (từ trái)'];
var CD=['','thu ảnh','đếm','giả lập'],UT=['','cứu hỏa','quân sự','công an','cứu thương'],MAU=['var(--do)','var(--vang)','var(--xanh)'];
var KIEU=['tự học','riêng từng hướng','thẳng trước','đối xứng'],PHA=['khởi động','xanh','vàng','đỏ toàn phần'];
var S=null,ip=['','','','',''];
function $(i){return document.getElementById(i)}
function bao(t){var b=$('bao');b.textContent=t;b.style.opacity=1;clearTimeout(bao.h);bao.h=setTimeout(function(){b.style.opacity=0},2200)}
function gui(url,body,tb){return fetch(url,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body}).then(function(r){return r.text()}).then(function(t){bao(tb||t)}).catch(function(){bao('Không gửi được: kiểm tra đã nối WiFi trung tâm')})}
function lenh(d,tb){return gui('/lenh','d='+encodeURIComponent(d),tb)}
function lenhCam(c,l,x,tb){return gui('/cam','den='+c+'&lenh='+l+(x?'&'+x:''),tb)}
function den(id,lb){return '<div><div class="den" id="'+id+'"><div class="bong"></div><div class="bong"></div><div class="bong"></div></div><div class="ten">'+lb+'</div></div>'}
function veThe(){var h='';for(var i=1;i<=4;i++)h+='<div class="the"><h3><span>'+TEN[i]+'</span><span id="ut'+i+'"></span></h3>'+
 '<div class="cum">'+den('d'+i+'t','thẳng')+den('d'+i+'r','trái')+'<div class="so">Làn ngoài <b id="s'+i+'">–</b><br>Làn trong <b id="l'+i+'">–</b><br><span class="mo" id="g'+i+'"></span></div></div>'+
 '<div class="cam" id="c'+i+'">CAM '+i+': chưa thấy</div>'+
 '<div><a class="nut" id="a'+i+'" target="_blank" style="display:none">Chọn vùng đếm</a><button data-c="'+i+'" data-l="nhay">Nháy LED</button><button data-c="'+i+'" data-l="lat" data-x="doc=1&ngang=1">Lật 180°</button><button data-c="'+i+'" data-l="khoidong">Khởi động lại</button>'+
 '<br><span class="mo">Chế độ camera:</span> <button data-c="'+i+'" data-l="chedo" data-x="cd=1">Thu ảnh</button><button data-c="'+i+'" data-l="chedo" data-x="cd=2">Đếm</button><button data-c="'+i+'" data-l="chedo" data-x="cd=3">Giả lập</button><button data-id="'+i+'">Đổi số CAM</button></div>'+
 '<div class="hang">Xe ưu tiên: <select id="ul'+i+'"><option value="0">loại ngẫu nhiên</option><option value="1">cứu hỏa</option><option value="2">quân sự</option><option value="3">công an</option><option value="4">cứu thương</option></select> <button data-xa="'+i+'">Thả</button> <span id="ao'+i+'"></span></div>'+
 '<div class="hang mo">Gõ tay: ngoài <input type="number" id="ts'+i+'" min="0" max="9" value="0"> trong <input type="number" id="tl'+i+'" min="0" max="9" value="0"> '+
 '<button data-g="'+i+'">Gửi cho trung tâm</button><button data-gl="'+i+'">Đặt cho CAM giả lập</button></div>'+
 '<img class="anh" id="i'+i+'" alt="" style="display:none"></div>';$('huong').innerHTML=h}
function veDen(id,m){var b=$(id).children;for(var k=0;k<3;k++)b[k].style.background=(k==m)?MAU[m]:'#555'}
function toHop(m){var a=[];for(var h=1;h<=4;h++){var t=m>>((h-1)*2)&1,r=m>>((h-1)*2+1)&1;if(t||r)a.push('H'+h+' '+(t&&r?'thẳng+trái':t?'thẳng':'trái'))}return a.length?a.join(', '):'đỏ toàn phần'}
function capNhat(d){S=d;
 var tom=(d.cd?'Thích ứng, '+KIEU[d.kieu]:'Cố định')+' · '+(d.cd?'lượt ':'chu kì ')+d.ck+', bước '+(d.buoc+1)+'/'+d.sb+' · '+(d.loi?'LỖI: nháy vàng':PHA[d.pha]+' '+toHop(d.pha==2?d.vang:d.xanh)+(d.con>=0?', còn '+d.con+' s':''))+(d.uh?' · ưu tiên Hướng '+d.uh:'');
 if(d.tua>1)tom+=' · tua x'+d.tua;$('tom').textContent=tom;
 for(var i=1;i<=4;i++){var h=d.h[i-1],c=d.c[i-1];veDen('d'+i+'t',d.den[(i-1)*2]);veDen('d'+i+'r',d.den[(i-1)*2+1]);
  $('s'+i).textContent=h.co?h.s:'–';$('l'+i).textContent=h.co?h.l:'–';
  $('g'+i).textContent=!h.co?'chưa có tin':(h.tay?'số gõ tay':(h.song?'':'MẤT TÍN HIỆU'))+(h.nhan?' · '+h.nhan+' gói, mất '+h.mat:'');
  $('ut'+i).innerHTML=(h.ut?'<span class="ut">xe '+UT[h.ut]+' làn '+(h.emg==2?'trong':'ngoài')+'</span> ':'')+(h.gia?'<span class="gia">dòng xe giả</span>':'');
  $('ao'+i).innerHTML=h.ao?'<span class="ao">xe '+UT[h.ao]+' ảo đang chờ</span>':'';
  var e=$('c'+i),a=$('a'+i);
  if(!c.co){e.innerHTML='CAM '+i+': <span class="xau">chưa thấy tin trạng thái</span>';a.style.display='none';ip[i]=''}
  else{var tuoi=c.cach,song=tuoi<8;ip[i]=c.ip&&c.rs?c.ip:'';
   e.innerHTML='CAM '+i+': <span class="'+(song?'tot':'xau')+'">'+(song?'đang chạy':'im '+tuoi+' s')+'</span> · '+CD[c.cd]+(c.cd==2?' '+c.kh.toFixed(1)+' khung/s':'')+
    ' · '+(c.rs?c.ip+' ('+c.rs+' dBm)':'chưa nối WiFi trung tâm')+' · vùng '+(c.vl?'đã lưu':'mặc định')+(c.lat==3?' · lật 180°':c.lat==1?' · lật dọc':c.lat==2?' · lật ngang':'')+
    (c.fw?' · bản '+c.fw:'')+(c.mh?'':' · <span class="xau">chưa có mô hình</span>')+(c.xn?'':' · <span class="xau">bản mới, chưa xác nhận</span>')+(c.rb?' · <span class="xau">đã quay về bản cũ</span>':'');
   if(ip[i]){a.href='/c'+i+'/';a.style.display=''}else a.style.display='none'}}
 $('wn').textContent=d.wn==2?'Hotspot: đã nối, địa chỉ '+d.wip+' (kênh '+d.kenh+')':d.wn==1?'Hotspot: đang tìm...':'WiFi trung tâm kênh '+d.kenh+(d.wcf?' · hotspot: không nối (bấm "Nối lại hotspot")':'');
 $('cd0').className=d.cd?'':'chon';$('cd1').className=d.cd?'chon':'';for(var k=0;k<4;k++)$('k'+k).className=(d.kieu==k)?'chon':'';
 if(document.activeElement!==$('tua'))$('tua').value=d.tua;if(document.activeElement!==$('cs'))$('cs').value=d.cs;
 if(document.activeElement!==$('tutha'))$('tutha').value=d.tutha;$('dong').checked=!!d.dong;if(document.activeElement!==$('md'))$('md').value=d.md;
 $('ao_tk').textContent=d.aoqua?'Đã có '+d.aoqua+' xe ưu tiên ảo qua.':'';
 veTs(d.ts);veNap(d.nap,d.c)}
var MUC=null,TS_DA=null;
function veTs(v){if(!MUC||!v)return;if(!TS_DA){var h='';for(var k=0;k<MUC.length;k++){var m=MUC[k];h+='<tr><td>'+m.ten+'</td><td><input type="number" step="'+(m.ms?'0.5':'1')+'" min="'+(m.ms?m.min/1000:m.min)+'" max="'+(m.ms?m.max/1000:m.max)+'" id="ts_'+m.k+'"></td><td class="mo">'+(m.ms?(m.min/1000)+'–'+(m.max/1000)+' s':m.min+'–'+m.max)+'</td></tr>'}$('ts').innerHTML=h;TS_DA=1}
 for(var k=0;k<MUC.length;k++){var e=$('ts_'+MUC[k].k);if(document.activeElement!==e&&!e.dataset.sua)e.value=MUC[k].ms?v[k]/1000:v[k]}}
function veNap(n,c){if(!n)return;$('nap_tt').innerHTML='bản '+n.fw+' (phân vùng '+n.pv+')'+(n.ota?'':' · <span class="canh">bảng phân vùng không nạp qua WiFi được</span>')+(n.xn?'':' · <span class="canh">bản mới, đang tự kiểm tra (chưa xác nhận)</span>')+(n.loi?' · <span class="canh">có một bản nạp gần đây bị lỗi, đã quay về bản này</span>':'');
 $('nap_cam').innerHTML=n.cam.co?'trung tâm đang giữ tệp bản '+n.cam.pb+', '+Math.round(n.cam.kt/1024)+' KB, chip '+(n.cam.chip==9?'ESP32-S3':'ESP32'):'chưa có tệp nào';
 var h='<tr><th>Camera</th><th>Bản đang chạy</th><th>Nạp</th><th></th></tr>';
 for(var i=1;i<=4;i++){var x=n.c[i-1],q=c[i-1];h+='<tr><td>CAM '+i+'</td><td>'+(q.co?(q.fw||'cũ'):'–')+'</td><td>'+(x.ten?x.ten+(x.tt==3?' '+x.np+' %':'')+(x.ghi?': '+x.ghi:''):'')+'</td><td><button data-nap="'+i+'">Nạp CAM '+i+'</button></td></tr>'}
 $('nc').innerHTML=h}
function hoi(){fetch('/trang_thai.json?t='+Date.now()).then(function(r){return r.json()}).then(capNhat).catch(function(){$('tom').textContent='Mất kết nối với trung tâm...'}).then(function(){setTimeout(hoi,500)})}
function anh(i){var im=$('i'+i);if(!$('xem_anh').checked||!ip[i]){im.style.display='none';setTimeout(function(){anh(i)},1500);return}
 im.style.display='';var xong=function(){im.onload=im.onerror=null;setTimeout(function(){anh(i)},1500)};im.onload=xong;im.onerror=xong;im.src='/c'+i+'/khung.jpg?t='+Date.now()}
document.addEventListener('click',function(e){var b=e.target.closest('button');if(!b)return;
 if(b.dataset.d)lenh(b.dataset.d,'Đã gửi: '+b.textContent);
 else if(b.dataset.c)lenhCam(b.dataset.c,b.dataset.l,b.dataset.x,(b.dataset.c=='0'?'Cả 4 camera: ':'CAM '+b.dataset.c+': ')+b.textContent);
 else if(b.dataset.g){var i=b.dataset.g;lenh('{"id":'+i+',"s":'+(+$('ts'+i).value||0)+',"l":'+(+$('tl'+i).value||0)+',"emg":0}','Trung tâm nhận số gõ tay Hướng '+i)}
 else if(b.dataset.gl){var i=b.dataset.gl;lenhCam(i,'gia','s='+(+$('ts'+i).value||0)+'&l='+(+$('tl'+i).value||0)+'&emg=0&ut=0','CAM '+i+' (giả lập) gửi số mới')}
 else if(b.dataset.xa){var i=b.dataset.xa;lenh('uutien '+i+' '+$('ul'+i).value,'Thả xe ưu tiên ảo vào Hướng '+i)}
 else if(b.dataset.nap){if(confirm('Nạp firmware đang giữ trên trung tâm cho CAM '+b.dataset.nap+'?'))lenh('napcam '+b.dataset.nap,'Đã xếp lịch nạp CAM '+b.dataset.nap)}
 else if(b.dataset.id){var i=b.dataset.id,m=prompt('Số mới cho camera đang là CAM '+i+' (1 đến 4):');if(m&&+m>=1&&+m<=4&&+m!=+i)lenhCam(i,'doiid','moi='+(+m),'CAM '+i+' đổi thành CAM '+m+', đang khởi động lại')}});
$('tha_nn').onclick=function(){lenh('uutien','Đã thả một xe ưu tiên ảo')};
$('tutha').onchange=function(){lenh('tutha '+this.value)};
$('dong').onchange=$('md').onchange=function(){lenh('donggia '+($('dong').checked?1:0)+' '+$('md').value)};
$('ts').addEventListener('input',function(e){if(e.target.id)e.target.dataset.sua=1});
$('ap_ts').onclick=function(){if(!MUC)return;var ds=[];for(var k=0;k<MUC.length;k++){var e=$('ts_'+MUC[k].k);if(e.dataset.sua){ds.push('dat '+MUC[k].k+' '+e.value);delete e.dataset.sua}}
 if(!ds.length){bao('Chưa sửa thông số nào');return}var p=Promise.resolve();ds.forEach(function(d){p=p.then(function(){return lenh(d)})});p.then(function(){bao('Đã gửi '+ds.length+' thông số; xem nhật ký')})};
function taiLen(url,inp,tb,xong){var f=$(inp).files[0];if(!f){bao('Chọn tệp .bin trước');return}var x=new XMLHttpRequest();x.open('POST',url);
 x.upload.onprogress=function(e){if(e.lengthComputable)$(tb).textContent='Đang tải lên '+Math.round(e.loaded*100/e.total)+' %'};
 x.onload=function(){$(tb).textContent=x.responseText;if(x.status==200&&xong)xong()};x.onerror=function(){$(tb).textContent='Mất kết nối khi tải lên'};
 var fd=new FormData();fd.append('f',f,f.name);x.send(fd)}
$('nap_tt_nut').onclick=function(){if(confirm('Nạp firmware mới cho trung tâm? Đèn về đỏ và trung tâm khởi động lại sau khi nạp.'))taiLen('/nap/tt','f_tt','nap_tt_tb')};
$('nap_cam_nut').onclick=function(){taiLen('/nap/cam','f_cam','nap_cam_tb')};
$('nap_ca4').onclick=function(){if(confirm('Nạp lần lượt cho mọi camera đang nối WiFi trung tâm?'))lenh('napcam 0','Đã xếp lịch nạp')};
$('quayve').onclick=function(){if(confirm('Khởi động lại trung tâm bằng bản firmware trước?'))gui('/nap/quayve','')};
var NK=0;function nhatKy(){fetch('/nhat_ky.json?tu='+NK).then(function(r){return r.json()}).then(function(d){var e=$('nk'),cuoi=e.scrollTop+e.clientHeight>=e.scrollHeight-4;
 if(d.so<NK)e.textContent='';d.d.forEach(function(x){e.textContent+=x+'\n'});NK=d.so;if(e.textContent.length>20000)e.textContent=e.textContent.slice(-15000);if(cuoi)e.scrollTop=e.scrollHeight}).catch(function(){}).then(function(){setTimeout(nhatKy,2000)})}
fetch('/muc_ts.json').then(function(r){return r.json()}).then(function(m){MUC=m;if(S)veTs(S.ts)});nhatKy();
$('kd5').onclick=function(){if(confirm('Khởi động lại cả 4 camera và board trung tâm?'))gui('/khoidong5','','Đang khởi động lại cả 5 board...')};
$('tua').onchange=function(){lenh('tua '+this.value)};$('dat_cs').onclick=function(){lenh('catsom '+(+$('cs').value||0))};
veThe();hoi();for(var i=1;i<=4;i++)(function(i){setTimeout(function(){anh(i)},i*350)})(i);
</script></body></html>)HTML";

// Gọi từ loop() để đặt bản chụp trạng thái mới.
inline void webDatTrangThai(const char *json) {
  if (!khoa_trang_thai) return;
  if (xSemaphoreTake(khoa_trang_thai, pdMS_TO_TICKS(5)) == pdTRUE) {
    snprintf(trang_thai_web, sizeof(trang_thai_web), "%s", json);
    xSemaphoreGive(khoa_trang_thai);
  }
}

// Gọi từ loop(): lấy một dòng lệnh trang web gửi lên (nếu có).
inline bool webLayLenh(char *dong, size_t co) {
  LenhWeb l;
  if (!hang_doi_lenh_web || xQueueReceive(hang_doi_lenh_web, &l, 0) != pdTRUE) return false;
  snprintf(dong, co, "%s", l.dong);
  return true;
}

static void webTrangThai() {
  static char b[sizeof(trang_thai_web)];
  if (xSemaphoreTake(khoa_trang_thai, pdMS_TO_TICKS(50)) == pdTRUE) {
    memcpy(b, trang_thai_web, sizeof(b));
    xSemaphoreGive(khoa_trang_thai);
  }
  may_chu_web.sendHeader("Cache-Control", "no-store");
  may_chu_web.send(200, "application/json", b);
}

static void webLenh() {
  LenhWeb l;
  String d = may_chu_web.arg("d");
  if (d.length() == 0 || d.length() >= sizeof(l.dong)) { may_chu_web.send(400, "text/plain; charset=utf-8", "Lệnh trống hoặc quá dài"); return; }
  snprintf(l.dong, sizeof(l.dong), "%s", d.c_str());
  bool duoc = xQueueSend(hang_doi_lenh_web, &l, pdMS_TO_TICKS(100)) == pdTRUE;
  may_chu_web.send(duoc ? 200 : 503, "text/plain; charset=utf-8", duoc ? "Đã nhận" : "Trung tâm đang bận, thử lại");
}

static void webLenhCam() {
  int den = may_chu_web.arg("den").toInt();
  String l = may_chu_web.arg("lenh");
  if (den < 0 || den > 4) { may_chu_web.send(400, "text/plain; charset=utf-8", "den phải từ 0 đến 4"); return; }
  char them[80] = "";
  if (l == "gia") {
    snprintf(them, sizeof(them), "\"s\":%d,\"l\":%d,\"emg\":%d,\"ut\":%d", (int)may_chu_web.arg("s").toInt(),
             (int)may_chu_web.arg("l").toInt(), (int)may_chu_web.arg("emg").toInt(), (int)may_chu_web.arg("ut").toInt());
  } else if (l == "lat") {
    snprintf(them, sizeof(them), "\"doc\":%d,\"ngang\":%d", may_chu_web.arg("doc").toInt() ? 1 : 0, may_chu_web.arg("ngang").toInt() ? 1 : 0);
  } else if (l == "chedo") {
    int cd = may_chu_web.arg("cd").toInt();
    if (cd < 1 || cd > 3) { may_chu_web.send(400, "text/plain; charset=utf-8", "cd phải là 1, 2 hoặc 3"); return; }
    snprintf(them, sizeof(them), "\"cd\":%d", cd);
  } else if (l == "doiid") {
    int moi = may_chu_web.arg("moi").toInt();
    if (den < 1 || moi < 1 || moi > 4) { may_chu_web.send(400, "text/plain; charset=utf-8", "Chọn đúng một camera và số mới từ 1 đến 4"); return; }
    snprintf(them, sizeof(them), "\"moi\":%d", moi);
  } else if (!(l == "xem" || l == "nhay" || l == "khoidong" || l == "vungmacdinh")) {
    may_chu_web.send(400, "text/plain; charset=utf-8", "Lệnh camera không có");
    return;
  }
  bool duoc = guiLenhCam(den, l.c_str(), them);
  may_chu_web.send(duoc ? 200 : 500, "text/plain; charset=utf-8", duoc ? "Đã gửi" : "Không gửi được ESP-NOW");
}

static void webKhoiDong5() {
  guiLenhCam(0, "khoidong", "");
  may_chu_web.send(200, "text/plain; charset=utf-8", "Đang khởi động lại cả 5 board");
  delay(300);
  guiLenhCam(0, "khoidong", "");          // gửi lại một lần cho chắc (gói phát chung không có xác nhận)
  delay(700);
  ESP.restart();
}

static String maHoaUrl(const String &v) {
  static const char HEX_[] = "0123456789ABCDEF";
  String r;
  for (size_t i = 0; i < v.length(); i++) {
    char c = v[i];
    if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~' || c == ',') r += c;
    else { r += '%'; r += HEX_[(uint8_t)c >> 4]; r += HEX_[(uint8_t)c & 15]; }
  }
  return r;
}

// /c<n>/<đường dẫn> → http://192.168.4.(10 + n)/<đường dẫn>
static void webChuyenTiep() {
  String uri = may_chu_web.uri();
  if (uri.length() < 4 || uri[0] != '/' || uri[1] != 'c' || uri[2] < '1' || uri[2] > '4' || uri[3] != '/') {
    may_chu_web.send(404, "text/plain; charset=utf-8", "Không có trang này");
    return;
  }
  int n = uri[2] - '0';
  String q;
  for (int i = 0; i < may_chu_web.args(); i++) {
    if (may_chu_web.argName(i) == "plain") continue;
    if (q.length()) q += '&';
    q += maHoaUrl(may_chu_web.argName(i)) + "=" + maHoaUrl(may_chu_web.arg(i));
  }
  bool post = may_chu_web.method() == HTTP_POST;
  String url = "http://192.168.4." + String(CAM_IP_BYTE_DAU + n) + uri.substring(3);
  if (!post && q.length()) url += "?" + q;
  WiFiClient kn;
  HTTPClient h;
  h.setConnectTimeout(1500);
  h.setTimeout(4000);
  const char *giu[] = {"Content-Type"};
  h.collectHeaders(giu, 1);
  if (!h.begin(kn, url)) { may_chu_web.send(502, "text/plain; charset=utf-8", "Địa chỉ camera sai"); return; }
  int ma;
  if (post) { h.addHeader("Content-Type", "application/x-www-form-urlencoded"); ma = h.POST(q); }
  else ma = h.GET();
  if (ma <= 0) {
    h.end();
    may_chu_web.send(504, "text/plain; charset=utf-8", "CAM " + String(n) + " không trả lời (chưa nối WiFi trung tâm?)");
    return;
  }
  int dai = h.getSize();
  if (dai < 0 || dai > 400000) {     // camera luôn báo độ dài; phòng khi không báo thì đọc cả chuỗi
    String t = h.getString();
    may_chu_web.send(ma, h.header("Content-Type"), t);
    h.end();
    return;
  }
  uint8_t *b = (uint8_t *)(psramFound() ? ps_malloc(dai + 1) : malloc(dai + 1));
  if (!b) { h.end(); may_chu_web.send(500, "text/plain; charset=utf-8", "Thiếu bộ nhớ"); return; }
  WiFiClient *dong = h.getStreamPtr();
  int da = 0;
  uint32_t t0 = millis();
  while (da < dai && millis() - t0 < 4000) {
    int co = dong->available();
    if (co > 0) da += dong->read(b + da, (size_t)min(co, dai - da));
    else if (!dong->connected()) break;
    else delay(1);
  }
  String loai = h.header("Content-Type");
  h.end();
  may_chu_web.sendHeader("Cache-Control", "no-store");
  may_chu_web.send_P(ma, loai.length() ? loai.c_str() : "application/octet-stream", (const char *)b, (size_t)da);
  free(b);
}

static void webNapTrungTamXong() {
  if (nap_loi.length()) { may_chu_web.send(400, "text/plain; charset=utf-8", nap_loi); return; }
  String t = String("Đã nạp bản ") + nap_kiem.phienBan() + ". Trung tâm khởi động lại; bản mới phải tự xác nhận chạy ổn trong 2 phút, không thì tự quay về bản cũ.";
  may_chu_web.send(200, "text/plain; charset=utf-8", t);
  delay(800);
  ESP.restart();
}

static void webNapCamXong() {
  if (nap_loi.length()) { may_chu_web.send(400, "text/plain; charset=utf-8", nap_loi); return; }
  String t = String("Trung tâm đã giữ firmware camera bản ") + anh_cam.pb + " (" + String(anh_cam.kt / 1024) + " KB). Bấm Nạp ở camera cần nạp.";
  may_chu_web.send(200, "text/plain; charset=utf-8", t);
}

static void webQuayVe() {
  const char *ly_do = napQuayVe();
  if (ly_do) { may_chu_web.send(400, "text/plain; charset=utf-8", ly_do); return; }
  may_chu_web.send(200, "text/plain; charset=utf-8", "Trung tâm khởi động lại bằng bản trước.");
  delay(800);
  ESP.restart();
}

static void webNhatKy() {
  static char b[NK_SO_DONG * NK_DAI_DONG + 64];
  nk.json(b, sizeof(b), (uint32_t)may_chu_web.arg("tu").toInt());
  may_chu_web.sendHeader("Cache-Control", "no-store");
  may_chu_web.send(200, "application/json", b);
}

static void webMucThamSo() {
  String t = "[";
  for (int i = 0; i < SO_MUC_THAM_SO; i++) {
    const MucThamSo &m = MUC_THAM_SO[i];
    t += String(i ? "," : "") + "{\"k\":\"" + m.khoa + "\",\"ten\":\"" + m.ten + "\",\"min\":" + m.nho_nhat + ",\"max\":" + m.lon_nhat +
         ",\"ms\":" + (m.la_ms ? 1 : 0) + "}";
  }
  t += "]";
  may_chu_web.send(200, "application/json", t);
}

static void tacVuWeb(void *) {
  napFat();          // mở FFat, đọc thông tin firmware camera đang giữ; lần đầu định dạng FFat (ở đây để không làm chậm đèn)
  for (;;) {
    may_chu_web.handleClient();
    vTaskDelay(pdMS_TO_TICKS(2));
  }
}

// Gọi trong setup(), sau nhanTinKhoiTao() (hàm đó đã bật WiFi).
inline void webKhoiTao() {
  hang_doi_lenh_web = xQueueCreate(8, sizeof(LenhWeb));
  khoa_trang_thai = xSemaphoreCreateMutex();
  may_chu_web.on("/", []() { may_chu_web.send_P(200, "text/html; charset=utf-8", TRANG_TRUNG_TAM); });
  may_chu_web.on("/trang_thai.json", webTrangThai);
  may_chu_web.on("/lenh", HTTP_POST, webLenh);
  may_chu_web.on("/cam", HTTP_POST, webLenhCam);
  may_chu_web.on("/khoidong5", HTTP_POST, webKhoiDong5);
  may_chu_web.on("/nap/tt", HTTP_POST, webNapTrungTamXong, []() { napNhanTrungTam(may_chu_web); });
  may_chu_web.on("/nap/cam", HTTP_POST, webNapCamXong, []() { napNhanAnhCam(may_chu_web); });
  may_chu_web.on("/fw/cam.bin", HTTP_GET, []() { napGuiAnhCam(may_chu_web); });
  may_chu_web.on("/nap/quayve", HTTP_POST, webQuayVe);
  may_chu_web.on("/nhat_ky.json", webNhatKy);
  may_chu_web.on("/muc_ts.json", webMucThamSo);
  may_chu_web.onNotFound(webChuyenTiep);
  may_chu_web.begin();
  if (MDNS.begin(TEN_MDNS)) MDNS.addService("http", "tcp", 80);
  if (TEN_WIFI_NGOAI[0]) wifiNgoaiThu();
  xTaskCreatePinnedToCore(tacVuWeb, "web", 12288, NULL, 1, NULL, 0);
}
