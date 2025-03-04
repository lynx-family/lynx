// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.testbench;

import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;

import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.Menu;
import android.view.MenuItem;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleRegistry;
import com.lynx.devtoolwrapper.LynxDevtoolGlobalHelper;
import com.lynx.tasm.LynxGroup;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.utils.DisplayMetricsHolder;
import com.lynx.testbench.R;
import java.util.ArrayList;

public class TestBenchActivity
    extends AppCompatActivity implements androidx.lifecycle.LifecycleOwner {
  private static final String TAG = "TestBenchActivity";
  public static final String LYNX_VIEW_WIDTH = "LynxViewWidth";
  public static final String LYNX_VIEW_HEIGHT = "LynxViewHeight";
  public static final String LYNX_VIEW_DENSITY = "LynxViewDensity";
  private String url;
  private LifecycleRegistry mLifecycleRegistry = null;
  private LynxView mLynxView;
  private RelativeLayout mContainerView;
  private TestBenchActionManager mActionManager;
  private boolean mHasBeenRemoveFromPageStack = false;

  public LynxGroup getLynxGroup() {
    return mActionManager.getLynxGroup();
  }

  public void setLynxGroup(LynxGroup lynxGroup) {
    mActionManager.setLynxGroup(lynxGroup);
  }

  public void setState(boolean hasBeenRemoveFromPageStack) {
    mHasBeenRemoveFromPageStack = hasBeenRemoveFromPageStack;
  }

  @Override
  protected void onCreate(@Nullable Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    Intent intent = getIntent();
    QueryMapUtils queryMap = new QueryMapUtils();
    url = intent.getStringExtra(TestBenchEnv.getInstance().mUriKey);
    queryMap.parse(url);
    if (queryMap.getBoolean("fullScreen", false)) {
      setContentView(R.layout.testbench_full_screen_activity);
      getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
    } else {
      setContentView(R.layout.testbench_activity);
      setSupportActionBar((Toolbar) findViewById(R.id.toolbar));
      ActionBar actionBar = getSupportActionBar();
      if (actionBar != null) {
        actionBar.setDisplayShowTitleEnabled(false);
      }
    }
    if (queryMap.getBoolean("landscape", false)) {
      setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
    }
    mContainerView = findViewById(R.id.container);
    TestBenchReplayStateView stateView = new TestBenchReplayStateView(this);
    mContainerView.addView(stateView);
    getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_NOTHING);
    LynxGroup group = null;
    mActionManager = new TestBenchActionManager(intent, this, mContainerView, stateView, null);
    TestBenchPageManager.getInstance().initLynxGroup(intent.getStringExtra("groupName"), this);
    TestBenchPageManager.getInstance().registerActivity(intent.getStringExtra("pageName"), this);
    if (mLifecycleRegistry == null) {
      mLifecycleRegistry = new LifecycleRegistry(this);
    }
    mLifecycleRegistry.markState(Lifecycle.State.CREATED);
    mActionManager.registerCallback(new TestBenchActionCallback() {
      @Override
      public void onLynxViewDidBuild(@NonNull LynxView kitView, @NonNull Intent intent,
          @NonNull Context context, @NonNull ViewGroup viewGroup) {
        mLynxView = kitView;
        attachToView(viewGroup, intent.getIntExtra(LYNX_VIEW_WIDTH, MATCH_PARENT),
            intent.getIntExtra(LYNX_VIEW_HEIGHT, MATCH_PARENT));
      }
    });

    ArrayList<TestBenchActionCallback> externalCallbacks =
        TestBenchPageManager.getInstance().getCallbacks();
    for (TestBenchActionCallback callback : externalCallbacks) {
      mActionManager.registerCallback(callback);
    }
    mActionManager.startWithUrl(url);
  }

  private void attachToView(ViewGroup view, int width, int height) {
    if (mLynxView != null) {
      view.addView(mLynxView, new LinearLayout.LayoutParams(width, height));
    }
  }

  @Override
  public Lifecycle getLifecycle() {
    if (mLifecycleRegistry == null) {
      mLifecycleRegistry = new LifecycleRegistry(this);
    }
    return mLifecycleRegistry;
  }

  @Override
  protected void onResume() {
    super.onResume();
    if (mLynxView != null) {
      mLynxView.onEnterForeground();
    }
    // Pass context to debug bridge
    LynxDevtoolGlobalHelper helper = LynxDevtoolGlobalHelper.getInstance();
    helper.setContext(getApplicationContext());

    mLifecycleRegistry.markState(androidx.lifecycle.Lifecycle.State.RESUMED);
  }
  @Override
  protected void onStart() {
    super.onStart();
    mLifecycleRegistry.markState(androidx.lifecycle.Lifecycle.State.STARTED);
  }

  @Override
  protected void onPause() {
    super.onPause();
    if (mLynxView != null) {
      mLynxView.onEnterBackground();
    }
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
    getMenuInflater().inflate(R.menu.testbench_menu, menu);
    return true;
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item) {
    if (item.getItemId() == R.id.action_refresh) {
      mActionManager.load();
      if (mLynxView != null) {
        mLynxView.onEnterForeground();
      }
    }
    return super.onOptionsItemSelected(item);
  }

  @Override
  public void onConfigurationChanged(Configuration newConfig) {
    super.onConfigurationChanged(newConfig);
    DisplayMetrics dm = DisplayMetricsHolder.getRealScreenDisplayMetrics(this);
    if (mLynxView != null) {
      mLynxView.updateScreenMetrics(dm.widthPixels, dm.heightPixels);
    }
  }

  @Override
  protected void onDestroy() {
    super.onDestroy();
    TestBenchPageManager.getInstance().onActivityDestroy(
        getIntent().getStringExtra("pageName"), mHasBeenRemoveFromPageStack);

    if (mActionManager != null) {
      mActionManager.destroy();
    }
  }
}
