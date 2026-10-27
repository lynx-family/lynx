// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

const assert = require('assert');
const http = require('http');
const {
  assertPublicOpenCardUrl,
  fetchPublicOpenCardSource,
  isPublicAddress,
} = require('../dist/open-card-source.js');

function testUrlPolicy() {
  for (const url of [
    'file:///etc/passwd',
    '/etc/passwd',
    'relative/template.lynx.bundle',
    'ftp://example.com/template.lynx.bundle',
    'http://127.0.0.1:18080/',
    'http://2130706433/',
    'http://0x7f.1/',
    'http://0.0.0.0/',
    'http://10.0.0.1/',
    'http://172.16.0.1/',
    'http://192.168.1.1/',
    'http://169.254.169.254/latest/meta-data/',
    'http://[::1]/',
    'http://[::ffff:127.0.0.1]/',
    'http://[fe80::1]/',
    'http://[fd00::1]/',
  ]) {
    assert.throws(() => assertPublicOpenCardUrl(url), Error, url);
  }
  for (const url of [
    'https://lynxjs.org/template.lynx.bundle',
    'http://93.184.215.14/template.lynx.bundle',
    'http://[2606:4700::1]/template.lynx.bundle',
  ]) {
    assert.strictEqual(assertPublicOpenCardUrl(url).toString(), url);
  }
  assert.strictEqual(isPublicAddress('8.8.8.8'), true);
  assert.strictEqual(isPublicAddress('100.64.0.1'), false);
  assert.strictEqual(isPublicAddress('not-an-ip'), false);
}

async function testResolvedHostnameIsChecked() {
  const server = http.createServer((_request, response) => {
    response.end('loopback content');
  });
  await new Promise((resolve) => server.listen(0, '127.0.0.1', resolve));
  try {
    const { port } = server.address();
    await assert.rejects(
      () => fetchPublicOpenCardSource(`http://localhost:${port}/`, 5000),
      /resolves to non-public address/
    );
  } finally {
    await new Promise((resolve) => server.close(resolve));
  }
}

// Replaces http.get with a fake that always redirects to `location`, so
// redirect hops can be checked without a public redirector.
async function withRedirectingHttpGet(location, run) {
  const originalGet = http.get;
  let calls = 0;
  http.get = (_url, _options, onResponse) => {
    calls += 1;
    const request = { on() {}, destroy() {} };
    setImmediate(() =>
      onResponse({
        statusCode: 302,
        headers: { location },
        resume() {},
        on() {},
      })
    );
    return request;
  };
  try {
    await run();
  } finally {
    http.get = originalGet;
  }
  return calls;
}

async function testRedirectsAreRechecked() {
  const publicUrl = 'http://93.184.215.14/template.lynx.bundle';
  const loopbackCalls = await withRedirectingHttpGet(
    'http://127.0.0.1:18080/',
    () =>
      assert.rejects(
        () => fetchPublicOpenCardSource(publicUrl, 5000),
        /targets a non-public address/
      )
  );
  assert.strictEqual(loopbackCalls, 1);

  const fileCalls = await withRedirectingHttpGet('file:///etc/passwd', () =>
    assert.rejects(
      () => fetchPublicOpenCardSource(publicUrl, 5000),
      /only accepts http\(s\) URLs/
    )
  );
  assert.strictEqual(fileCalls, 1);

  const loopCalls = await withRedirectingHttpGet(publicUrl, () =>
    assert.rejects(
      () => fetchPublicOpenCardSource(publicUrl, 5000),
      /too many redirects/
    )
  );
  assert.strictEqual(loopCalls, 6);
}

async function testPublicSourceLoads() {
  const template = await fetchPublicOpenCardSource(
    'https://lynxjs.org/lynx-examples/gallery/dist/GalleryComplete.lynx.bundle',
    30000
  );
  assert.ok(template.length > 0);
}

async function main() {
  testUrlPolicy();
  await testResolvedHostnameIsChecked();
  await testRedirectsAreRechecked();
  await testPublicSourceLoads();
}

main()
  .then(() => {
    console.log('open card source policy tests finished successfully.');
  })
  .catch((error) => {
    console.error('open card source policy tests failed:', error);
    process.exit(1);
  });
