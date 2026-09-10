// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.recorder;

import org.json.JSONArray;
import org.json.JSONObject;

public interface LynxRecorderReplayDataProvider {
  JSONArray getFunctionCall();
  JSONObject getCallbackData();
  JSONArray getActionList();
  JSONArray getJsbIgnoredInfo();
  JSONObject getJsbSettings();
  JSONObject getSharedData();
}
