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
    initSparkling();
    installLynxJSModule(); // register native module.
    initFresco();
    initLynxRecorder();
  }

  private void initSparkling() {
    // Use reflection so this compiles when only the public Sparkling AAR is available
    // (which lacks HybridKit/SparklingLynxConfig). The private SDK distribution provides
    // these classes; without it we skip initialization gracefully.
    try {
      Class<?> hybridKit = Class.forName("com.tiktok.sparkling.HybridKit");
      hybridKit.getMethod("init", Application.class).invoke(null, this);

      Class<?> baseInfoConfigClass = Class.forName("com.tiktok.sparkling.config.BaseInfoConfig");
      Object baseInfoConfig =
          baseInfoConfigClass.getConstructor(boolean.class).newInstance(BuildConfig.DEBUG);

      Class<?> lynxConfigClass = Class.forName("com.tiktok.sparkling.config.SparklingLynxConfig");
      Class<?> lynxBuilderClass =
          Class.forName("com.tiktok.sparkling.config.SparklingLynxConfig$Builder");
      Object lynxConfig = lynxBuilderClass.getConstructor(Context.class).newInstance(this);
      lynxConfig = lynxBuilderClass.getMethod("build").invoke(lynxConfig);

      Class<?> hybridConfigClass =
          Class.forName("com.tiktok.sparkling.config.SparklingHybridConfig");
      Class<?> hybridBuilderClass =
          Class.forName("com.tiktok.sparkling.config.SparklingHybridConfig$Builder");
      Object hybridBuilder =
          hybridBuilderClass.getConstructor(baseInfoConfigClass).newInstance(baseInfoConfig);
      hybridBuilder = hybridBuilderClass.getMethod("setLynxConfig", lynxConfigClass)
                          .invoke(hybridBuilder, lynxConfig);
      Object hybridConfig = hybridBuilderClass.getMethod("build").invoke(hybridBuilder);

      hybridKit.getMethod("setHybridConfig", hybridConfigClass, Application.class)
          .invoke(null, hybridConfig, this);
      hybridKit.getMethod("initLynxKit").invoke(null);
    } catch (Exception ignored) {
      // Sparkling SDK not available in this build — skipping Sparkling integration.
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

    // set devtool preset values
    LynxDevToolService.getINSTANCE().setLynxDebugPresetValue(true);
    LynxDevToolService.getINSTANCE().setLogBoxPresetValue(true);
    LynxDevToolService.getINSTANCE().setLoadQJSBridge(true);
    LynxDevToolService.getINSTANCE().setLoadV8Bridge(true);
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
