// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { describe, expect, it, vi } from 'vitest';

import {
  SPARKLING_GO_EXTENSION_ID,
  SPARKLING_GO_ROOT_SCHEME,
  getAvailableExtensions,
  launchExtension,
} from './extensions';

describe('Explorer extensions', () => {
  it('omits Sparkling Go when the host cannot create Sparkling containers', () => {
    expect(
      getAvailableExtensions({ supportsSparklingContainer: false })
    ).toEqual([]);
  });

  it('exposes a stable Sparkling Go descriptor when its capability is present', () => {
    expect(
      getAvailableExtensions({ supportsSparklingContainer: true })
    ).toEqual([
      {
        id: SPARKLING_GO_EXTENSION_ID,
        title: 'Sparkling Go',
        description: 'Container playground and examples',
        accentColor: '#e10543',
        rootScheme: SPARKLING_GO_ROOT_SCHEME,
      },
    ]);
  });

  it('reports the Sparkling version advertised by the native host', () => {
    vi.stubGlobal('lynx', {
      __globalProps: { explorerSparklingVersion: '2.1.0-rc.12' },
    });

    expect(
      getAvailableExtensions({ supportsSparklingContainer: true })[0]?.version
    ).toBe('2.1.0-rc.12');
    // cspell:ignore unstub
    vi.unstubAllGlobals();
  });

  it('launches Sparkling Go through explicit Sparkling route ownership', async () => {
    const open = vi.fn().mockResolvedValue(undefined);

    await launchExtension(
      SPARKLING_GO_EXTENSION_ID,
      { supportsSparklingContainer: true },
      open
    );

    expect(open).toHaveBeenCalledOnce();
    expect(open).toHaveBeenCalledWith(
      SPARKLING_GO_ROOT_SCHEME,
      'sparkling',
      'showcase'
    );
    expect(SPARKLING_GO_ROOT_SCHEME).toBe(
      'hybrid://lynxview_page?bundle=.%2FResource%2Fextensions%2Fsparkling-go%2Fmain.lynx.bundle&hide_status_bar=1&hide_nav_bar=1'
    );
  });

  it('rejects unavailable and unknown extensions without opening a route', async () => {
    const open = vi.fn().mockResolvedValue(undefined);

    await expect(
      launchExtension(
        SPARKLING_GO_EXTENSION_ID,
        { supportsSparklingContainer: false },
        open
      )
    ).rejects.toThrow('is unavailable');
    await expect(
      launchExtension(
        'missing-extension',
        { supportsSparklingContainer: true },
        open
      )
    ).rejects.toThrow('is unavailable');
    expect(open).not.toHaveBeenCalled();
  });
});
