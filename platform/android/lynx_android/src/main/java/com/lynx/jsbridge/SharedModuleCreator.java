// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge;

import android.content.Context;
import androidx.annotation.NonNull;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.LLog;
import java.lang.ref.WeakReference;
import java.lang.reflect.Constructor;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Only Used in LynxViewGroup , can create SharedLynxModule instance
 */
public class SharedModuleCreator implements IModuleCreator {
  private static final String TAG = "SharedModuleCreator";
  /**
   * mModulesByName is used to store LynxModuleWrapper by module name.
   */
  private final ConcurrentHashMap<String, LynxModuleWrapper> mModulesByName =
      new ConcurrentHashMap<>();
  private volatile IContextFinder mContextFinder;

  public SharedModuleCreator(IContextFinder contextFinder) {
    mContextFinder = contextFinder;
  }

  @Override
  public void resetContextFinder(@NonNull IContextFinder contextFinder) {
    mContextFinder = contextFinder;
    for (LynxModuleWrapper wrapper : mModulesByName.values()) {
      wrapper.setContextFinder(contextFinder);
    }
  }

  @Override
  public IContextFinder currentContextFinder() {
    return mContextFinder;
  }

  @Override
  public LynxModuleWrapper create(
      String name, ConcurrentHashMap<String, ParamWrapper> ClassParams) {
    if (name == null) {
      LLog.e("LynxModuleFactory:" + TAG, "getModule failed, name is null");
      return null;
    }
    LynxModuleWrapper moduleWrapper = mModulesByName.get(name);
    if (moduleWrapper != null) {
      return moduleWrapper;
    }
    LynxModuleWrapper createdModuleWrapper = createModuleWrapper(name, ClassParams);
    if (createdModuleWrapper == null) {
      return null;
    }
    mModulesByName.put(name, createdModuleWrapper);
    return createdModuleWrapper;
  }

  private LynxModuleWrapper createModuleWrapper(
      String name, ConcurrentHashMap<String, ParamWrapper> ClassParams) {
    // create new module wrapper
    ParamWrapper wrapper = ClassParams.get(name);
    if (wrapper == null) {
      wrapper = LynxEnv.inst().getModuleFactory().getWrappers().get(name);
      if (wrapper == null) {
        return null;
      }
    }

    Class<? extends LynxModule> clazz = wrapper.getModuleClass();
    LynxModule module = null;

    // LynxContextModule cannot be registered in SharedModuleFactory.
    boolean isLynxContextBaseModule = LynxContextModule.class.isAssignableFrom(clazz);
    if (isLynxContextBaseModule) {
      return null;
    }
    // Context in SharedModuleCreator is Application Context!.
    Context context = LynxEnv.inst().getAppContext();
    try {
      if (wrapper.getParam() == null) {
        for (Constructor<?> ctor : clazz.getConstructors()) {
          Class[] types = ctor.getParameterTypes();
          if (types.length == 1 && Context.class.equals(types[0])) {
            module = (LynxModule) ctor.newInstance(context);
            break;
          } else if (types.length == 2 && Context.class.equals(types[0])
              && Object.class.equals(types[1])) {
            module = (LynxModule) ctor.newInstance(context, null);
            break;
          }
        }
      } else {
        Constructor<?> ctor = clazz.getConstructor(Context.class, Object.class);
        module = (LynxModule) ctor.newInstance(context, wrapper.getParam());
      }
    } catch (Exception e) {
      LLog.e(TAG, "create module failed, name: " + name + ", e: " + e);
    }

    if (module == null) {
      LLog.e("LynxModuleFactory:" + TAG, "getModule" + name + "failed");
      return null;
    }
    // cache module wrapper
    LynxModuleWrapper moduleWrapper = new LynxModuleWrapper(name, module);
    moduleWrapper.setContextFinder(mContextFinder);
    return moduleWrapper;
  }

  @Override
  public void destroy() {
    for (LynxModuleWrapper wrapper : mModulesByName.values()) {
      wrapper.destroy();
    }
    mModulesByName.clear();
  }

  @Override
  @Type
  public int Type() {
    return Type.SHARED;
  }
}
