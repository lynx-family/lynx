// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import android.app.Application;
import android.content.Context;
import android.view.View;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.widget.EditText;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxView;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class DevToolPlatformAndroidDelegateTest {
  private Context mContext;
  private LynxView mLynxView;
  private DevToolPlatformAndroidDelegate mPlatformDelegate;

  @Before
  public void setUp() {
    mContext =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    LynxEnv.inst().init((Application) mContext, null, null, null, null);
    LynxDevtoolEnv.inst().init(mContext);

    mLynxView = mock(LynxView.class);
    mPlatformDelegate = new DevToolPlatformAndroidDelegate(mLynxView);
  }

  @Test
  public void insertTextCommitsToFocusedEditText() {
    EditText editText = new EditText(mContext);
    editText.setText("ac");
    editText.setSelection(1);
    when(mLynxView.findFocus()).thenReturn(editText);

    mPlatformDelegate.insertText("b");

    assertEquals("abc", editText.getText().toString());
  }

  @Test
  public void insertTextFallsBackToRootFocusedEditText() {
    EditText editText = new EditText(mContext);
    editText.setText("acd");
    editText.setSelection(1, 2);
    View rootView = mock(View.class);
    when(rootView.findFocus()).thenReturn(editText);
    when(mLynxView.findFocus()).thenReturn(null);
    when(mLynxView.getRootView()).thenReturn(rootView);

    mPlatformDelegate.insertText("b");

    assertEquals("abd", editText.getText().toString());
  }

  @Test
  public void insertTextFinishesComposingText() {
    ComposingEditText editText = new ComposingEditText(mContext);
    when(mLynxView.findFocus()).thenReturn(editText);

    mPlatformDelegate.insertText("b");

    assertEquals("b", editText.getText().toString());
    assertEquals(-1, BaseInputConnection.getComposingSpanStart(editText.getText()));
    assertEquals(-1, BaseInputConnection.getComposingSpanEnd(editText.getText()));
  }

  private static class ComposingEditText extends EditText {
    ComposingEditText(Context context) {
      super(context);
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
      return new BaseInputConnection(this, true) {
        @Override
        public boolean commitText(CharSequence text, int newCursorPosition) {
          ComposingEditText.this.getText().append(text);
          BaseInputConnection.setComposingSpans(ComposingEditText.this.getText());
          return true;
        }

        @Override
        public boolean finishComposingText() {
          BaseInputConnection.removeComposingSpans(ComposingEditText.this.getText());
          return true;
        }
      };
    }
  }
}
