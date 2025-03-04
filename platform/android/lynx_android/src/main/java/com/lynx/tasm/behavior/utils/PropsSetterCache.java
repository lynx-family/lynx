// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.utils;

import androidx.annotation.Nullable;
import com.lynx.react.bridge.Dynamic;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.LynxPropGroup;
import com.lynx.tasm.behavior.StylesDiffMap;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

/**
 * This class is responsible for holding view manager property setters and is used in a process of
 * updating views with the new properties set in JS.
 */
/*package*/ class PropsSetterCache {
  private static final Map<Class, Map<String, PropSetter>> CLASS_PROPS_CACHE =
      new ConcurrentHashMap<>();
  private static final Map<String, PropSetter> EMPTY_PROPS_MAP = new HashMap<>();

  public static void clear() {
    CLASS_PROPS_CACHE.clear();
    EMPTY_PROPS_MAP.clear();
  }

  /*package*/ static abstract class PropSetter {
    protected final String mPropName;
    protected final String mPropType;
    protected final Method mSetter;
    protected final @Nullable Integer mIndex; /* non-null only for group setters */

    private PropSetter(LynxProp prop, String defaultType, Method setter) {
      mPropName = prop.name();
      mPropType =
          LynxProp.USE_DEFAULT_TYPE.equals(prop.customType()) ? defaultType : prop.customType();
      mSetter = setter;
      mIndex = null;
    }

    private PropSetter(LynxPropGroup prop, String defaultType, Method setter, int index) {
      mPropName = prop.names()[index];
      mPropType = LynxPropGroup.USE_DEFAULT_TYPE.equals(prop.customType()) ? defaultType
                                                                           : prop.customType();
      mSetter = setter;
      mIndex = index;
    }

    public String getPropName() {
      return mPropName;
    }

    public String getPropType() {
      return mPropType;
    }

    public void updateShadowNodeProp(ShadowNode nodeToUpdate, StylesDiffMap props) {
      try {
        if (mIndex == null) {
          Object[] shadowArgs = {extractProperty(props)};
          mSetter.invoke(nodeToUpdate, shadowArgs);
        } else {
          Object[] shadowGroupArgs = {mIndex, extractProperty(props)};
          mSetter.invoke(nodeToUpdate, shadowGroupArgs);
        }
      } catch (Throwable t) {
        RuntimeException r = new RuntimeException("Fallback setter, error while updating property '"
                + mPropName + "' in shadow node of type: " + nodeToUpdate.getTagName() + ":" + t,
            t);
        r.setStackTrace(t.getStackTrace());
        throw r;
      }
    }

    public void updateLynxUIProp(LynxBaseUI ui, StylesDiffMap props) {
      try {
        if (mIndex == null) {
          Object[] uiArgs = {extractProperty(props)};
          mSetter.invoke(ui, uiArgs);
        } else {
          Object[] uiGroupArgs = {mIndex, extractProperty(props)};
          mSetter.invoke(ui, uiGroupArgs);
        }
      } catch (Throwable t) {
        RuntimeException r = new RuntimeException("Fallback setter, error while updating property '"
                + mPropName + "' in Lynx UI of type: " + ui.getClass() + ":" + t,
            t);
        r.setStackTrace(t.getStackTrace());
        throw r;
      }
    }

    protected abstract @Nullable Object extractProperty(StylesDiffMap props);
  }

  private static class DynamicPropSetter extends PropSetter {
    public DynamicPropSetter(LynxProp prop, Method setter) {
      super(prop, "mixed", setter);
    }

    public DynamicPropSetter(LynxPropGroup prop, Method setter, int index) {
      super(prop, "mixed", setter, index);
    }

    @Override
    protected Object extractProperty(StylesDiffMap props) {
      return props.getDynamic(mPropName);
    }
  }

  private static class IntPropSetter extends PropSetter {
    private final int mDefaultValue;

    public IntPropSetter(LynxProp prop, Method setter, int defaultValue) {
      super(prop, "number", setter);
      mDefaultValue = defaultValue;
    }

    public IntPropSetter(LynxPropGroup prop, Method setter, int index, int defaultValue) {
      super(prop, "number", setter, index);
      mDefaultValue = defaultValue;
    }

    @Override
    protected Object extractProperty(StylesDiffMap props) {
      return props.getInt(mPropName, mDefaultValue);
    }
  }

  private static class DoublePropSetter extends PropSetter {
    private final double mDefaultValue;

    public DoublePropSetter(LynxProp prop, Method setter, double defaultValue) {
      super(prop, "number", setter);
      mDefaultValue = defaultValue;
    }

    public DoublePropSetter(LynxPropGroup prop, Method setter, int index, double defaultValue) {
      super(prop, "number", setter, index);
      mDefaultValue = defaultValue;
    }

    @Override
    protected Object extractProperty(StylesDiffMap props) {
      return props.getDouble(mPropName, mDefaultValue);
    }
  }

  private static class BooleanPropSetter extends PropSetter {
    private final boolean mDefaultValue;

    public BooleanPropSetter(LynxProp prop, Method setter, boolean defaultValue) {
      super(prop, "boolean", setter);
      mDefaultValue = defaultValue;
    }

    @Override
    protected Object extractProperty(StylesDiffMap props) {
      return props.getBoolean(mPropName, mDefaultValue) ? Boolean.TRUE : Boolean.FALSE;
    }
  }

  private static class FloatPropSetter extends PropSetter {
    private final float mDefaultValue;

    public FloatPropSetter(LynxProp prop, Method setter, float defaultValue) {
      super(prop, "number", setter);
      mDefaultValue = defaultValue;
    }

    public FloatPropSetter(LynxPropGroup prop, Method setter, int index, float defaultValue) {
      super(prop, "number", setter, index);
      mDefaultValue = defaultValue;
    }

    @Override
    protected Object extractProperty(StylesDiffMap props) {
      return props.getFloat(mPropName, mDefaultValue);
    }
  }

  private static class ArrayPropSetter extends PropSetter {
    public ArrayPropSetter(LynxProp prop, Method setter) {
      super(prop, "Array", setter);
    }

    public ArrayPropSetter(LynxPropGroup prop, Method setter, int index) {
      super(prop, "Array", setter, index);
    }

    @Override
    protected @Nullable Object extractProperty(StylesDiffMap props) {
      return props.getArray(mPropName);
    }
  }

  private static class MapPropSetter extends PropSetter {
    public MapPropSetter(LynxProp prop, Method setter) {
      super(prop, "Map", setter);
    }

    @Override
    protected @Nullable Object extractProperty(StylesDiffMap props) {
      return props.getMap(mPropName);
    }
  }

  private static class StringPropSetter extends PropSetter {
    public StringPropSetter(LynxProp prop, Method setter) {
      super(prop, "String", setter);
    }
    public StringPropSetter(LynxPropGroup prop, Method setter, int index) {
      super(prop, "String", setter, index);
    }

    @Override
    protected @Nullable Object extractProperty(StylesDiffMap props) {
      return props.getString(mPropName);
    }
  }

  private static class BoxedBooleanPropSetter extends PropSetter {
    public BoxedBooleanPropSetter(LynxProp prop, Method setter) {
      super(prop, "boolean", setter);
    }

    @Override
    protected @Nullable Object extractProperty(StylesDiffMap props) {
      if (!props.isNull(mPropName)) {
        return props.getBoolean(mPropName, /* ignored */ false) ? Boolean.TRUE : Boolean.FALSE;
      }
      return null;
    }
  }

  private static class BoxedIntPropSetter extends PropSetter {
    public BoxedIntPropSetter(LynxProp prop, Method setter) {
      super(prop, "number", setter);
    }

    public BoxedIntPropSetter(LynxPropGroup prop, Method setter, int index) {
      super(prop, "number", setter, index);
    }

    @Override
    protected @Nullable Object extractProperty(StylesDiffMap props) {
      if (!props.isNull(mPropName)) {
        return props.getInt(mPropName, /* ignored */ 0);
      }
      return null;
    }
  }

  /*package*/ static Map<String, PropSetter> getNativePropSettersForLynxUIClass(
      Class<? extends LynxBaseUI> cls) {
    if (cls == null) {
      return EMPTY_PROPS_MAP;
    }
    Map<String, PropSetter> props = CLASS_PROPS_CACHE.get(cls);
    if (props != null) {
      return props;
    }
    // This is to include all the setters from parent classes. Once calculated the result will be
    // stored in CLASS_PROPS_CACHE so that we only scan for @ReactProp annotations once per class.
    props = new HashMap<>();
    props.putAll(getNativePropSettersForLynxUIClass(
        cls == LynxBaseUI.class ? null : (Class<? extends LynxBaseUI>) cls.getSuperclass()));
    extractPropSettersFromLynxUIClassDefinition(cls, props);
    CLASS_PROPS_CACHE.put(cls, props);
    return props;
  }

  /**
   * Returns map from property name to setter instances for all the property setters annotated with
   * {@link LynxProp} (or {@link LynxPropGroup} in the given {@link ShadowNode} subclass plus
   * all the setters declared by its parent classes up to {@link ShadowNode} which is treated
   * as a base class.
   */
  /*package*/ static Map<String, PropSetter> getNativePropSettersForShadowNodeClass(
      Class<? extends ShadowNode> cls) {
    if (cls == null) {
      return EMPTY_PROPS_MAP;
    }
    Map<String, PropSetter> props = CLASS_PROPS_CACHE.get(cls);
    if (props != null) {
      return props;
    }
    // This is to include all the setters from parent classes up to ShadowNode class
    props = new HashMap<>();
    props.putAll(getNativePropSettersForShadowNodeClass(
        cls == ShadowNode.class ? null : (Class<? extends ShadowNode>) cls.getSuperclass()));
    extractPropSettersFromShadowNodeClassDefinition(cls, props);
    CLASS_PROPS_CACHE.put(cls, props);
    return props;
  }

  private static PropSetter createPropSetter(
      LynxProp annotation, Method method, Class<?> propTypeClass) {
    if (propTypeClass == Dynamic.class) {
      return new DynamicPropSetter(annotation, method);
    } else if (propTypeClass == boolean.class) {
      return new BooleanPropSetter(annotation, method, annotation.defaultBoolean());
    } else if (propTypeClass == int.class) {
      return new IntPropSetter(annotation, method, annotation.defaultInt());
    } else if (propTypeClass == float.class) {
      return new FloatPropSetter(annotation, method, annotation.defaultFloat());
    } else if (propTypeClass == double.class) {
      return new DoublePropSetter(annotation, method, annotation.defaultDouble());
    } else if (propTypeClass == String.class) {
      return new StringPropSetter(annotation, method);
    } else if (propTypeClass == Boolean.class) {
      return new BoxedBooleanPropSetter(annotation, method);
    } else if (propTypeClass == Integer.class) {
      return new BoxedIntPropSetter(annotation, method);
    } else if (propTypeClass == ReadableArray.class) {
      return new ArrayPropSetter(annotation, method);
    } else if (propTypeClass == ReadableMap.class) {
      return new MapPropSetter(annotation, method);
    } else {
      throw new RuntimeException("Unrecognized type: " + propTypeClass
          + " for method: " + method.getDeclaringClass().getName() + "#" + method.getName());
    }
  }

  private static void createPropSetters(LynxPropGroup annotation, Method method,
      Class<?> propTypeClass, Map<String, PropSetter> props) {
    String[] names = annotation.names();
    if (propTypeClass == Dynamic.class) {
      for (int i = 0; i < names.length; i++) {
        props.put(names[i], new DynamicPropSetter(annotation, method, i));
      }
    } else if (propTypeClass == int.class) {
      for (int i = 0; i < names.length; i++) {
        props.put(names[i], new IntPropSetter(annotation, method, i, annotation.defaultInt()));
      }
    } else if (propTypeClass == float.class) {
      for (int i = 0; i < names.length; i++) {
        props.put(names[i], new FloatPropSetter(annotation, method, i, annotation.defaultFloat()));
      }
    } else if (propTypeClass == double.class) {
      for (int i = 0; i < names.length; i++) {
        props.put(
            names[i], new DoublePropSetter(annotation, method, i, annotation.defaultDouble()));
      }
    } else if (propTypeClass == Integer.class) {
      for (int i = 0; i < names.length; i++) {
        props.put(names[i], new BoxedIntPropSetter(annotation, method, i));
      }
    } else if (propTypeClass == String.class) {
      for (int i = 0; i < names.length; i++) {
        props.put(names[i], new StringPropSetter(annotation, method, i));
      }
    } else if (propTypeClass == ReadableArray.class) {
      for (int i = 0; i < names.length; i++) {
        props.put(names[i], new ArrayPropSetter(annotation, method, i));
      }
    } else {
      throw new RuntimeException("Unrecognized type: " + propTypeClass
          + " for method: " + method.getDeclaringClass().getName() + "#" + method.getName());
    }
  }

  private static void extractPropSettersFromLynxUIClassDefinition(
      Class<? extends LynxBaseUI> cls, Map<String, PropSetter> props) {
    for (Method method : cls.getDeclaredMethods()) {
      LynxProp annotation = method.getAnnotation(LynxProp.class);
      if (annotation != null) {
        Class<?>[] paramTypes = method.getParameterTypes();
        if (paramTypes.length != 1) {
          throw new RuntimeException(
              "Wrong number of args for prop setter: " + cls.getName() + "#" + method.getName());
        }
        props.put(annotation.name(), createPropSetter(annotation, method, paramTypes[0]));
      }

      LynxPropGroup groupAnnotation = method.getAnnotation(LynxPropGroup.class);
      if (groupAnnotation != null) {
        Class<?>[] paramTypes = method.getParameterTypes();
        if (paramTypes.length != 2) {
          throw new RuntimeException("Wrong number of args for group prop setter: " + cls.getName()
              + "#" + method.getName());
        }
        if (paramTypes[0] != int.class) {
          throw new RuntimeException("Second argument should be property index: " + cls.getName()
              + "#" + method.getName());
        }
        createPropSetters(groupAnnotation, method, paramTypes[1], props);
      }
    }
  }

  private static void extractPropSettersFromShadowNodeClassDefinition(
      Class<? extends ShadowNode> cls, Map<String, PropSetter> props) {
    for (Method method : cls.getDeclaredMethods()) {
      LynxProp annotation = method.getAnnotation(LynxProp.class);
      if (annotation != null) {
        Class<?>[] paramTypes = method.getParameterTypes();
        if (paramTypes.length != 1) {
          throw new RuntimeException(
              "Wrong number of args for prop setter: " + cls.getName() + "#" + method.getName());
        }
        props.put(annotation.name(), createPropSetter(annotation, method, paramTypes[0]));
      }

      LynxPropGroup groupAnnotation = method.getAnnotation(LynxPropGroup.class);
      if (groupAnnotation != null) {
        Class<?>[] paramTypes = method.getParameterTypes();
        if (paramTypes.length != 2) {
          throw new RuntimeException("Wrong number of args for group prop setter: " + cls.getName()
              + "#" + method.getName());
        }
        if (paramTypes[0] != int.class) {
          throw new RuntimeException("Second argument should be property index: " + cls.getName()
              + "#" + method.getName());
        }
        createPropSetters(groupAnnotation, method, paramTypes[1], props);
      }
    }
  }
}
