// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.modules;

import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import androidx.annotation.Nullable;
import com.lynx.explorer.LynxViewShellActivity;
import com.lynx.jsbridge.LynxContextModule;
import com.lynx.jsbridge.LynxMethod;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.WritableMap;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.TemplateData;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.utils.UIThreadUtils;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/** Page-scoped native test operations exposed as NativeModules.LynxTestModule. */
public class LynxTestModule extends LynxContextModule {
  public static final String NAME = "LynxTestModule";
  private volatile boolean mDestroyed;
  private final List<View> mButtons = new ArrayList<>();

  public LynxTestModule(LynxContext context) {
    super(context);
  }

  public LynxTestModule(LynxContext context, Object param) {
    super(context, param);
  }

  @Nullable
  private LynxView getView() {
    return !mDestroyed && mLynxContext != null ? mLynxContext.getLynxView() : null;
  }

  private interface ViewAction {
    void run(LynxView view);
  }

  private void withView(ViewAction action) {
    UIThreadUtils.runOnUiThreadImmediately(() -> {
      LynxView view = getView();
      if (view != null) {
        action.run(view);
      }
    });
  }

  @LynxMethod
  public void call(String method, ReadableMap data, Callback callback) {
    invoke(data, callback);
  }

  @LynxMethod
  public void invoke(ReadableMap data, Callback callback) {
    // Deliberately simulate work on the calling thread for bridge timing tests.
    try {
      Thread.sleep(100);
    } catch (InterruptedException exception) {
      Thread.currentThread().interrupt();
    }
    if (!mDestroyed && callback != null) {
      JavaOnlyMap result = new JavaOnlyMap();
      result.putString("result", "success");
      callback.invoke(result);
    }
  }

  @LynxMethod
  public WritableMap callSync(String method, ReadableMap data) {
    return invokeSync(data);
  }

  @LynxMethod
  public WritableMap invokeSync(ReadableMap data) {
    JavaOnlyMap result = new JavaOnlyMap();
    result.putString("result", "----lepus value--success");
    return result;
  }

  @LynxMethod
  public void updateData(ReadableMap data) {
    withView(view -> view.updateData(data.asHashMap()));
  }

  @LynxMethod
  public void resetData(ReadableMap data) {
    withView(view -> view.resetData(TemplateData.fromMap(data.asHashMap())));
  }

  @LynxMethod
  public void updateGlobalProps(ReadableMap props) {
    withView(view -> view.updateGlobalProps(props.asHashMap()));
  }

  @LynxMethod
  public void reloadTemplate(ReadableMap data, ReadableMap props) {
    withView(view
        -> view.reloadTemplate(
            TemplateData.fromMap(data.asHashMap()), TemplateData.fromMap(props.asHashMap())));
  }

  @LynxMethod
  public void reload() {
    withView(view -> {
      String url = view.getTemplateUrl();
      if (url == null || url.isEmpty()) {
        return;
      }
      if (url.startsWith("https://") || url.startsWith("http://")) {
        view.renderTemplateUrl(url, new HashMap<>());
        return;
      }
      // Local pages store their resolved asset name as the template URL.
      String path = url;
      if (LynxViewShellActivity.isAssetFilename(path)) {
        path = LynxViewShellActivity.getAssetFilename(path);
      } else if (path.startsWith("assets://")) {
        path = path.substring("assets://".length());
      }
      path = path.split("[?&]", 2)[0];
      byte[] template = LynxViewShellActivity.readFileFromAssets(mLynxContext, path);
      if (template != null) {
        view.renderTemplateWithBaseUrl(template, new HashMap<>(), url);
      } else {
        LLog.e(NAME, "Unable to reload template: " + url);
      }
    });
  }

  @LynxMethod
  public void getPageDataByKey(ReadableArray params, Callback callback) {
    if (callback == null) {
      return;
    }
    withView(view -> {
      String[] keys = new String[params.size()];
      for (int i = 0; i < params.size(); i++) {
        keys[i] = params.getString(i);
      }
      callback.invoke(toPageDataResult(view.getPageDataByKey(keys)));
    });
  }

  static JavaOnlyMap toPageDataResult(@Nullable Map<String, Object> data) {
    // The SDK returns null for empty keys or unavailable page data.
    return data == null ? new JavaOnlyMap() : JavaOnlyMap.from(data);
  }

  @LynxMethod
  public void updateScreenMatrix(ReadableMap matrix) {
    withView(view -> {
      view.updateScreenMetrics(matrix.getInt("width"), matrix.getInt("height"));
      view.requestLayout();
    });
  }

  @LynxMethod
  public void addButton(ReadableMap info) {
    withView(view -> {
      if (!(view.getParent() instanceof ViewGroup)) {
        return;
      }
      FrameLayout container = new FrameLayout(mLynxContext);
      Button button = new Button(mLynxContext);
      int[] count = {info.getInt("count", 0)};
      button.setText(String.valueOf(count[0]));
      button.setTextSize(info.getInt("fontSize"));
      float density = mLynxContext.getResources().getDisplayMetrics().density;
      FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
          ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
      params.leftMargin = (int) (info.getInt("left", 0) * density);
      params.topMargin = (int) (info.getInt("top", 0) * density);
      container.addView(button, params);
      button.setOnClickListener(ignored -> button.setText(String.valueOf(++count[0])));
      ((ViewGroup) view.getParent()).addView(container, 0);
      mButtons.add(container);
    });
  }

  @Override
  public void destroy() {
    mDestroyed = true;
    UIThreadUtils.runOnUiThreadImmediately(() -> {
      for (View button : mButtons) {
        if (button.getParent() instanceof ViewGroup) {
          ((ViewGroup) button.getParent()).removeView(button);
        }
      }
      mButtons.clear();
    });
    super.destroy();
  }
}
