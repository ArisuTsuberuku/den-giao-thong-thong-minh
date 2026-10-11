// dong_goi.js — Gộp web/index.html và các tệp .js thành MỘT tệp HTML duy nhất,
// tiện gửi qua Zalo, email hoặc chép sang máy khác (mở bằng trình duyệt, không cần mạng).
// Chạy: node cong_cu/dong_goi.js            → tạo Mo_phong_sa_ban.html ở thư mục gốc
//       node cong_cu/dong_goi.js tep.html --manh   → bản không có khung <html>, để đăng lên claude.ai
'use strict';
const fs = require('fs');
const path = require('path');
const web = path.join(__dirname, '..', 'web');
const dich = process.argv[2] || path.join(__dirname, '..', 'Mo_phong_sa_ban.html');
const manh = process.argv.includes('--manh');

let html = fs.readFileSync(path.join(web, 'index.html'), 'utf8');
html = html.replace(/<script src="([^"]+)"><\/script>/g, (_, ten) =>
  '<script>\n' + fs.readFileSync(path.join(web, ten), 'utf8').replace(/<\/script/g, '<\\/script') + '\n</script>');
if (manh) {
  const dau = html.match(/<head>([\s\S]*?)<\/head>/)[1].replace(/<meta[^>]*>\s*/g, '');
  const than = html.match(/<body>([\s\S]*?)<\/body>/)[1];
  html = dau.trim() + '\n' + than.trim() + '\n';
}
fs.writeFileSync(dich, html);
console.log('Đã ghi ' + dich + ' (' + Math.round(html.length / 1024) + ' KB)');
