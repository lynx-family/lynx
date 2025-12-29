// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.recorder;

import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.Looper;
import android.util.DisplayMetrics;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.WindowManager;
import android.widget.RelativeLayout;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import com.example.lynxdevtool.R;
import com.google.gson.Gson;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.provider.AbsTemplateProvider;
import com.lynx.tasm.utils.DisplayMetricsHolder;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.List;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class LynxRecorderMultiPagesActivity
    extends AppCompatActivity implements androidx.lifecycle.LifecycleOwner {
  private String url;
  private RelativeLayout mContainerView;
  private JSONArray mPages;
  private ArrayList<LynxRecorderView> mPageInstances;

  @Override
  protected void onCreate(@Nullable Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    Intent intent = getIntent();
    QueryMapUtils queryMap = new QueryMapUtils();
    url = intent.getStringExtra(LynxRecorderEnv.getInstance().mUriKey);
    mPageInstances = new ArrayList<LynxRecorderView>();
    queryMap.parse(url);
    if (queryMap.getBoolean("fullScreen", false)) {
      setContentView(R.layout.recorder_full_screen_activity);
      getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
    } else {
      setContentView(R.layout.recorder_multi_pages_activity);
      findViewById(R.id.action_refresh).setOnClickListener(new View.OnClickListener() {
        @Override
        public void onClick(View v) {
          for (LynxRecorderView view : mPageInstances) {
            view.reload();
          }
        }
      });
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
        LynxRecorderView tbView = new LynxRecorderView(this);
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
  }

  @Override
  public void onConfigurationChanged(Configuration newConfig) {
    super.onConfigurationChanged(newConfig);
    DisplayMetrics dm = DisplayMetricsHolder.getRealScreenDisplayMetrics(this);
    for (LynxRecorderView view : mPageInstances) {
      view.updateScreenMetrics(dm.widthPixels, dm.heightPixels);
    }
  }

  @Override
  protected void onDestroy() {
    super.onDestroy();
    for (LynxRecorderView view : mPageInstances) {
      view.destroy();
    }
  }
}
