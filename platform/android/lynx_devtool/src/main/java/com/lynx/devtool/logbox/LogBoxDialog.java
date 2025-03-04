// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.logbox;

import android.content.Context;
import android.text.TextUtils;
import android.webkit.JavascriptInterface;
import android.widget.Toast;
import androidx.annotation.NonNull;
import com.lynx.devtoolwrapper.LogBoxLogLevel;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.eventreport.LynxEventReporter;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.json.JSONException;
import org.json.JSONObject;

class LogBoxDialog extends LogBoxDialogBase {
  private static final String TAG = "LogBoxDialog";
  private static final String QUERY = "?downloadapi=true&supportyellowbox=true";
  // json key in message of call back
  private static final String KEY_CALLBACK_ID = "callbackId";
  private static final String KEY_DATA = "data";
  private static final String KEY_NAME = "name";
  private static final String KEY_URL = "url";
  private static final String KEY_VIEW_NUMBER = "viewNumber";
  private static final String KEY_EVENT = "event";
  private static final String KEY_CURRENT_VIEW = "currentView";
  private static final String KEY_VIEWS_COUNT = "viewsCount";
  private static final String KEY_TYPE = "type";
  private static final String KEY_TEMPLATE_URL = "templateUrl";
  // log box type
  private static final String TYPE_REDBOX = "redbox";
  private static final String TYPE_YELLOWBOX = "yellowbox";
  // event type
  private static final String EVENT_NEW_INFO = "receiveNewLog";
  private static final String EVENT_NEW_WARNING = "receiveNewWarning";
  private static final String EVENT_NEW_ERROR = "receiveNewError";
  private static final String EVENT_VIEW_INFO = "receiveViewInfo";
  private static final String EVENT_RESET = "reset";

  // error types
  private static final int UNKNOWN_ERROR = -1;
  private static final int JS_ERROR = 1;
  private static final int LEPUS_ERROR = 2;
  private static final int LEPUS_NG_ERROR = 3;
  private static final int NATIVE_ERROR = 4;

  private LogBoxLogLevel mLevel;
  private WeakReference<LynxLogBoxManager> mManager;
  private Runnable mLoadingFinishCallback;
  public Boolean isLoadingFinished = false;

  protected LogBoxDialog(
      Context context, LynxLogBoxManager manager, final Runnable loadingFinishCallback) {
    super(context);
    mManager = new WeakReference<>(manager);
    mLoadingFinishCallback = loadingFinishCallback;
    initWebView(QUERY, new LogBoxCallback());
  }

  public void updateViewInfo(
      int currentIndex, int viewCount, LogBoxLogLevel level, String templateUrl) {
    JSONObject event = new JSONObject();
    JSONObject data = new JSONObject();
    String level_str = level == LogBoxLogLevel.Error ? TYPE_REDBOX : TYPE_YELLOWBOX;
    try {
      event.put(KEY_EVENT, EVENT_VIEW_INFO);
      data.put(KEY_CURRENT_VIEW, currentIndex);
      data.put(KEY_VIEWS_COUNT, viewCount);
      data.put(KEY_TYPE, level_str);
      data.put(KEY_TEMPLATE_URL, templateUrl);
      event.put(KEY_DATA, data);
      sendEvent(event);
    } catch (JSONException e) {
      LLog.e(TAG, e.getMessage());
    }
  }

  public void showLogMessages(LogBoxLogLevel level, List<String> logs) {
    if (logs == null) {
      return;
    }
    for (String log : logs) {
      showLogMessage(level, log);
    }
  }

  public void showLogMessage(LogBoxLogLevel level, String log) {
    String eventValue = "";
    if (level == LogBoxLogLevel.Info) {
      eventValue = EVENT_NEW_INFO;
    } else if (level == LogBoxLogLevel.Warn) {
      eventValue = EVENT_NEW_WARNING;
    } else if (level == LogBoxLogLevel.Error) {
      eventValue = EVENT_NEW_ERROR;
    }
    JSONObject event = new JSONObject();
    try {
      event.put(KEY_EVENT, eventValue);
      event.put(KEY_DATA, log);
      sendEvent(event);
    } catch (JSONException e) {
      LLog.e(TAG, e.getMessage());
    }
  }

  public LogBoxLogLevel getLevel() {
    return mLevel;
  }

  public void showWithLevel(LogBoxLogLevel level) {
    mLevel = level;
    show();
  }

  public void onLoadingFinished() {
    isLoadingFinished = true;
  }

  @Override
  public boolean isLoadingFinished() {
    return isLoadingFinished;
  }

  @Override
  public boolean isShowing() {
    return isLoadingFinished && super.isShowing();
  }

  @Override
  public void reset() {
    LynxLogBoxManager manager = mManager.get();
    if (manager != null) {
      manager.onLogBoxDismiss();
    }
    sendEvent(EVENT_RESET);
  }

  @Override
  protected void finalize() throws Throwable {
    super.finalize();
    destroyWebView();
  }

  private class LogBoxCallback extends LogBoxDialogBase.Callback {
    private static final String BRIDGE_NAME = "bridgeName";
    // call back type
    private static final String CASE_GET_EXCEPTION_STACK = "getExceptionStack";
    private static final String CASE_GET_CORE_JS = "getCoreJs";
    private static final String CASE_GET_TEMPLATE_JS = "getTemplateJs";
    private static final String CASE_RELOAD = "reload";
    private static final String CASE_DISMISS = "dismiss";
    private static final String CASE_DOWNLOAD = "download";
    private static final String CASE_REMOVE_CURRENT_LOGS = "deleteLynxview";
    private static final String CASE_SWITCH_LOGS = "changeView";
    private static final String CASE_TOAST = "toast";
    private static final String CASE_REPORT_EVENT = "reportEvent";
    private static final String EVENT_REPORT_FEEDBACK = "lynxsdk_redbox_feedback";
    private static final String CASE_QUERY_RESOURCE = "queryResource";

    @JavascriptInterface
    public void postMessage(String strParams) {
      try {
        JSONObject params = new JSONObject(strParams);
        switch (params.getString(BRIDGE_NAME)) {
          case CASE_GET_EXCEPTION_STACK:
            loadMappingsWasm();
            UIThreadUtils.runOnUiThread(mLoadingFinishCallback);
            sendResult(params.getInt(KEY_CALLBACK_ID), new ArrayList<String>());
            break;
          case CASE_GET_CORE_JS:
            sendResult(params.getInt(KEY_CALLBACK_ID), getCoreJs());
            break;
          case CASE_GET_TEMPLATE_JS:
            JSONObject data = new JSONObject(params.getString(KEY_DATA));
            sendResult(params.getInt(KEY_CALLBACK_ID), getTemplateJs(data.getString(KEY_NAME)));
            break;
          case CASE_RELOAD:
            reload();
            break;
          case CASE_DISMISS:
            dismiss();
            break;
          case CASE_DOWNLOAD:
            JSONObject dataWithUrl = new JSONObject(params.getString(KEY_DATA));
            String url = dataWithUrl.getString(KEY_URL);
            download(params.getInt(KEY_CALLBACK_ID), url);
            break;
          case CASE_REMOVE_CURRENT_LOGS:
            // remove logs of current view that showed on logbox
            // and remove current view from view list of logbox
            removeLogsOfCurrentView();
            // request logs of new current view
            requestLogsOfCurrentView();
            break;
          case CASE_SWITCH_LOGS:
            JSONObject changeViewData = new JSONObject((params.getString(KEY_DATA)));
            int nextViewIndex = changeViewData.getInt(KEY_VIEW_NUMBER);
            requestLogsOfViewIndex(nextViewIndex);
            break;
          case CASE_TOAST:
            JSONObject toastData = new JSONObject((params.getString(KEY_DATA)));
            String toastMsg = toastData.getString("message");
            if (!TextUtils.isEmpty(toastMsg)) {
              UIThreadUtils.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                  Toast.makeText(getContext(), toastMsg, Toast.LENGTH_SHORT).show();
                }
              });
            }
            break;
          case CASE_REPORT_EVENT:
            JSONObject eventData = new JSONObject((params.getString(KEY_DATA)));
            handleReporting(eventData);
            break;
          case CASE_QUERY_RESOURCE:
            JSONObject resData = new JSONObject((params.getString(KEY_DATA)));
            String name = resData.getString("name");
            getResource(params.getInt(KEY_CALLBACK_ID), name);
            break;
          default:
            break;
        }
      } catch (Exception e) {
        LLog.e(TAG, e.getMessage());
      }
    }

    private void reload() {
      UIThreadUtils.runOnUiThread(new Runnable() {
        @Override
        public void run() {
          LynxLogBoxManager manager = mManager.get();
          if (manager == null) {
            return;
          }
          manager.reloadCurrentView(mLevel);
        }
      });
    }

    private void dismiss() {
      UIThreadUtils.runOnUiThread(new Runnable() {
        @Override
        public void run() {
          LynxLogBoxManager manager = mManager.get();
          if (manager == null) {
            return;
          }
          manager.dismissDialog();
        }
      });
    }

    private void removeLogsOfCurrentView() {
      UIThreadUtils.runOnUiThread(new Runnable() {
        @Override
        public void run() {
          LynxLogBoxManager manager = mManager.get();
          if (manager == null) {
            return;
          }
          manager.removeLogsOfCurrentView(mLevel);
        }
      });
    }

    private void requestLogsOfCurrentView() {
      UIThreadUtils.runOnUiThread(new Runnable() {
        @Override
        public void run() {
          LynxLogBoxManager manager = mManager.get();
          if (manager == null) {
            return;
          }
          manager.requestLogsOfCurrentView(mLevel);
        }
      });
    }

    private void requestLogsOfViewIndex(final int viewIndex) {
      UIThreadUtils.runOnUiThread(new Runnable() {
        @Override
        public void run() {
          LynxLogBoxManager manager = mManager.get();
          if (manager == null) {
            return;
          }
          manager.requestLogsOfViewIndex(viewIndex, mLevel);
        }
      });
    }

    private void reportEvent(final String event, final LynxEventReporter.PropsBuilder builder) {
      // LynxLogBoxManager should runs on main thread
      UIThreadUtils.runOnUiThread(new Runnable() {
        @Override
        public void run() {
          int instanceId = LynxEventReporter.INSTANCE_ID_UNKNOWN;
          LynxLogBoxManager manager = mManager.get();
          if (manager != null) {
            instanceId = manager.getInstanceIdOfCurrentView(mLevel);
          }
          LynxEventReporter.onEvent(event, instanceId, builder);
        }
      });
    }

    /**
     * Handle events that need to be reported
     * @param eventData event data containing the event name and props
     */
    private void handleReporting(@NonNull JSONObject eventData) {
      String eventName = eventData.optString("eventName");
      JSONObject eventProps = eventData.optJSONObject("eventProps");
      if (TextUtils.isEmpty(eventName) || eventProps == null) {
        LLog.i(TAG, "receive invalid event");
        return;
      }
      if (eventName.equals("feedback")) {
        reportFeedback(eventProps);
      }
    }

    /**
     * Handle feedback reporting
     * @param eventProps
     * {
     *     "error": object,
     *     "sourceMapTypes": string,
     *     "isParseStackSuccess": boolean,
     *     "isSatisfied": boolean
     * }
     */
    private void reportFeedback(@NonNull JSONObject eventProps) {
      JSONObject errorInfo = eventProps.optJSONObject("error");
      if (errorInfo == null) {
        return;
      }
      reportEvent(EVENT_REPORT_FEEDBACK, new LynxEventReporter.PropsBuilder() {
        @Override
        public Map<String, Object> build() {
          int errorType = errorInfo.optInt("errorType", UNKNOWN_ERROR);
          Map<String, Object> props = new HashMap<>();
          props.put("is_satisfied", eventProps.optBoolean("isSatisfied"));
          props.put("source_map_types", eventProps.optString("sourceMapTypes"));
          props.put("is_stack_parse_success", eventProps.optString("isParseStackSuccess"));
          // put data of error info
          props.put("error_message", errorInfo.optString("message"));
          props.put("original_stack", errorInfo.optString("stack"));
          switch (errorType) {
            case NATIVE_ERROR:
              props.put("code", errorInfo.optInt("code"));
              props.put("fix_suggestion", errorInfo.optString("fixSuggestion"));
              props.put("context", errorInfo.optString("context"));
              props.put("native_stack", errorInfo.optString("nativeStack"));
              break;
            case JS_ERROR:
            case LEPUS_ERROR:
            case LEPUS_NG_ERROR:
              props.put("debug_info_url", errorInfo.optString("debugUrl"));
              break;
            default:
              break;
          }
          return props;
        }
      });
    }
  }
}
