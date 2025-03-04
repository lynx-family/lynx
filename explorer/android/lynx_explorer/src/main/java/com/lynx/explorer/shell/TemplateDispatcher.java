// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.shell;

import android.content.Context;
import android.content.Intent;
import android.util.Log;
import androidx.appcompat.app.AppCompatActivity;
import com.lynx.explorer.LynxViewShellActivity;
import com.lynx.explorer.utils.QueryMapUtils;
import com.lynx.tasm.LynxLoadMeta;
import com.lynx.tasm.LynxView;
import java.util.HashMap;
import java.util.Map;

public abstract class TemplateDispatcher {
  protected TemplateLoader getLoader() {
    return new TemplateLoader() {
      @Override
      public void load(LynxView view, String url, QueryMapUtils queryMap) {
        LynxLoadMeta.Builder builder = new LynxLoadMeta.Builder();
        builder.setUrl(url);
        view.loadTemplate(builder.build());
      }

      @Override
      public Map<String, Object> parseData(String url) {
        return null;
      }
    };
  }

  abstract boolean checkUrl(String url);

  protected Intent getDispatchIntent(Context ctx, String url) {
    return new Intent(ctx, LynxViewShellActivity.class);
  }

  static {
    sDispatchers = new HashMap() {
      {
        put(HttpTemplateDispatcher.class.getSimpleName(), new HttpTemplateDispatcher());
        put(TestBenchDispatcher.class.getSimpleName(), new TestBenchDispatcher());
        put(LocalTemplateDispatcher.class.getSimpleName(), new LocalTemplateDispatcher());
      }
    };
  }

  public static final Map<String, LynxViewShellActivity> dispatchedActivities = new HashMap<>();

  private static final String TAG = "TemplateDispatcher";

  private static final Map<String, TemplateDispatcher> sDispatchers;

  public static TemplateLoader getLoader(String name) {
    TemplateDispatcher dispatcher = sDispatchers.get(name);
    if (dispatcher == null) {
      return null;
    }
    return dispatcher.getLoader();
  }

  public static void dispatchUrl(Context ctx, String url) {
    dispatchUrl(ctx, url, Intent.FLAG_ACTIVITY_NEW_TASK);
  }

  protected void pageRedirection(String url, Context ctx, int activityLaunchFlags,
      Map.Entry<String, TemplateDispatcher> entry) {
    Intent intent = getDispatchIntent(ctx, url);
    intent.addFlags(activityLaunchFlags);

    intent.putExtra(LynxViewShellActivity.URL_KEY, url);
    intent.putExtra(TemplateLoader.class.getSimpleName(), entry.getKey());
    ctx.startActivity(intent);
  }

  public static void dispatchUrl(Context ctx, String url, int activityLaunchFlags) {
    for (Map.Entry<String, TemplateDispatcher> entry : sDispatchers.entrySet()) {
      TemplateDispatcher dispatcher = entry.getValue();
      if (dispatcher.checkUrl(url)) {
        AppCompatActivity oldActivity = null;
        if (dispatchedActivities.containsKey(url)) {
          oldActivity = dispatchedActivities.get(url);
        }
        dispatcher.pageRedirection(url, ctx, activityLaunchFlags, entry);
        if (oldActivity != null) {
          oldActivity.finish();
        }
        return;
      }
    }
    Log.e(TAG, "cannot find loader for url:" + url);
  }
}
