// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.service;

/** Supplies the native transport of an installed optional Host Script runtime. */
public interface ILynxHostScriptTransportService extends IServiceProvider {
  /** Returns the process-lifetime native transport, or zero when unavailable. */
  long getHostScriptTransport();
}
