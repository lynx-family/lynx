// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { createSlice } from '@reduxjs/toolkit';
import { IErrorRecord } from '@/common/interface';

export interface IErrorState {
  errorsOnDisplay: IErrorRecord[];
  cachedErrors?: string[];
}

export const errorSlice = createSlice({
  name: 'error',

  initialState: {
    errorsOnDisplay: [],
    cachedErrors: [],
  } as IErrorState,

  reducers: {
    clearErrors: (state) => {
      state.errorsOnDisplay = [];
    },

    addError: (state, { payload }) => {
      state.errorsOnDisplay = state?.errorsOnDisplay ? [...state.errorsOnDisplay, payload.errorRecord] : [payload.errorRecord];
    },

    pushErrorCache: (state, { payload }) => {
      state.cachedErrors = state?.cachedErrors ? [...state.cachedErrors, payload] : [payload];
    },

    popErrorCache: (state) => {
      state.cachedErrors = state.cachedErrors.slice(1);
    },

    clearErrorCache: (state) => {
      state.cachedErrors = [];
    },
  },
});

export const { clearErrors, addError, pushErrorCache, popErrorCache, clearErrorCache } = errorSlice.actions;
export default errorSlice.reducer;
