// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.utils;

import java.util.*;

class CSSColor {
  private int r;
  private int g;
  private int b;
  private float a;

  public CSSColor() {
    this(0, 0, 0, 1.0f);
  }

  public CSSColor(int r, int g, int b) {
    this(r, g, b, 1.0f);
  }

  public CSSColor(int r, int g, int b, float a) {
    this.r = clampCssByte(r);
    this.g = clampCssByte(g);
    this.b = clampCssByte(b);
    this.a = clampCssFloat(a);
  }

  public int getR() {
    return r;
  }
  public int getG() {
    return g;
  }
  public int getB() {
    return b;
  }
  public float getA() {
    return a;
  }

  public void setR(int r) {
    this.r = clampCssByte(r);
  }
  public void setG(int g) {
    this.g = clampCssByte(g);
  }
  public void setB(int b) {
    this.b = clampCssByte(b);
  }
  public void setA(float a) {
    this.a = clampCssFloat(a);
  }

  private static int clampCssByte(double value) {
    value = Math.round(value);
    return (int) Math.max(0, Math.min(255, value));
  }

  private static float clampCssFloat(double value) {
    return (float) Math.max(0.0, Math.min(1.0, value));
  }

  private static boolean parseCssInt(String str, int[] output) {
    try {
      if (str.endsWith("%")) {
        double percentage = Double.parseDouble(str.substring(0, str.length() - 1));
        output[0] = clampCssByte(percentage / 100.0 * 255.0);
        return true;
      } else {
        int value = Integer.parseInt(str);
        output[0] = clampCssByte(value);
        return true;
      }
    } catch (NumberFormatException e) {
      return false;
    }
  }

  private static boolean parseCssFloat(String str, float[] output) {
    try {
      if (str.endsWith("%")) {
        double percentage = Double.parseDouble(str.substring(0, str.length() - 1));
        output[0] = clampCssFloat(percentage / 100.0);
        return true;
      } else {
        double value = Double.parseDouble(str);
        output[0] = clampCssFloat(value);
        return true;
      }
    } catch (NumberFormatException e) {
      return false;
    }
  }

  private static float cssHueToRgb(float m1, float m2, float h) {
    if (h < 0.0f) {
      h += 1.0f;
    } else if (h > 1.0f) {
      h -= 1.0f;
    }

    if (h * 6.0f < 1.0f) {
      return m1 + (m2 - m1) * h * 6.0f;
    }
    if (h * 2.0f < 1.0f) {
      return m2;
    }
    if (h * 3.0f < 2.0f) {
      return m1 + (m2 - m1) * (2.0f / 3.0f - h) * 6.0f;
    }
    return m1;
  }

  private static String removeWhitespace(String str) {
    if (str == null || str.isEmpty()) {
      return str;
    }

    StringBuilder sb = new StringBuilder(str.length());
    for (int i = 0; i < str.length(); i++) {
      char c = str.charAt(i);
      if (!Character.isWhitespace(c)) {
        sb.append(c);
      }
    }
    return sb.toString();
  }

  private static List<String> split(String s, char delimiter) {
    List<String> result = new ArrayList<>();
    int start = 0;
    int len = s.length();

    for (int i = 0; i < len; i++) {
      if (s.charAt(i) == delimiter) {
        result.add(s.substring(start, i));
        start = i + 1;
      }
    }
    result.add(s.substring(start));

    return result;
  }

  @Override
  public String toString() {
    return String.format("CSSColor{r=%d, g=%d, b=%d, a=%.2f}", r, g, b, a);
  }

  @Override
  public boolean equals(Object obj) {
    if (this == obj)
      return true;
    if (obj == null || getClass() != obj.getClass())
      return false;
    CSSColor cssColor = (CSSColor) obj;
    return r == cssColor.r && g == cssColor.g && b == cssColor.b
        && Float.compare(cssColor.a, a) == 0;
  }

  @Override
  public int hashCode() {
    return Objects.hash(r, g, b, a);
  }

  public static CSSColor parse(String colorStr) {
    CSSColor[] result = new CSSColor[1];
    if (parse(colorStr, result)) {
      return result[0];
    }
    return null;
  }

  public static boolean parse(String colorStr, CSSColor[] color) {
    if (colorStr == null || colorStr.isEmpty()) {
      return false;
    }

    String str = colorStr;
    str = removeWhitespace(str);
    str = str.toLowerCase();
    if (str.startsWith("#")) {
      return parseHexColor(str, color);
    }
    int openParen = str.indexOf('(');
    int closeParen = str.lastIndexOf(')');
    if (openParen != -1 && closeParen == str.length() - 1) {
      String funcName = str.substring(0, openParen);
      String params = str.substring(openParen + 1, closeParen);
      List<String> paramList = split(params, ',');

      return parseFunctionColor(funcName, paramList, color);
    }

    return parseNamedColor(str, color);
  }

  private static boolean parseHexColor(String str, CSSColor[] color) {
    try {
      if (str.length() == 4) { // #abc
        long value = Long.parseLong(str.substring(1), 16);
        if (value < 0 || value > 0xfff)
          return false;

        color[0] = new CSSColor((int) (((value & 0xf00) >> 4) | ((value & 0xf00) >> 8)),
            (int) ((value & 0xf0) | ((value & 0xf0) >> 4)),
            (int) ((value & 0xf) | ((value & 0xf) << 4)), 1.0f);
        return true;

      } else if (str.length() == 5) { // #abcd
        long value = Long.parseLong(str.substring(1), 16);
        if (value < 0 || value > 0xffff)
          return false;

        color[0] = new CSSColor((int) (((value & 0xf000) >> 8) | ((value & 0xf000) >> 12)),
            (int) (((value & 0xf00) >> 4) | ((value & 0xf00) >> 8)),
            (int) ((value & 0xf0) | ((value & 0xf0) >> 4)),
            ((value & 0xf) | ((value & 0xf) << 4)) / 255.0f);
        return true;

      } else if (str.length() == 7) { // #abcdef
        long value = Long.parseLong(str.substring(1), 16);
        if (value < 0 || value > 0xffffff)
          return false;

        color[0] = new CSSColor((int) ((value & 0xff0000) >> 16), (int) ((value & 0xff00) >> 8),
            (int) (value & 0xff), 1.0f);
        return true;

      } else if (str.length() == 9) { // #abcdef12
        long value = Long.parseLong(str.substring(1), 16);
        if (value < 0 || value > 0xffffffffL)
          return false;

        color[0] =
            new CSSColor((int) ((value & 0xff000000L) >> 24), (int) ((value & 0xff0000) >> 16),
                (int) ((value & 0xff00) >> 8), ((int) (value & 0xff)) / 255.0f);
        return true;
      }
    } catch (NumberFormatException e) {
      return false;
    }

    return false;
  }

  private static boolean parseFunctionColor(
      String funcName, List<String> params, CSSColor[] color) {
    float alpha = 1.0f;

    if ("rgba".equals(funcName) || "rgb".equals(funcName)) {
      if ("rgba".equals(funcName)) {
        if (params.size() != 4)
          return false;
        float[] alphaArray = new float[1];
        if (!parseCssFloat(params.get(3), alphaArray))
          return false;
        alpha = alphaArray[0];
      } else {
        if (params.size() != 3)
          return false;
      }

      int[] r = new int[1], g = new int[1], b = new int[1];
      if (!parseCssInt(params.get(0), r) || !parseCssInt(params.get(1), g)
          || !parseCssInt(params.get(2), b)) {
        return false;
      }

      color[0] = new CSSColor(r[0], g[0], b[0], alpha);
      return true;

    } else if ("hsla".equals(funcName) || "hsl".equals(funcName)) {
      if ("hsla".equals(funcName)) {
        if (params.size() != 4)
          return false;
        float[] alphaArray = new float[1];
        if (!parseCssFloat(params.get(3), alphaArray))
          return false;
        alpha = alphaArray[0];
      } else {
        if (params.size() != 3)
          return false;
      }

      try {
        double h = Double.parseDouble(params.get(0));
        float[] s = new float[1], l = new float[1];
        if (!parseCssFloat(params.get(1), s) || !parseCssFloat(params.get(2), l)) {
          return false;
        }

        color[0] = createFromHSLA((float) h, s[0] * 100, l[0] * 100, alpha);
        return true;
      } catch (NumberFormatException e) {
        return false;
      }
    }

    return false;
  }

  public static CSSColor createFromHSLA(float h, float s, float l, float a) {
    h /= 360.0f;
    s /= 100.0f;
    l /= 100.0f;

    while (h < 0.0f) h++;
    while (h > 1.0f) h--;

    float m2 = l <= 0.5f ? l * (s + 1.0f) : l + s - l * s;
    float m1 = l * 2.0f - m2;

    return new CSSColor(clampCssByte(cssHueToRgb(m1, m2, h + 1.0f / 3.0f) * 255.0f),
        clampCssByte(cssHueToRgb(m1, m2, h) * 255.0f),
        clampCssByte(cssHueToRgb(m1, m2, h - 1.0f / 3.0f) * 255.0f), clampCssFloat(a));
  }

  public static CSSColor createFromRGBA(float r, float g, float b, float a) {
    return new CSSColor(clampCssByte(r), clampCssByte(g), clampCssByte(b), clampCssFloat(a));
  }

  private static boolean parseNamedColor(String colorStr, CSSColor[] color) {
    CSSColor namedColor = getNamedColor(colorStr);
    if (namedColor != null) {
      color[0] = namedColor;
      return true;
    }
    return false;
  }

  private static final CSSColor[] NAMED_COLORS = {
      // [TRANSPARENT, ..., YELLOWGREEN]
      new CSSColor(0, 0, 0, 0), // transparent
      new CSSColor(240, 248, 255, 1), // aliceblue
      new CSSColor(250, 235, 215, 1), // antiquewhite
      new CSSColor(0, 255, 255, 1), // aqua
      new CSSColor(127, 255, 212, 1), // aquamarine
      new CSSColor(240, 255, 255, 1), // azure
      new CSSColor(245, 245, 220, 1), // beige
      new CSSColor(255, 228, 196, 1), // bisque
      new CSSColor(0, 0, 0, 1), // black
      new CSSColor(255, 235, 205, 1), // blanchedalmond
      new CSSColor(0, 0, 255, 1), // blue
      new CSSColor(138, 43, 226, 1), // blueviolet
      new CSSColor(165, 42, 42, 1), // brown
      new CSSColor(222, 184, 135, 1), // burlywood
      new CSSColor(95, 158, 160, 1), // cadetblue
      new CSSColor(127, 255, 0, 1), // chartreuse
      new CSSColor(210, 105, 30, 1), // chocolate
      new CSSColor(255, 127, 80, 1), // coral
      new CSSColor(100, 149, 237, 1), // cornflowerblue
      new CSSColor(255, 248, 220, 1), // cornsilk
      new CSSColor(220, 20, 60, 1), // crimson
      new CSSColor(0, 255, 255, 1), // cyan
      new CSSColor(0, 0, 139, 1), // darkblue
      new CSSColor(0, 139, 139, 1), // darkcyan
      new CSSColor(184, 134, 11, 1), // darkgoldenrod
      new CSSColor(169, 169, 169, 1), // darkgray
      new CSSColor(0, 100, 0, 1), // darkgreen
      new CSSColor(169, 169, 169, 1), // darkgrey
      new CSSColor(189, 183, 107, 1), // darkkhaki
      new CSSColor(139, 0, 139, 1), // darkmagenta
      new CSSColor(85, 107, 47, 1), // darkolivegreen
      new CSSColor(255, 140, 0, 1), // darkorange
      new CSSColor(153, 50, 204, 1), // darkorchid
      new CSSColor(139, 0, 0, 1), // darkred
      new CSSColor(233, 150, 122, 1), // darksalmon
      new CSSColor(143, 188, 143, 1), // darkseagreen
      new CSSColor(72, 61, 139, 1), // darkslateblue
      new CSSColor(47, 79, 79, 1), // darkslategray
      new CSSColor(47, 79, 79, 1), // darkslategrey
      new CSSColor(0, 206, 209, 1), // darkturquoise
      new CSSColor(148, 0, 211, 1), // darkviolet
      new CSSColor(255, 20, 147, 1), // deeppink
      new CSSColor(0, 191, 255, 1), // deepskyblue
      new CSSColor(105, 105, 105, 1), // dimgray
      new CSSColor(105, 105, 105, 1), // dimgrey
      new CSSColor(30, 144, 255, 1), // dodgerblue
      new CSSColor(178, 34, 34, 1), // firebrick
      new CSSColor(255, 250, 240, 1), // floralwhite
      new CSSColor(34, 139, 34, 1), // forestgreen
      new CSSColor(255, 0, 255, 1), // fuchsia
      new CSSColor(220, 220, 220, 1), // gainsboro
      new CSSColor(248, 248, 255, 1), // ghostwhite
      new CSSColor(255, 215, 0, 1), // gold
      new CSSColor(218, 165, 32, 1), // goldenrod
      new CSSColor(128, 128, 128, 1), // gray
      new CSSColor(0, 128, 0, 1), // green
      new CSSColor(173, 255, 47, 1), // greenyellow
      new CSSColor(128, 128, 128, 1), // grey
      new CSSColor(240, 255, 240, 1), // honeydew
      new CSSColor(255, 105, 180, 1), // hotpink
      new CSSColor(205, 92, 92, 1), // indianred
      new CSSColor(75, 0, 130, 1), // indigo
      new CSSColor(255, 255, 240, 1), // ivory
      new CSSColor(240, 230, 140, 1), // khaki
      new CSSColor(230, 230, 250, 1), // lavender
      new CSSColor(255, 240, 245, 1), // lavenderblush
      new CSSColor(124, 252, 0, 1), // lawngreen
      new CSSColor(255, 250, 205, 1), // lemonchiffon
      new CSSColor(173, 216, 230, 1), // lightblue
      new CSSColor(240, 128, 128, 1), // lightcoral
      new CSSColor(224, 255, 255, 1), // lightcyan
      new CSSColor(250, 250, 210, 1), // lightgoldenrodyellow
      new CSSColor(211, 211, 211, 1), // lightgray
      new CSSColor(144, 238, 144, 1), // lightgreen
      new CSSColor(211, 211, 211, 1), // lightgrey
      new CSSColor(255, 182, 193, 1), // lightpink
      new CSSColor(255, 160, 122, 1), // lightsalmon
      new CSSColor(32, 178, 170, 1), // lightseagreen
      new CSSColor(135, 206, 250, 1), // lightskyblue
      new CSSColor(119, 136, 153, 1), // lightslategray
      new CSSColor(119, 136, 153, 1), // lightslategrey
      new CSSColor(176, 196, 222, 1), // lightsteelblue
      new CSSColor(255, 255, 224, 1), // lightyellow
      new CSSColor(0, 255, 0, 1), // lime
      new CSSColor(50, 205, 50, 1), // limegreen
      new CSSColor(250, 240, 230, 1), // linen
      new CSSColor(255, 0, 255, 1), // magenta
      new CSSColor(128, 0, 0, 1), // maroon
      new CSSColor(102, 205, 170, 1), // mediumaquamarine
      new CSSColor(0, 0, 205, 1), // mediumblue
      new CSSColor(186, 85, 211, 1), // mediumorchid
      new CSSColor(147, 112, 219, 1), // mediumpurple
      new CSSColor(60, 179, 113, 1), // mediumseagreen
      new CSSColor(123, 104, 238, 1), // mediumslateblue
      new CSSColor(0, 250, 154, 1), // mediumspringgreen
      new CSSColor(72, 209, 204, 1), // mediumturquoise
      new CSSColor(199, 21, 133, 1), // mediumvioletred
      new CSSColor(25, 25, 112, 1), // midnightblue
      new CSSColor(245, 255, 250, 1), // mintcream
      new CSSColor(255, 228, 225, 1), // mistyrose
      new CSSColor(255, 228, 181, 1), // moccasin
      new CSSColor(255, 222, 173, 1), // navajowhite
      new CSSColor(0, 0, 128, 1), // navy
      new CSSColor(253, 245, 230, 1), // oldlace
      new CSSColor(128, 128, 0, 1), // olive
      new CSSColor(107, 142, 35, 1), // olivedrab
      new CSSColor(255, 165, 0, 1), // orange
      new CSSColor(255, 69, 0, 1), // orangered
      new CSSColor(218, 112, 214, 1), // orchid
      new CSSColor(238, 232, 170, 1), // palegoldenrod
      new CSSColor(152, 251, 152, 1), // palegreen
      new CSSColor(175, 238, 238, 1), // paleturquoise
      new CSSColor(219, 112, 147, 1), // palevioletred
      new CSSColor(255, 239, 213, 1), // papayawhip
      new CSSColor(255, 218, 185, 1), // peachpuff
      new CSSColor(205, 133, 63, 1), // peru
      new CSSColor(255, 192, 203, 1), // pink
      new CSSColor(221, 160, 221, 1), // plum
      new CSSColor(176, 224, 230, 1), // powderblue
      new CSSColor(128, 0, 128, 1), // purple
      new CSSColor(255, 0, 0, 1), // red
      new CSSColor(188, 143, 143, 1), // rosybrown
      new CSSColor(65, 105, 225, 1), // royalblue
      new CSSColor(139, 69, 19, 1), // saddlebrown
      new CSSColor(250, 128, 114, 1), // salmon
      new CSSColor(244, 164, 96, 1), // sandybrown
      new CSSColor(46, 139, 87, 1), // seagreen
      new CSSColor(255, 245, 238, 1), // seashell
      new CSSColor(160, 82, 45, 1), // sienna
      new CSSColor(192, 192, 192, 1), // silver
      new CSSColor(135, 206, 235, 1), // skyblue
      new CSSColor(106, 90, 205, 1), // slateblue
      new CSSColor(112, 128, 144, 1), // slategray
      new CSSColor(112, 128, 144, 1), // slategrey
      new CSSColor(255, 250, 250, 1), // snow
      new CSSColor(0, 255, 127, 1), // springgreen
      new CSSColor(70, 130, 180, 1), // steelblue
      new CSSColor(210, 180, 140, 1), // tan
      new CSSColor(0, 128, 128, 1), // teal
      new CSSColor(216, 191, 216, 1), // thistle
      new CSSColor(255, 99, 71, 1), // tomato
      new CSSColor(64, 224, 208, 1), // turquoise
      new CSSColor(238, 130, 238, 1), // violet
      new CSSColor(245, 222, 179, 1), // wheat
      new CSSColor(255, 255, 255, 1), // white
      new CSSColor(245, 245, 245, 1), // whitesmoke
      new CSSColor(255, 255, 0, 1), // yellow
      new CSSColor(154, 205, 50, 1) // yellowgreen
  };

  private static final Map<String, Integer> COLOR_NAME_MAP = new HashMap<>();

  static {
    String[] colorNames = {"transparent", "aliceblue", "antiquewhite", "aqua", "aquamarine",
        "azure", "beige", "bisque", "black", "blanchedalmond", "blue", "blueviolet", "brown",
        "burlywood", "cadetblue", "chartreuse", "chocolate", "coral", "cornflowerblue", "cornsilk",
        "crimson", "cyan", "darkblue", "darkcyan", "darkgoldenrod", "darkgray", "darkgreen",
        "darkgrey", "darkkhaki", "darkmagenta", "darkolivegreen", "darkorange", "darkorchid",
        "darkred", "darksalmon", "darkseagreen", "darkslateblue", "darkslategray", "darkslategrey",
        "darkturquoise", "darkviolet", "deeppink", "deepskyblue", "dimgray", "dimgrey",
        "dodgerblue", "firebrick", "floralwhite", "forestgreen", "fuchsia", "gainsboro",
        "ghostwhite", "gold", "goldenrod", "gray", "green", "greenyellow", "grey", "honeydew",
        "hotpink", "indianred", "indigo", "ivory", "khaki", "lavender", "lavenderblush",
        "lawngreen", "lemonchiffon", "lightblue", "lightcoral", "lightcyan", "lightgoldenrodyellow",
        "lightgray", "lightgreen", "lightgrey", "lightpink", "lightsalmon", "lightseagreen",
        "lightskyblue", "lightslategray", "lightslategrey", "lightsteelblue", "lightyellow", "lime",
        "limegreen", "linen", "magenta", "maroon", "mediumaquamarine", "mediumblue", "mediumorchid",
        "mediumpurple", "mediumseagreen", "mediumslateblue", "mediumspringgreen", "mediumturquoise",
        "mediumvioletred", "midnightblue", "mintcream", "mistyrose", "moccasin", "navajowhite",
        "navy", "oldlace", "olive", "olivedrab", "orange", "orangered", "orchid", "palegoldenrod",
        "palegreen", "paleturquoise", "palevioletred", "papayawhip", "peachpuff", "peru", "pink",
        "plum", "powderblue", "purple", "red", "rosybrown", "royalblue", "saddlebrown", "salmon",
        "sandybrown", "seagreen", "seashell", "sienna", "silver", "skyblue", "slateblue",
        "slategray", "slategrey", "snow", "springgreen", "steelblue", "tan", "teal", "thistle",
        "tomato", "turquoise", "violet", "wheat", "white", "whitesmoke", "yellow", "yellowgreen"};

    for (int i = 0; i < colorNames.length; i++) {
      COLOR_NAME_MAP.put(colorNames[i], i);
    }
  }

  public static CSSColor getNamedColor(String colorName) {
    Integer index = COLOR_NAME_MAP.get(colorName.toLowerCase());
    if (index != null && index >= 0 && index < NAMED_COLORS.length) {
      CSSColor color = NAMED_COLORS[index];
      return new CSSColor(color.r, color.g, color.b, color.a);
    }
    return null;
  }

  public static CSSColor createFromKeyword(String keyword) {
    return getNamedColor(keyword);
  }

  public static boolean isColorIdentifier(String identifier) {
    if (identifier == null) {
      return false;
    }
    return COLOR_NAME_MAP.containsKey(identifier.toLowerCase());
  }

  public int toInt() {
    int alpha = (int) (a * 255) & 0xFF;
    return (alpha << 24) | (r << 16) | (g << 8) | b;
  }

  public int cast() {
    return toInt();
  }
}
