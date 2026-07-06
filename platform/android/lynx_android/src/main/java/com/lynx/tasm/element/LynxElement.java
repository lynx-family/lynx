// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.element;

import android.view.View;
import androidx.annotation.Nullable;
import com.lynx.tasm.utils.UIThreadUtils;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public final class LynxElement {
  private final LynxElementOwner mOwner;
  private final int mSign;

  /**
   * @apidoc
   *
   * @brief Callback used by LynxElement APIs.
   */
  public interface Callback<T> extends LynxElementCallback<T> {
    @Override void onResult(@Nullable T result);
  }

  LynxElement(@Nullable LynxElementOwner owner, int sign) {
    mOwner = owner;
    mSign = sign;
  }

  public void findNodeById(String id, LynxElementCallback<LynxElement> callback) {
    if (callback == null) {
      return;
    }
    if (mOwner == null) {
      callback.onResult(null);
      return;
    }
    mOwner.query(mSign, LynxElementOwner.QUERY_FIND_BY_ID, id,
        result
        -> callback.onResult(mOwner.element(
            result instanceof Integer ? (Integer) result : LynxElementOwner.INVALID_SIGN)));
  }

  public void findNodeByFilter(
      LynxElementFilter filter, LynxElementCallback<LynxElement> callback) {
    if (callback == null) {
      return;
    }
    findNodesByFilter(filter, result -> callback.onResult(result.isEmpty() ? null : result.get(0)));
  }

  public void findNodesByFilter(
      LynxElementFilter filter, LynxElementCallback<List<LynxElement>> callback) {
    if (callback == null) {
      return;
    }
    List<LynxElement> matches = new ArrayList<>();
    collectMatches(this, filter, matches, callback);
  }

  private static void collectMatches(LynxElement current, LynxElementFilter filter,
      List<LynxElement> matches, LynxElementCallback<List<LynxElement>> callback) {
    if (filter != null && filter.accept(current)) {
      matches.add(current);
    }
    current.getChildren(children -> collectChildren(children, 0, filter, matches, callback));
  }

  private static void collectChildren(List<LynxElement> children, int index,
      LynxElementFilter filter, List<LynxElement> matches,
      LynxElementCallback<List<LynxElement>> callback) {
    if (index >= children.size()) {
      callback.onResult(matches);
      return;
    }
    collectMatches(children.get(index), filter, matches,
        ignored -> collectChildren(children, index + 1, filter, matches, callback));
  }

  public void getStatus(LynxElementCallback<LynxElementStatus> callback) {
    if (callback == null) {
      return;
    }
    if (mOwner == null) {
      callback.onResult(LynxElementStatus.DETACHED);
      return;
    }
    mOwner.query(mSign, LynxElementOwner.QUERY_STATUS, null,
        result
        -> callback.onResult(
            Boolean.TRUE.equals(result) ? LynxElementStatus.ALIVE : LynxElementStatus.DETACHED));
  }

  public void getSign(LynxElementCallback<Integer> callback) {
    if (callback != null) {
      callback.onResult(mSign);
    }
  }

  public void getTagName(LynxElementCallback<String> callback) {
    queryIdentityString("tagName", callback);
  }

  public void getId(LynxElementCallback<String> callback) {
    queryIdentityString("id", callback);
  }

  private void queryIdentityString(String key, LynxElementCallback<String> callback) {
    if (callback == null) {
      return;
    }
    if (mOwner == null) {
      callback.onResult(null);
      return;
    }
    mOwner.query(mSign, LynxElementOwner.QUERY_IDENTITY, null, result -> {
      Object value = LynxElementOwner.asMap(result).get(key);
      callback.onResult(value instanceof String ? (String) value : null);
    });
  }

  public void getParent(LynxElementCallback<LynxElement> callback) {
    if (callback == null) {
      return;
    }
    if (mOwner == null) {
      callback.onResult(null);
      return;
    }
    mOwner.query(mSign, LynxElementOwner.QUERY_PARENT, null,
        result
        -> callback.onResult(mOwner.element(
            result instanceof Integer ? (Integer) result : LynxElementOwner.INVALID_SIGN)));
  }

  public void getChildren(LynxElementCallback<List<LynxElement>> callback) {
    if (callback == null) {
      return;
    }
    if (mOwner == null) {
      callback.onResult(new ArrayList<>());
      return;
    }
    mOwner.query(mSign, LynxElementOwner.QUERY_CHILDREN, null, result -> {
      List<LynxElement> elements = new ArrayList<>();
      for (Integer sign : LynxElementOwner.asSignList(result)) {
        LynxElement element = mOwner.element(sign);
        if (element != null) {
          elements.add(element);
        }
      }
      callback.onResult(elements);
    });
  }

  public void getChildCount(LynxElementCallback<Integer> callback) {
    if (callback != null) {
      getChildren(children -> callback.onResult(children.size()));
    }
  }

  public void getRoot(LynxElementCallback<LynxElement> callback) {
    if (callback == null) {
      return;
    }
    if (mOwner == null) {
      callback.onResult(null);
      return;
    }
    mOwner.query(0, LynxElementOwner.QUERY_ROOT, null,
        result
        -> callback.onResult(mOwner.element(
            result instanceof Integer ? (Integer) result : LynxElementOwner.INVALID_SIGN)));
  }

  public void getPositionInfo(LynxElementCallback<Map<String, Object>> callback) {
    if (callback == null) {
      return;
    }
    if (mOwner == null) {
      callback.onResult(LynxElementOwner.asMap(null));
      return;
    }
    mOwner.query(mSign, LynxElementOwner.QUERY_POSITION, null,
        result -> callback.onResult(LynxElementOwner.asMap(result)));
  }

  public void getDataset(LynxElementCallback<Map<String, Object>> callback) {
    if (callback == null) {
      return;
    }
    if (mOwner == null) {
      callback.onResult(LynxElementOwner.asMap(null));
      return;
    }
    mOwner.query(mSign, LynxElementOwner.QUERY_DATASET, null,
        result -> callback.onResult(LynxElementOwner.asMap(result)));
  }

  public void getAttribute(String name, LynxElementCallback<Object> callback) {
    if (callback == null) {
      return;
    }
    if (mOwner == null) {
      callback.onResult(null);
      return;
    }
    mOwner.query(mSign, LynxElementOwner.QUERY_ATTRIBUTE, name, callback::onResult);
  }

  public void getAttributes(LynxElementCallback<Map<String, Object>> callback) {
    if (callback == null) {
      return;
    }
    if (mOwner == null) {
      callback.onResult(LynxElementOwner.asMap(null));
      return;
    }
    mOwner.query(mSign, LynxElementOwner.QUERY_ATTRIBUTES, null,
        result -> callback.onResult(LynxElementOwner.asMap(result)));
  }

  public void getPlatformView(LynxElementCallback<View> callback) {
    if (callback == null) {
      return;
    }
    if (mOwner == null) {
      callback.onResult(null);
      return;
    }
    mOwner.getPlatformView(mSign, callback);
  }

  public void toJSONString(LynxElementCallback<String> callback) {
    if (callback == null) {
      return;
    }
    if (mOwner == null || mSign == LynxElementOwner.INVALID_SIGN) {
      UIThreadUtils.runOnUiThread(() -> callback.onResult(null));
      return;
    }
    mOwner.query(mSign, LynxElementOwner.QUERY_JSON, null,
        result -> callback.onResult(result instanceof String ? (String) result : null));
  }
}
