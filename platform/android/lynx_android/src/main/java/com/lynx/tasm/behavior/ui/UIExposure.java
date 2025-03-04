// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui;

import android.graphics.RectF;
import android.util.DisplayMetrics;
import android.view.Choreographer;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxObserverManager;
import com.lynx.tasm.event.LynxDetailEvent;
import com.lynx.tasm.utils.UIThreadUtils;
import com.lynx.tasm.utils.UnitUtils;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;

public class UIExposure extends LynxObserverManager {
  public interface ICallBack {
    boolean canSendGlobalEvent();

    void sendGlobalEvent(String name, JavaOnlyArray params);

    LynxBaseUI findNode(int sign);
  }

  private static class CallBack implements Choreographer.FrameCallback {
    final private WeakReference<UIExposure> mExposure;

    private CallBack(UIExposure exposure) {
      this.mExposure = new WeakReference<UIExposure>(exposure);
    }

    @Override
    public void doFrame(long frameTimeNanos) {
      UIExposure exposure = mExposure.get();
      if (exposure != null) {
        exposure.observerHandlerInner();
      }
    }
  }

  static class UIExposureDetail {
    final private WeakReference<LynxBaseUI> mUI;
    final private String mExposureID;
    final private String mExposureScene;
    final private int mSign;
    final private ReadableMap mDataSet;
    final private String mUniqueID;
    final private JavaOnlyMap mExtraData;
    final private JavaOnlyMap mUseOptions;

    UIExposureDetail(LynxBaseUI ui, @Nullable String uniqueID, @Nullable JavaOnlyMap extraData,
        @Nullable JavaOnlyMap useOptions) {
      mUI = new WeakReference<LynxBaseUI>(ui);
      if (ui.getExposureID() == null) {
        mExposureID = ""; // if user not set exposure-id, use this default value: ""
      } else {
        mExposureID = ui.getExposureID();
      }
      if (ui.getExposureScene() == null) {
        mExposureScene = ""; // if user not set exposure-scene, use this default value: ""
      } else {
        mExposureScene = ui.getExposureScene();
      }
      mSign = ui.getSign();
      mDataSet = ui.getDataset();
      mUniqueID = uniqueID != null ? uniqueID : "";
      mExtraData = extraData != null ? JavaOnlyMap.from(extraData) : new JavaOnlyMap();
      mUseOptions = useOptions;
    }

    public int getSign() {
      return mSign;
    }

    @Override
    public boolean equals(Object o) {
      if (this == o) {
        return true;
      }
      if ((o == null) || (getClass() != o.getClass())) {
        return false;
      }
      UIExposureDetail that = (UIExposureDetail) o;
      return mSign == that.mSign && mExposureScene.equals(that.mExposureScene)
          && mExposureID.equals(that.mExposureID) && mUniqueID.equals(that.mUniqueID);
    }

    @Override
    public int hashCode() {
      // Aligned to iOS.
      LynxBaseUI ui = mUI.get();
      if (ui != null) {
        return ui.hashCode();
      }
      return 0;
    }

    public HashMap<String, Object> toMap() {
      HashMap<String, Object> map = new HashMap<>();
      map.put("exposure-id", mExposureID);
      map.put("exposure-scene", mExposureScene);
      map.put("dataset", mDataSet);
      map.put("unique-id", mUniqueID);
      map.put("extra-data", mExtraData);
      return map;
    }
  }
  // A map hold all UI which be set exposured property.
  final private HashMap<String, UIExposureDetail> mExposureDetailMap;
  // A map hold every UI which has exposure property and stay inside window.
  private HashSet<UIExposureDetail> mUiInWindowNow;
  // ditto, but UI from previous moment
  private HashSet<UIExposureDetail> mUiInWindowBefore;
  // The size of this window
  private RectF mWindowRect = null;
  // The flag that reflects whether the stopExposure is called.
  private boolean mIsStopExposure;
  private ICallBack mCallback = null;

  final private String TAG = "Lynx.UIExposure";
  final private String UIAPPEAREVENT = "uiappear";
  final private String UIDISAPPEAREVENT = "uidisappear";

  final private CallBack mCallBack;

  public UIExposure() {
    super("Lynx.UIExposure");
    mExposureDetailMap = new HashMap<>();
    mUiInWindowBefore = new HashSet<>();
    mUiInWindowNow = new HashSet<>();
    mRootBodyRef = new WeakReference<>(null);
    mCallBack = new CallBack(this);
  }

  public void setCallback(ICallBack callback) {
    mCallback = callback;
  }

  boolean canSendGlobalEvent() {
    if (mCallback == null) {
      LLog.e(TAG, "canSendGlobalEvent check failed since mCallback is null.");
      return true;
    }
    return mCallback.canSendGlobalEvent();
  }

  void sendGlobalEvent(String name, JavaOnlyArray params) {
    if (mCallback == null) {
      LLog.e(TAG, "sendEvent failed since mCallback is null.");
      return;
    }
    mCallback.sendGlobalEvent(name, params);
  }

  LynxBaseUI findNode(int sign) {
    if (mCallback == null) {
      LLog.e(TAG, "findNode failed since mCallback is null.");
      return null;
    }
    return mCallback.findNode(sign);
  }

  private float getIntersectionAreaRatio(RectF targetRect, RectF otherRect) {
    RectF intersectionRect = new RectF();
    if (intersectionRect.setIntersect(targetRect, otherRect)) {
      float originArea = targetRect.height() * targetRect.width();
      float intersectionArea = intersectionRect.height() * intersectionRect.width();
      return intersectionArea / originArea;
    } else {
      return 0f;
    }
  }

  private boolean checkIntersect(RectF targetRect, RectF otherRect, float ratio) {
    // ratio's default value is 0, and when ratio = 0, only calculate whether two rect are
    // intersected.
    if (ratio != 0f) {
      return getIntersectionAreaRatio(targetRect, otherRect) >= ratio;
    } else {
      return RectF.intersects(targetRect, otherRect);
    }
  }

  private RectF getUIRect(LynxBaseUI ui) {
    RectF uiRect = getBoundsOnScreenOfLynxBaseUI(ui);
    if (ui.getEnableExposureUIMargin()) {
      // get UI's frame, calculate the needed rect with exposureUIMargin
      float width = uiRect.width();
      float height = uiRect.height();
      DisplayMetrics metrics = ui.getLynxContext().getScreenMetrics();
      String left = ui.getExposureUIMarginLeft();
      float left_ = UnitUtils.toPxWithDisplayMetrics(left, 0, 0, 0, 0, width, 0, metrics);

      String right = ui.getExposureUIMarginRight();
      float right_ = UnitUtils.toPxWithDisplayMetrics(right, 0, 0, 0, 0, width, 0, metrics);

      String top = ui.getExposureUIMarginTop();
      float top_ = UnitUtils.toPxWithDisplayMetrics(top, 0, 0, 0, 0, height, 0, metrics);

      String bottom = ui.getExposureUIMarginBottom();
      float bottom_ = UnitUtils.toPxWithDisplayMetrics(bottom, 0, 0, 0, 0, height, 0, metrics);

      // uiRect's area < 0
      if (width + left_ + right_ <= 0 || height + top_ + bottom_ <= 0) {
        return new RectF();
      }
      uiRect.left -= left_;
      uiRect.top -= top_;
      uiRect.right += right_;
      uiRect.bottom += bottom_;
    } else {
      // old logic, when exposureMargin > 0, calculate ui's rect side instead of screen's rect side
      uiRect.left -= ui.getExposureScreenMarginRight() > 0 ? ui.getExposureScreenMarginRight() : 0;
      uiRect.right += ui.getExposureScreenMarginLeft() > 0 ? ui.getExposureScreenMarginLeft() : 0;
      uiRect.top -= ui.getExposureScreenMarginBottom() > 0 ? ui.getExposureScreenMarginBottom() : 0;
      uiRect.bottom += ui.getExposureScreenMarginTop() > 0 ? ui.getExposureScreenMarginTop() : 0;
    }
    return uiRect;
  }

  private RectF getBorderOfWindowRect(LynxBaseUI ui) {
    if (ui.getEnableExposureUIMargin()) {
      if (mWindowRect == null) {
        mWindowRect = getWindowRect(ui.getLynxContext());
      }
      // get screen's frame, calculate the needed rect with exposureMargin (not exposureUIMargin)
      RectF borderOfWindowRect = new RectF(mWindowRect);

      // screenRect's area < 0
      if (borderOfWindowRect.width() + ui.getExposureScreenMarginLeft()
                  + ui.getExposureScreenMarginRight()
              <= 0
          || borderOfWindowRect.height() + ui.getExposureScreenMarginTop()
                  + ui.getExposureScreenMarginBottom()
              <= 0) {
        return new RectF();
      }

      borderOfWindowRect.left -= ui.getExposureScreenMarginLeft();
      borderOfWindowRect.top -= ui.getExposureScreenMarginTop();
      borderOfWindowRect.right += ui.getExposureScreenMarginRight();
      borderOfWindowRect.bottom += ui.getExposureScreenMarginBottom();

      return borderOfWindowRect;
    } else {
      // old logic, when exposureMargin < 0, calculate screen's rect side
      RectF borderOfWindowRect = new RectF(mWindowRect.left
              - (int) (ui.getExposureScreenMarginLeft() < 0 ? ui.getExposureScreenMarginLeft() : 0),
          mWindowRect.top
              - (int) (ui.getExposureScreenMarginTop() < 0 ? ui.getExposureScreenMarginTop() : 0),
          mWindowRect.right
              + (int) (ui.getExposureScreenMarginRight() < 0 ? ui.getExposureScreenMarginRight()
                                                             : 0),
          mWindowRect.bottom
              + (int) (ui.getExposureScreenMarginBottom() < 0 ? ui.getExposureScreenMarginBottom()
                                                              : 0));
      return borderOfWindowRect;
    }
  }

  private boolean inWindow(LynxBaseUI ui) {
    // if ui's height == 0 || ui's width == 0, return false, see issue:#7731
    if (ui.getHeight() == 0 || ui.getWidth() == 0) {
      return false;
    }

    LynxBaseUI parent = ui;
    ArrayList<LynxBaseUI> parentList = new ArrayList<>();
    boolean isInOverlay = false;
    while (parent != null && parent != mRootBodyRef.get()) {
      if (parent instanceof LynxUI && !((LynxUI) parent).isVisible()) {
        return false;
      }
      if (parent.isScrollContainer()) {
        parentList.add(parent);
      }
      if (parent.isOverlay()) {
        isInOverlay = true;
        break;
      }
      parent = (LynxBaseUI) parent.getParent();
    }
    RectF uiRect = getUIRect(ui);
    float exposureAreaRatio = UnitUtils.toPx(ui.getExposureArea());

    for (LynxBaseUI parentUI : parentList) {
      RectF parentRect = getBoundsOnScreenOfLynxBaseUI(parentUI);
      if (!checkIntersect(uiRect, parentRect, exposureAreaRatio)) {
        return false;
      }
    }

    RectF viewRect = getBoundsOnScreenOfLynxBaseUI(mRootBodyRef.get());
    if (mWindowRect == null) {
      mWindowRect = getWindowRect(ui.getLynxContext());
    }
    boolean isIntersectWithRoot =
        !isInOverlay ? checkIntersect(uiRect, viewRect, exposureAreaRatio) : true;
    if (mWindowRect != null) {
      RectF borderOfWindowRect = getBorderOfWindowRect(ui);
      return isIntersectWithRoot && checkIntersect(uiRect, borderOfWindowRect, exposureAreaRatio);
    } else {
      return isIntersectWithRoot;
    }
  }

  // should be called in ui thread
  // NOTICE: make sure whether didObserveInner() is added before return.
  @Override
  protected void observerHandlerInner() {
    // After calling stopExposure, mHandler will be set to null, but it may not be destroyed
    // immediately. Therefore, the exposure detection task may still be executed, causing
    // unnecessary exposure events to be triggered.
    if (mIsStopExposure) {
      LLog.e(TAG, "Lynx exposureHandler failed since lynx.stopExposure() is called");
      didObserveInner();
      return;
    }

    // issue: #6896. In some cases, the users may delay the execution of LynxView's draw function
    // after LynxView attchToWindow and layout. In these cases, exposure & disexposure event
    // should not be triggred before LynxView draw. Thus, use mRootViewPainted to show whether the
    // RootView's onDraw func has been execed. Only when LynxView's draw function has been execed,
    // the exposureHandlerInner function will be execed.
    if (!mRootViewPainted) {
      LLog.e(TAG, "Lynx exposureHandler failed since rootView not draw");
      didObserveInner();
      return;
    }

    // issue: #6800. In some cases, when exec the exposureHandlerInner function, the lynxview has
    // been destroyed. To avoid NPE, before exec the exposureHandlerInner function, check if
    // lynxview is null.
    UIBody.UIBodyView view = getRootView();
    if (view == null) {
      LLog.e(TAG, "Lynx exposureHandler failed since rootView is null");
      didObserveInner();
      return;
    }

    // if lynxView is not real visible(lynxview is visible, but its parent view is gone), send
    // disexposure event.
    // must be judge before view.isLayoutRequested(), because when view is not shown,
    // view.isLayoutRequested() may still return true.
    if (!view.isShown()) {
      if (mEnableDisexposureWhenLynxHidden) {
        HashSet<UIExposureDetail> uiDisappear = new HashSet<>(mUiInWindowBefore);
        sendEvent(uiDisappear, "disexposure");
        mUiInWindowBefore.clear();
      }
      didObserveInner();
      return;
    }

    // when rootView's layout is not done, component's frame may have not been set, so exposure
    // will get the wrong position. So, delay the exposure until layout is done.
    if (!mEnableExposureWhenLayout && view.isLayoutRequested()) {
      // if handler has not run yet because of layout not finished, set mDelayedInInner to true.
      // or observerHandlerInner will be called from observerHandler, producing many useless task.
      postHandlerCallBackDelay(mCallBack);
      return;
    }

    // step 0: update mWindowRect, see issue:#7699
    mWindowRect = getWindowRect(mRootBodyRef.get().getLynxContext());

    // step 1: select UI in the screen from all exposed UI
    for (UIExposureDetail detail : mExposureDetailMap.values()) {
      LynxBaseUI v = detail.mUI.get();
      if (v != null) {
        if (inWindow(v)) {
          mUiInWindowNow.add(detail);
        }
      }
    }

    // step 2: in mUiInWindowMapBefore, not in mUiInWindowMapNow means the UI disappear
    HashSet<UIExposureDetail> uiDisappear = new HashSet<>();
    uiDisappear.addAll(mUiInWindowBefore);
    uiDisappear.removeAll(mUiInWindowNow);

    // step 3: in mUiInWindowMapNow, not in muiInWindowMapBefore means the UI appear
    HashSet<UIExposureDetail> uiAppear = new HashSet<>();
    uiAppear.addAll(mUiInWindowNow);
    uiAppear.removeAll(mUiInWindowBefore);

    mUiInWindowBefore = mUiInWindowNow;
    mUiInWindowNow = new HashSet<>();

    // step 4: send Global events or custom events.
    sendEvent(uiDisappear, "disexposure");
    sendEvent(uiAppear, "exposure");
    didObserveInner();
  }

  public void stopExposure(HashMap<String, Object> options) {
    mIsStopExposure = true;
    destroy();
    // Use the sendEvent field in options to control whether to send disexposure events.
    if (options.containsKey("sendEvent") && (Boolean) options.get("sendEvent")) {
      sendEvent(mUiInWindowBefore, "disexposure");
      mUiInWindowBefore.clear();
    }
  }

  public void resumeExposure() {
    mIsStopExposure = false;
    addToObserverTree();
  }

  protected void sendEvent(HashSet<UIExposureDetail> uiList, String eventName) {
    UIBody.UIBodyView view = getRootView();
    if (view == null) {
      return;
    }
    if (!uiList.isEmpty()) {
      if (canSendGlobalEvent()) {
        JavaOnlyArray params = new JavaOnlyArray();
        HashMap<LynxBaseUI, HashMap<String, ArrayList<UIExposureDetail>>> customParamMap =
            new HashMap<>();

        for (UIExposureDetail detail : uiList) {
          if (detail.mUseOptions != null && detail.mUseOptions.containsKey("sendCustom")
              && detail.mUseOptions.getBoolean("sendCustom")) {
            if (detail.mUseOptions.containsKey("specifyTarget")
                && detail.mUseOptions.getBoolean("specifyTarget")) {
              addDetailToCustomParamMap(customParamMap, detail);
              continue;
            }

            LynxBaseUI ui = detail.mUI.get();
            String transEventName = "";
            if (eventName == "exposure") {
              transEventName = UIAPPEAREVENT;
            } else {
              transEventName = UIDISAPPEAREVENT;
            }
            if (ui != null && ui.getEvents() != null
                && ui.getEvents().containsKey(transEventName)) {
              LynxDetailEvent event =
                  new LynxDetailEvent(ui.getSign(), transEventName, detail.toMap());
              ui.getLynxContext().getEventEmitter().sendCustomEvent(event);
            }
            continue;
          }

          params.pushMap(createResult(detail));
        }

        sendCustomParamMapEvent(customParamMap, eventName);

        if (params.size() > 0) {
          JavaOnlyArray array = new JavaOnlyArray();
          array.add(params);
          sendGlobalEvent(eventName, array);
        }
      } else {
        // Just use for Air 1.0, forward compatible is required
        for (UIExposureDetail detail : uiList) {
          LynxBaseUI ui = findNode(detail.getSign());
          if (ui != null && ui.getEvents() != null && ui.getEvents().containsKey(eventName)) {
            LynxDetailEvent event = new LynxDetailEvent(ui.getSign(), eventName, detail.toMap());
            ui.getLynxContext().getEventEmitter().sendCustomEvent(event);
          }
        }
      }
    }
  }

  private void sendCustomParamMapEvent(
      HashMap<LynxBaseUI, HashMap<String, ArrayList<UIExposureDetail>>> customParamMap,
      String eventName) {
    if (customParamMap.isEmpty()) {
      return;
    }
    for (Map.Entry<LynxBaseUI, HashMap<String, ArrayList<UIExposureDetail>>> entry :
        customParamMap.entrySet()) {
      LynxBaseUI receiveTarget = entry.getKey();
      for (Map.Entry<String, ArrayList<UIExposureDetail>> eventEntry :
          entry.getValue().entrySet()) {
        String bindEventName = eventEntry.getKey();
        JavaOnlyArray array = new JavaOnlyArray();
        for (UIExposureDetail detail : eventEntry.getValue()) {
          array.pushMap(createChildUIResult(detail));
        }
        JavaOnlyMap eventDetail = new JavaOnlyMap();
        eventDetail.put("isExposure", eventName == "exposure");
        eventDetail.put("childrenInfo", array);
        LynxDetailEvent event =
            new LynxDetailEvent(receiveTarget.getSign(), bindEventName, eventDetail);
        receiveTarget.getLynxContext().getEventEmitter().sendCustomEvent(event);
      }
    }
  }

  private void addDetailToCustomParamMap(
      HashMap<LynxBaseUI, HashMap<String, ArrayList<UIExposureDetail>>> customParamMap,
      UIExposureDetail detail) {
    LynxBaseUI receiveTarget = detail.mUI.get();
    if (receiveTarget == null) {
      return;
    }
    receiveTarget = receiveTarget.getExposeReceiveTarget();
    String bindEventName = detail.mUseOptions.getString("bindEventName");
    if (receiveTarget != null && receiveTarget.getEvents() != null && bindEventName != null
        && receiveTarget.getEvents().containsKey(bindEventName)) {
      if (customParamMap.containsKey(receiveTarget)) {
        HashMap<String, ArrayList<UIExposureDetail>> map = customParamMap.get(receiveTarget);
        if (map.containsKey(bindEventName)) {
          map.get(bindEventName).add(detail);
        } else {
          map.put(bindEventName, new ArrayList<>());
          map.get(bindEventName).add(detail);
        }
      } else {
        HashMap<String, ArrayList<UIExposureDetail>> map = new HashMap<>();
        ArrayList<UIExposureDetail> list = new ArrayList<>();
        list.add(detail);
        map.put(bindEventName, list);
        customParamMap.put(receiveTarget, map);
      }
    }
  }
  public boolean addUIToExposedMap(LynxBaseUI ui, @Nullable String uniqueID,
      @Nullable JavaOnlyMap data, @Nullable JavaOnlyMap options) {
    if (uniqueID != null || ui.getExposureID() != null) {
      String key;
      if (uniqueID != null) {
        key = uniqueID + "_" + ui.getExposureScene() + "_" + ui.getExposureID();
      } else {
        key = ui.getExposureScene() + "_" + ui.getExposureID() + "_" + ui.getSign();
      }
      mExposureDetailMap.put(key, new UIExposureDetail(ui, uniqueID, data, options));
      // Ensure add to Observer once.
      // After calling stopExposure, the exposure detection task should only be started in
      // resumeExposure.
      if (!mIsStopExposure && mExposureDetailMap.size() == 1) {
        addToObserverTree();
      }
      return true;
    }
    return false;
  }

  // In MULTI_THREAD mode, the method will be called in other thread, so read mExposureDetailMap in
  // main thread.
  public void removeUIFromExposedMap(LynxBaseUI ui, @Nullable String uniqueID) {
    if (uniqueID != null || ui.getExposureID() != null) {
      String key;
      if (uniqueID != null) {
        key = uniqueID + "_" + ui.getExposureScene() + "_" + ui.getExposureID();
      } else {
        key = ui.getExposureScene() + "_" + ui.getExposureID() + "_" + ui.getSign();
      }
      UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
        @Override
        public void run() {
          if (mExposureDetailMap.get(key) == null) {
            return;
          }
          LynxBaseUI targetUI = mExposureDetailMap.get(key).mUI.get();
          if (targetUI != null) {
            mExposureDetailMap.remove(key);
          }
          // if have no exposuredUI, you should free the system resource
          if (mExposureDetailMap.isEmpty()) {
            // When remove all nodes with exposure-related property, we need to disexposure the
            // exposed nodes.
            clear();
          }
        }
      });
    }
  }

  public void setRootUI(UIBody ui) {
    mRootBodyRef = new WeakReference<>(ui);
  }

  private JavaOnlyMap createResult(UIExposureDetail detail) {
    JavaOnlyMap result = new JavaOnlyMap();
    result.put("exposure-id", detail.mExposureID);
    result.put("exposureID", detail.mExposureID);
    result.put("exposure-scene", detail.mExposureScene);
    result.put("exposureScene", detail.mExposureScene);
    result.put("sign", String.valueOf(detail.mSign));
    result.put("dataSet", detail.mDataSet);
    result.put("dataset", detail.mDataSet);
    result.put("unique-id", detail.mUniqueID);
    result.put("extra-data", detail.mExtraData);
    return result;
  }

  private JavaOnlyMap createChildUIResult(UIExposureDetail detail) {
    JavaOnlyMap result = new JavaOnlyMap();
    result.put("extra-data", detail.mExtraData);
    return result;
  }

  public void clear() {
    // Due to the possibility of LynxView executing the destroy function in an asynchronous thread,
    // the removeListener operation may be executed in the asynchronous thread, causing a crash. To
    // fix this, the removeListener operation must be executed in the main thread.
    UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
      @Override
      public void run() {
        destroy();
        mExposureDetailMap.clear();

        // Before reloading the page, we need to disexposure the previous page.
        sendEvent(mUiInWindowBefore, "disexposure");
        mUiInWindowBefore.clear();
      }
    });
  }
}
