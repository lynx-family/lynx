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
  private long mGeometryRevision;
  private long mLeaseUntil;
  private int mStreamId;
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
        if (mSession == null) {
          mToken = token;
          mLastCards = "";
          mStreamId++;
          mSession = new ScreencastSession(this, 540, 960, 65);
          mMain.post(mLeaseCheck);
        }
        mLeaseUntil = SystemClock.uptimeMillis() + 6000;
        emit(new JSONObject()
                 .put("event", "started")
                 .put("token", token)
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
        }
      }
    } catch (JSONException error) {
      LLog.w("HostScreencast", "Invalid preview command: " + error);
    }
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
          cards.put(new JSONObject()
                        .put("sessionId", entry.getKey())
                        .put("x", rect.left)
                        .put("y", rect.top)
                        .put("width", rect.width())
                        .put("height", rect.height()));
        }
      }
      String serialized = cards.toString();
      boolean available = root != null;
      if (!serialized.equals(mLastCards) || available != mLastAvailable) {
        mLastCards = serialized;
        mLastAvailable = available;
        mCards = cards;
        mGeometryRevision++;
        emit(new JSONObject()
                 .put("event", "cards")
                 .put("token", mToken)
                 .put("streamId", mStreamId)
                 .put("cards", cards)
                 .put("available", available));
      }
    } catch (JSONException error) {
      LLog.w("HostScreencast", "Cannot serialize card geometry: " + error);
    }
    return root;
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
      emit(new JSONObject()
               .put("event", "frame")
               .put("token", mToken)
               .put("streamId", mStreamId)
               .put("frameId", frame[0])
               .put("data", data)
               .put("width", width * density)
               .put("height", height * density)
               .put("cards", mCards)
               .put("timestamp", timestamp));
    } catch (JSONException error) {
      LLog.w("HostScreencast", "Cannot serialize frame: " + error);
    }
  }

  private void emit(JSONObject message) {
    DebugRouter.getInstance().sendDataAsync(CHANNEL, -1, message.toString());
  }
}
