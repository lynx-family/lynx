// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { getAppStore } from '@/models/store';
import { addError } from './errorReducer';
import { parseErrorWrapper } from '@/parser';

export async function add(error: string) {
  try {
    const errorRecord = await parseErrorWrapper(error);
    getAppStore().dispatch(addError({ errorRecord }));
  } catch (e) {
    console.warn('Error occurred when add error data:', e);
  }
}
