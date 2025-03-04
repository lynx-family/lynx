// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.gesture.detector;

import android.util.Log;
import androidx.annotation.Nullable;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.LynxViewBuilder;
import com.lynx.tasm.PageConfig;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.gesture.GestureArenaMember;
import com.lynx.tasm.gesture.arena.GestureArenaManager;
import com.lynx.tasm.gesture.handler.BaseGestureHandler;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.*;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;

/**
 * Test class for the GestureDetectorManager, responsible for managing gesture detectors and arena
 * members.
 */
@RunWith(AndroidJUnit4.class)
public class GestureDetectorManagerTest {
  private static final String TAG = "GestureDetectorManagerTest";
  private LynxContext mContext;
  private LynxView mLynxView;
  private GestureDetectorManager mManager;
  private GestureDetector mDetector;
  private Map<Integer, Set<Integer>> mGestureToArenaMembers;

  private GestureArenaManager mArenaManager;

  private ConcreteArenaMember arenaMember1 = new ConcreteArenaMember() {
    // Implementation details specific to arenaMember1
    // This member has a gesture detector for WAIT_FOR relationship

    @Override
    public int getGestureArenaMemberId() {
      return 1;
    }

    @Override
    public int getSign() {
      return 1;
    }

    @Nullable
    @Override
    public Map<Integer, GestureDetector> getGestureDetectorMap() {
      Map<Integer, GestureDetector> map = new HashMap<>();
      Map<String, List<Integer>> relationMap = new HashMap<>();
      List<Integer> list = new ArrayList<>();
      list.add(2);
      relationMap.put(GestureDetector.WAIT_FOR, list);
      GestureDetector detector = new GestureDetector(1, 1, null, relationMap);
      map.put(1, detector);
      return map;
    }
  };
  private ConcreteArenaMember arenaMember2 = new ConcreteArenaMember() {
    // Implementation details specific to arenaMember2
    // This member has a gesture detector for SIMULTANEOUS relationship

    @Override
    public int getGestureArenaMemberId() {
      return 2;
    }

    @Override
    public int getSign() {
      return 2;
    }

    @Nullable
    @Override
    public Map<Integer, GestureDetector> getGestureDetectorMap() {
      Map<Integer, GestureDetector> map = new HashMap<>();
      Map<String, List<Integer>> relationMap = new HashMap<>();
      List<Integer> list = new ArrayList<>();
      list.add(3);
      relationMap.put(GestureDetector.SIMULTANEOUS, list);
      GestureDetector detector = new GestureDetector(2, 1, null, relationMap);
      map.put(2, detector);
      return map;
    }
  };

  ConcreteArenaMember arenaMember3 = new ConcreteArenaMember() {
    // Implementation details specific to arenaMember3
    // This member has a standalone gesture detector

    @Override
    public int getGestureArenaMemberId() {
      return 3;
    }

    @Override
    public int getSign() {
      return 3;
    }

    @Nullable
    @Override
    public Map<Integer, GestureDetector> getGestureDetectorMap() {
      Map<Integer, GestureDetector> map = new HashMap<>();
      GestureDetector detector = new GestureDetector(3, 1, null, null);
      map.put(3, detector);
      return map;
    }
  };

  ConcreteArenaMember arenaMember4 = new ConcreteArenaMember() {
    @Override
    public int getGestureArenaMemberId() {
      return 4;
    }

    @Override
    public int getSign() {
      return 4;
    }

    @Nullable
    @Override
    public Map<Integer, GestureDetector> getGestureDetectorMap() {
      Map<Integer, GestureDetector> map = new HashMap<>();
      Map<String, List<Integer>> relationMap = new HashMap<>();
      List<Integer> list = new ArrayList<>();
      list.add(2);
      list.add(3);
      relationMap.put(GestureDetector.WAIT_FOR, list);
      GestureDetector detector = new GestureDetector(4, 1, null, relationMap);
      map.put(4, detector);
      return map;
    }
  };

  ConcreteArenaMember arenaMember5 = new ConcreteArenaMember() {
    @Override
    public int getGestureArenaMemberId() {
      return 5;
    }

    @Override
    public int getSign() {
      return 5;
    }

    @Nullable
    @Override
    public Map<Integer, GestureDetector> getGestureDetectorMap() {
      Map<Integer, GestureDetector> map = new HashMap<>();
      Map<String, List<Integer>> relationMap = new HashMap<>();
      List<Integer> list = new ArrayList<>();
      list.add(3);
      list.add(2);
      relationMap.put(GestureDetector.WAIT_FOR, list);
      GestureDetector detector = new GestureDetector(5, 1, null, relationMap);
      map.put(5, detector);
      return map;
    }
  };

  /**
   * Concrete implementation of GestureArenaMember used for testing purposes.
   * This class serves as a mock member for the gesture arena and provides test-specific behavior.
   */
  class ConcreteArenaMember implements GestureArenaMember {
    @Override
    public void onGestureScrollBy(float deltaX, float deltaY) {}

    @Override
    public boolean canConsumeGesture(float deltaX, float deltaY) {
      return false;
    }

    @Override
    public boolean isAtBorder(boolean isStart) {
      return false;
    }

    @Override
    public int getSign() {
      return 0;
    }

    @Override
    public int getGestureArenaMemberId() {
      return 0;
    }

    @Override
    public int getMemberScrollX() {
      return 0;
    }

    @Override
    public int getMemberScrollY() {
      return 0;
    }

    @Override
    public void onInvalidate() {}

    @Nullable
    @Override
    public Map<Integer, GestureDetector> getGestureDetectorMap() {
      return null;
    }

    @Nullable
    @Override
    public Map<Integer, BaseGestureHandler> getGestureHandlers() {
      return null;
    }
  }
  @Before
  public void setUp() throws Exception {
    // Set up the testing environment before each test case
    // Initialize LynxContext, LynxView, GestureArenaManager, GestureDetectorManager, and detectors
    mContext = TestingUtils.getLynxContext();
    mLynxView = new LynxView(mContext, new LynxViewBuilder());
    mContext.setLynxView(mLynxView);
    JavaOnlyMap javaOnlyMap = Mockito.spy(JavaOnlyMap.class);
    javaOnlyMap.putBoolean("enableNewGesture", true);
    PageConfig config = new PageConfig(javaOnlyMap);
    mContext.onPageConfigDecoded(config);

    mArenaManager = new GestureArenaManager();
    mArenaManager.init(mContext.isEnableNewGesture(), mContext);
    mManager = new GestureDetectorManager(mArenaManager);
    Map<String, List<Integer>> map = new HashMap<>();
    List<String> gestureCallbackNames = new ArrayList<>();
    mDetector =
        new GestureDetector(1, GestureDetector.GESTURE_TYPE_DEFAULT, gestureCallbackNames, map);
    try {
      Field field = mManager.getClass().getDeclaredField("mGestureToArenaMembers");
      field.setAccessible(true);
      mGestureToArenaMembers = (Map<Integer, Set<Integer>>) field.get(mManager);
    } catch (Exception e) {
      Log.e(TAG, e.toString());
    }
  }

  @Test
  public void testGestureIdWithMemberId() {
    // Test case for registering and unregistering gesture detectors with the manager
    // Ensure that the manager correctly maps gesture IDs to associated arena member IDs
    mManager.registerGestureDetector(1, mDetector);
    Assert.assertEquals(1, mGestureToArenaMembers.size());
    mManager.unregisterGestureDetector(1, mDetector);
    Assert.assertEquals(0, mGestureToArenaMembers.size());
  }

  @Test
  public void testConvertResponseChainToCompeteChain() {
    // Test case for converting a response chain to a compete chain
    // Ensure that the conversion is performed correctly based on gesture detector relationships
    LinkedList<GestureArenaMember> responseList = new LinkedList<>();

    Assert.assertNotNull(arenaMember1.getGestureDetectorMap().get(1));
    Assert.assertNotNull(arenaMember2.getGestureDetectorMap().get(2));
    Assert.assertNotNull(arenaMember3.getGestureDetectorMap().get(3));

    mManager.registerGestureDetector(1, arenaMember1.getGestureDetectorMap().get(1));
    mManager.registerGestureDetector(2, arenaMember2.getGestureDetectorMap().get(2));
    mManager.registerGestureDetector(3, arenaMember3.getGestureDetectorMap().get(3));

    responseList.add(arenaMember1);
    responseList.add(arenaMember2);
    responseList.add(arenaMember3);

    Assert.assertEquals(responseList.size(), 3);

    LinkedList<GestureArenaMember> competeChain =
        mManager.convertResponseChainToCompeteChain(responseList);
    Assert.assertEquals(competeChain.size(), 2);

    Assert.assertEquals(
        competeChain.get(0).getGestureArenaMemberId(), arenaMember2.getGestureArenaMemberId());
    Assert.assertEquals(
        competeChain.get(1).getGestureArenaMemberId(), arenaMember1.getGestureArenaMemberId());
  }

  @Test
  public void testConvertResponseChainToCompeteChain1() {
    // Test case for converting a response chain to a compete chain
    // Ensure that the conversion is performed correctly based on gesture detector relationships
    LinkedList<GestureArenaMember> responseList = new LinkedList<>();

    Assert.assertNotNull(arenaMember2.getGestureDetectorMap().get(2));
    Assert.assertNotNull(arenaMember3.getGestureDetectorMap().get(3));
    Assert.assertNotNull(arenaMember4.getGestureDetectorMap().get(4));

    mManager.registerGestureDetector(4, arenaMember4.getGestureDetectorMap().get(4));
    mManager.registerGestureDetector(2, arenaMember2.getGestureDetectorMap().get(2));
    mManager.registerGestureDetector(3, arenaMember3.getGestureDetectorMap().get(3));

    responseList.add(arenaMember4);
    responseList.add(arenaMember2);
    responseList.add(arenaMember3);

    Assert.assertEquals(responseList.size(), 3);

    LinkedList<GestureArenaMember> competeChain =
        mManager.convertResponseChainToCompeteChain(responseList);
    Assert.assertEquals(competeChain.size(), 3);

    Assert.assertEquals(
        competeChain.get(0).getGestureArenaMemberId(), arenaMember2.getGestureArenaMemberId());
    Assert.assertEquals(
        competeChain.get(1).getGestureArenaMemberId(), arenaMember3.getGestureArenaMemberId());
    Assert.assertEquals(
        competeChain.get(2).getGestureArenaMemberId(), arenaMember4.getGestureArenaMemberId());
  }

  @Test
  public void testConvertResponseChainToCompeteChain2() {
    // Test case for converting a response chain to a compete chain
    // Ensure that the conversion is performed correctly based on gesture detector relationships
    LinkedList<GestureArenaMember> responseList = new LinkedList<>();

    Assert.assertNotNull(arenaMember2.getGestureDetectorMap().get(2));
    Assert.assertNotNull(arenaMember3.getGestureDetectorMap().get(3));
    Assert.assertNotNull(arenaMember5.getGestureDetectorMap().get(5));

    mManager.registerGestureDetector(5, arenaMember5.getGestureDetectorMap().get(5));
    mManager.registerGestureDetector(2, arenaMember2.getGestureDetectorMap().get(2));
    mManager.registerGestureDetector(3, arenaMember3.getGestureDetectorMap().get(3));

    responseList.add(arenaMember5);
    responseList.add(arenaMember2);
    responseList.add(arenaMember3);

    Assert.assertEquals(responseList.size(), 3);

    LinkedList<GestureArenaMember> competeChain =
        mManager.convertResponseChainToCompeteChain(responseList);
    Assert.assertEquals(competeChain.size(), 3);

    Assert.assertEquals(
        competeChain.get(0).getGestureArenaMemberId(), arenaMember3.getGestureArenaMemberId());
    Assert.assertEquals(
        competeChain.get(1).getGestureArenaMemberId(), arenaMember2.getGestureArenaMemberId());
    Assert.assertEquals(
        competeChain.get(2).getGestureArenaMemberId(), arenaMember5.getGestureArenaMemberId());
  }

  @Test
  public void testHandleSimultaneousWinner() {
    // Test case for handling simultaneous winners in the gesture arena
    // Ensure that the manager correctly identifies the competing members and selects the winner

    LinkedList<GestureArenaMember> responseList = new LinkedList<>();
    mManager.registerGestureDetector(1, arenaMember1.getGestureDetectorMap().get(1));
    mManager.registerGestureDetector(2, arenaMember2.getGestureDetectorMap().get(2));
    mManager.registerGestureDetector(3, arenaMember3.getGestureDetectorMap().get(3));
    mArenaManager.addMember(arenaMember1);
    mArenaManager.addMember(arenaMember2);
    mArenaManager.addMember(arenaMember3);

    HashSet<GestureArenaMember> members = mManager.handleSimultaneousWinner(arenaMember2).first;
    Assert.assertEquals(members.size(), 1);
    for (GestureArenaMember member : members) {
      Assert.assertEquals(member.getGestureArenaMemberId(), 3);
    }
  }

  @Test
  public void testFindCandidatesAfterCurrentInChain() {
    // Test case for finding candidates after the current member in the gesture chain
    // Ensure that the manager correctly identifies the candidates based on the chain and
    // relationships

    LinkedList<GestureArenaMember> responseList = new LinkedList<>();
    responseList.add(arenaMember1);
    responseList.add(arenaMember2);
    responseList.add(arenaMember3);
    Set<Integer> arenaMembers = new HashSet<>();
    arenaMembers.add(3);
    arenaMembers.add(2);

    try {
      Class<?> clazz = mManager.getClass();

      Method method = clazz.getDeclaredMethod("findCandidatesAfterCurrentInChain", LinkedList.class,
          GestureArenaMember.class, Set.class);

      method.setAccessible(true);

      LinkedList<Integer> result =
          (LinkedList<Integer>) method.invoke(mManager, responseList, arenaMember1, arenaMembers);
      Assert.assertEquals(result.size(), 2);
      Assert.assertEquals(1, (int) result.getFirst());
      Assert.assertEquals(2, (int) result.getLast());
    } catch (Exception e) {
      Log.e(TAG, e.toString());
    }
  }

  @Test
  public void testDestroy() {
    // Test case for destroying the GestureDetectorManager
    // Verify that the manager clears the registered detectors and associated mappings on
    // destruction

    mManager.registerGestureDetector(1, mDetector);
    Assert.assertEquals(1, mGestureToArenaMembers.size());
    mManager.onDestroy();
    Assert.assertEquals(0, mGestureToArenaMembers.size());
  }
}
