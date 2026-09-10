// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.svg

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class SVGColorResolverTest {
  @Test
  fun usesCSSColorWhenCurrentColorIsNotDeclared() {
    val resolver = SVGColorResolver()

    resolver.updateCSSColor("#112233FF")

    assertEquals("#112233FF", resolver.resolvedColor)
  }

  @Test
  fun declaredCurrentColorOverridesCSSColorUntilReset() {
    val resolver = SVGColorResolver()
    resolver.updateCSSColor("#112233FF")
    resolver.updateCurrentColor("#445566", true)

    assertFalse(resolver.updateCSSColor("#778899FF"))
    assertEquals("#445566", resolver.resolvedColor)

    assertTrue(resolver.updateCurrentColor(null, false))
    assertEquals("#778899FF", resolver.resolvedColor)
  }

  @Test
  fun emptyDeclaredCurrentColorDoesNotFallBackToCSSColor() {
    val resolver = SVGColorResolver()
    resolver.updateCSSColor("#112233FF")

    resolver.updateCurrentColor("  ", true)

    assertNull(resolver.resolvedColor)
  }

  @Test
  fun mapsARGBToCSSColor() {
    assertEquals("rgba(17,34,51,1.000000)", argbToSVGColor(0xFF112233L))
    assertEquals("rgba(255,0,0,0.501961)", argbToSVGColor(0x80FF0000L))
  }
}
