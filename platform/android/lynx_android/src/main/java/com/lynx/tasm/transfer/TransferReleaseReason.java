// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.transfer;

/** The reason a managed transfer's SDK handle was invalidated. */
public enum TransferReleaseReason { EXPLICIT_RELEASE, OWNER_DESTROY, LOAD_FAILED }
