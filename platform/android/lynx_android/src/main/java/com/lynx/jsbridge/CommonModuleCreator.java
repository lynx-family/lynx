// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge;

import android.content.Context;
import androidx.annotation.Nullable;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import java.lang.ref.WeakReference;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

/**
 * CommonModuleCreator is used to create lynx module instance and cache them. and use
 * lynxModuleWrapper to wrap them. detail:
 * 1. create lynx module instance by class name.
 * 2. cache module wrapper instance to avoid create multiple times.
 * 3. use lynxModuleWrapper to wrap module instance.
 * Difference between CommonModuleCreator and {@link SharedModuleCreator}:
 * 1. CommonModuleCreator can create LynxContextModule & LynxModule instance , and use unique
 * LynxContext.
 * 2. SharedModuleCreator can create LynxModule instance only, and use SharedContextFinder to find
 * LynxContext.
 */
public class CommonModuleCreator implements IModuleCreator {
  private static final String TAG = "CommonModuleCreator";

  private static final int SINGLE_PARAMETER_CONSTRUCTOR = 1;
  private static final int DOUBLE_PARAMETER_CONSTRUCTOR = 2;
  /**
   * cache module wrapper instance to avoid create multiple times.
   */
  private Map<String, LynxModuleWrapper> mModulesByName;
  /**
   * find LynxContext by instanceId.
   */
  private final IContextFinder mContextFinder;

  public CommonModuleCreator(IContextFinder contextFinder) {
    mContextFinder = contextFinder;
  }

  @Override
  public IContextFinder currentContextFinder() {
    return mContextFinder;
  }

  @Override
  @Nullable
  public LynxModuleWrapper create(
      String name, ConcurrentHashMap<String, ParamWrapper> ClassParams) {
    if (name == null) {
      LLog.e("LynxModuleFactory:" + TAG, "getModule failed, name is null");
      return null;
    }
    // Use cache
    if (mModulesByName == null) {
      mModulesByName = new HashMap<>();
    }
    if (mModulesByName.get(name) != null) {
      return mModulesByName.get(name);
    }
    // create new module wrapper
    ParamWrapper wrapper = ClassParams.get(name);
    if (wrapper == null) {
      wrapper = LynxEnv.inst().getModuleFactory().getWrappers().get(name);
      if (wrapper == null) {
        return null;
      }
    }
    // Since LynxContext may be used in non-MainThreads, weakRef must be used here.
    WeakReference<Context> weakContext = mContextFinder.findContext("");
    if (weakContext.get() == null) {
      LLog.e("LynxModuleFactory:" + TAG, "getModule failed, context is null");
      return null;
    }
    Class<? extends LynxModule> clazz = wrapper.getModuleClass();
    LynxModule module = null;
    // Create LynxContextModule instance
    try {
      boolean isLynxContextBaseModule = LynxContextModule.class.isAssignableFrom(clazz);
      Context context = weakContext.get();
      if (context == null) {
        LLog.e("LynxModuleFactory:" + TAG, clazz.getCanonicalName() + " called with Null context");
        return null;
      }
      // LynxContextModule
      if (isLynxContextBaseModule) {
        // LynxContextModule must be created with LynxContext
        boolean isLynxContext = context instanceof LynxContext;
        if (!isLynxContext) {
          throw new Exception(clazz.getCanonicalName() + " must be created with LynxContext");
        }

        // real create LynxContextModule instance
        if (wrapper.getParam() == null) {
          for (Constructor<?> ctor : clazz.getConstructors()) {
            Class[] types = ctor.getParameterTypes();
            if (types.length == SINGLE_PARAMETER_CONSTRUCTOR
                && LynxContext.class.equals(types[0])) {
              module = (LynxModule) ctor.newInstance((LynxContext) context);
              break;
            } else if (types.length == DOUBLE_PARAMETER_CONSTRUCTOR
                && LynxContext.class.equals(types[0]) && Object.class.equals(types[1])) {
              module = (LynxModule) ctor.newInstance((LynxContext) context, null);
              break;
            }
          }
        } else {
          Constructor<?> ctor = clazz.getConstructor(LynxContext.class, Object.class);
          module = (LynxModule) ctor.newInstance((LynxContext) context, wrapper.getParam());
        }
        // Create LynxModule instance
      } else {
        if (wrapper.getParam() == null) {
          for (Constructor<?> ctor : clazz.getConstructors()) {
            Class[] types = ctor.getParameterTypes();
            if (types.length == SINGLE_PARAMETER_CONSTRUCTOR && Context.class.equals(types[0])) {
              module = (LynxModule) ctor.newInstance(context);
              break;
            } else if (types.length == DOUBLE_PARAMETER_CONSTRUCTOR
                && Context.class.equals(types[0]) && Object.class.equals(types[1])) {
              module = (LynxModule) ctor.newInstance(context, null);
              break;
            }
          }
        } else {
          Constructor<?> ctor = clazz.getConstructor(Context.class, Object.class);
          module = (LynxModule) ctor.newInstance(context, wrapper.getParam());
        }
      }
    } catch (InstantiationException e) {
      LLog.e("LynxModuleFactory:" + TAG, "get Module failed" + e);
    } catch (IllegalAccessException e) {
      LLog.e("LynxModuleFactory:" + TAG, "get Module failed" + e);
    } catch (NoSuchMethodException e) {
      LLog.e("LynxModuleFactory:" + TAG, "get Module failed" + e);
    } catch (InvocationTargetException e) {
      LLog.e("LynxModuleFactory:" + TAG, "get TargetException " + e.getTargetException());
    } catch (Exception e) {
      LLog.e("LynxModuleFactory:" + TAG, "get Module failed" + e);
    }

    if (module == null) {
      LLog.e("LynxModuleFactory:" + TAG, "getModule" + name + "failed");
      return null;
    }

    // cache module wrapper
    LynxModuleWrapper moduleWrapper = new LynxModuleWrapper(name, module);
    moduleWrapper.setContextFinder(mContextFinder);
    mModulesByName.put(name, moduleWrapper);
    return moduleWrapper;
  }

  @Override
  public void destroy() {
    if (mModulesByName == null) {
      return;
    }
    for (LynxModuleWrapper wrapper : mModulesByName.values()) {
      wrapper.destroy();
    }
    mModulesByName.clear();
    mModulesByName = null;
  }

  @Type
  public int Type() {
    return Type.NORMAL;
  }
}
