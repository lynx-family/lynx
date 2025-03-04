// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import android.net.Uri;
import android.text.TextUtils;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.behavior.LynxContext;
import java.io.File;
import java.util.HashSet;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * Class hold some info like templateURL, thread strategy, pageConfig and etc.
 * It's used to report some common useful parameter when report event.
 * Mainly converted to JSONObject by toJSONObject() method,
 * and used as the third argument in API below:
 * @see ILynxEventReporterService#onReportEvent(String, JSONObject, JSONObject)
 */
public class LynxGenericInfo {
  private static final String TAG = "LynxGenericInfo";

  // GeneralInfo props name:
  // The last loaded URL in this lynxView, will be updated when lynxView render new template.
  private final static String PROP_NAME_URL = "url";
  // the relative_path would be equivalent to the url to remove applicationExternalCacheDir,
  // applicationFilesDir and LocalDir.
  // It can be more accurate to filter info by relative_path than by url on the tea platform.
  private final static String PROP_NAME_RELATIVE_PATH = "relative_path";
  // The last thread strategy this lynxView is using, will be updated when the lynxView is init.
  private final static String PROP_NAME_THREAD_MODE = "thread_mode";
  // Lynx SDK's Version, set by LynxEnv.
  private final static String PROP_NAME_LYNX_SDK_VERSION = "lynx_sdk_version";
  // lynx_session_id is an unique id for all living LynxView, constructed with these three info:
  // A unique device id, timestamp when first LoadTemplate, lynxViewIdentify.
  // It would be like "$currentTimestamp-$deviceID-$lynxViewIdentify"
  // It's assigned when LoadTemplate, shouldn't be modified anywhere.
  private final static String PROP_NAME_LYNX_SESSION_ID = "lynx_session_id";
  // Some useful info stored in PageConfig, will be updated when onPageConfigDecoded:
  // targetSdkVersion set by FE.
  private final static String PROP_NAME_LYNX_TARGET_SDK_VERSION = "lynx_target_sdk_version";
  // lynx_dsl could be ttml, react, react_nodiff or unkown.
  private final static String PROP_NAME_LYNX_DSL = "lynx_dsl";
  // lepus_type could be lepus or lepusNG.
  private final static String PROP_NAME_LYNX_LEPUS_TYPE = "lynx_lepus_type";
  // template's page version set by FE.
  private final static String PROP_NAME_LYNX_PAGE_VERSION = "lynx_page_version";

  private final static String TRACE_MONITOR_GENERIC_INFO_UPDATE_RELATIVE_URL =
      "LynxGenericInfo.updateRelativeURL";

  // GeneralInfo props value:
  private String mPropValueURL;
  private String mPropValueRelativePath;
  private int mPropValueThreadMode = -1;
  private String mPropValueLynxSdkVersion;
  private String mPropValueTargetSdkVersion;
  private String mPropValueSessionID;
  private String mPropValueLynxDSL;
  private String mPropValueLepusType;
  private String mPropValuePageVersion;

  private static final Set<String> mReservedQueryKeys = new HashSet<String>();

  static {
    mReservedQueryKeys.add("url");
    mReservedQueryKeys.add("surl");
    mReservedQueryKeys.add("channel");
    mReservedQueryKeys.add("bundle");
  }

  public LynxGenericInfo(LynxView lynxView) {
    TraceEvent.beginSection("LynxGenericInfo initialized");
    updateLynxSdkVersion();
    TraceEvent.endSection("LynxGenericInfo initialized");
  }

  public JSONObject toJSONObject() {
    JSONObject ret = new JSONObject();
    try {
      ret.putOpt(PROP_NAME_URL, mPropValueURL);
      ret.putOpt(PROP_NAME_RELATIVE_PATH, mPropValueRelativePath);
      ret.putOpt(PROP_NAME_THREAD_MODE, mPropValueThreadMode);
      ret.putOpt(PROP_NAME_LYNX_SDK_VERSION, mPropValueLynxSdkVersion);
      ret.putOpt(PROP_NAME_LYNX_TARGET_SDK_VERSION, mPropValueTargetSdkVersion);
      ret.putOpt(PROP_NAME_LYNX_SESSION_ID, mPropValueSessionID);
      ret.putOpt(PROP_NAME_LYNX_DSL, mPropValueLynxDSL);
      ret.putOpt(PROP_NAME_LYNX_LEPUS_TYPE, mPropValueLepusType);
      ret.putOpt(PROP_NAME_LYNX_PAGE_VERSION, mPropValuePageVersion);
    } catch (JSONException e) {
      LLog.w(TAG, "LynxGenericInfo toJSONObject failed");
      e.printStackTrace();
    }
    return ret;
  }

  // PageConfig Info
  public void updatePageConfigInfo(PageConfig config) {
    updatePageConfigLepusNG(config);
    updatePageConfigTargetSdkVersion(config);
    updatePageConfigPageVersion(config);
    updatePageConfigPageType(config);
  }

  private final static String PAGE_TYPE_TT = "tt";
  private final static String PAGE_TYPE_TTML = "ttml";
  private void updatePageConfigPageType(PageConfig config) {
    mPropValueLynxDSL = config.getPageType();
    // rename tt to ttml
    if (mPropValueLynxDSL != null && mPropValueLynxDSL.equals(PAGE_TYPE_TT)) {
      mPropValueLynxDSL = PAGE_TYPE_TTML;
    }
  }

  private void updatePageConfigPageVersion(PageConfig config) {
    mPropValuePageVersion = config.getPageVersion();
  }

  private final static String LEPUS_TYPE_LEPUS = "lepus";
  private final static String LEPUS_TYPE_LEPUSNG = "lepusNG";
  private void updatePageConfigLepusNG(PageConfig config) {
    boolean enableLepusNG = config.isEnableLepusNG();
    if (enableLepusNG) {
      mPropValueLepusType = LEPUS_TYPE_LEPUSNG;
    } else {
      mPropValueLepusType = LEPUS_TYPE_LEPUS;
    }
  }

  private void updatePageConfigTargetSdkVersion(PageConfig config) {
    mPropValueTargetSdkVersion = config.getTargetSdkVersion();
  }

  // URL Info
  public void updateLynxUrl(LynxContext lynxContext, String templateURL) {
    if (TextUtils.isEmpty(templateURL)) {
      return;
    }
    if (TextUtils.equals(templateURL, mPropValueURL)) {
      return;
    }
    mPropValueURL = templateURL;
    TraceEvent.beginSection(TRACE_MONITOR_GENERIC_INFO_UPDATE_RELATIVE_URL);
    updateRelativeURL(lynxContext);
    TraceEvent.endSection(TRACE_MONITOR_GENERIC_INFO_UPDATE_RELATIVE_URL);
  }

  public String getPropValueRelativePath() {
    return this.mPropValueRelativePath;
  }

  // get and cache applicationExternalCacheDir
  private static String applicationExternalCacheDir;
  private void getApplicationExternalCacheDir(LynxContext lynxContext) {
    if (applicationExternalCacheDir != null && !applicationExternalCacheDir.isEmpty()) {
      // if applicationExternalCacheDir has been gotten before, just use this value
      return;
    }
    if (lynxContext != null) {
      File externalCacheDir = lynxContext.getExternalCacheDir();
      if (externalCacheDir != null) {
        applicationExternalCacheDir = externalCacheDir.getPath();
      }
    }
  }

  // get and cache applicationFilesDir
  private static String applicationFilesDir;
  private void getApplicationFilesDir(LynxContext lynxContext) {
    if (applicationFilesDir != null && !applicationFilesDir.isEmpty()) {
      // if applicationFilesDir has been gotten before, just use this value
      return;
    }
    if (lynxContext != null) {
      File filesDir = lynxContext.getFilesDir();
      if (filesDir != null) {
        applicationFilesDir = filesDir.getPath();
      }
    }
  }

  /**
   * Remove redundant query parameters from relativePath and keep reserved query parameters
   * @param relativePath relative url
   * @return normalized relative url
   */
  private String removeRedundantQueryParams(String relativePath) {
    if (TextUtils.isEmpty(relativePath)) {
      return relativePath;
    }

    String finalUrl = relativePath;
    // try removing redundant query parmeters
    // e.g. https://lf-ecom-xx/obj/xxx/template.js?enter_from=XXX
    try {
      Uri uri = Uri.parse(relativePath);
      // Remove redundant query parameters to improve clustering accuracy
      if (uri.isHierarchical()) {
        Uri.Builder builder = new Uri.Builder();
        builder.scheme(uri.getScheme())
            .encodedAuthority(uri.getEncodedAuthority())
            .encodedPath(uri.getEncodedPath());
        for (String query : mReservedQueryKeys) {
          String parameter = uri.getQueryParameter(query);
          if (!TextUtils.isEmpty(parameter)) {
            builder.appendQueryParameter(query, parameter);
          }
        }
        finalUrl = builder.toString();
      }
    } catch (NullPointerException | UnsupportedOperationException | IllegalArgumentException
        | IndexOutOfBoundsException e) {
      LLog.w(TAG, "Parsing hierarchical schema failed for url is null with " + e.getMessage());
    }

    return finalUrl;
  }

  private void updateRelativeURL(LynxContext lynxContext) {
    mPropValueRelativePath = mPropValueURL;
    getApplicationExternalCacheDir(lynxContext);
    getApplicationFilesDir(lynxContext);
    // try removing applicationExternalCacheDir and applicationFilesDir
    if (applicationExternalCacheDir != null && !applicationExternalCacheDir.isEmpty()) {
      mPropValueRelativePath = mPropValueRelativePath.replace(applicationExternalCacheDir, "");
    }
    if (applicationFilesDir != null && !applicationFilesDir.isEmpty()) {
      mPropValueRelativePath = mPropValueRelativePath.replace(applicationFilesDir, "");
    }

    mPropValueRelativePath = removeRedundantQueryParams(mPropValueRelativePath);
  }

  // SdkVersion Info
  private void updateLynxSdkVersion() {
    mPropValueLynxSdkVersion = LynxEnv.inst().getLynxVersion();
  }

  // ThreadStrategy Info
  public void updateThreadStrategy(ThreadStrategyForRendering threadStrategyForRendering) {
    mPropValueThreadMode = threadStrategyForRendering.id();
  }
}
