// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.jsbridge;

import android.content.Context;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxViewClient;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public class LynxModuleFactory {
  private static final String TAG = "LynxModuleFactory";
  // When we use LynxBackgroundRuntime Standalone to create LynxView, JS Thread may
  // access LynxModule meanwhile new modules are registered during LynxView creation.
  // We use `ConcurrentHashMap` instead of `Map` here, because `putIfAbsent` on
  // `ConcurrentHashMap` requires API level 1, but on `Map ` requires API level 24.
  private final ConcurrentHashMap<String, ParamWrapper> mWrappers;
  private Map<String, LynxModuleWrapper> mModulesByName;
  private WeakReference<Context> mWeakContext;
  private LynxViewClient mLynxViewClient;
  private long mNativePtr = 0;
  private boolean mIsLynxViewDestroying = false;
  private boolean mHasDestroyed = false;
  private Object mLynxModuleExtraData;

  public LynxModuleFactory(@NonNull Context context) {
    mWrappers = new ConcurrentHashMap<>();
    setContext(context);
  }

  public void setContext(@NonNull Context context) {
    if (context instanceof LynxContext) {
      mLynxViewClient = ((LynxContext) context).getLynxViewClient();
    }
    mWeakContext = new WeakReference<>(context);
  }

  private Map<String, ParamWrapper> getWrappers() {
    return mWrappers;
  }

  public void setLynxModuleExtraData(Object data) {
    mLynxModuleExtraData = data;
  }

  public void registerModule(
      @NonNull String name, @NonNull Class<? extends LynxModule> module, @Nullable Object param) {
    ParamWrapper wrapper = new ParamWrapper();
    wrapper.setName(name);
    wrapper.setModuleClass(module);
    wrapper.setParam(param);

    ParamWrapper oldWrapper = mWrappers.get(name);
    if (oldWrapper != null) {
      LLog.e("LynxModuleFactory",
          "Duplicated LynxModule For Name: " + name + ", " + oldWrapper + " will be override");
    }
    mWrappers.put(name, wrapper);
    LLog.v("LynxModuleFactory", "registered module with name: " + name + " class" + module);
  }

  public void addModuleParamWrapper(List<ParamWrapper> wrappers) {
    if (wrappers == null || wrappers.size() == 0) {
      return;
    }
    for (ParamWrapper w : wrappers) {
      String name = w.getName();
      ParamWrapper oldWrapper = this.mWrappers.get(name);
      if (oldWrapper != null) {
        LLog.e("LynxModuleFactory",
            "Duplicated LynxModule For Name: " + name + ", " + oldWrapper + " will be override");
      }
      this.mWrappers.put(name, w);
    }
  }

  // Only used in LynxBackgroundRuntime Standalone to create LynxView, we already register
  // some modules in RuntimeOptions and we don't want the modules on LynxViewBuilder overwrite it.
  public void addModuleParamWrapperIfAbsent(List<ParamWrapper> wrappers) {
    if (wrappers == null || wrappers.size() == 0) {
      return;
    }
    for (ParamWrapper w : wrappers) {
      String name = w.getName();
      if (this.mWrappers.containsKey(name)) {
        LLog.w(TAG, "Duplicated LynxModule For Name: " + name + ", will be ignored");
      }
      this.mWrappers.putIfAbsent(name, w);
    }
  }

  private void getModuleExceptionReport(Exception e) {
    LLog.e("LynxModuleFactory", "get Module failed" + e);
  }

  public LynxModuleWrapper getModule(String name) {
    if (name == null) {
      LLog.e("LynxModuleFactory", "getModule failed, name is null");
      return null;
    }
    if (mModulesByName == null) {
      mModulesByName = new HashMap<>();
    }
    if (mModulesByName.get(name) != null) {
      return mModulesByName.get(name);
    }

    ParamWrapper wrapper = mWrappers.get(name);
    if (wrapper == null) {
      wrapper = LynxEnv.inst().getModuleFactory().getWrappers().get(name);
      if (wrapper == null) {
        return null;
      }
    }
    Class<? extends LynxModule> clazz = wrapper.getModuleClass();
    LynxModule module = null;
    try {
      boolean isLynxContxtBaseModule = LynxContextModule.class.isAssignableFrom(clazz);
      Context context = mWeakContext.get();
      if (context == null) {
        LLog.e(TAG, clazz.getCanonicalName() + " called with Null context");
        return null;
      }
      if (isLynxContxtBaseModule) { // LynxContextModule
        boolean isLynxContext = context instanceof LynxContext;
        if (!isLynxContext) {
          throw new Exception(clazz.getCanonicalName() + " must be created with LynxContext");
        }

        if (wrapper.getParam() == null) {
          for (Constructor<?> ctor : clazz.getConstructors()) {
            Class[] types = ctor.getParameterTypes();
            if (types.length == 1 && LynxContext.class.equals(types[0])) {
              module = (LynxModule) ctor.newInstance((LynxContext) context);
              break;
            } else if (types.length == 2 && LynxContext.class.equals(types[0])
                && Object.class.equals(types[1])) {
              module = (LynxModule) ctor.newInstance((LynxContext) context, null);
              break;
            }
          }
        } else {
          Constructor<?> ctor = clazz.getConstructor(LynxContext.class, Object.class);
          module = (LynxModule) ctor.newInstance((LynxContext) context, wrapper.getParam());
        }
      } else { // LynxModule
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
      }
    } catch (InstantiationException e) {
      getModuleExceptionReport(e);
    } catch (IllegalAccessException e) {
      getModuleExceptionReport(e);
    } catch (NoSuchMethodException e) {
      getModuleExceptionReport(e);
    } catch (InvocationTargetException e) {
      getModuleExceptionReport(e);
      LLog.e("LynxModuleFactory", "get TargetException " + e.getTargetException());
    } catch (Exception e) {
      getModuleExceptionReport(e);
    }
    if (module == null) {
      LLog.v("LynxModuleFactory", "getModule" + name + "failed");
      return null;
    }
    module.setExtraData(mLynxModuleExtraData);
    LynxModuleWrapper moduleWrapper = new LynxModuleWrapper(name, module);

    mModulesByName.put(name, moduleWrapper);
    return moduleWrapper;
  }

  public void markLynxViewIsDestroying() {
    mIsLynxViewDestroying = true;
  }

  public void retainJniObject() {
    if (!nativeRetainJniObject(mNativePtr)) {
      LLog.e("LynxModuleFactory", "LynxModuleFactory try to retainJniObject failed");
      destroy();
    }
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public long getNativePtr() {
    return mNativePtr;
  }

  @CalledByNative
  private LynxModuleWrapper moduleWrapperForName(String name) {
    LynxModuleWrapper module = getModule(name);
    return module;
  }

  @CalledByNative
  private void setNativePtr(long nativePtr) {
    mNativePtr = nativePtr;
  }

  @CalledByNative
  public void destroy() {
    if (mHasDestroyed) {
      return;
    }
    mHasDestroyed = true;
    if (mModulesByName != null) {
      for (LynxModuleWrapper wrapper : mModulesByName.values()) {
        wrapper.destroy();
      }
    }

    if (mIsLynxViewDestroying) {
      UIThreadUtils.runOnUiThread(new Runnable() {
        @Override
        public void run() {
          if (mLynxViewClient != null) {
            LLog.i("LynxModuleFactory", "lynx invoke onLynxViewAndJSRuntimeDestroy");
            mLynxViewClient.onLynxViewAndJSRuntimeDestroy();
          }
        }
      });
    }
    mNativePtr = 0;
    mModulesByName = null;
    mWrappers.clear();
  }

  private native boolean nativeRetainJniObject(long nativePtr);
}
