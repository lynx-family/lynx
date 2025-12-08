// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import static org.junit.Assert.*;
import static org.mockito.ArgumentMatchers.*;
import static org.mockito.Mockito.*;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.view.View;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.tasm.behavior.LynxContext;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@RunWith(AndroidJUnit4.class)
public class ContainerRendererTest {
  @Mock private PlatformRendererContext mockPlatformRendererContext;
  @Mock private Canvas mockCanvas;
  private Context realContext;
  private LynxContext realLynxContext;

  private ContainerRenderer containerRenderer;
  private static final int TEST_SIGN = 123;
  private static final int TEST_LEFT = 10;
  private static final int TEST_TOP = 20;
  private static final int TEST_RIGHT = 100;
  private static final int TEST_BOTTOM = 200;

  @Before
  public void setUp() {
    MockitoAnnotations.openMocks(this);

    // Use real Android context from instrumentation test
    realContext = InstrumentationRegistry.getInstrumentation().getTargetContext();

    // Create a simple LynxContext for testing - we'll mock the specific methods we need
    realLynxContext = spy(new TestLynxContext(realContext));

    containerRenderer = new ContainerRenderer(realLynxContext);
    Renderer renderer = containerRenderer.createRenderer(mockPlatformRendererContext, TEST_SIGN);
    renderer.setRenderHost(containerRenderer);
    containerRenderer.setRenderer(renderer);
  }

  // Simple test implementation of LynxContext
  private static class TestLynxContext extends LynxContext {
    public TestLynxContext(Context base) {
      super(base, base.getResources().getDisplayMetrics()); // Use real DisplayMetrics from context
    }

    @Override
    public void handleException(Exception e) {
      // Simple implementation for testing
      throw new RuntimeException(e);
    }
  }

  @Test
  public void testConstructor() {
    assertNotNull("ContainerRenderer should be created", containerRenderer);
    assertFalse("WillNotDraw should be false", containerRenderer.willNotDraw());
  }

  @Test
  public void testSetLynxFrame() {
    containerRenderer.getRenderer().setLynxFrame(
        TEST_LEFT, TEST_TOP, TEST_RIGHT, TEST_BOTTOM, 0, 0);

    Rect frame = containerRenderer.getRenderer().getLynxFrame();
    assertNotNull("LynxFrame should not be null", frame);
    assertEquals("Left should be set correctly", TEST_LEFT, frame.left);
    assertEquals("Top should be set correctly", TEST_TOP, frame.top);
    assertEquals("Right should be set correctly", TEST_RIGHT, frame.right);
    assertEquals("Bottom should be set correctly", TEST_BOTTOM, frame.bottom);
  }

  @Test
  public void testGetLynxFrame() {
    Rect initialFrame = containerRenderer.getRenderer().getLynxFrame();
    assertNotNull("Initial LynxFrame should not be null", initialFrame);
    assertEquals("Initial frame should be empty", 0, initialFrame.left);
    assertEquals("Initial frame should be empty", 0, initialFrame.top);
    assertEquals("Initial frame should be empty", 0, initialFrame.right);
    assertEquals("Initial frame should be empty", 0, initialFrame.bottom);
  }

  @Test
  public void testOnLayout_WithContainerRendererChildren() {
    // Create a real ContainerRenderer child instead of mocking it
    ContainerRenderer child = new ContainerRenderer(realLynxContext);
    Renderer renderer = child.createRenderer(mockPlatformRendererContext, TEST_SIGN + 1);
    renderer.setRenderHost(child);
    child.setRenderer(renderer);
    Rect childFrame = new Rect(5, 10, 50, 60);
    child.getRenderer().setLynxFrame(5, 10, 50, 60, 0, 0);

    // Add child to container
    containerRenderer.addView(child);

    // Call onLayout
    containerRenderer.onLayout(true, 0, 0, 100, 100);

    // Verify child was laid out with correct parameters
    assertEquals("Child left should be set correctly", 5, child.getLeft());
    assertEquals("Child top should be set correctly", 10, child.getTop());
    assertEquals("Child right should be set correctly", 50, child.getRight());
    assertEquals("Child bottom should be set correctly", 60, child.getBottom());
  }

  @Test
  public void testOnLayout_WithNonContainerRendererChildren() {
    // Create mock regular View child
    View mockChild = mock(View.class);

    // Add child to container
    containerRenderer.addView(mockChild);

    // Call onLayout
    containerRenderer.onLayout(true, 0, 0, 100, 100);

    // Verify layout was not called for non-ContainerRenderer child
    verify(mockChild, never()).layout(anyInt(), anyInt(), anyInt(), anyInt());
  }

  @Test
  public void testOnLayout_MultipleChildren() {
    // Create real ContainerRenderer children instead of mocking them
    ContainerRenderer child1 = new ContainerRenderer(realLynxContext);
    Renderer renderer1 = child1.createRenderer(mockPlatformRendererContext, TEST_SIGN + 1);
    renderer1.setRenderHost(child1);
    child1.setRenderer(renderer1);
    ContainerRenderer child2 = new ContainerRenderer(realLynxContext);
    Renderer renderer2 = child2.createRenderer(mockPlatformRendererContext, TEST_SIGN + 2);
    renderer2.setRenderHost(child2);
    child2.setRenderer(renderer2);

    View regularChild = mock(View.class);

    // Set frames for ContainerRenderer children
    child1.getRenderer().setLynxFrame(0, 0, 50, 50, 0, 0);
    child2.getRenderer().setLynxFrame(50, 50, 100, 100, 0, 0);

    // Add children
    containerRenderer.addView(child1);
    containerRenderer.addView(regularChild);
    containerRenderer.addView(child2);

    // Call onLayout
    containerRenderer.onLayout(true, 0, 0, 100, 100);

    // Verify only ContainerRenderer children were laid out with correct coordinates
    assertEquals("Child1 left should be set correctly", 0, child1.getLeft());
    assertEquals("Child1 top should be set correctly", 0, child1.getTop());
    assertEquals("Child1 right should be set correctly", 50, child1.getRight());
    assertEquals("Child1 bottom should be set correctly", 50, child1.getBottom());

    assertEquals("Child2 left should be set correctly", 50, child2.getLeft());
    assertEquals("Child2 top should be set correctly", 50, child2.getTop());
    assertEquals("Child2 right should be set correctly", 100, child2.getRight());
    assertEquals("Child2 bottom should be set correctly", 100, child2.getBottom());

    // Verify regular child was not laid out
    verify(regularChild, never()).layout(anyInt(), anyInt(), anyInt(), anyInt());
  }

  @Test
  public void testOnDraw_InitializesDisplayListApplier() {
    // Setup mock DisplayList
    DisplayList mockDisplayList = mock(DisplayList.class);
    mockDisplayList.ops = new int[] {};
    mockDisplayList.iArgv = new int[] {};
    mockDisplayList.fArgv = new float[] {};

    // Mock PlatformRendererContext to return our DisplayList
    doAnswer(invocation -> {
      DisplayList list = invocation.getArgument(1);
      list.ops = mockDisplayList.ops;
      list.iArgv = mockDisplayList.iArgv;
      list.fArgv = mockDisplayList.fArgv;
      return null;
    })
        .when(mockPlatformRendererContext)
        .getDisplayList(eq(TEST_SIGN), any(DisplayList.class));

    // Call onDraw
    containerRenderer.onDraw(mockCanvas);

    // Verify DisplayList was retrieved
    verify(mockPlatformRendererContext).getDisplayList(eq(TEST_SIGN), any(DisplayList.class));
  }

  @Test
  public void testOnDraw_UpdatesExistingDisplayListApplier() {
    // First call to initialize DisplayListApplier
    DisplayList initialDisplayList = new DisplayList();
    initialDisplayList.ops = new int[] {1, 2, 3};
    initialDisplayList.iArgv = new int[] {4, 5, 6};
    initialDisplayList.fArgv = new float[] {7.0f, 8.0f, 9.0f};

    doAnswer(invocation -> {
      DisplayList list = invocation.getArgument(1);
      list.ops = initialDisplayList.ops.clone();
      list.iArgv = initialDisplayList.iArgv.clone();
      list.fArgv = initialDisplayList.fArgv.clone();
      return null;
    })
        .when(mockPlatformRendererContext)
        .getDisplayList(eq(TEST_SIGN), any(DisplayList.class));

    // First onDraw call
    containerRenderer.onDraw(mockCanvas);

    // Update DisplayList
    DisplayList updatedDisplayList = new DisplayList();
    updatedDisplayList.ops = new int[] {10, 11, 12};
    updatedDisplayList.iArgv = new int[] {13, 14, 15};
    updatedDisplayList.fArgv = new float[] {16.0f, 17.0f, 18.0f};

    doAnswer(invocation -> {
      DisplayList list = invocation.getArgument(1);
      list.ops = updatedDisplayList.ops.clone();
      list.iArgv = updatedDisplayList.iArgv.clone();
      list.fArgv = updatedDisplayList.fArgv.clone();
      return null;
    })
        .when(mockPlatformRendererContext)
        .getDisplayList(eq(TEST_SIGN), any(DisplayList.class));

    // Second onDraw call
    containerRenderer.onDraw(mockCanvas);

    // Verify DisplayList was retrieved twice
    verify(mockPlatformRendererContext, times(2))
        .getDisplayList(eq(TEST_SIGN), any(DisplayList.class));
  }

  @Test
  public void testLayoutWithEmptyContainer() {
    // Call onLayout with no children
    containerRenderer.onLayout(true, 0, 0, 100, 100);

    // Should complete without exceptions
    assertEquals("Should have no children", 0, containerRenderer.getChildCount());
  }

  @Test
  public void testFrameBoundsValidation() {
    // Test with negative coordinates
    containerRenderer.getRenderer().setLynxFrame(-10, -20, 50, 60, 0, 0);
    Rect frame = containerRenderer.getRenderer().getLynxFrame();
    assertEquals("Should handle negative left", -10, frame.left);
    assertEquals("Should handle negative top", -20, frame.top);

    // Test with zero dimensions
    containerRenderer.getRenderer().setLynxFrame(0, 0, 0, 0, 0, 0);
    frame = containerRenderer.getRenderer().getLynxFrame();
    assertEquals("Should handle zero width", 0, frame.right);
    assertEquals("Should handle zero height", 0, frame.bottom);
  }

  @Test
  public void testChildLayoutWithDifferentFrameSizes() {
    // Test various frame sizes
    int[][] testFrames = {
        {0, 0, 1, 1}, // Minimal size
        {10, 20, 100, 200}, // Normal size
        {0, 0, 1000, 1000}, // Large size
        {-50, -50, 50, 50} // Negative origin
    };

    for (int[] frame : testFrames) {
      // Create a real ContainerRenderer child instead of mocking it
      ContainerRenderer child = new ContainerRenderer(realLynxContext);
      Renderer renderer = child.createRenderer(mockPlatformRendererContext, TEST_SIGN + 1);
      renderer.setRenderHost(child);
      child.setRenderer(renderer);
      child.getRenderer().setLynxFrame(frame[0], frame[1], frame[2], frame[3], 0, 0);

      containerRenderer.addView(child);
      containerRenderer.onLayout(true, 0, 0, 200, 200);

      // Verify the child was laid out with correct parameters
      assertEquals("Child left should be set correctly", frame[0], child.getLeft());
      assertEquals("Child top should be set correctly", frame[1], child.getTop());
      assertEquals("Child right should be set correctly", frame[2], child.getRight());
      assertEquals("Child bottom should be set correctly", frame[3], child.getBottom());

      containerRenderer.removeView(child);
    }
  }
}
