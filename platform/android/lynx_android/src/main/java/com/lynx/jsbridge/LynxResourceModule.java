// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.jsbridge;

import android.os.Build;
import android.util.Pair;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.ReadableType;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.LynxSubErrorCode;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.service.ILynxImageService;
import com.lynx.tasm.service.ILynxResourceService;
import com.lynx.tasm.service.LynxServiceCenter;

public class LynxResourceModule extends LynxContextModule {
  public static final String NAME = "LynxResourceModule";
  public static final String DATA_KEY = "data";
  public static final String URI_KEY = "uri";
  public static final String TYPE_KEY = "type";
  public static final String PARAMS_KEY = "params";
  public static final String CODE_KEY = "code";
  public static final String MSG_KEY = "msg";
  public static final String DETAIL_KEY = "details";

  public static final String IMAGE_TYPE = "image";
  public static final String VIDEO_TYPE = "video";
  public static final String AUDIO_TYPE = "audio";

  public static final long DEFAULT_MEDIA_SIZE = 500 * 1024;

  private ILynxImageService mImagePrefetchHelper;

  @RequiresApi(api = Build.VERSION_CODES.KITKAT)
  public LynxResourceModule(LynxContext context) {
    super(context);
    mImagePrefetchHelper = LynxServiceCenter.inst().getService(ILynxImageService.class);
    if (mImagePrefetchHelper == null) {
      LynxError error =
          new LynxError(LynxSubErrorCode.E_RESOURCE_MODULE_IMG_PREFETCH_HELPER_NOT_EXIST,
              "An exception occurred when try to get image prefetch helper.",
              "An error occurred while attempting to create a Java object "
                  + "ImagePrefetchHelper through reflection. This may be due to a "
                  + "change in the constructor interface of ImagePrefetchHelper, or "
                  + "because ImagePrefetchHelper is located in a plugin that is not "
                  + "ready. If you are unable to resolve this issue, you can seek "
                  + "help from the client RD.",
              LynxError.LEVEL_ERROR);
      onErrorOccurred(error);
    }
  }

  @LynxMethod
  void cancelResourcePrefetch(final ReadableMap data, final Callback callback) {
    TraceEvent.beginSection("cancelResourcePrefetch");

    JavaOnlyMap allResults = new JavaOnlyMap();
    Pair<Integer, String> resultPair = resourcePrefetch(data, true, allResults);
    Integer globalCode = resultPair.first;
    String globalMsg = resultPair.second;

    TraceEvent.endSection("cancelResourcePrefetch");
    allResults.putInt(CODE_KEY, globalCode);
    allResults.putString(MSG_KEY, globalMsg);
    if (callback != null) {
      callback.invoke(allResults);
    }
  }

  @LynxMethod
  void requestResourcePrefetch(final ReadableMap data, final Callback callback) {
    TraceEvent.beginSection("requestResourcePrefetch");

    JavaOnlyMap allResults = new JavaOnlyMap();
    Pair<Integer, String> resultPair = resourcePrefetch(data, false, allResults);
    Integer globalCode = resultPair.first;
    String globalMsg = resultPair.second;

    TraceEvent.endSection("requestResourcePrefetch");
    allResults.putInt(CODE_KEY, globalCode);
    allResults.putString(MSG_KEY, globalMsg);
    if (callback != null) {
      callback.invoke(allResults);
    }
  }

  private Pair<Integer, String> resourcePrefetch(
      final ReadableMap data, final boolean isCancel, final JavaOnlyMap allResults) {
    Integer globalCode = LynxSubErrorCode.E_SUCCESS;
    String globalMsg = "";
    ReadableArray resArray = data.getArray(DATA_KEY, null);
    if (resArray == null) {
      globalCode = LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR;
      globalMsg =
          "Parameters error in Lynx resource prefetch module! Value of 'data' should be an array.";
      LynxError error = new LynxError(globalCode, globalMsg,
          "Please check the parameters passed to Lynx resource prefetch module.",
          LynxError.LEVEL_ERROR);
      error.addCustomInfo("actionType", isCancel ? "cancel" : "request");
      onErrorOccurred(error);
    } else {
      JavaOnlyArray resultArray = new JavaOnlyArray();
      for (int i = 0; i < resArray.size(); ++i) {
        Integer code = LynxSubErrorCode.E_SUCCESS;
        String msg = "";
        JavaOnlyMap result = new JavaOnlyMap();
        String uri = "";
        if (resArray.getType(i) != ReadableType.Map) {
          code = LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR;
          msg =
              "Parameters error in Lynx resource prefetch module! The prefetch data should be a map.";
        } else {
          ReadableMap resData = resArray.getMap(i);
          uri = resData.getString(URI_KEY, null);
          String type = resData.getString(TYPE_KEY, null);
          ReadableMap params = resData.getMap(PARAMS_KEY, null);
          if (uri == null || type == null) {
            code = LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR;
            msg = "Parameters error in Lynx resource prefetch module! 'uri' or 'type' is null.";
          } else {
            Pair<Integer, String> resultPair = isCancel
                ? cancelResourcePrefetchInternal(uri, type, params)
                : requestResourcePrefetchInternal(uri, type, params);
            code = resultPair.first;
            msg = resultPair.second;
            result.putString(URI_KEY, uri);
            result.putString(TYPE_KEY, type);
          }
        }
        if (code != LynxSubErrorCode.E_SUCCESS) {
          LynxError error = new LynxError(code, msg,
              "If it is a parameter error, please check the parameters passed in. If the Resource service does not exist, it may be due to an error that occurred while creating the resource service through reflection. Please contact the client RD for help.",
              LynxError.LEVEL_ERROR);
          error.addCustomInfo("resourceUri", uri);
          error.addCustomInfo("actionType", isCancel ? "cancel" : "request");
          onErrorOccurred(error);
        }
        result.putInt(CODE_KEY, code);
        result.putString(MSG_KEY, msg);
        resultArray.pushMap(result);
      }
      allResults.putArray(DETAIL_KEY, resultArray);
    }
    return new Pair<>(globalCode, globalMsg);
  }

  private Pair<Integer, String> requestResourcePrefetchInternal(
      String uri, String type, @Nullable ReadableMap params) {
    Integer code = LynxSubErrorCode.E_SUCCESS;
    String msg = "";
    switch (type) {
      case IMAGE_TYPE: {
        if (mImagePrefetchHelper == null) {
          code = LynxSubErrorCode.E_RESOURCE_MODULE_IMG_PREFETCH_HELPER_NOT_EXIST;
          msg = "Image prefetch helper do not exist!";
          break;
        }
        mImagePrefetchHelper.prefetchImage(uri, mLynxContext.getFrescoCallerContext(), params);
        break;
      }
      case AUDIO_TYPE:
      case VIDEO_TYPE: {
        if (params == null) {
          code = LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR;
          msg = "missing preloadKey!";
        } else {
          String preloadKey = params.getString("preloadKey", null);
          String videoID = params.getString("videoID", null);
          long size = params.getLong("size", DEFAULT_MEDIA_SIZE);
          ILynxResourceService service =
              LynxServiceCenter.inst().getService(ILynxResourceService.class);
          if (service == null) {
            code = LynxSubErrorCode.E_RESOURCE_MODULE_RESOURCE_SERVICE_NOT_EXIST;
            msg = "Resource service do not exist!";
          } else if (preloadKey == null) {
            code = LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR;
            msg = "missing preloadKey!";
          } else {
            service.preloadMedia(uri, preloadKey, videoID, size);
          }
        }
        break;
      }
      default: {
        code = LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR;
        msg = "Parameters error! Unknown type :" + type;
        break;
      }
    }
    LLog.i("LynxResourceModule", "requestResourcePrefetch uri: " + uri + " type: " + type);
    return new Pair<>(code, msg);
  }

  private Pair<Integer, String> cancelResourcePrefetchInternal(
      String uri, String type, @Nullable ReadableMap params) {
    Integer code = LynxSubErrorCode.E_SUCCESS;
    String msg = "";
    switch (type) {
      case IMAGE_TYPE: {
        if (mImagePrefetchHelper == null) {
          code = LynxSubErrorCode.E_RESOURCE_MODULE_IMG_PREFETCH_HELPER_NOT_EXIST;
          msg = "Image prefetch helper do not exist!";
          break;
        }
        // TODO:@wujintian cancel prefetch image
        break;
      }
      case AUDIO_TYPE:
      case VIDEO_TYPE: {
        if (params == null) {
          code = LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR;
          msg = "missing preloadKey!";
        } else {
          String preloadKey = params.getString("preloadKey", null);
          String videoID = params.getString("videoID", null);
          ILynxResourceService service =
              LynxServiceCenter.inst().getService(ILynxResourceService.class);
          if (service == null) {
            code = LynxSubErrorCode.E_RESOURCE_MODULE_RESOURCE_SERVICE_NOT_EXIST;
            msg = "Resource service do not exist!";
          } else if (preloadKey == null) {
            code = LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR;
            msg = "missing preloadKey!";
          } else {
            service.cancelPreloadMedia(preloadKey, videoID);
          }
        }
        break;
      }
      default: {
        code = LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR;
        msg = "Parameters error! Unknown type :" + type;
        break;
      }
    }
    LLog.i("LynxResourceModule", "requestResourcePrefetch uri: " + uri + " type: " + type);
    return new Pair<>(code, msg);
  }

  private void onErrorOccurred(LynxError error) {
    mLynxContext.handleLynxError(error);
  }
}
