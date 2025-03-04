// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import android.content.Context;
import android.graphics.Rect;
import android.os.Build;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.WindowManager;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.WritableArray;
import com.lynx.react.bridge.mapbuffer.CompactArrayBuffer;
import com.lynx.react.bridge.mapbuffer.ReadableCompactArrayBuffer;
import com.lynx.react.bridge.mapbuffer.ReadableMapBuffer;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.behavior.shadow.ShadowNodeType;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.list.container.UIListContainer;
import com.lynx.tasm.behavior.ui.view.UIComponent;
import com.lynx.tasm.behavior.utils.LynxUIMethodsExecutor;
import com.lynx.tasm.event.EventsListener;
import com.lynx.tasm.gesture.detector.GestureDetector;
import com.lynx.tasm.utils.UIThreadUtils;
import java.util.Iterator;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;

class IntValueIndex {
  public static final int LEFT = 0;
  public static final int TOP = 1;
  public static final int WIDTH = 2;
  public static final int HEIGHT = 3;
  public static final int PADDING_LEFT = 4;
  public static final int PADDING_TOP = 5;
  public static final int PADDING_RIGHT = 6;
  public static final int PADDING_BOTTOM = 7;
  public static final int MARGIN_LEFT = 8;
  public static final int MARGIN_TOP = 9;
  public static final int MARGIN_RIGHT = 10;
  public static final int MARGIN_BOTTOM = 11;
  public static final int BORDER_LEFT = 12;
  public static final int BORDER_TOP = 13;
  public static final int BORDER_RIGHT = 14;
  public static final int BORDER_BOTTOM = 15;
  public static final int HAS_BOUND = 16;
  public static final int HAS_STICKY = 17;
  public static final int MAX_HEIGHT = 18;
  public static final int SIZE = 19;
}

class UIOperationType {
  public static final int INSERT = 0;
  public static final int REMOVE = 1;
  public static final int DESTROY = 2;
  public static final int READY_BATCHING = 3;
  public static final int REMOVE_BATCHING = 4;
  public static final int UPDATE_LAYOUT_PATCHING = 5;
  public static final int TASM_FINISH = 6;
  public static final int LAYOUT_FINISH = 7;
}

class CreateViewAsyncStatus {
  public static final int UNKOWN = -1;
  public static final int FUTURE_CANCEL_SUCCESS = 0;
  public static final int FUTURE_CANCEL_FAIL = 1;
  public static final int FUTURE_DONE = 2;
  public static final int FUTURE_DONE_EXCEPTION = 3;
}

public final class PaintingContext {
  private static final String TAG = "lynx_PaintingContext";
  private final LynxUIOwner mUIOwner;
  private boolean mDestroyed;
  private ConcurrentHashMap<String, Boolean> mNeedCreateNodeAsyncCache;

  private long mNativePaintingContextPtr = 0;

  public PaintingContext(LynxUIOwner uiOwner, int threadStrategy) {
    mUIOwner = uiOwner;
    mDestroyed = false;
    mNeedCreateNodeAsyncCache = new ConcurrentHashMap<String, Boolean>();
    mNativePaintingContextPtr =
        nativeCreatePaintingContext(this, threadStrategy, mUIOwner.isContextFree());
  }

  // this func will be execed on main thread.
  public void destroy() {
    mDestroyed = true;
  }

  public long getNativePaintingContextPtr() {
    return mNativePaintingContextPtr;
  }

  @CalledByNative
  public void updateLayout(int sign, float x, float y, float width, float height, float paddingLeft,
      float paddingTop, float paddingRight, float paddingBottom, float marginLeft, float marginTop,
      float marginRight, float marginBottom, float borderLeftWidth, float borderTopWidth,
      float borderRightWidth, float borderBottomWidth, float[] bounds, float[] sticky,
      float maxHeight, int nodeIndex) {
    mUIOwner.updateLayout(sign, (int) x, (int) y, (int) width, (int) height, (int) paddingLeft,
        (int) paddingTop, (int) paddingRight, (int) paddingBottom, (int) marginLeft,
        (int) marginTop, (int) marginRight, (int) marginBottom, (int) borderLeftWidth,
        (int) borderTopWidth, (int) borderRightWidth, (int) borderBottomWidth,
        bounds != null
            ? new Rect((int) bounds[0], (int) bounds[1], (int) bounds[2], (int) bounds[3])
            : null,
        sticky, maxHeight, nodeIndex);
  }

  /**
   * Updates the properties of a LynxUI node from native code.
   *
   * @param sign The unique identifier of the LynxUI node.
   * @param tendToFlatten Indicates if the LynxUI tends to flatten its layout.
   * @param props The properties to update, represented as a StylesDiffMap.
   * @param eventListeners Event listeners associated with the LynxUI node.
   * @param gestureDetectors Gesture detectors associated with the LynxUI node.
   */
  @CalledByNative
  public void updateProps(int sign, boolean tendToFlatten, ReadableMap props,
      ReadableMapBuffer styles, ReadableArray eventListeners, ReadableArray gestureDetectors) {
    // Convert event listeners and gesture detectors to appropriate data structures.
    Map<String, EventsListener> listeners = EventsListener.convertEventListeners(eventListeners);
    Map<Integer, GestureDetector> detectors =
        GestureDetector.convertGestureDetectors(gestureDetectors);
    // Update properties of the UI node through the UI owner.
    mUIOwner.updateProperties(sign, tendToFlatten,
        props != null ? new StylesDiffMap(props, styles) : null, listeners, detectors);
  }

  // TODO(zhouzhitao): Control group logic, will be removed once the experiment yields results
  @CalledByNative
  public Object createNode(final int sign, String tagName, final ReadableMap initialProps,
      final ReadableMapBuffer initialStyles, final ReadableArray eventListeners,
      final boolean isFlatten, int nodeIndex, final ReadableArray gestureDetectors) {
    String finalTagName = tagName;
    if (needCreateNodeAsync(finalTagName)) {
      final Future<Runnable> future = createNodeAsync(sign, finalTagName, initialProps,
          initialStyles, eventListeners, isFlatten, nodeIndex, gestureDetectors);
      return new Runnable() {
        @Override
        public void run() {
          boolean isSuccess = executeFuture(future, sign, finalTagName, initialProps, initialStyles,
              eventListeners, isFlatten, nodeIndex, gestureDetectors);
          mUIOwner.reportCreateViewConfig(sign, finalTagName, true);
        }
      };
    } else {
      return new Runnable() {
        @Override
        public void run() {
          createPaintingNodeSync(sign, finalTagName, initialProps, initialStyles, eventListeners,
              isFlatten, nodeIndex, gestureDetectors);
        }
      };
    }
  }

  private boolean needCreateNodeAsync(String tagName) {
    if (mNeedCreateNodeAsyncCache.containsKey(tagName)) {
      return mNeedCreateNodeAsyncCache.get(tagName);
    }
    boolean createAsync;
    if (!LynxEnv.inst().getCreateViewAsync()) {
      createAsync = false;
    } else if (!mUIOwner.getEnableCreateViewAsync()) {
      createAsync = false;
    } else {
      createAsync = mUIOwner.behaviorSupportCreateAsync(tagName);
    }
    mNeedCreateNodeAsyncCache.put(tagName, createAsync);
    return createAsync;
  }

  @CalledByNative
  public void createPaintingNodeSync(int sign, String tagName, ReadableMap initialProps,
      ReadableMapBuffer initialStyles, ReadableArray eventListeners, boolean isFlatten,
      int nodeIndex, ReadableArray gestureDetectors) {
    mUIOwner.createView(sign, tagName, initialProps, initialStyles, eventListeners, isFlatten,
        nodeIndex, gestureDetectors);
    mUIOwner.reportCreateViewConfig(sign, tagName, false);
  }

  // TODO(zhouzhitao): Control group logic, will be removed once the experiment yields results
  public Future<Runnable> createNodeAsync(int sign, String tagName, ReadableMap initialProps,
      ReadableMapBuffer initialStyles, ReadableArray eventListeners, boolean isFlatten,
      int nodeIndex, ReadableArray gestureDetectors) {
    StylesDiffMap styles = null;
    if (initialProps != null) {
      styles = new StylesDiffMap(initialProps, initialStyles);
    }
    Map<String, EventsListener> listeners = EventsListener.convertEventListeners(eventListeners);
    Map<Integer, GestureDetector> detectors =
        GestureDetector.convertGestureDetectors(gestureDetectors);
    final Future<Runnable> future =
        mUIOwner.createViewAsync(sign, tagName, styles, listeners, isFlatten, nodeIndex, detectors);
    return future;
  }

  @CalledByNative
  public Object createPaintingNodeAsync(int sign, String tagName, ReadableMap initialProps,
      ReadableMapBuffer initialStyles, ReadableArray eventListeners, boolean isFlatten,
      int nodeIndex, ReadableArray gestureDetectors) {
    return mUIOwner.createViewAsyncRunnable(sign, tagName, initialProps, initialStyles,
        eventListeners, isFlatten, nodeIndex, gestureDetectors);
  }

  /**
   * @return return true if create view task is done on async thread.
   */
  private boolean executeFuture(Future<Runnable> future, int sign, String tagName,
      ReadableMap initialProps, ReadableMapBuffer initialStyles, ReadableArray eventListeners,
      boolean isFlatten, int nodeIndex, ReadableArray gestureDetectors) {
    Runnable runnable = null;
    int status = CreateViewAsyncStatus.UNKOWN;

    if (!future.isDone()) {
      if (future.cancel(true)) {
        status = CreateViewAsyncStatus.FUTURE_CANCEL_SUCCESS;
      } else {
        status = CreateViewAsyncStatus.FUTURE_CANCEL_FAIL;
      }
      LLog.i(TAG, "createViewAsync not done, will create on ui thread, tagName:" + tagName);
    } else {
      try {
        runnable = future.get();
        status = CreateViewAsyncStatus.FUTURE_DONE;
      } catch (InterruptedException | ExecutionException e) {
        status = CreateViewAsyncStatus.FUTURE_DONE_EXCEPTION;
        String errorMessage = "createViewAsync failed, tagName:" + tagName + ", error:" + e;
        LLog.e(TAG, errorMessage);
        mUIOwner.getContext().handleException(new Exception(errorMessage));
      }
    }

    if (runnable != null) {
      runnable.run();
      mUIOwner.reportCreateAsyncSuccessEvent(sign, tagName, true, status);
      return true;
    } else {
      mUIOwner.createView(sign, tagName, initialProps, initialStyles, eventListeners, isFlatten,
          nodeIndex, gestureDetectors);
      mUIOwner.reportCreateAsyncSuccessEvent(sign, tagName, false, status);
      return false;
    }
  }

  @CalledByNative
  public void insertNode(int parentSign, int childSign, int index) {
    mUIOwner.insert(parentSign, childSign, index);
  }

  @CalledByNative
  public void removeNode(int parentSign, int childSign) {
    mUIOwner.remove(parentSign, childSign);
  }

  @CalledByNative
  public void destroyNode(int parentSign, int childSign) {
    mUIOwner.destroy(parentSign, childSign);
  }

  @CalledByNative
  private void flushUIOperationBatch(ReadableCompactArrayBuffer ui_operation_batch) {
    if (ui_operation_batch == null) {
      return;
    }
    Iterator<CompactArrayBuffer.Entry> iterator = ui_operation_batch.iterator();
    while (iterator.hasNext()) {
      int operation = iterator.next().getInt();
      switch (operation) {
        case UIOperationType.INSERT: {
          int parent = iterator.next().getInt();
          int child = iterator.next().getInt();
          int index = iterator.next().getInt();
          insertNode(parent, child, index);
        } break;
        case UIOperationType.REMOVE: {
          int parent = iterator.next().getInt();
          int child = iterator.next().getInt();
          removeNode(parent, child);
        } break;
        case UIOperationType.DESTROY: {
          int parent = iterator.next().getInt();
          int child = iterator.next().getInt();
          destroyNode(parent, child);
        } break;
        case UIOperationType.READY_BATCHING: {
          int size = iterator.next().getInt();
          int[] readyIds = new int[size];
          for (int i = 0; i < size; i++) {
            readyIds[i] = iterator.next().getInt();
          }
          updateNodeReadyPatching(readyIds);
        } break;
        case UIOperationType.REMOVE_BATCHING: {
          int size = iterator.next().getInt();
          int[] removeIds = new int[size];
          for (int i = 0; i < size; i++) {
            removeIds[i] = iterator.next().getInt();
          }
          updateNodeRemovePatching(removeIds);
        } break;
        case UIOperationType.UPDATE_LAYOUT_PATCHING: {
          // patching id
          int patching_id_count = iterator.next().getInt();
          int[] patching_id = new int[patching_id_count];
          for (int i = 0; i < patching_id_count; i++) {
            patching_id[i] = iterator.next().getInt();
          }

          // patching ints
          int patching_ints_count = iterator.next().getInt();
          int[] patching_ints = new int[patching_ints_count];
          for (int i = 0; i < patching_ints_count; i++) {
            patching_ints[i] = iterator.next().getInt();
          }

          // patching bounds
          int patching_bounds_count = iterator.next().getInt();
          float[] patching_bounds = new float[patching_bounds_count];
          for (int i = 0; i < patching_bounds_count; i++) {
            patching_bounds[i] = (float) iterator.next().getDouble();
          }

          // patching stickies
          int patching_stickies_count = iterator.next().getInt();
          float[] patching_stickies = new float[patching_stickies_count];
          for (int i = 0; i < patching_stickies_count; i++) {
            patching_stickies[i] = (float) iterator.next().getDouble();
          }

          // patching node index
          int patching_node_index_count = iterator.next().getInt();
          int[] patching_node_index = new int[patching_node_index_count];
          for (int i = 0; i < patching_node_index_count; i++) {
            patching_node_index[i] = iterator.next().getInt();
          }
          UpdateLayoutPatching(
              patching_id, patching_ints, patching_bounds, patching_stickies, patching_node_index);
        } break;
        case UIOperationType.TASM_FINISH: {
          long operationId = iterator.next().getLong();
          finishTasmOperation(operationId);
        } break;
        case UIOperationType.LAYOUT_FINISH: {
          int listComponentId = iterator.next().getInt();
          long operationId = iterator.next().getLong();
          // isFirstScreen is useless now, just pass true. Should delete later.
          FinishLayoutOperation(listComponentId, operationId, true);
        } break;
        default:
          LLog.e(TAG, "flushUIOperationBatch with unknown UIOperationType: " + operation);
          return;
      }
    }
  }

  @CalledByNative
  public void updateFlattenStatus(int sign, boolean flatten) {
    mUIOwner.updateFlatten(sign, flatten);
  }

  @CalledByNative
  public void setKeyframes(ReadableMap props) {
    mUIOwner.getContext().removeAnimationKeyframe(props.getString("removeKeyframe"));
    mUIOwner.getContext().setKeyframes(props.getMap("keyframes"));
  }

  /**
   * Sets the state of a gesture detector for a specific UI node.
   *
   * @param sign The unique identifier of the UI node.
   * @param gestureId The identifier of the specific gesture.
   * @param state GestureState::ACTIVE (1) GestureState::FAIL (2) GestureState::END (3)
   */
  @CalledByNative
  public void SetGestureDetectorState(int sign, int gestureId, int state) {
    // Forward the call to the UI owner to set the gesture detector state.
    mUIOwner.setGestureDetectorState(sign, gestureId, state);
  }

  /**
   * Handle whether internal lynxUI of the current gesture node consume the gesture and whether
   * native view outside the current node (outside of lynxView) consume the gesture.
   * @param sign The unique identifier of the UI node.
   * @param gestureId The identifier of the specific native gesture.
   * @param params {inner: boolean, consume: boolean, ...}
   */
  @CalledByNative
  public void consumeGesture(int sign, int gestureId, ReadableMap params) {
    mUIOwner.consumeGesture(sign, gestureId, params);
  }

  @CalledByNative
  public void validate(int sign) {
    mUIOwner.validate(sign);
  }

  @CalledByNative
  private void finishTasmOperation(long operationId) {
    mUIOwner.onTasmFinish(operationId);
  }

  @CalledByNative
  public void FinishLayoutOperation(int componentId, long operationId, boolean isFirstScreen) {
    mUIOwner.onLayoutFinish(componentId, operationId);
  }

  @CalledByNative
  public void onCollectExtraUpdates(int signature) {
    // this method can be removed
  }

  @CalledByNative
  public void updateExtraData(int signature, Object data) {
    mUIOwner.updateViewExtraData(signature, data);
  }

  @CalledByNative
  public float[] getBoundingClientOrigin(int sign) {
    float[] res = new float[] {0, 0};
    LynxBaseUI ui = mUIOwner.getNode(sign);
    if (ui != null) {
      Rect re = ui.getBoundingClientRect();
      res[0] = re.left;
      res[1] = re.top;
    }
    return res;
  }

  @CalledByNative
  public void scrollIntoView(int sign) {
    LynxBaseUI ui = mUIOwner.getNode(sign);
    if (ui == null) {
      return;
    }
    ui.scrollIntoView(false, "center", "center", null);
  }
  @CalledByNative
  public float[] getWindowSize(int sign) {
    float[] res = new float[2];
    WindowManager wm =
        (WindowManager) mUIOwner.getContext().getContext().getSystemService(Context.WINDOW_SERVICE);
    if (wm == null) {
      return null;
    }
    Display display = wm.getDefaultDisplay();
    DisplayMetrics dm = new DisplayMetrics();
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
      display.getRealMetrics(dm);
    } else {
      display.getMetrics(dm);
    }
    res[0] = dm.widthPixels;
    res[1] = dm.heightPixels;
    return res;
  }

  @CalledByNative
  public float[] getRectToWindow(int sign) {
    float[] res = new float[] {0, 0, 0, 0};
    LynxBaseUI ui = mUIOwner.getNode(sign);
    if (ui != null) {
      Rect re = ui.getRectToWindow();
      res[0] = re.left;
      res[1] = re.top;
      res[2] = re.width();
      res[3] = re.height();
    }
    return res;
  }

  @CalledByNative
  public float[] getRectToLynxView(int sign) {
    float[] res = new float[] {0, 0, 0, 0};
    LynxBaseUI ui = mUIOwner.getNode(sign);
    if (ui != null) {
      Rect re = ui.getBoundingClientRect();
      res[0] = re.left;
      res[1] = re.top;
      res[2] = re.width();
      res[3] = re.height();
    }
    return res;
  }

  @CalledByNative
  public float[] scrollBy(int sign, float width, float height) {
    if (UIThreadUtils.isOnUiThread()) {
      LynxBaseUI ui = mUIOwner.getNode(sign);
      if (ui != null) {
        return ui.scrollBy(width, height);
      } else {
        return getScrollDefaultResult(width, height);
      }
    } else {
      UIThreadUtils.runOnUiThread(() -> {
        LynxBaseUI ui = mUIOwner.getNode(sign);
        if (ui != null) {
          ui.scrollBy(width, height);
        }
      });
      return getScrollDefaultResult(width, height);
    }
  }

  // It is not consumed when method return
  private float[] getScrollDefaultResult(float width, float height) {
    float[] res = new float[4];
    res[0] = 0;
    res[1] = 0;
    res[2] = width;
    res[3] = height;
    return res;
  }

  @CalledByNative
  public void invoke(
      int sign, String method, ReadableMap params, final long context, final int callback) {
    UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
      private void cb(Object... args) {
        if (mDestroyed || mUIOwner.getContext() == null) {
          return;
        }
        mUIOwner.getContext().runOnTasmThread(
            () -> nativeInvokeCallback(context, callback, JavaOnlyArray.of(args)));
      }

      @Override
      public void run() {
        LynxBaseUI ui = mUIOwner.getNode(sign);
        if (ui != null) {
          LynxUIMethodsExecutor.invokeMethod(ui, method, params,
              (Object... args) -> UIThreadUtils.runOnUiThreadImmediately(() -> cb(args)));
        } else {
          cb(LynxUIMethodConstants.NO_UI_FOR_NODE,
              String.format("Worklet: node %d does not have a LynxUI", sign));
        }
      }
    });
  }

  private void setLayoutData(int sign, int x, int y, int width, int height, int paddingLeft,
      int paddingTop, int paddingRight, int paddingBottom, int marginLeft, int marginTop,
      int marginRight, int marginBottom, int borderLeftWidth, int borderTopWidth,
      int borderRightWidth, int borderBottomWidth, Rect bounds, float[] sticky, int maxHeight,
      int nodeIndex) {
    mUIOwner.updateLayout(sign, x, y, width, height, paddingLeft, paddingTop, paddingRight,
        paddingBottom, marginLeft, marginTop, marginRight, marginBottom, borderLeftWidth,
        borderTopWidth, borderRightWidth, borderBottomWidth, bounds, sticky, maxHeight, nodeIndex);
  }

  /*
   * The int array represents the rect(4), paddings(4), margins(4), borders(4), has_bound,
   * has_sticky and max_height for the layout info.
   * The float array allows for variable lengths.
   */
  @CalledByNative
  public void UpdateLayoutPatching(
      int[] signs, int[] ints, float[] bounds, float[] stickies, int[] nodeIndex) {
    final int size = IntValueIndex.SIZE; // the step of the for loop
    final int length = signs.length; // the count of patching
    int bIndex = 0;
    int sIndex = 0;
    for (int i = 0; i < length; i++) {
      Rect rect = null;
      if (ints[i * size + IntValueIndex.HAS_BOUND] != 0) {
        // bound != null, get value from bounds
        rect = new Rect((int) bounds[bIndex * 4], (int) bounds[bIndex * 4 + 1],
            (int) bounds[bIndex * 4 + 2], (int) bounds[bIndex * 4 + 3]);
        bIndex++;
      }
      float[] sticky = null;
      if (ints[i * size + IntValueIndex.HAS_STICKY] != 0) {
        // sticky != null, get value from stickies
        sticky = new float[] {stickies[sIndex * 4], stickies[sIndex * 4 + 1],
            stickies[sIndex * 4 + 2], stickies[sIndex * 4 + 3]};
        sIndex++;
      }
      setLayoutData(signs[i], ints[i * size + IntValueIndex.LEFT],
          ints[i * size + IntValueIndex.TOP], ints[i * size + IntValueIndex.WIDTH],
          ints[i * size + IntValueIndex.HEIGHT], ints[i * size + IntValueIndex.PADDING_LEFT],
          ints[i * size + IntValueIndex.PADDING_TOP], ints[i * size + IntValueIndex.PADDING_RIGHT],
          ints[i * size + IntValueIndex.PADDING_BOTTOM], ints[i * size + IntValueIndex.MARGIN_LEFT],
          ints[i * size + IntValueIndex.MARGIN_TOP], ints[i * size + IntValueIndex.MARGIN_RIGHT],
          ints[i * size + IntValueIndex.MARGIN_BOTTOM], ints[i * size + IntValueIndex.BORDER_LEFT],
          ints[i * size + IntValueIndex.BORDER_TOP], ints[i * size + IntValueIndex.BORDER_RIGHT],
          ints[i * size + IntValueIndex.BORDER_BOTTOM], rect, sticky,
          ints[i * size + IntValueIndex.MAX_HEIGHT], nodeIndex[i]);
    }
  }

  @CalledByNative
  public void updateNodeReadyPatching(int[] readyIds, int[] removeIds) {
    updateNodeReadyPatching(readyIds);
    updateNodeRemovePatching(removeIds);
  }

  private void updateNodeReadyPatching(int[] readyIds) {
    for (int sign : readyIds) {
      mUIOwner.onNodeReady(sign);
    }
  }

  private void updateNodeRemovePatching(int[] removeIds) {
    for (int sign : removeIds) {
      mUIOwner.onNodeRemoved(sign);
    }
  }

  @CalledByNative
  public void updateEventInfo(boolean hasTouchPseudo) {
    TouchEventDispatcher touchEventDispatcher = mUIOwner.getContext().getTouchEventDispatcher();
    if (touchEventDispatcher != null) {
      touchEventDispatcher.setHasTouchPseudo(hasTouchPseudo);
    }
  }

  @CalledByNative
  public void updateNodeReloadPatching(int[] reloadIds) {
    for (int sign : reloadIds) {
      mUIOwner.onNodeReload(sign);
    }
  }

  @CalledByNative
  public void reuseListNode(int tag, String itemKey) {
    mUIOwner.reuseListNode(tag, itemKey);
  }

  @CalledByNative
  public void listCellDisappear(int tag, boolean isExist, String itemKey) {
    mUIOwner.listCellDisappear(tag, isExist, itemKey);
  }

  @CalledByNative
  public void listCellAppear(int tag, String itemKey) {
    mUIOwner.listCellAppear(tag, itemKey);
  }

  @CalledByNative
  public void requestLayout() {
    UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
      @Override
      public void run() {
        if (mUIOwner.getRootUI() != null) {
          mUIOwner.getRootUI().requestLayout();
        }
      }
    });
  }

  /**
   * @param tagName tag name of the node to be queried
   * @return 32bit integer value representing data object related to tag name including layout node
   *     type and whether need to create node on a background thread.
   * Each data object value will adhere to the following layout:
   * ┌────────────────────────────────────────────────────────────────────────────────────────────┐
   * │                           Data object value in bit array form (from low to high)           │
   * │                                                                                            │
   * ├─────────────layout node type────────────────┬───create async──┬─────Reserved bits──────────┤
   * │                         16 bits             │     1 bit       │             15 bits        │
   * └────────────────────────────────────────────────────────────────────────────────────────────┘
   */
  @CalledByNative
  public int getTagInfo(String tagName) {
    int layoutNodeType = mUIOwner.getTagInfo(tagName);
    boolean isVirtual = (layoutNodeType & ShadowNodeType.VIRTUAL) > 0;
    if (isVirtual) {
      // For virtual nodes, we only need to return the layout node type.
      return (layoutNodeType & 0xFFFF);
    }

    boolean createNodeAsync = needCreateNodeAsync(tagName);
    return ((createNodeAsync ? 1 : 0) << 16 | (layoutNodeType & 0xFFFF));
  }

  @CalledByNative
  public void removeListItemNode(int listSign, int childSign) {
    String traceEvent = null;
    if (TraceEvent.enableTrace()) {
      traceEvent = TAG + ".removeListItemNode_" + listSign + "_" + childSign;
      TraceEvent.beginSection(traceEvent);
    }
    LynxBaseUI parent = mUIOwner.getNode(listSign);
    LynxBaseUI child = mUIOwner.getNode(childSign);
    if (parent instanceof UIListContainer && child instanceof UIComponent) {
      ((UIListContainer) parent).removeView(child);
    }
    if (TraceEvent.enableTrace()) {
      TraceEvent.endSection(traceEvent);
    }
  }

  @CalledByNative
  public void insertListItemNode(int listSign, int childSign) {
    String traceEvent = null;
    if (TraceEvent.enableTrace()) {
      traceEvent = TAG + ".insertListItemNode_" + listSign + "_" + childSign;
      TraceEvent.beginSection(traceEvent);
    }
    LynxBaseUI parent = mUIOwner.getNode(listSign);
    LynxBaseUI child = mUIOwner.getNode(childSign);
    if (parent instanceof UIListContainer && child instanceof UIComponent) {
      ((UIListContainer) parent).insertListItemNode(child);
    }
    if (TraceEvent.enableTrace()) {
      TraceEvent.endSection(traceEvent);
    }
  }

  @CalledByNative
  public void updateContentSizeAndOffset(
      int listSign, float contentSize, float deltaX, float deltaY) {
    LynxBaseUI parent = mUIOwner.getNode(listSign);
    if (parent instanceof UIListContainer) {
      ((UIListContainer) parent).updateContentSizeAndOffset(contentSize, deltaX, deltaY);
    }
  }

  @CalledByNative
  public void updateScrollInfo(
      int containerID, boolean smooth, float estimatedOffset, boolean scrolling) {
    LynxBaseUI ui = mUIOwner.getNode(containerID);
    if (ui instanceof UIListContainer) {
      ((UIListContainer) ui).updateScrollInfo(smooth, estimatedOffset, scrolling);
    }
  }

  private native void nativeInvokeCallback(long context, int callback, WritableArray array);

  private native long nativeCreatePaintingContext(
      Object paintingContext, int threadStrategy, boolean isContextFree);
}
