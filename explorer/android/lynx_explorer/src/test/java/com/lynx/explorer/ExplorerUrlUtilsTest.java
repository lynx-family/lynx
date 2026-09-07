// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;

import com.lynx.explorer.utils.ExplorerUrlUtils;
import com.lynx.explorer.utils.QueryMapUtils;
import org.junit.Test;

public class ExplorerUrlUtilsTest {
  @Test
  public void canonicalTestUrlKeepsNestedPathQueryAndEncoding() {
    String canonical = "sslocal://lynxtest?local://automation/foo/template.js"
        + "?width=720&standalone_url=local://automation/background/script.js"
        + "&payload=a%2Bb%3Dc";

    String normalized = ExplorerUrlUtils.normalizeLocalTestUrl(canonical);
    assertEquals("file://lynx?local://automation/foo/template.js"
            + "?width=720&standalone_url=local://automation/background/script.js"
            + "&payload=a%2Bb%3Dc",
        normalized);

    QueryMapUtils queryMap = new QueryMapUtils();
    queryMap.parse(normalized);
    assertEquals(720, queryMap.getInt("width", 0));
    assertEquals("local://automation/background/script.js", queryMap.getString("standalone_url"));
  }

  @Test
  public void existingRoutesAndNullAreUnchanged() {
    String existing = "file://lynx?local://homepage.lynx.bundle?fullscreen=true";

    assertEquals(existing, ExplorerUrlUtils.normalizeLocalTestUrl(existing));
    assertEquals("https://example.com/card.lynx.bundle",
        ExplorerUrlUtils.normalizeLocalTestUrl("https://example.com/card.lynx.bundle"));
    assertNull(ExplorerUrlUtils.normalizeLocalTestUrl(null));
  }
}
