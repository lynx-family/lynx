// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.utils;

public class UnicodeFontUtils {
  public static final int DECODE_DEFAULT = 0;
  public static final int DECODE_INSERT_ZERO_WIDTH_CHAR = 1;
  public static final int DECODE_CJK_INSERT_WORD_JOINER = 2;
  /**
   * DECODE_INSERT_ZERO_WIDTH_CHAR: insert u+200b after all character.
   * DECODE_CJK_INSERT_WORD_JOINER: insert u+2060 between cjk character.
   * */
  public static String decodeCSSContent(String unicodeStr, int decodeProperty) {
    if (unicodeStr == null) {
      return null;
    }
    int length = unicodeStr.length();
    StringBuilder stringBuffer = new StringBuilder(length);
    InsertCharContext insert = null;
    if (decodeProperty != DECODE_DEFAULT) {
      insert = new InsertCharContext();
    }
    for (int i = 0; i < length; i++) {
      // add css rule hexadecimal: \e960
      if (unicodeStr.charAt(i) == '\\' && i + 1 < length) {
        StringBuilder unicode = new StringBuilder();
        for (int j = i + 1; j < length && j < i + 5; j++) {
          char c = unicodeStr.charAt(j);
          if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
            unicode.append(c);
          } else {
            break;
          }
        }

        try {
          char code = (char) Integer.parseInt(unicode.toString(), 16);
          stringBuffer.append(code);
          i = i + unicode.length();
        } catch (Exception e) {
          stringBuffer.append(unicodeStr.charAt(i));
        }
      } else {
        stringBuffer.append(unicodeStr.charAt(i));
      }
      if (decodeProperty != DECODE_DEFAULT) {
        insert.InsertExtraChar(stringBuffer, decodeProperty);
      }
    }
    return stringBuffer.toString();
  }

  public static String decode(String unicodeStr, int decodeProperty) {
    if (unicodeStr == null) {
      return null;
    }
    int length = unicodeStr.length();
    StringBuilder stringBuffer = new StringBuilder(length);
    InsertCharContext insert = null;
    if (decodeProperty != DECODE_DEFAULT) {
      insert = new InsertCharContext();
    }
    for (int i = 0; i < length; i++) {
      // example:  &#xe61c;&#xe61d;&#xe61e;
      //           &#x4e2d;&#x56fd;
      //           &#20013;&#22269;
      if (unicodeStr.charAt(i) == '&' && i + 1 < length && unicodeStr.charAt(i + 1) == '#') {
        int end = -1;
        // utf32 char max value is FFFFFFFF/4294967295, max length is 10
        for (int j = i + 2; j < length && j < i + 2 + 10 + 1; j++) {
          if (unicodeStr.charAt(j) == ';') {
            end = j;
            break;
          }
        }
        if (end == -1) {
          stringBuffer.append(unicodeStr.charAt(i));
        } else {
          // find ;
          try {
            int codepoint;
            if (unicodeStr.charAt(i + 2) == 'x') {
              // hexadecimal
              codepoint = Integer.parseInt(unicodeStr.subSequence(i + 3, end).toString(), 16);
            } else {
              // decimal
              codepoint = Integer.parseInt(unicodeStr.subSequence(i + 2, end).toString(), 10);
            }
            stringBuffer.appendCodePoint(codepoint);
            i = end;
          } catch (Exception e) {
            stringBuffer.append(unicodeStr.charAt(i));
          }
        }
      } else if (unicodeStr.charAt(i) == '&' && i + 1 < length) {
        int end = -1;
        // &thinsp; has the max length 8, so we only check ; within 8 characters
        for (int j = i + 1; j < length && j < i + 7 + 1; j++) {
          char c = unicodeStr.charAt(j);
          if (c == ';') {
            end = j;
            break;
          }
          if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))) {
            break;
          }
        }
        if (end == -1) {
          stringBuffer.append(unicodeStr.charAt(i));
        } else {
          char c;
          int decimal = decodeEntity(unicodeStr.subSequence(i + 1, end).toString());
          if (decimal > 0) {
            c = (char) decimal;
            stringBuffer.append(c);
            i = end;
          }
        }
      } else {
        stringBuffer.append(unicodeStr.charAt(i));
      }
      if (decodeProperty != DECODE_DEFAULT) {
        insert.InsertExtraChar(stringBuffer, decodeProperty);
      }
    }
    return stringBuffer.toString();
  }

  // Reference: https://www.w3schools.com/charsets/ref_utf_punctuation.asp
  // Entity      Dec
  // &nbsp;      160
  // &ensp;      8194
  // &emsp;      8195
  // &thinsp;    8201
  // &zwnj;      8204
  // &zwj;       8205
  // &lrm;       8206
  // &rlm;       8207
  // &ndash;     8211
  // &lsquo;     8216
  // &rsquo;     8217
  // &sbquo;     8218
  // &ldquo;     8220
  // &rdquo;     8221
  // &bdquo;     8222
  // &dagger;    8224
  // &Dagger;    8225
  // &bull;      8226
  // &hellip;    8230
  // &permil;    8240
  // &prime;     8242
  // &Prime;     8243
  // &lsaquo;    8249
  // &rsaquo;    8250
  // &oline;     8254
  // &frasl;     8260
  // &lt;        60
  // &gt;        62
  // if the nums of entity become large , we should optimize the decode performance like
  // https://source.chromium.org/chromium/chromium/src/+/master:out/chromeos-Debug/gen/third_party/blink/renderer/core/html_entity_table.cc;l=4336;drc=1562cab3f1eda927938f8f4a5a91991fefde66d3
  private static int decodeEntity(String entity) {
    // TODO(linxs): move the decode to C++ side
    switch (entity) {
      case "amp":
        return 38;
      case "nbsp":
        return 160;
      case "ensp":
        return 8194;
      case "emsp":
        return 8195;
      case "thinsp":
        return 8201;
      case "zwnj":
        return 8204;
      case "zwj":
        return 8205;
      case "lrm":
        return 8206;
      case "rlm":
        return 8207;
      case "ndash":
        return 8211;
      case "lsquo":
        return 8216;
      case "rsquo":
        return 8217;
      case "sbquo":
        return 8218;
      case "ldquo":
        return 8220;
      case "rdquo":
        return 8221;
      case "bdquo":
        return 8222;
      case "dagger":
        return 8224;
      case "Dagger":
        return 8225;
      case "bull":
        return 8226;
      case "hellip":
        return 8230;
      case "permil":
        return 8240;
      case "prime":
        return 8242;
      case "Prime":
        return 8243;
      case "lsaquo":
        return 8249;
      case "rsaquo":
        return 8250;
      case "oline":
        return 8254;
      case "frasl":
        return 8260;
      case "lt":
        return 60;
      case "gt":
        return 62;
      case "middot":
        return 183;
      case "mldr":
        return 8230;
      case "cacute":
        return 263;
      case "quot":
        return 34;
      case "amacr":
        return 257;
      case "caron":
        return 711;
      case "emacr":
        return 275;
      case "mdash":
        return 8212;
      case "copy":
        return 169;
      case "times":
        return 215;
      case "darr":
        return 8595;
      case "imacr":
        return 299;
      case "iacute":
        return 237;
      case "igrave":
        return 236;
      case "agrave":
        return 224;
      case "ge":
        return 8805;
      case "le":
        return 8804;
    }
    return -1;
  }

  private static class InsertCharContext {
    private boolean mCjkBefore = false;
    private boolean mBreakCharBefore = false;
    private boolean mHighSurrogateBefore = false;
    public void InsertExtraChar(StringBuilder stringBuffer, int decodeProperty) {
      if (stringBuffer.length() == 0) {
        return;
      }

      int lastCharIndex = stringBuffer.length() - 1;
      char lastChar = stringBuffer.charAt(lastCharIndex);

      if (Character.isHighSurrogate(lastChar)) {
        mHighSurrogateBefore = true;
        return;
      }

      int charStart = stringBuffer.length() - (mHighSurrogateBefore ? 2 : 1);
      int codePoint = stringBuffer.codePointAt(charStart);
      if (decodeProperty == DECODE_INSERT_ZERO_WIDTH_CHAR) {
        if (isLatinOrSymbol(codePoint)) {
          if (mBreakCharBefore) {
            //&#8203; is a zero-width space character, and the corresponding Unicode encoding is
            // U+200B.
            // It can be used to adjust the alignment and formatting of text.
            stringBuffer.delete(charStart, stringBuffer.length());
            stringBuffer.append((char) 8203);
            stringBuffer.appendCodePoint(codePoint);
          } else {
            mBreakCharBefore = true;
          }
        } else {
          mBreakCharBefore = false;
        }
      } else if (decodeProperty == DECODE_CJK_INSERT_WORD_JOINER) {
        if (isCJK(codePoint)) {
          if (mCjkBefore) {
            // u+2060: word joiner, avoid line break
            stringBuffer.delete(charStart, stringBuffer.length());
            stringBuffer.append('\u2060');
            stringBuffer.appendCodePoint(codePoint);
          } else {
            mCjkBefore = true;
          }
        } else {
          mCjkBefore = false;
        }
      }
      mHighSurrogateBefore = false;
    }

    public static boolean isLatinOrSymbol(int codePoint) {
      Character.UnicodeBlock block = Character.UnicodeBlock.of(codePoint);
      return block == Character.UnicodeBlock.BASIC_LATIN
          || block == Character.UnicodeBlock.LATIN_1_SUPPLEMENT
          || block == Character.UnicodeBlock.LATIN_EXTENDED_A
          || block == Character.UnicodeBlock.LATIN_EXTENDED_B
          || block == Character.UnicodeBlock.GENERAL_PUNCTUATION
          || block == Character.UnicodeBlock.CURRENCY_SYMBOLS
          || block == Character.UnicodeBlock.MATHEMATICAL_OPERATORS
          || block == Character.UnicodeBlock.MISCELLANEOUS_SYMBOLS
          || block == Character.UnicodeBlock.DINGBATS;
    }

    private static boolean isCJK(int codepoint) {
      Character.UnicodeBlock block = Character.UnicodeBlock.of(codepoint);
      return block == Character.UnicodeBlock.CJK_UNIFIED_IDEOGRAPHS
          || block == Character.UnicodeBlock.CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A
          || block == Character.UnicodeBlock.CJK_UNIFIED_IDEOGRAPHS_EXTENSION_B
          || block == Character.UnicodeBlock.CJK_RADICALS_SUPPLEMENT
          || block == Character.UnicodeBlock.CJK_COMPATIBILITY_IDEOGRAPHS
          || block == Character.UnicodeBlock.KATAKANA || block == Character.UnicodeBlock.HIRAGANA
          || block == Character.UnicodeBlock.KATAKANA_PHONETIC_EXTENSIONS
          || block == Character.UnicodeBlock.HANGUL_JAMO
          || block == Character.UnicodeBlock.HANGUL_SYLLABLES;
    }
  }
}
