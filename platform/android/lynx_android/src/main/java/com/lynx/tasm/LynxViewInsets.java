// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import java.util.ArrayList;
import java.util.Collections;
import java.util.EnumMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * An immutable category update for one LynxView. Absent categories keep their previous state.
 * Coordinates are in screen space, in dp.
 */
public final class LynxViewInsets {
  @NonNull public final Map<Type, State> categories;

  private LynxViewInsets(Map<Type, State> categories) {
    EnumMap<Type, State> copy = new EnumMap<>(Type.class);
    copy.putAll(categories);
    this.categories = Collections.unmodifiableMap(copy);
  }

  @NonNull
  public static LynxViewInsets empty() {
    return new Builder().build();
  }

  LynxViewInsets merge(LynxViewInsets update) {
    EnumMap<Type, State> next = new EnumMap<>(Type.class);
    next.putAll(categories);
    next.putAll(update.categories);
    return new LynxViewInsets(next);
  }

  Map<String, Object> toMap() {
    Map<String, Object> result = new LinkedHashMap<>();
    for (Map.Entry<Type, State> entry : categories.entrySet()) {
      result.put(entry.getKey().key, entry.getValue().toMap());
    }
    return Collections.unmodifiableMap(result);
  }

  public enum Type {
    STATUS_BARS("statusBars"),
    NAVIGATION_BARS("navigationBars"),
    CAPTION_BAR("captionBar"),
    SYSTEM_BARS("systemBars"),
    SYSTEM_OVERLAYS("systemOverlays"),
    IME("ime"),
    DISPLAY_CUTOUT("displayCutout"),
    WATERFALL("waterfall"),
    ROUNDED_CORNERS("roundedCorners"),
    SYSTEM_GESTURES("systemGestures"),
    MANDATORY_SYSTEM_GESTURES("mandatorySystemGestures"),
    TAPPABLE_ELEMENT("tappableElement");

    private final String key;
    Type(String key) {
      this.key = key;
    }
  }

  public static final class Builder {
    private final EnumMap<Type, State> categories = new EnumMap<>(Type.class);

    /** Null omits the category from this update; it does not clear the View's stored value. */
    @NonNull
    public Builder setInset(@NonNull Type type, @Nullable State state) {
      if (type == null)
        throw new IllegalArgumentException("Inset type must not be null");
      if (state == null) {
        categories.remove(type);
        return this;
      }
      if (type == Type.ROUNDED_CORNERS && !(state.geometry instanceof Corners)) {
        throw new IllegalArgumentException("Rounded corners require corner geometry");
      }
      if (type != Type.ROUNDED_CORNERS && type != Type.DISPLAY_CUTOUT
          && !(state.geometry instanceof Edges)) {
        throw new IllegalArgumentException("This inset category requires edge geometry");
      }
      if (type == Type.DISPLAY_CUTOUT && state.geometry instanceof Corners) {
        throw new IllegalArgumentException("Display cutouts require edges or rectangles");
      }
      categories.put(type, state);
      return this;
    }

    @NonNull
    public LynxViewInsets build() {
      return new LynxViewInsets(categories);
    }
  }

  /** Source state is independent of geometry. */
  public static final class State {
    @NonNull public final Geometry geometry;
    @Nullable public final Boolean visible;

    public State(@NonNull Geometry geometry, @Nullable Boolean visible) {
      if (geometry == null)
        throw new IllegalArgumentException("Inset geometry must not be null");
      this.geometry = geometry;
      this.visible = visible;
    }

    private Map<String, Object> toMap() {
      Map<String, Object> result = new LinkedHashMap<>();
      result.put("geometry", geometry.toMap());
      if (visible != null)
        result.put("visible", visible);
      return Collections.unmodifiableMap(result);
    }
  }

  public abstract static class Geometry {
    private Geometry() {}
    abstract Map<String, Object> toMap();
  }

  public static final class Edges extends Geometry {
    public final float left;
    public final float top;
    public final float right;
    public final float bottom;

    public Edges(float left, float top, float right, float bottom) {
      this.left = length(left);
      this.top = length(top);
      this.right = length(right);
      this.bottom = length(bottom);
    }

    @Override
    Map<String, Object> toMap() {
      return map("type", "edges", "left", left, "top", top, "right", right, "bottom", bottom);
    }
  }

  public static final class Rect {
    public final float x;
    public final float y;
    public final float width;
    public final float height;

    public Rect(float x, float y, float width, float height) {
      this.x = coordinate(x);
      this.y = coordinate(y);
      this.width = length(width);
      this.height = length(height);
    }

    private Map<String, Object> toMap() {
      return map("x", x, "y", y, "width", width, "height", height);
    }
  }

  public static final class Rects extends Geometry {
    @NonNull public final List<Rect> rects;

    public Rects(@NonNull List<Rect> rects) {
      if (rects == null)
        throw new IllegalArgumentException("Rectangle list must not be null");
      List<Rect> copy = new ArrayList<>(rects);
      for (Rect rect : copy) {
        if (rect == null)
          throw new IllegalArgumentException("Rectangle must not be null");
      }
      this.rects = Collections.unmodifiableList(copy);
    }

    @Override
    Map<String, Object> toMap() {
      List<Object> values = new ArrayList<>();
      for (Rect rect : rects) values.add(rect.toMap());
      return map("type", "rects", "rects", Collections.unmodifiableList(values));
    }
  }

  public static final class Corner {
    public final float centerX;
    public final float centerY;
    public final float radius;

    public Corner(float centerX, float centerY, float radius) {
      this.centerX = coordinate(centerX);
      this.centerY = coordinate(centerY);
      this.radius = length(radius);
    }

    private Map<String, Object> toMap() {
      return map("centerX", centerX, "centerY", centerY, "radius", radius);
    }
  }

  public static final class Corners extends Geometry {
    @Nullable public final Corner topLeft;
    @Nullable public final Corner topRight;
    @Nullable public final Corner bottomRight;
    @Nullable public final Corner bottomLeft;

    public Corners(@Nullable Corner topLeft, @Nullable Corner topRight,
        @Nullable Corner bottomRight, @Nullable Corner bottomLeft) {
      this.topLeft = topLeft;
      this.topRight = topRight;
      this.bottomRight = bottomRight;
      this.bottomLeft = bottomLeft;
    }

    @Override
    Map<String, Object> toMap() {
      Map<String, Object> result = new LinkedHashMap<>();
      result.put("type", "corners");
      if (topLeft != null)
        result.put("topLeft", topLeft.toMap());
      if (topRight != null)
        result.put("topRight", topRight.toMap());
      if (bottomRight != null)
        result.put("bottomRight", bottomRight.toMap());
      if (bottomLeft != null)
        result.put("bottomLeft", bottomLeft.toMap());
      return Collections.unmodifiableMap(result);
    }
  }

  private static float coordinate(float value) {
    if (Float.isNaN(value) || Float.isInfinite(value)) {
      throw new IllegalArgumentException("Inset coordinates must be finite");
    }
    return value;
  }

  private static float length(float value) {
    if (coordinate(value) < 0) {
      throw new IllegalArgumentException("Inset lengths must be nonnegative");
    }
    return value;
  }

  private static Map<String, Object> map(Object... entries) {
    Map<String, Object> result = new LinkedHashMap<>();
    for (int i = 0; i < entries.length; i += 2) {
      result.put((String) entries[i], entries[i + 1]);
    }
    return Collections.unmodifiableMap(result);
  }
}
