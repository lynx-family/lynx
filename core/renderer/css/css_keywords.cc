/* C++ code produced by gperf version 3.3 */
/* Command-line: gperf -D -t --output-file css_keywords.cc css_keywords.tmpl  */
/* Computed positions: -k'1-4,6-8,12-13,$' */

#if !(                                                                         \
    (' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) && ('%' == 37) && \
    ('&' == 38) && ('\'' == 39) && ('(' == 40) && (')' == 41) &&               \
    ('*' == 42) && ('+' == 43) && (',' == 44) && ('-' == 45) && ('.' == 46) && \
    ('/' == 47) && ('0' == 48) && ('1' == 49) && ('2' == 50) && ('3' == 51) && \
    ('4' == 52) && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) && \
    ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) && ('=' == 61) && \
    ('>' == 62) && ('?' == 63) && ('A' == 65) && ('B' == 66) && ('C' == 67) && \
    ('D' == 68) && ('E' == 69) && ('F' == 70) && ('G' == 71) && ('H' == 72) && \
    ('I' == 73) && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) && \
    ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) && ('R' == 82) && \
    ('S' == 83) && ('T' == 84) && ('U' == 85) && ('V' == 86) && ('W' == 87) && \
    ('X' == 88) && ('Y' == 89) && ('Z' == 90) && ('[' == 91) &&                \
    ('\\' == 92) && (']' == 93) && ('^' == 94) && ('_' == 95) &&               \
    ('a' == 97) && ('b' == 98) && ('c' == 99) && ('d' == 100) &&               \
    ('e' == 101) && ('f' == 102) && ('g' == 103) && ('h' == 104) &&            \
    ('i' == 105) && ('j' == 106) && ('k' == 107) && ('l' == 108) &&            \
    ('m' == 109) && ('n' == 110) && ('o' == 111) && ('p' == 112) &&            \
    ('q' == 113) && ('r' == 114) && ('s' == 115) && ('t' == 116) &&            \
    ('u' == 117) && ('v' == 118) && ('w' == 119) && ('x' == 120) &&            \
    ('y' == 121) && ('z' == 122) && ('{' == 123) && ('|' == 124) &&            \
    ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error \
    "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gperf@gnu.org>."
#endif

#line 7 "css_keywords.tmpl"

// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/css_keywords.h"

#include <cstring>

#define size_t unsigned
// NOLINTBEGIN(modernize-use-nullptr)
namespace lynx {
namespace tasm {
#line 20 "css_keywords.tmpl"
struct TokenValue;
/* maximum key range = 1643, duplicates = 0 */

class CSSKeywordsHash {
 private:
  static inline unsigned int hash(const char *str, size_t len);

 public:
  static const struct TokenValue *GetTokenValue(const char *str, size_t len);
};

inline unsigned int CSSKeywordsHash::hash(const char *str, size_t len) {
  static const unsigned short asso_values[] = {
      1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645,
      1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645,
      1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645,
      1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 288,  1645, 1645,
      1645, 1645, 1645, 0,    1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645,
      1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645,
      1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645,
      1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645,
      1645, 5,    130,  275,  15,   0,    333,  80,   135,  50,   1645, 235,
      0,    35,   5,    0,    85,   30,   20,   60,   0,    170,  435,  500,
      510,  350,  0,    1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645,
      1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645,
      1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645,
      1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645,
      1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645,
      1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645,
      1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645,
      1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645,
      1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645,
      1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645,
      1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645, 1645,
      1645, 1645, 1645, 1645};
  unsigned int hval = len;

  switch (hval) {
    default:
      hval += asso_values[static_cast<unsigned char>(str[12])];
#if (defined __cplusplus &&                                     \
     (__cplusplus >= 201703L ||                                 \
      (__cplusplus >= 201103L && defined __clang__ &&           \
       __clang_major__ + (__clang_minor__ >= 9) > 3))) ||       \
    (defined __STDC_VERSION__ && __STDC_VERSION__ >= 202000L && \
     ((defined __GNUC__ && __GNUC__ >= 10) ||                   \
      (defined __clang__ && __clang_major__ >= 9)))
      [[fallthrough]];
#elif (defined __GNUC__ && __GNUC__ >= 7) || \
    (defined __clang__ && __clang_major__ >= 10)
      __attribute__((__fallthrough__));
#endif
    /*FALLTHROUGH*/
    case 12:
      hval += asso_values[static_cast<unsigned char>(str[11])];
#if (defined __cplusplus &&                                     \
     (__cplusplus >= 201703L ||                                 \
      (__cplusplus >= 201103L && defined __clang__ &&           \
       __clang_major__ + (__clang_minor__ >= 9) > 3))) ||       \
    (defined __STDC_VERSION__ && __STDC_VERSION__ >= 202000L && \
     ((defined __GNUC__ && __GNUC__ >= 10) ||                   \
      (defined __clang__ && __clang_major__ >= 9)))
      [[fallthrough]];
#elif (defined __GNUC__ && __GNUC__ >= 7) || \
    (defined __clang__ && __clang_major__ >= 10)
      __attribute__((__fallthrough__));
#endif
    /*FALLTHROUGH*/
    case 11:
    case 10:
    case 9:
    case 8:
      hval += asso_values[static_cast<unsigned char>(str[7])];
#if (defined __cplusplus &&                                     \
     (__cplusplus >= 201703L ||                                 \
      (__cplusplus >= 201103L && defined __clang__ &&           \
       __clang_major__ + (__clang_minor__ >= 9) > 3))) ||       \
    (defined __STDC_VERSION__ && __STDC_VERSION__ >= 202000L && \
     ((defined __GNUC__ && __GNUC__ >= 10) ||                   \
      (defined __clang__ && __clang_major__ >= 9)))
      [[fallthrough]];
#elif (defined __GNUC__ && __GNUC__ >= 7) || \
    (defined __clang__ && __clang_major__ >= 10)
      __attribute__((__fallthrough__));
#endif
    /*FALLTHROUGH*/
    case 7:
      hval += asso_values[static_cast<unsigned char>(str[6])];
#if (defined __cplusplus &&                                     \
     (__cplusplus >= 201703L ||                                 \
      (__cplusplus >= 201103L && defined __clang__ &&           \
       __clang_major__ + (__clang_minor__ >= 9) > 3))) ||       \
    (defined __STDC_VERSION__ && __STDC_VERSION__ >= 202000L && \
     ((defined __GNUC__ && __GNUC__ >= 10) ||                   \
      (defined __clang__ && __clang_major__ >= 9)))
      [[fallthrough]];
#elif (defined __GNUC__ && __GNUC__ >= 7) || \
    (defined __clang__ && __clang_major__ >= 10)
      __attribute__((__fallthrough__));
#endif
    /*FALLTHROUGH*/
    case 6:
      hval += asso_values[static_cast<unsigned char>(str[5])];
#if (defined __cplusplus &&                                     \
     (__cplusplus >= 201703L ||                                 \
      (__cplusplus >= 201103L && defined __clang__ &&           \
       __clang_major__ + (__clang_minor__ >= 9) > 3))) ||       \
    (defined __STDC_VERSION__ && __STDC_VERSION__ >= 202000L && \
     ((defined __GNUC__ && __GNUC__ >= 10) ||                   \
      (defined __clang__ && __clang_major__ >= 9)))
      [[fallthrough]];
#elif (defined __GNUC__ && __GNUC__ >= 7) || \
    (defined __clang__ && __clang_major__ >= 10)
      __attribute__((__fallthrough__));
#endif
    /*FALLTHROUGH*/
    case 5:
    case 4:
      hval += asso_values[static_cast<unsigned char>(str[3])];
#if (defined __cplusplus &&                                     \
     (__cplusplus >= 201703L ||                                 \
      (__cplusplus >= 201103L && defined __clang__ &&           \
       __clang_major__ + (__clang_minor__ >= 9) > 3))) ||       \
    (defined __STDC_VERSION__ && __STDC_VERSION__ >= 202000L && \
     ((defined __GNUC__ && __GNUC__ >= 10) ||                   \
      (defined __clang__ && __clang_major__ >= 9)))
      [[fallthrough]];
#elif (defined __GNUC__ && __GNUC__ >= 7) || \
    (defined __clang__ && __clang_major__ >= 10)
      __attribute__((__fallthrough__));
#endif
    /*FALLTHROUGH*/
    case 3:
      hval += asso_values[static_cast<unsigned char>(str[2])];
#if (defined __cplusplus &&                                     \
     (__cplusplus >= 201703L ||                                 \
      (__cplusplus >= 201103L && defined __clang__ &&           \
       __clang_major__ + (__clang_minor__ >= 9) > 3))) ||       \
    (defined __STDC_VERSION__ && __STDC_VERSION__ >= 202000L && \
     ((defined __GNUC__ && __GNUC__ >= 10) ||                   \
      (defined __clang__ && __clang_major__ >= 9)))
      [[fallthrough]];
#elif (defined __GNUC__ && __GNUC__ >= 7) || \
    (defined __clang__ && __clang_major__ >= 10)
      __attribute__((__fallthrough__));
#endif
    /*FALLTHROUGH*/
    case 2:
      hval += asso_values[static_cast<unsigned char>(str[1])];
#if (defined __cplusplus &&                                     \
     (__cplusplus >= 201703L ||                                 \
      (__cplusplus >= 201103L && defined __clang__ &&           \
       __clang_major__ + (__clang_minor__ >= 9) > 3))) ||       \
    (defined __STDC_VERSION__ && __STDC_VERSION__ >= 202000L && \
     ((defined __GNUC__ && __GNUC__ >= 10) ||                   \
      (defined __clang__ && __clang_major__ >= 9)))
      [[fallthrough]];
#elif (defined __GNUC__ && __GNUC__ >= 7) || \
    (defined __clang__ && __clang_major__ >= 10)
      __attribute__((__fallthrough__));
#endif
    /*FALLTHROUGH*/
    case 1:
      hval += asso_values[static_cast<unsigned char>(str[0])];
      break;
  }
  return hval + asso_values[static_cast<unsigned char>(str[len - 1])];
}

const struct TokenValue *CSSKeywordsHash::GetTokenValue(const char *str,
                                                        size_t len) {
  enum {
    TOTAL_KEYWORDS = 311,
    MIN_WORD_LENGTH = 1,
    MAX_WORD_LENGTH = 20,
    MIN_HASH_VALUE = 2,
    MAX_HASH_VALUE = 1644
  };

#if (defined __GNUC__ && __GNUC__ + (__GNUC_MINOR__ >= 6) > 4) || \
    (defined __clang__ && __clang_major__ >= 3)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
  static const struct TokenValue token_list[] = {
#line 28 "css_keywords.tmpl"
      {"to", TokenType::TO},
#line 92 "css_keywords.tmpl"
      {"toleft", TokenType::TOLEFT},
#line 70 "css_keywords.tmpl"
      {"at", TokenType::AT},
#line 276 "css_keywords.tmpl"
      {"all", TokenType::ALL},
#line 241 "css_keywords.tmpl"
      {"teal", TokenType::TEAL},
#line 331 "css_keywords.tmpl"
      {"on", TokenType::ON},
#line 27 "css_keywords.tmpl"
      {"none", TokenType::NONE},
#line 240 "css_keywords.tmpl"
      {"tan", TokenType::TAN},
#line 320 "css_keywords.tmpl"
      {"alternate", TokenType::ALTERNATE},
#line 251 "css_keywords.tmpl"
      {"rotate", TokenType::ROTATE},
#line 254 "css_keywords.tmpl"
      {"rotatez", TokenType::ROTATE_Z},
#line 71 "css_keywords.tmpl"
      {"data", TokenType::DATA},
#line 209 "css_keywords.tmpl"
      {"orange", TokenType::ORANGE},
#line 255 "css_keywords.tmpl"
      {"translate", TokenType::TRANSLATE},
#line 259 "css_keywords.tmpl"
      {"translatez", TokenType::TRANSLATE_Z},
#line 243 "css_keywords.tmpl"
      {"tomato", TokenType::TOMATO},
#line 76 "css_keywords.tmpl"
      {"dotted", TokenType::DOTTED},
#line 223 "css_keywords.tmpl"
      {"red", TokenType::RED},
#line 45 "css_keywords.tmpl"
      {"rad", TokenType::RAD},
#line 256 "css_keywords.tmpl"
      {"translate3d", TokenType::TRANSLATE_3D},
#line 188 "css_keywords.tmpl"
      {"linen", TokenType::LINEN},
#line 89 "css_keywords.tmpl"
      {"normal", TokenType::NORMAL},
#line 310 "css_keywords.tmpl"
      {"ease", TokenType::EASE},
#line 37 "css_keywords.tmpl"
      {"em", TokenType::EM},
#line 210 "css_keywords.tmpl"
      {"orangered", TokenType::ORANGERED},
#line 190 "css_keywords.tmpl"
      {"maroon", TokenType::MAROON},
#line 186 "css_keywords.tmpl"
      {"lime", TokenType::LIME},
#line 94 "css_keywords.tmpl"
      {"totop", TokenType::TOTOP},
#line 36 "css_keywords.tmpl"
      {"rem", TokenType::REM},
#line 306 "css_keywords.tmpl"
      {"linear", TokenType::LINEAR},
#line 80 "css_keywords.tmpl"
      {"groove", TokenType::GROOVE},
#line 158 "css_keywords.tmpl"
      {"green", TokenType::GREEN},
#line 52 "css_keywords.tmpl"
      {"repeat", TokenType::REPEAT},
#line 155 "css_keywords.tmpl"
      {"gold", TokenType::GOLD},
#line 227 "css_keywords.tmpl"
      {"salmon", TokenType::SALMON},
#line 67 "css_keywords.tmpl"
      {"ellipse", TokenType::ELLIPSE},
#line 187 "css_keywords.tmpl"
      {"limegreen", TokenType::LIMEGREEN},
#line 82 "css_keywords.tmpl"
      {"inset", TokenType::INSET},
#line 318 "css_keywords.tmpl"
      {"s", TokenType::SECOND},
#line 213 "css_keywords.tmpl"
      {"palegreen", TokenType::PALEGREEN},
#line 164 "css_keywords.tmpl"
      {"indigo", TokenType::INDIGO},
#line 78 "css_keywords.tmpl"
      {"solid", TokenType::SOLID},
#line 231 "css_keywords.tmpl"
      {"sienna", TokenType::SIENNA},
#line 307 "css_keywords.tmpl"
      {"ease-in", TokenType::EASE_IN},
#line 230 "css_keywords.tmpl"
      {"seashell", TokenType::SEASHELL},
#line 189 "css_keywords.tmpl"
      {"magenta", TokenType::MAGENTA},
#line 44 "css_keywords.tmpl"
      {"grad", TokenType::GRAD},
#line 156 "css_keywords.tmpl"
      {"goldenrod", TokenType::GOLDENROD},
#line 212 "css_keywords.tmpl"
      {"palegoldenrod", TokenType::PALEGOLDENROD},
#line 103 "css_keywords.tmpl"
      {"transparent", TokenType::TRANSPARENT},
#line 301 "css_keywords.tmpl"
      {"margin", TokenType::MARGIN},
#line 317 "css_keywords.tmpl"
      {"ms", TokenType::MILLISECOND},
#line 201 "css_keywords.tmpl"
      {"mintcream", TokenType::MINTCREAM},
#line 229 "css_keywords.tmpl"
      {"seagreen", TokenType::SEAGREEN},
#line 90 "css_keywords.tmpl"
      {"bold", TokenType::BOLD},
#line 163 "css_keywords.tmpl"
      {"indianred", TokenType::INDIANRED},
#line 81 "css_keywords.tmpl"
      {"ridge", TokenType::RIDGE},
#line 30 "css_keywords.tmpl"
      {"top", TokenType::TOP},
#line 73 "css_keywords.tmpl"
      {"medium", TokenType::MEDIUM},
#line 43 "css_keywords.tmpl"
      {"deg", TokenType::DEG},
#line 47 "css_keywords.tmpl"
      {"auto", TokenType::AUTO},
#line 215 "css_keywords.tmpl"
      {"palevioletred", TokenType::PALEVIOLETRED},
#line 313 "css_keywords.tmpl"
      {"step-end", TokenType::STEP_END},
#line 26 "css_keywords.tmpl"
      {"url", TokenType::URL},
#line 328 "css_keywords.tmpl"
      {"true", TokenType::TOKEN_TRUE},
#line 193 "css_keywords.tmpl"
      {"mediumorchid", TokenType::MEDIUMORCHID},
#line 24 "css_keywords.tmpl"
      {"hsl", TokenType::HSL},
#line 72 "css_keywords.tmpl"
      {"thin", TokenType::THIN},
#line 108 "css_keywords.tmpl"
      {"azure", TokenType::AZURE},
#line 46 "css_keywords.tmpl"
      {"turn", TokenType::TURN},
#line 32 "css_keywords.tmpl"
      {"bottom", TokenType::BOTTOM},
#line 91 "css_keywords.tmpl"
      {"tobottom", TokenType::TOBOTTOM},
#line 25 "css_keywords.tmpl"
      {"hsla", TokenType::HSLA},
#line 316 "css_keywords.tmpl"
      {"steps", TokenType::STEPS},
#line 191 "css_keywords.tmpl"
      {"mediumaquamarine", TokenType::MEDIUMAQUAMARINE},
#line 93 "css_keywords.tmpl"
      {"toright", TokenType::TORIGHT},
#line 195 "css_keywords.tmpl"
      {"mediumseagreen", TokenType::MEDIUMSEAGREEN},
#line 55 "css_keywords.tmpl"
      {"round", TokenType::ROUND},
#line 106 "css_keywords.tmpl"
      {"aqua", TokenType::AQUA},
#line 312 "css_keywords.tmpl"
      {"step-start", TokenType::STEP_START},
#line 75 "css_keywords.tmpl"
      {"hidden", TokenType::HIDDEN},
#line 40 "css_keywords.tmpl"
      {"sp", TokenType::SP},
#line 202 "css_keywords.tmpl"
      {"mistyrose", TokenType::MISTYROSE},
#line 83 "css_keywords.tmpl"
      {"outset", TokenType::OUTSET},
#line 308 "css_keywords.tmpl"
      {"ease-out", TokenType::EASE_OUT},
#line 23 "css_keywords.tmpl"
      {"rgba", TokenType::RGBA},
#line 228 "css_keywords.tmpl"
      {"sandybrown", TokenType::SANDYBROWN},
#line 77 "css_keywords.tmpl"
      {"dashed", TokenType::DASHED},
#line 242 "css_keywords.tmpl"
      {"thistle", TokenType::THISTLE},
#line 84 "css_keywords.tmpl"
      {"underline", TokenType::UNDERLINE},
#line 226 "css_keywords.tmpl"
      {"saddlebrown", TokenType::SADDLEBROWN},
#line 109 "css_keywords.tmpl"
      {"beige", TokenType::BEIGE},
#line 148 "css_keywords.tmpl"
      {"dodgerblue", TokenType::DODGERBLUE},
#line 271 "css_keywords.tmpl"
      {"height", TokenType::HEIGHT},
#line 192 "css_keywords.tmpl"
      {"mediumblue", TokenType::MEDIUMBLUE},
#line 110 "css_keywords.tmpl"
      {"bisque", TokenType::BISQUE},
#line 88 "css_keywords.tmpl"
      {"local", TokenType::LOCAL},
#line 31 "css_keywords.tmpl"
      {"right", TokenType::RIGHT},
#line 302 "css_keywords.tmpl"
      {"padding", TokenType::PADDING},
#line 107 "css_keywords.tmpl"
      {"aquamarine", TokenType::AQUAMARINE},
#line 206 "css_keywords.tmpl"
      {"oldlace", TokenType::OLDLACE},
#line 153 "css_keywords.tmpl"
      {"gainsboro", TokenType::GAINSBORO},
#line 273 "css_keywords.tmpl"
      {"color", TokenType::COLOR},
#line 113 "css_keywords.tmpl"
      {"blue", TokenType::BLUE},
#line 120 "css_keywords.tmpl"
      {"coral", TokenType::CORAL},
#line 129 "css_keywords.tmpl"
      {"darkgreen", TokenType::DARKGREEN},
#line 136 "css_keywords.tmpl"
      {"darkred", TokenType::DARKRED},
#line 134 "css_keywords.tmpl"
      {"darkorange", TokenType::DARKORANGE},
#line 79 "css_keywords.tmpl"
      {"double", TokenType::DOUBLE},
#line 112 "css_keywords.tmpl"
      {"blanchedalmond", TokenType::BLANCHEDALMOND},
#line 33 "css_keywords.tmpl"
      {"center", TokenType::CENTER},
#line 220 "css_keywords.tmpl"
      {"plum", TokenType::PLUM},
#line 137 "css_keywords.tmpl"
      {"darksalmon", TokenType::DARKSALMON},
#line 200 "css_keywords.tmpl"
      {"midnightblue", TokenType::MIDNIGHTBLUE},
#line 127 "css_keywords.tmpl"
      {"darkgoldenrod", TokenType::DARKGOLDENROD},
#line 143 "css_keywords.tmpl"
      {"darkviolet", TokenType::DARKVIOLET},
#line 29 "css_keywords.tmpl"
      {"left", TokenType::LEFT},
#line 244 "css_keywords.tmpl"
      {"turquoise", TokenType::TURQUOISE},
#line 196 "css_keywords.tmpl"
      {"mediumslateblue", TokenType::MEDIUMSLATEBLUE},
#line 101 "css_keywords.tmpl"
      {"blur", TokenType::BLUR},
#line 260 "css_keywords.tmpl"
      {"scale", TokenType::SCALE},
#line 179 "css_keywords.tmpl"
      {"lightsalmon", TokenType::LIGHTSALMON},
#line 49 "css_keywords.tmpl"
      {"contain", TokenType::CONTAIN},
#line 180 "css_keywords.tmpl"
      {"lightseagreen", TokenType::LIGHTSEAGREEN},
#line 326 "css_keywords.tmpl"
      {"paused", TokenType::PAUSED},
#line 114 "css_keywords.tmpl"
      {"blueviolet", TokenType::BLUEVIOLET},
#line 22 "css_keywords.tmpl"
      {"rgb", TokenType::RGB},
#line 95 "css_keywords.tmpl"
      {"path", TokenType::PATH},
#line 222 "css_keywords.tmpl"
      {"purple", TokenType::PURPLE},
#line 239 "css_keywords.tmpl"
      {"steelblue", TokenType::STEELBLUE},
#line 198 "css_keywords.tmpl"
      {"mediumturquoise", TokenType::MEDIUMTURQUOISE},
#line 327 "css_keywords.tmpl"
      {"running", TokenType::RUNNING},
#line 234 "css_keywords.tmpl"
      {"slateblue", TokenType::SLATEBLUE},
#line 330 "css_keywords.tmpl"
      {"fr", TokenType::FR},
#line 132 "css_keywords.tmpl"
      {"darkmagenta", TokenType::DARKMAGENTA},
#line 176 "css_keywords.tmpl"
      {"lightgreen", TokenType::LIGHTGREEN},
#line 138 "css_keywords.tmpl"
      {"darkseagreen", TokenType::DARKSEAGREEN},
#line 214 "css_keywords.tmpl"
      {"paleturquoise", TokenType::PALETURQUOISE},
#line 87 "css_keywords.tmpl"
      {"format", TokenType::FORMAT},
#line 258 "css_keywords.tmpl"
      {"translatey", TokenType::TRANSLATE_Y},
#line 123 "css_keywords.tmpl"
      {"crimson", TokenType::CRIMSON},
#line 194 "css_keywords.tmpl"
      {"mediumpurple", TokenType::MEDIUMPURPLE},
#line 329 "css_keywords.tmpl"
      {"false", TokenType::TOKEN_FALSE},
#line 324 "css_keywords.tmpl"
      {"both", TokenType::BOTH},
#line 238 "css_keywords.tmpl"
      {"springgreen", TokenType::SPRINGGREEN},
#line 53 "css_keywords.tmpl"
      {"no-repeat", TokenType::NO_REPEAT},
#line 311 "css_keywords.tmpl"
      {"ease-in-out", TokenType::EASE_IN_OUT},
#line 275 "css_keywords.tmpl"
      {"transform", TokenType::TRANSFORM},
#line 303 "css_keywords.tmpl"
      {"filter", TokenType::FILTER},
#line 54 "css_keywords.tmpl"
      {"space", TokenType::SPACE},
#line 289 "css_keywords.tmpl"
      {"margin-left", TokenType::MARGIN_LEFT},
#line 65 "css_keywords.tmpl"
      {"farthest-side", TokenType::FARTHEST_SIDE},
#line 218 "css_keywords.tmpl"
      {"peru", TokenType::PERU},
#line 69 "css_keywords.tmpl"
      {"polygon", TokenType::POLYGON},
#line 125 "css_keywords.tmpl"
      {"darkblue", TokenType::DARKBLUE},
#line 197 "css_keywords.tmpl"
      {"mediumspringgreen", TokenType::MEDIUMSPRINGGREEN},
#line 139 "css_keywords.tmpl"
      {"darkslateblue", TokenType::DARKSLATEBLUE},
#line 290 "css_keywords.tmpl"
      {"margin-right", TokenType::MARGIN_RIGHT},
#line 211 "css_keywords.tmpl"
      {"orchid", TokenType::ORCHID},
#line 321 "css_keywords.tmpl"
      {"alternate-reverse", TokenType::ALTERNATE_REVERSE},
#line 151 "css_keywords.tmpl"
      {"forestgreen", TokenType::FORESTGREEN},
#line 66 "css_keywords.tmpl"
      {"farthest-corner", TokenType::FARTHEST_CORNER},
#line 100 "css_keywords.tmpl"
      {"var", TokenType::VAR},
#line 207 "css_keywords.tmpl"
      {"olive", TokenType::OLIVE},
#line 245 "css_keywords.tmpl"
      {"violet", TokenType::VIOLET},
#line 85 "css_keywords.tmpl"
      {"line-through", TokenType::LINE_THROUGH},
#line 60 "css_keywords.tmpl"
      {"border-area", TokenType::BORDER_AREA},
#line 325 "css_keywords.tmpl"
      {"infinite", TokenType::INFINITE},
#line 167 "css_keywords.tmpl"
      {"lavender", TokenType::LAVENDER},
#line 293 "css_keywords.tmpl"
      {"padding-left", TokenType::PADDING_LEFT},
#line 61 "css_keywords.tmpl"
      {"linear-gradient", TokenType::LINEAR_GRADIENT},
#line 184 "css_keywords.tmpl"
      {"lightsteelblue", TokenType::LIGHTSTEELBLUE},
#line 59 "css_keywords.tmpl"
      {"text", TokenType::TEXT},
#line 170 "css_keywords.tmpl"
      {"lemonchiffon", TokenType::LEMONCHIFFON},
#line 280 "css_keywords.tmpl"
      {"min-height", TokenType::MIN_HEIGHT},
#line 319 "css_keywords.tmpl"
      {"reverse", TokenType::REVERSE},
#line 62 "css_keywords.tmpl"
      {"radial-gradient", TokenType::RADIAL_GRADIENT},
#line 236 "css_keywords.tmpl"
      {"slategrey", TokenType::SLATEGREY},
#line 291 "css_keywords.tmpl"
      {"margin-top", TokenType::MARGIN_TOP},
#line 235 "css_keywords.tmpl"
      {"slategray", TokenType::SLATEGRAY},
#line 296 "css_keywords.tmpl"
      {"padding-bottom", TokenType::PADDING_BOTTOM},
#line 169 "css_keywords.tmpl"
      {"lawngreen", TokenType::LAWNGREEN},
#line 257 "css_keywords.tmpl"
      {"translatex", TokenType::TRANSLATE_X},
#line 142 "css_keywords.tmpl"
      {"darkturquoise", TokenType::DARKTURQUOISE},
#line 172 "css_keywords.tmpl"
      {"lightcoral", TokenType::LIGHTCORAL},
#line 171 "css_keywords.tmpl"
      {"lightblue", TokenType::LIGHTBLUE},
#line 279 "css_keywords.tmpl"
      {"min-width", TokenType::MIN_WIDTH},
#line 295 "css_keywords.tmpl"
      {"padding-top", TokenType::PADDING_TOP},
#line 232 "css_keywords.tmpl"
      {"silver", TokenType::SILVER},
#line 117 "css_keywords.tmpl"
      {"cadetblue", TokenType::CADETBLUE},
#line 267 "css_keywords.tmpl"
      {"matrix3d", TokenType::MATRIX_3D},
#line 219 "css_keywords.tmpl"
      {"pink", TokenType::PINK},
#line 68 "css_keywords.tmpl"
      {"circle", TokenType::CIRCLE},
#line 144 "css_keywords.tmpl"
      {"deeppink", TokenType::DEEPPINK},
#line 118 "css_keywords.tmpl"
      {"chartreuse", TokenType::CHARTREUSE},
#line 104 "css_keywords.tmpl"
      {"aliceblue", TokenType::ALICEBLUE},
#line 294 "css_keywords.tmpl"
      {"padding-right", TokenType::PADDING_RIGHT},
#line 124 "css_keywords.tmpl"
      {"cyan", TokenType::CYAN},
#line 246 "css_keywords.tmpl"
      {"wheat", TokenType::WHEAT},
#line 292 "css_keywords.tmpl"
      {"margin-bottom", TokenType::MARGIN_BOTTOM},
#line 126 "css_keywords.tmpl"
      {"darkcyan", TokenType::DARKCYAN},
#line 178 "css_keywords.tmpl"
      {"lightpink", TokenType::LIGHTPINK},
#line 111 "css_keywords.tmpl"
      {"black", TokenType::BLACK},
#line 115 "css_keywords.tmpl"
      {"brown", TokenType::BROWN},
#line 208 "css_keywords.tmpl"
      {"olivedrab", TokenType::OLIVEDRAB},
#line 166 "css_keywords.tmpl"
      {"khaki", TokenType::KHAKI},
#line 199 "css_keywords.tmpl"
      {"mediumvioletred", TokenType::MEDIUMVIOLETRED},
#line 96 "css_keywords.tmpl"
      {"super-ellipse", TokenType::SUPER_ELLIPSE},
#line 288 "css_keywords.tmpl"
      {"border-bottom-color", TokenType::BORDER_BOTTOM_COLOR},
#line 225 "css_keywords.tmpl"
      {"royalblue", TokenType::ROYALBLUE},
#line 102 "css_keywords.tmpl"
      {"fit-content", TokenType::FIT_CONTENT},
#line 247 "css_keywords.tmpl"
      {"white", TokenType::WHITE},
#line 63 "css_keywords.tmpl"
      {"closest-side", TokenType::CLOSEST_SIDE},
#line 119 "css_keywords.tmpl"
      {"chocolate", TokenType::CHOCOLATE},
#line 74 "css_keywords.tmpl"
      {"thick", TokenType::THICK},
#line 162 "css_keywords.tmpl"
      {"hotpink", TokenType::HOTPINK},
#line 270 "css_keywords.tmpl"
      {"width", TokenType::WIDTH},
#line 39 "css_keywords.tmpl"
      {"vh", TokenType::VH},
#line 131 "css_keywords.tmpl"
      {"darkkhaki", TokenType::DARKKHAKI},
#line 309 "css_keywords.tmpl"
      {"ease-in-ease-out", TokenType::EASE_IN_EASE_OUT},
#line 203 "css_keywords.tmpl"
      {"moccasin", TokenType::MOCCASIN},
#line 183 "css_keywords.tmpl"
      {"lightslategrey", TokenType::LIGHTSLATEGREY},
#line 182 "css_keywords.tmpl"
      {"lightslategray", TokenType::LIGHTSLATEGRAY},
#line 64 "css_keywords.tmpl"
      {"closest-corner", TokenType::CLOSEST_CORNER},
#line 177 "css_keywords.tmpl"
      {"lightgrey", TokenType::LIGHTGREY},
#line 175 "css_keywords.tmpl"
      {"lightgray", TokenType::LIGHTGRAY},
#line 135 "css_keywords.tmpl"
      {"darkorchid", TokenType::DARKORCHID},
#line 253 "css_keywords.tmpl"
      {"rotatey", TokenType::ROTATE_Y},
#line 48 "css_keywords.tmpl"
      {"cover", TokenType::COVER},
#line 314 "css_keywords.tmpl"
      {"square-bezier", TokenType::SQUARE_BEZIER},
#line 105 "css_keywords.tmpl"
      {"antiquewhite", TokenType::ANTIQUEWHITE},
#line 99 "css_keywords.tmpl"
      {"grayscale", TokenType::GRAYSCALE},
#line 323 "css_keywords.tmpl"
      {"backwards", TokenType::BACKWARDS},
#line 221 "css_keywords.tmpl"
      {"powderblue", TokenType::POWDERBLUE},
#line 133 "css_keywords.tmpl"
      {"darkolivegreen", TokenType::DARKOLIVEGREEN},
#line 287 "css_keywords.tmpl"
      {"border-top-color", TokenType::BORDER_TOP_COLOR},
#line 248 "css_keywords.tmpl"
      {"whitesmoke", TokenType::WHITESMOKE},
#line 284 "css_keywords.tmpl"
      {"border-bottom-width", TokenType::BORDER_BOTTOM_WIDTH},
#line 300 "css_keywords.tmpl"
      {"border-color", TokenType::BORDER_COLOR},
#line 160 "css_keywords.tmpl"
      {"grey", TokenType::GREY},
#line 157 "css_keywords.tmpl"
      {"gray", TokenType::GRAY},
#line 168 "css_keywords.tmpl"
      {"lavenderblush", TokenType::LAVENDERBLUSH},
#line 286 "css_keywords.tmpl"
      {"border-right-color", TokenType::BORDER_RIGHT_COLOR},
#line 145 "css_keywords.tmpl"
      {"deepskyblue", TokenType::DEEPSKYBLUE},
#line 122 "css_keywords.tmpl"
      {"cornsilk", TokenType::CORNSILK},
#line 97 "css_keywords.tmpl"
      {"calc", TokenType::CALC},
#line 116 "css_keywords.tmpl"
      {"burlywood", TokenType::BURLYWOOD},
#line 42 "css_keywords.tmpl"
      {"max-content", TokenType::MAX_CONTENT},
#line 165 "css_keywords.tmpl"
      {"ivory", TokenType::IVORY},
#line 98 "css_keywords.tmpl"
      {"env", TokenType::ENV},
#line 174 "css_keywords.tmpl"
      {"lightgoldenrodyellow", TokenType::LIGHTGOLDENRODYELLOW},
#line 147 "css_keywords.tmpl"
      {"dimgrey", TokenType::DIMGREY},
#line 146 "css_keywords.tmpl"
      {"dimgray", TokenType::DIMGRAY},
#line 173 "css_keywords.tmpl"
      {"lightcyan", TokenType::LIGHTCYAN},
#line 216 "css_keywords.tmpl"
      {"papayawhip", TokenType::PAPAYAWHIP},
#line 181 "css_keywords.tmpl"
      {"lightskyblue", TokenType::LIGHTSKYBLUE},
#line 282 "css_keywords.tmpl"
      {"border-right-width", TokenType::BORDER_RIGHT_WIDTH},
#line 304 "css_keywords.tmpl"
      {"background-position", TokenType::BACKGROUND_POSITION},
#line 233 "css_keywords.tmpl"
      {"skyblue", TokenType::SKYBLUE},
#line 159 "css_keywords.tmpl"
      {"greenyellow", TokenType::GREENYELLOW},
#line 224 "css_keywords.tmpl"
      {"rosybrown", TokenType::ROSYBROWN},
#line 250 "css_keywords.tmpl"
      {"yellowgreen", TokenType::YELLOWGREEN},
#line 154 "css_keywords.tmpl"
      {"ghostwhite", TokenType::GHOSTWHITE},
#line 278 "css_keywords.tmpl"
      {"max-height", TokenType::MAX_HEIGHT},
#line 152 "css_keywords.tmpl"
      {"fuchsia", TokenType::FUCHSIA},
#line 121 "css_keywords.tmpl"
      {"cornflowerblue", TokenType::CORNFLOWERBLUE},
#line 149 "css_keywords.tmpl"
      {"firebrick", TokenType::FIREBRICK},
#line 141 "css_keywords.tmpl"
      {"darkslategrey", TokenType::DARKSLATEGREY},
#line 140 "css_keywords.tmpl"
      {"darkslategray", TokenType::DARKSLATEGRAY},
#line 150 "css_keywords.tmpl"
      {"floralwhite", TokenType::FLORALWHITE},
#line 332 "css_keywords.tmpl"
      {"off", TokenType::OFF},
#line 130 "css_keywords.tmpl"
      {"darkgrey", TokenType::DARKGREY},
#line 128 "css_keywords.tmpl"
      {"darkgray", TokenType::DARKGRAY},
#line 57 "css_keywords.tmpl"
      {"padding-box", TokenType::PADDING_BOX},
#line 322 "css_keywords.tmpl"
      {"forwards", TokenType::FORWARDS},
#line 262 "css_keywords.tmpl"
      {"scaley", TokenType::SCALE_Y},
#line 277 "css_keywords.tmpl"
      {"max-width", TokenType::MAX_WIDTH},
#line 252 "css_keywords.tmpl"
      {"rotatex", TokenType::ROTATE_X},
#line 305 "css_keywords.tmpl"
      {"offset-distance", TokenType::OFFSET_DISTANCE},
#line 274 "css_keywords.tmpl"
      {"visibility", TokenType::VISIBILITY},
#line 237 "css_keywords.tmpl"
      {"snow", TokenType::SNOW},
#line 268 "css_keywords.tmpl"
      {"opacity", TokenType::OPACITY},
#line 285 "css_keywords.tmpl"
      {"border-left-color", TokenType::BORDER_LEFT_COLOR},
#line 266 "css_keywords.tmpl"
      {"matrix", TokenType::MATRIX},
#line 58 "css_keywords.tmpl"
      {"content-box", TokenType::CONTENT_BOX},
#line 315 "css_keywords.tmpl"
      {"cubic-bezier", TokenType::CUBIC_BEZIER},
#line 204 "css_keywords.tmpl"
      {"navajowhite", TokenType::NAVAJOWHITE},
#line 51 "css_keywords.tmpl"
      {"repeat-y", TokenType::REPEAT_Y},
#line 34 "css_keywords.tmpl"
      {"px", TokenType::PX},
#line 297 "css_keywords.tmpl"
      {"flex-basis", TokenType::FLEX_BASIS},
#line 56 "css_keywords.tmpl"
      {"border-box", TokenType::BORDER_BOX},
#line 185 "css_keywords.tmpl"
      {"lightyellow", TokenType::LIGHTYELLOW},
#line 35 "css_keywords.tmpl"
      {"rpx", TokenType::RPX},
#line 272 "css_keywords.tmpl"
      {"background-color", TokenType::BACKGROUND_COLOR},
#line 205 "css_keywords.tmpl"
      {"navy", TokenType::NAVY},
#line 265 "css_keywords.tmpl"
      {"skewy", TokenType::SKEW_Y},
#line 161 "css_keywords.tmpl"
      {"honeydew", TokenType::HONEYDEW},
#line 283 "css_keywords.tmpl"
      {"border-top-width", TokenType::BORDER_TOP_WIDTH},
#line 41 "css_keywords.tmpl"
      {"ppx", TokenType::PPX},
#line 299 "css_keywords.tmpl"
      {"border-width", TokenType::BORDER_WIDTH},
#line 217 "css_keywords.tmpl"
      {"peachpuff", TokenType::PEACHPUFF},
#line 263 "css_keywords.tmpl"
      {"skew", TokenType::SKEW},
#line 264 "css_keywords.tmpl"
      {"skewx", TokenType::SKEW_X},
#line 249 "css_keywords.tmpl"
      {"yellow", TokenType::YELLOW},
#line 261 "css_keywords.tmpl"
      {"scalex", TokenType::SCALE_X},
#line 281 "css_keywords.tmpl"
      {"border-left-width", TokenType::BORDER_LEFT_WIDTH},
#line 50 "css_keywords.tmpl"
      {"repeat-x", TokenType::REPEAT_X},
#line 38 "css_keywords.tmpl"
      {"vw", TokenType::VW},
#line 298 "css_keywords.tmpl"
      {"flex-grow", TokenType::FLEX_GROW},
#line 269 "css_keywords.tmpl"
      {"scalexy", TokenType::SCALE_XY},
#line 86 "css_keywords.tmpl"
      {"wavy", TokenType::WAVY}};
#if (defined __GNUC__ && __GNUC__ + (__GNUC_MINOR__ >= 6) > 4) || \
    (defined __clang__ && __clang_major__ >= 3)
#pragma GCC diagnostic pop
#endif

  static const short lookup[] = {
      -1,  -1,  0,   -1,  -1,  -1,  1,   2,   3,   4,   -1,  -1,  5,   -1,  6,
      -1,  -1,  -1,  7,   -1,  -1,  -1,  -1,  -1,  8,   -1,  -1,  -1,  -1,  -1,
      -1,  9,   10,  -1,  11,  -1,  12,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  13,
      14,  15,  -1,  -1,  -1,  -1,  16,  -1,  17,  -1,  -1,  -1,  -1,  18,  -1,
      -1,  19,  -1,  -1,  -1,  20,  21,  -1,  -1,  22,  -1,  -1,  23,  -1,  24,
      -1,  25,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  26,
      27,  -1,  -1,  28,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  29,  -1,  -1,  -1,
      -1,  30,  -1,  -1,  -1,  31,  32,  -1,  -1,  33,  -1,  34,  35,  -1,  36,
      37,  38,  -1,  -1,  39,  -1,  40,  -1,  -1,  -1,  41,  42,  43,  44,  -1,
      -1,  -1,  45,  -1,  46,  -1,  -1,  -1,  -1,  47,  -1,  -1,  -1,  48,  -1,
      -1,  49,  -1,  -1,  -1,  -1,  50,  51,  -1,  52,  -1,  -1,  -1,  53,  54,
      -1,  -1,  -1,  -1,  55,  56,  -1,  -1,  57,  -1,  -1,  58,  -1,  59,  60,
      -1,  -1,  -1,  61,  -1,  -1,  -1,  -1,  62,  -1,  -1,  -1,  -1,  63,  64,
      -1,  -1,  65,  66,  67,  68,  -1,  -1,  -1,  69,  -1,  70,  -1,  71,  72,
      73,  74,  75,  -1,  76,  77,  -1,  -1,  -1,  78,  79,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  80,  81,  -1,  82,  -1,  83,  -1,  -1,  -1,
      -1,  -1,  -1,  84,  85,  -1,  -1,  -1,  -1,  -1,  86,  87,  88,  -1,  89,
      -1,  -1,  -1,  -1,  -1,  -1,  90,  -1,  -1,  -1,  91,  -1,  -1,  -1,  -1,
      92,  93,  -1,  -1,  -1,  94,  95,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      96,  -1,  -1,  -1,  -1,  97,  -1,  98,  -1,  -1,  99,  -1,  100, -1,  101,
      102, -1,  -1,  -1,  103, 104, -1,  -1,  -1,  105, -1,  -1,  106, -1,  -1,
      107, -1,  -1,  -1,  -1,  -1,  108, -1,  -1,  109, -1,  110, -1,  -1,  111,
      112, -1,  113, 114, -1,  115, -1,  116, -1,  117, 118, -1,  -1,  -1,  119,
      120, 121, 122, -1,  -1,  -1,  -1,  -1,  123, -1,  -1,  124, -1,  -1,  -1,
      125, -1,  -1,  126, 127, -1,  128, -1,  -1,  129, 130, -1,  131, -1,  132,
      133, 134, -1,  -1,  -1,  135, -1,  136, 137, -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  138, 139, -1,  140, -1,  -1,  -1,  -1,  141, 142, 143,
      -1,  -1,  -1,  -1,  -1,  -1,  144, 145, -1,  -1,  -1,  -1,  -1,  -1,  146,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  147, -1,  148, 149, -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  150, -1,  151, -1,  -1,  152,
      -1,  -1,  153, 154, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  155, 156, -1,
      157, 158, 159, -1,  160, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  161, -1,
      -1,  -1,  -1,  162, -1,  -1,  -1,  -1,  -1,  -1,  163, 164, 165, -1,  166,
      -1,  167, -1,  -1,  -1,  -1,  -1,  -1,  168, -1,  169, -1,  -1,  170, 171,
      -1,  -1,  -1,  -1,  172, -1,  -1,  173, 174, -1,  -1,  -1,  175, 176, 177,
      -1,  -1,  -1,  178, 179, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  180, -1,  181, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      182, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  183, -1,
      184, -1,  -1,  -1,  185, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  186, -1,  187, -1,  188, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  189, -1,  -1,  -1,  190, -1,  -1,  -1,  -1,  -1,  191,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  192, -1,  -1,  -1,
      -1,  -1,  -1,  193, -1,  194, -1,  -1,  -1,  195, -1,  196, -1,  -1,  197,
      198, 199, -1,  200, 201, 202, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      203, -1,  -1,  -1,  204, 205, -1,  -1,  -1,  -1,  206, -1,  -1,  -1,  -1,
      -1,  207, 208, -1,  -1,  -1,  -1,  -1,  -1,  209, -1,  -1,  210, -1,  -1,
      211, -1,  -1,  -1,  -1,  212, -1,  -1,  -1,  213, 214, -1,  215, -1,  -1,
      216, -1,  217, -1,  218, -1,  -1,  219, 220, 221, -1,  -1,  -1,  -1,  222,
      -1,  -1,  223, -1,  224, -1,  -1,  -1,  -1,  225, 226, -1,  227, -1,  -1,
      228, 229, -1,  -1,  -1,  -1,  -1,  230, -1,  231, -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  232, -1,  -1,  -1,  -1,  -1,  233, -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  234,
      -1,  -1,  -1,  -1,  235, -1,  -1,  -1,  -1,  -1,  236, -1,  237, -1,  -1,
      -1,  -1,  -1,  -1,  -1,  238, -1,  -1,  -1,  239, -1,  -1,  -1,  -1,  240,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  241, 242, -1,  -1,  -1,  -1,  -1,
      -1,  243, -1,  244, -1,  -1,  -1,  -1,  -1,  245, -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  246, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  247,
      -1,  -1,  -1,  -1,  -1,  248, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  249, -1,  -1,  -1,  -1,  -1,  -1,
      250, -1,  251, -1,  -1,  -1,  -1,  252, -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  253, -1,  -1,  -1,  -1,  -1,
      254, -1,  -1,  -1,  -1,  -1,  -1,  255, -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  256, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  257,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  258, -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  259, -1,  -1,  260, -1,  261, -1,  -1,  -1,  262, -1,  -1,  -1,  -1,
      -1,  -1,  -1,  263, -1,  264, -1,  -1,  -1,  265, -1,  -1,  -1,  -1,  -1,
      -1,  -1,  266, 267, -1,  -1,  -1,  -1,  268, 269, -1,  -1,  270, 271, -1,
      -1,  -1,  -1,  272, -1,  -1,  -1,  -1,  -1,  273, -1,  274, -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  275, 276, -1,  -1,
      -1,  -1,  277, -1,  278, 279, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  280, -1,  -1,  281, 282, -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  283, -1,  -1,  -1,  -1,  -1,  -1,  -1,  284,
      285, 286, -1,  -1,  -1,  -1,  287, -1,  -1,  -1,  -1,  -1,  288, 289, -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  290, -1,
      -1,  291, -1,  292, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  293, -1,  -1,  294, 295, -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  296, -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  297, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  298, -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  299, -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  300, -1,  -1,  -1,  301, -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  302, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  303, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  304, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  305, -1,  -1,  -1,  -1,  -1,  -1,  -1,  306, -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  307, -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  308, -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  309, -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  310};

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH) {
    unsigned int key = hash(str, len);

    if (key <= MAX_HASH_VALUE) {
      int index = lookup[key];

      if (index >= 0) {
        const char *s = token_list[index].name;

        if (*str == *s && !strcmp(str + 1, s + 1)) return &token_list[index];
      }
    }
  }
  return nullptr;
}
#line 333 "css_keywords.tmpl"

const struct TokenValue *GetTokenValue(const char *str, unsigned len) {
  return CSSKeywordsHash::GetTokenValue(str, len);
}
}  // namespace tasm
}  // namespace lynx
// NOLINTEND(modernize-use-nullptr)
