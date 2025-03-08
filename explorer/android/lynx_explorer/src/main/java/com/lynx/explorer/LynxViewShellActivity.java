// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer;

import android.content.Context;
import android.content.Intent;
import android.content.res.AssetManager;
import android.content.res.Configuration;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.DisplayCutout;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.WindowInsets;
import android.widget.FrameLayout;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsControllerCompat;
import com.lynx.explorer.input.LynxExplorerInput;
import com.lynx.explorer.provider.DemoGenericResourceFetcher;
import com.lynx.explorer.provider.DemoMediaResourceFetcher;
import com.lynx.explorer.provider.DemoTemplateResourceFetcher;
import com.lynx.explorer.utils.QueryMapUtils;
import com.lynx.tasm.LynxBooleanOption;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.LynxViewBuilder;
import com.lynx.tasm.TemplateData;
import com.lynx.tasm.TimingHandler;
import com.lynx.tasm.behavior.Behavior;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.utils.DisplayMetricsHolder;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;
import java.util.Map;

public class LynxViewShellActivity extends AppCompatActivity {
  public static final String URL_KEY = "url";
  private static final String URL_PREFIX = "file://lynx?local://";
  private static final String TAG = "LynxViewShellActivity";
  private static final String HOME_PAGE_URL = "file://lynx?local://homepage.lynx.bundle?fullscreen=true";
  private static final String DEFAULT_TOP_BAR_COLOR = "#F0F2F5";
  private static final String DEFAULT_TOP_BAR_TITLE_COLOR = "#000000";
  private static final String DEFAULT_TOP_BAR_BACK_BUTTON_STYLE = "light";
  private ViewGroup mLynxContainer;
  private LynxView mLynxView;
  private String mFrontendTheme;
  private TimingHandler.ExtraTimingInfo extraTimingInfo = new TimingHandler.ExtraTimingInfo();

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    extraTimingInfo.mOpenTime = System.currentTimeMillis();
    extraTimingInfo.mContainerInitStart = System.currentTimeMillis();

    Intent intent = getIntent();
    String url = intent.getStringExtra(URL_KEY);
    if (url == null) {
      url = HOME_PAGE_URL;
    }

    setTopBarAppearance(url);
    mLynxContainer = findViewById(R.id.lynx_container);

    extraTimingInfo.mContainerInitEnd = System.currentTimeMillis();

    openTargetUrl(url);
  }

  @Override
  protected void onDestroy() {
    if (mLynxView != null) {
      mLynxView.destroy();
    }
    super.onDestroy();
  }

  private void setTopBarAppearance(String url) {
    if (isAssetFilename(url)) {
      url = getAssetFilename(url);
    }

    QueryMapUtils queryMap = new QueryMapUtils();
    queryMap.parse(url);
    boolean isFullscreen = queryMap.getBoolean("fullscreen", false);

    if (!isFullscreen) {
      setContentView(R.layout.default_display);

      Toolbar toolbar = findViewById(R.id.toolbar);

      setWindowColor(toolbar, queryMap);
      setBackButtonStyle(toolbar, queryMap);
      setActionBarTitle(toolbar, queryMap);

      setSupportActionBar(toolbar);
      getSupportActionBar().setDisplayShowTitleEnabled(false);
      getSupportActionBar().setDisplayHomeAsUpEnabled(true);
    } else {
      setContentView(R.layout.fullscreen_display);
      setStatusBarAppearance();
    }
  }

  private void setWindowColor(Toolbar toolbar, QueryMapUtils queryMap) {
    String color = queryMap.contains("bar_color") ? "#" + queryMap.getString("bar_color")
        : DEFAULT_TOP_BAR_COLOR;

    toolbar.setBackgroundColor(Color.parseColor(color));
    getWindow().setStatusBarColor(Color.parseColor(color));

    // set background color as the action bar color for better visual experience
    getWindow().getDecorView().setBackgroundColor(Color.parseColor(color));
  }

  private void setBackButtonStyle(Toolbar toolbar, QueryMapUtils queryMap) {
    String backButtonStyle = queryMap.contains("back_button_style")
        ? queryMap.getString("back_button_style")
        : DEFAULT_TOP_BAR_BACK_BUTTON_STYLE;
    if (backButtonStyle.equals("dark")) {
      toolbar.setNavigationIcon(R.drawable.back_dark);

      mFrontendTheme = "dark";
    } else {
      toolbar.setNavigationIcon(R.drawable.back_light);

      mFrontendTheme = "light";
    }
  }

  private void setActionBarTitle(Toolbar toolbar, QueryMapUtils queryMap) {
    TextView tv = toolbar.findViewById(R.id.toolbar_title);
    String title = queryMap.contains("title") ? queryMap.getString("title") : null;
    String titleColor = queryMap.contains("title_color") ? "#" + queryMap.getString("title_color")
        : DEFAULT_TOP_BAR_TITLE_COLOR;

    if (tv != null) {
      tv.setText(title);
      tv.setTextColor(Color.parseColor(titleColor));
    }
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item) {
    if (item.getItemId() == android.R.id.home) {
      onBackPressed();
      return true;
    }
    return super.onOptionsItemSelected(item);
  }


public boolean isNotchScreen() {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.P) {
        return false;
    }
    
    WindowManager windowManager = getSystemService(WindowManager.class);
    if (windowManager == null) {
        return false;
    }


    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
      return getWindow().getDecorView()
          .getRootWindowInsets()
          .getDisplayCutout() != null;
    } else {
      Display display = windowManager.getDefaultDisplay();
      DisplayCutout cutout = null;

      try {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
          cutout = display.getCutout();
        } else {
          WindowInsets windowInsets = getWindow().getDecorView().getRootWindowInsets();
          if (windowInsets != null) {
            cutout = windowInsets.getDisplayCutout();
          }
        }
      } catch (Exception e) {
        Log.e(TAG, "Error checking for notch: " + e.getMessage());
        return false;
      }

      return cutout != null;
  }
  }

  private void setStatusBarAppearance() {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.P) {
      return;
    }

    if (isNotchScreen()) {
      WindowManager.LayoutParams params = getWindow().getAttributes();
      params.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
      getWindow().setAttributes(params);
      getWindow().getDecorView().setSystemUiVisibility(
          View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN);
    }

    WindowInsetsControllerCompat windowInsetsController = WindowCompat.getInsetsController(getWindow(),
        getWindow().getDecorView());

    windowInsetsController.setSystemBarsBehavior(
        WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
  }

  private void openTargetUrl(String url) {
    if (url == null) {
      Log.i(TAG, "openTargetUrl failed: url is null.");
      return;
    }

    LynxViewBuilder builder = new LynxViewBuilder();
    builder.addBehaviors(new ImageBehavior().create());
    builder.addBehavior(new Behavior("input", false) {
      @Override
      public LynxExplorerInput createUI(LynxContext context) {
        return new LynxExplorerInput(context);
      }
    });
    builder.setEnableGenericResourceFetcher(LynxBooleanOption.TRUE);
    builder.setGenericResourceFetcher(new DemoGenericResourceFetcher());
    builder.setTemplateResourceFetcher(new DemoTemplateResourceFetcher(this));
    builder.setMediaResourceFetcher(new DemoMediaResourceFetcher());
    // Parse the URL parameters and specify the LynxView width, height, and density
    // according to the
    // parameters.
    QueryMapUtils queryMap = new QueryMapUtils();
    queryMap.parse(url);
    if (queryMap.contains("width") && queryMap.contains("height")) {
      builder.setPresetMeasuredSpec(
          View.MeasureSpec.makeMeasureSpec(queryMap.getInt("width", 720), View.MeasureSpec.EXACTLY),
          View.MeasureSpec.makeMeasureSpec(
              queryMap.getInt("height", 1280), View.MeasureSpec.EXACTLY));
    }
    if (queryMap.contains("density")) {
      builder.setDensity(queryMap.getFloat("density", 320) / 160.f);
    }
    LynxView lynxView = builder.build(this);
    lynxView.updateGlobalProps(getGlobalProps(this));
    extraTimingInfo.mPrepareTemplateStart = System.currentTimeMillis();

    // Add a mock initData as example.
    Map<String, Object> initData = new HashMap<>();
    initData.put("mockData", "Hello Lynx Explorer");

    if (isAssetFilename(url)) {
      url = getAssetFilename(url);

      String[] strs = url.split("[?]");
      if (strs.length > 1) {
        url = strs[0];
      }

      strs = url.split("[&]");
      if (strs.length > 1) {
        url = strs[0];
      }

      byte[] templateBundleData = readFileFromAssets(this, url);
      extraTimingInfo.mPrepareTemplateEnd = System.currentTimeMillis();
      lynxView.setExtraTiming(extraTimingInfo);
      lynxView.renderTemplateWithBaseUrl(templateBundleData, initData, url);
    } else if (url.startsWith("https://") || url.startsWith("http://")) {
      extraTimingInfo.mPrepareTemplateEnd = System.currentTimeMillis();
      lynxView.setExtraTiming(extraTimingInfo);
      lynxView.renderTemplateUrl(url, initData);
    } else if (url.startsWith("assets://")) {
      url = url.substring("assets://".length());
      byte[] templateBundleData = readFileFromAssets(this, url);
      extraTimingInfo.mPrepareTemplateEnd = System.currentTimeMillis();
      lynxView.setExtraTiming(extraTimingInfo);
      lynxView.renderTemplateWithBaseUrl(templateBundleData, initData, url);
    } else {
      Log.i(TAG, "openTargetUrl failed: not supported url.");
    }
    mLynxContainer.addView(lynxView,
        new FrameLayout.LayoutParams(queryMap.getInt("width", ViewGroup.LayoutParams.MATCH_PARENT),
            queryMap.getInt("height", ViewGroup.LayoutParams.MATCH_PARENT)));
    mLynxView = lynxView;
  }

  private TemplateData getGlobalProps(Context context) {
    DisplayMetrics displayMetrics = DisplayMetricsHolder.getRealScreenDisplayMetrics(context);
    Map globalProps = new HashMap();
    globalProps.put("isNotchScreen", isNotchScreen());
    globalProps.put("screenWidth", displayMetrics.widthPixels / displayMetrics.density);
    globalProps.put("screenHeight", displayMetrics.heightPixels / displayMetrics.density);

    String theme = "Light";
    if ((context.getResources().getConfiguration().uiMode
        & Configuration.UI_MODE_NIGHT_MASK) == Configuration.UI_MODE_NIGHT_YES) {
      theme = "Dark";
    }
    globalProps.put("theme", theme);

    if (mFrontendTheme == "dark") {
      globalProps.put("frontendTheme", "dark");
    } else {
      globalProps.put("frontendTheme", "light");
    }

    return TemplateData.fromMap(globalProps);
  }

  public static boolean isAssetFilename(String url) {
    return url.startsWith(URL_PREFIX);
  }

  public static String getAssetFilename(String url) {
    return url.substring(URL_PREFIX.length());
  }

  public static byte[] readFileFromAssets(Context context, String fileName) {
    AssetManager assetManager = context.getAssets();
    ByteArrayOutputStream byteArrayOutputStream = null;
    InputStream inputStream = null;

    try {
      inputStream = assetManager.open(fileName);
      byteArrayOutputStream = new ByteArrayOutputStream();
      byte[] buffer = new byte[1024];
      int length;

      while ((length = inputStream.read(buffer)) != -1) {
        byteArrayOutputStream.write(buffer, 0, length);
      }

      return byteArrayOutputStream.toByteArray();
    } catch (IOException e) {
      e.printStackTrace();
      return null;
    } finally {
      try {
        if (inputStream != null) {
          inputStream.close();
        }
        if (byteArrayOutputStream != null) {
          byteArrayOutputStream.close();
        }
      } catch (IOException e) {
        e.printStackTrace();
      }
    }
  }
}
