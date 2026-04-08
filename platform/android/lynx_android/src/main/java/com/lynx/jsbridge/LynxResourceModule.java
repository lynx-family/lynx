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
import com.lynx.tasm.base.trace.TraceEventDef;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.fontface.FontFaceManager;
import com.lynx.tasm.image.ImageContent;
import com.lynx.tasm.image.model.ImageInfo;
import com.lynx.tasm.image.model.ImageLoadListener;
import com.lynx.tasm.image.model.ImageRequestInfo;
import com.lynx.tasm.service.ILynxImageService;
import com.lynx.tasm.service.ILynxResourceService;
import com.lynx.tasm.service.LynxServiceCenter;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;
import java.util.Map;
import org.json.JSONObject;

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
  public static final String FONT_TYPE = "font";

  public static final long DEFAULT_MEDIA_SIZE = 500 * 1024;

  private static final String PARAM_ERROR_FIX_SUGGESTION =
      "Please check the parameters passed to Lynx resource prefetch module.";

  private static final String RESOURCE_SERVICE_ERROR_FIX_SUGGESTION =
      "If it is a parameter error, please check the parameters passed in. If the Resource service "
      + "does not exist, it may be due to an error that occurred while creating the resource "
      + "service through reflection. Please contact the client RD for help.";

  private static final String AWAIT_TIMEOUT_MSG =
      "The prefetch task did not complete within the specified timeout.";

  private static final String PARAM_ERROR_DATA_NOT_ARRAY =
      "Parameters error in Lynx resource prefetch module! Value of 'data' should be an array.";

  private static final String PARAM_ERROR_NOT_MAP =
      "Parameters error in Lynx resource prefetch module! The prefetch data should be a map.";

  private static final String PARAM_ERROR_URI_OR_TYPE_NULL =
      "Parameters error in Lynx resource prefetch module! 'uri' or 'type' is null.";

  private static final String IMG_HELPER_NOT_EXIST_MSG = "Image prefetch helper do not exist!";

  private static final String RESOURCE_SERVICE_NOT_EXIST_MSG = "Resource service do not exist!";

  private static final String MISSING_PRELOAD_KEY_MSG = "missing preloadKey!";

  private interface PrefetchInternalCallback {
    void onComplete(int code, String msg);
  }

  private ILynxImageService mImagePrefetchHelper;

  @RequiresApi(api = Build.VERSION_CODES.KITKAT)
  public LynxResourceModule(LynxContext context) {
    super(context);
    mImagePrefetchHelper = LynxServiceCenter.inst().getService(ILynxImageService.class);
    if (mImagePrefetchHelper == null) {
      reportError(LynxSubErrorCode.E_RESOURCE_MODULE_IMG_PREFETCH_HELPER_NOT_EXIST,
          "An exception occurred when try to get image prefetch helper.",
          "An error occurred while attempting to create a Java object "
              + "ImagePrefetchHelper through reflection. This may be due to a "
              + "change in the constructor interface of ImagePrefetchHelper, or "
              + "because ImagePrefetchHelper is located in a plugin that is not "
              + "ready. If you are unable to resolve this issue, you can seek "
              + "help from the client RD.",
          null, null);
    }
  }

  @LynxMethod
  void cancelResourcePrefetch(final ReadableMap data, final Callback callback) {
    TraceEvent.beginSection(TraceEventDef.CANCEL_RESOURCE_PREFETCH);
    JavaOnlyMap allResults = resourcePrefetchSync(data, true);
    TraceEvent.endSection(TraceEventDef.CANCEL_RESOURCE_PREFETCH);
    invokeCallback(callback, allResults);
  }

  @LynxMethod
  void requestResourcePrefetch(
      final ReadableMap data, final Callback callback, @Nullable final ReadableMap config) {
    TraceEvent.beginSection(TraceEventDef.REQUEST_RESOURCE_PREFETCH);

    boolean awaitComplete = false;
    long awaitTimeout = 60000;
    if (config != null) {
      awaitComplete = config.getBoolean("awaitComplete", awaitComplete);
      awaitTimeout = config.getLong("awaitTimeout", awaitTimeout);
    }

    if (awaitComplete) {
      LLog.i(NAME, " start resourcePrefetchAwaitComplete with timeout: " + awaitTimeout);
      resourcePrefetchAsync(data, callback, awaitTimeout);
    } else {
      JavaOnlyMap allResults = resourcePrefetchSync(data, false);
      invokeCallback(callback, allResults);
    }

    TraceEvent.endSection(TraceEventDef.REQUEST_RESOURCE_PREFETCH);
  }

  @LynxMethod
  public void requestResourcePrefetchImage(final ReadableMap data, final Callback callback) {
    String uri = data.getString(URI_KEY, null);
    if (uri == null) {
      int code = LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR;
      String msg = "Parameters error in Lynx resource prefetch module! 'uri' is null.";
      reportError(code, msg, PARAM_ERROR_FIX_SUGGESTION, null, null);
      invokeCallback(callback, createResult(code, msg));
      return;
    }
    if (mImagePrefetchHelper == null) {
      reportError(LynxSubErrorCode.E_RESOURCE_MODULE_IMG_PREFETCH_HELPER_NOT_EXIST,
          IMG_HELPER_NOT_EXIST_MSG, RESOURCE_SERVICE_ERROR_FIX_SUGGESTION, null, null);
      invokeCallback(callback,
          createResult(LynxSubErrorCode.E_RESOURCE_MODULE_IMG_PREFETCH_HELPER_NOT_EXIST,
              IMG_HELPER_NOT_EXIST_MSG));
      return;
    }
    ReadableMap params = data.getMap(PARAMS_KEY, null);
    mImagePrefetchHelper.prefetchImage(uri, mLynxContext.getFrescoCallerContext(),
        (Map<String, Object>) params, new ImageLoadListener() {
          @Override
          public void onRequestSubmit(ImageRequestInfo imageRequestInfo) {}

          // imageContent, requestInfo, and imageInfo are null and cannot be used.
          @Override
          public void onSuccess(@Nullable ImageContent imageContent, ImageRequestInfo requestInfo,
              ImageInfo imageInfo) {
            invokeCallback(callback, createResult(0, ""));
          }

          @Override
          public void onFailure(int errorCode, Throwable throwable) {
            invokeCallback(callback, createResult(errorCode, "prefetch image failed"));
          }

          @Override
          public void onImageMonitorInfo(JSONObject monitorInfo) {}
        });
  }

  private JavaOnlyMap resourcePrefetchSync(final ReadableMap data, final boolean isCancel) {
    String actionType = isCancel ? "cancel" : "request";
    ReadableArray resArray = data.getArray(DATA_KEY, null);
    if (resArray == null) {
      reportError(LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR, PARAM_ERROR_DATA_NOT_ARRAY,
          PARAM_ERROR_FIX_SUGGESTION, null, actionType);
      return createResult(
          LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR, PARAM_ERROR_DATA_NOT_ARRAY);
    }

    JavaOnlyMap allResults = createResult(LynxSubErrorCode.E_SUCCESS, "");
    JavaOnlyArray resultArray = new JavaOnlyArray();
    for (int i = 0; i < resArray.size(); ++i) {
      String uri = "";
      String type = "";
      int code;
      String msg;

      if (resArray.getType(i) != ReadableType.Map) {
        code = LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR;
        msg = PARAM_ERROR_NOT_MAP;
      } else {
        ReadableMap resData = resArray.getMap(i);
        uri = resData.getString(URI_KEY, null);
        type = resData.getString(TYPE_KEY, null);
        if (uri == null || type == null) {
          code = LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR;
          msg = PARAM_ERROR_URI_OR_TYPE_NULL;
        } else {
          Pair<Integer, String> resultPair = isCancel
              ? cancelResourcePrefetchInternal(uri, type, resData.getMap(PARAMS_KEY, null))
              : requestResourcePrefetchInternal(uri, type, resData.getMap(PARAMS_KEY, null), null);
          code = resultPair.first;
          msg = resultPair.second;
        }
      }

      if (code != LynxSubErrorCode.E_SUCCESS) {
        reportError(code, msg, RESOURCE_SERVICE_ERROR_FIX_SUGGESTION, uri, actionType);
      }
      JavaOnlyMap result = createResult(code, msg);
      if (uri != null && !uri.isEmpty()) {
        result.putString(URI_KEY, uri);
      }
      if (type != null && !type.isEmpty()) {
        result.putString(TYPE_KEY, type);
      }
      resultArray.pushMap(result);
    }
    allResults.putArray(DETAIL_KEY, resultArray);
    return allResults;
  }

  private void resourcePrefetchAsync(
      final ReadableMap data, final Callback callback, long awaitTimeout) {
    final ReadableArray resArray = data.getArray(DATA_KEY, null);
    if (resArray == null) {
      reportError(LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR, PARAM_ERROR_DATA_NOT_ARRAY,
          PARAM_ERROR_FIX_SUGGESTION, null, "request");
      invokeCallback(callback,
          createResult(
              LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR, PARAM_ERROR_DATA_NOT_ARRAY));
      return;
    }

    final int totalCount = resArray.size();
    if (totalCount == 0) {
      invokeCallback(callback, createResult(LynxSubErrorCode.E_SUCCESS, ""));
      return;
    }

    final JavaOnlyMap allResults = createResult(LynxSubErrorCode.E_SUCCESS, "");
    final int[] completedCount = {0};
    final boolean[] isCallbackInvoked = {false};
    final JavaOnlyArray resultArray = new JavaOnlyArray();
    allResults.putArray(DETAIL_KEY, resultArray);

    final WeakReference<LynxResourceModule> weakModule = new WeakReference<>(this);

    scheduleTimeout(
        weakModule, callback, allResults, isCallbackInvoked, completedCount, awaitTimeout);

    for (int i = 0; i < totalCount; ++i) {
      final String uri;
      final String type;
      final ReadableMap params;
      final int initialCode;
      final String initialMsg;

      if (resArray.getType(i) != ReadableType.Map) {
        initialCode = LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR;
        initialMsg = PARAM_ERROR_NOT_MAP;
        uri = "";
        type = "";
        params = null;
      } else {
        ReadableMap resData = resArray.getMap(i);
        uri = resData.getString(URI_KEY, "");
        type = resData.getString(TYPE_KEY, "");
        params = resData.getMap(PARAMS_KEY, null);
        if (uri == null || type == null || uri.isEmpty() || type.isEmpty()) {
          initialCode = LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR;
          initialMsg = PARAM_ERROR_URI_OR_TYPE_NULL;
        } else {
          initialCode = LynxSubErrorCode.E_SUCCESS;
          initialMsg = "";
        }
      }

      PrefetchInternalCallback internalCallback = new PrefetchInternalCallback() {
        @Override
        public void onComplete(final int code, final String msg) {
          LynxResourceModule module = weakModule.get();
          if (module == null || module.mLynxContext == null) {
            return;
          }
          if (code != LynxSubErrorCode.E_SUCCESS) {
            module.reportError(code, msg, RESOURCE_SERVICE_ERROR_FIX_SUGGESTION, uri, "request");
          }
          module.mLynxContext.runOnJSThread(new Runnable() {
            @Override
            public void run() {
              if (isCallbackInvoked[0]) {
                return;
              }
              JavaOnlyMap result = createResult(code, msg);
              result.putString(URI_KEY, uri);
              result.putString(TYPE_KEY, type);
              resultArray.pushMap(result);
              completedCount[0]++;
              LLog.i(NAME, "onComplete:" + uri + " msg:" + msg + " count:" + completedCount[0]);
              if (completedCount[0] == totalCount && callback != null) {
                isCallbackInvoked[0] = true;
                callback.invoke(allResults);
              }
            }
          });
        }
      };

      if (initialCode != LynxSubErrorCode.E_SUCCESS) {
        internalCallback.onComplete(initialCode, initialMsg);
      } else {
        requestResourcePrefetchInternal(uri, type, params, internalCallback);
      }
    }
  }

  private void scheduleTimeout(final WeakReference<LynxResourceModule> weakModule,
      final Callback callback, final JavaOnlyMap allResults, final boolean[] isCallbackInvoked,
      final int[] completedCount, long awaitTimeout) {
    UIThreadUtils.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        LynxResourceModule module = weakModule.get();
        if (module == null || module.mLynxContext == null) {
          return;
        }
        module.mLynxContext.runOnJSThread(new Runnable() {
          @Override
          public void run() {
            if (isCallbackInvoked[0]) {
              return;
            }
            isCallbackInvoked[0] = true;
            allResults.putInt(CODE_KEY, LynxSubErrorCode.E_RESOURCE_MODULE_AWAIT_TIMEOUT);
            allResults.putString(MSG_KEY, AWAIT_TIMEOUT_MSG);
            LynxResourceModule module = weakModule.get();
            if (module != null) {
              module.reportError(LynxSubErrorCode.E_RESOURCE_MODULE_AWAIT_TIMEOUT,
                  AWAIT_TIMEOUT_MSG, RESOURCE_SERVICE_ERROR_FIX_SUGGESTION, null, "request");
            }
            if (callback != null) {
              LLog.i(NAME, " timeout invoke, count: " + completedCount[0]);
              callback.invoke(allResults);
            }
          }
        });
      }
    }, awaitTimeout);
  }

  private Pair<Integer, String> requestResourcePrefetchInternal(String uri, String type,
      @Nullable ReadableMap params, @Nullable final PrefetchInternalCallback asyncCallback) {
    int code = LynxSubErrorCode.E_SUCCESS;
    String msg = "";
    boolean isAsync = false;
    switch (type) {
      case IMAGE_TYPE: {
        if (mImagePrefetchHelper == null) {
          code = LynxSubErrorCode.E_RESOURCE_MODULE_IMG_PREFETCH_HELPER_NOT_EXIST;
          msg = IMG_HELPER_NOT_EXIST_MSG;
          break;
        }
        if (asyncCallback != null) {
          isAsync = true;
          mImagePrefetchHelper.prefetchImage(uri, mLynxContext.getFrescoCallerContext(),
              (Map<String, Object>) params, new ImageLoadListener() {
                @Override
                public void onRequestSubmit(ImageRequestInfo imageRequestInfo) {}

                @Override
                public void onSuccess(@Nullable ImageContent imageContent,
                    ImageRequestInfo requestInfo, ImageInfo imageInfo) {
                  asyncCallback.onComplete(LynxSubErrorCode.E_SUCCESS, "");
                }

                @Override
                public void onFailure(int errorCode, Throwable throwable) {
                  asyncCallback.onComplete(errorCode, "prefetch image failed");
                }

                @Override
                public void onImageMonitorInfo(JSONObject monitorInfo) {}
              });
        } else {
          mImagePrefetchHelper.prefetchImage(
              uri, mLynxContext.getFrescoCallerContext(), (Map<String, Object>) params);
        }
        break;
      }
      case FONT_TYPE: {
        if (asyncCallback != null) {
          isAsync = true;
          FontFaceManager.getInstance().prefetchFont(
              mLynxContext, uri, params, new FontFaceManager.FontFacePrefetchListener() {
                @Override
                public void onComplete(int code, String msg) {
                  asyncCallback.onComplete(code, msg);
                }
              });
        } else {
          FontFaceManager.getInstance().prefetchFont(mLynxContext, uri, params, null);
        }
        break;
      }
      case AUDIO_TYPE:
      case VIDEO_TYPE: {
        ILynxResourceService.PreloadMediaCallback mediaCallback = null;
        if (asyncCallback != null) {
          mediaCallback = new ILynxResourceService.PreloadMediaCallback() {
            @Override
            public void onComplete(int code, String msg) {
              asyncCallback.onComplete(code, msg);
            }
          };
        }
        Pair<Integer, String> mediaResult = resolveMediaAndPreload(uri, params, mediaCallback);
        code = mediaResult.first;
        msg = mediaResult.second;
        isAsync = (asyncCallback != null && code == LynxSubErrorCode.E_SUCCESS);
        break;
      }
      default: {
        code = LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR;
        msg = "Parameters error! Unknown type :" + type;
        break;
      }
    }
    if (asyncCallback != null && !isAsync) {
      asyncCallback.onComplete(code, msg);
    }
    LLog.i(NAME, "requestResourcePrefetch uri: " + uri + " type: " + type);
    return new Pair<>(code, msg);
  }

  private Pair<Integer, String> cancelResourcePrefetchInternal(
      String uri, String type, @Nullable ReadableMap params) {
    Pair<Integer, String> result;
    switch (type) {
      case IMAGE_TYPE:
        if (mImagePrefetchHelper == null) {
          result = new Pair<>(LynxSubErrorCode.E_RESOURCE_MODULE_IMG_PREFETCH_HELPER_NOT_EXIST,
              IMG_HELPER_NOT_EXIST_MSG);
          break;
        }
        // TODO:@wujintian cancel prefetch image
        result = successResult();
        break;
      case AUDIO_TYPE:
      case VIDEO_TYPE:
        result = resolveMediaAndCancel(params);
        break;
      default:
        result = new Pair<>(LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR,
            "Parameters error! Unknown type :" + type);
        break;
    }
    LLog.i(NAME, "cancelResourcePrefetch uri: " + uri + " type: " + type);
    return result;
  }

  private Pair<Integer, String> resolveMediaAndPreload(String uri, @Nullable ReadableMap params,
      @Nullable ILynxResourceService.PreloadMediaCallback asyncCallback) {
    if (params == null) {
      return new Pair<>(LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR, MISSING_PRELOAD_KEY_MSG);
    }
    String preloadKey = params.getString("preloadKey", null);
    if (preloadKey == null) {
      return new Pair<>(LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR, MISSING_PRELOAD_KEY_MSG);
    }
    ILynxResourceService service = LynxServiceCenter.inst().getService(ILynxResourceService.class);
    if (service == null) {
      return new Pair<>(LynxSubErrorCode.E_RESOURCE_MODULE_RESOURCE_SERVICE_NOT_EXIST,
          RESOURCE_SERVICE_NOT_EXIST_MSG);
    }
    String videoID = params.getString("videoID", null);
    long size = params.getLong("size", DEFAULT_MEDIA_SIZE);
    service.preloadMedia(uri, preloadKey, videoID, size, asyncCallback);
    return successResult();
  }

  private Pair<Integer, String> resolveMediaAndCancel(@Nullable ReadableMap params) {
    if (params == null) {
      return new Pair<>(LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR, MISSING_PRELOAD_KEY_MSG);
    }
    String preloadKey = params.getString("preloadKey", null);
    if (preloadKey == null) {
      return new Pair<>(LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR, MISSING_PRELOAD_KEY_MSG);
    }
    ILynxResourceService service = LynxServiceCenter.inst().getService(ILynxResourceService.class);
    if (service == null) {
      return new Pair<>(LynxSubErrorCode.E_RESOURCE_MODULE_RESOURCE_SERVICE_NOT_EXIST,
          RESOURCE_SERVICE_NOT_EXIST_MSG);
    }
    String videoID = params.getString("videoID", null);
    service.cancelPreloadMedia(preloadKey, videoID);
    return successResult();
  }

  private static Pair<Integer, String> successResult() {
    return new Pair<>(LynxSubErrorCode.E_SUCCESS, "");
  }

  private static void invokeCallback(@Nullable Callback callback, JavaOnlyMap result) {
    if (callback != null) {
      callback.invoke(result);
    }
  }

  private void reportError(int code, String msg, String fixSuggestion, @Nullable String uri,
      @Nullable String actionType) {
    LynxError error = new LynxError(code, msg, fixSuggestion, LynxError.LEVEL_ERROR);
    if (uri != null) {
      error.addCustomInfo("resourceUri", uri);
    }
    if (actionType != null) {
      error.addCustomInfo("actionType", actionType);
    }
    mLynxContext.handleLynxError(error);
  }

  private JavaOnlyMap createResult(int code, String msg) {
    JavaOnlyMap result = new JavaOnlyMap();
    result.putInt(CODE_KEY, code);
    result.putString(MSG_KEY, msg);
    return result;
  }
}
