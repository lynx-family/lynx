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
/* maximum key range = 1604, duplicates = 0 */

class CSSKeywordsHash {
 private:
  static inline unsigned int hash(const char *str, size_t len);

 public:
  static const struct TokenValue *GetTokenValue(const char *str, size_t len);
};

inline unsigned int CSSKeywordsHash::hash(const char *str, size_t len) {
  static const unsigned short asso_values[] = {
      1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606,
      1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606,
      1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606,
      1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 260,  1606, 1606,
      1606, 1606, 1606, 0,    1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606,
      1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606,
      1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606,
      1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606,
      1606, 5,    120,  295,  15,   0,    415,  155,  90,   20,   1606, 275,
      0,    45,   5,    0,    75,   365,  20,   55,   0,    110,  378,  368,
      285,  425,  0,    1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606,
      1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606,
      1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606,
      1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606,
      1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606,
      1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606,
      1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606,
      1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606,
      1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606,
      1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606,
      1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606, 1606,
      1606, 1606, 1606, 1606};
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
    TOTAL_KEYWORDS = 328,
    MIN_WORD_LENGTH = 1,
    MAX_WORD_LENGTH = 20,
    MIN_HASH_VALUE = 2,
    MAX_HASH_VALUE = 1605
  };

#if (defined __GNUC__ && __GNUC__ + (__GNUC_MINOR__ >= 6) > 4) || \
    (defined __clang__ && __clang_major__ >= 3)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
  static const struct TokenValue token_list[] = {
#line 28 "css_keywords.tmpl"
      {"to", TokenType::TO},
#line 99 "css_keywords.tmpl"
      {"toleft", TokenType::TOLEFT},
#line 77 "css_keywords.tmpl"
      {"at", TokenType::AT},
#line 288 "css_keywords.tmpl"
      {"all", TokenType::ALL},
#line 253 "css_keywords.tmpl"
      {"teal", TokenType::TEAL},
#line 347 "css_keywords.tmpl"
      {"on", TokenType::ON},
#line 27 "css_keywords.tmpl"
      {"none", TokenType::NONE},
#line 252 "css_keywords.tmpl"
      {"tan", TokenType::TAN},
#line 334 "css_keywords.tmpl"
      {"alternate", TokenType::ALTERNATE},
#line 263 "css_keywords.tmpl"
      {"rotate", TokenType::ROTATE},
#line 266 "css_keywords.tmpl"
      {"rotatez", TokenType::ROTATE_Z},
#line 78 "css_keywords.tmpl"
      {"data", TokenType::DATA},
#line 200 "css_keywords.tmpl"
      {"linen", TokenType::LINEN},
#line 221 "css_keywords.tmpl"
      {"orange", TokenType::ORANGE},
#line 267 "css_keywords.tmpl"
      {"translate", TokenType::TRANSLATE},
#line 271 "css_keywords.tmpl"
      {"translatez", TokenType::TRANSLATE_Z},
#line 83 "css_keywords.tmpl"
      {"dotted", TokenType::DOTTED},
#line 235 "css_keywords.tmpl"
      {"red", TokenType::RED},
#line 255 "css_keywords.tmpl"
      {"tomato", TokenType::TOMATO},
#line 46 "css_keywords.tmpl"
      {"rad", TokenType::RAD},
#line 268 "css_keywords.tmpl"
      {"translate3d", TokenType::TRANSLATE_3D},
#line 324 "css_keywords.tmpl"
      {"ease", TokenType::EASE},
#line 29 "css_keywords.tmpl"
      {"line-range", TokenType::LINE_RANGE},
#line 176 "css_keywords.tmpl"
      {"indigo", TokenType::INDIGO},
#line 198 "css_keywords.tmpl"
      {"lime", TokenType::LIME},
#line 320 "css_keywords.tmpl"
      {"linear", TokenType::LINEAR},
#line 222 "css_keywords.tmpl"
      {"orangered", TokenType::ORANGERED},
#line 96 "css_keywords.tmpl"
      {"normal", TokenType::NORMAL},
#line 101 "css_keywords.tmpl"
      {"totop", TokenType::TOTOP},
#line 74 "css_keywords.tmpl"
      {"ellipse", TokenType::ELLIPSE},
#line 89 "css_keywords.tmpl"
      {"inset", TokenType::INSET},
#line 202 "css_keywords.tmpl"
      {"maroon", TokenType::MAROON},
#line 38 "css_keywords.tmpl"
      {"em", TokenType::EM},
#line 85 "css_keywords.tmpl"
      {"solid", TokenType::SOLID},
#line 243 "css_keywords.tmpl"
      {"sienna", TokenType::SIENNA},
#line 321 "css_keywords.tmpl"
      {"ease-in", TokenType::EASE_IN},
#line 199 "css_keywords.tmpl"
      {"limegreen", TokenType::LIMEGREEN},
#line 53 "css_keywords.tmpl"
      {"repeat", TokenType::REPEAT},
#line 175 "css_keywords.tmpl"
      {"indianred", TokenType::INDIANRED},
#line 332 "css_keywords.tmpl"
      {"s", TokenType::SECOND},
#line 37 "css_keywords.tmpl"
      {"rem", TokenType::REM},
#line 225 "css_keywords.tmpl"
      {"palegreen", TokenType::PALEGREEN},
#line 48 "css_keywords.tmpl"
      {"auto", TokenType::AUTO},
#line 239 "css_keywords.tmpl"
      {"salmon", TokenType::SALMON},
#line 242 "css_keywords.tmpl"
      {"seashell", TokenType::SEASHELL},
#line 79 "css_keywords.tmpl"
      {"thin", TokenType::THIN},
#line 26 "css_keywords.tmpl"
      {"url", TokenType::URL},
#line 344 "css_keywords.tmpl"
      {"true", TokenType::TOKEN_TRUE},
#line 100 "css_keywords.tmpl"
      {"toright", TokenType::TORIGHT},
#line 224 "css_keywords.tmpl"
      {"palegoldenrod", TokenType::PALEGOLDENROD},
#line 120 "css_keywords.tmpl"
      {"azure", TokenType::AZURE},
#line 115 "css_keywords.tmpl"
      {"transparent", TokenType::TRANSPARENT},
#line 227 "css_keywords.tmpl"
      {"palevioletred", TokenType::PALEVIOLETRED},
#line 47 "css_keywords.tmpl"
      {"turn", TokenType::TURN},
#line 24 "css_keywords.tmpl"
      {"hsl", TokenType::HSL},
#line 213 "css_keywords.tmpl"
      {"mintcream", TokenType::MINTCREAM},
#line 31 "css_keywords.tmpl"
      {"top", TokenType::TOP},
#line 97 "css_keywords.tmpl"
      {"bold", TokenType::BOLD},
#line 56 "css_keywords.tmpl"
      {"round", TokenType::ROUND},
#line 82 "css_keywords.tmpl"
      {"hidden", TokenType::HIDDEN},
#line 331 "css_keywords.tmpl"
      {"ms", TokenType::MILLISECOND},
#line 25 "css_keywords.tmpl"
      {"hsla", TokenType::HSLA},
#line 91 "css_keywords.tmpl"
      {"underline", TokenType::UNDERLINE},
#line 90 "css_keywords.tmpl"
      {"outset", TokenType::OUTSET},
#line 254 "css_keywords.tmpl"
      {"thistle", TokenType::THISTLE},
#line 327 "css_keywords.tmpl"
      {"step-end", TokenType::STEP_END},
#line 80 "css_keywords.tmpl"
      {"medium", TokenType::MEDIUM},
#line 322 "css_keywords.tmpl"
      {"ease-out", TokenType::EASE_OUT},
#line 87 "css_keywords.tmpl"
      {"groove", TokenType::GROOVE},
#line 110 "css_keywords.tmpl"
      {"saturate", TokenType::SATURATE},
#line 170 "css_keywords.tmpl"
      {"green", TokenType::GREEN},
#line 205 "css_keywords.tmpl"
      {"mediumorchid", TokenType::MEDIUMORCHID},
#line 167 "css_keywords.tmpl"
      {"gold", TokenType::GOLD},
#line 330 "css_keywords.tmpl"
      {"steps", TokenType::STEPS},
#line 207 "css_keywords.tmpl"
      {"mediumseagreen", TokenType::MEDIUMSEAGREEN},
#line 326 "css_keywords.tmpl"
      {"step-start", TokenType::STEP_START},
#line 84 "css_keywords.tmpl"
      {"dashed", TokenType::DASHED},
#line 214 "css_keywords.tmpl"
      {"mistyrose", TokenType::MISTYROSE},
#line 41 "css_keywords.tmpl"
      {"sp", TokenType::SP},
#line 45 "css_keywords.tmpl"
      {"grad", TokenType::GRAD},
#line 88 "css_keywords.tmpl"
      {"ridge", TokenType::RIDGE},
#line 33 "css_keywords.tmpl"
      {"bottom", TokenType::BOTTOM},
#line 98 "css_keywords.tmpl"
      {"tobottom", TokenType::TOBOTTOM},
#line 168 "css_keywords.tmpl"
      {"goldenrod", TokenType::GOLDENROD},
#line 201 "css_keywords.tmpl"
      {"magenta", TokenType::MAGENTA},
#line 241 "css_keywords.tmpl"
      {"seagreen", TokenType::SEAGREEN},
#line 125 "css_keywords.tmpl"
      {"blue", TokenType::BLUE},
#line 240 "css_keywords.tmpl"
      {"sandybrown", TokenType::SANDYBROWN},
#line 313 "css_keywords.tmpl"
      {"margin", TokenType::MARGIN},
#line 238 "css_keywords.tmpl"
      {"saddlebrown", TokenType::SADDLEBROWN},
#line 86 "css_keywords.tmpl"
      {"double", TokenType::DOUBLE},
#line 104 "css_keywords.tmpl"
      {"superellipse", TokenType::SUPERELLIPSE},
#line 204 "css_keywords.tmpl"
      {"mediumblue", TokenType::MEDIUMBLUE},
#line 126 "css_keywords.tmpl"
      {"blueviolet", TokenType::BLUEVIOLET},
#line 102 "css_keywords.tmpl"
      {"path", TokenType::PATH},
#line 124 "css_keywords.tmpl"
      {"blanchedalmond", TokenType::BLANCHEDALMOND},
#line 210 "css_keywords.tmpl"
      {"mediumturquoise", TokenType::MEDIUMTURQUOISE},
#line 283 "css_keywords.tmpl"
      {"height", TokenType::HEIGHT},
#line 113 "css_keywords.tmpl"
      {"blur", TokenType::BLUR},
#line 232 "css_keywords.tmpl"
      {"plum", TokenType::PLUM},
#line 342 "css_keywords.tmpl"
      {"paused", TokenType::PAUSED},
#line 234 "css_keywords.tmpl"
      {"purple", TokenType::PURPLE},
#line 65 "css_keywords.tmpl"
      {"text", TokenType::TEXT},
#line 32 "css_keywords.tmpl"
      {"right", TokenType::RIGHT},
#line 251 "css_keywords.tmpl"
      {"steelblue", TokenType::STEELBLUE},
#line 246 "css_keywords.tmpl"
      {"slateblue", TokenType::SLATEBLUE},
#line 121 "css_keywords.tmpl"
      {"beige", TokenType::BEIGE},
#line 338 "css_keywords.tmpl"
      {"both", TokenType::BOTH},
#line 95 "css_keywords.tmpl"
      {"local", TokenType::LOCAL},
#line 23 "css_keywords.tmpl"
      {"rgba", TokenType::RGBA},
#line 208 "css_keywords.tmpl"
      {"mediumslateblue", TokenType::MEDIUMSLATEBLUE},
#line 218 "css_keywords.tmpl"
      {"oldlace", TokenType::OLDLACE},
#line 230 "css_keywords.tmpl"
      {"peru", TokenType::PERU},
#line 285 "css_keywords.tmpl"
      {"color", TokenType::COLOR},
#line 206 "css_keywords.tmpl"
      {"mediumpurple", TokenType::MEDIUMPURPLE},
#line 132 "css_keywords.tmpl"
      {"coral", TokenType::CORAL},
#line 92 "css_keywords.tmpl"
      {"line-through", TokenType::LINE_THROUGH},
#line 44 "css_keywords.tmpl"
      {"deg", TokenType::DEG},
#line 269 "css_keywords.tmpl"
      {"translatex", TokenType::TRANSLATE_X},
#line 165 "css_keywords.tmpl"
      {"gainsboro", TokenType::GAINSBORO},
#line 160 "css_keywords.tmpl"
      {"dodgerblue", TokenType::DODGERBLUE},
#line 50 "css_keywords.tmpl"
      {"contain", TokenType::CONTAIN},
#line 191 "css_keywords.tmpl"
      {"lightsalmon", TokenType::LIGHTSALMON},
#line 212 "css_keywords.tmpl"
      {"midnightblue", TokenType::MIDNIGHTBLUE},
#line 155 "css_keywords.tmpl"
      {"darkviolet", TokenType::DARKVIOLET},
#line 34 "css_keywords.tmpl"
      {"center", TokenType::CENTER},
#line 192 "css_keywords.tmpl"
      {"lightseagreen", TokenType::LIGHTSEAGREEN},
#line 141 "css_keywords.tmpl"
      {"darkgreen", TokenType::DARKGREEN},
#line 148 "css_keywords.tmpl"
      {"darkred", TokenType::DARKRED},
#line 146 "css_keywords.tmpl"
      {"darkorange", TokenType::DARKORANGE},
#line 325 "css_keywords.tmpl"
      {"ease-in-out", TokenType::EASE_IN_OUT},
#line 272 "css_keywords.tmpl"
      {"scale", TokenType::SCALE},
#line 109 "css_keywords.tmpl"
      {"contrast", TokenType::CONTRAST},
#line 139 "css_keywords.tmpl"
      {"darkgoldenrod", TokenType::DARKGOLDENROD},
#line 54 "css_keywords.tmpl"
      {"no-repeat", TokenType::NO_REPEAT},
#line 149 "css_keywords.tmpl"
      {"darksalmon", TokenType::DARKSALMON},
#line 108 "css_keywords.tmpl"
      {"brightness", TokenType::BRIGHTNESS},
#line 279 "css_keywords.tmpl"
      {"matrix3d", TokenType::MATRIX_3D},
#line 61 "css_keywords.tmpl"
      {"notch", TokenType::NOTCH},
#line 135 "css_keywords.tmpl"
      {"crimson", TokenType::CRIMSON},
#line 219 "css_keywords.tmpl"
      {"olive", TokenType::OLIVE},
#line 257 "css_keywords.tmpl"
      {"violet", TokenType::VIOLET},
#line 335 "css_keywords.tmpl"
      {"alternate-reverse", TokenType::ALTERNATE_REVERSE},
#line 181 "css_keywords.tmpl"
      {"lawngreen", TokenType::LAWNGREEN},
#line 22 "css_keywords.tmpl"
      {"rgb", TokenType::RGB},
#line 30 "css_keywords.tmpl"
      {"left", TokenType::LEFT},
#line 112 "css_keywords.tmpl"
      {"var", TokenType::VAR},
#line 60 "css_keywords.tmpl"
      {"scoop", TokenType::SCOOP},
#line 314 "css_keywords.tmpl"
      {"padding", TokenType::PADDING},
#line 137 "css_keywords.tmpl"
      {"darkblue", TokenType::DARKBLUE},
#line 55 "css_keywords.tmpl"
      {"space", TokenType::SPACE},
#line 223 "css_keywords.tmpl"
      {"orchid", TokenType::ORCHID},
#line 151 "css_keywords.tmpl"
      {"darkslateblue", TokenType::DARKSLATEBLUE},
#line 196 "css_keywords.tmpl"
      {"lightsteelblue", TokenType::LIGHTSTEELBLUE},
#line 179 "css_keywords.tmpl"
      {"lavender", TokenType::LAVENDER},
#line 188 "css_keywords.tmpl"
      {"lightgreen", TokenType::LIGHTGREEN},
#line 66 "css_keywords.tmpl"
      {"border-area", TokenType::BORDER_AREA},
#line 346 "css_keywords.tmpl"
      {"fr", TokenType::FR},
#line 333 "css_keywords.tmpl"
      {"reverse", TokenType::REVERSE},
#line 343 "css_keywords.tmpl"
      {"running", TokenType::RUNNING},
#line 291 "css_keywords.tmpl"
      {"min-width", TokenType::MIN_WIDTH},
#line 258 "css_keywords.tmpl"
      {"wheat", TokenType::WHEAT},
#line 270 "css_keywords.tmpl"
      {"translatey", TokenType::TRANSLATE_Y},
#line 182 "css_keywords.tmpl"
      {"lemonchiffon", TokenType::LEMONCHIFFON},
#line 111 "css_keywords.tmpl"
      {"hue-rotate", TokenType::HUE_ROTATE},
#line 345 "css_keywords.tmpl"
      {"false", TokenType::TOKEN_FALSE},
#line 315 "css_keywords.tmpl"
      {"filter", TokenType::FILTER},
#line 259 "css_keywords.tmpl"
      {"white", TokenType::WHITE},
#line 94 "css_keywords.tmpl"
      {"format", TokenType::FORMAT},
#line 339 "css_keywords.tmpl"
      {"infinite", TokenType::INFINITE},
#line 144 "css_keywords.tmpl"
      {"darkmagenta", TokenType::DARKMAGENTA},
#line 118 "css_keywords.tmpl"
      {"aqua", TokenType::AQUA},
#line 67 "css_keywords.tmpl"
      {"linear-gradient", TokenType::LINEAR_GRADIENT},
#line 150 "css_keywords.tmpl"
      {"darkseagreen", TokenType::DARKSEAGREEN},
#line 282 "css_keywords.tmpl"
      {"width", TokenType::WIDTH},
#line 244 "css_keywords.tmpl"
      {"silver", TokenType::SILVER},
#line 301 "css_keywords.tmpl"
      {"margin-left", TokenType::MARGIN_LEFT},
#line 59 "css_keywords.tmpl"
      {"bevel", TokenType::BEVEL},
#line 183 "css_keywords.tmpl"
      {"lightblue", TokenType::LIGHTBLUE},
#line 68 "css_keywords.tmpl"
      {"radial-gradient", TokenType::RADIAL_GRADIENT},
#line 292 "css_keywords.tmpl"
      {"min-height", TokenType::MIN_HEIGHT},
#line 250 "css_keywords.tmpl"
      {"springgreen", TokenType::SPRINGGREEN},
#line 76 "css_keywords.tmpl"
      {"polygon", TokenType::POLYGON},
#line 127 "css_keywords.tmpl"
      {"brown", TokenType::BROWN},
#line 287 "css_keywords.tmpl"
      {"transform", TokenType::TRANSFORM},
#line 117 "css_keywords.tmpl"
      {"antiquewhite", TokenType::ANTIQUEWHITE},
#line 302 "css_keywords.tmpl"
      {"margin-right", TokenType::MARGIN_RIGHT},
#line 72 "css_keywords.tmpl"
      {"farthest-side", TokenType::FARTHEST_SIDE},
#line 318 "css_keywords.tmpl"
      {"transform-origin", TokenType::TRANSFORM_ORIGIN},
#line 349 "css_keywords.tmpl"
      {"from", TokenType::FROM},
#line 203 "css_keywords.tmpl"
      {"mediumaquamarine", TokenType::MEDIUMAQUAMARINE},
#line 119 "css_keywords.tmpl"
      {"aquamarine", TokenType::AQUAMARINE},
#line 58 "css_keywords.tmpl"
      {"square", TokenType::SQUARE},
#line 305 "css_keywords.tmpl"
      {"padding-left", TokenType::PADDING_LEFT},
#line 130 "css_keywords.tmpl"
      {"chartreuse", TokenType::CHARTREUSE},
#line 129 "css_keywords.tmpl"
      {"cadetblue", TokenType::CADETBLUE},
#line 73 "css_keywords.tmpl"
      {"farthest-corner", TokenType::FARTHEST_CORNER},
#line 116 "css_keywords.tmpl"
      {"aliceblue", TokenType::ALICEBLUE},
#line 40 "css_keywords.tmpl"
      {"vh", TokenType::VH},
#line 122 "css_keywords.tmpl"
      {"bisque", TokenType::BISQUE},
#line 220 "css_keywords.tmpl"
      {"olivedrab", TokenType::OLIVEDRAB},
#line 103 "css_keywords.tmpl"
      {"super-ellipse", TokenType::SUPER_ELLIPSE},
#line 211 "css_keywords.tmpl"
      {"mediumvioletred", TokenType::MEDIUMVIOLETRED},
#line 303 "css_keywords.tmpl"
      {"margin-top", TokenType::MARGIN_TOP},
#line 256 "css_keywords.tmpl"
      {"turquoise", TokenType::TURQUOISE},
#line 209 "css_keywords.tmpl"
      {"mediumspringgreen", TokenType::MEDIUMSPRINGGREEN},
#line 260 "css_keywords.tmpl"
      {"whitesmoke", TokenType::WHITESMOKE},
#line 308 "css_keywords.tmpl"
      {"padding-bottom", TokenType::PADDING_BOTTOM},
#line 184 "css_keywords.tmpl"
      {"lightcoral", TokenType::LIGHTCORAL},
#line 264 "css_keywords.tmpl"
      {"rotatex", TokenType::ROTATE_X},
#line 233 "css_keywords.tmpl"
      {"powderblue", TokenType::POWDERBLUE},
#line 43 "css_keywords.tmpl"
      {"max-content", TokenType::MAX_CONTENT},
#line 307 "css_keywords.tmpl"
      {"padding-top", TokenType::PADDING_TOP},
#line 323 "css_keywords.tmpl"
      {"ease-in-ease-out", TokenType::EASE_IN_EASE_OUT},
#line 163 "css_keywords.tmpl"
      {"forestgreen", TokenType::FORESTGREEN},
#line 306 "css_keywords.tmpl"
      {"padding-right", TokenType::PADDING_RIGHT},
#line 75 "css_keywords.tmpl"
      {"circle", TokenType::CIRCLE},
#line 300 "css_keywords.tmpl"
      {"border-bottom-color", TokenType::BORDER_BOTTOM_COLOR},
#line 128 "css_keywords.tmpl"
      {"burlywood", TokenType::BURLYWOOD},
#line 226 "css_keywords.tmpl"
      {"paleturquoise", TokenType::PALETURQUOISE},
#line 278 "css_keywords.tmpl"
      {"matrix", TokenType::MATRIX},
#line 35 "css_keywords.tmpl"
      {"px", TokenType::PX},
#line 190 "css_keywords.tmpl"
      {"lightpink", TokenType::LIGHTPINK},
#line 231 "css_keywords.tmpl"
      {"pink", TokenType::PINK},
#line 180 "css_keywords.tmpl"
      {"lavenderblush", TokenType::LAVENDERBLUSH},
#line 36 "css_keywords.tmpl"
      {"rpx", TokenType::RPX},
#line 248 "css_keywords.tmpl"
      {"slategrey", TokenType::SLATEGREY},
#line 178 "css_keywords.tmpl"
      {"khaki", TokenType::KHAKI},
#line 156 "css_keywords.tmpl"
      {"deeppink", TokenType::DEEPPINK},
#line 247 "css_keywords.tmpl"
      {"slategray", TokenType::SLATEGRAY},
#line 70 "css_keywords.tmpl"
      {"closest-side", TokenType::CLOSEST_SIDE},
#line 81 "css_keywords.tmpl"
      {"thick", TokenType::THICK},
#line 237 "css_keywords.tmpl"
      {"royalblue", TokenType::ROYALBLUE},
#line 131 "css_keywords.tmpl"
      {"chocolate", TokenType::CHOCOLATE},
#line 49 "css_keywords.tmpl"
      {"cover", TokenType::COVER},
#line 123 "css_keywords.tmpl"
      {"black", TokenType::BLACK},
#line 71 "css_keywords.tmpl"
      {"closest-corner", TokenType::CLOSEST_CORNER},
#line 228 "css_keywords.tmpl"
      {"papayawhip", TokenType::PAPAYAWHIP},
#line 296 "css_keywords.tmpl"
      {"border-bottom-width", TokenType::BORDER_BOTTOM_WIDTH},
#line 114 "css_keywords.tmpl"
      {"fit-content", TokenType::FIT_CONTENT},
#line 304 "css_keywords.tmpl"
      {"margin-bottom", TokenType::MARGIN_BOTTOM},
#line 143 "css_keywords.tmpl"
      {"darkkhaki", TokenType::DARKKHAKI},
#line 42 "css_keywords.tmpl"
      {"ppx", TokenType::PPX},
#line 174 "css_keywords.tmpl"
      {"hotpink", TokenType::HOTPINK},
#line 215 "css_keywords.tmpl"
      {"moccasin", TokenType::MOCCASIN},
#line 289 "css_keywords.tmpl"
      {"max-width", TokenType::MAX_WIDTH},
#line 145 "css_keywords.tmpl"
      {"darkolivegreen", TokenType::DARKOLIVEGREEN},
#line 136 "css_keywords.tmpl"
      {"cyan", TokenType::CYAN},
#line 147 "css_keywords.tmpl"
      {"darkorchid", TokenType::DARKORCHID},
#line 298 "css_keywords.tmpl"
      {"border-right-color", TokenType::BORDER_RIGHT_COLOR},
#line 138 "css_keywords.tmpl"
      {"darkcyan", TokenType::DARKCYAN},
#line 106 "css_keywords.tmpl"
      {"env", TokenType::ENV},
#line 299 "css_keywords.tmpl"
      {"border-top-color", TokenType::BORDER_TOP_COLOR},
#line 69 "css_keywords.tmpl"
      {"conic-gradient", TokenType::CONIC_GRADIENT},
#line 290 "css_keywords.tmpl"
      {"max-height", TokenType::MAX_HEIGHT},
#line 312 "css_keywords.tmpl"
      {"border-color", TokenType::BORDER_COLOR},
#line 195 "css_keywords.tmpl"
      {"lightslategrey", TokenType::LIGHTSLATEGREY},
#line 166 "css_keywords.tmpl"
      {"ghostwhite", TokenType::GHOSTWHITE},
#line 194 "css_keywords.tmpl"
      {"lightslategray", TokenType::LIGHTSLATEGRAY},
#line 337 "css_keywords.tmpl"
      {"backwards", TokenType::BACKWARDS},
#line 249 "css_keywords.tmpl"
      {"snow", TokenType::SNOW},
#line 133 "css_keywords.tmpl"
      {"cornflowerblue", TokenType::CORNFLOWERBLUE},
#line 294 "css_keywords.tmpl"
      {"border-right-width", TokenType::BORDER_RIGHT_WIDTH},
#line 63 "css_keywords.tmpl"
      {"padding-box", TokenType::PADDING_BOX},
#line 186 "css_keywords.tmpl"
      {"lightgoldenrodyellow", TokenType::LIGHTGOLDENRODYELLOW},
#line 177 "css_keywords.tmpl"
      {"ivory", TokenType::IVORY},
#line 62 "css_keywords.tmpl"
      {"border-box", TokenType::BORDER_BOX},
#line 57 "css_keywords.tmpl"
      {"squircle", TokenType::SQUIRCLE},
#line 173 "css_keywords.tmpl"
      {"honeydew", TokenType::HONEYDEW},
#line 64 "css_keywords.tmpl"
      {"content-box", TokenType::CONTENT_BOX},
#line 216 "css_keywords.tmpl"
      {"navajowhite", TokenType::NAVAJOWHITE},
#line 189 "css_keywords.tmpl"
      {"lightgrey", TokenType::LIGHTGREY},
#line 154 "css_keywords.tmpl"
      {"darkturquoise", TokenType::DARKTURQUOISE},
#line 187 "css_keywords.tmpl"
      {"lightgray", TokenType::LIGHTGRAY},
#line 265 "css_keywords.tmpl"
      {"rotatey", TokenType::ROTATE_Y},
#line 105 "css_keywords.tmpl"
      {"calc", TokenType::CALC},
#line 134 "css_keywords.tmpl"
      {"cornsilk", TokenType::CORNSILK},
#line 236 "css_keywords.tmpl"
      {"rosybrown", TokenType::ROSYBROWN},
#line 162 "css_keywords.tmpl"
      {"floralwhite", TokenType::FLORALWHITE},
#line 107 "css_keywords.tmpl"
      {"grayscale", TokenType::GRAYSCALE},
#line 157 "css_keywords.tmpl"
      {"deepskyblue", TokenType::DEEPSKYBLUE},
#line 317 "css_keywords.tmpl"
      {"background-position", TokenType::BACKGROUND_POSITION},
#line 295 "css_keywords.tmpl"
      {"border-top-width", TokenType::BORDER_TOP_WIDTH},
#line 273 "css_keywords.tmpl"
      {"scalex", TokenType::SCALE_X},
#line 51 "css_keywords.tmpl"
      {"repeat-x", TokenType::REPEAT_X},
#line 309 "css_keywords.tmpl"
      {"flex-basis", TokenType::FLEX_BASIS},
#line 164 "css_keywords.tmpl"
      {"fuchsia", TokenType::FUCHSIA},
#line 286 "css_keywords.tmpl"
      {"visibility", TokenType::VISIBILITY},
#line 336 "css_keywords.tmpl"
      {"forwards", TokenType::FORWARDS},
#line 328 "css_keywords.tmpl"
      {"square-bezier", TokenType::SQUARE_BEZIER},
#line 329 "css_keywords.tmpl"
      {"cubic-bezier", TokenType::CUBIC_BEZIER},
#line 171 "css_keywords.tmpl"
      {"greenyellow", TokenType::GREENYELLOW},
#line 262 "css_keywords.tmpl"
      {"yellowgreen", TokenType::YELLOWGREEN},
#line 276 "css_keywords.tmpl"
      {"skewx", TokenType::SKEW_X},
#line 245 "css_keywords.tmpl"
      {"skyblue", TokenType::SKYBLUE},
#line 311 "css_keywords.tmpl"
      {"border-width", TokenType::BORDER_WIDTH},
#line 185 "css_keywords.tmpl"
      {"lightcyan", TokenType::LIGHTCYAN},
#line 297 "css_keywords.tmpl"
      {"border-left-color", TokenType::BORDER_LEFT_COLOR},
#line 172 "css_keywords.tmpl"
      {"grey", TokenType::GREY},
#line 193 "css_keywords.tmpl"
      {"lightskyblue", TokenType::LIGHTSKYBLUE},
#line 169 "css_keywords.tmpl"
      {"gray", TokenType::GRAY},
#line 197 "css_keywords.tmpl"
      {"lightyellow", TokenType::LIGHTYELLOW},
#line 275 "css_keywords.tmpl"
      {"skew", TokenType::SKEW},
#line 161 "css_keywords.tmpl"
      {"firebrick", TokenType::FIREBRICK},
#line 159 "css_keywords.tmpl"
      {"dimgrey", TokenType::DIMGREY},
#line 158 "css_keywords.tmpl"
      {"dimgray", TokenType::DIMGRAY},
#line 39 "css_keywords.tmpl"
      {"vw", TokenType::VW},
#line 277 "css_keywords.tmpl"
      {"skewy", TokenType::SKEW_Y},
#line 316 "css_keywords.tmpl"
      {"box-shadow", TokenType::BOX_SHADOW},
#line 284 "css_keywords.tmpl"
      {"background-color", TokenType::BACKGROUND_COLOR},
#line 341 "css_keywords.tmpl"
      {"-infinity", TokenType::NEGATIVE_INFINITY},
#line 261 "css_keywords.tmpl"
      {"yellow", TokenType::YELLOW},
#line 293 "css_keywords.tmpl"
      {"border-left-width", TokenType::BORDER_LEFT_WIDTH},
#line 153 "css_keywords.tmpl"
      {"darkslategrey", TokenType::DARKSLATEGREY},
#line 319 "css_keywords.tmpl"
      {"offset-distance", TokenType::OFFSET_DISTANCE},
#line 152 "css_keywords.tmpl"
      {"darkslategray", TokenType::DARKSLATEGRAY},
#line 142 "css_keywords.tmpl"
      {"darkgrey", TokenType::DARKGREY},
#line 140 "css_keywords.tmpl"
      {"darkgray", TokenType::DARKGRAY},
#line 274 "css_keywords.tmpl"
      {"scaley", TokenType::SCALE_Y},
#line 52 "css_keywords.tmpl"
      {"repeat-y", TokenType::REPEAT_Y},
#line 280 "css_keywords.tmpl"
      {"opacity", TokenType::OPACITY},
#line 217 "css_keywords.tmpl"
      {"navy", TokenType::NAVY},
#line 348 "css_keywords.tmpl"
      {"off", TokenType::OFF},
#line 310 "css_keywords.tmpl"
      {"flex-grow", TokenType::FLEX_GROW},
#line 340 "css_keywords.tmpl"
      {"infinity", TokenType::INFINITY_TOKEN},
#line 229 "css_keywords.tmpl"
      {"peachpuff", TokenType::PEACHPUFF},
#line 281 "css_keywords.tmpl"
      {"scalexy", TokenType::SCALE_XY},
#line 93 "css_keywords.tmpl"
      {"wavy", TokenType::WAVY}};
#if (defined __GNUC__ && __GNUC__ + (__GNUC_MINOR__ >= 6) > 4) || \
    (defined __clang__ && __clang_major__ >= 3)
#pragma GCC diagnostic pop
#endif

  static const short lookup[] = {
      -1,  -1,  0,   -1,  -1,  -1,  1,   2,   3,   4,   -1,  -1,  5,   -1,  6,
      -1,  -1,  -1,  7,   -1,  -1,  -1,  -1,  -1,  8,   -1,  -1,  -1,  -1,  -1,
      -1,  9,   10,  -1,  11,  12,  13,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  14,
      15,  -1,  -1,  -1,  -1,  -1,  16,  -1,  17,  -1,  -1,  18,  -1,  19,  -1,
      -1,  20,  -1,  -1,  21,  22,  23,  -1,  -1,  24,  -1,  25,  -1,  -1,  26,
      -1,  27,  -1,  -1,  -1,  28,  -1,  29,  -1,  -1,  30,  31,  -1,  -1,  -1,
      -1,  -1,  32,  -1,  -1,  33,  34,  35,  -1,  36,  -1,  37,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  38,  -1,  39,  -1,  40,  41,  -1,  -1,  -1,  -1,  42,
      -1,  43,  -1,  44,  45,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  46,  47,
      -1,  -1,  48,  49,  -1,  50,  51,  -1,  52,  53,  -1,  -1,  -1,  54,  55,
      -1,  -1,  -1,  56,  57,  58,  59,  60,  -1,  61,  -1,  -1,  -1,  -1,  62,
      -1,  -1,  -1,  -1,  -1,  -1,  63,  64,  65,  -1,  -1,  66,  -1,  67,  -1,
      -1,  68,  -1,  69,  -1,  70,  -1,  71,  -1,  72,  73,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  74,  75,  76,  -1,  -1,  77,  -1,  -1,  78,  -1,  -1,
      -1,  -1,  -1,  -1,  79,  80,  81,  -1,  82,  83,  -1,  -1,  84,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  85,  86,  87,  -1,  -1,  -1,  -1,
      -1,  88,  -1,  -1,  -1,  -1,  89,  -1,  -1,  -1,  -1,  90,  91,  -1,  -1,
      92,  -1,  -1,  -1,  -1,  93,  -1,  -1,  -1,  94,  -1,  -1,  -1,  -1,  95,
      96,  97,  -1,  -1,  98,  -1,  -1,  -1,  -1,  99,  -1,  100, -1,  -1,  -1,
      -1,  101, -1,  -1,  102, 103, -1,  -1,  -1,  104, -1,  -1,  -1,  -1,  105,
      106, -1,  -1,  -1,  107, 108, -1,  -1,  -1,  109, -1,  -1,  -1,  -1,  -1,
      110, -1,  111, -1,  112, 113, -1,  114, -1,  -1,  115, -1,  116, 117, -1,
      118, -1,  -1,  -1,  119, 120, -1,  121, -1,  -1,  -1,  122, 123, -1,  -1,
      124, 125, -1,  126, 127, -1,  -1,  128, -1,  -1,  129, 130, -1,  -1,  -1,
      131, -1,  -1,  -1,  -1,  -1,  -1,  -1,  132, -1,  -1,  -1,  -1,  133, 134,
      -1,  -1,  -1,  -1,  -1,  135, -1,  -1,  -1,  -1,  136, -1,  -1,  -1,  -1,
      -1,  -1,  -1,  137, -1,  138, -1,  139, -1,  -1,  -1,  -1,  -1,  140, 141,
      -1,  -1,  -1,  -1,  -1,  142, -1,  143, -1,  -1,  -1,  -1,  -1,  144, 145,
      -1,  -1,  -1,  -1,  -1,  -1,  146, -1,  -1,  -1,  147, -1,  148, 149, -1,
      150, -1,  -1,  -1,  -1,  -1,  151, -1,  152, 153, -1,  154, -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  155, 156, 157, -1,  -1,  158, -1,  159, -1,  160,
      -1,  -1,  -1,  161, -1,  162, -1,  163, -1,  -1,  164, -1,  -1,  -1,  -1,
      165, 166, -1,  167, -1,  -1,  168, -1,  169, -1,  -1,  170, -1,  -1,  171,
      172, -1,  173, 174, 175, -1,  176, -1,  177, 178, -1,  -1,  -1,  -1,  -1,
      179, -1,  -1,  -1,  -1,  180, 181, 182, 183, 184, 185, -1,  186, 187, -1,
      -1,  188, -1,  -1,  189, -1,  -1,  -1,  -1,  -1,  -1,  190, -1,  -1,  -1,
      191, 192, 193, -1,  -1,  -1,  -1,  -1,  -1,  -1,  194, -1,  -1,  -1,  195,
      196, -1,  -1,  -1,  197, 198, -1,  -1,  -1,  -1,  -1,  199, 200, 201, -1,
      -1,  -1,  -1,  202, -1,  203, -1,  -1,  -1,  204, -1,  -1,  -1,  -1,  -1,
      -1,  -1,  205, 206, 207, 208, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  209, -1,  -1,  -1,  -1,  -1,  210, -1,  -1,  211, -1,  -1,  -1,
      -1,  212, -1,  -1,  -1,  -1,  213, -1,  -1,  -1,  -1,  214, -1,  -1,  -1,
      -1,  -1,  -1,  215, -1,  -1,  216, -1,  -1,  217, -1,  -1,  218, 219, -1,
      -1,  220, 221, -1,  222, -1,  -1,  -1,  -1,  223, -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  224, -1,  225, 226, 227, -1,  -1,  228, 229,
      -1,  -1,  230, -1,  -1,  -1,  -1,  -1,  -1,  -1,  231, -1,  -1,  -1,  232,
      -1,  -1,  -1,  -1,  233, -1,  -1,  -1,  234, -1,  235, -1,  -1,  -1,  236,
      -1,  -1,  -1,  237, 238, -1,  239, -1,  240, 241, -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  242, -1,  -1,  -1,  243, 244, 245, -1,  -1,  246, -1,  -1,
      -1,  -1,  -1,  -1,  247, -1,  -1,  -1,  -1,  -1,  248, -1,  -1,  -1,  -1,
      -1,  -1,  -1,  249, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  250, 251,
      -1,  252, -1,  -1,  -1,  -1,  -1,  -1,  -1,  253, -1,  -1,  -1,  -1,  -1,
      254, -1,  255, -1,  256, -1,  -1,  -1,  257, 258, -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  259, 260, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  261, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  262, -1,
      -1,  263, -1,  264, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  265, -1,  266, -1,  -1,  267, 268,
      -1,  -1,  -1,  -1,  -1,  -1,  269, 270, -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  271, -1,  -1,  -1,  272, 273, -1,  -1,  274, -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  275, -1,  -1,  -1,  276, -1,
      -1,  -1,  277, -1,  278, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  279,
      -1,  -1,  -1,  -1,  -1,  -1,  280, -1,  -1,  281, -1,  -1,  -1,  -1,  282,
      -1,  283, -1,  284, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      285, -1,  286, 287, -1,  -1,  -1,  -1,  -1,  -1,  -1,  288, -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  289, -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  290, -1,  291, -1,  -1,  -1,  -1,  292, -1,  -1,  -1,  293, -1,
      -1,  -1,  294, -1,  -1,  295, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  296,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  297, -1,  298, -1,  -1,  299, -1,  300,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  301, 302, -1,  -1,  -1,  303, -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  304, -1,  -1,
      -1,  -1,  305, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  306, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  307, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  308, -1,
      -1,  309, -1,  -1,  310, -1,  -1,  -1,  -1,  -1,  -1,  -1,  311, -1,  -1,
      312, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  313, -1,
      314, -1,  -1,  315, -1,  -1,  -1,  -1,  316, -1,  -1,  -1,  -1,  317, -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  318, -1,  319, -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  320, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  321, -1,  -1,
      -1,  -1,  -1,  322, -1,  -1,  -1,  323, -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  324, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  325, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  326, -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      327};

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
  return static_cast<struct TokenValue *>(0);
}
#line 350 "css_keywords.tmpl"

const struct TokenValue *GetTokenValue(const char *str, unsigned len) {
  return CSSKeywordsHash::GetTokenValue(str, len);
}
}  // namespace tasm
}  // namespace lynx
// NOLINTEND(modernize-use-nullptr)
