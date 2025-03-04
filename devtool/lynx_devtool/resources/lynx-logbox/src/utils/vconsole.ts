// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import VConsole from 'vconsole';
const vConsole = new VConsole({
  onReady() {
    this.hideSwitch();
  },
});

export default vConsole;
