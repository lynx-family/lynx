// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.navigator;

import java.util.Map;

public class LynxRoute {
  private String routeName;
  private String templateUrl;
  private Map<String, Object> param;

  public LynxRoute(String templateUrl, Map<String, Object> param) {
    this.templateUrl = templateUrl;
    this.param = param;
    this.routeName = templateUrl;
  }

  public LynxRoute(String templateUrl, String routeName, Map<String, Object> param) {
    this.templateUrl = templateUrl;
    this.routeName = routeName;
    this.param = param;
  }

  public String getTemplateUrl() {
    return this.templateUrl;
  }

  public String getRouteName() {
    return this.routeName;
  }

  public Map<String, Object> getParam() {
    return this.param;
  }
}
