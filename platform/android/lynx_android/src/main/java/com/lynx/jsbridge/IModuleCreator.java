// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge;

import androidx.annotation.IntDef;
import androidx.annotation.Nullable;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.concurrent.ConcurrentHashMap;

/**
 * IModuleCreator is used to create LynxModuleWrapper in {@link LynxModuleFactory}
 * {@link CommonModuleCreator} is used to create default LynxModule instance.
 * {@link SharedModuleCreator} is used to create shared LynxModule instance. only use in {@link
 * com.lynx.tasm.group.LynxViewGroup}
 */
public interface IModuleCreator {
  @IntDef({Type.NORMAL, Type.SHARED})
  @Retention(RetentionPolicy.SOURCE)
  public @interface Type {
    int NORMAL = 0;
    int SHARED = 1;
  }

  IContextFinder currentContextFinder();

  /**
   * create LynxModuleWrapper.
   * @param name moduleName
   * @param ClassParams {@link LynxModuleFactory#mWrappers}
   * @return create new LynxModule instance & wrap it with LynxModuleWrapper.
   */
  @Nullable
  LynxModuleWrapper create(String name, ConcurrentHashMap<String, ParamWrapper> ClassParams);

  void destroy();

  @Type int Type();
}
