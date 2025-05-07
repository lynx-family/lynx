// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.logbox;

import android.content.Context;
import android.text.TextUtils;
import android.webkit.JavascriptInterface;
import android.widget.Toast;
import com.lynx.devtoolwrapper.LogBoxLogLevel;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;
import java.util.List;
import org.json.JSONException;
import org.json.JSONObject;

class LogBoxDialog extends LogBoxDialogBase {
  private static final String TAG = "LogBoxDialog";
  // json key in message of call back
  private static final String KEY_CALLBACK_ID = "callbackId";
  private static final String KEY_LOG = "log";
  private static final String KEY_NAMESPACE = "namespace";
  private static final String KEY_DATA = "data";
  private static final String KEY_VIEW_NUMBER = "viewNumber";
  private static final String KEY_EVENT = "event";
  private static final String KEY_CURRENT_VIEW = "currentView";
  private static final String KEY_VIEWS_COUNT = "viewsCount";
  private static final String KEY_LEVEL = "level";
  private static final String KEY_TEMPLATE_URL = "templateUrl";
  // event type
  private static final String EVENT_NEW_LOG = "receiveNewLog";
  private static final String EVENT_VIEW_INFO = "receiveViewInfo";
  private static final String EVENT_RESET = "reset";

  private LogBoxLogLevel mLevel;
  private WeakReference<LynxLogBoxManager> mManager;
  private Runnable mLoadingFinishCallback;
  public Boolean isLoadingFinished = false;

  protected LogBoxDialog(
      Context context, LynxLogBoxManager manager, final Runnable loadingFinishCallback) {
    super(context);
    mManager = new WeakReference<>(manager);
    mLoadingFinishCallback = loadingFinishCallback;
    initWebView(new LogBoxCallback());
  }

  public void updateViewInfo(
      int currentIndex, int viewCount, LogBoxLogLevel level, String templateUrl) {
    JSONObject event = new JSONObject();
    JSONObject data = new JSONObject();
    try {
      event.put(KEY_EVENT, EVENT_VIEW_INFO);
      data.put(KEY_CURRENT_VIEW, currentIndex);
      data.put(KEY_VIEWS_COUNT, viewCount);
      data.put(KEY_LEVEL, level.value);
      data.put(KEY_TEMPLATE_URL, templateUrl);
      event.put(KEY_DATA, data);
      sendEvent(event);
    } catch (JSONException e) {
      LLog.e(TAG, e.getMessage());
    }
  }

  protected void showLogMessages(String namespace, List<String> logs) {
    if (logs == null) {
      return;
    }
    for (String log : logs) {
      showLogMessage(namespace, log);
    }
  }

  protected void showLogMessage(String namespace, String log) {
    JSONObject event = new JSONObject();
    JSONObject data = new JSONObject();
    try {
      data.put(KEY_LOG, log);
      data.put(KEY_NAMESPACE, namespace);
      event.put(KEY_EVENT, EVENT_NEW_LOG);
      event.put(KEY_DATA, data);
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
    private static final String CASE_DISMISS = "dismiss";
    private static final String CASE_REMOVE_CURRENT_LOGS = "deleteLynxview";
    private static final String CASE_SWITCH_LOGS = "changeView";
    private static final String CASE_TOAST = "toast";
    private static final String CASE_QUERY_RESOURCE = "queryResource";
    private static final String CASE_LOAD_ERROR_PARSER = "loadErrorParser";

    @JavascriptInterface
    public void postMessage(String strParams) {
      try {
        JSONObject params = new JSONObject(strParams);
        JSONObject data = new JSONObject((params.getString(KEY_DATA)));
        switch (params.getString(BRIDGE_NAME)) {
          case CASE_GET_EXCEPTION_STACK:
            loadMappingsWasm();
            UIThreadUtils.runOnUiThread(mLoadingFinishCallback);
            break;
          case CASE_DISMISS:
            dismiss();
            break;
          case CASE_REMOVE_CURRENT_LOGS:
            // remove logs of current view that showed on logbox
            // and remove current view from view list of logbox
            removeLogsOfCurrentView();
            // request logs of new current view
            requestLogsOfCurrentView();
            break;
          case CASE_SWITCH_LOGS:
            int nextViewIndex = data.getInt(KEY_VIEW_NUMBER);
            requestLogsOfViewIndex(nextViewIndex);
            break;
          case CASE_TOAST:
            String toastMsg = data.getString("message");
            if (!TextUtils.isEmpty(toastMsg)) {
              UIThreadUtils.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                  Toast.makeText(getContext(), toastMsg, Toast.LENGTH_SHORT).show();
                }
              });
            }
            break;
          case CASE_QUERY_RESOURCE:
            String name = data.getString("name");
            getResource(params.getInt(KEY_CALLBACK_ID), name);
            break;
          case CASE_LOAD_ERROR_PARSER:
            String namespace = data.getString("namespace");
            loadErrorParser(params.getInt(KEY_CALLBACK_ID), namespace);
            break;
          default:
            break;
        }
      } catch (Exception e) {
        LLog.e(TAG, e.getMessage());
      }
    }

    private void loadErrorParser(int callbackId, String namespace) {
      DevToolLogBoxEnv.inst().loadErrorParser(getContext(), namespace, new DevToolLogBoxCallback() {
        @Override
        public void onSuccess(String data) {
          evaluateJs(data);
          sendResult(callbackId, true);
        }

        @Override
        public void onFailure(String reason) {
          sendResult(callbackId, false);
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
  }
}
