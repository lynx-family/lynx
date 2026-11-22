// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import android.app.Application;
import android.content.Context;
import android.view.MotionEvent;
import android.view.View;
import android.widget.FrameLayout;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.tasm.behavior.BehaviorRegistry;
import com.lynx.tasm.behavior.LynxUIRenderer;
import com.lynx.tasm.group.ILynxViewGroup;
import com.lynx.tasm.group.LynxHost;
import com.lynx.tasm.group.LynxViewGroupBuilder;
import com.lynx.tasm.image.model.LynxImageFetcher;
import com.lynx.tasm.loader.LynxFontFaceLoader;
import com.lynx.tasm.service.ILynxImageService;
import com.lynx.tasm.service.LynxServiceCenter;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxPageViewTest {
  private Context mContext;

  private static Object field(LynxTemplateRender render, String name) {
    try {
      java.lang.reflect.Field field = LynxTemplateRender.class.getDeclaredField(name);
      field.setAccessible(true);
      return field.get(render);
    } catch (ReflectiveOperationException error) {
      throw new AssertionError(error);
    }
  }

  private static com.lynx.tasm.core.JSProxy jsProxy(LynxPageView page) {
    return (com.lynx.tasm.core.JSProxy) field(page.getTemplateRender(), "mJSProxy");
  }

  private static long nativePtr(LynxTemplateRender render) {
    return (Long) field(render, "mNativePtr");
  }

  private static LynxHostEnvironment environment(LynxTemplateRender render) {
    return (LynxHostEnvironment) field(render, "mHostEnvironment");
  }

  private LynxHostEnvironment createHostEnvironment(
      LynxBackgroundRuntimeOptions options, BehaviorRegistry behaviors) {
    return new LynxHostEnvironment(options, behaviors);
  }

  private static LynxPageOption pageOption(ThreadStrategyForRendering strategy) {
    return pageOption(strategy, false);
  }

  private static LynxPageOption pageOption(
      ThreadStrategyForRendering strategy, boolean multiAsync) {
    return new LynxPageOption.Builder()
        .setThreadStrategyForRendering(strategy)
        .setEnableMultiAsyncThread(multiAsync)
        .build();
  }

  private static LynxPageOption uiOnlyOption(ThreadStrategyForRendering strategy) {
    return new LynxPageOption.Builder()
        .setThreadStrategyForRendering(strategy)
        .setEnableJSRuntime(false)
        .build();
  }

  private LynxPageView createTouchPage(LynxHost host, LynxUIRenderer renderer) {
    LynxPageView page = host.createPageView(mContext,
        new LynxPageOption.Builder()
            .setEnableJSRuntime(false)
            .setUIRendererCreator(() -> renderer)
            .build());
    page.layout(0, 0, 100, 100);
    return page;
  }

  private static boolean dispatchTouch(LynxPageView page, int action) {
    MotionEvent event = MotionEvent.obtain(0, 10, action, 10, 20, 0);
    try {
      return page.dispatchTouchEvent(event);
    } finally {
      event.recycle();
    }
  }

  @Test
  public void touchReachesLynxBeforeNativeChildrenWithoutDuplicateDispatch() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxHost host = new LynxViewGroupBuilder().buildHost();
      try {
        LynxUIRenderer renderer = spy(new LynxUIRenderer());
        LynxPageView page = createTouchPage(host, renderer);
        List<String> events = new ArrayList<>();
        doAnswer(invocation -> {
          MotionEvent event = invocation.getArgument(0);
          events.add("lynx:" + event.getActionMasked());
          event.setLocation(40, 50);
          return true;
        })
            .when(renderer)
            .onTouchEvent(any(MotionEvent.class), any());
        View child = new View(mContext) {
          @Override
          public boolean onTouchEvent(MotionEvent event) {
            events.add("child:" + event.getActionMasked());
            assertEquals(10, event.getX(), 0);
            assertEquals(20, event.getY(), 0);
            return true;
          }
        };
        page.addView(child);
        child.layout(0, 0, 100, 100);
        for (int action :
            new int[] {MotionEvent.ACTION_DOWN, MotionEvent.ACTION_MOVE, MotionEvent.ACTION_UP}) {
          assertTrue(dispatchTouch(page, action));
        }
        assertEquals(
            Arrays.asList("lynx:0", "child:0", "lynx:2", "child:2", "lynx:1", "child:1"), events);
        page.removeView(child);
        events.clear();
        assertTrue(dispatchTouch(page, MotionEvent.ACTION_DOWN));
        assertTrue(dispatchTouch(page, MotionEvent.ACTION_CANCEL));
        assertEquals(Arrays.asList("lynx:0", "lynx:3"), events);
      } finally {
        host.release();
      }
    });
  }

  @Test
  public void touchPoliciesControlNativeDispatchAndReportFailures() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxHost host = new LynxViewGroupBuilder().buildHost();
      try {
        LynxUIRenderer renderer = spy(new LynxUIRenderer());
        LynxPageView page = createTouchPage(host, renderer);
        List<Boolean> disallowIntercept = new ArrayList<>();
        FrameLayout parent = new FrameLayout(mContext) {
          @Override
          public void requestDisallowInterceptTouchEvent(boolean disallow) {
            disallowIntercept.add(disallow);
            super.requestDisallowInterceptTouchEvent(disallow);
          }
        };
        parent.addView(page);
        List<Integer> childEvents = new ArrayList<>();
        View child = new View(mContext) {
          @Override
          public boolean onTouchEvent(MotionEvent event) {
            childEvents.add(event.getActionMasked());
            return true;
          }
        };
        page.addView(child);
        child.layout(0, 0, 100, 100);
        doReturn(false).when(renderer).onTouchEvent(any(MotionEvent.class), any());
        assertFalse(dispatchTouch(page, MotionEvent.ACTION_DOWN));
        assertTrue(childEvents.isEmpty());

        doReturn(true).when(renderer).onTouchEvent(any(MotionEvent.class), any());
        doReturn(true).when(renderer).blockNativeEvent(any(MotionEvent.class));
        assertTrue(dispatchTouch(page, MotionEvent.ACTION_DOWN));
        assertTrue(disallowIntercept.contains(true));
        assertEquals(Arrays.asList(MotionEvent.ACTION_DOWN), childEvents);
        doReturn(true).when(renderer).consumeSlideEvent(any(MotionEvent.class));
        assertTrue(dispatchTouch(page, MotionEvent.ACTION_MOVE));
        assertEquals(Arrays.asList(MotionEvent.ACTION_DOWN), childEvents);
        assertTrue(dispatchTouch(page, MotionEvent.ACTION_UP));

        // A new gesture resets the latched slide-consumption decision.
        doReturn(false).when(renderer).consumeSlideEvent(any(MotionEvent.class));
        assertTrue(dispatchTouch(page, MotionEvent.ACTION_DOWN));
        assertEquals(MotionEvent.ACTION_DOWN, (int) childEvents.get(childEvents.size() - 1));
        doReturn(false).when(renderer).blockNativeEvent(any(MotionEvent.class));
        doReturn(true).when(renderer).onInterceptTouchEvent(any(MotionEvent.class));
        disallowIntercept.clear();
        assertTrue(dispatchTouch(page, MotionEvent.ACTION_MOVE));
        assertTrue(disallowIntercept.contains(true));
        assertEquals(MotionEvent.ACTION_CANCEL, (int) childEvents.get(childEvents.size() - 1));
        assertTrue(dispatchTouch(page, MotionEvent.ACTION_CANCEL));
        doReturn(false).when(renderer).onInterceptTouchEvent(any(MotionEvent.class));

        clearInvocations(renderer);
        doReturn(true).when(renderer).shouldInvokeNativeViewMethod();
        assertTrue(dispatchTouch(page, MotionEvent.ACTION_DOWN));
        assertTrue(dispatchTouch(page, MotionEvent.ACTION_CANCEL));
        verify(renderer, never()).onTouchEvent(any(MotionEvent.class), any());
        doReturn(false).when(renderer).shouldInvokeNativeViewMethod();

        LynxViewClient client = mock(LynxViewClient.class);
        page.getTemplateRender().addLynxViewClient(client);
        doThrow(new IllegalStateException("touch test"))
            .when(renderer)
            .onTouchEvent(any(MotionEvent.class), any());
        assertFalse(dispatchTouch(page, MotionEvent.ACTION_DOWN));
        verify(client).onReceivedError(
            argThat((LynxError error) -> error.getSubCode() == LynxSubErrorCode.E_EVENT_EXCEPTION));
        page.destroy();
        clearInvocations(renderer);
        assertFalse(dispatchTouch(page, MotionEvent.ACTION_DOWN));
        verify(renderer, never()).onTouchEvent(any(MotionEvent.class), any());
      } finally {
        host.release();
      }
    });
  }

  @Test
  public void invalidBundleReportsOnlyToItsPageAndDestroyStopsLoading() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxHost host = new LynxViewGroupBuilder().buildHost();
      try {
        LynxPageView first =
            host.createPageView(mContext, uiOnlyOption(ThreadStrategyForRendering.ALL_ON_UI));
        LynxPageView second =
            host.createPageView(mContext, uiOnlyOption(ThreadStrategyForRendering.ALL_ON_UI));
        LynxViewClient firstClient = org.mockito.Mockito.mock(LynxViewClient.class);
        LynxViewClient secondClient = org.mockito.Mockito.mock(LynxViewClient.class);
        first.getTemplateRender().addLynxViewClient(firstClient);
        second.getTemplateRender().addLynxViewClient(secondClient);
        first.loadTemplate(null, null, "invalid.js");
        org.mockito.Mockito.verify(firstClient)
            .onReceivedError(org.mockito.ArgumentMatchers.argThat(
                (LynxError error)
                    -> error.getSubCode() == LynxSubErrorCode.E_APP_BUNDLE_LOAD_BAD_BUNDLE));
        org.mockito.Mockito.verify(secondClient, org.mockito.Mockito.never())
            .onReceivedError(org.mockito.ArgumentMatchers.any(LynxError.class));
        first.destroy();
        org.mockito.Mockito.clearInvocations(firstClient);
        first.loadTemplate(null, null, "destroyed.js");
        org.mockito.Mockito.verify(firstClient, org.mockito.Mockito.never())
            .onReceivedError(org.mockito.ArgumentMatchers.any(LynxError.class));
      } finally {
        host.release();
      }
    });
  }

  @Test
  public void missingPreloadScriptReportsThroughNativePage() throws Exception {
    CountDownLatch errorReceived = new CountDownLatch(1);
    java.util.concurrent.atomic.AtomicInteger errors =
        new java.util.concurrent.atomic.AtomicInteger();
    LynxGroup runtimeGroup =
        new LynxGroup.LynxGroupBuilder()
            .setID("missing-preload-test")
            .setPreloadJSPaths(new String[] {"assets://missing_preload_test.js"})
            .build();
    LynxHost[] host = new LynxHost[1];
    TemplateBundle bundle =
        TemplateBundle.fromTemplate(ResourceHelper.loadByteArray(mContext, "hello-world.js"));
    try {
      InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
        host[0] = new LynxViewGroupBuilder().setLynxGroup(runtimeGroup).buildHost();
        LynxPageView page = host[0].createPageView(
            mContext, new LynxPageOption.Builder().setEnablePendingJsTask(true).build());
        page.getTemplateRender().addLynxViewClient(new LynxViewClient() {
          @Override
          public void onReceivedError(LynxError error) {
            if (error.getSubCode()
                == LynxSubErrorCode.E_RESOURCE_EXTERNAL_RESOURCE_LOCAL_RESOURCE_LOAD_FAIL) {
              errors.incrementAndGet();
              errorReceived.countDown();
            }
          }
        });
        page.loadTemplate(bundle, null, "preload-error-test");
      });
      assertTrue(
          "Missing preload script must report an error", errorReceived.await(10, TimeUnit.SECONDS));
      InstrumentationRegistry.getInstrumentation().waitForIdleSync();
      assertEquals(1, errors.get());
    } finally {
      InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
        if (host[0] != null) {
          host[0].release();
        }
        runtimeGroup.destroy();
      });
      bundle.release();
    }
  }

  @Test
  public void hostOptionsSnapshotKeepsBuilderRegistrationsIndependent() {
    LynxBackgroundRuntimeOptions options = new LynxBackgroundRuntimeOptions();
    options.registerModule("shared", com.lynx.jsbridge.LynxModule.class, null);
    com.lynx.tasm.provider.LynxResourceProvider provider =
        org.mockito.Mockito.mock(com.lynx.tasm.provider.LynxResourceProvider.class);
    options.setResourceProviders("shared", provider);
    LynxHostEnvironment environment =
        createHostEnvironment(options, new LynxViewBuilder().getBehaviorRegistry());
    LynxBackgroundRuntimeOptions shared = environment.getRuntimeOptions();
    assertNotSame(options, shared);
    assertNotSame(options.getWrappers(), shared.getWrappers());
    options.registerModule("builder-only", com.lynx.jsbridge.LynxModule.class, null);
    options.getAllResourceProviders().clear();
    assertSame(shared, environment.getRuntimeOptions());
    assertEquals(1, shared.getWrappers().size());
    assertEquals(2, options.getWrappers().size());
    assertSame(provider, shared.getResourceProvidersByKey("shared"));
  }

  @Test
  public void hostRegistrationsAreCapturedBeforePageCreation() {
    LynxViewGroupBuilder builder = new LynxViewGroupBuilder();
    builder.registerModule("custom", com.lynx.jsbridge.LynxModule.class, null);
    com.lynx.tasm.behavior.Behavior original = new com.lynx.tasm.behavior.Behavior("host-test") {};
    builder.addBehavior(original);
    LynxHost host = builder.buildHost();
    try {
      assertNotNull(host.getHostEnvironment().getResourceLoader());
      builder.registerModule("late", com.lynx.jsbridge.LynxModule.class, null);
      builder.addBehavior(new com.lynx.tasm.behavior.Behavior("host-test") {});
      assertSame(original, host.getHostEnvironment().getBehaviorRegistry().get("host-test"));
      assertEquals(1, host.getHostEnvironment().getRuntimeOptions().getWrappers().size());
    } finally {
      host.release();
    }
  }

  @Test
  public void pageOptionsDoNotFollowLaterBuilderChanges() {
    LynxPageOption.Builder builder = new LynxPageOption.Builder()
                                         .setColorScheme(LynxColorScheme.DARK)
                                         .setDensity(2.0f)
                                         .setScreenSize(640, 480)
                                         .setEnableJSRuntime(false)
                                         .setEnablePendingJsTask(true);
    LynxPageOption option = builder.build();
    builder.setColorScheme(LynxColorScheme.LIGHT)
        .setEnableJSRuntime(true)
        .setEnablePendingJsTask(false);
    assertEquals(LynxColorScheme.DARK, option.getColorScheme());
    assertEquals(2.0f, option.getDensity(), 0.0f);
    assertEquals(640, option.getScreenWidth());
    assertFalse(option.isEnableJSRuntime());
    assertTrue(option.isEnablePendingJsTask());
  }

  @Test
  public void pagesChooseTheirRuntimePolicyWithinOneHost() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxHost host = new LynxViewGroupBuilder()
                          .setEmbeddedMode(EmbeddedMode.EMBEDDED_MODE_BASE)
                          .setEnableJSRuntime(false)
                          .setEnablePendingJsTask(true)
                          .buildHost();
      try {
        LynxPageView uiOnly = host.createPageView(mContext,
            new LynxPageOption.Builder()
                .setEnableJSRuntime(false)
                .setEnablePendingJsTask(true)
                .build());
        Context pageContext = new android.view.ContextThemeWrapper(mContext, 0);
        LynxPageView scripted = host.createPageView(pageContext,
            new LynxPageOption.Builder()
                .setEnableJSRuntime(true)
                .setEnablePendingJsTask(false)
                .build());
        assertSame(mContext, uiOnly.getContext());
        assertSame(pageContext, scripted.getContext());
        assertSame(uiOnly.getHostEnvironment(), scripted.getHostEnvironment());
        assertNull(uiOnly.getLynxContext().getRuntimeCacheManager());
        assertNull(scripted.getLynxContext().getRuntimeCacheManager());
        assertNull(uiOnly.getTemplateRender().mModuleFactory);
        assertNotNull(scripted.getTemplateRender().mModuleFactory);
        assertTrue(uiOnly.getLynxContext().isEmbeddedModeOn());
        assertTrue(scripted.getLynxContext().isEmbeddedModeOn());
      } finally {
        host.release();
      }
    });
  }

  @Test
  public void surfaceDestructionDoesNotReplaceHostResourceLoader() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxHost first = new LynxViewGroupBuilder().buildHost();
      LynxHost second = new LynxViewGroupBuilder().buildHost();
      com.lynx.tasm.core.resource.LynxResourceLoader loader =
          first.getHostEnvironment().getResourceLoader();
      try {
        LynxPageView page =
            first.createPageView(mContext, pageOption(ThreadStrategyForRendering.ALL_ON_UI));
        LynxPageView sibling =
            first.createPageView(mContext, pageOption(ThreadStrategyForRendering.ALL_ON_UI));
        LynxPageView independent =
            second.createPageView(mContext, pageOption(ThreadStrategyForRendering.ALL_ON_UI));
        assertSame(loader, environment(page.getTemplateRender()).getResourceLoader());
        assertSame(loader, environment(sibling.getTemplateRender()).getResourceLoader());
        assertNotSame(loader, environment(independent.getTemplateRender()).getResourceLoader());
        page.destroy();
        assertSame(loader, first.getHostEnvironment().getResourceLoader());
        assertNotEquals(0, nativePtr(sibling.getTemplateRender()));
        LynxPageView next =
            first.createPageView(mContext, pageOption(ThreadStrategyForRendering.ALL_ON_UI));
        assertSame(loader, environment(next.getTemplateRender()).getResourceLoader());
        first.release();
        first.release();
        assertNull(sibling.getTemplateRender().getLynxContext());
        assertNull(next.getTemplateRender().getLynxContext());
        assertNotNull(independent.getTemplateRender().getLynxContext());
      } finally {
        first.release();
        second.release();
      }
    });
  }

  @Test
  public void hostEnvironmentSharesConfiguredGroupWithoutChangingItsPolicy() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxGroup.LynxGroupBuilder builder =
          new LynxGroup.LynxGroupBuilder().setID("shell-config-test").setEnableJSGroupThread(true);
      builder.mEnableWhiteBoard = true;
      LynxGroup source = builder.build();
      LynxBackgroundRuntimeOptions options = new LynxBackgroundRuntimeOptions();
      options.setLynxGroup(source);
      LynxHostEnvironment environment =
          createHostEnvironment(options, new LynxViewBuilder().getBehaviorRegistry());
      try {
        LynxBackgroundRuntimeOptions first = environment.getRuntimeOptions();
        LynxBackgroundRuntimeOptions second = environment.getRuntimeOptions();
        assertSame(source, first.getLynxGroup());
        assertSame(source, second.getLynxGroup());
        assertFalse(first.getLynxGroup().enableNewShareGroup());
        assertTrue(first.getLynxGroup().enableJSGroupThread());
        assertSame(first, second);
        assertSame(source, environment.getRuntimeOptions().getLynxGroup());
        assertNotEquals(0, source.getWhiteBoardPtr());
      } finally {
        source.destroy();
      }
    });
  }

  @Test
  public void hostUsesConfiguredGroupOrCreatesItsOwnDefault() throws Exception {
    java.lang.reflect.Method rendererGroupId =
        LynxTemplateRender.class.getDeclaredMethod("getGroupID");
    rendererGroupId.setAccessible(true);
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      for (LynxGroup configuredGroup : new LynxGroup[] {null,
               new LynxGroup.LynxGroupBuilder().setID(LynxGroup.SINGNLE_GROUP).build(),
               new LynxGroup.LynxGroupBuilder().setID("surface-configured-group").build(),
               new LynxGroup.LynxGroupBuilder().setEnableNewShareGroup(true).build()}) {
        String originalId = configuredGroup == null ? null : configuredGroup.getID();
        LynxHost group = new LynxViewGroupBuilder().setLynxGroup(configuredGroup).buildHost();
        LynxHost other = new LynxViewGroupBuilder().buildHost();
        ILynxViewGroup legacyGroup = new LynxViewGroupBuilder()
                                         .setContext(mContext)
                                         .setLynxGroup(configuredGroup)
                                         .setEnableJSRuntime(false)
                                         .build();
        LynxView legacy = null;
        try {
          LynxPageView first =
              group.createPageView(mContext, uiOnlyOption(ThreadStrategyForRendering.ALL_ON_UI));
          LynxPageView second =
              group.createPageView(mContext, uiOnlyOption(ThreadStrategyForRendering.ALL_ON_UI));
          LynxPageView independent =
              other.createPageView(mContext, uiOnlyOption(ThreadStrategyForRendering.ALL_ON_UI));
          LynxGroup effectiveGroup = group.getHostEnvironment().getRuntimeOptions().getLynxGroup();
          assertNotNull(effectiveGroup);
          assertEquals(effectiveGroup.getID(), rendererGroupId.invoke(first.getTemplateRender()));
          assertEquals(effectiveGroup.getID(), rendererGroupId.invoke(second.getTemplateRender()));
          assertNotEquals(
              effectiveGroup.getID(), rendererGroupId.invoke(independent.getTemplateRender()));
          if (configuredGroup == null) {
            assertNotEquals(LynxGroup.SINGNLE_GROUP, effectiveGroup.getID());
            assertFalse(effectiveGroup.enableNewShareGroup());
          } else {
            assertSame(configuredGroup, effectiveGroup);
            assertEquals(originalId, effectiveGroup.getID());
          }
          legacy = new LynxViewBuilder().setLynxViewGroup(legacyGroup).build(mContext);
          assertEquals(originalId == null ? LynxGroup.SINGNLE_GROUP : originalId,
              rendererGroupId.invoke(legacy.mLynxTemplateRender));
        } catch (ReflectiveOperationException error) {
          throw new AssertionError(error);
        } finally {
          if (legacy != null) {
            legacy.destroy();
          }
          legacyGroup.release();
          group.release();
          other.release();
          if (configuredGroup != null) {
            configuredGroup.destroy();
          }
        }
      }
    });
  }

  @Test
  public void defaultSurfacesShareLiveJSContext() throws Exception {
    assertPagesFollowJSContextPolicy(null, true);
  }

  @Test
  public void configuredSharedGroupSharesLiveJSContext() throws Exception {
    assertPagesFollowJSContextPolicy(new LynxGroup.LynxGroupBuilder().build(), true);
  }

  @Test
  public void singleGroupPagesKeepIndependentJSContexts() throws Exception {
    assertPagesFollowJSContextPolicy(
        new LynxGroup.LynxGroupBuilder().setID(LynxGroup.SINGNLE_GROUP).build(), false);
  }

  @Test
  public void isolatedContextOptionKeepsIndependentJSContexts() throws Exception {
    assertPagesFollowJSContextPolicy(
        new LynxGroup.LynxGroupBuilder().setEnableNewShareGroup(true).build(), false);
  }

  private void assertPagesFollowJSContextPolicy(LynxGroup configuredGroup, boolean sharedContext)
      throws Exception {
    LynxHost[] groups = new LynxHost[2];
    LynxPageView[] surfaces = new LynxPageView[3];
    TemplateBundle bundle =
        TemplateBundle.fromTemplate(ResourceHelper.loadByteArray(mContext, "hello-world.js"));
    CountDownLatch loaded = new CountDownLatch(surfaces.length);
    boolean registerImageService = !LynxServiceCenter.inst().hasService(ILynxImageService.class);
    if (registerImageService) {
      ILynxImageService imageService = org.mockito.Mockito.mock(ILynxImageService.class);
      org.mockito.Mockito.doReturn(ILynxImageService.class).when(imageService).getServiceClass();
      LynxServiceCenter.inst().registerService(imageService);
    }
    try {
      InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
        groups[0] = new LynxViewGroupBuilder().setLynxGroup(configuredGroup).buildHost();
        groups[1] = new LynxViewGroupBuilder().buildHost();
        surfaces[0] =
            groups[0].createPageView(mContext, pageOption(ThreadStrategyForRendering.ALL_ON_UI));
        surfaces[1] = groups[0].createPageView(
            mContext, pageOption(ThreadStrategyForRendering.MULTI_THREADS));
        surfaces[2] =
            groups[1].createPageView(mContext, pageOption(ThreadStrategyForRendering.ALL_ON_UI));
        assertNotEquals(jsProxy(surfaces[0]).getRuntimeId(), jsProxy(surfaces[1]).getRuntimeId());
        for (LynxPageView surface : surfaces) {
          surface.getTemplateRender().addLynxViewClient(new LynxViewClient() {
            @Override
            public void onLoadSuccess() {
              loaded.countDown();
            }
          });
          surface.loadTemplate(bundle, TemplateData.fromString("{}"), "hello-world.js");
        }
      });
      assertTrue("All surfaces must load", loaded.await(15, TimeUnit.SECONDS));
      // Write after all pages exist: sharing a VM or copying initial globals cannot pass this.
      assertContextProbe(surfaces[0],
          "globalThis.__lynxSurfaceContextProbe = 'shared';"
              + "throw new Error('surface-context-probe:written');",
          "surface-context-probe:written");
      assertContextProbe(surfaces[1],
          "throw new Error('surface-context-probe:' + globalThis.__lynxSurfaceContextProbe);",
          sharedContext ? "surface-context-probe:shared" : "surface-context-probe:undefined");
      assertContextProbe(surfaces[2],
          "throw new Error('surface-context-probe:' + typeof globalThis.__lynxSurfaceContextProbe);",
          "surface-context-probe:undefined");
      InstrumentationRegistry.getInstrumentation().runOnMainSync(surfaces[0] ::destroy);
      assertContextProbe(surfaces[1],
          "throw new Error('surface-context-probe:' + globalThis.__lynxSurfaceContextProbe);",
          sharedContext ? "surface-context-probe:shared" : "surface-context-probe:undefined");
    } finally {
      InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
        for (LynxHost group : groups) {
          if (group != null) {
            group.release();
          }
        }
        if (configuredGroup != null) {
          configuredGroup.destroy();
        }
      });
      bundle.release();
      if (registerImageService) {
        LynxServiceCenter.inst().unregisterService(ILynxImageService.class);
      }
    }
  }

  private void assertContextProbe(LynxPageView surface, String script, String expected)
      throws Exception {
    CountDownLatch reported = new CountDownLatch(1);
    java.util.concurrent.atomic.AtomicReference<String> result =
        new java.util.concurrent.atomic.AtomicReference<>();
    // EvaluateScript returns to a JS callback. Use the page's error callback to observe
    // a deliberately thrown probe result without adding a production native test API.
    LynxViewClient client = new LynxViewClient() {
      @Override
      public void onReceivedError(LynxError error) {
        result.set(error.getMsg());
        reported.countDown();
      }
    };
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      surface.getTemplateRender().addLynxViewClient(client);
      jsProxy(surface).evaluateScript(
          "surface-context-probe.js", script.getBytes(java.nio.charset.StandardCharsets.UTF_8), -1);
    });
    try {
      assertTrue("JS context probe did not report a result", reported.await(10, TimeUnit.SECONDS));
      assertTrue("Unexpected probe result: " + result.get(), result.get().contains(expected));
    } finally {
      InstrumentationRegistry.getInstrumentation().runOnMainSync(
          () -> surface.getTemplateRender().removeLynxViewClient(client));
    }
  }

  @Test
  public void surfaceRemainsIndependentOfLegacyViewAndGroup() throws Exception {
    assertFalse(LynxView.class.isAssignableFrom(LynxPageView.class));
    assertEquals(
        LynxTemplateRender.class, LynxPageView.class.getDeclaredField("mTemplateRender").getType());
    assertNotNull(LynxHost.class.getMethod("createPageView", Context.class, LynxPageOption.class));
    for (java.lang.reflect.Method method : ILynxViewGroup.class.getMethods()) {
      assertNotEquals("createPageView", method.getName());
      assertNotEquals("getHostEnvironment", method.getName());
    }
  }

  @Before
  public void setUp() {
    mContext =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    LynxEnv.inst().init((Application) mContext, null, null, null, null);
  }

  @Test
  public void legacyInitialViewportPreservesConfigParsingAndFallback() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      boolean defaultValue = LynxEnv.inst().enableSkipUpdateViewportOnInitWhenMeasureSpecEmpty();
      String[] values = {null, "  TrUe  ", "1", "false", "0", "invalid"};
      boolean[] expected = {defaultValue, true, true, false, false, defaultValue};
      for (int i = 0; i < values.length; ++i) {
        java.util.Map<String, String> config = values[i] == null
            ? null
            : java.util.Collections.singletonMap(
                LynxEnvKey.ENABLE_SKIP_UPDATE_VIEWPORT_ON_INIT_WHEN_MEASURE_SPEC_EMPTY
                    .getDescription(),
                values[i]);
        LynxView view =
            new LynxViewBuilder().setEnableJSRuntime(false).setLynxViewConfig(config).build(
                mContext);
        try {
          assertEquals(expected[i],
              field(
                  view.mLynxTemplateRender, "mEnableSkipUpdateViewportOnInitWhenMeasureSpecEmpty"));
        } finally {
          view.destroy();
        }
      }
    });
  }

  @Test
  public void loadGlobalPropsArePageOwnedAndIgnoreBuilderDefaults() {
    TemplateBundle bundle =
        TemplateBundle.fromTemplate(ResourceHelper.loadByteArray(mContext, "hello-world.js"));
    try {
      InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
        TemplateData defaults = TemplateData.fromString("{\"scope\":\"group\",\"shared\":true}");
        TemplateData overrides = TemplateData.fromString("{\"scope\":\"page\"}");
        LynxHost group = new LynxViewGroupBuilder().setGlobalProps(defaults).buildHost();
        try {
          LynxPageView first =
              group.createPageView(mContext, uiOnlyOption(ThreadStrategyForRendering.ALL_ON_UI));
          LynxPageView second =
              group.createPageView(mContext, uiOnlyOption(ThreadStrategyForRendering.ALL_ON_UI));
          first.loadTemplate(bundle, null, "first.js", overrides);
          second.loadTemplate(bundle, null, "second.js");
          assertEquals("page",
              ((TemplateData) field(first.getTemplateRender(), "globalProps"))
                  .toMap()
                  .get("scope"));
          assertFalse(((TemplateData) field(first.getTemplateRender(), "globalProps"))
                          .toMap()
                          .containsKey("shared"));
          assertNull(((TemplateData) field(second.getTemplateRender(), "globalProps")));
          assertNull(group.getHostEnvironment().getRuntimeOptions().getGlobalProps());
          assertEquals("group", defaults.toMap().get("scope"));
          assertFalse(overrides.toMap().containsKey("shared"));
          first.loadTemplate(bundle, null, "reload.js");
          assertEquals("page",
              ((TemplateData) field(first.getTemplateRender(), "globalProps"))
                  .toMap()
                  .get("scope"));
        } finally {
          group.release();
          defaults.recycle();
          overrides.recycle();
        }
      });
    } finally {
      bundle.release();
    }
  }

  @Test
  public void resourceLoadersAreScopedToTheirOwnGroup() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxHost first = new LynxViewGroupBuilder().buildHost();
      LynxHost second = new LynxViewGroupBuilder().buildHost();
      try {
        LynxPageView firstSurface =
            first.createPageView(mContext, pageOption(ThreadStrategyForRendering.ALL_ON_UI));
        LynxPageView secondSurface =
            second.createPageView(mContext, pageOption(ThreadStrategyForRendering.ALL_ON_UI));
        assertNotSame(environment(firstSurface.getTemplateRender()).getResourceLoader(),
            environment(secondSurface.getTemplateRender()).getResourceLoader());
        first.release();
        assertNotNull(secondSurface.getTemplateRender().getLynxContext());
      } finally {
        first.release();
        second.release();
      }
    });
  }

  @Test
  public void sharedEnvironmentHasIndependentShellsAndThreadStrategies() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxGroup runtimeGroup =
          new LynxGroup.LynxGroupBuilder().setEnableNewShareGroup(true).build();
      com.lynx.tasm.provider.LynxResourceProvider provider =
          org.mockito.Mockito.mock(com.lynx.tasm.provider.LynxResourceProvider.class);
      LynxHost group = new LynxViewGroupBuilder()
                           .setLynxGroup(runtimeGroup)
                           .setResourceProvider("host-provider", provider)
                           .buildHost();
      LynxHostEnvironment shell = group.getHostEnvironment();
      for (java.lang.reflect.Method method : LynxHostEnvironment.class.getDeclaredMethods()) {
        assertFalse(java.lang.reflect.Modifier.isNative(method.getModifiers()));
      }
      LynxPageView first = null;
      LynxPageView second = null;
      try {
        first = group.createPageView(mContext, pageOption(ThreadStrategyForRendering.ALL_ON_UI));
        second =
            group.createPageView(mContext, pageOption(ThreadStrategyForRendering.MULTI_THREADS));
        LynxTemplateRender firstPage = first.getTemplateRender();
        LynxTemplateRender secondPage = second.getTemplateRender();
        assertSame(shell.getRuntimeOptions(), firstPage.mLynxRuntimeOptions);
        assertSame(shell.getRuntimeOptions(), secondPage.mLynxRuntimeOptions);
        assertSame(runtimeGroup, firstPage.mLynxRuntimeOptions.getLynxGroup());
        assertSame(runtimeGroup, secondPage.mLynxRuntimeOptions.getLynxGroup());
        assertTrue(runtimeGroup.enableNewShareGroup());
        assertNotSame(firstPage, secondPage);
        assertNotSame(firstPage.mModuleFactory, secondPage.mModuleFactory);
        assertSame(shell, environment(firstPage));
        assertSame(shell, environment(secondPage));
        assertSame(shell, first.getHostEnvironment());
        assertSame(shell, second.getHostEnvironment());
        assertEquals(ThreadStrategyForRendering.ALL_ON_UI, first.getThreadStrategyForRendering());
        assertEquals(
            ThreadStrategyForRendering.MULTI_THREADS, second.getThreadStrategyForRendering());
        assertNotEquals(0, nativePtr(firstPage));
        assertNotEquals(0, nativePtr(secondPage));
        assertNotEquals(nativePtr(firstPage), nativePtr(secondPage));
        assertNotSame(firstPage.getLynxContext(), secondPage.getLynxContext());
        assertNotSame(field(firstPage, "mLynxUIRender"), field(secondPage, "mLynxUIRender"));
        assertSame(environment(firstPage).getResourceLoader(),
            environment(secondPage).getResourceLoader());
        assertSame(environment(firstPage).getResourceLoader(),
            environment(firstPage).getRuntimeResourceLoader());
        assertSame(environment(secondPage).getResourceLoader(),
            environment(secondPage).getRuntimeResourceLoader());
        assertSame(environment(firstPage).getRuntimeResourceLoader(),
            environment(secondPage).getRuntimeResourceLoader());
        assertSame(shell.getProviderRegistry(), firstPage.getLynxContext().getProviderRegistry());
        assertSame(shell.getProviderRegistry(), secondPage.getLynxContext().getProviderRegistry());
        first.destroy();
        assertNull(firstPage.getLynxContext());
        assertNotNull(secondPage.getLynxContext());
        assertSame(provider,
            secondPage.getLynxContext().getProviderRegistry().getProviderByKey("host-provider"));
        assertSame(provider, shell.getProviderRegistry().getProviderByKey("host-provider"));
        group.release();
        assertNull(secondPage.getLynxContext());
        assertEquals(0, nativePtr(secondPage));
        assertNull(second.getLynxContext());
        try {
          group.createPageView(mContext, pageOption(ThreadStrategyForRendering.ALL_ON_UI));
          fail("A released group must reject new surfaces");
        } catch (IllegalStateException expected) {
        }
      } finally {
        if (first != null) {
          first.destroy();
        }
        if (second != null) {
          second.destroy();
        }
        group.release();
        runtimeGroup.destroy();
      }
    });
  }

  @Test
  public void surfacesSupportExplicitMultiAsyncWithoutChangingHostDefaults() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxHost group = new LynxViewGroupBuilder().buildHost();
      try {
        for (ThreadStrategyForRendering strategy : ThreadStrategyForRendering.values()) {
          LynxPageView shared = group.createPageView(mContext, pageOption(strategy, false));
          LynxPageView independent = null;
          try {
            independent = group.createPageView(mContext, pageOption(strategy, true));
            assertSame(shared.getHostEnvironment(), independent.getHostEnvironment());
            assertNotEquals(
                nativePtr(shared.getTemplateRender()), nativePtr(independent.getTemplateRender()));
            assertEquals(strategy, shared.getThreadStrategyForRendering());
            assertEquals(strategy, independent.getThreadStrategyForRendering());
            shared.destroy();
            assertNotNull(independent.getTemplateRender().getLynxContext());
          } finally {
            shared.destroy();
            if (independent != null) {
              independent.destroy();
            }
          }
        }
      } finally {
        group.release();
      }
    });
  }

  @Test
  public void backgroundHostReleaseClosesPagesAndRejectsNewSurfaces() {
    java.util.concurrent.atomic.AtomicReference<LynxHost> groupRef =
        new java.util.concurrent.atomic.AtomicReference<>();
    java.util.concurrent.atomic.AtomicReference<LynxPageView> surfaceRef =
        new java.util.concurrent.atomic.AtomicReference<>();
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxHost group = new LynxViewGroupBuilder().buildHost();
      groupRef.set(group);
      surfaceRef.set(
          group.createPageView(mContext, pageOption(ThreadStrategyForRendering.ALL_ON_UI)));
    });
    try {
      groupRef.get().release();
      InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
        assertNull(surfaceRef.get().getTemplateRender().getLynxContext());
        try {
          groupRef.get().createPageView(mContext, pageOption(ThreadStrategyForRendering.ALL_ON_UI));
          fail("A released group must reject new surfaces");
        } catch (IllegalStateException expected) {
        }
      });
    } finally {
      InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> groupRef.get().release());
    }
  }

  @Test
  public void ordinaryViewStillOwnsItsOriginalRenderer() throws Exception {
    java.lang.reflect.Field hostField =
        LynxTemplateRender.class.getDeclaredField("mHostEnvironment");
    hostField.setAccessible(true);
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxView view = new LynxView(mContext, new LynxViewBuilder());
      LynxView sibling = new LynxView(mContext, new LynxViewBuilder());
      try {
        assertNotNull(view.mLynxTemplateRender.getLynxContext());
        assertNotNull(hostField.get(view.mLynxTemplateRender));
        assertSame(
            ((LynxHostEnvironment) hostField.get(view.mLynxTemplateRender)).getRuntimeOptions(),
            view.mLynxTemplateRender.mLynxRuntimeOptions);
        assertNotSame(
            hostField.get(view.mLynxTemplateRender), hostField.get(sibling.mLynxTemplateRender));
      } catch (IllegalAccessException error) {
        throw new AssertionError(error);
      } finally {
        view.destroy();
        sibling.destroy();
      }
    });
  }

  @Test
  public void legacyGroupReleaseDoesNotReleaseSurfaceHost() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxHost group = new LynxViewGroupBuilder().buildHost();
      ILynxViewGroup legacyGroup = new LynxViewGroupBuilder().setContext(mContext).build();
      LynxView view = null;
      try {
        view = new LynxViewBuilder().setLynxViewGroup(legacyGroup).build(mContext);
        LynxPageView surface =
            group.createPageView(mContext, pageOption(ThreadStrategyForRendering.ALL_ON_UI));
        assertNotSame(view.mLynxTemplateRender.getLynxContext(), surface.getLynxContext());
        view.destroy();
        legacyGroup.release();
        assertNotNull(surface.getTemplateRender().getLynxContext());
      } finally {
        if (view != null) {
          view.destroy();
        }
        legacyGroup.release();
        group.release();
      }
    });
  }

  @Test
  public void hostPagesLoadUpdateAndReloadThroughTemplateRender() throws Exception {
    byte[] bytes;
    try (InputStream input = mContext.getAssets().open("hello-world.js");
         ByteArrayOutputStream output = new ByteArrayOutputStream()) {
      byte[] buffer = new byte[4096];
      int size;
      while ((size = input.read(buffer)) != -1) {
        output.write(buffer, 0, size);
      }
      bytes = output.toByteArray();
    }
    TemplateBundle bundle = TemplateBundle.fromTemplate(bytes);
    LynxHost[] group = new LynxHost[1];
    LynxPageView[] surfaces = new LynxPageView[2];
    LynxView[] legacy = new LynxView[1];
    LynxViewBuilder[] legacyBuilder = new LynxViewBuilder[1];
    Object initialExtraData = new Object();
    LynxFontFaceLoader.Loader fontLoader =
        org.mockito.Mockito.mock(LynxFontFaceLoader.Loader.class);
    LynxImageFetcher imageFetcher = org.mockito.Mockito.mock(LynxImageFetcher.class);
    CountDownLatch legacyLoaded = new CountDownLatch(1);
    CountDownLatch legacyRendered = new CountDownLatch(1);
    CountDownLatch loaded = new CountDownLatch(2);
    CountDownLatch rendered = new CountDownLatch(2);
    CountDownLatch updated = new CountDownLatch(1);
    java.util.concurrent.atomic.AtomicReference<LynxError> error =
        new java.util.concurrent.atomic.AtomicReference<>();
    boolean registerImageService = !LynxServiceCenter.inst().hasService(ILynxImageService.class);
    if (registerImageService) {
      ILynxImageService imageService = org.mockito.Mockito.mock(ILynxImageService.class);
      org.mockito.Mockito.doReturn(ILynxImageService.class).when(imageService).getServiceClass();
      LynxServiceCenter.inst().registerService(imageService);
    }
    try {
      InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
        group[0] = new LynxViewGroupBuilder().buildHost();
        for (int i = 0; i < surfaces.length; ++i) {
          surfaces[i] = group[0].createPageView(mContext,
              pageOption(i == 0 ? ThreadStrategyForRendering.ALL_ON_UI
                                : ThreadStrategyForRendering.MULTI_THREADS));
          assertEquals(
              true, field(surfaces[i].getTemplateRender(), "mEnableNativeLifecycleOptimization"));
          assertEquals(true,
              field(surfaces[i].getTemplateRender(),
                  "mEnableSkipUpdateViewportOnInitWhenMeasureSpecEmpty"));
          surfaces[i].getTemplateRender().addLynxViewClient(new LynxViewClient() {
            private boolean firstScreen;
            @Override
            public void onFirstScreen() {
              if (!firstScreen) {
                firstScreen = true;
                rendered.countDown();
              }
            }
            @Override
            public void onLoadSuccess() {
              loaded.countDown();
            }
            @Override
            public void onDataUpdated() {
              updated.countDown();
            }
            @Override
            public void onUpdateDataWithoutChange() {
              updated.countDown();
            }
            @Override
            public void onReceivedError(LynxError value) {
              error.set(value);
            }
          });
          surfaces[i].loadTemplate(bundle,
              TemplateData.fromString("{\"message\":\"initial\",\"surface\":" + i + "}"),
              "hello-world.js");
        }
      });
      assertTrue("Both surfaces must load", loaded.await(15, TimeUnit.SECONDS));
      InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
        legacyBuilder[0] = new LynxViewBuilder()
                               .setLynxModuleExtraData(initialExtraData)
                               .setFontLoader(fontLoader)
                               .setImageFetcher(imageFetcher);
        legacy[0] = legacyBuilder[0].build(mContext);
        assertSame(initialExtraData, legacy[0].getLynxContext().getLynxExtraData());
        assertSame(fontLoader, legacy[0].getLynxContext().getFontLoader());
        assertSame(imageFetcher, legacy[0].getLynxContext().getImageFetcher());
        assertEquals(
            false, field(legacy[0].mLynxTemplateRender, "mEnableNativeLifecycleOptimization"));
        legacy[0].addLynxViewClient(new LynxViewClient() {
          @Override
          public void onLoadSuccess() {
            legacyLoaded.countDown();
          }
          @Override
          public void onFirstScreen() {
            legacyRendered.countDown();
          }
        });
        legacy[0].renderTemplateBundle(
            bundle, TemplateData.fromString("{\"marker\":42}"), "hello-world.js");
      });
      assertTrue("Legacy view must still load", legacyLoaded.await(15, TimeUnit.SECONDS));
      InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
        legacy[0].measure(android.view.View.MeasureSpec.makeMeasureSpec(
                              300, android.view.View.MeasureSpec.EXACTLY),
            android.view.View.MeasureSpec.makeMeasureSpec(
                200, android.view.View.MeasureSpec.EXACTLY));
        legacy[0].layout(0, 0, 300, 200);
      });
      assertTrue("Legacy view must render before reading data",
          legacyRendered.await(15, TimeUnit.SECONDS));
      InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
        int width = android.view.View.MeasureSpec.makeMeasureSpec(
            300, android.view.View.MeasureSpec.EXACTLY);
        int height = android.view.View.MeasureSpec.makeMeasureSpec(
            200, android.view.View.MeasureSpec.EXACTLY);
        for (LynxPageView surface : surfaces) {
          surface.measure(width, height);
          surface.layout(0, 0, 300, 200);
          assertEquals(300, surface.getMeasuredWidth());
        }
      });
      assertTrue("Both surfaces must render", rendered.await(15, TimeUnit.SECONDS));
      InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
        for (LynxPageView surface : surfaces) {
          // Flattened text is drawn without an Android child View; inspect the renderer tree.
          assertTrue("The loaded page must mount renderer children: "
                  + surface.getThreadStrategyForRendering(),
              !surface.getLynxContext().getUIBody().getChildren().isEmpty());
        }
        surfaces[1].updateData(TemplateData.fromString("{\"message\":\"updated\"}"));
        surfaces[0].destroy();
        surfaces[1].updateData(TemplateData.fromString("{\"afterSiblingDestroy\":true}"));
      });
      assertTrue("Remaining surface must update", updated.await(15, TimeUnit.SECONDS));
      CountDownLatch reloaded = new CountDownLatch(2);
      InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
        LynxTemplateRender surfacePage = surfaces[1].getTemplateRender();
        LynxTemplateRender legacyPage = legacy[0].mLynxTemplateRender;
        int oldSurfaceId = surfacePage.getLynxContext().getInstanceId();
        int oldLegacyId = legacyPage.getLynxContext().getInstanceId();
        Object oldSurfaceModules = surfacePage.mModuleFactory;
        Object oldLegacyModules = legacyPage.mModuleFactory;
        Object surfaceResources = environment(surfacePage).getResourceLoader();
        LynxViewClient reloadClient = new LynxViewClient() {
          @Override
          public void onLoadSuccess() {
            reloaded.countDown();
          }
        };
        surfacePage.addLynxViewClient(reloadClient);
        legacy[0].addLynxViewClient(reloadClient);
        surfaces[1].loadTemplate(bundle, TemplateData.fromString("{\"reload\":true}"), "reload.js");
        Object reloadedExtraData = new Object();
        legacyBuilder[0].setLynxModuleExtraData(reloadedExtraData);
        legacyBuilder[0].setFontLoader(null).setImageFetcher(null).setEnableJSRuntime(false);
        legacy[0].renderTemplateBundle(
            bundle, TemplateData.fromString("{\"reload\":true}"), "reload.js");
        // Reload keeps the configuration captured when the renderer was constructed.
        assertSame(initialExtraData, legacyPage.getLynxContext().getLynxExtraData());
        assertSame(fontLoader, legacyPage.getLynxContext().getFontLoader());
        assertSame(imageFetcher, legacyPage.getLynxContext().getImageFetcher());
        assertTrue(legacyPage.enableJSRuntime());
        LynxViewBuilder frameBuilder = legacyPage.getLynxViewBuilder();
        assertSame(fontLoader, frameBuilder.fontLoader);
        assertSame(imageFetcher, frameBuilder.imageFetcher);
        assertNotEquals(oldSurfaceId, surfacePage.getLynxContext().getInstanceId());
        assertNotEquals(oldLegacyId, legacyPage.getLynxContext().getInstanceId());
        assertNotSame(oldSurfaceModules, surfacePage.mModuleFactory);
        assertNotSame(oldLegacyModules, legacyPage.mModuleFactory);
        assertSame(surfaceResources, environment(surfacePage).getResourceLoader());
        assertEquals("reload.js", surfacePage.getLynxContext().getTemplateUrl());
        assertEquals("reload.js", legacyPage.getLynxContext().getTemplateUrl());
        TemplateData surfaceUpdate = TemplateData.fromString("{\"updatedAgain\":true}");
        TemplateData legacyUpdate = TemplateData.fromString("{\"updatedAgain\":true}");
        surfaces[1].updateData(surfaceUpdate);
        legacy[0].updateData(legacyUpdate);
      });
      assertTrue(
          "Both independent host pipelines must reload", reloaded.await(15, TimeUnit.SECONDS));
      assertNull(error.get());
    } finally {
      InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
        if (legacy[0] != null) {
          legacy[0].destroy();
        }
        for (LynxPageView surface : surfaces) {
          if (surface != null) {
            surface.destroy();
          }
        }
        if (group[0] != null) {
          group[0].release();
        }
      });
      bundle.release();
      if (registerImageService) {
        LynxServiceCenter.inst().unregisterService(ILynxImageService.class);
      }
    }
  }
}
