// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.core.resource;

import android.text.TextUtils;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.ILynxErrorReceiver;
import com.lynx.tasm.LynxBackgroundRuntimeOptions;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.LynxInfoReportHelper;
import com.lynx.tasm.LynxSubErrorCode;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.component.DynamicComponentFetcher;
import com.lynx.tasm.core.LynxThreadPool;
import com.lynx.tasm.core.resource.LynxResourceType;
import com.lynx.tasm.provider.LynxExternalResourceFetcherWrapper;
import com.lynx.tasm.provider.LynxProviderRegistry;
import com.lynx.tasm.provider.LynxResourceCallback;
import com.lynx.tasm.provider.LynxResourceProvider;
import com.lynx.tasm.provider.LynxResourceRequest;
import com.lynx.tasm.provider.LynxResourceResponse;
import com.lynx.tasm.resourceprovider.generic.LynxGenericResourceFetcher;
import com.lynx.tasm.resourceprovider.template.LynxTemplateResourceFetcher;
import com.lynx.tasm.utils.UIThreadUtils;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.lang.ref.WeakReference;

public class LynxResourceLoader {
  static final String CORE_JS = "assets://lynx_core.js";
  static final String CORE_DEBUG_JS = "lynx_core_dev.js";
  static final String FILE_SCHEME = "file://";
  static final String ASSETS_SCHEME = "assets://";
  // For compatibility with iOS, on iOS the path of assets://lynx_core.js and assets://[others].js
  // is different
  static final String LYNX_ASSETS_SCHEME = "lynx_assets://";

  static final int RESOURCE_LOADER_SUCCESS = 0;
  static final int RESOURCE_LOADER_FAILED = -1;
  static final String TAG = "LynxResourceLoader";
  static final String METHOD_NAME_LOAD_SCRIPT = "loadExternalResource";
  static final String METHOD_NAME_LOAD_LOCAL_SCRIPT = "loadLocalResource";
  static final String MSG_NULL_DATA = "get null data for provider.";

  private final LynxBackgroundRuntimeOptions mLynxRuntimeOptions;
  private final LynxExternalResourceFetcherWrapper mFetcherWrapper;
  private final TemplateLoaderHelper mTemplateLoaderHelper;
  private final LynxGenericResourceFetcher mGenericResourceFetcher;

  private final WeakReference<ILynxErrorReceiver> mWeekErrorReceiver;

  private final LynxInfoReportHelper mReportHelper = new LynxInfoReportHelper();

  public LynxResourceLoader(LynxBackgroundRuntimeOptions runtimeOptions,
      DynamicComponentFetcher fetcher, ILynxErrorReceiver errorReceiver,
      LynxTemplateResourceFetcher templateFetcher,
      LynxGenericResourceFetcher genericResourceFetcher) {
    mLynxRuntimeOptions = runtimeOptions;
    mFetcherWrapper = new LynxExternalResourceFetcherWrapper(fetcher);
    mWeekErrorReceiver = new WeakReference<>(errorReceiver);
    mTemplateLoaderHelper = new TemplateLoaderHelper(templateFetcher);
    mGenericResourceFetcher = genericResourceFetcher;
  }

  @CalledByNative
  private void loadResource(long responseHandler, String url, int type, boolean reqInCurThread) {
    if (reqInCurThread) {
      loadResource(responseHandler, url, type);
    } else {
      try {
        LynxThreadPool.getBriefIOExecutor().execute(new Runnable() {
          @Override
          public void run() {
            loadResource(responseHandler, url, type);
          }
        });
      } catch (Throwable e) {
        // to release native pointer if exception occurs
        InvokeNativeCallbackWithBytes(
            responseHandler, null, RESOURCE_LOADER_FAILED, e.getMessage());
      }
    }
  }

  void loadResource(long responseHandler, String url, int type) {
    switch (type) {
      case LynxResourceType.LYNX_RESOURCE_TYPE_ASSETS:
        byte[] data = loadJSSource(url);
        InvokeNativeCallbackWithBytes(responseHandler, data, RESOURCE_LOADER_SUCCESS, null);
        break;
      case LynxResourceType.LYNX_RESOURCE_TYPE_EXTERNAL_BYTE_CODE:
        if (fetchExternalByteCodeByGenericFetcher(responseHandler, url)) {
          break;
        }
        InvokeNativeCallbackWithBytes(
            responseHandler, null, RESOURCE_LOADER_FAILED, "No available provider or fetcher.");
        break;
      case LynxResourceType.LYNX_RESOURCE_TYPE_EXTERNAL_JS:
        // 1. try to use GenericResourceFetcher
        if (fetchScriptByGenericFetcher(responseHandler, url)) {
          break;
        }
        // 2. try to use external js provider;
        if (fetchScriptByProvider(responseHandler, url)) {
          break;
        }
        // invoke callback directly if no provider or fetcher set;
        InvokeNativeCallbackWithBytes(
            responseHandler, null, RESOURCE_LOADER_FAILED, "No available provider or fetcher.");
        break;
      case LynxResourceType.LYNX_RESOURCE_TYPE_JS_LAZY_BUNDLE:
        // 1. try to use LynxTemplateResourceFetcher
        if (fetchTemplateByGenericTemplateFetcher(responseHandler, url, type)) {
          break;
        }
        // 2. try to use LynxExternalResourceFetcherWrapper
        if (fetchTemplateByFetcherWrapper(responseHandler, url, type)) {
          break;
        }
        // 3. try to use LynxResourceProvider
        if (fetchTemplateByProvider(responseHandler, url, type)) {
          break;
        }
        // invoke callback directly if no provider or fetcher set;
        InvokeNativeCallbackWithBytes(
            responseHandler, null, RESOURCE_LOADER_FAILED, "No available provider or fetcher.");
        break;
      case LynxResourceType.LYNX_RESOURCE_TYPE_FRAME:
        // 1. frame only support LynxTemplateResourceFetcher
        if (fetchTemplateByGenericTemplateFetcher(responseHandler, url, type)) {
          break;
        }
        // invoke callback directly if no provider or fetcher set;
        InvokeNativeCallbackWithBytes(
            responseHandler, null, RESOURCE_LOADER_FAILED, "No available provider or fetcher.");
        break;
      default:
        InvokeNativeCallbackWithBytes(
            responseHandler, null, RESOURCE_LOADER_FAILED, "Unsupported type" + type);
        break;
    }
  }

  private String toTypeString(int type) {
    switch (type) {
      case LynxResourceType.LYNX_RESOURCE_TYPE_EXTERNAL_JS:
        return LynxProviderRegistry.LYNX_PROVIDER_TYPE_EXTERNAL_JS;
      case LynxResourceType.LYNX_RESOURCE_TYPE_JS_LAZY_BUNDLE:
        return LynxProviderRegistry.LYNX_PROVIDER_TYPE_DYNAMIC_COMPONENT;
    }
    return "";
  }

  void reportError(final String methodName, final String url, final int code, final String errorMsg,
      final String rootCause) {
    UIThreadUtils.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        ILynxErrorReceiver errorReceiver = mWeekErrorReceiver.get();
        if (errorReceiver == null) {
          return;
        }
        LynxError lynxError = new LynxError(code,
            String.format("%s %s failed, the error message is: %s", TAG, methodName, errorMsg),
            LynxError.LYNX_ERROR_SUGGESTION_REF_OFFICIAL_SITE, LynxError.LEVEL_ERROR);
        lynxError.setRootCause(rootCause);
        lynxError.addCustomInfo(LynxError.LYNX_ERROR_KEY_RESOURCE_URL, url);
        errorReceiver.onErrorOccurred(lynxError);
      }
    });
  }

  private byte[] toByteArray(@NonNull InputStream in) throws IOException {
    ByteArrayOutputStream out = new ByteArrayOutputStream();
    byte[] buffer = new byte[1024 * 4];
    int n = 0;
    while ((n = in.read(buffer)) != -1) {
      out.write(buffer, 0, n);
    }
    return out.toByteArray();
  }

  private byte[] loadJSSource(String name) {
    /* 1. if the file name is assets://lynx_core.js
     *   i. if devtool is enabled, try to load assets://lynx_core_dev.js
     * 2. if the file name is start with "file://", use FileInputStream to get the file content.
     * 3. if the file name is start with "assets://", use
     * LynxEnv.inst().getAppContext().getResources().getAssets().open() to get the
     * file content
     * 4. if the file name is start with "lynx_assets://", the same as above. (the reason for this
     * is compatibility with iOS resource loader, the paths of assets://lynx_core.js and
     * assets://other_file.js are different in iOS)
     * */
    if (TextUtils.isEmpty(name)) {
      LLog.w(TAG, "loadJSSource failed with empty name");
      return null;
    }
    LLog.i(TAG, "loadJSSource with name " + name);
    InputStream inputStream = null;
    String errorMessage = null;
    try {
      if (CORE_JS.equals(name)) {
        //  get DebugResource from DevTool
        if (LynxEnv.inst().isDevtoolEnabled()) {
          try {
            inputStream =
                LynxEnv.inst().getAppContext().getResources().getAssets().open(CORE_DEBUG_JS);
          } catch (IOException e) {
            e.printStackTrace();
          }
          if (inputStream != null) {
            nativeConfigLynxResourceSetting();
          }
        }
      }
      if (inputStream == null) {
        if (name.length() > FILE_SCHEME.length() && name.startsWith(FILE_SCHEME)) {
          String path = name.substring(FILE_SCHEME.length());
          File file;
          // An absolute path starts with /, A relative path starts with ./
          if (path.startsWith("/")) {
            file = new File(path);
          } else {
            file = new File(LynxEnv.inst().getAppContext().getFilesDir(), path);
          }
          inputStream = new FileInputStream(file);
        } else if (name.length() > ASSETS_SCHEME.length() && name.startsWith(ASSETS_SCHEME)) {
          inputStream = LynxEnv.inst().getAppContext().getResources().getAssets().open(
              name.substring(ASSETS_SCHEME.length()));
        } else if (name.startsWith(LYNX_ASSETS_SCHEME)) {
          return loadLynxJSAsset(name);
        }
      }
      if (null != inputStream) {
        return toByteArray(inputStream);
      }
    } catch (Throwable e) {
      errorMessage = e.getMessage();
    } finally {
      if (inputStream != null) {
        try {
          inputStream.close();
        } catch (IOException e) {
          // ignore
        }
      }
      if (errorMessage != null) {
        reportError(METHOD_NAME_LOAD_LOCAL_SCRIPT, name,
            LynxSubErrorCode.E_RESOURCE_EXTERNAL_RESOURCE_LOCAL_RESOURCE_LOAD_FAIL,
            "Error when loading js source", errorMessage);
        LLog.e(TAG, "loadJSSource " + name + "with error message: " + errorMessage);
      }
    }
    return null;
  }

  public byte[] loadLynxJSAsset(String name) {
    String assetName = name.substring(LYNX_ASSETS_SCHEME.length());
    InputStream inputStream = null;
    try {
      // If devtool is enabled, try to load [filename]_dev.js first.
      // If the file is not available, try to load [filename].js.
      if (LynxEnv.inst().isDevtoolEnabled()) {
        try {
          String[] assetSplitByDot = assetName.split("\\.");
          if (assetSplitByDot.length == 2) {
            // devAssetName = [filename]_dev.js
            String devAssetName = assetSplitByDot[0] + "_dev"
                + "." + assetSplitByDot[1];
            inputStream =
                LynxEnv.inst().getAppContext().getResources().getAssets().open(devAssetName);
          }
        } catch (IOException e) {
          // loading [devAssetName].js failed will try to load assetName
        }
      }
      // in prod or in dev but [filename]_dev.js unavailable
      if (null == inputStream) {
        inputStream = LynxEnv.inst().getAppContext().getResources().getAssets().open(assetName);
      }
      if (null != inputStream) {
        return toByteArray(inputStream);
      }
    } catch (Throwable e) {
      LLog.e(TAG, "loadLynxJSAsset " + name + "with error message " + e.getMessage());
    } finally {
      if (inputStream != null) {
        try {
          inputStream.close();
        } catch (IOException e) {
          LLog.e(TAG, "loadLynxJSAsset inputStream close error: " + e.getMessage());
        }
      }
    }
    LLog.e(TAG, "loadLynxJSAsset failed, can not load " + name);
    return null;
  }

  /**
   * fetch lazy bundle template by LynxExternalResourceFetcherWrapper
   */
  private boolean fetchTemplateByFetcherWrapper(
      long responseHandler, String url, int resourceType) {
    return mFetcherWrapper.fetchResourceWithDynamicComponentFetcher(
        url, new LynxExternalResourceFetcherWrapper.LoadedHandler() {
          private final TemplateResourceCallback mCallback =
              new TemplateResourceCallback(url, responseHandler, mReportHelper, resourceType);

          @Override
          public void onLoaded(@Nullable byte[] data, @Nullable Throwable error) {
            mCallback.onTemplateLoaded(
                error == null, data, null, error != null ? error.getMessage() : null);
          }
        });
  }

  /**
   * fetch lazy bundle template by LynxResourceProvider
   * @return whether the request has been sent, to determine whether using another loader
   */
  private boolean fetchTemplateByProvider(long responseHandler, String url, int resourceType) {
    LynxResourceProvider provider =
        getResourceProviderByType(LynxResourceType.LYNX_RESOURCE_TYPE_JS_LAZY_BUNDLE);
    if (provider == null) {
      return false;
    }
    final LynxResourceRequest request = new LynxResourceRequest(url);
    provider.request(request, new LynxResourceCallback<byte[]>() {
      private final TemplateResourceCallback mCallback =
          new TemplateResourceCallback(url, responseHandler, mReportHelper, resourceType);

      @Override
      public void onResponse(@NonNull LynxResourceResponse<byte[]> response) {
        super.onResponse(response);
        mCallback.onTemplateLoaded(response.success(), response.getData(), null,
            response.getError() != null ? response.getError().getMessage() : null);
      }
    });
    return true;
  }

  /**
   * fetch lazy bundle or frame template by LynxTemplateResourceFetcher
   * @return whether the request has been sent, to determine whether using another loader
   */
  private boolean fetchTemplateByGenericTemplateFetcher(
      long responseHandler, String url, int resourceType) {
    boolean hasTemplateFetcher = mTemplateLoaderHelper.hasTemplateFetcher();
    LLog.i(TAG, "Generic template fetcher existed: " + hasTemplateFetcher);
    if (!hasTemplateFetcher) {
      return false;
    }
    final TemplateResourceCallback callback =
        new TemplateResourceCallback(url, responseHandler, mReportHelper, resourceType);
    mTemplateLoaderHelper.fetchTemplateByGenericTemplateFetcher(url, callback);
    return true;
  }

  // TODO(nihao.royal): merge api called for generic fetcher;
  private boolean fetchExternalByteCodeByGenericFetcher(long responseHandler, String url) {
    if (mGenericResourceFetcher == null) {
      return false;
    }
    com.lynx.tasm.resourceprovider.LynxResourceRequest request =
        new com.lynx.tasm.resourceprovider.LynxResourceRequest(url,
            com.lynx.tasm.resourceprovider.LynxResourceRequest.LynxResourceType
                .LynxResourceTypeExternalByteCode);
    mGenericResourceFetcher.fetchResource(
        request, new com.lynx.tasm.resourceprovider.LynxResourceCallback<byte[]>() {
          private final GenericResourceCallback callback =
              new GenericResourceCallback(LynxResourceLoader.this, url, responseHandler);

          @Override
          public void onResponse(
              com.lynx.tasm.resourceprovider.LynxResourceResponse<byte[]> response) {
            boolean success = response.getState()
                == com.lynx.tasm.resourceprovider.LynxResourceResponse.ResponseState.SUCCESS;
            Throwable error = response.getError();
            callback.onResourceLoaded(
                success, response.getData(), error != null ? error.getMessage() : "");
          }
        });
    return true;
  }

  // use package path to avoid class name conflict
  private boolean fetchScriptByGenericFetcher(long responseHandler, String url) {
    if (mGenericResourceFetcher != null) {
      com.lynx.tasm.resourceprovider.LynxResourceRequest resourceRequest =
          new com.lynx.tasm.resourceprovider.LynxResourceRequest(url,
              com.lynx.tasm.resourceprovider.LynxResourceRequest.LynxResourceType
                  .LynxResourceTypeExternalJSSource);
      mGenericResourceFetcher.fetchResource(
          resourceRequest, new com.lynx.tasm.resourceprovider.LynxResourceCallback<byte[]>() {
            private final ExternalScriptResourceCallback callback =
                new ExternalScriptResourceCallback(LynxResourceLoader.this, url, responseHandler);

            @Override
            public void onResponse(
                com.lynx.tasm.resourceprovider.LynxResourceResponse<byte[]> response) {
              boolean success = response.getState()
                  == com.lynx.tasm.resourceprovider.LynxResourceResponse.ResponseState.SUCCESS;
              Throwable error = response.getError();
              callback.onScriptLoaded(
                  success, response.getData(), error != null ? error.getMessage() : "");
            }
          });
      return true;
    }
    return false;
  }

  /**
   * fetch script by LynxResourceProvider
   *
   * @return false if provider not set;
   */
  private boolean fetchScriptByProvider(long responseHandler, String url) {
    LynxResourceProvider provider =
        getResourceProviderByType(LynxResourceType.LYNX_RESOURCE_TYPE_EXTERNAL_JS);
    if (provider == null) {
      return false;
    }
    final LynxResourceRequest request = new LynxResourceRequest(url);
    provider.request(request, new LynxResourceCallback<byte[]>() {
      private final ExternalScriptResourceCallback callback =
          new ExternalScriptResourceCallback(LynxResourceLoader.this, url, responseHandler);

      @Override
      public void onResponse(@NonNull LynxResourceResponse<byte[]> response) {
        boolean success = response.success();
        Throwable error = response.getError();
        callback.onScriptLoaded(
            success, response.getData(), error != null ? error.getMessage() : null);
      }
    });
    return true;
  }

  private LynxResourceProvider getResourceProviderByType(int type) {
    LynxResourceProvider provider = mLynxRuntimeOptions != null
        ? mLynxRuntimeOptions.getResourceProvidersByKey(toTypeString(type))
        : null;
    if (provider == null) {
      LLog.e(TAG, "lynx resource provider is null, type: " + type);
    }
    return provider;
  }

  static void InvokeNativeCallbackWithBytes(
      long responseHandler, byte[] data, int errCode, String errMsg) {
    nativeInvokeCallback(responseHandler, data, 0L, errCode, errMsg);
  }

  static native void nativeInvokeCallback(
      long responseHandler, byte[] data, long bundleNativePtr, int errCode, String errMsg);

  private native void nativeConfigLynxResourceSetting();
}
