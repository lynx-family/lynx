// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer;

import android.app.Application;
import android.content.Context;
import com.facebook.drawee.backends.pipeline.Fresco;
import com.facebook.imagepipeline.core.ImagePipelineConfig;
import com.facebook.imagepipeline.memory.PoolConfig;
import com.facebook.imagepipeline.memory.PoolFactory;
import com.lynx.devtool.recorder.LynxRecorderPageManager;
import com.lynx.devtoolwrapper.DevToolSettings;
import com.lynx.explorer.modules.LynxModuleAdapter;
import com.lynx.explorer.provider.DemoTemplateProvider;
import com.lynx.explorer.shell.LynxRecorderDefaultActionCallback;
import com.lynx.service.devtool.LynxDevToolService;
import com.lynx.service.http.LynxHttpService;
import com.lynx.service.image.LynxImageService;
import com.lynx.service.log.LynxLogService;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.service.ILynxHttpService;
import com.lynx.tasm.service.ILynxImageService;
import com.lynx.tasm.service.LynxServiceCenter;

public class ExplorerApplication extends Application {
  @Override
  public void onCreate() {
    super.onCreate();
    initLynxService();
    initLynxEnv();
    if (BuildConfig.ENABLE_SPARKLING) {
      initSparkling();
    }
    installLynxJSModule(); // register native module.
    initFresco();
    initLynxRecorder();
  }

  private void initSparkling() {
    // Use reflection so disabled builds compile without Sparkling on the classpath.
    // The classes below live under .hybridkit.* (not the top-level package) and HybridKit
    // is a Kotlin object whose methods must be invoked on its INSTANCE field, not statically.
    try {
      Class<?> hybridKitClass = Class.forName("com.tiktok.sparkling.hybridkit.HybridKit");
      Object hybridKit = hybridKitClass.getField("INSTANCE").get(null);
      hybridKitClass.getMethod("init", Application.class).invoke(hybridKit, this);

      Class<?> baseInfoConfigClass =
          Class.forName("com.tiktok.sparkling.hybridkit.config.BaseInfoConfig");
      // BaseInfoConfig(isDebug: Boolean) — primary Kotlin constructor.
      Object baseInfoConfig =
          baseInfoConfigClass.getConstructor(boolean.class).newInstance(BuildConfig.DEBUG);

      // SparklingLynxConfig.Builder(Application) — takes Application, not Context.
      Class<?> lynxBuilderClass =
          Class.forName("com.tiktok.sparkling.hybridkit.config.SparklingLynxConfig$Builder");
      Object lynxConfig = lynxBuilderClass.getConstructor(Application.class).newInstance(this);
      lynxConfig = lynxBuilderClass.getMethod("build").invoke(lynxConfig);

      // SparklingHybridConfig.Builder.setLynxConfig(ILynxConfig) — takes the interface,
      // SparklingLynxConfig implements it.
      Class<?> lynxConfigInterfaceClass =
          Class.forName("com.tiktok.sparkling.hybridkit.config.ILynxConfig");
      Class<?> hybridConfigClass =
          Class.forName("com.tiktok.sparkling.hybridkit.config.SparklingHybridConfig");
      Class<?> hybridBuilderClass =
          Class.forName("com.tiktok.sparkling.hybridkit.config.SparklingHybridConfig$Builder");
      Object hybridBuilder =
          hybridBuilderClass.getConstructor(baseInfoConfigClass).newInstance(baseInfoConfig);
      // setLynxConfig returns void in this Kotlin builder — don't reassign hybridBuilder.
      hybridBuilderClass.getMethod("setLynxConfig", lynxConfigInterfaceClass)
          .invoke(hybridBuilder, lynxConfig);
      Object hybridConfig = hybridBuilderClass.getMethod("build").invoke(hybridBuilder);

      hybridKitClass.getMethod("setHybridConfig", hybridConfigClass, Application.class)
          .invoke(hybridKit, hybridConfig, this);
      hybridKitClass.getMethod("initLynxKit").invoke(hybridKit);
      Class<?> sparklingNavigationRegistrar =
          Class.forName("com.lynx.explorer.sparkling.SparklingNavigationRegistrar");
      sparklingNavigationRegistrar.getMethod("install", Application.class).invoke(null, this);
      android.util.Log.i("ExplorerApplication", "Sparkling HybridKit initialized");
    } catch (ClassNotFoundException e) {
      if (BuildConfig.DEBUG) {
        throw new IllegalStateException("Sparkling is enabled but SDK classes are missing", e);
      }
      android.util.Log.e(
          "ExplorerApplication", "Sparkling SDK classes missing in enabled build", e);
    } catch (Exception e) {
      if (BuildConfig.DEBUG) {
        throw new IllegalStateException("Sparkling init failed", e);
      }
      android.util.Log.e("ExplorerApplication", "Sparkling init failed in enabled build", e);
    }
  }

  private void initLynxEnv() {
    LynxEnv.inst().init(this, null, new DemoTemplateProvider(), null);
  }

  private void initLynxRecorder() {
    LynxRecorderPageManager.getInstance().registerCallback(new LynxRecorderDefaultActionCallback());
  }

  private void initLynxService() {
    LynxServiceCenter.inst().registerService(LynxLogService.INSTANCE);
    LynxServiceCenter.inst().registerService(LynxImageService.getInstance());
    LynxServiceCenter.inst().registerService(LynxHttpService.INSTANCE);
    LynxServiceCenter.inst().registerService(LynxDevToolService.getINSTANCE());

    // enable debugging for all sessions
    LynxDevToolService.getINSTANCE().enableAllSessions();

    // set devtool bootstrap defaults
    DevToolSettings.inst().bootstrap().applyDevelopmentDefaultsIfUnset();
  }

  // merge it into InitProcessor later.
  private void installLynxJSModule() {
    LynxModuleAdapter.getInstance().Init(this);
  }

  private void initFresco() {
    final PoolFactory factory = new PoolFactory(PoolConfig.newBuilder().build());
    ImagePipelineConfig.Builder builder =
        ImagePipelineConfig.newBuilder(getApplicationContext()).setPoolFactory(factory);
    Fresco.initialize(getApplicationContext(), builder.build());
  }
}
