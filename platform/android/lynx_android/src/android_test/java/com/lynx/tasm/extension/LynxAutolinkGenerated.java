// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.extension;

import android.content.Context;
import androidx.annotation.NonNull;
import com.lynx.jsbridge.LynxModule;
import com.lynx.tasm.LynxViewBuilder;
import com.lynx.tasm.behavior.Behavior;
import com.lynx.tasm.service.IServiceProvider;
import java.util.Arrays;

public final class LynxAutolinkGenerated {
  static final String GENERATED_BEHAVIOR = "autolink-generated-behavior";
  static final String GENERATED_MODULE = "AutolinkGeneratedModule";
  private static final String[] PROVIDERS = new String[] {GeneratedProvider.class.getName()};

  private static boolean sEnabled;
  private static int sProviderSetupCount;
  private static Context sProviderContext;

  private LynxAutolinkGenerated() {}

  public static void setupGlobal(@NonNull Context context) {
    if (!sEnabled) {
      return;
    }
    LynxExtensionRegistry.setupGlobal(context, PROVIDERS);
  }

  public static void setup(@NonNull LynxViewBuilder builder) {
    LynxExtensionRegistry.setup(builder, PROVIDERS);
  }

  static void enableForTest() {
    sEnabled = true;
  }

  static void resetForTest() {
    sEnabled = false;
    sProviderSetupCount = 0;
    sProviderContext = null;
  }

  static int getSetupCount() {
    return sProviderSetupCount;
  }

  static Context getProviderContext() {
    return sProviderContext;
  }

  static class GeneratedModule extends LynxModule {
    public GeneratedModule(Context context) {
      super(context);
    }
  }

  static class GeneratedProvider implements LynxExtensionProvider {
    @Override
    public void register(@NonNull LynxExtensionRegistry registry) {
      sProviderSetupCount++;
      registry.addBehaviors(Arrays.asList(new Behavior(GENERATED_BEHAVIOR)));
      registry.registerModule(GENERATED_MODULE, GeneratedModule.class);
      registry.registerService(GeneratedService.class);
      sProviderContext = registry.getContextOrThrow();
    }
  }

  static class GeneratedService implements IServiceProvider {
    boolean mInitialized;

    @Override
    public Class<? extends IServiceProvider> getServiceClass() {
      return GeneratedService.class;
    }

    @Override
    public void onInitialize(Context context) {
      mInitialized = true;
    }
  }
}
