// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import static org.junit.Assert.*;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;

import com.lynx.tasm.service.ILynxMonitorService;
import java.lang.reflect.Field;
import org.junit.Test;

public class LynxInfoReportHelperTest {
  @Test
  public void reportLynxCrashContext() {
    try {
      LynxInfoReportHelper helper = new LynxInfoReportHelper();
      // Android mokito doesn't support mocking static method
      // And importing PowerMokito is not intended, thus need to use reflection
      Field monitorService = helper.getClass().getDeclaredField("mMonitorService");
      monitorService.setAccessible(true);
      ILynxMonitorService mockMonitorService = mock(ILynxMonitorService.class);
      monitorService.set(helper, mockMonitorService);
      helper.reportLynxCrashContext("last_lynx_url", "this is a lynx url");
      verify(mockMonitorService).reportCrashGlobalContextTag("last_lynx_url", "this is a lynx url");
    } catch (NoSuchFieldException e) {
      fail();
    } catch (IllegalAccessException e) {
      fail();
    }
  }
}
