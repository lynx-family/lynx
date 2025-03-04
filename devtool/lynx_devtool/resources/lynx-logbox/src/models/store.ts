// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { configureStore } from '@reduxjs/toolkit';
import errorReducer from './errorReducer';
import viewsInfoReducer from './viewsInfo';

const store = configureStore({
  reducer: {
    error: errorReducer,
    viewsInfo: viewsInfoReducer,
  },
});

export function getAppStore() {
  return store;
}
