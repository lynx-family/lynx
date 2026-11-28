// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.animax;

import android.content.Context;
import androidx.annotation.Keep;
import androidx.annotation.RestrictTo;
import com.lynx.animax.ability.LynxAbility;
import com.lynx.animax.loader.AnimaXResourceLoaderHolder;
import com.lynx.animax.loader.LynxHttpAnimaXLoader;
import com.lynx.animax.util.AnimaX;
import com.lynx.animax.util.LynxAnimaX;
import com.lynx.animax.util.UIThreadUtils;
import com.lynx.jsbridge.LynxExtensionModule;
import com.lynx.jsbridge.LynxMethod;
import com.lynx.jsbridge.LynxModule;
import com.lynx.jsbridge.RuntimeLifecycleListener;
import com.lynx.jsbridge.RuntimeLifecycleListenerDelegate;
import com.lynx.react.bridge.Callback;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxGroup;
import com.lynx.tasm.behavior.BehaviorRegistry;
import com.lynx.tasm.behavior.LynxContext;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

@Keep
@RestrictTo(RestrictTo.Scope.LIBRARY)
public final class LynxAnimaXPreloadModule
    extends LynxExtensionModule implements AnimaXPreloadHost.Provider, UIAnimaX.CompositionHost {
  public static final String NAME = UIAnimaX.CompositionHost.KEY;

  private final RuntimeLifecycleListenerDelegate.ExtensionDelegate mExtensionDelegate;
  private HostRuntimeListener mRuntime;
  private LynxAbility mAbility;
  private AnimaXResourceLoaderHolder mResourceLoader;
  private volatile boolean mDestroyed;

  public LynxAnimaXPreloadModule(LynxContext context, LynxGroup group, BehaviorRegistry registry) {
    super(context, group, registry);
    // The native delegate holds the factory, so the factory must not retain
    // this module, or a page released without destroy() is never collected.
    WeakReference<LynxAnimaXPreloadModule> module = new WeakReference<>(this);
    mExtensionDelegate = RuntimeLifecycleListenerDelegate.createExtensionDelegate(() -> {
      LynxAnimaXPreloadModule owner = module.get();
      return owner == null ? null : owner.createRuntimeListener();
    });
  }

  private RuntimeLifecycleListenerDelegate createRuntimeListener() {
    List<Callback> callbacks = Collections.emptyList();
    HostRuntimeListener runtime;
    synchronized (this) {
      // Lifecycle registration must not load AnimaX on pages that never use it.
      if (mDestroyed) {
        return null;
      }
      if (mRuntime != null) {
        callbacks = mRuntime.reset();
        if (mRuntime.host != null) {
          mRuntime.host.invalidate();
        }
      }
      runtime = new HostRuntimeListener(this);
      mRuntime = runtime;
    }
    completeCallbacks(callbacks, false, "EnvironmentUnavailable", "AnimaX runtime was replaced");
    return new RuntimeLifecycleListenerDelegate(new WeakReference<>(mContext), runtime);
  }

  private enum InitializationState { UNINITIALIZED, INITIALIZING, READY }

  // Protected by the module lock. READY means this Env owns the host, not just
  // that the process libraries have loaded. A replacement runtime starts fresh.
  // The runtime holds this listener through a JNI global reference, so it must
  // not retain the module, whose context reaches the page.
  private static final class HostRuntimeListener implements RuntimeLifecycleListener {
    private final WeakReference<LynxAnimaXPreloadModule> owner;
    private long env;
    private AnimaXPreloadHost host;
    private InitializationState state = InitializationState.UNINITIALIZED;
    private List<Callback> callbacks = new ArrayList<>();

    private HostRuntimeListener(LynxAnimaXPreloadModule module) {
      owner = new WeakReference<>(module);
    }

    private List<Callback> takeCallbacks() {
      List<Callback> pending = callbacks;
      callbacks = new ArrayList<>();
      return pending;
    }

    private List<Callback> reset() {
      state = InitializationState.UNINITIALIZED;
      return takeCallbacks();
    }

    @Override
    public void onRuntimeAttach(long env, String runtimeType) {
      LynxAnimaXPreloadModule module = owner.get();
      if (module == null) {
        return;
      }
      List<Callback> pending = Collections.emptyList();
      synchronized (module) {
        if (this.env != 0 && this.env != env) {
          pending = reset();
          if (host != null) {
            host.invalidate();
            host = null;
          }
        }
        this.env = env;
      }
      completeCallbacks(pending, false, "EnvironmentUnavailable", "AnimaX runtime was replaced");
      module.attachRuntimeOnJS(this, env, false);
    }

    @Override
    public void onRuntimeDetach() {
      LynxAnimaXPreloadModule module = owner.get();
      if (module == null) {
        // The page was collected without destroy(). Releasing the Env data
        // invalidates the host, and the host's cleanup reference frees it.
        return;
      }
      List<Callback> pending;
      synchronized (module) {
        env = 0;
        pending = reset();
        if (host != null) {
          host.onRuntimeDetach();
          host = null;
        }
      }
      completeCallbacks(pending, false, "EnvironmentUnavailable", "AnimaX runtime has exited");
    }
  }

  // Registering the class is cheap; Lynx creates the module on its first call.
  public static void registerNativeModule() {
    LynxEnv.inst().registerModule("AnimaXPreload", InitModule.class);
  }

  @Keep
  public static final class InitModule extends LynxModule {
    public InitModule(Context context) {
      super(context);
    }

    @LynxMethod
    public void ensureReady(LynxContext context, Callback callback) {
      // Lynx passes null when JS passes undefined or null for the callback.
      if (callback == null) {
        return;
      }
      // Shared module instances have an application context. Use the calling
      // page injected by Lynx rather than retaining a context in this module.
      LynxExtensionModule module = context == null ? null : context.getExtensionModuleByKey(NAME);
      if (!(module instanceof LynxAnimaXPreloadModule)) {
        callback.invoke(false, "EnvironmentUnavailable", "AnimaX page is unavailable");
        return;
      }
      ((LynxAnimaXPreloadModule) module).ensureReady(callback);
    }
  }

  private void ensureReady(Callback callback) {
    HostRuntimeListener runtime;
    long env;
    boolean ready;
    boolean available;
    synchronized (this) {
      runtime = mRuntime;
      env = runtime == null ? 0 : runtime.env;
      available = !mDestroyed && env != 0;
      ready = available && runtime.state == InitializationState.READY;
      if (available && !ready) {
        runtime.callbacks.add(callback);
        if (runtime.state == InitializationState.INITIALIZING) {
          return;
        }
        runtime.state = InitializationState.INITIALIZING;
      }
    }
    if (ready) {
      // NativeModule callbacks already return to their owning JS runtime.
      // Do not send an attached runtime through UI just to confirm it again.
      callback.invoke(true, "", "");
    } else if (!available) {
      callback.invoke(false, "EnvironmentUnavailable", "AnimaX runtime is unavailable");
    } else if (!UIThreadUtils.getUiThreadHandler().post(() -> initializeOnUI(runtime, env))) {
      failInitialization(runtime, env, "EnvironmentUnavailable", "AnimaX UI dispatch failed");
    }
  }

  private boolean isCurrentRuntime(HostRuntimeListener runtime, long env) {
    // Caller holds the module lock.
    return !mDestroyed && mRuntime == runtime && env != 0 && runtime.env == env;
  }

  private void initializeOnUI(HostRuntimeListener runtime, long env) {
    synchronized (this) {
      if (!isCurrentRuntime(runtime, env) || runtime.state != InitializationState.INITIALIZING) {
        return;
      }
    }
    try {
      // Keep library initialization on UI with its existing process-wide lock,
      // but do not hold the page lock while other JS callers join this request.
      LynxAnimaX.inst().init();
      if (!LynxAnimaX.inst().hasInitialized()) {
        failInitialization(
            runtime, env, "UnsupportedCapability", "AnimaX libraries are unavailable");
        return;
      }
      synchronized (this) {
        if (!isCurrentRuntime(runtime, env)) {
          return;
        }
        getOrCreateHost(runtime);
      }
    } catch (RuntimeException | LinkageError error) {
      failInitialization(
          runtime, env, "UnsupportedCapability", "AnimaX initialization failed: " + error);
      return;
    }
    mContext.runOnJSThread(() -> attachRuntimeOnJS(runtime, env, true));
  }

  private void failInitialization(
      HostRuntimeListener runtime, long env, String code, String message) {
    List<Callback> callbacks;
    synchronized (this) {
      if (!isCurrentRuntime(runtime, env) || runtime.state != InitializationState.INITIALIZING) {
        return;
      }
      callbacks = runtime.reset();
    }
    completeCallbacks(callbacks, false, code, message);
  }

  private static void completeCallbacks(
      List<Callback> callbacks, boolean success, String code, String message) {
    for (Callback callback : callbacks) {
      callback.invoke(success, code, message);
    }
  }

  private AnimaXPreloadHost getOrCreateHost(HostRuntimeListener runtime) {
    if (runtime.host == null) {
      runtime.host = new AnimaXPreloadHost(this);
    }
    return runtime.host;
  }

  @Override
  public long getExtensionDelegatePtr() {
    return mExtensionDelegate.getNativePtr();
  }

  @Override
  public void setUp() {}

  @Override
  public void onTemplateLoad(String url) {}

  @Override
  public void setCompositionMode(AnimaXPlayer player, boolean enabled) {
    player.setCompositionMode(enabled);
  }

  @Override
  public Map<String, Object> getCompositionReceiverInfo(AnimaXPlayer player) {
    com.lynx.animax.base.bridge.ReadableMap receiver = player.getCompositionReceiverInfo();
    if (receiver == null) {
      return null;
    }
    Map<String, Object> detail = new HashMap<>();
    detail.put("elementID", receiver.getString("elementID"));
    detail.put("instanceToken", receiver.getString("instanceToken"));
    return detail;
  }

  @Override
  public void cancelPendingCompositionBinding(AnimaXPlayer player) {
    player.cancelPendingCompositionBinding();
  }

  @Override
  public synchronized boolean attachPlayer(AnimaXPlayer player) {
    UIThreadUtils.assertOnUiThread();
    if (mDestroyed || mRuntime == null || !LynxAnimaX.inst().hasInitialized()) {
      return false;
    }
    HostRuntimeListener runtime = mRuntime;
    boolean attached = getOrCreateHost(runtime).attachPlayer(player);
    // Component creation already initialized AnimaX. Keep source property
    // flushing synchronous; only the new host's Env attachment is dispatched.
    long env = runtime.env;
    mContext.runOnJSThread(() -> attachRuntimeOnJS(runtime, env, false));
    return attached;
  }

  private void attachRuntimeOnJS(HostRuntimeListener runtime, long env, boolean finishRequest) {
    List<Callback> callbacks;
    boolean attached;
    synchronized (this) {
      if (!isCurrentRuntime(runtime, env) || runtime.host == null) {
        return;
      }
      attached = runtime.host.onRuntimeAttach(env);
      if (!attached && !finishRequest) {
        return;
      }
      runtime.state = attached ? InitializationState.READY : InitializationState.UNINITIALIZED;
      callbacks = runtime.takeCallbacks();
    }
    completeCallbacks(callbacks, attached, attached ? "" : "EnvironmentUnavailable",
        attached ? "" : "AnimaX host attachment failed");
  }

  @Override
  public float getDeviceDensity() {
    UIThreadUtils.assertOnUiThread();
    return mDestroyed ? 0 : mContext.getResources().getDisplayMetrics().density;
  }

  @Override
  public AnimaXResourceLoaderHolder getResourceLoader() {
    UIThreadUtils.assertOnUiThread();
    if (mDestroyed) {
      return null;
    }
    if (mResourceLoader == null) {
      AnimaX.inst().setAppContextIfUnset(mContext.getApplicationContext());
      mAbility = new LynxAbility(null, mContext);
      mResourceLoader = new AnimaXResourceLoaderHolder(
          () -> Collections.singletonList(new LynxHttpAnimaXLoader(mAbility, mContext)));
    }
    return mResourceLoader;
  }

  @Override
  public void destroy() {
    List<Callback> callbacks = Collections.emptyList();
    synchronized (this) {
      if (mDestroyed) {
        return;
      }
      mDestroyed = true;
      if (mRuntime != null) {
        callbacks = mRuntime.reset();
        if (mRuntime.host != null) {
          mRuntime.host.invalidate();
        }
      }
    }
    mExtensionDelegate.destroy();
    UIThreadUtils.runOnUiThreadImmediately(this::releaseResourceServices);
    completeCallbacks(callbacks, false, "EnvironmentUnavailable", "AnimaX page has exited");
  }

  private void releaseResourceServices() {
    if (mResourceLoader != null) {
      mResourceLoader.release();
      mResourceLoader = null;
    }
    if (mAbility != null) {
      mAbility.release();
      mAbility = null;
    }
  }
}
