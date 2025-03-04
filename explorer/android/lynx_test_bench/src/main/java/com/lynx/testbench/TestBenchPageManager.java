// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.testbench;

import android.content.Context;
import android.content.Intent;
import android.os.Looper;
import com.google.gson.Gson;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.provider.AbsTemplateProvider;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;
import org.json.JSONException;
import org.json.JSONObject;

public class TestBenchPageManager {
  private static final String TAG = "TestBenchPageManager";
  private static TestBenchPageManager instance;
  private AtomicInteger count = new AtomicInteger(0);
  private JSONObject mPages;
  private JSONObject mRouters;
  private JSONObject mGroups;
  private JSONObject mActivities;
  private ArrayList<String> mPageStack;
  private int mActivityLaunchFlags;
  private ArrayList<TestBenchActionCallback> mCallbacks;

  private boolean mIsMultiEnv;

  private int incrementAndGet() {
    return count.incrementAndGet();
  }

  public ArrayList<TestBenchActionCallback> getCallbacks() {
    return mCallbacks;
  }

  public void registerCallback(TestBenchActionCallback callback) {
    mCallbacks.add(callback);
  }

  private String getRawPageName(String pageName) {
    return pageName.split("#")[0];
  }

  private String buildPageName(String rawPageName) {
    return rawPageName + "#" + String.valueOf(incrementAndGet());
  }

  private TestBenchPageManager() {
    mCallbacks = new ArrayList<>();
  }

  private void clear() {
    mIsMultiEnv = false;
    mPages = new JSONObject();
    mRouters = new JSONObject();
    mActivities = new JSONObject();
    mGroups = new JSONObject();
    mPageStack = new ArrayList<>();
  }

  public static synchronized TestBenchPageManager getInstance() {
    if (instance == null) {
      instance = new TestBenchPageManager();
      instance.clear();
    }
    return instance;
  }

  private Intent getTestBenchIntent(Context ctx, String url) {
    Intent intent = new Intent(ctx, TestBenchActivity.class);
    intent.addFlags(mActivityLaunchFlags);
    intent.putExtra(TestBenchEnv.getInstance().mUriKey, url);
    return intent;
  }

  void replaySignalPage(String url, Context ctx) {
    clear();
    mIsMultiEnv = false;
    Intent intent = getTestBenchIntent(ctx, url);
    ctx.startActivity(intent);
  }

  void replayMultiPagesInOneActivity(String url, Context ctx) {
    clear();
    mIsMultiEnv = true;
    Intent intent = new Intent(ctx, TestBenchMultiPagesActivity.class);
    intent.addFlags(mActivityLaunchFlags);
    intent.putExtra(TestBenchEnv.getInstance().mUriKey, url);
    ctx.startActivity(intent);
  }

  void replayMultiPages(String url, Context ctx) {
    clear();
    mIsMultiEnv = true;
    loadDescribeFile(url, ctx);
  }

  public void initLynxGroup(String groupName, TestBenchActivity currActivity) {
    if (!mIsMultiEnv) {
      return;
    }
    TestBenchActivity activity = (TestBenchActivity) mGroups.opt(groupName);
    if (activity == null) {
      try {
        mGroups.put(groupName, currActivity);
      } catch (JSONException e) {
        e.printStackTrace();
      }
    } else {
      currActivity.setLynxGroup(activity.getLynxGroup());
    }
  }

  public void registerActivity(String pageName, TestBenchActivity currActivity) {
    if (!mIsMultiEnv) {
      return;
    }
    try {
      mActivities.put(pageName, currActivity);
    } catch (JSONException e) {
      e.printStackTrace();
    }
  }

  public void onActivityDestroy(String pageName, boolean hasBeenRemoveFromPageStack) {
    if (!mIsMultiEnv) {
      return;
    }
    if (!hasBeenRemoveFromPageStack) {
      mPageStack.remove(mPageStack.size() - 1);
    }
    mActivities.remove(pageName);
  }

  public void replayPageFromOpenSchema(ReadableMap params) {
    if (!mIsMultiEnv) {
      return;
    }
    String currPageName = mPageStack.get(mPageStack.size() - 1);
    String currRawPageName = getRawPageName(currPageName);
    String label = params.getString("label");
    JSONObject nextPageInfo = mRouters.optJSONObject(currRawPageName).optJSONObject(label);
    boolean popLast = nextPageInfo.optBoolean("popLast");
    String nextPage = nextPageInfo.optString("next");
    if (popLast) {
      mPageStack.remove(mPageStack.size() - 1);
    }
    mPageStack.add(buildPageName(nextPage));
    replayCurrPage((TestBenchActivity) mActivities.opt(currPageName), popLast);
  }

  void replayCurrPage(Context ctx, boolean popLast) {
    String pageName = mPageStack.get(mPageStack.size() - 1);
    String rawPageName = getRawPageName(pageName);
    JSONObject pageInfo = mPages.optJSONObject(rawPageName);
    if (pageInfo == null) {
      return;
    }

    Intent intent = getTestBenchIntent(ctx, pageInfo.optString("url"));
    intent.putExtra("groupName", pageInfo.optString("group"));
    intent.putExtra("pageName", pageName);

    ctx.startActivity(intent);
    if (popLast) {
      ((TestBenchActivity) ctx).setState(true);
      ((TestBenchActivity) ctx).finish();
    }
  }

  void loadDescribeFile(String rawUrl, Context ctx) {
    QueryMapUtils queryMap = new QueryMapUtils();
    queryMap.parse(rawUrl);
    String url = queryMap.getString("url");
    LynxEnv.inst().getTemplateProvider().loadTemplate(url, new AbsTemplateProvider.Callback() {
      @Override
      public void onSuccess(byte[] template) {
        if (!Thread.currentThread().equals(Looper.getMainLooper().getThread())) {
          throw new IllegalThreadStateException("Callback must be fired on main thread.");
        };
        Gson gson = new Gson();
        String str = new String(template, Charset.forName("UTF-8"));
        Map map = gson.fromJson(str, Map.class);
        JSONObject describeFile = new JSONObject(map);
        mPages = describeFile.optJSONObject("pages");
        mRouters = describeFile.optJSONObject("routers");
        mPageStack.add(buildPageName(describeFile.optString("root")));
        replayCurrPage(ctx, false);
      }

      @Override
      public void onFailed(String msg) {
        return;
      }
    });
  }

  private boolean isDescribeFile(String url) {
    QueryMapUtils queryMap = new QueryMapUtils();
    queryMap.parse(url);
    if (queryMap.getString("describe_file") != null
        && queryMap.getBoolean("describe_file", false)) {
      return true;
    }
    return false;
  }

  private boolean isMultiPagesFile(String url) {
    QueryMapUtils queryMap = new QueryMapUtils();
    queryMap.parse(url);
    if (queryMap.getString("multi-page") != null && queryMap.getBoolean("multi-page", false)) {
      return true;
    }
    return false;
  }

  public void startReplay(String url, Context ctx, int activityLaunchFlags) {
    mActivityLaunchFlags = activityLaunchFlags;
    if (isDescribeFile(url)) {
      replayMultiPages(url, ctx);
    } else if (isMultiPagesFile(url)) {
      replayMultiPagesInOneActivity(url, ctx);
    } else {
      replaySignalPage(url, ctx);
    }
  }
}
