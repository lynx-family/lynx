// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.graphics.Rect;
import android.os.Build;
import android.util.SparseBooleanArray;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.LynxViewBuilder;
import com.lynx.tasm.behavior.shadow.ShadowNodeType;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.UIShadowProxy;
import com.lynx.tasm.behavior.ui.view.UIView;
import com.lynx.tasm.event.EventsListener;
import com.lynx.tasm.gesture.detector.GestureDetector;
import com.lynx.tasm.performance.memory.MemoryRecord;
import com.lynx.tasm.utils.LynxConstants;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.Collections;
import java.util.HashMap;
import java.util.IdentityHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.WeakHashMap;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class LynxUIOwnerTest {
  private static final String MEMORY_UI_TAG = "memory-ui";
  private static final String EVENT_LOOKUP_UI_TAG = "event-lookup-ui";

  static final class MemoryUI extends UIView {
    MemoryUI(LynxContext context) {
      super(context);
    }

    @Override
    public long getMemoryUsageBytes() {
      return 10;
    }
  }

  private static final class MemoryUIBehavior extends Behavior {
    MemoryUIBehavior() {
      super(MEMORY_UI_TAG);
    }

    @Override
    public LynxUI createUI(LynxContext context) {
      return new MemoryUI(context);
    }
  }

  static final class SharedMemoryUI extends UIView {
    private final long mOwnBytes;
    private final Object mSharedOwner;
    long mSharedBytes;

    SharedMemoryUI(
        LynxContext context, int sign, String tag, long ownBytes, long sharedBytes, Object owner) {
      super(context);
      setSign(sign, tag);
      mOwnBytes = ownBytes;
      mSharedBytes = sharedBytes;
      mSharedOwner = owner;
    }

    @Override
    public long getMemoryUsageBytes() {
      return mOwnBytes + mSharedBytes;
    }

    @Override
    public Map<String, String> getMemoryUsageDetail() {
      return Collections.singletonMap("detail-" + getSign(), "retained");
    }

    @Override
    public long getMemoryUsageBytesWithSharedOwners(Set<Object> sharedOwners) {
      return mOwnBytes + (sharedOwners.add(mSharedOwner) ? mSharedBytes : 0);
    }
  }

  static final class EventLookupUI extends UIView {
    private int mGetEventsCallCount;

    EventLookupUI(LynxContext context) {
      super(context);
    }

    @Override
    public Map<String, EventsListener> getEvents() {
      mGetEventsCallCount++;
      return super.getEvents();
    }

    int getEventsCallCount() {
      return mGetEventsCallCount;
    }
  }

  private static final class EventLookupUIBehavior extends Behavior {
    private EventLookupUI mLastCreatedUI;

    EventLookupUIBehavior() {
      super(EVENT_LOOKUP_UI_TAG);
    }

    @Override
    public LynxUI createUI(LynxContext context) {
      mLastCreatedUI = new EventLookupUI(context);
      return mLastCreatedUI;
    }
  }

  private static int getExternalMemoryCandidateCount(LynxUIOwner uiOwner) {
    try {
      Field field = LynxUIOwner.class.getDeclaredField("mRemovedUICandidateIds");
      field.setAccessible(true);
      return ((SparseBooleanArray) field.get(uiOwner)).size();
    } catch (NoSuchFieldException | IllegalAccessException exception) {
      throw new AssertionError(exception);
    }
  }

  private static WeakHashMap<LynxBaseUI, double[]> getPositionChangeRectCache(LynxUIOwner uiOwner) {
    try {
      Field field = LynxUIOwner.class.getDeclaredField("mLastPositionChangeRects");
      field.setAccessible(true);
      return (WeakHashMap<LynxBaseUI, double[]>) field.get(uiOwner);
    } catch (NoSuchFieldException | IllegalAccessException exception) {
      throw new AssertionError(exception);
    }
  }

  private LynxUI mLynxUI = null;
  private LynxContext mContext;
  private LynxView mLynxView;
  private UIBody mUIBody;

  @Before
  public void setUp() throws Exception {
    mContext = TestingUtils.getLynxContext();
    mUIBody = new UIBody(mContext, new UIBody.UIBodyView(mContext));
    mLynxView = new LynxView(mContext, new LynxViewBuilder());
    mContext.setLynxView(mLynxView);
    mLynxUI = mock(LynxUI.class);
    mLynxUI.updateLayout(
        0, 0, 100, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, new Rect(0, 0, 100, 100));

    LynxUI child = mock(LynxUI.class);
    child.updateLayout(0, 0, 50, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, new Rect(0, 0, 50, 50));
    mLynxUI.insertChild(child, 0);
  }

  @After
  public void tearDown() throws Exception {
    mLynxUI = null;
  }

  @Test
  public void testTouchPseudoEnabledAfterDispatcherCreation() {
    LynxContext contextSpy = spy(mContext);
    LynxUIOwner uiOwner = new LynxUIOwner(
        contextSpy, new BehaviorRegistry(new BuiltInBehavior().create()), mUIBody.getBodyView());
    TouchEventDispatcher dispatcher = mock(TouchEventDispatcher.class);
    contextSpy.setTouchEventDispatcher(dispatcher);

    assertFalse(uiOwner.getHasTouchPseudo());
    uiOwner.setHasTouchPseudo(false);
    uiOwner.setHasTouchPseudo(true);
    uiOwner.setHasTouchPseudo(false);
    uiOwner.setHasTouchPseudo(true);

    assertTrue(uiOwner.getHasTouchPseudo());
    verify(dispatcher).setHasTouchPseudo(true);
    verify(dispatcher, never()).setHasTouchPseudo(false);
  }

  @Test
  public void testGestureBindingSurvivesFlattenConversion() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxContext contextSpy = spy(mContext);
      LynxUIOwner uiOwner = new LynxUIOwner(
          contextSpy, new BehaviorRegistry(new BuiltInBehavior().create()), mUIBody.getBodyView());
      contextSpy.setLynxUIOwner(uiOwner);
      uiOwner.initGestureArenaManager(contextSpy);
      uiOwner.createViewInternal(1, LynxConstants.ROOT_TAG_NAME,
          new StylesDiffMap(new JavaOnlyMap()), null, false, 0, null);
      uiOwner.createViewInternal(
          2, "view", new StylesDiffMap(new JavaOnlyMap()), null, true, 1, null);
      uiOwner.insert(1, 2, 0);
      assertTrue(uiOwner.getNode(2).isFlatten());

      Map<Integer, GestureDetector> detectors = new HashMap<>();
      detectors.put(1, new GestureDetector(1, GestureDetector.GESTURE_TYPE_PAN, null, null));
      uiOwner.updateProperties(2, false, new StylesDiffMap(new JavaOnlyMap()), null, detectors);

      LynxBaseUI ui = uiOwner.getNode(2);
      assertFalse(ui.isFlatten());
      assertNotNull(ui.getGestureHandlers());
      assertNotNull(ui.getGestureHandlers().get(GestureDetector.GESTURE_TYPE_PAN));
      assertSame(ui, uiOwner.getGestureArenaManager().getMemberById(2));
    });
  }

  @Test
  public void testCopyUI() {
    // Avoid reporting it in the lower version of Android.
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.N) {
      return;
    }
    try {
      Class lynxEnvCls = LynxEnv.inst().getClass();
      Method method = lynxEnvCls.getDeclaredMethod("getBehaviors");
      List<Behavior> behaviors = (List<Behavior>) method.invoke(LynxEnv.inst());

      LynxContext contextSpy = spy(mContext);
      LynxUIOwner uiOwner =
          new LynxUIOwner(contextSpy, new BehaviorRegistry(behaviors), mUIBody.getBodyView());
      LynxBaseUI copiedUI = mLynxUI.clone();
      assertEquals(100, copiedUI.getWidth());
      assertEquals(100, copiedUI.getHeight());

      LynxBaseUI copiedChild = copiedUI.getChildAt(0);
      assertEquals(50, copiedChild.getWidth());
      assertEquals(50, copiedChild.getHeight());

    } catch (Throwable e) {
      e.printStackTrace();
    }
  }

  @Test
  public void testGetTagInfo() {
    try {
      List<Behavior> behaviors = new BuiltInBehavior().create();

      LynxContext contextSpy = spy(mContext);
      LynxUIOwner uiOwner =
          new LynxUIOwner(contextSpy, new BehaviorRegistry(behaviors), mUIBody.getBodyView());

      assertEquals(uiOwner.getTagInfo("list"), ShadowNodeType.COMMON);
      assertEquals(uiOwner.getTagInfo("text"), ShadowNodeType.CUSTOM);
      assertEquals(uiOwner.getTagInfo("view"), ShadowNodeType.COMMON);
      assertEquals(uiOwner.getTagInfo("raw-text"), ShadowNodeType.CUSTOM | ShadowNodeType.VIRTUAL);
      assertEquals(
          uiOwner.getTagInfo("inline-text"), ShadowNodeType.CUSTOM | ShadowNodeType.VIRTUAL);
      assertEquals(uiOwner.getTagInfo("frame"), ShadowNodeType.CUSTOM);
      assertEquals(uiOwner.getTagInfo("xxxx"), 0);
    } catch (Throwable e) {
      e.printStackTrace();
      fail();
    }
  }

  @Test
  public void testExternalMemoryRemovedIdCandidatesFollowUILifetime() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      List<Behavior> behaviors = new BuiltInBehavior().create();
      behaviors.add(new MemoryUIBehavior());
      LynxContext contextSpy = spy(mContext);
      LynxUIOwner uiOwner =
          new LynxUIOwner(contextSpy, new BehaviorRegistry(behaviors), mUIBody.getBodyView());
      contextSpy.setLynxUIOwner(uiOwner);

      uiOwner.createViewInternal(1, LynxConstants.ROOT_TAG_NAME, null, null, false, 0, null);
      uiOwner.createViewInternal(2, MEMORY_UI_TAG, null, null, false, 0, null);
      uiOwner.createViewInternal(3, MEMORY_UI_TAG, null, null, false, 0, null);
      uiOwner.insert(1, 2, 0);
      uiOwner.insert(2, 3, 0);
      uiOwner.cacheRemovedUIIds(new int[] {2, 2, 3});
      long[] snapshot = uiOwner.getExternalMemorySnapshot();
      assertEquals(20, snapshot[0]);
      assertEquals(0, snapshot[1]);

      uiOwner.remove(1, 2);
      uiOwner.cacheRemovedUIIds(new int[] {2, 2, 3});
      snapshot = uiOwner.getExternalMemorySnapshot();
      assertEquals(20, snapshot[0]);
      assertEquals(20, snapshot[1]);
      assertEquals(20, uiOwner.getExternalMemorySnapshot()[1]);
      assertEquals(2, getExternalMemoryCandidateCount(uiOwner));

      uiOwner.insert(1, 2, 0);
      assertEquals(0, uiOwner.getExternalMemorySnapshot()[1]);
      assertEquals(2, getExternalMemoryCandidateCount(uiOwner));
      uiOwner.remove(1, 2);
      assertEquals(20, uiOwner.getExternalMemorySnapshot()[1]);

      uiOwner.createViewInternal(4, MEMORY_UI_TAG, null, null, false, 0, null);
      uiOwner.insert(1, 4, 0);
      uiOwner.cacheRemovedUIIds(new int[] {2, 2, 3});
      uiOwner.remove(1, 4);
      uiOwner.cacheRemovedUIIds(new int[] {2, 4, 4});
      snapshot = uiOwner.getExternalMemorySnapshot();
      assertEquals(30, snapshot[0]);
      assertEquals(30, snapshot[1]);
      assertEquals(30, uiOwner.getExternalMemorySnapshot()[1]);

      uiOwner.destroy(-1, 4);
      assertEquals(2, getExternalMemoryCandidateCount(uiOwner));
      snapshot = uiOwner.getExternalMemorySnapshot();
      assertEquals(20, snapshot[0]);
      assertEquals(20, snapshot[1]);

      uiOwner.destroy(-1, 2);
      uiOwner.destroy(-1, 3);
      assertEquals(0, getExternalMemoryCandidateCount(uiOwner));
      assertEquals(0, uiOwner.getExternalMemorySnapshot()[1]);
    });
  }

  @Test
  public void testExternalMemorySnapshotPrunesMissingCandidates() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxContext contextSpy = spy(mContext);
      LynxUIOwner uiOwner = new LynxUIOwner(
          contextSpy, new BehaviorRegistry(new BuiltInBehavior().create()), mUIBody.getBodyView());

      uiOwner.cacheRemovedUIIds(new int[] {404, 405});
      assertEquals(2, getExternalMemoryCandidateCount(uiOwner));
      assertEquals(0, uiOwner.getExternalMemorySnapshot()[1]);
      assertEquals(0, getExternalMemoryCandidateCount(uiOwner));
    });
  }

  @Test
  public void testMemoryUsageDeduplicatesSharedOwnersWithoutReadSideEffects() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxContext context = spy(mContext);
      LynxUIOwner owner = new LynxUIOwner(
          context, new BehaviorRegistry(new BuiltInBehavior().create()), mUIBody.getBodyView());
      context.setLynxUIOwner(owner);
      // Equal values must still identify two independent allocations.
      Object firstOwner = new String("shared-owner");
      Object secondOwner = new String("shared-owner");
      SharedMemoryUI first = new SharedMemoryUI(context, 1, "surface", 10, 100, firstOwner);
      SharedMemoryUI second = new SharedMemoryUI(context, 2, "surface", 20, 100, firstOwner);
      SharedMemoryUI third = new SharedMemoryUI(context, 3, "surface-other", 30, 200, secondOwner);
      owner.setNode(1, first);
      owner.setNode(2, new UIShadowProxy(context, second));
      owner.setNode(3, third);
      MemoryUI ordinary = new MemoryUI(context);
      ordinary.setSign(4, MEMORY_UI_TAG);
      owner.setNode(4, ordinary);
      Set<Object> sharedOwners = Collections.newSetFromMap(new IdentityHashMap<>());
      sharedOwners.add(firstOwner);
      assertEquals(10, ordinary.getMemoryUsageBytesWithSharedOwners(sharedOwners));
      assertEquals(1, sharedOwners.size());

      Map<String, MemoryRecord> records = owner.getMemoryUsage();
      assertEquals(130, records.get("surface").mSizeBytes);
      assertEquals(2, records.get("surface").mInstanceCount);
      assertEquals("retained", records.get("surface").mDetail.get("detail-1"));
      assertEquals("retained", records.get("surface").mDetail.get("detail-2"));
      assertEquals(230, records.get("surface-other").mSizeBytes);
      assertEquals(10, records.get(MEMORY_UI_TAG).mSizeBytes);
      assertEquals(130, owner.getMemoryUsage().get("surface").mSizeBytes);

      // The same owner can appear under another tag in a subsequent collection.
      second.setSign(2, "surface-alias");
      first.mSharedBytes = second.mSharedBytes = 150;
      records = owner.getMemoryUsage();
      assertEquals(
          180, records.get("surface").mSizeBytes + records.get("surface-alias").mSizeBytes);
      assertEquals(1, records.get("surface").mInstanceCount);
      assertEquals(1, records.get("surface-alias").mInstanceCount);

      owner.destroy(-1, 1);
      owner.destroy(-1, 2);
      owner.destroy(-1, 3);
      records = owner.getMemoryUsage();
      assertFalse(records.containsKey("surface"));
      assertFalse(records.containsKey("surface-alias"));
      assertFalse(records.containsKey("surface-other"));
      assertEquals(10, records.get(MEMORY_UI_TAG).mSizeBytes);
      // A host diagnostic read and the later reporter read see the same live snapshot.
      assertFalse(owner.getMemoryUsage().containsKey("surface"));
      assertEquals(records.size(), owner.getMemoryUsage().size());
      owner.destroy(-1, 4);
      assertTrue(owner.getMemoryUsage().isEmpty());
      assertTrue(owner.getMemoryUsage().isEmpty());
    });
  }

  @Test
  public void testExternalMemoryShadowProxyDelegation() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxContext contextSpy = spy(mContext);
      LynxUIOwner uiOwner = new LynxUIOwner(
          contextSpy, new BehaviorRegistry(new BuiltInBehavior().create()), mUIBody.getBodyView());
      contextSpy.setLynxUIOwner(uiOwner);

      MemoryUI memoryUI = new MemoryUI(contextSpy);
      memoryUI.setSign(1, MEMORY_UI_TAG);
      MemoryUI descendant = new MemoryUI(contextSpy);
      descendant.setSign(2, MEMORY_UI_TAG);
      memoryUI.insertChild(descendant, 0);
      UIShadowProxy proxy = new UIShadowProxy(contextSpy, memoryUI);
      uiOwner.createViewInternal(100, LynxConstants.ROOT_TAG_NAME, null, null, false, 0, null);
      uiOwner.setNode(1, proxy);
      uiOwner.setNode(2, descendant);
      uiOwner.insert(100, 1, 0);

      assertEquals(20, uiOwner.getExternalMemorySnapshot()[0]);
      assertEquals(0, uiOwner.getExternalMemorySnapshot()[1]);

      uiOwner.remove(100, 1);
      uiOwner.cacheRemovedUIIds(new int[] {1});
      assertEquals(20, uiOwner.getExternalMemorySnapshot()[1]);
    });
  }

  @Test
  public void testPositionChangeListenerRegistration() {
    LynxUIOwner uiOwner = new LynxUIOwner(mContext, new BehaviorRegistry(), mUIBody.getBodyView());
    LynxBaseUI observedUI = mock(LynxBaseUI.class);
    Map<String, EventsListener> events = new HashMap<>();
    events.put("positionchange",
        new EventsListener("positionchange", "bindEvent", "onPositionChange", null, null));
    when(observedUI.getSign()).thenReturn(42);
    when(observedUI.getEvents()).thenReturn(events);

    uiOwner.setNode(42, observedUI);

    assertTrue(uiOwner.hasPositionChangeListenerForTesting(42));

    LynxBaseUI replacementUI = mock(LynxBaseUI.class);
    when(replacementUI.getSign()).thenReturn(42);
    when(replacementUI.getEvents()).thenReturn(new HashMap<>());
    uiOwner.setNode(42, replacementUI);

    assertFalse(uiOwner.hasPositionChangeListenerForTesting(42));

    LynxBaseUI globalBindUI = mock(LynxBaseUI.class);
    Map<String, EventsListener> globalEvents = new HashMap<>();
    globalEvents.put("positionchange",
        new EventsListener("positionchange", "global-bindEvent", "onPositionChange", null, null));
    when(globalBindUI.getSign()).thenReturn(42);
    when(globalBindUI.getEvents()).thenReturn(globalEvents);
    uiOwner.setNode(42, globalBindUI);

    assertFalse(uiOwner.hasPositionChangeListenerForTesting(42));

    UIShadowProxy proxy = mock(UIShadowProxy.class);
    LynxBaseUI proxyChild = mock(LynxBaseUI.class);
    when(proxy.getSign()).thenReturn(43);
    when(proxy.getEvents()).thenReturn(new HashMap<>());
    when(proxy.getChild()).thenReturn(proxyChild);
    when(proxyChild.getEvents()).thenReturn(events);
    uiOwner.setNode(43, proxy);

    assertTrue(uiOwner.hasPositionChangeListenerForTesting(43));
  }

  @Test
  public void testInitialPositionChangeListenerUsesEventMetadata() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      List<Behavior> behaviors = new BuiltInBehavior().create();
      EventLookupUIBehavior behavior = new EventLookupUIBehavior();
      behaviors.add(behavior);
      LynxContext contextSpy = spy(mContext);
      LynxUIOwner uiOwner =
          new LynxUIOwner(contextSpy, new BehaviorRegistry(behaviors), mUIBody.getBodyView());
      contextSpy.setLynxUIOwner(uiOwner);

      Map<String, EventsListener> events = new HashMap<>();
      events.put("positionchange",
          new EventsListener("positionchange", "bindEvent", "onPositionChange", null, null));
      uiOwner.createViewInternal(41, EVENT_LOOKUP_UI_TAG, null, events, false, 0, null);

      assertTrue(uiOwner.hasPositionChangeListenerForTesting(41));
      assertEquals(0, behavior.mLastCreatedUI.getEventsCallCount());

      uiOwner.createViewInternal(42, EVENT_LOOKUP_UI_TAG, null, null, false, 0, null);

      assertFalse(uiOwner.hasPositionChangeListenerForTesting(42));
      assertEquals(0, behavior.mLastCreatedUI.getEventsCallCount());
    });
  }

  @Test
  public void testFreshPositionChangeListenerDoesNotClearRectCache() {
    LynxUIOwner uiOwner = new LynxUIOwner(mContext, new BehaviorRegistry(), mUIBody.getBodyView());
    WeakHashMap<LynxBaseUI, double[]> rectCache = getPositionChangeRectCache(uiOwner);

    LynxBaseUI ui = mock(LynxBaseUI.class);
    when(ui.getSign()).thenReturn(41);
    when(ui.getEvents()).thenReturn(new HashMap<>());
    rectCache.put(ui, new double[] {1, 2, 3, 4});
    uiOwner.setNode(41, ui);

    assertTrue(rectCache.containsKey(ui));
    assertFalse(uiOwner.hasPositionChangeListenerForTesting(41));

    UIShadowProxy proxy = mock(UIShadowProxy.class);
    LynxBaseUI child = mock(LynxBaseUI.class);
    when(proxy.getSign()).thenReturn(42);
    when(proxy.getEvents()).thenReturn(new HashMap<>());
    when(proxy.getChild()).thenReturn(child);
    when(child.getEvents()).thenReturn(new HashMap<>());
    rectCache.put(proxy, new double[] {5, 6, 7, 8});
    uiOwner.setNode(42, proxy);

    assertTrue(rectCache.containsKey(proxy));
    assertFalse(uiOwner.hasPositionChangeListenerForTesting(42));
  }

  @Test
  public void testHasResponseChainEvent() {
    Map<String, EventsListener> events = new HashMap<>();

    events.put("tap", new EventsListener("tap", "", "", null, null));
    assertTrue(LynxUIOwner.hasResponseChainEvent(events, "tap"));

    events.put("tap", new EventsListener("tap", null, null, "", ""));
    assertTrue(LynxUIOwner.hasResponseChainEvent(events, "tap"));

    events.put("tap", new EventsListener("tap", null, null, null, null));
    assertFalse(LynxUIOwner.hasResponseChainEvent(events, "tap"));

    events.put("tap", new EventsListener("tap", "global-bindEvent", "onTap", null, null));
    assertFalse(LynxUIOwner.hasResponseChainEvent(events, "tap"));

    assertFalse(LynxUIOwner.hasResponseChainEvent(events, "missing"));
    assertFalse(LynxUIOwner.hasResponseChainEvent(null, "tap"));
  }
}
