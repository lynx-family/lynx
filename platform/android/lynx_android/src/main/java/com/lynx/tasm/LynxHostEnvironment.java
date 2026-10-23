// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import android.content.Context;
import androidx.annotation.Keep;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import com.lynx.jsbridge.*;
import com.lynx.jsbridge.network.LynxFetchModuleEventSender;
import com.lynx.tasm.behavior.BehaviorRegistry;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.component.DynamicComponentFetcher;
import com.lynx.tasm.core.resource.LynxResourceLoader;
import com.lynx.tasm.group.ILynxViewConfigProvider;
import com.lynx.tasm.provider.LynxProviderRegistry;
import com.lynx.tasm.provider.LynxResourceProvider;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Java host configuration and registrations, independent of page state.
 * A renderer owns a private environment; surfaces in a view group share one.
 */
@RestrictTo(RestrictTo.Scope.LIBRARY)
public final class LynxHostEnvironment {
  private final LynxBackgroundRuntimeOptions mRuntimeOptions;
  private final BehaviorRegistry mBehaviorRegistry;
  private final List<ParamWrapper> mMainThreadRegistrations;
  private final List<ParamWrapper> mBackgroundThreadRegistrations;
  @Keep @Nullable private LynxResourceLoader mLoader;
  @Keep private LynxResourceLoader mResourceLoader;
  private final LynxProviderRegistry mProviders;

  /** Returns the configured behavior registry shared by this host's consumers. */
  BehaviorRegistry getBehaviorRegistry() {
    return mBehaviorRegistry;
  }

  /**
   * Copies the configured runtime options and prepares provider and module registrations once.
   * The configured behavior registry is retained by reference. A renderer owns a private host;
   * surfaces in the same group share their host.
   */
  public LynxHostEnvironment(ILynxViewConfigProvider configProvider) {
    LynxBackgroundRuntimeOptions options = configProvider.getLynxRuntimeOptions();
    mBehaviorRegistry = configProvider.getBehaviorRegistry();
    mRuntimeOptions = options == null ? new LynxBackgroundRuntimeOptions()
                                      : new LynxBackgroundRuntimeOptions(options);
    mProviders = createProviderRegistry();
    Map<String, ParamWrapper> builtIns = createBuiltInRegistrations();
    Map<String, ParamWrapper> mainThread = new HashMap<>(builtIns);
    Map<String, ParamWrapper> backgroundThread = new HashMap<>();
    for (ParamWrapper wrapper : mRuntimeOptions.getWrappers()) {
      mainThread.put(wrapper.getName(), wrapper);
      backgroundThread.put(wrapper.getName(), wrapper);
    }
    // Preserve the renderer's precedence: users win on MTS, built-ins win on BTS.
    backgroundThread.putAll(builtIns);
    mMainThreadRegistrations = Collections.unmodifiableList(new ArrayList<>(mainThread.values()));
    mBackgroundThreadRegistrations =
        Collections.unmodifiableList(new ArrayList<>(backgroundThread.values()));
  }

  private static void addRegistration(
      Map<String, ParamWrapper> registrations, String name, Class<? extends LynxModule> type) {
    ParamWrapper wrapper = new ParamWrapper();
    wrapper.setName(name);
    wrapper.setModuleClass(type);
    registrations.put(name, wrapper);
  }

  private static Map<String, ParamWrapper> createBuiltInRegistrations() {
    Map<String, ParamWrapper> registrations = new HashMap<>();
    addRegistration(
        registrations, LynxIntersectionObserverModule.NAME, LynxIntersectionObserverModule.class);
    addRegistration(registrations, LynxUIMethodModule.NAME, LynxUIMethodModule.class);
    addRegistration(registrations, LynxTextInfoModule.NAME, LynxTextInfoModule.class);
    addRegistration(registrations, LynxAccessibilityModule.NAME, LynxAccessibilityModule.class);
    addRegistration(registrations, LynxSetModule.NAME, LynxSetModule.class);
    addRegistration(registrations, LynxResourceModule.NAME, LynxResourceModule.class);
    addRegistration(registrations, LynxExposureModule.NAME, LynxExposureModule.class);
    return registrations;
  }

  /**
   * Creates a page-owned main-thread factory from the merged registrations, with user modules
   * taking precedence. The creator and extra data are bound to the supplied page.
   */
  LynxModuleFactory createMainThreadFactory(LynxContext context, Object extraData) {
    LynxModuleFactory factory = new LynxModuleFactory();
    factory.addModuleParamWrapper(mMainThreadRegistrations);
    factory.bind(new CommonModuleCreator(createContextFinder(context)));
    factory.setLynxModuleExtraData(extraData);
    return factory;
  }

  /**
   * Creates a page-owned background-thread factory with built-in modules taking precedence.
   * Uses the host's auth validator and binds the creator, extra data, and Fetch sender to the page.
   */
  LynxModuleFactory createBackgroundThreadFactory(LynxContext context, Object extraData) {
    LynxModuleFactory factory = new LynxModuleFactory();
    factory.addModuleParamWrapper(mBackgroundThreadRegistrations);
    factory.bind(new CommonModuleCreator(createContextFinder(context)));
    factory.registerModuleAuthValidator(mRuntimeOptions.getModuleAuthValidator());
    factory.setLynxModuleExtraData(extraData);
    registerFetchModule(factory, context);
    return factory;
  }

  /**
   * Rebinds a standalone runtime's existing factory to a renderer and applies host registrations.
   * Preserves its creator, cached module instances, auth validator, and runtime-only registrations,
   * while replacing the Fetch registration with a sender for the supplied page.
   */
  void prepareReusedBackgroundThreadFactory(
      LynxModuleFactory factory, LynxContext context, Object extraData) {
    factory.resetContextFinder(createContextFinder(context));
    factory.setLynxModuleExtraData(extraData);
    // Retain runtime-only registrations and cached instances, overriding matching registrations
    // exactly as the renderer did when attaching a standalone runtime.
    factory.addModuleParamWrapper(mBackgroundThreadRegistrations);
    registerFetchModule(factory, context);
  }

  private static IContextFinder createContextFinder(LynxContext context) {
    return new IContextFinder() {
      private final WeakReference<Context> mContext = new WeakReference<>(context);

      public WeakReference<Context> findContext(String instanceId) {
        return mContext;
      }

      public void registerContext(String instanceId, WeakReference<Context> context) {}
    };
  }

  private static void registerFetchModule(LynxModuleFactory factory, LynxContext context) {
    LynxFetchModuleEventSender sender = new LynxFetchModuleEventSender();
    sender.setWeakContext(context);
    factory.registerModule(LynxFetchModule.NAME, LynxFetchModule.class, sender);
  }

  /**
   * Returns the host-owned provider registry, merged once with host providers overriding globals.
   * Pages borrow this registry and must not clear it when they are destroyed.
   */
  LynxProviderRegistry getProviderRegistry() {
    return mProviders;
  }

  private LynxProviderRegistry createProviderRegistry() {
    LynxProviderRegistry providers = new LynxProviderRegistry();
    for (Map.Entry<String, LynxResourceProvider> entry :
        LynxEnv.inst().getResourceProvider().entrySet()) {
      providers.addLynxResourceProvider(entry.getKey(), entry.getValue());
    }
    for (Map.Entry<String, LynxResourceProvider> entry :
        mRuntimeOptions.getAllResourceProviders()) {
      providers.addLynxResourceProvider(entry.getKey(), entry.getValue());
    }
    return providers;
  }

  /**
   * Creates and retains a private renderer's engine loader without runtime-option providers.
   * Reads fetchers from the current context at engine creation and uses the supplied error
   * receiver.
   */
  LynxResourceLoader createEngineResourceLoader(
      DynamicComponentFetcher fetcher, LynxContext context, ILynxErrorReceiver errorReceiver) {
    mLoader = new LynxResourceLoader(null, fetcher, errorReceiver,
        context.getTemplateResourceFetcher(), context.getGenericResourceFetcher());
    return mLoader;
  }

  /**
   * Creates and retains a private renderer's runtime loader using the host's runtime options.
   * Keeps it separate from the engine loader and reads fetchers from the current page context.
   * Local script errors use the supplied Java receiver, preserving the renderer's error behavior.
   */
  LynxResourceLoader createRuntimeResourceLoader(
      DynamicComponentFetcher fetcher, LynxContext context, ILynxErrorReceiver errorReceiver) {
    mResourceLoader = new LynxResourceLoader(mRuntimeOptions, fetcher, errorReceiver,
        context.getTemplateResourceFetcher(), context.getGenericResourceFetcher());
    return mResourceLoader;
  }

  /** Returns the retained engine loader, or null before it has been created. */
  LynxResourceLoader getEngineResourceLoader() {
    return mLoader;
  }

  /** Returns the retained runtime loader, or null before a runtime or surface loader is created. */
  LynxResourceLoader getRuntimeResourceLoader() {
    return mResourceLoader;
  }

  /**
   * Returns the host-owned runtime options by reference. Surfaces share these options;
   * page-specific state belongs to the page rather than this configuration.
   */
  LynxBackgroundRuntimeOptions getRuntimeOptions() {
    return mRuntimeOptions;
  }
}
