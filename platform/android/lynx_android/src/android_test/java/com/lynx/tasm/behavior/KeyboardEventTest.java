// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import android.app.Activity;
import android.content.Context;
import android.os.Build;
import android.view.View;
import android.view.Window;
import android.view.WindowInsetsAnimation;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.SdkSuppress;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.tasm.LynxView;
import java.lang.reflect.Field;
import java.util.Map;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
@SdkSuppress(minSdkVersion = Build.VERSION_CODES.R)
public class KeyboardEventTest {
  private Context mContext;
  private LynxContext mLynxContext;
  private RecordingHostView mHostView;
  private Map<View, ?> mDispatchers;

  private static class RecordingHostView extends View {
    WindowInsetsAnimation.Callback callback;

    RecordingHostView(Context context) {
      super(context);
    }

    @Override
    public void setWindowInsetsAnimationCallback(WindowInsetsAnimation.Callback callback) {
      super.setWindowInsetsAnimationCallback(callback);
      this.callback = callback;
    }
  }

  @SuppressWarnings("unchecked")
  @Before
  public void setUp() throws Exception {
    mContext = InstrumentationRegistry.getInstrumentation().getTargetContext();
    Field field = KeyboardEvent.class.getDeclaredField("sInsetsAnimationDispatchers");
    field.setAccessible(true);
    mDispatchers = (Map<View, ?>) field.get(null);
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      mHostView = new RecordingHostView(mContext);
      Activity activity = mock(Activity.class);
      Window window = mock(Window.class);
      when(activity.getWindow()).thenReturn(window);
      when(window.getDecorView()).thenReturn(mHostView);
      mLynxContext = mock(LynxContext.class);
      when(mLynxContext.getBaseContext()).thenReturn(activity);
      when(mLynxContext.getScreenMetrics()).thenReturn(mContext.getResources().getDisplayMetrics());
    });
  }

  @After
  public void tearDown() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      // Also isolate failures on implementations that skip unregistering.
      mDispatchers.remove(mHostView);
      mHostView.setWindowInsetsAnimationCallback(null);
    });
  }

  @Test
  public void stopWithoutRegistrationIsSafe() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      KeyboardEvent event = new KeyboardEvent(mLynxContext);
      event.stop();
      event.stop();
      assertFalse(event.isStart());
      assertFalse(mDispatchers.containsKey(mHostView));
      assertNull(mHostView.callback);
    });
  }

  @Test
  public void stopNotStartedEventReleasesInsetsAnimationDispatcher() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      KeyboardEvent event = new KeyboardEvent(mLynxContext);
      View input = new View(mContext);
      View lynxView = new View(mContext);
      event.inputDidLayoutForOwner(input, input, lynxView, true, 0f);

      assertFalse(event.isStart());
      assertNotNull(mDispatchers.get(mHostView));
      assertNotNull(mHostView.callback);

      event.stop();
      event.stop();

      assertFalse(mDispatchers.containsKey(mHostView));
      assertNull(mHostView.callback);
    });
  }

  @Test
  public void stopPreservesDispatcherUntilLastContextIsReleased() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      KeyboardEvent first = new KeyboardEvent(mLynxContext);
      KeyboardEvent second = new KeyboardEvent(mLynxContext);
      View firstInput = new View(mContext);
      View secondInput = new View(mContext);
      first.inputDidLayoutForOwner(firstInput, firstInput, firstInput, true, 0f);
      Object dispatcher = mDispatchers.get(mHostView);
      WindowInsetsAnimation.Callback callback = mHostView.callback;
      second.inputDidLayoutForOwner(secondInput, secondInput, secondInput, true, 0f);

      assertNotNull(dispatcher);
      assertSame(dispatcher, mDispatchers.get(mHostView));
      first.stop();
      first.stop();
      assertSame(dispatcher, mDispatchers.get(mHostView));
      assertSame(callback, mHostView.callback);

      second.stop();
      second.stop();
      assertFalse(mDispatchers.containsKey(mHostView));
      assertNull(mHostView.callback);
    });
  }

  @Test
  public void destroyReleasesNotStartedKeyboardEventOnEveryCycle() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      for (int cycle = 0; cycle < 5; cycle++) {
        LynxView lynxView = new LynxView(mContext) {
          @Override
          public LynxContext getLynxContext() {
            return KeyboardEventTest.this.mLynxContext;
          }
        };
        KeyboardEvent event = lynxView.getKeyboardEvent();
        View input = new View(mContext);
        event.inputDidLayoutForOwner(input, input, lynxView, true, 0f);

        assertFalse(event.isStart());
        assertNotNull(mDispatchers.get(mHostView));
        lynxView.destroy();
        lynxView.destroy();
        assertFalse(mDispatchers.containsKey(mHostView));
        assertNull(mHostView.callback);
      }
    });
  }
}
