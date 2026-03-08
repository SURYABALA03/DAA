const http = require('http');
const fs = require('fs');
const path = require('path');
const { execFile } = require('child_process');

const PORT = 3000;
const ALGO_PATH = path.join(__dirname, 'fleet_algo');

function sendJSON(res, data, status = 200) {
  res.writeHead(status, { 'Content-Type': 'application/json', 'Access-Control-Allow-Origin': '*' });
  res.end(JSON.stringify(data));
}

function sendFile(res, filePath, contentType) {
  fs.readFile(filePath, (err, data) => {
    if (err) { res.writeHead(404); res.end('Not found'); return; }
    res.writeHead(200, { 'Content-Type': contentType });
    res.end(data);
  });
}

const server = http.createServer((req, res) => {
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

  if (req.method === 'OPTIONS') { res.writeHead(204); res.end(); return; }

  const url = req.url.split('?')[0];

  // Serve frontend
  if (url === '/' || url === '/index.html') {
    return sendFile(res, path.join(__dirname, 'index.html'), 'text/html');
  }

  // API: Run fleet optimization
  if (url === '/api/optimize' && req.method === 'POST') {
    let body = '';
    req.on('data', chunk => body += chunk);
    req.on('end', () => {
      try {
        const { employees, num_cabs, seats_per_cab } = JSON.parse(body);

        if (!employees || !num_cabs || !seats_per_cab) {
          return sendJSON(res, { error: 'Missing parameters' }, 400);
        }

        const empJson = JSON.stringify(employees);
        execFile(ALGO_PATH, [empJson, String(num_cabs), String(seats_per_cab)],
          { timeout: 5000 },
          (err, stdout, stderr) => {
            if (err) return sendJSON(res, { error: 'Algorithm failed: ' + err.message }, 500);
            try {
              const result = JSON.parse(stdout.trim());
              sendJSON(res, result);
            } catch (e) {
              sendJSON(res, { error: 'Parse error: ' + stdout }, 500);
            }
          }
        );
      } catch (e) {
        sendJSON(res, { error: 'Invalid JSON: ' + e.message }, 400);
      }
    });
    return;
  }

  res.writeHead(404); res.end('Not found');
});

server.listen(PORT, () => {
  console.log(`\n🚗 Smart Fleet Management System`);
  console.log(`✅ Server running at http://localhost:${PORT}`);
  console.log(`📊 C Algorithm Engine: ${ALGO_PATH}\n`);
});
