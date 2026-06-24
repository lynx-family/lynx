// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.animax.ui;

import static org.mockito.Mockito.*;

import android.content.Context;
import android.graphics.SurfaceTexture;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.animax.ability.BaseAbility;
import com.lynx.animax.ability.Event;
import com.lynx.animax.ability.LynxAbility;
import com.lynx.animax.ability.NativeAbility;
import com.lynx.animax.base.bridge.ReadableMap;
import com.lynx.animax.listener.AnimaXErrorParam;
import com.lynx.animax.listener.AnimaXParam;
import com.lynx.animax.listener.AnimationListenerAdapter;
import com.lynx.animax.util.DeviceUtil;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import junit.framework.TestCase;
import org.junit.Before;
import org.junit.Test;
import org.mockito.InOrder;

public class LynxAnimaXViewTest extends TestCase {
  private static final long sTimeout = 1000;

  private String easyLottieJson;
  private boolean isNotCapable;

  @Before
  public void setUp() {
    easyLottieJson =
        "{\"v\":\"5.6.9\",\"fr\":25,\"ip\":0,\"op\":200,\"w\":1000,\"h\":1000,\"nm\":\"Composition 1\",\"ddd\":0,\"assets\":[],\"layers\":[{\"ddd\":0,\"ind\":1,\"ty\":1,\"nm\":\"Intermediate Blue Solid Color 1\",\"sr\":1,\"ks\":{\"o\":{\"a\":0,\"k\":100,\"ix\":11},\"r\":{\"a\":0,\"k\":0,\"ix\":10},\"p\":{\"a\":0,\"k\":[500,500,0],\"ix\":2},\"a\":{\"a\":0,\"k\":[500,500,0],\"ix\":1},\"s\":{\"a\":0,\"k\":[100,100,100],\"ix\":6}},\"ao\":0,\"sw\":1000,\"sh\":1000,\"sc\":\"#505fec\",\"ip\":0,\"op\":200,\"st\":0,\"bm\":0}],\"markers\":[]}";

    isNotCapable = !DeviceUtil.checkCapability(new NativeAbility());
  }

  @Test
  public void testLynxAbility() {
    LynxContext context = mock(LynxContext.class);
    LynxBaseUI ui = mock(LynxBaseUI.class);
    LynxAbility spyLynxAbility = spy(new LynxAbility(ui, context));

    AnimaXView animaXView = createAnimaXView((BaseAbility) spyLynxAbility, true);
    if (animaXView == null) {
      return;
    }

    if (isNotCapable) {
      verify(spyLynxAbility, times(1)).sendEventOnUI(eq(Event.ERROR.ordinal()), anyMap());
    } else {
      InOrder inOrder = inOrder(spyLynxAbility);
      inOrder.verify(spyLynxAbility).sendEvent(eq(Event.READY.ordinal()), any(ReadableMap.class));
      inOrder.verify(spyLynxAbility).sendEvent(eq(Event.START.ordinal()), any(ReadableMap.class));
    }
  }

  private AnimaXView createAnimaXView(BaseAbility ability, boolean autoplay) {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    AnimaXContext animaxContext = new AnimaXContext.Builder(ability, context).build();

    AnimaXView view = new AnimaXView(animaxContext);
    final CountDownLatch latch = new CountDownLatch(1);
    view.addAnimationListener(new AnimationListenerAdapter() {
      @Override
      public void onReady(AnimaXParam param) {
        if (!autoplay) {
          latch.countDown();
        }
      }

      @Override
      public void onStart(AnimaXParam param) {
        if (autoplay) {
          latch.countDown();
        }
      }

      @Override
      public void onError(AnimaXErrorParam param) {
        if (isNotCapable) {
          latch.countDown();
        }
      }
    });
    configAutoPlayLoop(view, autoplay);

    try {
      // wait for 3 seconds, if not ready or error, means the emulator may occur device error.
      boolean completed = latch.await(3, TimeUnit.SECONDS);
      if (!completed) {
        return null;
      }
    } catch (InterruptedException e) {
      e.printStackTrace();
      return null;
    }

    return view;
  }

  private void configAutoPlayLoop(AnimaXView animaXView, boolean autoplay) {
    animaXView.setLoop(true);
    animaXView.setAutoPlay(autoplay);
    animaXView.setObjectFit(ObjectFit.CENTER);
    animaXView.onSurfaceTextureAvailable(mock(SurfaceTexture.class), 100, 100);
    animaXView.setJson(easyLottieJson);
  }
}
