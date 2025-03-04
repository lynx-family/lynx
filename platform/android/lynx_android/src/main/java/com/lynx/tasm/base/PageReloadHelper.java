// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.base;

import android.text.TextUtils;
import android.util.Base64;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import com.lynx.tasm.LynxTemplateRender;
import com.lynx.tasm.TemplateBundle;
import com.lynx.tasm.TemplateData;
import com.lynx.tasm.behavior.shadow.text.TextRendererCache;
import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;

public class PageReloadHelper {
  private class InitData {
    private byte[] mTemplate = null;
    private String mBaseUrl = null;
  }

  private class InitUrlData {
    private byte[] mTemplate = null;
    private String mInitUrl = null;
  }

  private class InitBundleData {
    private TemplateBundle mTemplateBundle = null;
    private String mBaseUrl = null;
  }

  private static final String TAG = "PageReloadHelper";
  private WeakReference<LynxTemplateRender> mTemplateRender;
  private String mUrl;
  private boolean mInitWithBinary;
  private boolean mInitWithUrl;
  private boolean mInitWithBundle;
  private InitData mInitBinaryData;
  private InitBundleData mInitBundleData;
  private InitUrlData mInitUrlData;

  private ByteBuffer mFragmentsBuffer;
  private boolean mIgnoreCache;
  private TemplateData mInitTemplateData = TemplateData.fromMap(new HashMap<>());

  public PageReloadHelper(@Nullable LynxTemplateRender templateRender) {
    mTemplateRender = new WeakReference<>(templateRender);
    mUrl = null;
    mInitWithBinary = false;
    mInitWithUrl = false;
    mInitWithBundle = false;
    mInitBinaryData = new InitData();
    mInitBundleData = new InitBundleData();
    mInitUrlData = new InitUrlData();
    mFragmentsBuffer = null;
    mIgnoreCache = false;
  }

  public void attach(LynxTemplateRender templateRender) {
    mTemplateRender = new WeakReference<>(templateRender);
  }

  public void saveURL(@NonNull final String templateUrl, @Nullable final TemplateData templateData,
      @Nullable final Map<String, Object> map, @Nullable final String jsonData) {
    if (templateData != null) {
      loadFromURL(templateUrl, templateData);
    } else if (map != null) {
      loadFromURL(templateUrl, TemplateData.fromMap(map));
    } else if (jsonData != null) {
      loadFromURL(templateUrl, TemplateData.fromString(jsonData));
    } else {
      loadFromURL(templateUrl, null);
    }
  }

  public void loadFromURL(
      @NonNull final String templateUrl, @Nullable final TemplateData templateData) {
    LLog.i(TAG, "loadFromURL:" + templateUrl);
    mInitWithBinary = false;
    mInitWithBundle = false;
    mInitWithUrl = true;
    mInitUrlData.mInitUrl = templateUrl;
    updateInitTemplateData(templateData);

    mUrl = templateUrl;
  }

  public void update(TemplateData data) {
    updateInitTemplateData(data);
  }

  private void updateInitTemplateData(TemplateData data) {
    if (data == null) {
      return;
    }
    // Copy data and save it in mInitTemplateData. Don't be impacted by original templateData.
    mInitTemplateData.updateWithTemplateData(data.deepClone());
  }

  public void loadFromLocalFile(byte[] template, TemplateData templateData, String baseUrl) {
    LLog.i(TAG, "loadFromLocalFile:" + baseUrl);
    mInitWithUrl = false;
    mInitWithBundle = false;
    mInitWithBinary = true;
    mInitBinaryData.mTemplate = template;
    mInitBinaryData.mBaseUrl = baseUrl;
    updateInitTemplateData(templateData);

    mUrl = baseUrl;
  }

  public void loadFromBundle(TemplateBundle bundle, TemplateData templateData, String baseUrl) {
    LLog.i(TAG, "loadFromBundle:" + baseUrl);
    mInitWithBinary = false;
    mInitWithUrl = false;
    mInitWithBundle = true;
    mInitBundleData.mTemplateBundle = bundle;
    mInitBundleData.mBaseUrl = baseUrl;
    updateInitTemplateData(templateData);

    mUrl = baseUrl;
  }

  public void reload(boolean ignoreCache) {
    reload(ignoreCache, null);
  }

  public void reload(
      boolean ignoreCache, String templateBin, boolean loadFromFragments, int templateSize) {
    if (!TextUtils.isEmpty(templateBin)) {
      LLog.i(TAG, "reload with single template binary transferred by usb");
      byte[] templateBytes = null;
      try {
        templateBytes = Base64.decode(templateBin, Base64.DEFAULT);
      } catch (IllegalArgumentException e) {
        LLog.e(TAG, "Template base64 decode failed:" + e.getMessage());
      }
      reload(ignoreCache, templateBytes);
      return;
    }

    // if true, reload after template fragments transfer completed
    if (loadFromFragments && templateSize > 0) {
      LLog.i(TAG, "reload with template fragments transferred by usb");
      mFragmentsBuffer = ByteBuffer.allocate(templateSize);
      mIgnoreCache = ignoreCache;
      return;
    }

    reload(ignoreCache, null);
  }

  private void reload(boolean ignoreCache, byte[] templateBin) {
    if (ignoreCache) {
      clearCache();
    }
    LynxTemplateRender templateRender = mTemplateRender.get();
    if (templateRender == null) {
      return;
    }

    String url = null;
    TemplateBundle templateBundle = null;
    if (mInitWithBinary) {
      if (TextUtils.isEmpty(mUrl) || !mUrl.startsWith("http")) {
        if (templateBin == null) {
          templateBin = mInitBinaryData.mTemplate;
          LLog.w(TAG,
              "Reloading lynx view with the old template binary data, the code changes may not take effect.");
        }
      }
      url = mUrl;
    } else if (mInitWithBundle) {
      templateBundle = mInitBundleData.mTemplateBundle;
      url = mUrl;
    } else {
      String currentUrl = mInitUrlData.mInitUrl;
      if (currentUrl != null) {
        long time = System.currentTimeMillis();
        int index = currentUrl.lastIndexOf("?");
        if (index != -1) {
          currentUrl = currentUrl.substring(0, index + 1);
        } else {
          currentUrl += "?";
        }
        currentUrl += String.valueOf(time);
      }
      url = currentUrl;
    }

    if (templateBin != null) {
      templateRender.renderTemplateWithBaseUrl(templateBin, mInitTemplateData, url);
    } else if (templateBundle != null) {
      templateRender.renderTemplateBundle(templateBundle, mInitTemplateData, url);
    } else if (url != null) {
      templateRender.renderTemplateUrl(url, mInitTemplateData);
    } else {
      LLog.w(TAG, "Failed to reload, the lynx view may not have been loaded before.");
    }
  }

  public void onReceiveTemplateFragment(String fragment, boolean isLastFragment) {
    LLog.i(TAG, "on receive template fragment");
    try {
      if (!TextUtils.isEmpty(fragment) && mFragmentsBuffer != null) {
        byte[] decodedFragment = Base64.decode(fragment, Base64.DEFAULT);
        if (decodedFragment != null && decodedFragment.length > 0
            && mFragmentsBuffer.remaining() >= decodedFragment.length) {
          mFragmentsBuffer.put(decodedFragment);
        }
      }
    } catch (Exception e) {
      LLog.e(TAG, "Template fragments base64 decode failed:" + e.getMessage());
    }

    if (isLastFragment) {
      LLog.i(TAG, "end of template fragments");
      if (mFragmentsBuffer != null && mFragmentsBuffer.position() > 0) {
        reload(mIgnoreCache, mFragmentsBuffer.array());
      } else {
        reload(mIgnoreCache, null);
      }
      mFragmentsBuffer = null;
    }
  }

  public void navigate(String url) {
    mInitWithBinary = false;
    mInitWithBundle = false;
    mInitWithUrl = true;
    mInitUrlData.mInitUrl = url;
    mInitTemplateData = TemplateData.fromString("");

    LynxTemplateRender templateRender = mTemplateRender.get();
    if (templateRender != null) {
      templateRender.renderTemplateUrl(mInitUrlData.mInitUrl, mInitTemplateData);
    }
  }

  public String getURL() {
    return mUrl;
  }

  /**
   * clear cache where refresh page
   * currently, clear layout text cache
   */
  public void clearCache() {
    TextRendererCache.cache().clearCache();
  }

  public long getTemplateDataPtr() {
    if (mInitTemplateData != null) {
      return mInitTemplateData.getNativePtr();
    }
    return 0;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public String getTemplateJsInfo(int offset, int size) {
    byte[] template = null;
    if (mInitWithBinary) {
      template = mInitBinaryData.mTemplate;
    } else if (mInitWithUrl) {
      template = mInitUrlData.mTemplate;
    }
    if (template != null && offset < template.length) {
      int length = template.length;
      size = (offset + size) > length ? length - offset : size;
      return Base64.encodeToString(template, offset, size, Base64.DEFAULT);
    }
    return "";
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public void onTemplateLoadSuccess(byte[] template) {
    mInitUrlData.mTemplate = template;
  }
}
