// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtoolwrapper;

import android.content.Context;
import android.util.DisplayMetrics;
import android.view.InputEvent;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import androidx.core.util.Consumer;
import com.lynx.jsbridge.LynxModuleFactory;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.LynxDevToolDelegateImpl;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.LynxTemplateRender;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.TemplateBundle;
import com.lynx.tasm.TemplateData;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.PageReloadHelper;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.base.trace.TraceEventDef;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.provider.LynxResourceCallback;
import com.lynx.tasm.service.ILynxDevToolService;
import com.lynx.tasm.service.LynxServiceCenter;
import java.lang.ref.WeakReference;
import java.lang.reflect.Constructor;
import java.util.Map;
import org.json.JSONObject;

public class LynxDevtool {
  /**
   * DevTool service instance that provides debugging functionality.
   * Before using this service, check the LynxEnv.inst().isLynxDebugEnabled() switch status
   * to ensure that debugging features are enabled.
   */
  private static ILynxDevToolService sDevToolService = null;
  private static final String TAG = "LynxDevtool";
  @Keep private LynxBaseInspectorOwnerNG mOwner = null;
  private ILynxLogBox mLogBox = null;
  private PageReloadHelper mReloader = null;
  private WeakReference<LynxView> mView = null;
  private WeakReference<LynxTemplateRender> mRender = null;

  public LynxDevtool(LynxView view, LynxTemplateRender render, boolean debuggable) {
    init(view, render, debuggable, render.getLynxContext().getContext());
  }

  public LynxDevtool(Context context, boolean debuggable) {
    init(null, null, debuggable, context);
  }

  private void init(LynxView view, LynxTemplateRender render, boolean debuggable, Context context) {
    TraceEvent.beginSection(TraceEventDef.DEVTOOL_INIT);
    try {
      LLog.i(
          TAG, "Initialize LynxDevtool, lynxDebugEnabled:" + LynxEnv.inst().isLynxDebugEnabled());

      mView = new WeakReference<>(view);
      mRender = new WeakReference<>(render);
      if (LynxEnv.inst().isLynxDebugEnabled()) {
        LLog.i(TAG,
            "devtoolEnabled:" + LynxEnv.inst().isDevtoolEnabled() + ", logBoxEnabled:"
                + LynxEnv.inst().isLogBoxEnabled() + ", debuggable:" + debuggable);

        sDevToolService = LynxServiceCenter.inst().getService(ILynxDevToolService.class);

        if ((LynxEnv.inst().isDevtoolEnabled() || debuggable) && sDevToolService != null) {
          mOwner = sDevToolService.createInspectorOwner(view, debuggable);
          if (mOwner != null) {
            LLog.i(TAG, "owner init");
          }
        }

        if (LynxEnv.inst().isLogBoxEnabled() && sDevToolService != null) {
          mLogBox = sDevToolService.createLogBox(this);
          LLog.i(TAG, "LogBox init");
        }

        if (render != null) {
          DisplayMetrics dm = render.getLynxContext().getScreenMetrics();
          updateScreenMetrics(dm.widthPixels, dm.heightPixels, dm.density);
        }
      }

      if (mOwner != null || mLogBox != null) {
        mReloader = new PageReloadHelper(view);
      }

      if (mOwner != null) {
        mOwner.setDevToolDelegate(new LynxDevToolDelegateImpl(render));
      }

    } catch (Exception e) {
      LLog.e(TAG, "failed to init LynxDevtool: " + e.toString());
      mOwner = null;
      mLogBox = null;
      mReloader = null;
    }
    TraceEvent.endSection(TraceEventDef.DEVTOOL_INIT);
  }

  public void attachContext(Context context) {
    if (mLogBox != null) {
      mLogBox.attachContext(context);
    }
  }

  public void destroy() {
    if (mOwner != null) {
      mOwner.destroy();
      mOwner = null;
      LLog.i(TAG, "mOwner = null");
    }
  }

  public void onLoadFromLocalFile(byte[] template, TemplateData templateData, String baseUrl) {
    if (mReloader != null) {
      mReloader.loadFromLocalFile(template, templateData, baseUrl);
    }
    if (mLogBox != null) {
      mLogBox.onLoadTemplate();
    }
  }

  public void onGlobalPropsChanged(TemplateData globalProps) {
    if (mOwner != null) {
      mOwner.onGlobalPropsUpdated(globalProps);
    }
    if (mReloader != null) {
      mReloader.onGlobalPropsUpdated(globalProps);
    }
  }

  public void onTemplateDataChanged(TemplateData templateData) {
    if (mOwner != null) {
      mOwner.onTemplateDataUpdated(templateData);
    }
    if (mReloader != null) {
      mReloader.onTemplateDataUpdated(templateData);
    }
  }

  public void onTemplateDataReset(TemplateData templateData) {
    if (mOwner != null) {
      mOwner.onTemplateDataReset(templateData);
    }
    if (mReloader != null) {
      mReloader.onTemplateDataReset(templateData);
    }
  }

  public void onLoadFromURL(@NonNull final String templateUrl, @NonNull final String postUrl,
      @Nullable final TemplateData templateData, @Nullable final Map<String, Object> map,
      @Nullable final String jsonData) {
    if (mReloader != null) {
      mReloader.saveURL(templateUrl, templateData, map, jsonData);
    }
    if (mLogBox != null) {
      mLogBox.onLoadTemplate();
    }
  }

  public void onLoadFromBundle(
      TemplateBundle templateBundle, TemplateData templateData, String baseUrl) {
    if (mReloader != null) {
      mReloader.loadFromBundle(templateBundle, templateData, baseUrl);
    }
    if (mLogBox != null) {
      mLogBox.onLoadTemplate();
    }
  }

  public void onRootViewInputEvent(InputEvent ev) {
    if (mOwner != null) {
      mOwner.onRootViewInputEvent(ev);
    }
  }

  public void onTemplateAssemblerCreated(long ptr) {
    if (mOwner != null) {
      mOwner.onTemplateAssemblerCreated(ptr);
    }
  }

  public long onBackgroundRuntimeCreated(String groupName) {
    if (mOwner != null) {
      return mOwner.onBackgroundRuntimeCreated(groupName);
    }
    return 0;
  }

  public void onEnterForeground() {
    if (mOwner != null) {
      mOwner.continueCasting();
    }
  }

  public void onEnterBackground() {
    if (mOwner != null) {
      mOwner.pauseCasting();
    }
  }

  public void showErrorMessage(final LynxError error) {
    if (mLogBox != null) {
      mLogBox.showLogMessage(error);
    }
    if (mOwner != null) {
      mOwner.showErrorMessageOnConsole(error);
    }
  }

  public void onRegisterModule(LynxModuleFactory moduleFactory) {
    if (!LynxEnv.inst().isLynxDebugEnabled()) {
      return;
    }

    if (sDevToolService == null) {
      LLog.e(TAG, "sDevToolService is null");
      return;
    }

    Class setModuleClass = sDevToolService.getDevToolSetModuleClass();
    if (setModuleClass != null) {
      moduleFactory.registerModule(setModuleClass.getSimpleName(), setModuleClass, null);
      LLog.i(TAG, "register LynxDevToolSetModule!");
    }

    Class webSocketModuleClass = sDevToolService.getLynxWebSocketModuleClass();
    if (webSocketModuleClass != null) {
      moduleFactory.registerModule(
          webSocketModuleClass.getSimpleName(), webSocketModuleClass, null);
      LLog.i(TAG, "register LynxWebSocketModule!");
    }

    Class lynxTrailModuleClass = sDevToolService.getLynxTrailModule();
    if (lynxTrailModuleClass != null) {
      moduleFactory.registerModule(
          lynxTrailModuleClass.getSimpleName(), lynxTrailModuleClass, null);
      LLog.i(TAG, "register LynxTrailModule!");
    }
  }

  public void attach(LynxView lynxView) {
    mView = new WeakReference<>(lynxView);
    if (mOwner != null) {
      mOwner.attach(lynxView);
    }
  }

  public void attach(@NonNull LynxView lynxView, @NonNull LynxTemplateRender render) {
    mView = new WeakReference<>(lynxView);
    mRender = new WeakReference<>(render);

    if (mOwner != null) {
      mOwner.attach(lynxView);
    }

    if (mReloader != null) {
      mReloader.attach(lynxView);
    }

    if (mOwner != null) {
      mOwner.setDevToolDelegate(new LynxDevToolDelegateImpl(render));
    }

    DisplayMetrics dm = render.getLynxContext().getScreenMetrics();
    updateScreenMetrics(dm.widthPixels, dm.heightPixels, dm.density);
  }

  public LynxBaseInspectorOwner getBaseInspectorOwner() {
    return mOwner;
  }

  public void updateScreenMetrics(int width, int height, float density) {
    if (mOwner != null) {
      mOwner.updateScreenMetrics(width, height, density);
    }
  }

  public void attachToDebugBridge(String url) {
    if (mOwner != null) {
      mOwner.attachToDebugBridge(url);
    }
  }

  public void onPageUpdate() {
    if (mOwner != null) {
      mOwner.onPageUpdate();
    }
  }

  public String getTemplateUrl() {
    LynxTemplateRender render = mRender.get();
    return render == null ? "" : render.getTemplateUrl();
  }

  public Map<String, Object> getAllJsSource() {
    LynxTemplateRender render = mRender.get();
    return render == null ? null : render.getAllJsSource();
  }

  public LynxContext getLynxContext() {
    LynxTemplateRender render = mRender.get();
    return render == null ? null : render.getLynxContext();
  }

  public void attachLynxUIOwner(LynxUIOwner uiOwner) {
    if (mOwner != null) {
      mOwner.attachLynxUIOwnerToAgent(uiOwner);
      mOwner.setReloadHelper(mReloader);
    }
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public void onTemplateLoadSuccess(byte[] template) {
    if (mReloader != null) {
      mReloader.onTemplateLoadSuccess(template);
    }
  }

  public void onPerfMetricsEvent(String eventName, @NonNull JSONObject data) {
    if (mOwner != null) {
      mOwner.onPerfMetricsEvent(eventName, data);
    }
  }

  public String getDebugInfoUrl(String fileName) {
    if (mOwner != null) {
      return mOwner.getDebugInfoUrl(fileName);
    }
    return "";
  }

  public Boolean enableAirStrictMode() {
    LynxTemplateRender render = mRender.get();
    if (render != null) {
      return render.enableAirStrictMode();
    }
    return false;
  }

  public void onReceiveMessageEvent(ReadableMap event) {
    if (mOwner != null) {
      mOwner.onReceiveMessageEvent(event);
    }
  }
}
