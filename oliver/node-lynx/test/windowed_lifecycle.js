// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

const path = require('path');
const { getHelloWorldTemplateUrl } = require('./helpers/remote-templates.js');
const { runWindowedLifecycleSmoke } = require('./helpers/windowed-lifecycle.js');

runWindowedLifecycleSmoke({
  packageRoot: path.resolve(__dirname, '..'),
  templateUrl: getHelloWorldTemplateUrl(),
}).catch((error) => { console.error(error); process.exitCode = 1; });
