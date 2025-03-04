// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import static com.lynx.tasm.eventreport.LynxEventReporter.INSTANCE_ID_UNKNOWN;

import android.content.Context;
import android.graphics.Canvas;
import android.os.Looper;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import androidx.annotation.AnyThread;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import androidx.annotation.UiThread;
import com.lynx.devtoolwrapper.LogBoxLogLevel;
import com.lynx.devtoolwrapper.LynxBaseInspectorOwner;
import com.lynx.devtoolwrapper.LynxDevtool;
import com.lynx.jsbridge.JSModule;
import com.lynx.jsbridge.LynxModuleFactory;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.tasm.ILynxEngine;
import com.lynx.tasm.LynxBackgroundRuntime;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxEnvKey;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.LynxGenericInfo;
import com.lynx.tasm.LynxLoadMeta;
import com.lynx.tasm.LynxLoadMode;
import com.lynx.tasm.LynxUpdateMeta;
import com.lynx.tasm.LynxViewClient;
import com.lynx.tasm.LynxViewClientV2;
import com.lynx.tasm.PlatformCallBack;
import com.lynx.tasm.TemplateBundle;
import com.lynx.tasm.TemplateData;
import com.lynx.tasm.ThreadStrategyForRendering;
import com.lynx.tasm.TimingHandler;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.behavior.ILynxUIRenderer;
import com.lynx.tasm.behavior.ImageInterceptor;
import com.lynx.tasm.behavior.KeyboardEvent;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.herotransition.HeroTransitionManager;
import com.lynx.tasm.behavior.ui.IDrawChildHook;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.UIBody.UIBodyView;
import com.lynx.tasm.behavior.ui.UIGroup;
import com.lynx.tasm.core.VSyncMonitor;
import com.lynx.tasm.eventreport.LynxEventReporter;
import com.lynx.tasm.featurecount.LynxFeatureCounter;
import com.lynx.tasm.performance.TimingCollector;
import com.lynx.tasm.theme.LynxTheme;
import com.lynx.tasm.utils.CallStackUtil;
import com.lynx.tasm.utils.DisplayMetricsHolder;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

@Keep
public class LynxView extends UIBodyView {
  protected LynxTemplateRender mLynxTemplateRender;
  private boolean mCanDispatchTouchEvent;
  protected boolean mDispatchTouchEventToDev = true;
  private final static String VIEW_TAG = "lynxview";
  private final static String TAG = "LynxView";
  private boolean mIsAccessibilityDisabled = false;
  private KeyboardEvent mKeyboardEvent;

  private String mUrl;
  private volatile boolean mHasReportedAccessFromNonUiThread = false;
  private static final Looper sMainLooper = Looper.getMainLooper();

  public LynxView(Context context) {
    super(context);
  }

  public LynxView(Context context, AttributeSet attrs) {
    super(context, attrs);
  }

  @Keep
  public LynxView(Context context, LynxViewBuilder builder) {
    super(context);
    initWithLynxViewBuilder(builder);
  }

  public LynxView(Context context, @NonNull ILynxEngine templateRender) {
    super(context);
    mLynxTemplateRender = (LynxTemplateRender) templateRender;
    initialize(context, null);
  }

  /**
   * method to initialize a LynxView with a prepared LynxViewBuilder.
   *
   * @param builder
   */
  public void initWithLynxViewBuilder(LynxViewBuilder builder) {
    if (builder.lynxBackgroundRuntime != null) {
      initLynxViewWithRuntime(getContext(), builder);
      return;
    }
    builder.threadStrategy =
        builder.lynxUIRenderer().getSupportedThreadStrategy(builder.threadStrategy);

    LLog.i(TAG, "new lynxview detail " + this.toString());
    builder.lynxUIRenderer().onInitLynxView(
        this, getContext(), builder.lynxRuntimeOptions.getLynxGroup());
    initialize(getContext(), builder);
  }

  private void initialize(Context context, LynxViewBuilder builder) {
    setFocusableInTouchMode(true);
    VSyncMonitor.setCurrentWindowManager(
        (WindowManager) context.getSystemService(Context.WINDOW_SERVICE));
    // We try to get Choreographer of ui thread here. It used to be called on LynxEnv init, but in
    // some cases RuntimeException will be thrown either because the timing is too early or file
    // descriptor leaks. So instead we get Choreographer here to avoid RuntimeException. Actually,
    // it will be executed only once because there is a null judgment in it. And there is no
    // thread-safety concern because there will be only one Choreographer instance for ui thread.
    VSyncMonitor.initUIThreadChoreographer();
    initLynxTemplateRender(context, builder);
    mKeyboardEvent = new KeyboardEvent(getLynxContext());
  }

  private void initLynxViewWithRuntime(Context context, LynxViewBuilder builder) {
    LynxBackgroundRuntime runtime = builder.lynxBackgroundRuntime;
    LLog.w(TAG, "init LynxView with runtime, " + runtime);
    if (!runtime.attachToLynxView()) {
      throw new RuntimeException("Build LynxView using an used LynxBackgroundRuntime: " + runtime);
    }

    LynxModuleFactory manager = runtime.getModuleFactory();
    manager.addModuleParamWrapperIfAbsent(builder.lynxRuntimeOptions.getWrappers());

    builder.lynxRuntimeOptions.merge(runtime.getLynxRuntimeOptions());

    initialize(context, builder);
  }

  private void initLynxTemplateRender(Context context, LynxViewBuilder builder) {
    if (mLynxTemplateRender != null) {
      mLynxTemplateRender.attachLynxView(this);
      return;
    }

    // isAccessibilityDisabled() is called when init LynxTemplateRender
    mIsAccessibilityDisabled = builder.lynxUIRenderer().isAccessibilityDisabled();
    mLynxTemplateRender = new LynxTemplateRender(context, this, builder);
  }

  public LynxContext getLynxContext() {
    if (mLynxTemplateRender != null) {
      return mLynxTemplateRender.getLynxContext();
    }
    return null;
  }

  public void reloadAndInit() {
    ILynxUIRenderer lynxUIRenderer = lynxUIRenderer();
    if (lynxUIRenderer != null) {
      lynxUIRenderer.onReloadAndInitUIThreadPart();
    }
    removeAllViews();
  }

  @Override
  public Object getTag() {
    return VIEW_TAG;
  }

  @Override
  @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
  public boolean isAccessibilityDisabled() {
    return mIsAccessibilityDisabled;
  }

  public LynxBaseInspectorOwner getBaseInspectorOwner() {
    if (mLynxTemplateRender != null) {
      LynxDevtool devTool = mLynxTemplateRender.getDevTool();
      if (devTool != null) {
        return devTool.getBaseInspectorOwner();
      }
    }
    return null;
  }

  public void setExtraTiming(TimingHandler.ExtraTimingInfo extraTiming) {
    if (extraTiming == null) {
      return;
    }
    mLynxTemplateRender.setExtraTiming(extraTiming);
  }

  /**
   * Put parameters for reporting events, overriding old values if the parameters already exist.
   * @param params common parameters for report events, these values will be merged with params of
   *     EventReporter.onEvent.
   */
  public void putParamsForReportingEvents(final Map<String, Object> params) {
    if (params == null || mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.putExtraParamsForReportingEvents(params);
  }

  @RestrictTo({RestrictTo.Scope.LIBRARY_GROUP})
  @AnyThread
  @LynxTemplateRender.RenderPhaseName
  public String getRenderPhase() {
    return mLynxTemplateRender.getRenderPhase();
  }

  public HashMap<String, Object> getAllTimingInfo() {
    return mLynxTemplateRender.getAllTimingInfo();
  }
  public void setExtraTiming(Map<String, Long> extraTiming) {
    if (extraTiming == null) {
      return;
    }
    TimingHandler.ExtraTimingInfo info = new TimingHandler.ExtraTimingInfo();
    if (extraTiming.containsKey(TimingHandler.OPEN_TIME)) {
      info.mOpenTime = extraTiming.get(TimingHandler.OPEN_TIME);
    }
    if (extraTiming.containsKey(TimingHandler.CONTAINER_INIT_START)) {
      info.mContainerInitStart = extraTiming.get(TimingHandler.CONTAINER_INIT_START);
    }
    if (extraTiming.containsKey(TimingHandler.CONTAINER_INIT_END)) {
      info.mContainerInitEnd = extraTiming.get(TimingHandler.CONTAINER_INIT_END);
    }
    if (extraTiming.containsKey(TimingHandler.PREPARE_TEMPLATE_START)) {
      info.mPrepareTemplateStart = extraTiming.get(TimingHandler.PREPARE_TEMPLATE_START);
    }
    if (extraTiming.containsKey(TimingHandler.PREPARE_TEMPLATE_END)) {
      info.mPrepareTemplateEnd = extraTiming.get(TimingHandler.PREPARE_TEMPLATE_END);
    }
    mLynxTemplateRender.setExtraTiming(info);
  }

  @Override
  public void bindDrawChildHook(IDrawChildHook hook) {
    if ((lynxUIRenderer() != null) && lynxUIRenderer().disableBindDrawChildHook()) {
      return;
    }

    super.bindDrawChildHook(hook);
  }

  public void onEnterForeground() {
    checkAccessFromNonUiThread("onEnterForeground");

    LLog.i(TAG, "onEnterForeground " + this.toString());

    if (mLynxTemplateRender != null) {
      mLynxTemplateRender.onEnterForeground();
    }

    ILynxUIRenderer lynxUIRenderer = lynxUIRenderer();
    if (lynxUIRenderer != null) {
      lynxUIRenderer.onEnterForeground();
    }
  }

  /**
   * Enter background
   */
  public void onEnterBackground() {
    checkAccessFromNonUiThread("onEnterBackground");

    LLog.i(TAG, "onEnterBackground" + this.toString());

    if (mLynxTemplateRender != null) {
      mLynxTemplateRender.onEnterBackground();
    }

    ILynxUIRenderer lynxUIRenderer = lynxUIRenderer();
    if (lynxUIRenderer != null) {
      lynxUIRenderer.onEnterBackground();
    }
  }

  public void addLynxViewClient(LynxViewClient client) {
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.addLynxViewClient(client);
  }

  public void addLynxViewClientV2(LynxViewClientV2 client) {
    if (mLynxTemplateRender != null) {
      mLynxTemplateRender.addLynxViewClientV2(client);
    }
  }

  /**
   * EXPERIMENTAL API
   * Called on UI thread.
   * Updating the screen size for lynxview.
   * Updating the screen size does not trigger a re-layout.
   * It will be useful for the screen size change, like screen rotation.
   * It can make some css properties based on rpx shows better.
   * Multiple views are not supported with different settings!
   *
   * ATTENTION: In most cases, Should call this function with REAL screen metrics.
   * May use {@link com.lynx.tasm.utils.DisplayMetricsHolder#getRealScreenDisplayMetrics(Context)}
   * to get real screen metrics.
   * @param width(px) screen width
   * @param height(px) screen screen
   */
  public void updateScreenMetrics(int width, int height) {
    checkAccessFromNonUiThread("updateScreenMetrics");
    if (mLynxTemplateRender == null) {
      return;
    }
    DisplayMetricsHolder.updateDisplayMetrics(width, height);
    mLynxTemplateRender.updateScreenMetrics(width, height);
  }

  public void removeLynxViewClient(LynxViewClient client) {
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.removeLynxViewClient(client);
  }

  public void removeLynxViewClientV2(LynxViewClientV2 client) {
    if (mLynxTemplateRender != null) {
      mLynxTemplateRender.removeLynxViewClientV2(client);
    }
  }

  public void setImageInterceptor(ImageInterceptor interceptor) {
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.setImageInterceptor(interceptor);
  }

  public void setAsyncImageInterceptor(ImageInterceptor interceptor) {
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.setAsyncImageInterceptor(interceptor);
  }

  public void pauseRootLayoutAnimation() {
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.pauseRootLayoutAnimation();
  }

  public void resumeRootLayoutAnimation() {
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.resumeRootLayoutAnimation();
  }

  /**
   * Render template with url from remote through template provider.
   *
   * @param templateUrl
   * @param templateData
   */
  public void renderTemplateUrl(@NonNull String templateUrl, final TemplateData templateData) {
    LLog.i(TAG, "renderTemplateUrl " + templateUrl + "with templateData in" + this.toString());
    mUrl = templateUrl;
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.renderTemplateUrl(templateUrl, templateData);
  }

  public void renderTemplateUrl(@NonNull String templateUrl, final String jsonData) {
    LLog.i(TAG, "renderTemplateUrl " + templateUrl + "with jsonData in" + this.toString());
    mUrl = templateUrl;
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.renderTemplateUrl(templateUrl, jsonData);
  }

  public void renderTemplateUrl(@NonNull String templateUrl, final Map<String, Object> data) {
    LLog.i(TAG, "renderTemplateUrl " + templateUrl + "with Map in" + this.toString());
    mUrl = templateUrl;
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.renderTemplateUrl(templateUrl, data);
  }

  public JSModule getJSModule(String module) {
    if (mLynxTemplateRender == null) {
      return null;
    }
    return mLynxTemplateRender.getJSModule(module);
  }

  public void sendGlobalEvent(String name, JavaOnlyArray params) {
    if (LynxEnv.inst().isHighlightTouchEnabled()) {
      showMessageOnConsole(TAG + ": send global event " + name + " for lynx " + hashCode(),
          LogBoxLogLevel.Info.ordinal());
    }
    LLog.i(TAG, "LynxView sendGlobalEvent " + name + " with this: " + this.toString());
    if (mLynxTemplateRender == null) {
      LLog.e(TAG,
          "LynxVew sendGlobalEvent failed since mLynxTemplateRender is null with this: "
              + this.toString());
      return;
    }
    if (enableAirStrictMode()) {
      // In Air mode, send global event by triggerEventBus
      triggerEventBus(name, params);
    } else {
      mLynxTemplateRender.sendGlobalEvent(name, params);
    }
  }

  public void sendGlobalEventToLepus(String name, List<Object> params) {
    if (LynxEnv.inst().isHighlightTouchEnabled()) {
      showMessageOnConsole(TAG + ": send global event " + name + " to lepus for lynx " + hashCode(),
          LogBoxLogLevel.Info.ordinal());
    }
    checkAccessFromNonUiThread("sendGlobalEventToLepus");
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.sendGlobalEventToLepus(name, params);
  }

  /**
   * Render given template using a base scheme url.
   *
   * @param template
   * @param templateData
   * @param baseUrl
   */
  public void renderTemplateWithBaseUrl(
      byte[] template, TemplateData templateData, String baseUrl) {
    LLog.i(TAG, "renderTemplateWithBaseUrl " + baseUrl + "with templateData in " + this.toString());
    mUrl = baseUrl;
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.renderTemplateWithBaseUrl(template, templateData, baseUrl);
  }

  public void renderTemplateWithBaseUrl(byte[] template, Map<String, Object> data, String baseUrl) {
    LLog.i(TAG, "renderTemplateWithBaseUrl " + baseUrl + "with map in " + this.toString());
    mUrl = baseUrl;
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.renderTemplateWithBaseUrl(template, data, baseUrl);
  }

  public void renderTemplateWithBaseUrl(byte[] template, String templateData, String baseUrl) {
    LLog.i(TAG, "renderTemplateWithBaseUrl " + baseUrl + "with string data in" + this.toString());
    mUrl = baseUrl;
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.renderTemplateWithBaseUrl(template, templateData, baseUrl);
  }

  public void renderTemplate(byte[] template, Map<String, Object> initData) {
    LLog.i(TAG, "renderTemplate with init data in " + this.toString());
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.renderTemplate(template, initData);
  }

  public void renderTemplate(byte[] template, TemplateData templateData) {
    LLog.i(TAG, "renderTemplate with templateData in " + this.toString());
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.renderTemplate(template, templateData);
  }

  /**
   * Load LynxView with a pre-decoded template.js
   * Load LynxView with a pre-decoded template.js with {@link TemplateBundle#fromTemplate(byte[])}},
   * it can be used for LynxTemplateBundle reuse.
   *
   * @param bundle The pre-decoded template.js with {@link TemplateBundle#fromTemplate(byte[])}}
   * @param templateData The initData to be used when render first screen.
   */
  public void renderTemplateBundle(
      @NonNull TemplateBundle bundle, TemplateData templateData, String baseUrl) {
    LLog.i(TAG, "renderTemplateBundle with templateData in " + this.toString());
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.renderTemplateBundle(bundle, templateData, baseUrl);
  }

  /**
   * Load LynxView with a wrapped load metaInfo data structure
   * Load metaInfo data structure contains a pre-decoded template.js/lynx initData
   * @param meta LynxLoadMeta data structure
   */
  @AnyThread
  public void loadTemplate(@NonNull LynxLoadMeta meta) {
    if (meta == null) {
      return;
    }
    if (isLayoutRequested() && LynxLoadMode.PRE_PAINTING == meta.getLoadMode()) {
      /**
       * When LynxView is created, isLayoutRequested native flag is set to true by default.
       * Previously, when implementing pre_painting, request layout is blocked
       * until updateData by client is invoked.
       *
       * However, since isLayoutRequested flag is true, immediately after block is removed,
       * LynxView onMeasure/onLayout is invoked, causing text with
       * pre_painting data(cached data) is shown, which is not expected.
       *
       * Trigger measure & layout before loadTemplate with pre_painting mode
       * to consume isLayoutRequested native flags to mitigate this unexpected behavior.
       */
      if (this.getChildCount() > 0) {
        removeAllViewsInLayout();
      }

      ViewGroup.LayoutParams lp = this.getLayoutParams();
      int widthMeasureSpec = MeasureSpec.makeMeasureSpec(
          (lp != null && lp.width >= 0) ? lp.width : 0, MeasureSpec.EXACTLY);
      int heightMeasureSpec = MeasureSpec.makeMeasureSpec(
          (lp != null && lp.height >= 0) ? lp.height : 0, MeasureSpec.EXACTLY);
      this.measure(widthMeasureSpec, heightMeasureSpec);

      this.layout(0, 0, (lp != null && lp.width >= 0) ? lp.width : 0,
          (lp != null && lp.height >= 0) ? lp.height : 0);
    }

    LLog.i(TAG, "loadTemplate with LynxLoadMeta in " + this.toString());
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.loadTemplate(meta);
  }

  /**
   * Render with ssr data.
   * SSR data is a kind of file format which is generated by server runtime of Lynx on server side.
   * The data represented the objects to be rendered on the page.
   * And with ssr data, the first screen will be rendered extremely fast.
   * But the ssr data doesn't contains the logic of page, the page won't be interactable if rendered
   * only with ssr data. To make the lynx view interactable, call ssrHydrate api with the page
   * template to attach the events.
   *
   * @param data The binary ssr data
   * @param ssrUrl The url of ssr data
   * @param injectedData The data to be forwarded to the placeholders placed when rendering on
   *     server.
   */
  public void renderSSR(
      @NonNull byte[] data, @NonNull String ssrUrl, final Map<String, Object> injectedData) {
    LLog.d(TAG, "renderSSR " + ssrUrl);
    if (mLynxTemplateRender == null) {
      return;
    }

    mLynxTemplateRender.renderSSR(data, ssrUrl, injectedData);
  }

  /**
   * Render with ssr data.
   * SSR data is a kind of file format which is generated by server runtime of Lynx on server side.
   * The data represented the objects to be rendered on the page.
   * And with ssr data, the first screen will be rendered extremely fast.
   * But the ssr data doesn't contains the logic of page, the page won't be interactable if rendered
   * only with ssr data. To make the lynx view interactable, call ssrHydrate api with the page
   * template to attach the events.
   *
   * @param data The binary ssr data
   * @param ssrUrl The url of ssr data
   * @param templateData The data to be forwarded to the placeholders placed when rendering on
   *     server.
   */
  public void renderSSR(
      @NonNull byte[] data, @NonNull String ssrUrl, final TemplateData templateData) {
    LLog.d(TAG, "renderSSR " + ssrUrl);
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.renderSSR(data, ssrUrl, templateData);
  }

  /**
   * Render with ssr data.
   * SSR data is a kind of file format which is generated by server runtime of Lynx on server side.
   * The data represented the objects to be rendered on the page.
   * And with ssr data, the first screen will be rendered extremely fast.
   * But the ssr data doesn't contains the logic of page, the page won't be interactable if rendered
   * only with ssr data. To make the lynx view interactable, call ssrHydrate api with the page
   * template to attach the events.
   *
   * @param ssrUrl The url of ssr data
   * @param templateData The data to be forwarded to the placeholders placed when rendering on
   *     server.
   */
  public void renderSSRUrl(@NonNull String ssrUrl, final TemplateData templateData) {
    LLog.d(TAG, "renderSSRUrl " + ssrUrl);
    mUrl = ssrUrl;
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.renderSSRUrl(ssrUrl, templateData);
  }

  /**
   * Render with ssr data.
   * SSR data is a kind of file format which is generated by server runtime of Lynx on server side.
   * The data represented the objects to be rendered on the page.
   * And with ssr data, the first screen will be rendered extremely fast.
   * But the ssr data doesn't contains the logic of page, the page won't be interactable if rendered
   * only with ssr data. To make the lynx view interactable, call ssrHydrate api with the page
   * template to attach the events.
   *
   * @param ssrUrl The url of ssr data
   * @param injectedData The data to be forwarded to the placeholders placed when rendering on
   *     server.
   */
  public void renderSSRUrl(@NonNull String ssrUrl, final Map<String, Object> injectedData) {
    LLog.d(TAG, "renderSSRUrl " + ssrUrl);
    mUrl = ssrUrl;
    if (mLynxTemplateRender == null) {
      return;
    }

    mLynxTemplateRender.renderSSRUrl(ssrUrl, injectedData);
  }

  /**
   * Attach page logic to a LynxView rendered with renderSSR.
   * Calling this api will make LynxView rendered with SSR data interactable and behave just like a
   * normal lynxview.
   *
   * @param tem The binary of the template
   * @param url The url of the template
   * @param data The init data of the template
   */
  public void ssrHydrate(
      @NonNull byte[] template, @NonNull String hydrateUrl, final Map<String, Object> data) {
    LLog.d(TAG,
        "ssrHydrate " + hydrateUrl + (data == null ? "" : (" with data in " + data.toString())));
    if (mLynxTemplateRender == null) {
      return;
    }

    mLynxTemplateRender.ssrHydrateWithBaseUrl(template, data, hydrateUrl);
  }

  /**
   * Attach page logic to a LynxView rendered with renderSSR.
   * Calling this api will make LynxView rendered with SSR data interactable and behave just like a
   * normal lynxview.
   *
   * @param tem The binary of the template
   * @param url The url of the template
   * @param data The init data of the template
   */
  public void ssrHydrate(
      @NonNull byte[] template, @NonNull String hydrateUrl, final TemplateData data) {
    LLog.d(TAG,
        "ssrHydrate " + hydrateUrl + (data == null ? "" : (" with data in " + data.toString())));
    if (mLynxTemplateRender == null) {
      return;
    }

    mLynxTemplateRender.ssrHydrateWithBaseUrl(template, data, hydrateUrl);
  }

  /**
   * Attach page logic to a LynxView rendered with renderSSR.
   * Calling this api will make LynxView rendered with SSR data interactable and behave just like a
   * normal lynxview.
   *
   * @param url The url of the template
   * @param data The init data of the template
   */
  public void ssrHydrateUrl(@NonNull String hydrateUrl, final Map<String, Object> data) {
    if (data != null) {
      LLog.d(TAG, "ssrHydrateUrl  " + hydrateUrl + " with data in " + data.toString());
    } else {
      LLog.d(TAG, "ssrHydrateUrl  " + hydrateUrl);
    }
    if (mLynxTemplateRender == null) {
      return;
    }

    mLynxTemplateRender.ssrHydrateUrl(hydrateUrl, data);
  }

  /**
   * Attach page logic to a LynxView rendered with renderSSR.
   * Calling this api will make LynxView rendered with SSR data interactable and behave just like a
   * normal lynxview.
   *
   * @param url The url of the template
   * @param data The init data of the template
   */
  public void ssrHydrateUrl(@NonNull String hydrateUrl, final TemplateData data) {
    LLog.d(TAG, "ssrHydrateUrl  " + hydrateUrl + " with data in " + data.toString());
    if (mLynxTemplateRender == null) {
      return;
    }

    mLynxTemplateRender.ssrHydrateUrl(hydrateUrl, data);
  }

  @Nullable
  public String getTemplateUrl() {
    if (mLynxTemplateRender == null) {
      return null;
    }
    return mLynxTemplateRender.getTemplateUrl();
  }

  public void updateData(String json) {
    this.updateData(json, null);
  }

  public void updateData(String json, String processorName) {
    checkAccessFromNonUiThread("updateData");
    LLog.i(TAG, "updateData with json in " + this.toString());
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.updateData(json, processorName);
  }

  /**
   * Normally, updateData before renderTemplate does not take effect. If you want to cache the data
   * updated before renderTemplate and use it together when renderTemplate, please set
   * EnablePreUpdateData option in LynxViewBuilder.
   */
  public void updateData(TemplateData data) {
    checkAccessFromNonUiThread("updateData");
    LLog.i(TAG, "updateData with data in " + data.toString());
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.updateData(data);
  }

  /**
   * **EXPERIMENTAL API**
   *
   * Method to update Lynx All in One.
   * Construct LynxUpdateMeta with coming data & globalProps & etc to be updated at once.
   *
   * @param meta LynxUpdateMeta to be used.
   */
  @AnyThread
  public void updateMetaData(LynxUpdateMeta meta) {
    // TODO(nihao.royal) migrate updateData/updateGlobalProps to this api later~
    checkAccessFromNonUiThread("updateMetaData");
    if (mLynxTemplateRender != null && meta != null) {
      mLynxTemplateRender.updateMetaData(meta);
    }
  }

  /**
   Clear current data and update with the given data.
   If you want to clear the current data only, you can pass an Empty LynxTemplateData
   */
  public void resetData(TemplateData data) {
    checkAccessFromNonUiThread("resetData");
    LLog.i(TAG, "resetData with json in " + data.toString());
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.resetData(data);
  }

  /**
   * Reload template with data
   * Clear current data and update with the given data.
   * If you want to clear the current data only, you can pass an Empty LynxTemplateData.
   * The lifecycle and sub-component's data would be renewed.
   */
  public void reloadTemplate(TemplateData data) {
    reloadTemplate(data, null);
  }
  /**
   * Reload template with data and globalProps.
   * @param globalProps default : null. Global props will be reset by new non null globalProps.
   *     Empty globalProps could overwrite the original props, but null object will do nothing.
   */
  public void reloadTemplate(TemplateData data, TemplateData globalProps) {
    checkAccessFromNonUiThread("reloadTemplate");
    LLog.i(TAG,
        "reloadTemplate with data: " + String.valueOf(data)
            + ", with globalProps:" + String.valueOf(globalProps));
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.reloadTemplate(data, globalProps);
  }

  /**
   * Deprecated, please use {@link #LynxTemplateResourceFetcher} instead.
   * Register lazy bundle with TemplateBundle.
   * Use the parsed TemplateBundle to preload a lazy bundle, making lazy bundle ready in
   * a flash
   * @return Whether the bundle is successfully registered into lynx view. Reasons for failure may
   *     include: the url or bundle is invalid; the bundle does not come from a lazy bundle
   *     template.
   * @param url url of lazy bundle
   * @param bundle parsed bundle of lazy bundle
   */
  @Deprecated
  public boolean registerDynamicComponent(@NonNull String url, @NonNull TemplateBundle bundle) {
    LLog.i(TAG, "register lazy bundle with TemplateBundle: " + url);
    if (mLynxTemplateRender == null) {
      return false;
    }
    return mLynxTemplateRender.registerLazyBundle(url, bundle);
  }

  /**
   * **EXPERIMENTAL API**
   *
   * Method to acquire current LynxView card data by Key.
   * Called IN **UI Thread**.
   *
   * Attentions: should be called after lynxview template loaded.
   * @param keys keys to be acquired.
   */
  public @Nullable Map<String, Object> getPageDataByKey(String[] keys) {
    checkAccessFromNonUiThread("getPageDataByKey");
    if (keys == null || keys.length == 0) {
      LLog.i(TAG, "getPageDataByKey called with empty keys.");
      return null;
    }
    if (mLynxTemplateRender == null) {
      return null;
    }

    return mLynxTemplateRender.getPageDataByKey(keys);
  }

  /**
   * Update GlobalProps by Increment. This can be called after LoadTemplate
   * @param props
   */
  public void updateGlobalProps(@NonNull Map<String, Object> props) {
    updateGlobalProps(TemplateData.fromMap(props));
  }

  /**
   * Update GlobalProps by Increment. This can be called after LoadTemplate
   * @param props If props is null or isEmpty, global props will not be updated
   */
  public void updateGlobalProps(@NonNull TemplateData props) {
    checkAccessFromNonUiThread("updateGlobalProps");
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.updateGlobalProps(props);
  }

  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    LLog.d("Lynx",
        "onMeasure:" + hashCode() + ", width" + MeasureSpec.toString(widthMeasureSpec) + ", height"
            + MeasureSpec.toString(heightMeasureSpec));

    if (mLynxTemplateRender == null) {
      super.onMeasure(widthMeasureSpec, heightMeasureSpec);
      return;
    }

    ILynxUIRenderer lynxUIRenderer = lynxUIRenderer();
    if ((lynxUIRenderer != null) && lynxUIRenderer.shouldInvokeNativeViewMethod()) {
      super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }

    mLynxTemplateRender.onMeasure(widthMeasureSpec, heightMeasureSpec);
  }

  @Keep
  @Override
  protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
    if (mLynxTemplateRender == null) {
      return;
    }

    ILynxUIRenderer lynxUIRenderer = lynxUIRenderer();
    if ((lynxUIRenderer != null) && lynxUIRenderer.shouldInvokeNativeViewMethod()) {
      super.onLayout(changed, left, top, right, bottom);
    }

    mLynxTemplateRender.onLayout(changed, left, top, right, bottom);

    if (changed && getLynxContext() != null && getLynxContext().useRelativeKeyboardHeightApi()) {
      if (mKeyboardEvent.isStart()) {
        mKeyboardEvent.detectKeyboardChangeAndSendEvent();
      }
    }
  }

  @Keep
  public void updateData(Map<String, Object> data) {
    this.updateData(data, null);
  }

  public void updateData(Map<String, Object> data, String processorName) {
    checkAccessFromNonUiThread("updateData");
    LLog.i(TAG, "updateData with map in " + this.toString());
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.updateData(data, processorName);
  }

  public void updateViewport(int widthMeasureSpec, int heightMeasureSpec) {
    checkAccessFromNonUiThread("updateViewport");
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.updateViewport(widthMeasureSpec, heightMeasureSpec);
  }

  /**
   * Changing the font scaling ratio in client settings will automatically change the text size;
   *
   * @param scale font scaling.
   */
  public void updateFontScale(float scale) {
    checkAccessFromNonUiThread("updateFontScale");
    int instanceId = -1;
    if (mLynxTemplateRender != null && mLynxTemplateRender.getLynxContext() != null) {
      instanceId = mLynxTemplateRender.getLynxContext().getInstanceId();
    }
    LynxFeatureCounter.count(LynxFeatureCounter.JAVA_UPDATE_FONT_SCALE, instanceId);
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.updateFontScale(scale);
  }

  public void destroy() {
    LLog.i(TAG, "lynxview destroy " + this.toString());
    TraceEvent.beginSection("DestroyLynxView");
    if (mKeyboardEvent.isStart()) {
      mKeyboardEvent.stop();
    }

    if (mLynxTemplateRender != null) {
      HeroTransitionManager.inst().onLynxViewDestroy(this);
      mLynxTemplateRender.onDetachedFromWindow();
      mLynxTemplateRender.destroy();
      mLynxTemplateRender = null;
    }

    if (mA11yWrapper != null) {
      mA11yWrapper.onDestroy();
    }

    TraceEvent.endSection("DestroyLynxView");
  }

  // TODO: 2020/3/25  final
  public ThreadStrategyForRendering getThreadStrategyForRendering() {
    if (mLynxTemplateRender == null) {
      return null;
    }
    return mLynxTemplateRender.getThreadStrategyForRendering();
  }

  @Override
  public boolean dispatchKeyEvent(KeyEvent event) {
    if ((lynxUIRenderer() == null) || (!lynxUIRenderer().needHandleDispatchKeyEvent())) {
      return super.dispatchKeyEvent(event);
    }

    boolean consumed = lynxUIRenderer().dispatchKeyEvent(event);

    if (consumed && mLynxTemplateRender != null) {
      mLynxTemplateRender.onDispatchInputEvent(event);
    }

    return consumed;
  }

  /**
   * If set enableEventThrough in index.json and users touch on LynxRootUI, return false to let
   * LynxView not consume events. Otherwise, call super.dispatchTouchEvent(ev) to let
   * onInterceptTouchEvent & onTouchEvent decide whether to consume events. Call
   * mLynxTemplateRender.onDispatchTouchEvent only when LynxView can consume event to avoid
   * debugging menu pop up after click.
   */
  @Keep
  @Override
  public boolean dispatchTouchEvent(MotionEvent ev) {
    try {
      LLog.i("Lynx", "LynxView dispatchTouchEvent, this: " + hashCode());

      if (mLynxTemplateRender == null) {
        return super.dispatchTouchEvent(ev);
      }

      ILynxUIRenderer lynxUIRenderer = lynxUIRenderer();
      if ((lynxUIRenderer != null) && lynxUIRenderer.shouldInvokeNativeViewMethod()) {
        return super.dispatchTouchEvent(ev);
      }

      int action = ev.getAction();
      if (action == MotionEvent.ACTION_DOWN) {
        if (LynxEnv.inst().isHighlightTouchEnabled()) {
          showMessageOnConsole(
              TAG + ": dispatch touch for lynx " + hashCode() + ", touch " + action,
              LogBoxLogLevel.Info.ordinal());
        }
        mCanDispatchTouchEvent = true;
      }

      boolean consumed = false;
      if (mCanDispatchTouchEvent) {
        // Dispatch event to sub ui
        consumed = mLynxTemplateRender.dispatchTouchEvent(ev);
        // If consumed && mLynxTemplateRender.blockNativeEvent(ev), call
        // getParent().requestDisallowInterceptTouchEvent(true) to consume event;
        if (consumed && mLynxTemplateRender.blockNativeEvent(ev) && getParent() != null) {
          getParent().requestDisallowInterceptTouchEvent(true);
        }
      }
      if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_CANCEL) {
        if (LynxEnv.inst().isHighlightTouchEnabled()) {
          showMessageOnConsole(
              TAG + ": dispatch touch for lynx " + hashCode() + ", touch " + action,
              LogBoxLogLevel.Info.ordinal());
        }
        mCanDispatchTouchEvent = false;
      }

      // If consumed, let ViewGroup call onTouchEvent. Otherwise, return false.
      if (consumed) {
        if (mDispatchTouchEventToDev) {
          // Dispatch event to devtool only when LynxView can consume event to avoid
          // debugging menu pop up after click.
          mLynxTemplateRender.onDispatchInputEvent(ev);
        }
        // If not consumeSlideEvent, call super.dispatchTouchEvent let other view consume the
        // events. Otherwise, return true such that only LynxView can consume the events, which
        // means only the consume-slide-event ui can respond the slide events.
        if (mLynxTemplateRender.consumeSlideEvent(ev)) {
          return true;
        } else {
          return super.dispatchTouchEvent(ev);
        }
      }
    } catch (Throwable e) {
      if (mLynxTemplateRender != null) {
        LynxError error = new LynxError(LynxSubErrorCode.E_EVENT_EXCEPTION,
            "An exception occurred during dispatchTouchEvent(): " + e.getMessage(),
            "This error is caught by native, please ask Lynx for help", LynxError.LEVEL_ERROR);
        error.setCallStack(CallStackUtil.getStackTraceStringTrimmed(e));
        mLynxTemplateRender.onErrorOccurred(error);
      }
    }
    return false;
  }

  /**
   * Used to output logs to the console of DevTool. This function is effective only when DevTool is
   * connected.
   * @param msg Message wanted.
   * @param level The log level.
   */
  private void showMessageOnConsole(String msg, int level) {
    LynxBaseInspectorOwner inspectorOwner = getBaseInspectorOwner();
    if (inspectorOwner == null) {
      return;
    }
    inspectorOwner.showMessageOnConsole(msg, level);
  }

  @Keep
  @Override
  public boolean onInterceptTouchEvent(MotionEvent ev) {
    try {
      LLog.i("Lynx", "LynxView onInterceptTouchEvent, this: " + hashCode());

      if (mLynxTemplateRender != null && mCanDispatchTouchEvent) {
        mLynxTemplateRender.onInterceptTouchEvent(ev);
      }
      return super.onInterceptTouchEvent(ev);
    } catch (Throwable e) {
      if (mLynxTemplateRender != null) {
        LynxError error = new LynxError(LynxSubErrorCode.E_EVENT_EXCEPTION,
            "An exception occurred during onInterceptTouchEvent(): " + e.getMessage(),
            "This error is caught by native, please ask Lynx for help", LynxError.LEVEL_ERROR);
        error.setCallStack(CallStackUtil.getStackTraceStringTrimmed(e));
        mLynxTemplateRender.onErrorOccurred(error);
      }
    }
    return false;
  }

  @Keep
  @Override
  public boolean onTouchEvent(MotionEvent ev) {
    try {
      LLog.i("Lynx", "LynxView onTouchEvent, this: " + hashCode());
      if (mLynxTemplateRender != null) {
        if (mCanDispatchTouchEvent) {
          mLynxTemplateRender.onTouchEvent(ev);
        }

        if (!mLynxTemplateRender.blockNativeEvent(ev) || getParent() == null) {
          super.onTouchEvent(ev);
        }
      }

      // In case when there is no children interested in handling touch event, we
      // return true from
      // the root view in order to receive subsequent events related to that gesture
      return true;
    } catch (Throwable e) {
      if (mLynxTemplateRender != null) {
        LynxError error = new LynxError(LynxSubErrorCode.E_EVENT_EXCEPTION,
            "An exception occurred during onTouchEvent(): " + e.getMessage(),
            "This error is caught by native, please ask Lynx for help", LynxError.LEVEL_ERROR);
        error.setCallStack(CallStackUtil.getStackTraceStringTrimmed(e));
        mLynxTemplateRender.onErrorOccurred(error);
      }
    }
    return false;
  }

  @Override
  protected void dispatchDraw(Canvas canvas) {
    super.dispatchDraw(canvas);
    if (mLynxTemplateRender != null) {
      mLynxTemplateRender.onRootViewDraw(canvas);
    }
  }

  @Keep
  @Override
  protected void onAttachedToWindow() {
    // ensure super attach before ui owner attach
    super.onAttachedToWindow();
    LLog.i("Lynx", "onAttachedToWindow:" + hashCode());
    if (mLynxTemplateRender != null) {
      // ui owner attach here
      mLynxTemplateRender.onAttachedToWindow();
    }
  }

  @Keep
  @Override
  protected void onDetachedFromWindow() {
    LLog.i("Lynx", "onDetachedFromWindow:" + hashCode());
    if (mLynxTemplateRender != null) {
      // ui owner detach here
      mLynxTemplateRender.onDetachedFromWindow();
    }
    // ensure super detach after ui owner detach
    super.onDetachedFromWindow();
  }

  @Nullable
  public View findViewByName(String name) {
    if (mLynxTemplateRender == null) {
      return null;
    }
    return mLynxTemplateRender.findViewByName(name);
  }

  @Nullable
  public LynxBaseUI findUIByName(String name) {
    if (mLynxTemplateRender == null) {
      return null;
    }
    return mLynxTemplateRender.findUIByName(name);
  }

  @Nullable
  public View findViewByIdSelector(String id) {
    if (mLynxTemplateRender == null) {
      return null;
    }
    return mLynxTemplateRender.findViewByIdSelector(id);
  }

  @Nullable
  public LynxBaseUI findUIByIdSelector(String id) {
    if (mLynxTemplateRender == null) {
      return null;
    }
    return mLynxTemplateRender.findUIByIdSelector(id);
  }

  public void innerSetMeasuredDimension(int w, int h) {
    setMeasuredDimension(w, h);
  }

  public UIGroup<UIBodyView> getLynxUIRoot() {
    if (mLynxTemplateRender == null) {
      return null;
    }
    return mLynxTemplateRender.getLynxRootUI();
  }

  @UiThread
  public void syncFlush() {
    if (mLynxTemplateRender != null) {
      mLynxTemplateRender.syncFlush();
    }
  }

  @Nullable
  public LynxBaseUI findUIByIndex(int index) {
    if (mLynxTemplateRender != null) {
      return mLynxTemplateRender.findUIByIndex(index);
    }
    return null;
  }

  @Override
  public void setOnClickListener(@Nullable OnClickListener listener) {
    // Workaround: In some usage scenarios user may set click listener on LynxView and
    // cause "click twice" issue, need to clear focusableInTouchMode flag in this case.
    // Attention: This may have impact on input focus scenarios, when clearFocus()
    // is called, current focus may be set to the first focusable component in some devices.
    setFocusableInTouchMode(listener == null);
    super.setOnClickListener(listener);
  }

  @Override
  public void setVisibility(int visibility) {
    super.setVisibility(visibility);
    LLog.i("Lynx", "setVisibility:" + hashCode() + " " + visibility);
  }

  public void setTimingCollector(TimingCollector timingCollector, ILynxUIRenderer lynxUIRenderer) {
    if ((lynxUIRenderer != null) && !lynxUIRenderer.enableTimingCollector()) {
      return;
    }
    super.setTimingCollector(timingCollector);
  }

  public void runOnTasmThread(Runnable runnable) {
    checkAccessFromNonUiThread("runOnTasmThread");
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.runOnTasmThread(runnable);
  }

  /**
   * start lynx runtime if runtime is pending.
   */
  public void startLynxRuntime() {
    checkAccessFromNonUiThread("startLynxRuntime");
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.startLynxRuntime();
  }

  /**
   * start or pause the UI operations.
   * @param enableUIFlush if enableUIFlush is false, lynx will pause UI operations until
   *     processRender is called.
   */
  public void setEnableUIFlush(boolean enableUIFlush) {
    checkAccessFromNonUiThread("setEnableUIFlush");
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.setEnableUIFlush(enableUIFlush);
  }

  /**
   * Turn on flush flag and flushing UI Operations.
   */
  @UiThread
  public void processRender() {
    checkAccessFromNonUiThread("processRender");
    if (mLynxTemplateRender == null) {
      return;
    }
    LLog.i(TAG, "LynxView call processRender in " + this.toString());
    mLynxTemplateRender.processRender();
  }

  public boolean enableJSRuntime() {
    if (mLynxTemplateRender != null) {
      return mLynxTemplateRender.enableJSRuntime();
    }
    // default is true
    return true;
  }

  /**
   * send global event to lepus runtime(for lynx air)
   *
   * @param name   event name
   * @param params context for event
   */
  public void triggerEventBus(String name, List<Object> params) {
    checkAccessFromNonUiThread("triggerEventBus");
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.triggerEventBus(name, params);
  }

  public boolean enableAirStrictMode() {
    if (mLynxTemplateRender != null) {
      return mLynxTemplateRender.enableAirStrictMode();
    }
    // default is false
    return false;
  }

  public KeyboardEvent getKeyboardEvent() {
    return mKeyboardEvent;
  }

  /**
   * Enable user bytecode. Controls bytecode for files other than lynx_core.js, such as
   * app-service.js.
   * @param enableUserBytecode if bytecode should bw enabled.
   * @param url Source url of the template.
   *            SourceUrl will be used to mark js files in bytecode.
   *            If the js file changes while url remains the same, code
   *            cache knows the js file is updated and will remove the
   *            cache generated by the former version of the js file.
   */
  public void setEnableUserBytecode(boolean enableUserBytecode, String url) {
    if (mLynxTemplateRender != null) {
      mLynxTemplateRender.setEnableBytecode(enableUserBytecode, url);
    }
  }

  private void checkAccessFromNonUiThread(String method) {
    boolean sEnableCheckAccessFromNonUiThread = LynxEnv.inst().enableCheckAccessFromNonUIThread();
    if (!sEnableCheckAccessFromNonUiThread || mHasReportedAccessFromNonUiThread) {
      return;
    }

    if (sMainLooper == Looper.myLooper() || TextUtils.isEmpty(mUrl)) {
      return;
    }

    mHasReportedAccessFromNonUiThread = true;
    Map<String, Object> props = new HashMap<>();
    props.put(LynxEventReporter.PROP_NAME_URL, mUrl);
    props.put(LynxEventReporter.PROP_NAME_THREAD_MODE,
        getThreadStrategyForRendering() != null ? getThreadStrategyForRendering().id() : -1);
    props.put(LynxEventReporter.PROP_NAME_LYNX_SDK_VERSION, LynxEnv.inst().getLynxVersion());
    props.put("method", method);
    LynxEventReporter.onEvent(
        "lynxsdk_access_lynxview_from_non_ui_thread", props, INSTANCE_ID_UNKNOWN);
  }

  /**
   * Set Storage for Lynx From TemplateData.
   *
   * @param key shared-data-key
   * @param value shared-data
   */
  public void setSessionStorageItem(String key, TemplateData value) {
    checkAccessFromNonUiThread("setSessionStorageItem");
    if (value != null && mLynxTemplateRender != null) {
      mLynxTemplateRender.setSessionStorageItem(key, value);
    }
  }

  /**
   * Get Storage for Lynx.
   *
   * @param key shared-data-key
   * @param callback callback to getSharedData.
   */
  public void getSessionStorageItem(String key, PlatformCallBack callback) {
    checkAccessFromNonUiThread("getSessionStorageItem");
    if (mLynxTemplateRender != null) {
      mLynxTemplateRender.getSessionStorageItem(key, callback);
    }
  }

  /**
   * subscribe storage listener for specified key
   *
   * @param key shared-data-key
   * @param callBack callback to getSharedData.
   * @return listenerId.
   */
  public double subscribeSessionStorage(String key, PlatformCallBack callBack) {
    checkAccessFromNonUiThread("subscribeSessionStorage");
    if (mLynxTemplateRender != null) {
      return mLynxTemplateRender.subscribeSessionStorage(key, callBack);
    }
    return PlatformCallBack.InvalidId;
  }

  /**
   * unsubscribe storage listener for specified key
   *
   * @param key shared-data-key.
   * @param id listenerId.
   */
  public void unsubscribeSessionStorage(String key, double id) {
    checkAccessFromNonUiThread("removeGlobalSharedDataListener");
    if (mLynxTemplateRender != null) {
      mLynxTemplateRender.unsubscribeSessionStorage(key, id);
    }
  }

  public void attachEngineToUIThread() {
    if (mLynxTemplateRender != null) {
      mLynxTemplateRender.attachEngineToUIThread();
    }
  }

  public void detachEngineFromUIThread() {
    if (mLynxTemplateRender != null) {
      mLynxTemplateRender.detachEngineFromUIThread();
    }
  }

  public ILynxUIRenderer lynxUIRenderer() {
    if (mLynxTemplateRender != null) {
      return mLynxTemplateRender.lynxUIRenderer();
    }
    return null;
  }

  public static LynxViewBuilder builder() {
    return new LynxViewBuilder();
  }

  @Keep
  @Deprecated
  public static LynxViewBuilder builder(Context context) {
    return new LynxViewBuilder();
  }

  @Deprecated
  public void setTheme(LynxTheme theme) {
    LynxContext lynxContext = getLynxContext();
    if (lynxContext != null) {
      LynxFeatureCounter.count(
          LynxFeatureCounter.JAVA_SET_THEME_ANDROID, lynxContext.getInstanceId());
    }
    checkAccessFromNonUiThread("setTheme");
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.setTheme(theme);
  }

  @Deprecated
  public void setTheme(ByteBuffer rawTheme) {
    LynxContext lynxContext = getLynxContext();
    if (lynxContext != null) {
      LynxFeatureCounter.count(
          LynxFeatureCounter.JAVA_SET_THEME_ANDROID, lynxContext.getInstanceId());
    }
    checkAccessFromNonUiThread("setTheme");
    if (mLynxTemplateRender == null || rawTheme == null) {
      return;
    }
    mLynxTemplateRender.setTheme(rawTheme);
  }

  @Deprecated
  public LynxTheme getTheme() {
    LynxContext lynxContext = getLynxContext();
    if (lynxContext != null) {
      LynxFeatureCounter.count(
          LynxFeatureCounter.JAVA_GET_THEME_ANDROID, lynxContext.getInstanceId());
    }
    if (mLynxTemplateRender == null) {
      return null;
    }
    return mLynxTemplateRender.getTheme();
  }

  /**
   * Deprecated, please use {@link #registerDynamicComponent(String, TemplateBundle)} instead.
   * <p>
   * Preload lazy bundles with urls.
   * If you want to improve the efficiency of the first-screen display of lazy bundles, you
   * can preload them.
   */
  @Deprecated
  public void preloadDynamicComponents(@NonNull String[] urls) {
    checkAccessFromNonUiThread("preloadDynamicComponents");
    LLog.i(TAG, "preload lazy bundles: " + TextUtils.join(", ", urls));
    if (mLynxTemplateRender == null || urls == null || urls.length == 0) {
      return;
    }
    mLynxTemplateRender.preloadLazyBundles(urls);
  }

  /**
   * Get template's version. Should be called after loadTemplate, return empty string otherwise.
   */
  @Deprecated
  public String getPageVersion() {
    return mLynxTemplateRender == null ? "" : mLynxTemplateRender.getPageVersion();
  }

  /**
   * Deprecated, please use {@link #getPageDataByKey(String[])} instead.
   * <p>
   * Method to acquire current LynxView card data.
   * Called IN UI Thread.
   * Attentions: should be called after lynxview template loaded.
   *
   * @param callback callback to acquire result.
   */
  @Deprecated
  public void getCurrentData(LynxGetDataCallback callback) {
    checkAccessFromNonUiThread("getCurrentData");
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.getCurrentData(callback);
  }

  /**
   * Must be called before LoadTemplate
   * <p>
   * Deprecated Since Version Lynx2.3. You should use {@link LynxView#updateGlobalProps(Map)}
   * instead.
   */
  @Deprecated
  public void setGlobalProps(Map<String, Object> props) {
    checkAccessFromNonUiThread("setGlobalProps");
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.updateGlobalProps(TemplateData.fromMap(props));
  }

  /**
   * Must be called before LoadTemplate
   * <p>
   * Deprecated Since Version Lynx2.3. You should use {@link LynxView#updateGlobalProps(Map)}
   * instead.
   */
  @Deprecated
  public void setGlobalProps(TemplateData props) {
    checkAccessFromNonUiThread("setGlobalProps");
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.updateGlobalProps(props);
  }

  /**
   * @return Performance data
   * @deprecated Forced retrieval of performance data is obsolete and always returns null.
   * Instead, override the method {@link LynxViewClient#onFirstLoadPerfReady(LynxPerfMetric)}
   * to collect performance data.
   */
  @Nullable
  @Deprecated
  public LynxPerfMetric forceGetPerf() {
    return null;
  }

  /**
   * Prefer using inheritance to log onMeasure.
   * Experimental method, avoid using it if possible.
   * Returns the duration of the first onMeasure.
   * If it returns -1, onMeasure has not occurred.
   *
   * @return The duration of the first onMeasure.
   */
  @Deprecated
  public long getFirstMeasureTime() {
    if (mLynxTemplateRender == null) {
      return -1;
    }
    return mLynxTemplateRender.getFirstMeasureTime();
  }

  /**
   * renderTemplate without UIFlush, always used with processRender
   *
   * @deprecated Please use {@link #loadTemplate(LynxLoadMeta)} instead.
   */
  @Deprecated
  public void processLayout(@NonNull String templateUrl, final TemplateData templateData) {
    LLog.i(TAG, "processLayout " + templateUrl + "with templateData in " + this.toString());
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.processLayout(templateUrl, templateData);
  }

  /**
   * renderSSRUrl without UIFlush, always used with processRender
   *
   * @deprecated Please use {@link #loadTemplate(LynxLoadMeta)} instead.
   */
  @Deprecated
  public void processLayoutWithSSRUrl(@NonNull String ssrUrl, final TemplateData templateData) {
    LLog.i(TAG, "processLayoutWithSSRUrl in " + this.toString());
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.processLayoutWithSSRUrl(ssrUrl, templateData);
  }

  /**
   * renderTemplateBundle without UIFlush, always used with processRender
   *
   * @deprecated Please use {@link #loadTemplate(LynxLoadMeta)} instead.
   */
  @Deprecated
  public void processLayoutWithTemplateBundle(
      @NonNull final TemplateBundle bundle, final TemplateData templateData, String baseUrl) {
    LLog.i(TAG, "processLayoutWithTemplateBundle in " + this.toString());
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.processLayoutWithTemplateBundle(bundle, templateData, baseUrl);
  }

  /**
   * Deprecated api, please use {@link #updateFontScale(float)} instead.
   */
  @Deprecated
  public void updateFontScacle(float scale) {
    checkAccessFromNonUiThread("updateFontScale");
    int instanceId = -1;
    if (mLynxTemplateRender != null && mLynxTemplateRender.getLynxContext() != null) {
      instanceId = mLynxTemplateRender.getLynxContext().getInstanceId();
    }
    LynxFeatureCounter.count(LynxFeatureCounter.JAVA_UPDATE_FONT_SCALE, instanceId);
    if (mLynxTemplateRender == null) {
      return;
    }
    mLynxTemplateRender.updateFontScale(scale);
  }

  public LynxConfigInfo getLynxConfigInfo() {
    return new LynxConfigInfo.Builder().buildLynxConfigInfo();
  }

  /**
   * Deprecated, please use {@link #setEnableUserBytecode(boolean, boolean)} instead.
   */
  public void setEnableUserCodeCache(boolean enableUserCodeCache, String url) {
    setEnableUserBytecode(enableUserCodeCache, url);
  }
}
