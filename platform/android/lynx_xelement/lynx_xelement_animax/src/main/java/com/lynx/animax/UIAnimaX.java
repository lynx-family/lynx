// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.animax;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Path;
import android.graphics.RectF;
import android.view.MotionEvent;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import com.lynx.animax.ability.LynxAbility;
import com.lynx.animax.base.AnimaXError;
import com.lynx.animax.loader.LynxHttpAnimaXLoader;
import com.lynx.animax.monitor.AnimaXMonitorUtil;
import com.lynx.animax.monitor.LynxAnimaXMonitorDefault;
import com.lynx.animax.service.IAnimaXMonitorService;
import com.lynx.animax.service.IAnimaXResourceFactoryService;
import com.lynx.animax.setter.UIAnimaXPropsPrioritySetter;
import com.lynx.animax.ui.AnimaXContainerView;
import com.lynx.animax.ui.AnimaXContext;
import com.lynx.animax.ui.AnimaXImageView;
import com.lynx.animax.ui.AnimaXView;
import com.lynx.animax.ui.IAnimaXView;
import com.lynx.animax.util.AnimaXLog;
import com.lynx.animax.util.DeviceUtil;
import com.lynx.animax.util.UIPropertyUtil;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.ForegroundListener;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.LynxPropsHolder;
import com.lynx.tasm.behavior.LynxUIMethod;
import com.lynx.tasm.behavior.LynxUIMethodConstants;
import com.lynx.tasm.behavior.LynxUIMethodsHolder;
import com.lynx.tasm.behavior.StylesDiffMap;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.utils.BackgroundDrawable;
import com.lynx.tasm.behavior.ui.utils.LynxUIHelper;
import com.lynx.tasm.event.EventsListener;
import com.lynx.tasm.event.LynxCustomEvent;
import com.lynx.tasm.eventreport.LynxEventReporter;
import com.lynx.tasm.service.ILynxTrailService;
import com.lynx.tasm.service.LynxServiceCenter;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

/**
 * Lynx UI component "animax-view", this wrap LynxUI and provide property implementation
 * Only created by LynxAnimaX.createUI()
 */
@RestrictTo(RestrictTo.Scope.LIBRARY)
@LynxUIMethodsHolder
@LynxPropsHolder
public class UIAnimaX extends LynxUI<View> implements ForegroundListener {
  private static final String TAG = "UIAnimaX";

  private static final UIAnimaXPropsPrioritySetter.ExecutionPriority RESOURCE_LOAD =
      UIAnimaXPropsPrioritySetter.ExecutionPriority.LOW;
  private static final UIAnimaXPropsPrioritySetter.ExecutionPriority RESOURCE_SETUP =
      UIAnimaXPropsPrioritySetter.ExecutionPriority.HIGH;

  private enum DisplayMode {
    IMAGE("image"),
    SURFACE("surface"),
    AUTO("auto");
    private final String value;

    DisplayMode(String value) {
      this.value = value;
    }

    public static DisplayMode fromString(String mode) {
      if (mode == null) {
        return SURFACE;
      }
      for (DisplayMode m : values()) {
        if (m.value.equals(mode)) {
          return m;
        }
      }
      return SURFACE;
    }
  }

  private final LynxContext mContext;

  // These instances may be null due to createView fallback.
  @Nullable private LynxAbility mAbility;
  @Nullable private IAnimaXView mAnimaXView;
  @Nullable private AnimaXPlayer mAnimaXPlayer;
  @Nullable private final AnimaXContainerView mAnimaXContainerView;

  @NonNull
  private final UIAnimaXPropsPrioritySetter mPropsSetter = new UIAnimaXPropsPrioritySetter();

  // Init props. Stash props before AnimaXPlayer/AnimaXView is actually created.
  private boolean mMultiThreadAccelerate = false;
  private boolean mIgnoreAttachStatus = false;
  private DisplayMode mDisplayMode = DisplayMode.SURFACE;
  private String mTag;

  // UI AnimaX Props
  // Allow Lynx UIAnimaX send touch event to element.
  private boolean mEnableLynxTapLayerEvent = false;
  private boolean mIgnoreLynxLifecycle = false;
  private boolean mEnableScreenShot = false;
  private String mSrc;

  // Cross-flush flags.
  private boolean mHasReportMotionEvent = false;

  public UIAnimaX(LynxContext context) {
    this(context, null);
  }
  public UIAnimaX(LynxContext context, Object params) {
    super(context, params);
    mContext = context;
    if (!(mView instanceof AnimaXContainerView)) {
      // Send init error event to fronted to process callback.
      postInitError();
    }
    mAnimaXContainerView = (AnimaXContainerView) mView;
  }

  // The init error is post to main thread because getSign is a invalid value for now.
  private void postInitError() {
    mView.post(()
                   -> mContext.getEventEmitter().sendCustomEvent(new LynxCustomEvent(
                       getSign(), "error", AnimaXError.createBlockErrorParam()) {
      @Override
      public String paramsName() {
        return "detail";
      }
    }));
  }

  @Override
  protected View createView(Context context) {
    if (!(context instanceof LynxContext)) {
      LLog.e(TAG, "context is not LynxContext, create AnimaXView fail");
      return new View(context);
    }
    LynxContext lynxContext = (LynxContext) context;
    mAbility = new LynxAbility(this, lynxContext);
    return new AnimaXContainerView(context);
  }

  @Override
  public boolean dispatchTouch(MotionEvent ev) {
    if (mEnableLynxTapLayerEvent && mAnimaXView != null && isUserInteractionEnabled()
        && ev.getAction() == MotionEvent.ACTION_DOWN) {
      RectF viewScreenRect = LynxUIHelper.convertRectFromUIToScreen(
          this, new RectF(0, 0, this.getWidth(), this.getHeight()));
      MotionEvent transformedEvent = MotionEvent.obtain(ev);
      float pointX = ev.getRawX() - viewScreenRect.left;
      float pointY = ev.getRawY() - viewScreenRect.top;
      if (mAnimaXContainerView == null) {
        transformedEvent.setLocation(pointX, pointY);
      } else {
        transformedEvent.setLocation(pointX - mAnimaXContainerView.getPaddingLeft(),
            pointY - mAnimaXContainerView.getPaddingTop());
      }
      mAnimaXView.handleTouchEvent(transformedEvent);
    }
    return super.dispatchTouch(ev);
  }

  @Override
  public void destroy() {
    super.destroy();

    AnimaXLog.i(TAG, "UIAnimaX destroy");

    if (mAnimaXView != null) {
      mAnimaXView.release();
    } else if (mAnimaXPlayer != null) {
      mAnimaXPlayer.release();
    }
    mAnimaXView = null;
    mAnimaXPlayer = null;
  }

  @Override
  public void onNodeReload() {
    super.onNodeReload();

    if (mAnimaXPlayer != null) {
      mAnimaXPlayer.reload();
    }
  }

  @Override
  public long getMemoryUsageBytes() {
    if (mAnimaXPlayer == null) {
      return 0;
    }
    return mAnimaXPlayer.getMemoryUsageBytes();
  }

  private void createAnimaXPlayer() {
    if (mAbility == null) {
      return;
    }

    AnimaXContext animaxContext = new AnimaXContext.Builder(mAbility, mContext.getContext())
                                      .multiThreadAccelerate(mMultiThreadAccelerate)
                                      .build();

    mAbility.registerService(IAnimaXMonitorService.class, new LynxAnimaXMonitorDefault(mContext));

    mAbility.registerService(IAnimaXResourceFactoryService.class,
        () -> Collections.singletonList(new LynxHttpAnimaXLoader(mAbility, mContext)));
    mAnimaXPlayer = new AnimaXPlayer(animaxContext);
    mPropsSetter.init(mAnimaXPlayer);
  }

  private void createAnimaXView() {
    if (mAbility == null || mContext == null) {
      LLog.e(TAG, "LynxAbility or LynxContext is not created");
      postInitError();
      return;
    }

    if (mAnimaXContainerView == null) {
      LLog.e(TAG, "AnimaXContainerView is not created");
      postInitError();
      return;
    }

    if (mAnimaXPlayer == null) {
      LLog.e(TAG, "AnimaXPlayer is not created");
      postInitError();
      return;
    }

    if (!DeviceUtil.checkCapability(mAbility)) {
      LLog.e(TAG, "Device is not support, create AnimaXView fail");
      postInitError();
      return;
    }

    boolean useImageDisplayMode = mDisplayMode == DisplayMode.IMAGE
        || (mDisplayMode == DisplayMode.AUTO && DeviceUtil.shouldUseImageViewByTag(mAbility, mTag));
    if (useImageDisplayMode) {
      AnimaXLog.i(TAG, "create AnimaXImageView");
      mAbility.getMonitorDelegate().setDisplayMode(DisplayMode.IMAGE.value);
      mAnimaXView = new AnimaXImageView(mAnimaXPlayer);
      // TODO(lixianruo.cyrus): add screenshot for AnimaXImageView.
      mAnimaXContainerView.addChildAnimaXView((View) mAnimaXView, null);
    } else {
      AnimaXLog.i(TAG, "create AnimaXView");
      mAbility.getMonitorDelegate().setDisplayMode(DisplayMode.SURFACE.value);
      mAnimaXView = new AnimaXView(mAnimaXPlayer);
      mAnimaXContainerView.addChildAnimaXView(
          (View) mAnimaXView, new AnimaXContainerView.IDispatchDrawHook() {
            @Override
            public void beforeDispatchDraw(Canvas canvas) {
              tryToAddClip(canvas);
            }
            @Override
            public boolean useBitmapOnDraw() {
              return mEnableScreenShot;
            }
          });
    }
    // Ensure platform AnimaXView do not send touch event to element.
    // Only UIAnimaX can send event by dispatchTouch from LynxUI.
    mAnimaXView.setEnableTapLayerEvent(false);
    mAnimaXView.setIgnoreAttachStatus(mIgnoreAttachStatus);
  }
  // LynxProp START

  @LynxProp(name = "tag")
  public void setTag(String tag) {
    mTag = tag;
    mAbility.getMonitorDelegate().setTag(tag);
  }

  @LynxProp(name = "display-mode")
  public void setDisplayMode(String displayMode) {
    mDisplayMode = DisplayMode.fromString(displayMode);
  }

  @LynxProp(name = "multi-thread-accelerate", defaultBoolean = true)
  public void setMultiThreadAccelerate(boolean enable) {
    mMultiThreadAccelerate = enable;
  }

  /**
   * Set whether to enable screenshot
   * @param enable whether enable screenshot.
   */
  @LynxProp(name = "android-enable-screenshot")
  public void setEnableScreenshot(boolean enable) {
    mEnableScreenShot = enable;
  }

  /**
   * Whether the animation autoplay when its resource loaded.
   * @param enable whether enable autoplay.
   */
  @LynxProp(name = "autoplay", defaultBoolean = true)
  public void setAutoPlay(boolean enable) {
    mPropsSetter.enqueueTask(player -> { player.setAutoPlay(enable); });
  }

  /**
   * Set the speed of animation. the default value is 1.
   * @param speed a relative value. Can be positive, negative and 0.
   */
  @LynxProp(name = "speed")
  public void setSpeed(float speed) {
    mPropsSetter.enqueueTask(player -> player.setSpeed(speed));
  }

  /**
   * Set the initial progress of animation. You can set 0.f to start from the start-frame, and
   * set 1.f to start from the end-frame.
   * @param progress a progress between 0.f and 1.f.
   */
  @LynxProp(name = "progress")
  public void setProgress(float progress) {
    mPropsSetter.enqueueTask(player -> player.setProgress(progress));
  }

  /**
   * Set how to show the animation when the size of the animation and of the view aren't match.
   * if you set to "contain", the default value, animation will be proportional scaled. one of the
   * animation side will be scaled to match the view, and the other will less than the view. It
   * looks like the view contains the animation. if you set to "cover", animation will be
   * proportional scaled. one of the animation side will be scaled to match the view, and the other
   * will greater than the view. Some parts of animation may be invisible. if you set to "center",
   * animation won't be resized, and the animation will be placed on the center of the view.
   * @param objectFit "contain", "cover", or "center" as described.
   */
  @LynxProp(name = "objectfit")
  public void setObjectFit(String objectFit) {
    mPropsSetter.enqueueTask(
        player -> player.setObjectFit(UIPropertyUtil.convertStringToObjectFit(objectFit)));
  }

  /**
   * Set how to align the animation within the view when the animation size and the view size do not
   * match. If you set to "center", the default value, the animation will be centered both
   * horizontally and vertically. If you set to "left", the animation will be aligned to the left
   * edge of the view and vertically centered. If you set to "right", the animation will be aligned
   * to the right edge of the view and vertically centered. If you set to "top", the animation will
   * be aligned to the top edge of the view and horizontally centered. If you set to "bottom", the
   * animation will be aligned to the bottom edge of the view and horizontally centered. If you set
   * to "top-left", the animation will be aligned to the top-left corner of the view. If you set
   * to "top-right", the animation will be aligned to the top-right corner of the view. If you set
   * to "bottom-left", the animation will be aligned to the bottom-left corner of the view. If you
   * set to "bottom-right", the animation will be aligned to the bottom-right corner of the view.
   * @param objectPosition "center", "left", "right", "top", "bottom", "top-left", "top-right",
   *     "bottom-left" or "bottom-right" as described.
   */
  @LynxProp(name = "object-position")
  public void setObjectPosition(String objectPosition) {
    mPropsSetter.enqueueTask(player
        -> player.setObjectPosition(UIPropertyUtil.convertStringToObjectPosition(objectPosition)));
  }

  /**
   * Whether keep end-frame when the animation is stopped.
   * @param enable keep end-frame on view if the enable is true, change to start-frame if the enable
   *     is false.
   */
  @LynxProp(name = "keeplastframe", defaultBoolean = true)
  public void setKeepLastFrame(boolean enable) {
    mPropsSetter.enqueueTask(player -> player.setKeepLastFrame(enable));
  }

  /**
   * When the view move off the screen, animation will stop. You can set this value to true to
   * disable this action. Android specific.
   * @param ignore animation will stop if the view move off the screen and ignore is false.
   */
  @LynxProp(name = "ignore-attach-status", defaultBoolean = false)
  public void setIgnoreAttachStatus(boolean ignore) {
    if (mAnimaXView != null) {
      mAnimaXView.setIgnoreAttachStatus(ignore);
    } else {
      mIgnoreAttachStatus = ignore;
    }
  }

  private void setViewTag(String src) {
    if (mSrc != null || mContext == null || mView == null) {
      return;
    }
    if (src == null) {
      src = AnimaXMonitorUtil.SRC_URL_UNKNOWN;
    } else {
      mSrc = src;
    }
    String tag =
        "url: " + AnimaXMonitorUtil.clearUrlQuery(mContext.getTemplateUrl()) + ", src: " + src;
    mView.setTag(tag);
  }

  /**
   * Set the animation content directly.
   * @param json the animation content.
   */
  @LynxProp(name = "json")
  public void setJson(String json) {
    mPropsSetter.enqueueTask(player -> {
      player.setJson(json);
      setViewTag("json");
      reportMotionEvent("json");
    }, RESOURCE_LOAD);
  }

  /**
   * Set the animation source url.
   * This source should be a json file.
   * @param src animation source url.
   */
  @LynxProp(name = "src")
  public void setSrc(String src) {
    mPropsSetter.enqueueTask(player -> {
      player.setSrc(src);
      setViewTag(src);
      reportMotionEvent(src);
    }, RESOURCE_LOAD);
  }

  /**
   * Whether loop animation.
   * if you set loop to true, loop-count will be ignored.
   * @param enable enable loop playback.
   */
  @LynxProp(name = "loop", defaultBoolean = false)
  public void setLoop(boolean enable) {
    mPropsSetter.enqueueTask(player -> player.setLoop(enable));
  }

  /**
   * Set loop count of the animation.
   * if you set loop to true, this property will be ignore. be sure to check this.
   * @param loopCount loop count of the animation. it should be positive number or 0. 0 means loop
   *     forever.
   */
  @LynxProp(name = "loop-count", defaultInt = 1)
  public void setLoopCount(int loopCount) {
    mPropsSetter.enqueueTask(player -> player.setLoopCount(loopCount));
  }

  /**
   * Extention to animation format in json.
   * Use this property with src-polyfill.
   * You can use this property to defer to describe an image assert as follow:
   * "assert": [{
   *     "id": "image_0",
   *     "w": 380,
   *     "h": 380,
   *     "u": "",
   *     "p": "%s",
   *     "e": 0
   * }]
   * that is, set "u" to "", set "p" to "%s".
   * Later, you can set src-polyfill with this image assert id and image url as follow:
   * {
   *     "image_0": "path/to/image_0.png",
   * }
   * @param srcFormat animation source url.
   */
  @LynxProp(name = "src-format")
  public void setSrcFormat(String srcFormat) {
    mPropsSetter.enqueueTask(player -> {
      player.setSrc(srcFormat);
      setViewTag(srcFormat);
      reportMotionEvent(srcFormat);
    }, RESOURCE_LOAD);
  }

  /**
   * See src-format.
   * @param polyfillMap a map, keys are "id" in "assert", values are images url
   */
  @LynxProp(name = "src-polyfill")
  public void setSrcPolyfill(ReadableMap polyfillMap) {
    if (polyfillMap instanceof JavaOnlyMap) {
      mPropsSetter.enqueueTask(
          player -> player.setSrcPolyfill(adaptMap((JavaOnlyMap) polyfillMap)), RESOURCE_SETUP);
    } else {
      AnimaXLog.e(TAG, "setSrcPolyfill fail");
    }
  }

  private com.lynx.animax.base.bridge.JavaOnlyMap adaptMap(JavaOnlyMap fromMap) {
    com.lynx.animax.base.bridge.JavaOnlyMap toMap = new com.lynx.animax.base.bridge.JavaOnlyMap();
    for (Map.Entry<String, Object> entry : fromMap.asHashMap().entrySet()) {
      if (entry.getValue() instanceof String) {
        toMap.put(entry.getKey(), entry.getValue());
      }
    }
    return toMap;
  }

  /**
   * Set the start frame of the animation. Then the animation will play from start-frame to
   * end-frame.
   * @param startFrame start-frame of the animation. a value less than origin start frame will be
   *     converted to origin start frame. origin start frame is "ip" in json file.
   */
  @LynxProp(name = "start-frame", defaultInt = 0)
  public void setStartFrame(int startFrame) {
    mPropsSetter.enqueueTask(player -> player.setStartFrame(startFrame));
  }

  /**
   * Set the end frame of the animation. The the animation will play from start-frame to end-frame.
   * @param endFrame end-frame of the animation. a value greater than origin end frame will be
   *     converted to origin end frame, -1 will be converted to origin end frame too. origin end
   *     frame is "op" in json file.
   */
  @LynxProp(name = "end-frame", defaultInt = -1)
  public void setEndFrame(int endFrame) {
    mPropsSetter.enqueueTask(player -> player.setEndFrame(endFrame));
  }

  /**
   * Set auto reverse for the even loop.
   * If true, the even loop(counting from 1) will be played from last frame to first frame; the odd
   * loop will be played as usual.
   * @param isAutoReverse whether enable auto reverse.
   */
  @LynxProp(name = "auto-reverse", defaultBoolean = false)
  public void setReverseMode(boolean isAutoReverse) {
    mPropsSetter.enqueueTask(player -> player.setAutoReverse(isAutoReverse));
  }

  @LynxProp(name = "anti-aliasing")
  public void setAntiAliasing(String antiAliasing) {
    mPropsSetter.enqueueTask(player -> player.setAntiAliasing(!"none".equals(antiAliasing)));
  }
  // LynxProp END

  // AnimaX only LynxProp START
  /**
   * Set bind fps event callback's interval
   * @param interval - xx ms fps event should be back
   */
  @LynxProp(name = "fps-event-interval")
  public void setFpsEventInterval(int interval) {
    mPropsSetter.enqueueTask(player -> player.setFpsEventInterval(interval));
  }

  /**
   * Set max frame rate
   * @param maxFrameRate max frame rate of rendering animation
   */
  @LynxProp(name = "max-frame-rate")
  public void setMaxFrameRate(double maxFrameRate) {
    mPropsSetter.enqueueTask(player -> player.setMaxFrameRate(maxFrameRate));
  }

  /**
   * Declare there is resource property changes before call play method
   * This will be invalid if autoplay was set
   * @param dynamic Declares there is dynamic resource.
   */
  @LynxProp(name = "dynamic-resource")
  public void setDynamicResource(boolean dynamic) {
    mPropsSetter.enqueueTask(player -> player.setDynamicResource(dynamic), RESOURCE_SETUP);
  }

  /**
   * Mutes the audio layer.
   * The audio layer could be muted and unmuted when the animation is playing.
   * @param muted if true, mutes audio layer.
   */
  @LynxProp(name = "muted", defaultBoolean = false)
  public void setMuted(boolean mute) {
    mPropsSetter.enqueueTask(player -> player.setMuted(mute));
  }

  /**
   * Declares audio layer should be parsed.
   * This will be invalid if src was set.
   * @param enable if true, the audio layer will be parsed.
   */
  @LynxProp(name = "enable-audio", defaultBoolean = false)
  public void setEnableAudio(boolean enable) {
    mPropsSetter.enqueueTask(player -> player.setEnableAudio(enable), RESOURCE_SETUP);
  }

  /**
   * Set video frame timeout, if the video frame is not available in timeout ms, the video player
   * give up current frame and wait for next frame.
   * @param timeout video frame timeout in ms
   */
  @LynxProp(name = "video-frame-timeout")
  public void setVideoFrameTimeout(int timeout) {
    if (mAbility != null) {
      mAbility.getVideoPlayerConfig().setVideoFrameTimeout(timeout);
    }
  }

  /**
   * Whether ignore lynx lifecycle event
   * @param ignoreLynxLifecycle ignore lynx lifecycle event if true. Default false.
   */
  @LynxProp(name = "ignore-lynx-lifecycle", defaultBoolean = false)
  public void setIgnoreLynxLifecycle(boolean ignoreLynxLifecycle) {
    mIgnoreLynxLifecycle = ignoreLynxLifecycle;
  }

  @Override
  public void setEvents(Map<String, EventsListener> events) {
    super.setEvents(events);
    // Only enable layer event when bindtaplayers is set.
    mEnableLynxTapLayerEvent = events != null && events.containsKey("taplayers");
  }
  // AnimaX only LynxProp END

  @Override
  public void updatePropertiesInterval(StylesDiffMap map) {
    super.updatePropertiesInterval(map);
    if (mAnimaXPlayer == null) {
      createAnimaXPlayer();
    }
    if (mAnimaXView == null) {
      createAnimaXView();
    }
    mPropsSetter.flush();
  }

  // LynxUIMethod START
  /**
   * Play animation.
   * Play from the origin start frame of the animation. You can call play() when the resource are
   * loaded, animation will always play from origin start frame.
   * @param params   not used.
   * @param callback javascript callback.
   */
  @LynxUIMethod
  public void play(ReadableMap params, Callback callback) {
    if (mAnimaXPlayer == null) {
      invokeErrorCallback(callback);
      return;
    }

    mAnimaXPlayer.play();
    if (callback != null) {
      callback.invoke(LynxUIMethodConstants.SUCCESS);
    }
  }

  /**
   * Pause animation.
   * @param params   not used.
   * @param callback javascript callback.
   */
  @LynxUIMethod
  public void pause(ReadableMap params, Callback callback) {
    if (mAnimaXPlayer == null) {
      invokeErrorCallback(callback);
      return;
    }

    mAnimaXPlayer.pause();
    if (callback != null) {
      callback.invoke(LynxUIMethodConstants.SUCCESS);
    }
  }

  /**
   * Resume animation from break point.
   * You can call resume() after pause(). A call after stop() will play animation from the start
   * frame.
   * @param params   not used.
   * @param callback javascript callback.
   */
  @LynxUIMethod
  public void resume(ReadableMap params, Callback callback) {
    if (mAnimaXPlayer == null) {
      invokeErrorCallback(callback);
      return;
    }

    mAnimaXPlayer.resume();
    if (callback != null) {
      callback.invoke(LynxUIMethodConstants.SUCCESS);
    }
  }

  /**
   * Stop animation.
   * Stop animation, reset its progress to 0.
   * @param params   not used.
   * @param callback javascript callback.
   */
  @LynxUIMethod
  public void stop(ReadableMap params, Callback callback) {
    if (mAnimaXPlayer == null) {
      invokeErrorCallback(callback);
      return;
    }

    mAnimaXPlayer.stop();
    if (callback != null) {
      callback.invoke(LynxUIMethodConstants.SUCCESS);
    }
  }

  @LynxUIMethod
  public void getDuration(ReadableMap params, Callback callback) {
    if (mAnimaXPlayer == null) {
      invokeErrorCallback(callback);
      return;
    }

    if (callback != null) {
      JavaOnlyMap result = new JavaOnlyMap();
      result.putDouble("data", mAnimaXPlayer.getDurationMs());
      callback.invoke(LynxUIMethodConstants.SUCCESS, result);
    }
  }

  @LynxUIMethod
  public void isAnimating(ReadableMap params, Callback callback) {
    if (mAnimaXPlayer == null) {
      invokeErrorCallback(callback);
      return;
    }

    if (callback != null) {
      JavaOnlyMap result = new JavaOnlyMap();
      result.putBoolean("data", mAnimaXPlayer.isAnimating());
      callback.invoke(LynxUIMethodConstants.SUCCESS, result);
    }
  }

  /**
   * Whether receive event subscribed by subscribeUpdateEvent. the default value is true.
   * @param params   a map contain "isListen". if "isListen" is true, when your subscribed frame is
   *     rendered, you will be notified.
   * @param callback javascript callback.
   */
  @LynxUIMethod
  public void listenAnimationUpdate(ReadableMap params, Callback callback) {
    if (mAbility == null) {
      invokeErrorCallback(callback);
      return;
    }

    if (params != null) {
      mAbility.setListenUpdate(params.getBoolean("isListen"));
    }
    if (callback != null) {
      JavaOnlyMap result = new JavaOnlyMap();
      result.putBoolean("data", mAbility.getListenUpdate());
      callback.invoke(LynxUIMethodConstants.SUCCESS, result);
    }
  }

  /**
   * Seek to a frame.
   * @param params   a map contains "frame" to which you want to seek.
   * @param callback javascript callback.
   */
  @LynxUIMethod
  public void seek(ReadableMap params, Callback callback) {
    if (mAnimaXPlayer == null) {
      invokeErrorCallback(callback);
      return;
    }

    int frame = params.getInt("frame");
    mAnimaXPlayer.seek(frame);
    if (callback != null) {
      callback.invoke(LynxUIMethodConstants.SUCCESS);
    }
  }

  /**
   * Subscribe event that a new frame is rendered.
   * @param params   a map contain "frame", when this frame is rendered, you will be notified.
   *     subscribe frame 0 means origin start frame, frame 1 means origin start frame plus 1, and so
   *     on.
   * @param callback javascript callback.
   */
  @LynxUIMethod
  public void subscribeUpdateEvent(ReadableMap params, Callback callback) {
    if (mAnimaXPlayer == null) {
      invokeErrorCallback(callback);
      return;
    }

    int frame = params.getInt("frame");
    mAnimaXPlayer.subscribeUpdateEvent(frame);
    if (callback != null) {
      callback.invoke(LynxUIMethodConstants.SUCCESS);
    }
  }

  /**
   * Unsubscribe a frame that is subscribed by subscribeUpdateEvent.
   * @param params   a map contain "frame".
   * @param callback javascript callback.
   */
  @LynxUIMethod
  public void unsubscribeUpdateEvent(ReadableMap params, Callback callback) {
    if (mAnimaXPlayer == null) {
      invokeErrorCallback(callback);
      return;
    }

    int frame = params.getInt("frame");
    mAnimaXPlayer.unsubscribeUpdateEvent(frame);
    if (callback != null) {
      callback.invoke(LynxUIMethodConstants.SUCCESS);
    }
  }

  /**
   * Subscribe event that a list of new frames is rendered.
   * @param params   a map contain "frames", when any frame is rendered, you will be notified.
   *     subscribe frame 0 means origin start frame, frame 1 means origin start frame plus 1, and so
   *     on.
   * @param callback javascript callback.
   */
  @LynxUIMethod
  public void subscribeUpdateEvents(ReadableMap params, Callback callback) {
    if (mAnimaXPlayer == null) {
      invokeErrorCallback(callback);
      return;
    }

    int[] frames = getFramesFromParams(params);
    if (frames == null) {
      invokeErrorCallback(callback);
      return;
    }

    mAnimaXPlayer.subscribeUpdateEvents(frames, true);
    if (callback != null) {
      callback.invoke(LynxUIMethodConstants.SUCCESS);
    }
  }

  /**
   * Unsubscribe a list of frames that is subscribed by subscribeUpdateEvent.
   * @param params   a map contain "frames".
   * @param callback javascript callback.
   */
  @LynxUIMethod
  public void unsubscribeUpdateEvents(ReadableMap params, Callback callback) {
    if (mAnimaXPlayer == null) {
      invokeErrorCallback(callback);
      return;
    }

    int[] frames = getFramesFromParams(params);
    if (frames == null) {
      invokeErrorCallback(callback);
      return;
    }

    mAnimaXPlayer.subscribeUpdateEvents(frames, false);
    if (callback != null) {
      callback.invoke(LynxUIMethodConstants.SUCCESS);
    }
  }

  @LynxUIMethod
  public void getCurrentFrame(ReadableMap params, Callback callback) {
    if (mAnimaXPlayer == null) {
      invokeErrorCallback(callback);
      return;
    }

    if (callback != null) {
      callback.invoke(LynxUIMethodConstants.SUCCESS, mAnimaXPlayer.getCurrentFrame());
    }
  }

  /**
   * Play a specific segment of the animation from a given start frame to an end frame.
   * The animation will immediately jump to the start frame and begin playing the specified segment.
   */
  @LynxUIMethod
  public void playSegment(ReadableMap params, Callback callback) {
    if (mAnimaXPlayer == null) {
      invokeErrorCallback(callback);
      return;
    }

    int startFrame = params.getInt("startFrame");
    int endFrame = params.getInt("endFrame");

    // Validate the frame range, ensuring the end frame is greater than the start frame.
    if (endFrame > 0 && startFrame > endFrame) {
      invokeErrorCallback(callback, "startFrame and endFrame are not valid!");
      return;
    }

    // Play the animation segment, the assuming behavior is immediate playback.
    mAnimaXPlayer.playSegment(startFrame, endFrame);

    // Invoke the callback with success after starting the segment playback.
    if (callback != null) {
      callback.invoke(LynxUIMethodConstants.SUCCESS);
    }
  }

  private void invokeErrorCallback(Callback callback) {
    invokeErrorCallback(callback, "animax view is not inited.");
  }

  private void invokeErrorCallback(Callback callback, String errorMessage) {
    if (callback != null) {
      callback.invoke(LynxUIMethodConstants.UNKNOWN, errorMessage);
    }
  }

  @Nullable
  private int[] getFramesFromParams(ReadableMap params) {
    ReadableArray paramsArray = params.getArray("frames");
    if (paramsArray == null) {
      return null;
    }

    int[] frames = new int[paramsArray.size()];
    for (int i = 0; i < paramsArray.size(); i++) {
      frames[i] = paramsArray.getInt(i);
    }
    return frames;
  }
  // LynxUIMethod END

  // ForegroundListener interface START
  @Override
  public void onLynxViewEnterForeground() {
    if (mIgnoreLynxLifecycle) {
      return;
    }
    if (mAnimaXPlayer != null) {
      mAnimaXPlayer.enterForeground();
    }
  }

  @Override
  public void onLynxViewEnterBackground() {
    if (mIgnoreLynxLifecycle) {
      return;
    }
    if (mAnimaXPlayer != null) {
      mAnimaXPlayer.enterBackground();
    }
  }
  // ForegroundListener interface END

  private void reportMotionEvent(String src) {
    if (src == null || src.isEmpty() || mContext == null || !mContext.enableEventReporter()
        || mHasReportMotionEvent || !enableMotionEventReport()) {
      return;
    }

    mHasReportMotionEvent = true;
    LynxEventReporter.onEvent("lynxsdk_motion_ui_event", mContext.getInstanceId(), () -> {
      Map<String, Object> data = new HashMap<>();
      data.put("component_name", "animax-view");
      data.put("src", src);
      return data;
    });
  }

  private boolean enableMotionEventReport() {
    ILynxTrailService service = LynxServiceCenter.inst().getService(ILynxTrailService.class);
    if (service == null) {
      return false;
    }
    String value = service.stringValueForTrailKey("enable_motion_ui_report");
    return "true".equals(value);
  }

  @Override
  public void onLayoutUpdated() {
    super.onLayoutUpdated();
    if (mAnimaXContainerView != null && mAnimaXView != null) {
      int leftBorderAndPadding = getBorderLeftWidth() + getPaddingLeft();
      int topBorderAndPadding = getBorderTopWidth() + getPaddingTop();
      int rightBorderAndPadding = getBorderRightWidth() + getPaddingRight();
      int bottomBorderAndPadding = getBorderBottomWidth() + getPaddingBottom();
      mAnimaXContainerView.setPadding(
          leftBorderAndPadding, topBorderAndPadding, rightBorderAndPadding, bottomBorderAndPadding);
      mAnimaXView.requestLayout();
    }
  }

  private void tryToAddClip(final Canvas canvas) {
    BackgroundDrawable drawable =
        getLynxBackground() != null ? getLynxBackground().getDrawable() : null;
    if (drawable == null) {
      return;
    }
    Path path = drawable.getInnerClipPathForBorderRadius();
    if (path != null) {
      canvas.clipPath(path);
    } else if (getSkewX() != 0 || getSkewY() != 0) {
      // Shearing transformation should clip bounds manually.
      canvas.clipRect(getClipBounds());
    }
  }
}
