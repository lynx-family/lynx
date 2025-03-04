// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.utils;

import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.ReadableMapKeySetIterator;
import com.lynx.react.bridge.mapbuffer.MapBuffer;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.LynxSubErrorCode;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.CSSPropertySetter;
import com.lynx.tasm.behavior.StylesDiffMap;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.utils.CallStackUtil;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public class PropsUpdater {
  // TODO merge two map
  private static final Map<Class<?>, LynxUISetter<?>> LYNX_UI_SETTER_MAP =
      new ConcurrentHashMap<>();
  private static final Map<Class<?>, ShadowNodeSetter<?>> SHADOW_NODE_SETTER_MAP =
      new ConcurrentHashMap<>();

  private static final Map<String, Settable> PRE_REGISTER_MAP = new HashMap<>();

  public static void registerSetter(Settable settable) {
    PRE_REGISTER_MAP.put(settable.getClass().getName(), settable);
  }

  public static void clear() {
    PropsSetterCache.clear();
    SHADOW_NODE_SETTER_MAP.clear();
    LYNX_UI_SETTER_MAP.clear();
  }

  public static void updateProps(LynxBaseUI lynxUI, StylesDiffMap props) {
    LynxUISetter<LynxBaseUI> setter = findLynxUISetter(lynxUI.getClass());
    ReadableMap propMap = props.mBackingMap;
    ReadableMapKeySetIterator iterator = propMap.keySetIterator();
    while (iterator.hasNextKey()) {
      String key = iterator.nextKey();
      try {
        setter.setProperty(lynxUI, key, props);
      } catch (Throwable e) {
        String errMsg =
            "Error while updating property '" + key + "' in ui '" + lynxUI.getTagName() + "': " + e;
        LynxError error = new LynxError(LynxSubErrorCode.E_CSS, errMsg, "", LynxError.LEVEL_ERROR);
        error.setCallStack(CallStackUtil.getStackTraceStringWithLineTrimmed(e));
        error.setUserDefineInfo(lynxUI.getPlatformCustomInfo());
        lynxUI.getLynxContext().handleLynxError(error);
      }
    }

    CSSPropertySetter.updateStyles(lynxUI, props.getStyleMap());
  }

  public static <T extends ShadowNode> void updateProps(T node, StylesDiffMap props) {
    ShadowNodeSetter<T> setter = findNodeSetter(node.getClass());
    ReadableMap propMap = props.mBackingMap;
    ReadableMapKeySetIterator iterator = propMap.keySetIterator();
    while (iterator.hasNextKey()) {
      String key = iterator.nextKey();
      setter.setProperty(node, key, props);
    }

    CSSPropertySetter.updateStyles(node, (MapBuffer) props.getStyleMap());
  }

  /* package */ static <T extends LynxBaseUI> LynxUISetter<T> findLynxUISetter(
      Class<? extends LynxBaseUI> lynxUIClass) {
    @SuppressWarnings("unchecked")
    LynxUISetter<T> setter = (LynxUISetter<T>) LYNX_UI_SETTER_MAP.get(lynxUIClass);
    if (setter == null) {
      setter = findGeneratedSetter(lynxUIClass);
      if (setter == null) {
        String log = "PropsSetter not generated for class: " + lynxUIClass.getName()
            + ". You should add module lynxProcessor";
        if (LynxEnv.inst().isCheckPropsSetter() && LynxEnv.inst().isLynxDebugEnabled()) {
          throw new IllegalStateException(log);
        }
        LLog.e("PropsUpdater", log);
        setter = new FallbackLynxUISetter<>(lynxUIClass);
      }
      LYNX_UI_SETTER_MAP.put(lynxUIClass, setter);
    }

    return setter;
  }

  /* package */ static <T extends ShadowNode> ShadowNodeSetter<T> findNodeSetter(
      Class<? extends ShadowNode> nodeClass) {
    @SuppressWarnings("unchecked")
    ShadowNodeSetter<T> setter = (ShadowNodeSetter<T>) SHADOW_NODE_SETTER_MAP.get(nodeClass);
    if (setter == null) {
      setter = findGeneratedSetter(nodeClass);
      if (setter == null) {
        setter = new FallbackShadowNodeSetter<>(nodeClass);
      }
      SHADOW_NODE_SETTER_MAP.put(nodeClass, setter);
    }

    return setter;
  }

  private static <T> T findGeneratedSetter(Class<?> cls) {
    String clsName = cls.getName() + "$$PropsSetter";
    // First find in pre register map
    Settable setter = PRE_REGISTER_MAP.get(clsName);
    if (setter != null) {
      return (T) setter;
    }
    //
    //    String log = "PropsSetter Map does not contains setter for " + cls.getName()
    //        + ". You should add auto-register.";
    //    if (LynxEnv.inst().isCheckPropsSetter() &&
    //    LynxEnv.inst().isDebug()) {
    //      throw new IllegalStateException(log);
    //    }
    //
    //    LLog.w("PropsUpdater", log);

    // Second try newInstance
    try {
      Class<?> setterClass = Class.forName(clsName);
      // noinspection unchecked
      return (T) setterClass.newInstance();
    } catch (ClassNotFoundException e) {
      return null;
    } catch (InstantiationException | IllegalAccessException e) {
      throw new RuntimeException("Unable to instantiate methods getter for " + clsName, e);
    }
  }

  private static class FallbackLynxUISetter<T extends LynxBaseUI> implements LynxUISetter<T> {
    private final Map<String, PropsSetterCache.PropSetter> mPropSetters;

    private FallbackLynxUISetter(Class<? extends LynxBaseUI> lynxUI) {
      mPropSetters = PropsSetterCache.getNativePropSettersForLynxUIClass(lynxUI);
    }

    @Override
    public void setProperty(LynxBaseUI ui, String name, StylesDiffMap props) {
      PropsSetterCache.PropSetter setter = mPropSetters.get(name);
      if (setter != null) {
        setter.updateLynxUIProp(ui, props);
      }
    }
  }

  private static class FallbackShadowNodeSetter<T extends ShadowNode>
      implements ShadowNodeSetter<T> {
    private final Map<String, PropsSetterCache.PropSetter> mPropSetters;

    private FallbackShadowNodeSetter(Class<? extends ShadowNode> shadowNodeClass) {
      mPropSetters = PropsSetterCache.getNativePropSettersForShadowNodeClass(shadowNodeClass);
    }

    @Override
    public void setProperty(ShadowNode node, String name, StylesDiffMap props) {
      PropsSetterCache.PropSetter setter = mPropSetters.get(name);
      if (setter != null) {
        setter.updateShadowNodeProp(node, props);
      }
    }
  }
}
