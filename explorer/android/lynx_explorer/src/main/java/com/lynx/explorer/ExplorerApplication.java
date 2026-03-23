// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer;

import android.app.Activity;
import android.app.Application;
import android.os.Bundle;
import com.facebook.drawee.backends.pipeline.Fresco;
import com.facebook.imagepipeline.core.ImagePipelineConfig;
import com.facebook.imagepipeline.memory.PoolConfig;
import com.facebook.imagepipeline.memory.PoolFactory;
import com.lynx.debugrouter.DebugRouter;
import com.lynx.devtool.recorder.LynxRecorderPageManager;
import com.lynx.explorer.modules.ExplorerAppMessageHandler;
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
    installLynxJSModule(); // register native module.
    initDebugRouterHandlers();
    initActivityTracker();
    initFresco();
    initLynxRecorder();
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

    // enable all sessions debug
    LynxDevToolService.getINSTANCE().enableAllSessions();

    // set devtool preset values
    LynxDevToolService.getINSTANCE().setLynxDebugPresetValue(true);
    LynxDevToolService.getINSTANCE().setLogBoxPresetValue(true);
    LynxDevToolService.getINSTANCE().setLoadQJSBridge(true);
    LynxDevToolService.getINSTANCE().setLoadV8Bridge(true);
  }

  private void initDebugRouterHandlers() {
    DebugRouter.getInstance().addMessageHandler(ExplorerAppMessageHandler.createOpenPageHandler());
    DebugRouter.getInstance().addMessageHandler(ExplorerAppMessageHandler.createClosePageHandler());
  }

  private void initActivityTracker() {
    registerActivityLifecycleCallbacks(new ActivityLifecycleCallbacks() {
      @Override
      public void onActivityCreated(Activity activity, Bundle savedInstanceState) {}

      @Override
      public void onActivityStarted(Activity activity) {}

      @Override
      public void onActivityResumed(Activity activity) {
        LynxModuleAdapter.getInstance().onActivityResumed(activity);
      }

      @Override
      public void onActivityPaused(Activity activity) {
        LynxModuleAdapter.getInstance().onActivityPaused(activity);
      }

      @Override
      public void onActivityStopped(Activity activity) {}

      @Override
      public void onActivitySaveInstanceState(Activity activity, Bundle outState) {}

      @Override
      public void onActivityDestroyed(Activity activity) {}
    });
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
