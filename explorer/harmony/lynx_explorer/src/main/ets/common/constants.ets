// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export default class Constants {
  /**
   * The index page message.
   */
  static readonly SCAN_URL: string = 'pages/Scan';
  static readonly LYNX_URL: string = 'pages/Lynx';
  static readonly LYNX_RECORDER_URL: string = 'pages/LynxRecorder';
  /**
   * Full the width.
   */
  static readonly FULL_WIDTH: string = '100%';
  /**
   * Full the height.
   */
  static readonly FULL_HEIGHT: string = '100%';
  static readonly SSL_LOCAL: string = 'sslocal://lynxview?';
  static readonly SSL_LOCAL_UITEST: string = 'sslocal://lynxtest?';
  static readonly LOCAL: string = 'local://';
  static readonly FILE_LOCAL: string = 'file://lynx?local://';
  static readonly DEVTOOL: string = 'lynx://remote_debug_lynx/enable';
  static readonly TEST_BENCH: string = 'sslocal://arkview?';
}
