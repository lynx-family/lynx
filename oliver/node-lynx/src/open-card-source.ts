// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import * as dns from 'dns';
import * as http from 'http';
import * as https from 'https';
import * as net from 'net';

const DEFAULT_TIMEOUT_MS = 10000;
const MAX_REDIRECTS = 5;
const ALLOW_ANY_SOURCE_HINT =
  'enable allowAnySource (CLI: --allow-any-open-card-source) to allow it';

// Hosts a DebugRouter peer must not reach through node-lynx by default:
// loopback, private, link-local (including cloud metadata), shared, reserved,
// multicast, and IPv6 ranges that embed IPv4 addresses. IPv4 and IPv6 use
// separate lists because BlockList also matches IPv4 against IPv4-mapped IPv6
// subnets.
const nonPublicIPv4 = new net.BlockList();
const nonPublicIPv6 = new net.BlockList();
for (const [prefix, bits] of [
  ['0.0.0.0', 8],
  ['10.0.0.0', 8],
  ['100.64.0.0', 10],
  ['127.0.0.0', 8],
  ['169.254.0.0', 16],
  ['172.16.0.0', 12],
  ['192.0.0.0', 24],
  ['192.168.0.0', 16],
  ['198.18.0.0', 15],
  ['224.0.0.0', 4],
  ['240.0.0.0', 4],
] as const) {
  nonPublicIPv4.addSubnet(prefix, bits, 'ipv4');
}
for (const [prefix, bits] of [
  ['::', 96],
  ['::ffff:0:0', 96],
  ['64:ff9b::', 96],
  ['fc00::', 7],
  ['fe80::', 10],
  ['ff00::', 8],
] as const) {
  nonPublicIPv6.addSubnet(prefix, bits, 'ipv6');
}

export function isPublicAddress(address: string): boolean {
  if (net.isIPv4(address)) {
    return !nonPublicIPv4.check(address, 'ipv4');
  }
  return net.isIPv6(address) && !nonPublicIPv6.check(address, 'ipv6');
}

/**
 * Validates a URL received from DebugRouter OpenCard. Only http(s) URLs whose
 * literal IP host is public pass; hostnames are checked again after DNS
 * resolution by `fetchPublicOpenCardSource`.
 */
export function assertPublicOpenCardUrl(url: string): URL {
  let parsed: URL | undefined;
  try {
    parsed = new URL(url);
  } catch {
    // Local paths are not URLs; reject them with the same hint below.
  }
  if (
    !parsed ||
    (parsed.protocol !== 'http:' && parsed.protocol !== 'https:')
  ) {
    throw new Error(
      `OpenCard only accepts http(s) URLs by default; ${ALLOW_ANY_SOURCE_HINT}: ${url}`
    );
  }
  const host = parsed.hostname.replace(/^\[(.*)\]$/, '$1');
  if (net.isIP(host) && !isPublicAddress(host)) {
    throw new Error(
      `OpenCard URL targets a non-public address; ${ALLOW_ANY_SOURCE_HINT}: ${url}`
    );
  }
  return parsed;
}

const publicOnlyLookup: net.LookupFunction = (hostname, options, callback) => {
  dns.lookup(hostname, options, (error, address, family) => {
    if (error) {
      callback(error, address, family);
      return;
    }
    const addresses = Array.isArray(address)
      ? address.map((entry) => entry.address)
      : [address];
    const blocked = addresses.find((entry) => !isPublicAddress(entry));
    if (blocked !== undefined) {
      callback(
        new Error(
          `OpenCard host ${hostname} resolves to non-public address ${blocked}; ${ALLOW_ANY_SOURCE_HINT}`
        ),
        address,
        family
      );
      return;
    }
    callback(null, address, family);
  });
};

type HttpResult = { location: string } | { body: Buffer };

function requestOnce(
  target: URL,
  onRequest: (request: http.ClientRequest) => void
): Promise<HttpResult> {
  return new Promise((resolve, reject) => {
    const client = target.protocol === 'https:' ? https : http;
    const request = client.get(
      target,
      { lookup: publicOnlyLookup },
      (response) => {
        response.on('error', reject);
        const statusCode = response.statusCode ?? 0;
        const location = response.headers.location;
        if (statusCode >= 300 && statusCode < 400 && location) {
          response.resume();
          resolve({ location });
          return;
        }
        if (statusCode < 200 || statusCode >= 300) {
          response.resume();
          reject(
            new Error(`request failed with status ${statusCode}: ${target}`)
          );
          return;
        }
        const chunks: Buffer[] = [];
        response.on('data', (chunk) =>
          chunks.push(Buffer.isBuffer(chunk) ? chunk : Buffer.from(chunk))
        );
        response.on('end', () => resolve({ body: Buffer.concat(chunks) }));
      }
    );
    request.on('error', reject);
    onRequest(request);
  });
}

/**
 * Downloads an OpenCard template from a public http(s) URL. Every redirect hop
 * and every resolved address is re-checked, so a public URL cannot bounce the
 * request to loopback, private, or metadata hosts.
 */
export async function fetchPublicOpenCardSource(
  url: string,
  timeoutMs = DEFAULT_TIMEOUT_MS
): Promise<Buffer> {
  let activeRequest: http.ClientRequest | undefined;
  const timer = setTimeout(
    () =>
      activeRequest?.destroy(
        new Error(`download ${url} timed out after ${timeoutMs}ms`)
      ),
    timeoutMs
  );
  try {
    let current = url;
    for (let redirects = 0; ; redirects++) {
      const result = await requestOnce(
        assertPublicOpenCardUrl(current),
        (request) => {
          activeRequest = request;
        }
      );
      if ('body' in result) {
        return result.body;
      }
      if (redirects >= MAX_REDIRECTS) {
        throw new Error(`too many redirects: ${url}`);
      }
      current = new URL(result.location, current).toString();
    }
  } finally {
    clearTimeout(timer);
  }
}
