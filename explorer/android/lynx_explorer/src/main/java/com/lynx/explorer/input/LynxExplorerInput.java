// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.input;

import android.content.Context;
import android.graphics.Color;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.Gravity;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import androidx.appcompat.widget.AppCompatEditText;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.LynxUIMethod;
import com.lynx.tasm.behavior.LynxUIMethodConstants;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.event.LynxCustomEvent;
import java.util.HashMap;
import java.util.Map;

public class LynxExplorerInput extends LynxUI<AppCompatEditText> {
  public LynxExplorerInput(LynxContext context) {
    super(context);
  }

  @Override
  protected AppCompatEditText createView(Context context) {
    AppCompatEditText view = new AppCompatEditText(context);
    view.setLines(1);
    view.setSingleLine();
    view.setGravity(Gravity.CENTER_VERTICAL);
    view.setBackground(null);
    view.setImeOptions(EditorInfo.IME_ACTION_NONE);
    view.setHorizontallyScrolling(true);
    view.setPadding(0, 0, 0, 0);
    view.setTextSize(14);
    view.addTextChangedListener(new TextWatcher() {
      @Override
      public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count) {}

      @Override
      public void afterTextChanged(Editable s) {
        emitEvent("input", new HashMap<String, Object>() {
          { put("value", s.toString()); }
        });
      }
    });
    view.setOnFocusChangeListener(new View.OnFocusChangeListener() {
      @Override
      public void onFocusChange(View v, boolean hasFocus) {
        if (!hasFocus) {
          emitEvent("blur", null);
        }
      }
    });
    return view;
  }

  @Override
  public void onLayoutUpdated() {
    super.onLayoutUpdated();
    int paddingTop = mPaddingTop + mBorderTopWidth;
    int paddingBottom = mPaddingBottom + mBorderBottomWidth;
    int paddingLeft = mPaddingLeft + mBorderLeftWidth;
    int paddingRight = mPaddingRight + mBorderRightWidth;
    mView.setPadding(paddingLeft, paddingTop, paddingRight, paddingBottom);
  }

  @LynxProp(name = "value")
  public void setValue(String value) {
    if (!value.equals(mView.getText().toString())) {
      mView.setText(value);
    }
  }

  @LynxUIMethod
  public void focus(ReadableMap params, Callback callback) {
    if (mView.requestFocus()) {
      if (showSoftInput()) {
        callback.invoke(LynxUIMethodConstants.SUCCESS);
      } else {
        callback.invoke(LynxUIMethodConstants.UNKNOWN, "fail to show keyboard");
      }
    } else {
      callback.invoke(LynxUIMethodConstants.UNKNOWN, "fail to focus");
    }
  }

  private boolean showSoftInput() {
    InputMethodManager imm =
        (InputMethodManager) getLynxContext().getSystemService(Context.INPUT_METHOD_SERVICE);
    return imm.showSoftInput(mView, InputMethodManager.SHOW_IMPLICIT, null);
  }

  @LynxProp(name = "placeholder")
  public void setPlaceHolder(String value) {
    mView.setHint(value);
  }

  @LynxProp(name = "text-color")
  public void setTextColor(String value) {
    String textColor = "#" + value;
    String hintColor = "#40" + value;
    mView.setHintTextColor(Color.parseColor(hintColor));
    mView.setTextColor(Color.parseColor(textColor));
  }

  private void emitEvent(String name, Map<String, Object> value) {
    LynxCustomEvent detail = new LynxCustomEvent(getSign(), name);
    if (value != null) {
      for (Map.Entry<String, Object> entry : value.entrySet()) {
        detail.addDetail(entry.getKey(), entry.getValue());
      }
    }

    getLynxContext().getEventEmitter().sendCustomEvent(detail);
  }
}
