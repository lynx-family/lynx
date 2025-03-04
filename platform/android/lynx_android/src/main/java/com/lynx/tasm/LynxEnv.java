// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import android.app.Application;
import android.content.Context;
import android.content.SharedPreferences;
import android.text.TextUtils;
import android.view.inputmethod.InputMethodManager;
import androidx.annotation.AnyThread;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import com.google.gson.Gson;
import com.google.gson.JsonSyntaxException;
import com.lynx.BuildConfig;
import com.lynx.config.LynxLiteConfigs;
import com.lynx.devtoolwrapper.LynxDevToolUtils;
import com.lynx.jsbridge.LynxModule;
import com.lynx.jsbridge.LynxModuleFactory;
import com.lynx.tasm.IDynamicHandler;
import com.lynx.tasm.INativeLibraryLoader;
import com.lynx.tasm.LynxEnvKey;
import com.lynx.tasm.LynxEnvLazyInitializer;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.LynxSubErrorCode;
import com.lynx.tasm.LynxViewClient;
import com.lynx.tasm.LynxViewClientGroup;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.GlobalRefQueue;
import com.lynx.tasm.base.JNINamespace;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.LynxNativeMemoryTracer;
import com.lynx.tasm.base.LynxTraceEnv;
import com.lynx.tasm.base.TraceController;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.behavior.Behavior;
import com.lynx.tasm.behavior.BehaviorBundle;
import com.lynx.tasm.behavior.BuiltInBehavior;
import com.lynx.tasm.behavior.herotransition.HeroTransitionManager;
import com.lynx.tasm.behavior.shadow.text.TextRendererCache;
import com.lynx.tasm.behavior.ui.background.BackgroundImageLoader;
import com.lynx.tasm.behavior.utils.LynxUIMethodsHolderAutoRegister;
import com.lynx.tasm.behavior.utils.PropsHolderAutoRegister;
import com.lynx.tasm.fluency.FluencySample;
import com.lynx.tasm.icu.ICURegister;
import com.lynx.tasm.provider.AbsNetworkingModuleProvider;
import com.lynx.tasm.provider.AbsTemplateProvider;
import com.lynx.tasm.provider.LynxResourceProvider;
import com.lynx.tasm.provider.ResProvider;
import com.lynx.tasm.provider.ThemeResourceProvider;
import com.lynx.tasm.service.ILynxDevToolService;
import com.lynx.tasm.service.ILynxExtensionService;
import com.lynx.tasm.service.ILynxSystemInvokeService;
import com.lynx.tasm.service.ILynxTrailService;
import com.lynx.tasm.service.LynxServiceCenter;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicBoolean;

@Keep
@JNINamespace("lynxenv")
public class LynxEnv {
  protected static final String TAG = "LynxEnv";
  public static final String SP_NAME = "lynx_env_config";

  protected static volatile LynxEnv sInstance;

  protected static final ConcurrentHashMap<String, String> sExperimentSettingsMap =
      new ConcurrentHashMap<>();

  static {
    LynxUIMethodsHolderAutoRegister.init();
  }

  protected Application mContext;
  protected AbsTemplateProvider mTemplateProvider;
  protected AbsNetworkingModuleProvider mNetworkingModuleProvider;
  protected ResProvider mResProvider;
  protected ThemeResourceProvider mThemeResourceProvider;
  protected BehaviorBundle mViewManagerBundle;
  protected final AtomicBoolean hasInit = new AtomicBoolean(false);
  protected boolean mLynxDebugEnabled = false;
  /**
   * mDevToolComponentAttach: indicates whether DevTool Component is attached to
   * the host.
   * mDevToolEnabled: control whether to enable DevTool Debug
   * eg:
   * when host client attach DevTool, mDevToolComponentAttach is set true by
   * reflection to find
   * class defined in DevTool and now if we set mDevToolEnabled switch true,
   * DevTool Debug is
   * usable. if set mDevToolEnabled false, DevTool Debug is unavailable.
   * when host client doesn't attach DevTool, can't find class defined in DevTool
   * and
   * mDevToolComponentAttach is set false in this case, no matter mDevToolEnabled
   * switch is set true
   * or false ,DevTool Debug is unavailable
   * To sum up, mDevToolComponentAttach indicates host package type, online
   * package without DevTool
   * or localtest with DevTool mDevToolEnabled switch is controlled by user to
   * enable/disable
   * DevTool Debug, and useless is host doesn't attach DevTool
   */
  protected boolean mDevToolComponentAttach = false;
  protected boolean mDebugModeEnabled = false;
  protected boolean mLayoutOnlyEnabled = true;
  protected boolean mRecordEnable = false;
  protected boolean mHighlightTouchEnabled = false;
  protected boolean mCreateViewAsync = true;
  protected boolean mVsyncAlignedFlushGlobalSwitch = true;
  @Deprecated protected boolean mEnableJSDebug = true;
  @Deprecated protected boolean mDebug = false;

  protected volatile boolean mIsNativeLibraryLoaded = false;
  protected boolean mIsDevLibraryLoaded = false;
  protected boolean mIsNativeUIThreadInited = false;
  protected LynxModuleFactory mModuleFactory;
  protected final Map<String, Behavior> mBehaviorMap = new HashMap<>();
  protected final LynxViewClientGroup mClient = new LynxViewClientGroup();
  protected BackgroundImageLoader mBgImageLoader = null;

  protected InputMethodManager mInputMethodManager = null;

  /**
   * If set, an IllegalStateException will be thrown if PropsSetter is not
   * generated.
   */
  protected boolean mIsCheckPropsSetter = true;

  protected static Initializer sInitializer;
  protected volatile boolean hasCalledInitializer = false;
  protected INativeLibraryLoader mLibraryLoader = null;

  protected SharedPreferences mSharedPreferences;

  protected Map<String, LynxResourceProvider> mGlobalResourceProvider = new HashMap<>();
  // Android Chinese is zh-CN, while iOS is zh-Hans-CN
  protected String mLocale = null;
  protected String mLastUrl = null;

  protected boolean mDisableImagePostProcessor = false;
  protected boolean mEnableLoadImageFromService = false;

  protected boolean mEnableImageAsyncRedirect = false;
  protected boolean mEnableImageAsyncRedirectOnCreate = false;

  protected boolean mEnableImageAsyncRequest = false;

  protected boolean mEnableImageEventReport = false;

  // Whether enable reporting image memory with new protocol
  protected boolean mEnableImageMemoryReport = false;

  protected boolean mEnableComponentStatisticReport = false;

  protected boolean mEnableTransformForPositionCalculation = false;

  protected boolean mEnableSVGAsync = false;
  protected boolean mEnableGenericResourceFetcher = false;
  protected boolean mEnableTextBoringLayout = true;
  protected boolean mEnableCheckAccessFromNonUIThread = false;

  protected final Object mLazyInitLock = new Object();

  private static ILynxDevToolService devtoolService = null;

  private Boolean mHasV8BridgeLoadSuccess = false;

  private boolean mForceDisableQuickJsCache = false;

  protected LynxEnv() {}

  public static LynxEnv inst() {
    if (sInstance == null) {
      synchronized (LynxEnv.class) {
        if (sInstance == null) {
          sInstance = new LynxEnv();
        }
      }
    }
    return sInstance;
  }

  /**
   * Lynx Env init, need call before use LynxView.
   */
  public synchronized void init(Application context, INativeLibraryLoader nativeLibraryLoader,
      AbsTemplateProvider templateProvider, BehaviorBundle behaviorBundle) {
    init(context, nativeLibraryLoader, templateProvider, behaviorBundle, null);
  }

  /**
   * Lynx Env init, need call before use LynxView.
   * Use {@link com.lynx.tasm.LynxEnv#init(Application, INativeLibraryLoader, AbsTemplateProvider,
   * BehaviorBundle)} instead.
   */
  @Deprecated
  public synchronized void init(Application context, INativeLibraryLoader nativeLibraryLoader,
      AbsTemplateProvider templateProvider, BehaviorBundle behaviorBundle,
      @Nullable IDynamicHandler dynamicHandler) {
    initLynxServiceCenter();
    // The DevTool needs to be initialized as early as possible because LLog requires the state of
    // the DevTool to determine whether to output logs to the console.
    initDevtoolComponentAttachSwitch();
    mLynxDebugEnabled = mDevToolComponentAttach;

    if (hasInit.get()) {
      LLog.w(TAG, "LynxEnv is already initialized");
      return;
    }

    hasInit.set(true);
    // LLog can be called only after setting hasInit as true, otherwise, there will
    // be a circular
    // dependency
    LLog.i(TAG, "LynxEnv start init");

    // turn on systrace for app
    setAppTracingAllowed();

    // PropsHolderAutoRegister init
    PropsHolderAutoRegister.init();

    // Prepare environment
    mContext = context;
    mViewManagerBundle = behaviorBundle;
    mTemplateProvider = templateProvider;
    mLibraryLoader = nativeLibraryLoader;

    // get and set DebugMode form SharedPreferences
    setDebugMode(mContext);

    // init Behaviors and warm behaviors
    initBehaviors();

    // init ModuleFactory
    getModuleFactory().setContext(context);

    // init lynx trail service
    initLynxTrailService(context);

    // Calling sequence:
    // initDevtoolEnv() > loadNativeLibraries() > syncDevtoolComponentAttachSwitch()
    //
    // initDevtoolEnv():
    // This function will initialize LynxDevtoolEnv, and lynx_debug.so may be loaded
    // during the
    // process, so it should be called before the function loadNativeLibraries().
    //
    // loadNativeLibraries():
    // This function will load the native libraries if LynxDevtoolEnv hasn't been
    // initialized
    // before, such as the DevTool Component isn't attached or mlynxDebugEnabled is
    // set to false.
    //
    // syncDevtoolComponentAttachSwitch():
    // This function will sync the value of mDevToolComponentAttach to native. It
    // needs to be called
    // after the function loadNativeLibraries(), because at this time we can ensure
    // that the native
    // libraries have been loaded.
    initDevtoolEnv();

    if (!initNativeLibraries(nativeLibraryLoader)) {
      // This is a fatal error.
      // We won't be able to function without necessary libraries.
      // Thus a crash is coming right away.
      return;
    }

    syncDevtoolComponentAttachSwitch();

    // init Trace
    initTrace(mContext);

    LLog.initLynxLog();

    // settings update
    postUpdateSettings();

    // ensure init ui thread native loop
    initNativeUIThread();

    // init global cache pool
    initNativeGlobalPool();

    initImageExperimentSettings();
    initMemoryReportExperimentSettings();
    initEnableComponentStatisticReport();
    initEnableTransformForPositionCalculation();
    initEnableSvgAsync();
    initEnableGenericResourceFetcher();
    initEnableTextBoringLayout();
    initEnableCheckAccessFromNonUiThread();

    ICURegister.loadLibrary(mLibraryLoader);
    // notify LynxEnv prepared
    ILynxExtensionService extensionService =
        LynxServiceCenter.inst().getService(ILynxExtensionService.class);
    if (extensionService != null) {
      extensionService.onLynxEnvSetup();
    } else {
      LLog.w(TAG, "LynxEnv failed to get LynxExtensionService");
    }
  }

  /**
   * get debug mode flag from SharedPreferences and set to mDebugModeEnabled
   *
   * @param context
   */
  protected void setDebugMode(Context context) {
    mSharedPreferences = context.getSharedPreferences(SP_NAME, Context.MODE_PRIVATE);
    if (mSharedPreferences == null) {
      mDebugModeEnabled = false;
    } else {
      mDebugModeEnabled = mSharedPreferences.getBoolean(LynxEnvKey.SP_KEY_ENABLE_DEBUG_MODE, false);
    }
    TraceEvent.markTraceDebugMode(mDebugModeEnabled);
  }

  /**
   * turn on systrace for app
   * invoke android.os.Trace.setAppTracingAllowed
   */
  protected void setAppTracingAllowed() {
    if (TraceEvent.enableSystemTrace()) {
      Class<?> trace = null;
      try {
        LLog.d(TAG, "turn on systrace for app");
        trace = Class.forName("android.os.Trace");
        Method setAppTracingAllowed =
            trace.getDeclaredMethod("setAppTracingAllowed", boolean.class);
        setAppTracingAllowed.invoke(null, true);
      } catch (ClassNotFoundException e) {
        e.printStackTrace();
      } catch (NoSuchMethodException e) {
        e.printStackTrace();
      } catch (IllegalAccessException e) {
        e.printStackTrace();
      } catch (InvocationTargetException e) {
        e.printStackTrace();
      }
    }
  }

  /**
   * init trace env
   */
  protected void initTrace(Context context) {
    if (mIsNativeLibraryLoaded && TraceEvent.enableTrace()) {
      try {
        TraceController.getInstance().init(context);
        TraceController.getInstance().startStartupTracingIfNeeded();
      } catch (Exception e) {
        e.printStackTrace();
        LLog.e(TAG, "trace controller init failed");
      }
    }
  }

  @Deprecated
  public interface Initializer {
    void init();
  }

  /**
   * @see LynxEnvLazyInitializer#setLazyInitializer(LynxEnvLazyInitializer.Initializer)
   */
  @Deprecated
  public static void setLazyInitializer(Initializer initializer) {
    sInitializer = initializer;
  }

  public void lazyInitIfNeeded() {
    LynxEnvLazyInitializer.Initializer initializer;
    synchronized (mLazyInitLock) {
      if (hasInit.get() || hasCalledInitializer) {
        return;
      }
      initializer = LynxEnvLazyInitializer.getsInitializer();
      hasCalledInitializer = initializer != null || sInitializer != null;
    }

    if (initializer != null) {
      initializer.init();
      return;
    }

    if (sInitializer != null) {
      sInitializer.init();
    }
  }

  @Deprecated
  public void warmClass() {
    // do nothing
  }

  /**
   * Register module with module name and module class.
   *
   * @param name   Module's name, will be used in JS code.
   * @param module module class.
   */
  public void registerModule(String name, Class<? extends LynxModule> module) {
    registerModule(name, module, null);
  }

  /**
   * Register module with module name and module class.
   *
   * @param name   Module's name, will be used in JS code.
   * @param module module class, containing a no-parameter constructor.
   * @param param  the param will be provided to the module class's constructor
   *               when creating the
   *               module object.
   */
  public void registerModule(String name, Class<? extends LynxModule> module, Object param) {
    getModuleFactory().registerModule(name, module, param);
  }

  public LynxModuleFactory getModuleFactory() {
    if (mModuleFactory == null) {
      mModuleFactory = new LynxModuleFactory(mContext);
    }
    return mModuleFactory;
  }

  public void addResourceProvider(String key, LynxResourceProvider provider) {
    if (!TextUtils.isEmpty(key)) {
      mGlobalResourceProvider.put(key, provider);
    }
  }

  public Map<String, LynxResourceProvider> getResourceProvider() {
    return mGlobalResourceProvider;
  }

  /**
   * Assign an INativeLibraryLoader or a fallback loader to mLibraryLoader.
   *
   * After this initialization, mLibraryLoader won't be null.
   */
  protected void initLibraryLoader(INativeLibraryLoader loader) {
    if (loader != null) {
      mLibraryLoader = loader;
    } else if (mLibraryLoader == null) {
      // Use System as fallback loader
      // Plus, this a lambda with method reference
      mLibraryLoader = System::loadLibrary;
    }
  }

  /**
   * Use mLibraryLoader to load required libraries.
   * In most cases, you should call {@link LynxEnv#init(Application, INativeLibraryLoader,
   * AbsTemplateProvider, BehaviorBundle)}. you may call this, only in case you just want to load
   * lynx associated native libraries.
   *
   * @return whether loading is successful
   */
  public synchronized boolean initNativeLibraries(INativeLibraryLoader loader) {
    if (mIsNativeLibraryLoaded) {
      return true;
    }
    initLibraryLoader(loader);
    try {
      // We want to load libquick.so before liblynx.so
      if (LynxLiteConfigs.requireQuickSharedLibrary()) {
        // It is ok that quick does not exist when it's already statically linked
        mLibraryLoader.loadLibrary("quick");
      }
      mLibraryLoader.loadLibrary("lynx");
      if (!LynxTraceEnv.inst().isNativeLibraryLoaded()) {
        mLibraryLoader.loadLibrary("lynxtrace");
        LynxTraceEnv.inst().markNativeLibraryLoaded(true);
      }
    } catch (UnsatisfiedLinkError error) {
      // TODO(yueming): is catching this error necessary?
      // How about crashing directly?
      // Because it will eventually, whatsoever.
      LLog.e(TAG, error.getMessage() + ". Loader used was: " + mLibraryLoader);
      return false;
    }
    mIsNativeLibraryLoaded = true;
    LLog.i(TAG, "Loading native libraries succeeded");
    return true;
  }

  /*
   * This method is deprecated.
   * Please call boolean loadNativeLibraries() instead.
   * If you want to set a customized INativeLibraryLoader, do it in init().
   */
  @Deprecated
  public void loadNativeLynxLibrary(INativeLibraryLoader nativeLibraryLoader) {
    initNativeLibraries(nativeLibraryLoader);
  }

  protected void initBehaviors() {
    synchronized (mBehaviorMap) {
      List<Behavior> builtInBehaviors = new BuiltInBehavior().create();
      for (Behavior behavior : builtInBehaviors) {
        addBehaviorInner(behavior);
      }
      if (getBehaviorBundle() != null) {
        List<Behavior> behaviorsInBundle = getBehaviorBundle().create();
        if (behaviorsInBundle == null) {
          return;
        }
        for (Behavior behavior : behaviorsInBundle) {
          addBehaviorInner(behavior);
        }
      }
    }
  }

  public void addBehaviors(@NonNull List<Behavior> behaviors) {
    synchronized (mBehaviorMap) {
      for (Behavior behavior : behaviors) {
        addBehaviorInner(behavior);
      }
    }
  }

  public void addBehavior(@NonNull Behavior behavior) {
    synchronized (mBehaviorMap) {
      addBehaviorInner(behavior);
    }
  }

  /**
   * Adds a behavior to the behavior map. If a behavior with the same name already exists,
   * it will be overridden. This method is intended to be called from within a synchronized
   * block to ensure thread safety.
   *
   * @param behavior The behavior to be added. If null, the method will return without doing
   *     anything.
   *
   * Note: This method should only be called from within a synchronized block on mBehaviorMap
   * to ensure that concurrent access from multiple threads does not cause data inconsistency.
   *
   * Example usage:
   * synchronized (mBehaviorMap) {
   *     addBehaviorInner(behavior);
   * }
   */
  private void addBehaviorInner(Behavior behavior) {
    if (behavior == null) {
      return;
    }
    String name = behavior.getName();
    if (name == null) {
      return;
    }
    if (mBehaviorMap.containsKey(name)) {
      LLog.e(
          TAG, "Duplicated Behavior For Name " + name + " was added, oldBehavior will be override");
    }
    mBehaviorMap.put(name, behavior);
  }

  public List<Behavior> getBehaviors() {
    List<Behavior> behaviors;
    synchronized (mBehaviorMap) {
      behaviors = new ArrayList<>(mBehaviorMap.values());
    }
    return behaviors;
  }

  public Map<String, Behavior> getBehaviorMap() {
    Map<String, Behavior> behaviorMap;
    synchronized (mBehaviorMap) {
      behaviorMap = new HashMap<>(mBehaviorMap);
    }
    return behaviorMap;
  }

  public AbsTemplateProvider getTemplateProvider() {
    return mTemplateProvider;
  }

  public AbsNetworkingModuleProvider getNetworkingModuleProvider() {
    return mNetworkingModuleProvider;
  }

  public ResProvider getResProvider() {
    return mResProvider;
  }

  public void setResProvider(ResProvider resProvider) {
    this.mResProvider = resProvider;
  }

  public ThemeResourceProvider getThemeResourceProviderProvider() {
    return mThemeResourceProvider;
  }

  public void setThemeResourceProvider(ThemeResourceProvider themeResourceProvider) {
    this.mThemeResourceProvider = themeResourceProvider;
  }

  public BehaviorBundle getBehaviorBundle() {
    return mViewManagerBundle;
  }

  public synchronized String getLastUrl() {
    return mLastUrl;
  }

  public void setLastUrl(String url) {
    mLastUrl = url;
  }

  public boolean hasInited() {
    return hasInit.get();
  }

  protected void initDevtoolEnv() {
    if (isLynxDebugEnabled() && mContext != null) {
      try {
        devtoolService = LynxServiceCenter.inst().getService(ILynxDevToolService.class);
        if (devtoolService != null) {
          devtoolService.devtoolEnvInit(mContext);
        } else {
          LLog.e(TAG, "failed to get DevtoolService");
        }
      } catch (Exception e) {
        LLog.e(TAG, "initDevtoolEnv failed: " + e.toString());
      }
    }
  }

  /**
   * Set devtool switch value.
   *
   * @param key   Switch name.
   * @param value Switch value. Each key has a value of a fixed type, which may be
   *              Boolean or Integer.
   */
  public void setDevtoolEnv(String key, Object value) {
    if (isNativeLibraryLoaded() && isLynxDebugEnabled()) {
      LynxDevToolUtils.setDevtoolEnv(key, value);
    }
  }

  public void setDevtoolEnv(String groupKey, Set<String> newGroupValues) {
    if (isNativeLibraryLoaded() && isLynxDebugEnabled()) {
      LynxDevToolUtils.setDevtoolEnv(groupKey, newGroupValues);
    }
  }

  /**
   * Get devtool switch value.
   *
   * @param key          Switch name.
   * @param defaultValue The default value when the switch value cannot be found.
   * @return The Value of the switch.
   */
  public boolean getDevtoolEnv(String key, boolean defaultValue) {
    return (boolean) getDevtoolEnvInternal(key, defaultValue);
  }

  public int getDevtoolEnv(String key, int defaultValue) {
    return (int) getDevtoolEnvInternal(key, defaultValue);
  }

  protected Object getDevtoolEnvInternal(String key, Object defaultValue) {
    if (!isNativeLibraryLoaded()) {
      LLog.e(TAG, "getDevtoolEnv must be called after init! key: " + key);
      return defaultValue;
    }
    if (!isLynxDebugEnabled()) {
      LLog.e(TAG, "getDevtoolEnv must be called when isLynxDebugEnabled = true key: " + key);
      return defaultValue;
    }
    return LynxDevToolUtils.getDevtoolEnv(key, defaultValue);
  }

  public Set<String> getDevtoolEnv(String groupKey) {
    if (!isNativeLibraryLoaded()) {
      return new HashSet<>();
    }
    if (!isLynxDebugEnabled()) {
      LLog.e(
          TAG, "getDevtoolEnv must be called when isLynxDebugEnabled = true groupKey: " + groupKey);
      return new HashSet<>();
    }
    return LynxDevToolUtils.getDevtoolEnv(groupKey);
  }

  //------------warning---------------
  // Don't use LLog in this method,
  // otherwise will cause endless loop.
  // Because this method used by LLog.
  //------------warning----------------
  public boolean isLynxDebugEnabled() {
    // Return true only if the DevTool Component is attached and mLynxDebugEnabled
    // is true. It
    // avoids unnecessary reflection calls.
    return mDevToolComponentAttach && mLynxDebugEnabled;
  }

  // Provided for hosts that load the DevTool component after initializing
  // LynxEnv.
  // If the host uses the DevTool component as a plug-in, the DevTool component
  // hasn't been loaded
  // when initializing LynxEnv, so this function needs to be called after the
  // DevTool component is
  // loaded.
  // Only called by external developers.
  public void initDevtool() {
    initDevtoolComponentAttachSwitch();
    initDevtoolEnv();
    syncDevtoolComponentAttachSwitch();
  }

  public void enableLynxDebug(boolean enableLynxDebug) {
    LLog.i(TAG, enableLynxDebug ? "enable lynx debug" : "disable lynx debug");
    mLynxDebugEnabled = enableLynxDebug;
    initDevtoolEnv();
  }

  protected void initDevtoolComponentAttachSwitch() {
    devtoolService = LynxServiceCenter.inst().getService(ILynxDevToolService.class);
    if (devtoolService != null) {
      mDevToolComponentAttach = devtoolService.isDevtoolAttached();
    } else {
      mDevToolComponentAttach = false;
    }
    LLog.i(TAG,
        "The current application has embedded the DevTool Component: " + mDevToolComponentAttach);
  }

  protected void syncDevtoolComponentAttachSwitch() {
    // Since the default value in native is false, we only sync to native when the
    // value is true,
    // which can reduce native calls.
    if (isNativeLibraryLoaded() && mDevToolComponentAttach) {
      setBooleanLocalEnv(LynxEnvKey.DEVTOOL_COMPONENT_ATTACH, true);
    }
  }

  public boolean isDevtoolComponentAttach() {
    return mDevToolComponentAttach;
  }

  public boolean isDevtoolEnabled() {
    if (!isLynxDebugEnabled()) {
      return false;
    }
    return getDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_DEVTOOL, false);
  }

  // if true, user can debug any lynx view
  public void enableDevtool(boolean enableDevTool) {
    LLog.i(TAG, enableDevTool ? "Turn on devtool" : "Turn off devtool");
    if (enableDevTool) {
      LLog.setMinimumLoggingLevel(LLog.VERBOSE);
    } else {
      LLog.setMinimumLoggingLevel(LLog.INFO);
    }

    setDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_DEVTOOL, enableDevTool);
  }

  // if true, user can debug lynx view which is debuggable
  public boolean isDevtoolEnabledForDebuggableView() {
    return getDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_DEVTOOL_FOR_DEBUGGABLE_VIEW, false);
  }

  public void enableDevtoolForDebuggableView(boolean enable) {
    LLog.i(TAG,
        enable ? "Turn on devtool for debuggable view" : "Turn off devtool for debuggable view");
    setDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_DEVTOOL_FOR_DEBUGGABLE_VIEW, enable);
  }

  /**
   * @Deprecated
   * Please use isLogBoxEnabled() instead.
   */
  @Deprecated
  public boolean isRedBoxEnabled() {
    return isLogBoxEnabled();
  }

  public boolean isLogBoxEnabled() {
    return isDevtoolComponentAttach() && getDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_LOGBOX, true);
  }

  /**
   * @Deprecated
   * Please use enableLogBox(boolean enableLogBox) instead.
   */
  @Deprecated
  public void enableRedBox(boolean enableLogBox) {
    enableLogBox(enableLogBox);
  }

  public void enableLogBox(boolean enableLogBox) {
    LLog.i(TAG, enableLogBox ? "Turn on logbox" : "Turn off logbox");
    setDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_LOGBOX, enableLogBox);
  }

  public boolean isPixelCopyEnabled() {
    return getDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_PIXEL_COPY, true);
  }

  public void enablePixelCopy(boolean enabled) {
    setDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_PIXEL_COPY, enabled);
  }

  public boolean isHighlightTouchEnabled() {
    return mHighlightTouchEnabled && isLynxDebugEnabled();
  }

  public void enableHighlightTouch(boolean enableHighlightTouch) {
    LLog.i(TAG, enableHighlightTouch ? "Turn on highlighttouch" : "Turn off highlighttouch");
    setDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_HIGHLIGHT_TOUCH, enableHighlightTouch);
    mHighlightTouchEnabled = enableHighlightTouch;
  }

  public boolean isDebugModeEnabled() {
    return mDebugModeEnabled;
  }

  public void enableDebugMode(boolean enableDebugMode) {
    LLog.i(TAG, enableDebugMode ? "Turn on DebugMode" : "Turn off DebugMode");
    mDebugModeEnabled = enableDebugMode;
    TraceEvent.markTraceDebugMode(enableDebugMode);
    if (mSharedPreferences == null) {
      LLog.e(TAG, "enableDebugMode() must be called after init()");
      return;
    }
    mSharedPreferences.edit()
        .putBoolean(LynxEnvKey.SP_KEY_ENABLE_DEBUG_MODE, enableDebugMode)
        .apply();
  }

  // This interface is used by TestBench and is only used to debug.
  public boolean isLaunchRecordEnabled() {
    return getDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_LAUNCH_RECORD, false);
  }

  // This interface is used by TestBench and is only used to debug.
  public void enableLaunchRecord(boolean enableLaunchRecord) {
    LLog.i(TAG, enableLaunchRecord ? "Turn on launch record" : "Turn off launch record");
    setDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_LAUNCH_RECORD, enableLaunchRecord);
  }

  public void enableLayoutOnly(boolean enableLayoutOnly) {
    LLog.i(TAG, enableLayoutOnly ? "Turn on LayoutOnly" : "Turn off LayoutOnly");
    mLayoutOnlyEnabled = enableLayoutOnly;
  }

  public boolean enableVSyncAlignedMessageLoopGlobal() {
    return getBooleanFromExternalEnv(LynxEnvKey.ENABLE_VSYNC_ALIGNED_MESSAGE_LOOP_GLOBAL, false);
  }

  public boolean isLayoutOnlyEnabled() {
    return mLayoutOnlyEnabled;
  }

  public void setRecordEnable(boolean enable) {
    mRecordEnable = enable;
  }

  public boolean getRecordEnable() {
    return mRecordEnable;
  }

  public void setCreateViewAsync(boolean isCreateViewAsync) {
    mCreateViewAsync = isCreateViewAsync;
    LLog.i("LynxEnv_mCreateViewAsync:", mCreateViewAsync ? "true" : "false");
  }

  public void setVsyncAlignedFlushGlobalSwitch(boolean enable) {
    mVsyncAlignedFlushGlobalSwitch = enable;
    setBooleanLocalEnv(LynxEnvKey.ENABLE_VSYNC_ALIGNED_FLUSH, enable);
    LLog.i(TAG, "mVsyncAlignedFlushGlobalSwitch: " + mVsyncAlignedFlushGlobalSwitch);
  }

  public boolean getCreateViewAsync() {
    return mCreateViewAsync;
  }

  public boolean getVsyncAlignedFlushGlobalSwitch() {
    return mVsyncAlignedFlushGlobalSwitch;
  }

  // config by lynx settings
  public boolean isSettingsEnableNewImage() {
    return getBooleanFromExternalEnv(LynxEnvKey.USE_NEW_IMAGE, true);
  }

  // config by lynx web playground(jsb)

  @Deprecated
  public void setDebug(boolean debug) {
    mDebug = debug;
  }

  @Deprecated
  public void setEnableDevtoolDebug(boolean enableDevToolDebug) {
    enableDevtool(enableDevToolDebug);
  }

  @Deprecated
  public boolean isEnableDevtoolDebug() {
    return isDevtoolEnabled();
  }

  @Deprecated
  public void setEnableJSDebug(boolean enableJSDebug) {
    mEnableJSDebug = enableJSDebug;
  }

  @Deprecated
  public boolean isEnableJSDebug() {
    return mEnableJSDebug;
  }

  @Deprecated
  public void setEnableLogBox(boolean enableLogBox) {
    enableLogBox(enableLogBox);
  }

  @Deprecated
  public boolean isEnableLogBox() {
    return isLogBoxEnabled();
  }

  @Deprecated
  public boolean isRadonCompatibleEnabled() {
    return true;
  }

  @Deprecated
  public void enableRadonCompatible(boolean enableRadonCompatible) {}

  public boolean isNativeLibraryLoaded() {
    lazyInitIfNeeded();
    return mIsNativeLibraryLoaded;
  }

  public void setNativeLibraryLoaded(boolean status) {
    mIsNativeLibraryLoaded = status;
  }

  public Context getAppContext() {
    return mContext;
  }

  /**
   * use {@link ResProvider} instead.
   *
   * @param netProvider
   */
  @Deprecated
  // if want use NetworkingModule.request on js runtime should set it
  public void setNetworkingModuleProvider(AbsNetworkingModuleProvider netProvider) {
    mNetworkingModuleProvider = netProvider;
  }

  /// Call this method to release internal cache memory when process receives
  /// memory warning.
  public void onLowMemory() {
    TextRendererCache.cache().onLowMemory();
  }

  public InputMethodManager getInputMethodManager() {
    if (null == mInputMethodManager) {
      mInputMethodManager =
          (InputMethodManager) mContext.getSystemService(Context.INPUT_METHOD_SERVICE);
    }
    return mInputMethodManager;
  }

  public void setBackgroundImageLoader(BackgroundImageLoader bgImageLoader) {
    mBgImageLoader = bgImageLoader;
  }

  public BackgroundImageLoader getBackgroundImageLoader() {
    return mBgImageLoader;
  }

  public HeroTransitionManager getHeroTransitionManager() {
    return HeroTransitionManager.inst();
  }

  public String getLynxVersion() {
    return BuildConfig.LYNX_SDK_VERSION;
  }

  /**
   * Get the version of the SSR API. You should always include the SSR API version
   * when generating
   * SSR data with the SSR server, otherwise you may encounter compatibility
   * issues.It can be called
   * on any thread, but ensure the native library is loaded.
   *
   * @return If the local library is loaded, return SSRApiVersion.Otherwise,
   *         return empty string.
   */
  public String getSSRApiVersion() {
    if (!mIsNativeLibraryLoaded) {
      LLog.e(TAG, "The local library is not loaded, getting the ssr api version failed.");
      return "";
    }
    return nativeGetSSRApiVersion();
  }

  public void addLynxViewClient(LynxViewClient client) {
    if (client == null)
      return;
    mClient.addClient(client);
  }

  public void removeLynxViewClient(LynxViewClient client) {
    if (client == null)
      return;
    mClient.removeClient(client);
  }

  public void setLocale(String locale) {
    this.mLocale = locale;
  }

  public String getLocale() {
    if (mLocale == null) {
      ILynxSystemInvokeService systemInvokeService =
          LynxServiceCenter.inst().getService(ILynxSystemInvokeService.class);
      if (systemInvokeService != null) {
        mLocale = systemInvokeService.getLocale();
      } else {
        mLocale = Locale.getDefault().getLanguage() + "-" + Locale.getDefault().getCountry();
      }
    }
    return mLocale;
  }

  public LynxViewClientGroup getLynxViewClient() {
    return mClient;
  }

  public void setCheckPropsSetter(boolean enable) {
    mIsCheckPropsSetter = enable;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
  public void setSettings(HashMap<String, Object> newSettings) {
    sExperimentSettingsMap.clear();
    postUpdateSettings();
  }

  protected void postUpdateSettings() {
    if (mIsNativeLibraryLoaded) {
      nativeCleanExternalCache();
      FluencySample.needCheckUpdate();
    }
  }

  public boolean isCheckPropsSetter() {
    return mIsCheckPropsSetter;
  }

  public void setDevLibraryLoaded(boolean status) {
    mIsDevLibraryLoaded = status;
  }

  public void setUpNativeMemoryTracer(Context context, int minWatchedSize) {
    LynxNativeMemoryTracer.setup(context, minWatchedSize);
  }

  public void setUpNativeMemoryTracer(Context context) {
    LynxNativeMemoryTracer.setup(context);
  }

  public boolean isDevLibraryLoaded() {
    return mIsDevLibraryLoaded;
  }

  public INativeLibraryLoader getLibraryLoader() {
    return mLibraryLoader;
  }

  // issue: #1510
  public void reportModuleCustomError(String message) {
    mClient.onReceivedError(new LynxError(message, LynxSubErrorCode.E_NATIVE_MODULES_CUSTOM_ERROR));
  }

  public void setPiperMonitorState(boolean state) {
    setBooleanLocalEnv(LynxEnvKey.ENABLE_PIPER_MONITOR, state);
  }

  protected void onPiperInvoked(Map<String, Object> info) {
    mClient.onPiperInvoked(info);
  }

  @CalledByNative
  public static void reportPiperInvoked(
      String moduleName, String methodName, String paramStr, String url) {
    Map<String, Object> info = new HashMap<String, Object>();
    info.put("module-name", moduleName);
    info.put("method-name", methodName);
    info.put("url", url);
    if (!paramStr.isEmpty()) {
      ArrayList<Object> array = new ArrayList<Object>();
      array.add(paramStr);
      info.put("params", array);
    }
    inst().onPiperInvoked(info);
  }

  protected native String nativeGetSSRApiVersion();

  @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
  public native void nativeSetLocalEnv(String key, String value);

  public native void nativeSetGroupedEnv(String key, boolean value, String groupKey);

  public native void nativeSetGroupedEnvWithGroupSet(String groupKey, Set<String> newGroupValues);

  protected native void nativeCleanExternalCache();

  public native void nativeSetEnvMask(String key, boolean value);

  protected native String nativeGetDebugEnvDescription();

  public HashMap<String, String> GetNativeEnvDebugDescription() {
    HashMap<String, String> nativeEnvMap = null;
    String nativeEnvJsonString = nativeGetDebugEnvDescription();
    Gson gson = new Gson();
    try {
      nativeEnvMap = gson.fromJson(nativeEnvJsonString, HashMap.class);
    } catch (JsonSyntaxException e) {
      LLog.e(TAG, "Convert native env json string failed. e: " + e.getMessage());
    }
    return nativeEnvMap;
  }

  public HashMap<String, String> GetPlatformEnvDebugDescription() {
    HashMap<String, String> platformEnvMap = new HashMap<>();
    for (LynxEnvKey key : LynxEnvKey.values()) {
      String keyString = key.getDescription();
      if (keyString != null) {
        String value = getStringFromExternalEnv(key);
        if (value != null) {
          platformEnvMap.put(keyString, value);
        }
      }
    }
    return platformEnvMap;
  }

  public void setStringLocalEnv(LynxEnvKey key, String value) {
    nativeSetLocalEnv(key.getDescription(), value);
  }

  public void setBooleanLocalEnv(LynxEnvKey key, boolean value) {
    nativeSetLocalEnv(key.getDescription(), value ? "1" : "0");
  }

  public boolean disableImagePostProcessor() {
    return mDisableImagePostProcessor;
  }

  public boolean enableLoadImageFromService() {
    return mEnableLoadImageFromService;
  }

  // report image event to tea
  public boolean enableImageEventReport() {
    return mEnableImageEventReport;
  }

  public boolean enableImageAsyncRedirect() {
    return mEnableImageAsyncRedirect;
  }

  public boolean enableImageAsyncRedirectOnCreate() {
    return mEnableImageAsyncRedirectOnCreate;
  }

  public boolean enableImageAsyncRequest() {
    return mEnableImageAsyncRequest;
  }

  protected void initImageExperimentSettings() {
    mDisableImagePostProcessor =
        getBooleanFromExternalEnv(LynxEnvKey.DISABLE_POST_PROCESSOR, false);
    mEnableLoadImageFromService = getBooleanFromExternalEnv(LynxEnvKey.USE_NEW_IMAGE, false);
    mEnableImageAsyncRedirect =
        getBooleanFromExternalEnv(LynxEnvKey.ENABLE_IMAGE_ASYNC_REDIRECT, false);
    mEnableImageAsyncRedirectOnCreate =
        getBooleanFromExternalEnv(LynxEnvKey.ENABLE_IMAGE_ASYNC_REDIRECT_ON_CREATE, false);
    mEnableImageAsyncRequest =
        getBooleanFromExternalEnv(LynxEnvKey.ENABLE_IMAGE_ASYNC_REQUEST, false);
    mEnableImageEventReport =
        getBooleanFromExternalEnv(LynxEnvKey.ENABLE_IMAGE_EVENT_REPORT, false);
  }

  /**
   * Get whether switch to new Image memory report protocol
   *
   * @return whether switch to new Image memory report protocol
   */
  @RestrictTo(RestrictTo.Scope.LIBRARY)
  @AnyThread
  public boolean enableImageMemoryReport() {
    return mEnableImageMemoryReport;
  }

  protected void initMemoryReportExperimentSettings() {
    mEnableImageMemoryReport =
        getBooleanFromExternalEnv(LynxEnvKey.ENABLE_IMAGE_MEMORY_REPORT, false);
  }

  @AnyThread
  public boolean enableComponentStatisticReport() {
    return mEnableComponentStatisticReport;
  }

  protected void initEnableComponentStatisticReport() {
    mEnableComponentStatisticReport =
        getBooleanFromExternalEnv(LynxEnvKey.ENABLE_COMPONENT_STATISTIC_REPORT, false);
  }

  @AnyThread
  public boolean enableTransformForPositionCalculation() {
    return mEnableTransformForPositionCalculation;
  }

  protected void initEnableTransformForPositionCalculation() {
    mEnableTransformForPositionCalculation =
        getBooleanFromExternalEnv(LynxEnvKey.ENABLE_TRANSFORM_FOR_POSITION_CALCULATION, false);
  }

  public boolean enableSvgAsync() {
    return this.mEnableSVGAsync;
  }

  public void initEnableSvgAsync() {
    mEnableSVGAsync = getBooleanFromExternalEnv(LynxEnvKey.ENABLE_SVG_ASYNC, false);
  }

  public boolean enableGenericResourceFetcher() {
    return this.mEnableGenericResourceFetcher;
  }

  public boolean enableTextBoringLayout() {
    return this.mEnableTextBoringLayout;
  }

  public boolean enableCheckAccessFromNonUIThread() {
    return this.mEnableCheckAccessFromNonUIThread;
  }

  protected void initEnableGenericResourceFetcher() {
    mEnableGenericResourceFetcher =
        getBooleanFromExternalEnv(LynxEnvKey.ENABLE_GENERIC_RESOURCE_FETCHER, false);
  }

  protected void initEnableTextBoringLayout() {
    mEnableTextBoringLayout = getBooleanFromExternalEnv(LynxEnvKey.ENABLE_TEXT_BORING_LAYOUT, true);
  }

  protected void initEnableCheckAccessFromNonUiThread() {
    mEnableCheckAccessFromNonUIThread =
        getBooleanFromExternalEnv(LynxEnvKey.ENABLE_CHECK_ACCESS_FROM_NON_UI_THREAD, false);
  }

  protected void initLynxTrailService(Context context) {
    ILynxTrailService service = LynxServiceCenter.inst().getService(ILynxTrailService.class);
    if (service != null) {
      service.initialize(context);
    }
  }

  private void initLynxServiceCenter() {
    LynxServiceCenter.inst().initialize();
  }

  public void initNativeUIThread() {
    if (mIsNativeUIThreadInited) {
      return;
    }
    UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
      @Override
      public void run() {
        if (!mIsNativeLibraryLoaded) {
          return;
        }
        nativeInitUIThread();
        mIsNativeUIThreadInited = true;
      }
    });
  }

  protected static native void nativeInitUIThread();

  @CalledByNative
  protected static String getStringFromExternalEnv(String key) {
    String result = sExperimentSettingsMap.get(key);
    if (result == null) {
      ILynxTrailService service = LynxServiceCenter.inst().getService(ILynxTrailService.class);
      if (service != null) {
        result = service.stringValueForTrailKey(key);
      }
      // When LynxTrailService is updated, the cache in LynxEnv is cleared, so this behavior will
      // not cause the TrailValue update to fail. On the other hand, Temporarily storing an empty
      // string here can improve the efficiency of repeated requests.
      if (result == null) {
        result = "";
      }
      sExperimentSettingsMap.put(key, result);
    }
    return result;
  }

  public static String getStringFromExternalEnv(LynxEnvKey key) {
    return getStringFromExternalEnv(key.getDescription());
  }

  public static boolean getBooleanFromExternalEnv(LynxEnvKey key, boolean defaultValue) {
    final String value = getStringFromExternalEnv(key.getDescription());
    if (value != null && !value.isEmpty()) {
      return "1".equals(value) || "true".equalsIgnoreCase(value);
    }
    return defaultValue;
  }

  protected static final GlobalRefQueue sGlobalRefQueue = new GlobalRefQueue();

  protected static native void nativeRunJavaTaskOnConcurrentLoop(long taskId, int taskType);

  public static boolean runJavaTaskOnConcurrentLoop(Runnable task, int taskType) {
    long taskId = sGlobalRefQueue.push(task);
    if (taskId < 0) {
      LLog.e(TAG, "Failed to get free slot for java task");
      return false;
    }
    nativeRunJavaTaskOnConcurrentLoop(taskId, taskType);
    return true;
  }

  @CalledByNative
  public static void onJavaTaskOnConcurrentLoop(long taskId, int taskType) {
    Runnable task = (Runnable) sGlobalRefQueue.pop(taskId);
    if (task == null) {
      LLog.e(TAG, "Failed to get java task for id " + taskId + " type " + taskType);
      return;
    }
    task.run();
  }

  protected void initNativeGlobalPool() {
    if (mIsNativeLibraryLoaded) {
      nativePrepareLynxGlobalPool();
    }
  }

  /**
   * clear bytecode for bytecodeSourceUrl
   * when bytecodeSourceUrl is empty, that means clean all bytecode.
   */
  public static void clearBytecode(String bytecodeSourceUrl, boolean useV8) {
    if (inst().isNativeLibraryLoaded()) {
      nativeClearBytecode(bytecodeSourceUrl, useV8);
    }
  }

  public boolean tryToLoadV8Bridge(boolean unused) {
    synchronized (this) {
      if (mHasV8BridgeLoadSuccess) {
        return true;
      }

      final String v8BridgeLibName = "lynx_v8_bridge";
      try {
        if (mLibraryLoader != null) {
          mLibraryLoader.loadLibrary(v8BridgeLibName);
        } else {
          System.loadLibrary(v8BridgeLibName);
        }
        mHasV8BridgeLoadSuccess = true;
      } catch (Throwable e) {
        LLog.w(TAG, "try to load library " + v8BridgeLibName + " error" + e.toString());
        mHasV8BridgeLoadSuccess = false;
      }
      return mHasV8BridgeLoadSuccess;
    }
  }

  public void forceDisableQuickJsCache() {
    mForceDisableQuickJsCache = true;
    setBooleanLocalEnv(LynxEnvKey.FORCE_DISABLE_QUICKJS_CACHE, mForceDisableQuickJsCache);
  }

  protected static native void nativePrepareLynxGlobalPool();
  private static native void nativeClearBytecode(String bytecodeSourceUrl, boolean useV8);
}
