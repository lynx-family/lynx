// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { createSlice } from '@reduxjs/toolkit';

export interface IViewsInfoState {
  currentView: number;
  viewsCount: number;
  type: string;
  templateUrl: string | undefined;
  showURL: boolean;
}

export const viewsSlice = createSlice({
  name: 'viewsInfo',
  initialState: {
    currentView: 0,
    viewsCount: 0,
    templateUrl: null,
    type: 'redbox',
    showURL: false,
  } as IViewsInfoState,

  reducers: {
    updateViewsInfo: (state, action) => {
      state.currentView = action.payload.currentView;
      state.viewsCount = action.payload.viewsCount;
      state.templateUrl = action.payload.templateUrl;
      state.type = action.payload.type;
    },
    toggleURLDisplay: (state) => {
      state.showURL = !state?.showURL;
    },
  },
});

export const { updateViewsInfo, toggleURLDisplay } = viewsSlice.actions;
export default viewsSlice.reducer;
