// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.helper.timingoverlay;

import android.content.Context;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.drawable.GradientDrawable;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.widget.FrameLayout;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.example.lynxdevtool.R;
import java.lang.ref.WeakReference;
import java.util.List;

/**
 * View that displays timing information overlay.
 * Android equivalent of iOS LynxTimingOverlayView.
 */
public class TimingOverlayView extends FrameLayout {
  /**
   * Interface for providing data to the overlay view.
   */
  public interface DataSource {
    /**
     * Check if the overlay should be hidden.
     * @return true if the overlay should be hidden, false otherwise
     */
    boolean overlayViewIsHidden();

    /**
     * Get the rectangle to highlight.
     * @return Rectangle to highlight or null if no highlight is needed
     */
    @Nullable Rect overlayViewHighlightRect();

    /**
     * Get the background color for the overlay.
     * @return Color to use for the overlay background
     */
    @Nullable Integer overlayViewBackgroundColor();

    /**
     * Get the lines of text to display in the overlay.
     * @return List of strings to display
     */
    @NonNull List<String> overlayViewDescriptionLines();
  }

  private TextView mOverlayInfo;
  private View mInfoBackground;
  private View mHighlightBorder;
  private boolean mNeedsUpdate = false;
  private DataSource mDataSource;
  private WeakReference<View> mTargetView;

  /**
   * Constructor.
   * @param context Android context
   * @param dataSource Data source for the overlay
   * @param targetView The view that the overlay should follow
   */
  public TimingOverlayView(
      @NonNull Context context, @NonNull DataSource dataSource, @NonNull View targetView) {
    super(context);
    mDataSource = dataSource;
    mTargetView = new WeakReference<>(targetView);

    // Inflate layout
    LayoutInflater.from(context).inflate(R.layout.timing_overlay_view, this, true);

    // Get views
    mOverlayInfo = findViewById(R.id.overlay_info);
    mInfoBackground = findViewById(R.id.info_background);
    mHighlightBorder = findViewById(R.id.highlight_border);

    // Set up view
    setUserInteractionEnabled(false);
    setVisibility(GONE);

    // Set up highlight border
    GradientDrawable borderDrawable = new GradientDrawable();
    borderDrawable.setStroke(2, Color.argb(128, 128, 0, 0));
    borderDrawable.setColor(Color.TRANSPARENT);
    mHighlightBorder.setBackground(borderDrawable);

    // Add layout listener to target view to update our position when it changes
    targetView.getViewTreeObserver().addOnGlobalLayoutListener(
        new ViewTreeObserver.OnGlobalLayoutListener() {
          @Override
          public void onGlobalLayout() {
            updatePositionToMatchTargetView();
          }
        });
  }

  /**
   * Set user interaction enabled/disabled.
   * @param enabled true to enable user interaction, false to disable
   */
  private void setUserInteractionEnabled(boolean enabled) {
    setClickable(!enabled);
    setFocusable(!enabled);
  }

  @Override
  protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
    super.onLayout(changed, left, top, right, bottom);

    if (mNeedsUpdate) {
      updateData();
      mNeedsUpdate = false;
    }
  }

  /**
   * Updates the position and size of this overlay to match the target view.
   * This ensures the overlay's frame consistently matches the target view's frame,
   * even though they may not share the same parent view.
   */
  private void updatePositionToMatchTargetView() {
    View targetView = mTargetView != null ? mTargetView.get() : null;
    if (targetView == null || !targetView.isAttachedToWindow()) {
      return;
    }

    ViewGroup parent = (ViewGroup) getParent();
    if (parent == null || !parent.isAttachedToWindow()) {
      return;
    }

    try {
      // Get the target view's position in screen coordinates
      int[] targetLocation = new int[2];
      targetView.getLocationOnScreen(targetLocation);

      // Get our parent view's position in screen coordinates
      int[] parentLocation = new int[2];
      parent.getLocationOnScreen(parentLocation);

      // Calculate the relative position of the target view to our parent
      int relativeLeft = targetLocation[0] - parentLocation[0];
      int relativeTop = targetLocation[1] - parentLocation[1];

      // Update our layout params to match the target view's position and size
      ViewGroup.LayoutParams currentParams = getLayoutParams();
      if (currentParams instanceof ViewGroup.MarginLayoutParams) {
        ViewGroup.MarginLayoutParams params = (ViewGroup.MarginLayoutParams) currentParams;
        params.leftMargin = relativeLeft;
        params.topMargin = relativeTop;
        params.width = targetView.getWidth();
        params.height = targetView.getHeight();
        setLayoutParams(params);
      }
    } catch (IllegalArgumentException e) {
      // This can happen if the view is in the process of being detached
      // Just ignore and we'll update next time
    }
  }

  /**
   * Mark the view as needing an update.
   */
  public void setNeedsUpdate() {
    mNeedsUpdate = true;
    requestLayout();
  }

  /**
   * Update the view with data from the data source.
   */
  private void updateData() {
    View targetView = mTargetView != null ? mTargetView.get() : null;
    if (targetView == null) {
      setVisibility(GONE);
      return;
    }

    // Update visibility based on FSP state
    if (mDataSource.overlayViewIsHidden()) {
      setVisibility(GONE);
    } else {
      setVisibility(VISIBLE);
    }

    // Update background color based on FSP state
    Integer backgroundColor = mDataSource.overlayViewBackgroundColor();
    if (backgroundColor != null) {
      setBackgroundColor(backgroundColor);
    } else {
      setBackgroundColor(Color.argb(64, 255, 255, 255)); // Default: translucent white
    }

    // Update label text
    List<String> lines = mDataSource.overlayViewDescriptionLines();
    if (lines != null && !lines.isEmpty()) {
      mOverlayInfo.setText(TextUtils.join("\n", lines));
    } else {
      mOverlayInfo.setText("");
    }

    // Update highlight
    Rect highlightRect = mDataSource.overlayViewHighlightRect();
    setHighlightRect(highlightRect);

    // Update info background size to match text
    mOverlayInfo.post(() -> {
      ViewGroup.LayoutParams params = mInfoBackground.getLayoutParams();
      params.width = mOverlayInfo.getWidth() + 24; // Add padding
      params.height = mOverlayInfo.getHeight() + 24; // Add padding
      mInfoBackground.setLayoutParams(params);
    });
  }

  /**
   * Set the rectangle to highlight.
   * @param rect Rectangle to highlight or null to hide highlight
   */
  private void setHighlightRect(@Nullable Rect rect) {
    if (rect == null || rect.isEmpty()) {
      mHighlightBorder.setVisibility(GONE);
      return;
    }

    // Update highlight border position and size
    FrameLayout.LayoutParams params = (FrameLayout.LayoutParams) mHighlightBorder.getLayoutParams();
    params.leftMargin = rect.left;
    params.topMargin = rect.top;
    params.width = rect.width();
    params.height = rect.height();
    mHighlightBorder.setLayoutParams(params);

    mHighlightBorder.setVisibility(VISIBLE);
    bringChildToFront(mHighlightBorder);
  }
}
