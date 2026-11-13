// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer;

import android.content.Context;
import android.graphics.Color;
import android.os.Handler;
import android.os.Looper;
import android.text.Layout;
import android.text.StaticLayout;
import android.text.TextPaint;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.LynxTransferListener;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.LynxViewClient;
import java.util.ArrayList;
import java.util.Collections;
import java.util.IdentityHashMap;
import java.util.LinkedHashMap;
import java.util.Map;

/** Local chat controller; native rows contain only real transferred Lynx views. */
final class TransferChatHost extends FrameLayout implements LynxTransferListener {
  private static final String TAG = "TransferChat";
  private final LynxView mLynxView;
  private final LinearLayout mRows;
  private final ScrollView mScroll;
  private final EditText mInput;
  private final Button mSend;
  private final Handler mHandler = new Handler(Looper.getMainLooper());
  private final Map<String, Handle> mHandles = new LinkedHashMap<>();
  private final Map<View, Handle> mViews = new IdentityHashMap<>();
  private int mGeneration;
  private int mNextId;
  private boolean mLoaded;

  TransferChatHost(Context context, LynxView lynxView) {
    super(context);
    mLynxView = lynxView;
    // Keep the shared page measured, but outside the visible chat surface.
    lynxView.setTranslationY(-getResources().getDisplayMetrics().heightPixels - dp(100));
    addView(lynxView, new FrameLayout.LayoutParams(LayoutParams.MATCH_PARENT, dp(1)));
    LinearLayout chat = new LinearLayout(context);
    chat.setOrientation(LinearLayout.VERTICAL);
    chat.setBackgroundColor(Color.rgb(245, 247, 250));
    addView(chat, new FrameLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));
    mScroll = new ScrollView(context);
    mScroll.setFillViewport(true);
    mRows = new LinearLayout(context);
    mRows.setOrientation(LinearLayout.VERTICAL);
    mRows.setPadding(dp(12), dp(12), dp(12), dp(12));
    mScroll.addView(mRows, new ScrollView.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
    chat.addView(mScroll, new LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, 0, 1));
    LinearLayout composer = new LinearLayout(context);
    composer.setGravity(Gravity.CENTER_VERTICAL);
    composer.setPadding(dp(8), dp(8), dp(8), dp(8));
    mInput = new EditText(context);
    mInput.setHint("Message (local demo)");
    mInput.setMaxLines(3);
    composer.addView(mInput, new LinearLayout.LayoutParams(0, LayoutParams.WRAP_CONTENT, 1));
    mSend = new Button(context);
    mSend.setText("Send");
    composer.addView(mSend);
    Button clear = new Button(context);
    clear.setText("Clear");
    composer.addView(clear);
    chat.addView(composer);
    mSend.setEnabled(false);
    clear.setEnabled(false);
    mSend.setOnClickListener(v -> send());
    clear.setOnClickListener(v -> {
      for (Handle handle : new ArrayList<>(mHandles.values())) {
        handle.destroy();
      }
      Log.i(TAG, "clear");
    });
    mRows.addOnLayoutChangeListener((v, l, t, r, b, ol, ot, or, ob) -> {
      if (r - l != or - ol) {
        for (Handle handle : mHandles.values()) {
          handle.resize();
        }
      }
      lynxView.post(lynxView::requestLayout);
    });
    lynxView.registerTransferListener(this);
    lynxView.addLynxViewClient(new LynxViewClient() {
      @Override
      public void onLoadSuccess() {
        post(() -> {
          if (mLoaded) {
            return;
          }
          mLoaded = true;
          mSend.setEnabled(true);
          clear.setEnabled(true);
          Log.i(TAG, "template loaded; create initial conversation");
          addBubble("user", "Hello! How does this chat work?");
          addBubble("assistant", "Hi! These bubbles are rendered by Lynx and mounted in native rows. Send a message to try a local streamed reply.");
        });
      }
    });
  }

  private void send() {
    String text = mInput.getText().toString().trim();
    if (!mLoaded || text.isEmpty()) {
      return;
    }
    mInput.setText("");
    addBubble("user", text);
    Handle assistant = addBubble("assistant", "");
    String answer = "You said: \"" + text
        + "\". This deterministic reply streams locally every 80 ms. Each bubble is a real transfer-view inside a native chat row.";
    assistant.stream = new Runnable() {
      private int end;
      @Override
      public void run() {
        if (mHandles.get(assistant.id) != assistant || !mLoaded) {
          return;
        }
        end = Math.min(answer.length(), end + 4);
        assistant.setProps("assistant", answer.substring(0, end));
        if (end < answer.length()) {
          mHandler.postDelayed(this, 80);
        } else {
          Log.i(TAG, "stream complete " + assistant.id);
        }
      }
    };
    mHandler.postDelayed(assistant.stream, 80);
  }

  private Handle addBubble(String role, String text) {
    Handle handle = createTransfer("chat-" + ++mNextId, "ChatBubble", props(role, text));
    FrameLayout slot = new FrameLayout(getContext());
    mRows.addView(slot, new LinearLayout.LayoutParams(1, 1));
    handle.attach(slot);
    return handle;
  }

  // Proof-of-concept controller API, not an SDK ABI. Calls are on the UI thread after load.
  Handle createTransfer(String id, String componentName, Map<String, Object> initialProps) {
    Handle handle = new Handle(id, ++mGeneration, initialProps);
    mHandles.put(id, handle);
    Map<String, Object> payload = handle.scope();
    payload.put("componentName", componentName);
    payload.put("initialProps", initialProps);
    payload.put("propsVersion", handle.propsVersion);
    emit("__CreateTransfer", payload);
    Log.i(TAG, "create " + id + " generation=" + handle.generation);
    return handle;
  }

  final class Handle {
    final String id;
    final int generation;
    int propsVersion = 1;
    String role;
    String text;
    FrameLayout slot;
    View view;
    Runnable stream;

    Handle(String id, int generation, Map<String, Object> initialProps) {
      this.id = id;
      this.generation = generation;
      role = (String) initialProps.get("role");
      text = (String) initialProps.get("text");
    }

    Map<String, Object> scope() {
      Map<String, Object> key = new LinkedHashMap<>();
      key.put("transferId", id);
      key.put("generation", generation);
      Map<String, Object> payload = new LinkedHashMap<>();
      payload.put("reloadVersion", 0);
      payload.put("key", key);
      return payload;
    }

    void attach(FrameLayout host) {
      slot = host;
      resize();
      if (view != null) {
        slot.addView(view, new FrameLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));
      }
      Log.i(TAG, "attach " + id);
    }

    void resize() {
      if (slot == null) {
        return;
      }
      TextPaint paint = new TextPaint(TextPaint.ANTI_ALIAS_FLAG);
      paint.setTextSize(dp(16));
      int available = mRows.getWidth() > 0 ? mRows.getWidth()
          : getResources().getDisplayMetrics().widthPixels;
      int maxWidth = Math.max(dp(49), Math.min((int) (available * .85f), available - dp(24)));
      int width = Math.min(maxWidth, Math.max(dp(96), (int) Math.ceil(Layout.getDesiredWidth(text, paint)) + dp(48)));
      StaticLayout layout = new StaticLayout(text, paint, Math.max(1, width - dp(48)),
          Layout.Alignment.ALIGN_NORMAL, 1, 0, false);
      LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(width, layout.getLineCount() * dp(24) + dp(48));
      params.gravity = "user".equals(role) ? Gravity.END : Gravity.START;
      params.bottomMargin = dp(12);
      slot.setLayoutParams(params);
      mLynxView.post(mLynxView::requestLayout);
      mScroll.post(() -> mScroll.fullScroll(View.FOCUS_DOWN));
    }

    void setProps(String role, String text) {
      if (mHandles.get(id) != this) {
        return;
      }
      this.role = role;
      this.text = text;
      resize();
      Map<String, Object> payload = scope();
      payload.put("props", props(role, text));
      payload.put("propsVersion", ++propsVersion);
      emit("__SetTransferProps", payload);
      Log.d(TAG, "setProps " + id + " version=" + propsVersion);
    }

    void destroy() {
      if (mHandles.get(id) != this) {
        return;
      }
      mHandles.remove(id);
      if (stream != null) {
        mHandler.removeCallbacks(stream);
      }
      if (slot != null) {
        mRows.removeView(slot);
      }
      emit("__DestroyTransfer", scope());
      Log.i(TAG, "destroy " + id + " generation=" + generation);
    }
  }

  private boolean matches(Handle handle, ReadableMap dataset) {
    ReadableMap meta = dataset == null ? null : dataset.getMap("__transferMeta", null);
    ReadableMap key = meta == null ? null : meta.getMap("key", null);
    return handle != null && key != null && meta.getDouble("reloadVersion", -1) == 0
        && handle.id.equals(key.getString("transferId", null))
        && key.getDouble("generation", -1) == handle.generation;
  }

  @Override
  public boolean onCreate(@NonNull String id, @NonNull View view, @Nullable ReadableMap dataset) {
    Handle handle = mHandles.get(id);
    if (!matches(handle, dataset) || handle.view != null) {
      Log.w(TAG, "reject stale/unmatched onCreate " + id);
      return false;
    }
    handle.view = view;
    mViews.put(view, handle);
    view.addOnLayoutChangeListener((v, l, t, r, b, ol, ot, or, ob) -> mLynxView.post(mLynxView::requestLayout));
    if (handle.slot != null) {
      handle.slot.addView(view, new FrameLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));
    }
    Log.i(TAG, "onCreate " + id + " generation=" + handle.generation);
    return true;
  }

  @Override
  public void onDatasetUpdate(@NonNull String id, @Nullable ReadableMap dataset) {
    if (matches(mHandles.get(id), dataset)) {
      Log.d(TAG, "onDatasetUpdate " + id);
    }
  }

  @Override
  public void onRemove(@NonNull String id, @NonNull View view) {
    Handle handle = mViews.remove(view);
    if (handle != null) {
      if (handle.slot != null) {
        handle.slot.removeView(view);
      }
      handle.view = null;
      // A failed background commit can remove the view before the native owner.
      // Defer cleanup to avoid re-entering the JS engine during UI teardown.
      mHandler.post(handle::destroy);
      Log.i(TAG, "onRemove " + id + " generation=" + handle.generation);
    }
  }

  @Override
  protected void onDetachedFromWindow() {
    mHandler.removeCallbacksAndMessages(null);
    mLoaded = false;
    Log.i(TAG, "host detached; streams stopped");
    super.onDetachedFromWindow();
  }

  private void emit(String event, Map<String, Object> payload) {
    mLynxView.sendGlobalEventToLepus(event, Collections.<Object>singletonList(payload));
  }

  private static Map<String, Object> props(String role, String text) {
    Map<String, Object> props = new LinkedHashMap<>();
    props.put("role", role);
    props.put("text", text);
    return props;
  }

  private int dp(int value) {
    return Math.round(value * getResources().getDisplayMetrics().density);
  }
}
