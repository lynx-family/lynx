// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import android.text.TextUtils;
import androidx.annotation.NonNull;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

// LynxError instance is not thread safe,
// should not use it in multi thread
public class LynxError extends LynxErrorCodeLegacy {
  private static final String TAG = "LynxError";

  // The prefix indicates that the field should be placed in the 'context' field.
  final static String LYNX_ERROR_KEY_PREFIX_CONTEXT = "lynx_context_";
  // The field 'context' is mainly used to store custom infos that are of interest
  // to the user, while other custom infos are mainly used to assist client-side
  // debugging. LogBox will determine whether to display the information based on
  // whether a custom info is present in field 'context'.
  final static String LYNX_ERROR_KEY_CONTEXT = "context";

  // Suggestion for errors with a complex cause that require a detailed explanation
  // on the official site.
  public final static String LYNX_ERROR_SUGGESTION_REF_OFFICIAL_SITE =
      "Please refer to the solution in Doc 'LynxError FAQ' on the official website.";

  // Some commonly used keys of LynxError's customInfo
  public final static String LYNX_ERROR_KEY_RESOURCE_TYPE = "type";
  public final static String LYNX_ERROR_KEY_RESOURCE_URL = "src";
  public final static String LYNX_ERROR_KEY_IMAGE_CATEGORIZED_CODE = "image_categorized_code";

  public final static String LYNX_THROWABLE = "throwable";
  public static final int JAVA_ERROR = -3;
  public static final int JS_ERROR = -2;
  public static final int NATIVE_ERROR = -1;

  public static final String LEVEL_ERROR = "error";
  public static final String LEVEL_WARN = "warn";

  // Indicates whether the error only needs to be displayed
  // using LogBox and does not require reporting.
  private boolean mIsLogBoxOnly = false;

  private boolean mIsNewErrorCode = false;

  private int mSubCode;
  @Deprecated private String msg = "";
  @Deprecated private JSONObject errorObj;

  // be used to record error info that
  // provided by custom components.
  // compatible with old interface
  private JSONObject mUserDefineInfo;

  // indicates the source of the error, can take
  // value of JAVA_ERROR, NATIVE_ERROR
  private int mErrorType;

  // be used to store json string cache, avoid
  // regenerate json string for same error content.
  private String mJsonStringCache;

  // Error fields to organize structured error,
  // recommend use following fields to construct
  // lynx error over msg and errorObj.

  /** Required fields */
  // a summary message of the error
  private String mSummaryMessage;
  // url of the template that reported the error
  private String mTemplateUrl;
  // error level, can take value LEVEL_ERROR or LEVEL_WARN
  private String mLevel;
  // version of the card that reported the error
  private String mCardVersion;
  private List<LynxSubErrorCode.Consumer> mConsumers;

  /** Optional fields */
  // fix suggestion for the error
  private String mFixSuggestion;
  // the call stack when the error occurred
  private String mCallStack;
  // the origin cause of the error, usually comes from outside
  private String mRootCause;

  /** Custom fields */
  // some specific info of the error
  private Map<String, Object> mCustomInfo;

  @Deprecated
  public LynxError(String msg, int code) {
    this.mSubCode = code;
    this.msg = msg;
  }

  // constructor for new error code
  public LynxError(int code, String msg) {
    this(code, msg, "", LynxSubErrorCode.Level.ERROR.value);
  }

  public LynxError(int errorCode, String errorMessage, String fixSuggestion, String level) {
    this(errorCode, errorMessage, fixSuggestion, level, JAVA_ERROR);
  }

  public LynxError(
      int errorCode, String errorMessage, String fixSuggestion, String level, int errorType) {
    this(errorCode, errorMessage, fixSuggestion, level, errorType, null);
  }

  LynxError(int errorCode, String errorMessage, String fixSuggestion, String level, int errorType,
      Map<String, Object> customInfo) {
    this(errorCode, errorMessage, fixSuggestion, level, errorType, customInfo, false);
  }

  LynxError(int errorCode, String errorMessage, String fixSuggestion, String level, int errorType,
      Map<String, Object> customInfo, boolean isLogBoxOnly) {
    mSubCode = errorCode;
    mSummaryMessage = errorMessage;
    mErrorType = errorType;
    mCustomInfo = customInfo;
    mIsLogBoxOnly = isLogBoxOnly;
    LynxSubErrorCode.MetaData metaData = LynxSubErrorCode.getMetaData(errorCode);
    if (metaData != null) {
      mLevel = metaData.mLevel != LynxSubErrorCode.Level.UNDECIDED ? metaData.mLevel.value : level;
      mFixSuggestion = metaData.mFixSuggestion.isEmpty() ? fixSuggestion : metaData.mFixSuggestion;
      mConsumers = metaData.mConsumer;
      mIsNewErrorCode = true;
    } else {
      mLevel = level;
      mFixSuggestion = fixSuggestion;
      mConsumers = List.of();
    }
  }

  // if the method is called by LynxError constructed by deprecated
  // constructors, the method will not affect the generating of json string
  public void addCustomInfo(String key, String value) {
    if (TextUtils.isEmpty(key) || TextUtils.isEmpty(value)) {
      return;
    }
    mJsonStringCache = null;
    if (mCustomInfo == null) {
      mCustomInfo = new HashMap<>();
    }
    mCustomInfo.put(key, value);
  }

  public void setCustomInfo(Map<String, Object> customInfo) {
    mJsonStringCache = null;
    mCustomInfo = customInfo;
  }

  public boolean containsCustomField(String key) {
    if (mCustomInfo == null || TextUtils.isEmpty(key)) {
      return false;
    }
    return mCustomInfo.containsKey(key);
  }

  public void setTemplateUrl(String url) {
    mJsonStringCache = null;
    mTemplateUrl = url;
  }

  public void setCardVersion(String version) {
    mJsonStringCache = null;
    mCardVersion = version;
  }

  public void setCallStack(String stack) {
    mJsonStringCache = null;
    mCallStack = stack;
  }

  public void setRootCause(String rootCause) {
    mJsonStringCache = null;
    mRootCause = rootCause;
  }

  public String getRootCause() {
    return mRootCause;
  }

  public void setUserDefineInfo(JSONObject object) {
    mJsonStringCache = null;
    mUserDefineInfo = object;
  }

  public String getSummaryMessage() {
    return mSummaryMessage == null ? "" : mSummaryMessage;
  }

  public String getLevel() {
    return mLevel;
  }

  public int getErrorCode() {
    if (mIsNewErrorCode) {
      return mSubCode / 100;
    }
    return mSubCode;
  }

  public int getSubCode() {
    return mSubCode;
  }

  public String getFixSuggestion() {
    return mFixSuggestion == null ? "" : mFixSuggestion;
  }

  public int getType() {
    return mErrorType;
  }

  public Map<String, String> getContextInfo() {
    Map<String, String> res = new HashMap<>();
    if (mCustomInfo == null) {
      return res;
    }
    for (Map.Entry<String, Object> entry : mCustomInfo.entrySet()) {
      String key = entry.getKey();
      if (key.startsWith(LYNX_ERROR_KEY_PREFIX_CONTEXT)) {
        res.put(key.substring(LYNX_ERROR_KEY_PREFIX_CONTEXT.length()), entry.getValue().toString());
      } else if (key.equals(LYNX_ERROR_KEY_RESOURCE_URL)) {
        res.put(key, entry.getValue().toString());
      }
    }
    return res;
  }

  public String getMsg() {
    if (!TextUtils.isEmpty(mJsonStringCache)) {
      return mJsonStringCache;
    }
    if (!TextUtils.isEmpty(mSummaryMessage)) {
      mJsonStringCache = generateJsonString();
    } else if (this.errorObj != null) {
      mJsonStringCache = this.errorObj.toString();
    } else if (this.msg != null) {
      mJsonStringCache = this.msg;
    }
    return mJsonStringCache == null ? "" : mJsonStringCache;
  }

  @Deprecated
  public JSONObject getErrorObj() {
    return this.errorObj;
  }

  static void putStringToJsonObject(JSONObject object, String key, String value)
      throws JSONException {
    if (object != null && !TextUtils.isEmpty(value) && !TextUtils.isEmpty(key)) {
      object.put(key, value);
    }
  }

  static void putMapValueToJsonObject(JSONObject object, Map<String, Object> map)
      throws JSONException {
    if (map == null || map.isEmpty() || object == null) {
      return;
    }
    JSONObject contextObject = new JSONObject();
    for (Map.Entry<String, Object> entry : map.entrySet()) {
      if (entry.getKey().startsWith(LYNX_ERROR_KEY_PREFIX_CONTEXT)) {
        contextObject.put(entry.getKey(), entry.getValue());
      } else {
        object.put(entry.getKey(), entry.getValue());
      }
    }
    if (contextObject.length() > 0) {
      object.put(LYNX_ERROR_KEY_CONTEXT, contextObject);
    }
  }

  // covert fields to json string
  private String generateJsonString() {
    String jsonStr = null;
    JSONObject json = new JSONObject();
    try {
      // put required fields
      json.put("error_code", getErrorCode());
      json.put("sub_code", mSubCode);
      putStringToJsonObject(json, "url", mTemplateUrl);
      putStringToJsonObject(json, "error", mSummaryMessage);
      putStringToJsonObject(json, "card_version", mCardVersion);
      putStringToJsonObject(json, "sdk", LynxEnv.inst().getLynxVersion());
      putStringToJsonObject(json, "level", mLevel);
      JSONArray jConsumers = new JSONArray(mConsumers);
      json.put("consumers", jConsumers);
      // put optional fields
      putStringToJsonObject(json, "fix_suggestion", mFixSuggestion);
      putStringToJsonObject(json, "error_stack", mCallStack);
      putStringToJsonObject(json, "root_cause", mRootCause);
      // put custom fields
      putMapValueToJsonObject(json, mCustomInfo);
      if (mUserDefineInfo != null && mUserDefineInfo.length() > 0) {
        json.put("user_define_info", mUserDefineInfo);
      }
      jsonStr = json.toString();
    } catch (JSONException e) {
      LLog.e(TAG, e.getMessage());
    }
    return jsonStr == null ? "" : jsonStr;
  }

  public boolean isValid() {
    if (!TextUtils.isEmpty(this.msg) || this.errorObj != null
        || !TextUtils.isEmpty(mSummaryMessage)) {
      return true;
    }
    return false;
  }

  public boolean isFatal() {
    return mLevel == LynxSubErrorCode.Level.FATAL.value;
  }

  public boolean isJSError() {
    int code = getErrorCode();
    return code >= 200 && code < 300;
  }

  public boolean isLepusError() {
    int code = getErrorCode();
    return code >= 1100 && code < 1200;
  }

  // Set whether the error only needs to be displayed
  // using LogBox and does not require reporting.
  public void setLogBoxOnly(boolean value) {
    mIsLogBoxOnly = value;
  }

  public boolean isLogBoxOnly() {
    return mIsLogBoxOnly;
  }

  @NonNull
  @Override
  public String toString() {
    return "{\"code\": " + getErrorCode() + ",\"msg\":" + getMsg() + "}";
  }

  @CalledByNative
  private static LynxError createLynxError(int errorCode, String errorMessage, String fixSuggestion,
      String level, JavaOnlyMap customInfo, boolean isLogBoxOnly) {
    return new LynxError(
        errorCode, errorMessage, fixSuggestion, level, NATIVE_ERROR, customInfo, isLogBoxOnly);
  }
}
