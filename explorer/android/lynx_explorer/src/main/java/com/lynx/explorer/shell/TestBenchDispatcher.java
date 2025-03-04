// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.shell;

import android.content.Context;
import android.content.Intent;
import com.lynx.testbench.TestBenchActivity;
import com.lynx.testbench.TestBenchEnv;
import java.util.Map;

public class TestBenchDispatcher extends TemplateDispatcher {
  @Override
  protected void pageRedirection(String url, Context ctx, int activityLaunchFlags,
      Map.Entry<String, TemplateDispatcher> entry) {
    Intent intent = new Intent(ctx, TestBenchActivity.class);
    intent.addFlags(activityLaunchFlags);

    intent.putExtra(TestBenchEnv.getInstance().mUriKey, url);
    intent.putExtra(TemplateLoader.class.getSimpleName(), entry.getKey());
    ctx.startActivity(intent);
  }

  @Override
  public boolean checkUrl(String url) {
    return url.startsWith(TestBenchEnv.getInstance().mTestBenchUrlPrefix);
  }
}
