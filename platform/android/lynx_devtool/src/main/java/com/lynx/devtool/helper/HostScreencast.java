// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.helper;

import android.graphics.Rect;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.view.View;
import com.lynx.debugrouter.DebugRouter;
import com.lynx.debugrouter.DebugRouterGlobalHandler;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/** One leased preview per host process, independent of the inspected card. */
public final class HostScreencast implements DebugRouterGlobalHandler, ScreencastSession.Host {
  private static final String CHANNEL = "LynxHostScreencast";
  private static final HostScreencast INSTANCE = new HostScreencast();
  private final Handler mMain = new Handler(Looper.getMainLooper());
  private final Map<Integer, WeakReference<LynxView>> mViews = new HashMap<>();
  private WeakReference<View> mRoot = new WeakReference<>(null);
  private ScreencastSession mSession;
  private String mToken = "";
  private String mLastCards = "";
  private boolean mLastAvailable;
  private JSONArray mCards = new JSONArray();
  private final Rect mCaptureRegion = new Rect();
  private long mGeometryRevision;
  private long mLeaseUntil;
  private int mStreamId;
  private String mMode = "host";
  private int mSelectedSession;
  private boolean mRegistered;
  private final Runnable mLeaseCheck = new Runnable() {
    @Override
    public void run() {
      if (mSession == null)
        return;
      if (SystemClock.uptimeMillis() >= mLeaseUntil) {
        stop();
      } else {
        mMain.postDelayed(this, 1000);
      }
    }
  };

  public static void attach(int sessionId, LynxView view) {
    INSTANCE.mMain.post(() -> {
      INSTANCE.mViews.put(sessionId, new WeakReference<>(view));
      if (!INSTANCE.mRegistered) {
        DebugRouter.getInstance().addGlobalHandler(INSTANCE);
        INSTANCE.mRegistered = true;
      }
    });
  }

  public static void detach(int sessionId) {
    INSTANCE.mMain.post(() -> INSTANCE.mViews.remove(sessionId));
  }

  @Override
  public void openCard(String url) {}

  @Override
  public void onMessage(String type, int sessionId, String message) {
    if (!CHANNEL.equals(type))
      return;
    mMain.post(() -> handle(message));
  }

  private void handle(String message) {
    try {
      JSONObject request = new JSONObject(message);
      String token = request.optString("token");
      if (token.isEmpty() || token.length() > 128)
        return;
      String method = request.optString("method");
      if ("start".equals(method)) {
        if (mSession != null && !mToken.equals(token)) {
          emit(new JSONObject().put("event", "busy").put("token", token));
          return;
        }
        String mode = request.optString("mode");
        if (!"card".equals(mode) && !"region".equals(mode)) {
          mode = "host";
        }
        int selected = "host".equals(mode) ? 0 : request.optInt("selectedSession");
        if (mSession != null && (!mode.equals(mMode) || selected != mSelectedSession)) {
          stop();
        }
        if (mSession == null) {
          mToken = token;
          mMode = mode;
          mSelectedSession = selected;
          mLastCards = "";
          mStreamId++;
          boolean card = "card".equals(mode);
          boolean fullWindow = "host".equals(mode);
          mSession =
              new ScreencastSession(this, card ? ScreencastSnapshot.ISOLATED_VIEW : "fullscreen",
                  fullWindow ? 540 : 1080, fullWindow ? 960 : 1920, fullWindow ? 65 : 85);
          mMain.post(mLeaseCheck);
        }
        mLeaseUntil = SystemClock.uptimeMillis() + 6000;
        emit(new JSONObject()
                 .put("event", "started")
                 .put("token", token)
                 .put("cardPreview", true)
                 .put("regionPreview", true)
                 .put("mode", mMode)
                 .put("selectedSession", mSelectedSession)
                 .put("streamId", mStreamId));
      } else if (mSession != null && mToken.equals(token)) {
        if ("stop".equals(method)) {
          stop();
        } else if ("heartbeat".equals(method)) {
          mLeaseUntil = SystemClock.uptimeMillis() + 6000;
        } else if ("ack".equals(method)) {
          if (!request.has("streamId") || request.optInt("streamId") == mStreamId) {
            mSession.acknowledge(request.optInt("frameId"), request.optBoolean("applied", true));
          }
        } else if ("inspect".equals(method) && "card".equals(mMode)
            && request.optInt("streamId") == mStreamId
            && request.optInt("selectedSession") == mSelectedSession) {
          inspect(request);
        }
      }
    } catch (JSONException error) {
      LLog.w("HostScreencast", "Invalid preview command: " + error);
    }
  }

  private void inspect(JSONObject request) throws JSONException {
    WeakReference<LynxView> reference = mViews.get(mSelectedSession);
    LynxView view = reference == null ? null : reference.get();
    int nodeId = 0;
    JSONObject bounds = null;
    double x = request.optDouble("x", -1);
    double y = request.optDouble("y", -1);
    if (view != null && view.isAttachedToWindow() && view.lynxUIRenderer() != null && x >= 0
        && y >= 0 && x < view.getWidth() && y < view.getHeight()) {
      nodeId = view.lynxUIRenderer().getNodeForLocation((float) x, (float) y, "lynxview");
      if (view.getLynxContext() != null && view.getLynxContext().getLynxUIOwner() != null) {
        LynxBaseUI ui = view.getLynxContext().getLynxUIOwner().getNode(nodeId);
        if (ui != null) {
          Rect rect = ui.getBoundingClientRect();
          bounds = new JSONObject()
                       .put("x", rect.left)
                       .put("y", rect.top)
                       .put("width", rect.width())
                       .put("height", rect.height());
        }
      }
    }
    emit(new JSONObject()
             .put("event", "inspected")
             .put("token", mToken)
             .put("mode", mMode)
             .put("streamId", mStreamId)
             .put("selectedSession", mSelectedSession)
             .put("requestId", request.optInt("requestId"))
             .put("nodeId", nodeId)
             .put("bounds", bounds));
  }

  private void stop() {
    if (mSession != null)
      mSession.stop();
    mSession = null;
    mMain.removeCallbacks(mLeaseCheck);
    mToken = "";
    mLastCards = "";
  }

  @Override
  public View getView() {
    View root = mRoot.get();
    if (root == null || !root.isAttachedToWindow() || !root.hasWindowFocus())
      root = null;
    Iterator<Map.Entry<Integer, WeakReference<LynxView>>> iterator = mViews.entrySet().iterator();
    while (iterator.hasNext()) {
      LynxView view = iterator.next().getValue().get();
      if (view == null) {
        iterator.remove();
      } else if (view.isAttachedToWindow() && view.hasWindowFocus()) {
        root = view.getRootView();
      }
    }
    mRoot = new WeakReference<>(root);
    View target = root;
    if ("card".equals(mMode)) {
      WeakReference<LynxView> reference = mViews.get(mSelectedSession);
      LynxView selected = reference == null ? null : reference.get();
      target = selected != null && selected.isAttachedToWindow()
              && selected.getWindowVisibility() == View.VISIBLE && selected.getWidth() > 0
              && selected.getHeight() > 0
          ? selected
          : null;
    }
    mCaptureRegion.setEmpty();
    boolean region = "region".equals(mMode);
    try {
      JSONArray cards = new JSONArray();
      if (root != null) {
        int[] origin = new int[2];
        root.getLocationOnScreen(origin);
        for (Map.Entry<Integer, WeakReference<LynxView>> entry : mViews.entrySet()) {
          LynxView view = entry.getValue().get();
          Rect rect = new Rect();
          if (view == null || view.getRootView() != root || !view.isShown() || view.getAlpha() <= 0
              || !view.getGlobalVisibleRect(rect))
            continue;
          rect.offset(origin[0], origin[1]);
          if (region && entry.getKey() == mSelectedSession) {
            mCaptureRegion.set(rect);
          }
          cards.put(new JSONObject()
                        .put("sessionId", entry.getKey())
                        .put("x", rect.left)
                        .put("y", rect.top)
                        .put("width", rect.width())
                        .put("height", rect.height()));
        }
      }
      String serialized = cards.toString();
      if (region && mCaptureRegion.isEmpty()) {
        target = null;
      }
      boolean available = target != null;
      if (!serialized.equals(mLastCards) || available != mLastAvailable) {
        mLastCards = serialized;
        mLastAvailable = available;
        mCards = cards;
        mGeometryRevision++;
        emit(new JSONObject()
                 .put("event", "cards")
                 .put("token", mToken)
                 .put("mode", mMode)
                 .put("selectedSession", mSelectedSession)
                 .put("streamId", mStreamId)
                 .put("cards", cards)
                 .put("available", available));
      }
    } catch (JSONException error) {
      LLog.w("HostScreencast", "Cannot serialize card geometry: " + error);
    }
    return target;
  }

  @Override
  public Rect getCaptureRegion() {
    return "region".equals(mMode) ? mCaptureRegion : null;
  }

  @Override
  public long getRevision() {
    return mGeometryRevision;
  }

  @Override
  public void sendFrame(String data, float width, float height, double timestamp, int[] frame) {
    try {
      View root = mRoot.get();
      float density = root == null ? 1 : root.getResources().getDisplayMetrics().density;
      JSONObject message = new JSONObject()
                               .put("event", "frame")
                               .put("token", mToken)
                               .put("streamId", mStreamId)
                               .put("mode", mMode)
                               .put("selectedSession", mSelectedSession)
                               .put("frameId", frame[0])
                               .put("data", data)
                               .put("width", width * density)
                               .put("height", height * density)
                               .put("cards", mCards)
                               .put("timestamp", timestamp);
      if ("region".equals(mMode)) {
        message.put(
            "origin", new JSONObject().put("x", mCaptureRegion.left).put("y", mCaptureRegion.top));
      }
      emit(message);
    } catch (JSONException error) {
      LLog.w("HostScreencast", "Cannot serialize frame: " + error);
    }
  }

  private void emit(JSONObject message) {
    DebugRouter.getInstance().sendDataAsync(CHANNEL, -1, message.toString());
  }
}
