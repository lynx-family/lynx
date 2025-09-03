// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.image.model;

import androidx.annotation.Nullable;
import com.lynx.tasm.image.ImageContent;
import org.json.JSONObject;

public interface ImageLoadListener {
  void onRequestSubmit(ImageRequestInfo imageRequestInfo);

  void onSuccess(
      @Nullable ImageContent imageContent, ImageRequestInfo requestInfo, ImageInfo imageInfo);

  void onFailure(int errorCode, Throwable throwable);

  void onImageMonitorInfo(JSONObject monitorInfo);
}
