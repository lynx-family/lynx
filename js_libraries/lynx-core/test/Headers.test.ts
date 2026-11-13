// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { describe, expect, test } from 'vitest';

import { Response } from '../src/modules/fetch/Response';

function createHeadersFromNativeResponse(headers: [string, string][]) {
  return new Response(null, { headers }).headers;
}

describe('Headers', () => {
  test('looks up mixed-case native response headers case-insensitively', () => {
    const headers = createHeadersFromNativeResponse([
      ['Link', '<https://example.com/next>; rel="next"'],
      ['Content-Type', 'application/json'],
    ]);

    expect(headers.get('link')).toBe(headers.get('Link'));
    expect(headers.get('LINK')).toBe(headers.get('Link'));
    expect(headers.get('content-type')).toBe('application/json');
    expect(headers.has('CONTENT-TYPE')).toBe(true);
  });

  test('normalizes names consistently for mutations', () => {
    const headers = createHeadersFromNativeResponse([['X-Page', 'first']]);

    headers.append('x-PAGE', 'second');
    expect(headers.get('X-page')).toBe('first, second');
    expect([...headers.entries()]).toEqual([['x-page', 'first, second']]);

    headers.set('X-PAGE', 'replacement');
    expect(headers.get('x-page')).toBe('replacement');
    expect(headers.has('x-PaGe')).toBe(true);

    headers.delete('X-page');
    expect(headers.has('x-page')).toBe(false);
    expect(headers.get('X-PAGE')).toBeNull();
  });

  test('combines duplicate names regardless of case', () => {
    const headers = createHeadersFromNativeResponse([
      ['Warning', '199 first'],
      ['warning', '299 second'],
    ]);

    expect(headers.get('WARNING')).toBe('199 first, 299 second');
    expect([...headers.keys()]).toEqual(['warning']);
  });
});
