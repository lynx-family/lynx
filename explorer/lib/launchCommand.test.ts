// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { describe, expect, it } from 'vitest';

import {
  parseLaunchCommand,
  retargetLaunchCommand,
  setCommandBoolean,
  setCommandTheme,
} from './launchCommand';

describe('launch command', () => {
  it('detects Sparkling canonical schemes and their presentation aliases', () => {
    const command = parseLaunchCommand(
      'hybrid://lynxview_page?url=https%3A%2F%2Fexample.com%2Fmain.lynx.bundle&hide_nav_bar=1&hide_status_bar=1&trans_status_bar=true&force_theme_style=dark',
      'lynx'
    );

    expect(command).toMatchObject({
      runtime: 'sparkling',
      runtimeLocked: true,
      fullscreen: true,
      hiddenNav: true,
      theme: 'dark',
    });
  });

  it('writes the Sparkling fullscreen dialect bidirectionally', () => {
    const command = parseLaunchCommand(
      'https://example.com/main.bundle',
      'sparkling'
    );
    const enabled = setCommandBoolean(command, 'fullscreen', true);

    expect(enabled).toContain('hide_nav_bar=true');
    expect(enabled).toContain('hide_status_bar=true');
    expect(enabled).toContain('trans_status_bar=true');
    expect(enabled).not.toContain('fullscreen=');

    const disabled = setCommandBoolean(
      parseLaunchCommand(enabled, 'sparkling'),
      'fullscreen',
      false
    );
    expect(disabled).toBe('https://example.com/main.bundle');
  });

  it('does not collapse Sparkling edge-to-edge into the fullscreen preset', () => {
    const command = parseLaunchCommand(
      'hybrid://lynxview_page?url=https%3A%2F%2Fexample.com%2Fmain.lynx.bundle&hide_nav_bar=1&trans_status_bar=true',
      'lynx'
    );

    expect(command.fullscreen).toBe(false);
    expect(command.hiddenNav).toBe(true);
  });

  it('reflects typed query changes back into smart controls', () => {
    const command = parseLaunchCommand(
      'https://example.com/main.bundle?hidden_nav=yes&back_button_style=light',
      'lynx'
    );

    expect(command.hiddenNav).toBe(true);
    expect(command.theme).toBe('light');
    expect(setCommandTheme(command, null)).toBe(
      'https://example.com/main.bundle?hidden_nav=yes'
    );
  });

  it('translates presentation options when the selected runtime changes', () => {
    const lynx = parseLaunchCommand(
      'https://example.com/main.bundle?fullscreen=true&back_button_style=dark&bar_color=181D25&title_color=FFFFFF',
      'lynx'
    );
    const sparkling = retargetLaunchCommand(lynx, 'sparkling');

    expect(sparkling).toContain('hide_nav_bar=true');
    expect(sparkling).toContain('hide_status_bar=true');
    expect(sparkling).toContain('trans_status_bar=true');
    expect(sparkling).toContain('force_theme_style=dark');
    expect(sparkling).toContain('nav_bar_color=181D25');
    expect(sparkling).not.toContain('fullscreen=');
    expect(sparkling).not.toContain('back_button_style=');
  });
});
