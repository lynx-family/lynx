// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.testbench;

import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.Looper;
import android.util.DisplayMetrics;
import android.view.Menu;
import android.view.MenuItem;
import android.view.WindowManager;
import android.widget.RelativeLayout;
import androidx.annotation.Nullable;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import com.google.gson.Gson;
import com.lynx.devtoolwrapper.LynxDevtoolGlobalHelper;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.provider.AbsTemplateProvider;
import com.lynx.tasm.utils.DisplayMetricsHolder;
import com.lynx.testbench.R;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.List;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class TestBenchMultiPagesActivity
    extends AppCompatActivity implements androidx.lifecycle.LifecycleOwner {
  private String url;
  private RelativeLayout mContainerView;
  private JSONArray mPages;
  private ArrayList<TestBenchView> mPageInstances;

  @Override
  protected void onCreate(@Nullable Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    Intent intent = getIntent();
    QueryMapUtils queryMap = new QueryMapUtils();
    url = intent.getStringExtra(TestBenchEnv.getInstance().mUriKey);
    mPageInstances = new ArrayList<TestBenchView>();
    queryMap.parse(url);
    if (queryMap.getBoolean("fullScreen", false)) {
      setContentView(R.layout.testbench_full_screen_activity);
      getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
    } else {
      setContentView(R.layout.testbench_multi_pages_activity);
      Toolbar toolbar = findViewById(R.id.toolbar);
      if (toolbar != null) {
        setSupportActionBar(toolbar);
        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
          actionBar.setDisplayShowTitleEnabled(false);
        }
      }
    }
    if (queryMap.getBoolean("landscape", false)) {
      setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
    }
    mContainerView = findViewById(R.id.container);
    getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_NOTHING);
    loadFile();
  }

  private void loadFile() {
    QueryMapUtils queryMap = new QueryMapUtils();
    queryMap.parse(url);
    String url = queryMap.getString("url");
    LynxEnv.inst().getTemplateProvider().loadTemplate(url, new AbsTemplateProvider.Callback() {
      @Override
      public void onSuccess(byte[] template) {
        if (!Thread.currentThread().equals(Looper.getMainLooper().getThread())) {
          throw new IllegalThreadStateException("Callback must be fired on main thread.");
        };
        Gson gson = new Gson();
        String str = new String(template, Charset.forName("UTF-8"));
        List list = gson.fromJson(str, List.class);
        mPages = new JSONArray(list);
        buildPages();
      }

      @Override
      public void onFailed(String msg) {
        return;
      }
    });
  }

  private void buildPages() {
    for (int i = 0; i < mPages.length(); i++) {
      try {
        JSONObject page = mPages.getJSONObject(i);
        String url = page.getString("url");
        int x = page.getJSONObject("frame").getInt("x");
        int y = page.getJSONObject("frame").getInt("y");
        TestBenchView tbView = new TestBenchView(this);
        mContainerView.addView(tbView);
        mPageInstances.add(tbView);
        tbView.loadPageWithPoint(url, new int[] {x, y}, getIntent());
      } catch (JSONException e) {
        e.printStackTrace();
      }
    }
  }

  @Override
  protected void onResume() {
    super.onResume();
    LynxDevtoolGlobalHelper helper = LynxDevtoolGlobalHelper.getInstance();
    helper.setContext(getApplicationContext());
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
    getMenuInflater().inflate(R.menu.testbench_menu, menu);
    return true;
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item) {
    if (item.getItemId() == R.id.action_refresh) {
      for (TestBenchView view : mPageInstances) {
        view.reload();
      }
    }
    return super.onOptionsItemSelected(item);
  }

  @Override
  public void onConfigurationChanged(Configuration newConfig) {
    super.onConfigurationChanged(newConfig);
    DisplayMetrics dm = DisplayMetricsHolder.getRealScreenDisplayMetrics(this);
    for (TestBenchView view : mPageInstances) {
      view.updateScreenMetrics(dm.widthPixels, dm.heightPixels);
    }
  }

  @Override
  protected void onDestroy() {
    super.onDestroy();
    for (TestBenchView view : mPageInstances) {
      view.destroy();
    }
  }
}
