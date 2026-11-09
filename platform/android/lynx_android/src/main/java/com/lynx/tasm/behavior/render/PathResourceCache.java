// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

// Accessed on the renderer's UI thread. Prepared tables strongly retain entries.
/** @hide */
final class PathResourceCache {
  interface GeometrySource {
    ByteBuffer getBuffer(int index, long key, int part);
  }

  static final class Table {
    static final Table EMPTY = new Table(new Entry[0][0]);
    private final Entry[][] entries;
    private HashMap<Integer, Entry> normalized;

    Table(Entry[][] entries) {
      this.entries = entries;
    }

    Entry get(int index, int rule) {
      return index >= 0 && index < entries.length && rule >= 0 && rule < 2 ? entries[index][rule]
                                                                           : null;
    }

    Entry atBounds(int itemIndex, Entry base, RectF bounds) {
      if (base.bounds.equals(bounds)) {
        if (normalized != null)
          normalized.remove(itemIndex);
        return base;
      }
      Entry entry = normalized == null ? null : normalized.get(itemIndex);
      if (entry != null && entry.bounds.equals(bounds))
        return entry;
      if (normalized == null)
        normalized = new HashMap<>();
      entry = base.atBounds(bounds);
      normalized.put(itemIndex, entry);
      return entry;
    }
  }

  static final class Entry {
    private final Path path;
    private final RectF bounds;
    private final boolean rect;
    private final float[] radii;
    private HashMap<BoundsKey, WeakReference<Entry>> normalized;
    private int normalizationCount;

    Entry(Path path, RectF bounds, boolean rect) {
      this(path, bounds, rect, null);
    }

    Entry(Path path, RectF bounds, boolean rect, float[] radii) {
      this.path = path;
      this.bounds = bounds;
      this.rect = rect;
      this.radii = radii;
    }

    boolean isPrimitive() {
      return rect || radii != null;
    }

    void getPlacedBounds(RectF out, float x, float y) {
      out.set(bounds);
      out.offset(x, y);
    }

    Entry atBounds(RectF localBounds) {
      if (bounds.equals(localBounds))
        return this;
      if (normalized == null)
        normalized = new HashMap<>();
      Iterator<WeakReference<Entry>> iterator = normalized.values().iterator();
      while (iterator.hasNext()) {
        if (iterator.next().get() == null)
          iterator.remove();
      }
      BoundsKey key = new BoundsKey(localBounds);
      WeakReference<Entry> weak = normalized.get(key);
      Entry entry = weak == null ? null : weak.get();
      if (entry == null) {
        Path variant = new Path();
        variant.setFillType(path.getFillType());
        if (rect)
          variant.addRect(localBounds, Path.Direction.CW);
        else
          variant.addRoundRect(localBounds, radii, Path.Direction.CW);
        entry = new Entry(variant, new RectF(localBounds), rect, radii);
        normalized.put(key, new WeakReference<>(entry));
        normalizationCount++;
      }
      return entry;
    }

    int getNormalizationCount() {
      return normalizationCount;
    }

    void draw(Canvas canvas, Paint paint, float x, float y) {
      canvas.save();
      canvas.translate(x, y);
      if (rect) {
        canvas.drawRect(bounds, paint);
      } else {
        canvas.drawPath(path, paint);
      }
      canvas.restore();
    }

    void clip(Canvas canvas, float x, float y, Matrix savedMatrix) {
      canvas.getMatrix(savedMatrix);
      canvas.translate(x, y);
      canvas.clipPath(path);
      canvas.setMatrix(savedMatrix);
    }
  }

  private static final class BoundsKey {
    final RectF bounds;

    BoundsKey(RectF bounds) {
      this.bounds = new RectF(bounds);
    }

    @Override
    public boolean equals(Object other) {
      return other instanceof BoundsKey && bounds.equals(((BoundsKey) other).bounds);
    }

    @Override
    public int hashCode() {
      return bounds.hashCode();
    }
  }

  private static final class Key {
    final long resource;
    final int rule;

    Key(long resource, int rule) {
      this.resource = resource;
      this.rule = rule;
    }

    @Override
    public boolean equals(Object other) {
      if (!(other instanceof Key))
        return false;
      Key key = (Key) other;
      return resource == key.resource && rule == key.rule;
    }

    @Override
    public int hashCode() {
      return 31 * (int) (resource ^ (resource >>> 32)) + rule;
    }
  }

  private final HashMap<Key, WeakReference<Entry>> cache = new HashMap<>();
  private int compilationCount;

  Table prepare(ByteBuffer keys, ByteBuffer items, GeometrySource source) {
    prune();
    if (keys == null || items == null || keys.capacity() % 8 != 0
        || items.capacity() % DisplayListApplier.DISPLAY_LIST_ITEM_SIZE != 0) {
      return Table.EMPTY;
    }
    keys = keys.duplicate().order(ByteOrder.nativeOrder());
    items = items.duplicate().order(ByteOrder.nativeOrder());
    int count = keys.capacity() / 8;
    if (count > items.capacity() / DisplayListApplier.DISPLAY_LIST_ITEM_SIZE) {
      return Table.EMPTY;
    }
    Entry[][] prepared = new Entry[count][2];
    for (int offset = 0; offset < items.capacity();
         offset += DisplayListApplier.DISPLAY_LIST_ITEM_SIZE) {
      int op = items.getInt(offset);
      if (op != DisplayListApplier.OP_FILL_PATH && op != DisplayListApplier.OP_CLIP_PATH)
        continue;
      int index = items.getInt(offset + 4);
      int rule = items.getInt(offset + 8);
      if (index < 0 || index >= count || rule < 0 || rule > 1 || prepared[index][rule] != null)
        continue;
      long resource = keys.getLong(index * 8);
      if (resource == 0)
        continue;
      Key key = new Key(resource, rule);
      WeakReference<Entry> weak = cache.get(key);
      Entry entry = weak == null ? null : weak.get();
      if (entry == null) {
        entry = decode(source.getBuffer(index, resource, 0), source.getBuffer(index, resource, 1),
            source.getBuffer(index, resource, 2), rule);
        if (entry != null) {
          cache.put(key, new WeakReference<>(entry));
          compilationCount++;
        }
      }
      prepared[index][rule] = entry;
    }
    return new Table(prepared);
  }

  void clear() {
    cache.clear();
  }

  int getCompilationCount() {
    return compilationCount;
  }

  private void prune() {
    Iterator<Map.Entry<Key, WeakReference<Entry>>> iterator = cache.entrySet().iterator();
    while (iterator.hasNext()) {
      if (iterator.next().getValue().get() == null)
        iterator.remove();
    }
  }

  private static boolean finite(float value) {
    return !Float.isNaN(value) && !Float.isInfinite(value);
  }

  static Entry decode(ByteBuffer header, ByteBuffer commands, ByteBuffer points, int rule) {
    if (header == null || header.capacity() != 60 || rule < 0 || rule > 1)
      return null;
    header = header.duplicate().order(ByteOrder.nativeOrder());
    int kind = header.getInt(0);
    int commandCount = header.getInt(4);
    int pointCount = header.getInt(8);
    float x = header.getFloat(12);
    float y = header.getFloat(16);
    float width = header.getFloat(20);
    float height = header.getFloat(24);
    if (!finite(x) || !finite(y) || !finite(width) || !finite(height) || width < 0 || height < 0
        || !finite(x + width) || !finite(y + height))
      return null;
    RectF bounds = new RectF(x, y, x + width, y + height);
    Path path = new Path();
    float[] radii = null;
    path.setFillType(rule == 0 ? Path.FillType.WINDING : Path.FillType.EVEN_ODD);
    if (kind == 0 || kind == 1) {
      if (commandCount != 0 || pointCount != 0)
        return null;
      if (kind == 0) {
        path.addRect(bounds, Path.Direction.CW);
      } else {
        radii = new float[8];
        for (int i = 0; i < 8; i++) {
          radii[i] = header.getFloat(28 + i * 4);
          if (!finite(radii[i]) || radii[i] < 0)
            return null;
        }
        path.addRoundRect(bounds, radii, Path.Direction.CW);
      }
    } else if (kind == 2) {
      if (pointCount < 0 || pointCount > (1 << 20) || commandCount < 0 || commandCount > (2 << 20))
        return null;
      if (commandCount == 0 && pointCount == 0)
        return new Entry(path, bounds, false);
      if (commands == null || points == null || commands.capacity() != commandCount * 4
          || points.capacity() != pointCount * 8)
        return null;
      commands = commands.duplicate().order(ByteOrder.nativeOrder());
      points = points.duplicate().order(ByteOrder.nativeOrder());
      for (int i = 0; i < pointCount; i++) {
        float px = points.getFloat(i * 8);
        float py = points.getFloat(i * 8 + 4);
        if (!finite(px) || !finite(py) || px < bounds.left || px > bounds.right || py < bounds.top
            || py > bounds.bottom)
          return null;
      }
      boolean inSubpath = false;
      int consumed = 0;
      for (int i = 0; i < commandCount; i++) {
        int verb = commands.getInt(i * 4);
        int take;
        switch (verb) {
          case 0:
            if (inSubpath)
              return null;
            inSubpath = true;
            take = 1;
            break;
          case 1:
          case 2:
          case 3:
            if (!inSubpath)
              return null;
            take = verb;
            break;
          case 4:
          case 5:
            if (!inSubpath)
              return null;
            if (verb == 4)
              path.close();
            inSubpath = false;
            take = 0;
            break;
          default:
            return null;
        }
        if (take > pointCount - consumed)
          return null;
        int at = consumed * 8;
        if (verb == 0)
          path.moveTo(points.getFloat(at), points.getFloat(at + 4));
        if (verb == 1)
          path.lineTo(points.getFloat(at), points.getFloat(at + 4));
        if (verb == 2) {
          path.quadTo(points.getFloat(at), points.getFloat(at + 4), points.getFloat(at + 8),
              points.getFloat(at + 12));
        }
        if (verb == 3) {
          path.cubicTo(points.getFloat(at), points.getFloat(at + 4), points.getFloat(at + 8),
              points.getFloat(at + 12), points.getFloat(at + 16), points.getFloat(at + 20));
        }
        consumed += take;
      }
      if (inSubpath || consumed != pointCount)
        return null;
    } else {
      return null;
    }
    return new Entry(path, bounds, kind == 0, radii);
  }
}
