const http = require('http');
const fs = require('fs');
const path = require('path');

const root = __dirname;
const cliPort = process.argv
  .slice(2)
  .map((arg) => arg.match(/^--port=(\d+)$/)?.[1])
  .find(Boolean);
const port = Number(cliPort || process.env.PORT || 8766);

const types = {
  '.html': 'text/html; charset=utf-8',
  '.js': 'text/javascript; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.json': 'application/json; charset=utf-8',
  '.svg': 'image/svg+xml',
  '.ico': 'image/x-icon'
};

function send(res, status, body, type = 'text/plain; charset=utf-8') {
  res.writeHead(status, {
    'Content-Type': type,
    'Cache-Control': 'no-store'
  });
  res.end(body);
}

function resolvePath(url) {
  const parsed = new URL(url, `http://127.0.0.1:${port}`);
  const requested = parsed.pathname === '/' ? '/index.html' : parsed.pathname;
  const decoded = decodeURIComponent(requested);
  const full = path.normalize(path.join(root, decoded));
  const relative = path.relative(root, full);

  if (relative.startsWith('..') || path.isAbsolute(relative)) {
    return null;
  }

  return full;
}

const server = http.createServer((req, res) => {
  const full = resolvePath(req.url);

  if (!full) {
    send(res, 403, 'Forbidden');
    return;
  }

  fs.readFile(full, (error, data) => {
    if (error) {
      send(res, error.code === 'ENOENT' ? 404 : 500,
           error.code === 'ENOENT' ? 'Not Found' : 'Server Error');
      return;
    }

    const type = types[path.extname(full).toLowerCase()] || 'application/octet-stream';
    send(res, 200, data, type);
  });
});

server.listen(port, '127.0.0.1', () => {
  if (process.stdout.writable) {
    process.stdout.write(`OpenEarable BLE Console: http://127.0.0.1:${port}/\n`);
  }
});
