// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer;

import android.app.Application;
import com.facebook.drawee.backends.pipeline.Fresco;
import com.facebook.imagepipeline.core.ImagePipelineConfig;
import com.facebook.imagepipeline.memory.PoolConfig;
import com.facebook.imagepipeline.memory.PoolFactory;
import com.lynx.explorer.modules.LynxModuleAdapter;
import com.lynx.explorer.provider.DemoTemplateProvider;
import com.lynx.service.devtool.LynxDevToolService;
import com.lynx.service.http.LynxHttpService;
import com.lynx.service.image.LynxImageService;
import com.lynx.service.log.LynxLogService;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.service.ILynxDevToolService;
import com.lynx.tasm.service.ILynxHttpService;
import com.lynx.tasm.service.ILynxImageService;
import com.lynx.tasm.service.ILynxLogService;
import com.lynx.tasm.service.LynxServiceCenter;

public class ExplorerApplication extends Application {
  @Override
  public void onCreate() {
    super.onCreate();
    initLynxService();
    initLynxEnv();
    installLynxJSModule(); // register js bridge.
    initFresco();
  }

  private void initLynxEnv() {
    LynxEnv.inst().init(this, null, new DemoTemplateProvider(), null, null);
  }

  private void initLynxService() {
    LynxServiceCenter.inst().registerService(
        ILynxImageService.class, LynxImageService.getInstance());
    LynxServiceCenter.inst().registerService(ILynxLogService.class, LynxLogService.INSTANCE);
    LynxServiceCenter.inst().registerService(
        ILynxDevToolService.class, LynxDevToolService.INSTANCE);
    LynxServiceCenter.inst().registerService(ILynxHttpService.class, LynxHttpService.INSTANCE);
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
