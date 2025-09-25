// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { UIMethods } from '../../types';

// UIMethods types check
export function invoke<T extends keyof UIMethods>(_param: UIMethods[T]) {}
