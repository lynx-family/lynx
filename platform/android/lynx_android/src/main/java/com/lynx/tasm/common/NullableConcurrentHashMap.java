// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.common;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import java.util.Collection;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

// TODO: 2020/8/27 Special handling for NULL
public class NullableConcurrentHashMap<K, V> extends ConcurrentHashMap<K, V> {
  private static final Object NULL = new Object();

  @Nullable
  @Override
  public V put(@NonNull K key, @NonNull V value) {
    V result = super.put((K) (key == null ? NULL : key), (V) (value == null ? NULL : value));
    if (result == NULL) {
      result = null;
    }
    return result;
  }

  @Override
  public void putAll(@NonNull Map<? extends K, ? extends V> m) {
    if (m == null) {
      return;
    }
    for (Entry<? extends K, ? extends V> entry : m.entrySet()) {
      put(entry.getKey(), entry.getValue());
    }
  }

  @Nullable
  @Override
  public V get(@NonNull Object key) {
    V v = super.get(key == null ? NULL : key);
    return v == NULL ? null : v;
  }

  @NonNull
  @Override
  public Set<Entry<K, V>> entrySet() {
    final Set<Entry<K, V>> entrySet = super.entrySet();
    return new Set<Entry<K, V>>() {
      @Override
      public int size() {
        return entrySet.size();
      }

      @Override
      public boolean isEmpty() {
        return entrySet.isEmpty();
      }

      @Override
      public boolean contains(@Nullable Object o) {
        return entrySet.contains(o == null ? NULL : o);
      }

      @NonNull
      @Override
      public Iterator<Entry<K, V>> iterator() {
        final Iterator<Entry<K, V>> iterator = entrySet.iterator();
        return new Iterator<Entry<K, V>>() {
          @Override
          public boolean hasNext() {
            return iterator.hasNext();
          }

          @Override
          public Entry<K, V> next() {
            final Entry<K, V> entry = iterator.next();
            return new Entry<K, V>() {
              @Override
              public K getKey() {
                K k = entry.getKey();
                return k == NULL ? null : k;
              }

              @Override
              public V getValue() {
                V v = entry.getValue();
                return v == NULL ? null : v;
              }

              @Override
              public V setValue(V value) {
                return entry.setValue(value == null ? (V) NULL : value);
              }
            };
          }
        };
      }

      @NonNull
      @Override
      public Object[] toArray() {
        Object[] objects = entrySet.toArray();
        for (int i = 0; i < objects.length; i++) {
          if (objects[i] == NULL) {
            objects[i] = null;
          }
        }
        return objects;
      }

      @NonNull
      @Override
      public <T> T[] toArray(@NonNull T[] a) {
        return entrySet.toArray(a);
      }

      @Override
      public boolean add(Entry<K, V> kvEntry) {
        return entrySet.add(kvEntry);
      }

      @Override
      public boolean remove(@Nullable Object o) {
        return entrySet.remove(o);
      }

      @Override
      public boolean containsAll(@NonNull Collection<?> c) {
        return entrySet.containsAll(c);
      }

      @Override
      public boolean addAll(@NonNull Collection<? extends Entry<K, V>> c) {
        return entrySet.addAll(c);
      }

      @Override
      public boolean retainAll(@NonNull Collection<?> c) {
        return entrySet.retainAll(c);
      }

      @Override
      public boolean removeAll(@NonNull Collection<?> c) {
        return entrySet.retainAll(c);
      }

      @Override
      public void clear() {
        entrySet.clear();
      }
    };
  }
}
