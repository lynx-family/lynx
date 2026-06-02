/* C++ code produced by gperf version 3.3 */
/* Command-line: gperf -m 100 -D -t --output-file css_keywords.cc
 * css_keywords.tmpl  */
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

#line 9 "css_keywords.tmpl"

// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/css_keywords.h"

#include <cstring>

// NOLINTBEGIN(modernize-use-nullptr)
namespace lynx {
namespace tasm {
#line 22 "css_keywords.tmpl"
struct TokenValue;
enum {
  TOTAL_KEYWORDS = 320,
  MIN_WORD_LENGTH = 1,
  MAX_WORD_LENGTH = 20,
  MIN_HASH_VALUE = 24,
  MAX_HASH_VALUE = 1122
};

/* maximum key range = 1099, duplicates = 0 */

class CSSKeywordsHash {
 private:
  static inline unsigned int hash(const char* str, size_t len);

 public:
  static const struct TokenValue* GetTokenValue(const char* str, size_t len);
};

inline unsigned int CSSKeywordsHash::hash(const char* str, size_t len) {
  static const unsigned short asso_values[] = {
      1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123,
      1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123,
      1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123,
      1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 50,   1123, 1123,
      1123, 1123, 1123, 7,    1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123,
      1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123,
      1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123,
      1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123,
      1123, 8,    99,   277,  21,   7,    332,  97,   181,  11,   1123, 186,
      7,    44,   8,    10,   40,   170,  8,    61,   7,    77,   469,  204,
      307,  167,  8,    1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123,
      1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123,
      1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123,
      1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123,
      1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123,
      1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123,
      1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123,
      1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123,
      1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123,
      1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123,
      1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123, 1123,
      1123, 1123, 1123, 1123};
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

struct stringpool_t {
  char stringpool_str0[sizeof("at")];
  char stringpool_str1[sizeof("on")];
  char stringpool_str2[sizeof("to")];
  char stringpool_str3[sizeof("all")];
  char stringpool_str4[sizeof("tan")];
  char stringpool_str5[sizeof("teal")];
  char stringpool_str6[sizeof("none")];
  char stringpool_str7[sizeof("linen")];
  char stringpool_str8[sizeof("toleft")];
  char stringpool_str9[sizeof("rotate")];
  char stringpool_str10[sizeof("orange")];
  char stringpool_str11[sizeof("linear")];
  char stringpool_str12[sizeof("data")];
  char stringpool_str13[sizeof("red")];
  char stringpool_str14[sizeof("rad")];
  char stringpool_str15[sizeof("rotatez")];
  char stringpool_str16[sizeof("alternate")];
  char stringpool_str17[sizeof("translate")];
  char stringpool_str18[sizeof("translatez")];
  char stringpool_str19[sizeof("line-range")];
  char stringpool_str20[sizeof("indigo")];
  char stringpool_str21[sizeof("totop")];
  char stringpool_str22[sizeof("lime")];
  char stringpool_str23[sizeof("repeat")];
  char stringpool_str24[sizeof("translate3d")];
  char stringpool_str25[sizeof("orangered")];
  char stringpool_str26[sizeof("normal")];
  char stringpool_str27[sizeof("maroon")];
  char stringpool_str28[sizeof("dotted")];
  char stringpool_str29[sizeof("ease")];
  char stringpool_str30[sizeof("tomato")];
  char stringpool_str31[sizeof("em")];
  char stringpool_str32[sizeof("inset")];
  char stringpool_str33[sizeof("top")];
  char stringpool_str34[sizeof("palegreen")];
  char stringpool_str35[sizeof("url")];
  char stringpool_str36[sizeof("indianred")];
  char stringpool_str37[sizeof("transparent")];
  char stringpool_str38[sizeof("rem")];
  char stringpool_str39[sizeof("limegreen")];
  char stringpool_str40[sizeof("sienna")];
  char stringpool_str41[sizeof("true")];
  char stringpool_str42[sizeof("turn")];
  char stringpool_str43[sizeof("azure")];
  char stringpool_str44[sizeof("ellipse")];
  char stringpool_str45[sizeof("solid")];
  char stringpool_str46[sizeof("auto")];
  char stringpool_str47[sizeof("ease-in")];
  char stringpool_str48[sizeof("s")];
  char stringpool_str49[sizeof("round")];
  char stringpool_str50[sizeof("green")];
  char stringpool_str51[sizeof("salmon")];
  char stringpool_str52[sizeof("sp")];
  char stringpool_str53[sizeof("groove")];
  char stringpool_str54[sizeof("mintcream")];
  char stringpool_str55[sizeof("no-repeat")];
  char stringpool_str56[sizeof("ridge")];
  char stringpool_str57[sizeof("palevioletred")];
  char stringpool_str58[sizeof("underline")];
  char stringpool_str59[sizeof("grad")];
  char stringpool_str60[sizeof("gold")];
  char stringpool_str61[sizeof("bold")];
  char stringpool_str62[sizeof("palegoldenrod")];
  char stringpool_str63[sizeof("ms")];
  char stringpool_str64[sizeof("ease-in-out")];
  char stringpool_str65[sizeof("seashell")];
  char stringpool_str66[sizeof("outset")];
  char stringpool_str67[sizeof("medium")];
  char stringpool_str68[sizeof("margin")];
  char stringpool_str69[sizeof("step-end")];
  char stringpool_str70[sizeof("steps")];
  char stringpool_str71[sizeof("purple")];
  char stringpool_str72[sizeof("magenta")];
  char stringpool_str73[sizeof("saturate")];
  char stringpool_str74[sizeof("goldenrod")];
  char stringpool_str75[sizeof("ease-out")];
  char stringpool_str76[sizeof("mediumorchid")];
  char stringpool_str77[sizeof("blue")];
  char stringpool_str78[sizeof("blur")];
  char stringpool_str79[sizeof("step-start")];
  char stringpool_str80[sizeof("seagreen")];
  char stringpool_str81[sizeof("peru")];
  char stringpool_str82[sizeof("plum")];
  char stringpool_str83[sizeof("bottom")];
  char stringpool_str84[sizeof("mistyrose")];
  char stringpool_str85[sizeof("thin")];
  char stringpool_str86[sizeof("border-area")];
  char stringpool_str87[sizeof("rgba")];
  char stringpool_str88[sizeof("deg")];
  char stringpool_str89[sizeof("beige")];
  char stringpool_str90[sizeof("double")];
  char stringpool_str91[sizeof("linear-gradient")];
  char stringpool_str92[sizeof("translatey")];
  char stringpool_str93[sizeof("mediumseagreen")];
  char stringpool_str94[sizeof("ease-in-ease-out")];
  char stringpool_str95[sizeof("sandybrown")];
  char stringpool_str96[sizeof("paused")];
  char stringpool_str97[sizeof("blueviolet")];
  char stringpool_str98[sizeof("toright")];
  char stringpool_str99[sizeof("tobottom")];
  char stringpool_str100[sizeof("margin-left")];
  char stringpool_str101[sizeof("radial-gradient")];
  char stringpool_str102[sizeof("saddlebrown")];
  char stringpool_str103[sizeof("min-height")];
  char stringpool_str104[sizeof("margin-right")];
  char stringpool_str105[sizeof("mediumblue")];
  char stringpool_str106[sizeof("mediumturquoise")];
  char stringpool_str107[sizeof("hidden")];
  char stringpool_str108[sizeof("polygon")];
  char stringpool_str109[sizeof("hsl")];
  char stringpool_str110[sizeof("gainsboro")];
  char stringpool_str111[sizeof("darkgreen")];
  char stringpool_str112[sizeof("darkorange")];
  char stringpool_str113[sizeof("lawngreen")];
  char stringpool_str114[sizeof("darkviolet")];
  char stringpool_str115[sizeof("hsla")];
  char stringpool_str116[sizeof("mediumpurple")];
  char stringpool_str117[sizeof("padding-left")];
  char stringpool_str118[sizeof("margin-top")];
  char stringpool_str119[sizeof("aqua")];
  char stringpool_str120[sizeof("darkred")];
  char stringpool_str121[sizeof("dodgerblue")];
  char stringpool_str122[sizeof("steelblue")];
  char stringpool_str123[sizeof("slateblue")];
  char stringpool_str124[sizeof("border-right-color")];
  char stringpool_str125[sizeof("thistle")];
  char stringpool_str126[sizeof("padding-top")];
  char stringpool_str127[sizeof("padding")];
  char stringpool_str128[sizeof("darksalmon")];
  char stringpool_str129[sizeof("rgb")];
  char stringpool_str130[sizeof("aquamarine")];
  char stringpool_str131[sizeof("brightness")];
  char stringpool_str132[sizeof("right")];
  char stringpool_str133[sizeof("running")];
  char stringpool_str134[sizeof("local")];
  char stringpool_str135[sizeof("coral")];
  char stringpool_str136[sizeof("height")];
  char stringpool_str137[sizeof("color")];
  char stringpool_str138[sizeof("dashed")];
  char stringpool_str139[sizeof("padding-bottom")];
  char stringpool_str140[sizeof("center")];
  char stringpool_str141[sizeof("mediumslateblue")];
  char stringpool_str142[sizeof("darkgoldenrod")];
  char stringpool_str143[sizeof("darkblue")];
  char stringpool_str144[sizeof("brown")];
  char stringpool_str145[sizeof("contain")];
  char stringpool_str146[sizeof("super-ellipse")];
  char stringpool_str147[sizeof("text")];
  char stringpool_str148[sizeof("springgreen")];
  char stringpool_str149[sizeof("min-width")];
  char stringpool_str150[sizeof("oldlace")];
  char stringpool_str151[sizeof("mediumaquamarine")];
  char stringpool_str152[sizeof("antiquewhite")];
  char stringpool_str153[sizeof("darkslateblue")];
  char stringpool_str154[sizeof("fr")];
  char stringpool_str155[sizeof("darkmagenta")];
  char stringpool_str156[sizeof("hue-rotate")];
  char stringpool_str157[sizeof("turquoise")];
  char stringpool_str158[sizeof("bisque")];
  char stringpool_str159[sizeof("darkseagreen")];
  char stringpool_str160[sizeof("left")];
  char stringpool_str161[sizeof("scale")];
  char stringpool_str162[sizeof("translatex")];
  char stringpool_str163[sizeof("slategrey")];
  char stringpool_str164[sizeof("slategray")];
  char stringpool_str165[sizeof("crimson")];
  char stringpool_str166[sizeof("border-bottom-color")];
  char stringpool_str167[sizeof("filter")];
  char stringpool_str168[sizeof("rotatey")];
  char stringpool_str169[sizeof("blanchedalmond")];
  char stringpool_str170[sizeof("lightsalmon")];
  char stringpool_str171[sizeof("royalblue")];
  char stringpool_str172[sizeof("contrast")];
  char stringpool_str173[sizeof("midnightblue")];
  char stringpool_str174[sizeof("space")];
  char stringpool_str175[sizeof("infinite")];
  char stringpool_str176[sizeof("paleturquoise")];
  char stringpool_str177[sizeof("powderblue")];
  char stringpool_str178[sizeof("lightseagreen")];
  char stringpool_str179[sizeof("wheat")];
  char stringpool_str180[sizeof("format")];
  char stringpool_str181[sizeof("white")];
  char stringpool_str182[sizeof("false")];
  char stringpool_str183[sizeof("path")];
  char stringpool_str184[sizeof("darkcyan")];
  char stringpool_str185[sizeof("transform-origin")];
  char stringpool_str186[sizeof("margin-bottom")];
  char stringpool_str187[sizeof("lightgreen")];
  char stringpool_str188[sizeof("width")];
  char stringpool_str189[sizeof("matrix3d")];
  char stringpool_str190[sizeof("transform")];
  char stringpool_str191[sizeof("pink")];
  char stringpool_str192[sizeof("from")];
  char stringpool_str193[sizeof("fit-content")];
  char stringpool_str194[sizeof("burlywood")];
  char stringpool_str195[sizeof("mediumspringgreen")];
  char stringpool_str196[sizeof("grey")];
  char stringpool_str197[sizeof("gray")];
  char stringpool_str198[sizeof("max-content")];
  char stringpool_str199[sizeof("padding-right")];
  char stringpool_str200[sizeof("border-right-width")];
  char stringpool_str201[sizeof("repeat-y")];
  char stringpool_str202[sizeof("farthest-corner")];
  char stringpool_str203[sizeof("cyan")];
  char stringpool_str204[sizeof("deeppink")];
  char stringpool_str205[sizeof("lightsteelblue")];
  char stringpool_str206[sizeof("farthest-side")];
  char stringpool_str207[sizeof("both")];
  char stringpool_str208[sizeof("rosybrown")];
  char stringpool_str209[sizeof("forestgreen")];
  char stringpool_str210[sizeof("snow")];
  char stringpool_str211[sizeof("lightblue")];
  char stringpool_str212[sizeof("var")];
  char stringpool_str213[sizeof("conic-gradient")];
  char stringpool_str214[sizeof("closest-side")];
  char stringpool_str215[sizeof("border-color")];
  char stringpool_str216[sizeof("aliceblue")];
  char stringpool_str217[sizeof("square-bezier")];
  char stringpool_str218[sizeof("olive")];
  char stringpool_str219[sizeof("closest-corner")];
  char stringpool_str220[sizeof("cadetblue")];
  char stringpool_str221[sizeof("border-top-color")];
  char stringpool_str222[sizeof("greenyellow")];
  char stringpool_str223[sizeof("yellowgreen")];
  char stringpool_str224[sizeof("violet")];
  char stringpool_str225[sizeof("dimgrey")];
  char stringpool_str226[sizeof("dimgray")];
  char stringpool_str227[sizeof("orchid")];
  char stringpool_str228[sizeof("whitesmoke")];
  char stringpool_str229[sizeof("papayawhip")];
  char stringpool_str230[sizeof("max-height")];
  char stringpool_str231[sizeof("lavender")];
  char stringpool_str232[sizeof("deepskyblue")];
  char stringpool_str233[sizeof("border-bottom-width")];
  char stringpool_str234[sizeof("lightpink")];
  char stringpool_str235[sizeof("alternate-reverse")];
  char stringpool_str236[sizeof("border-left-color")];
  char stringpool_str237[sizeof("padding-box")];
  char stringpool_str238[sizeof("lemonchiffon")];
  char stringpool_str239[sizeof("darkturquoise")];
  char stringpool_str240[sizeof("lightslategrey")];
  char stringpool_str241[sizeof("lightslategray")];
  char stringpool_str242[sizeof("silver")];
  char stringpool_str243[sizeof("reverse")];
  char stringpool_str244[sizeof("khaki")];
  char stringpool_str245[sizeof("darkgrey")];
  char stringpool_str246[sizeof("darkgray")];
  char stringpool_str247[sizeof("black")];
  char stringpool_str248[sizeof("chartreuse")];
  char stringpool_str249[sizeof("lightgrey")];
  char stringpool_str250[sizeof("lightgray")];
  char stringpool_str251[sizeof("grayscale")];
  char stringpool_str252[sizeof("circle")];
  char stringpool_str253[sizeof("darkslategrey")];
  char stringpool_str254[sizeof("darkslategray")];
  char stringpool_str255[sizeof("yellow")];
  char stringpool_str256[sizeof("line-through")];
  char stringpool_str257[sizeof("lightcoral")];
  char stringpool_str258[sizeof("skyblue")];
  char stringpool_str259[sizeof("border-box")];
  char stringpool_str260[sizeof("border-top-width")];
  char stringpool_str261[sizeof("darkkhaki")];
  char stringpool_str262[sizeof("hotpink")];
  char stringpool_str263[sizeof("cornflowerblue")];
  char stringpool_str264[sizeof("skewy")];
  char stringpool_str265[sizeof("max-width")];
  char stringpool_str266[sizeof("olivedrab")];
  char stringpool_str267[sizeof("cubic-bezier")];
  char stringpool_str268[sizeof("honeydew")];
  char stringpool_str269[sizeof("lightgoldenrodyellow")];
  char stringpool_str270[sizeof("border-left-width")];
  char stringpool_str271[sizeof("px")];
  char stringpool_str272[sizeof("mediumvioletred")];
  char stringpool_str273[sizeof("rotatex")];
  char stringpool_str274[sizeof("rpx")];
  char stringpool_str275[sizeof("skew")];
  char stringpool_str276[sizeof("thick")];
  char stringpool_str277[sizeof("ivory")];
  char stringpool_str278[sizeof("backwards")];
  char stringpool_str279[sizeof("opacity")];
  char stringpool_str280[sizeof("content-box")];
  char stringpool_str281[sizeof("matrix")];
  char stringpool_str282[sizeof("lightyellow")];
  char stringpool_str283[sizeof("scaley")];
  char stringpool_str284[sizeof("ppx")];
  char stringpool_str285[sizeof("cornsilk")];
  char stringpool_str286[sizeof("moccasin")];
  char stringpool_str287[sizeof("forwards")];
  char stringpool_str288[sizeof("darkorchid")];
  char stringpool_str289[sizeof("infinity")];
  char stringpool_str290[sizeof("lightskyblue")];
  char stringpool_str291[sizeof("repeat-x")];
  char stringpool_str292[sizeof("background-position")];
  char stringpool_str293[sizeof("darkolivegreen")];
  char stringpool_str294[sizeof("visibility")];
  char stringpool_str295[sizeof("ghostwhite")];
  char stringpool_str296[sizeof("lightcyan")];
  char stringpool_str297[sizeof("floralwhite")];
  char stringpool_str298[sizeof("skewx")];
  char stringpool_str299[sizeof("border-width")];
  char stringpool_str300[sizeof("cover")];
  char stringpool_str301[sizeof("chocolate")];
  char stringpool_str302[sizeof("navy")];
  char stringpool_str303[sizeof("vh")];
  char stringpool_str304[sizeof("firebrick")];
  char stringpool_str305[sizeof("calc")];
  char stringpool_str306[sizeof("offset-distance")];
  char stringpool_str307[sizeof("vw")];
  char stringpool_str308[sizeof("flex-basis")];
  char stringpool_str309[sizeof("fuchsia")];
  char stringpool_str310[sizeof("navajowhite")];
  char stringpool_str311[sizeof("env")];
  char stringpool_str312[sizeof("lavenderblush")];
  char stringpool_str313[sizeof("scalex")];
  char stringpool_str314[sizeof("background-color")];
  char stringpool_str315[sizeof("flex-grow")];
  char stringpool_str316[sizeof("scalexy")];
  char stringpool_str317[sizeof("off")];
  char stringpool_str318[sizeof("wavy")];
  char stringpool_str319[sizeof("peachpuff")];
};
static const struct stringpool_t stringpool_contents = {"at",
                                                        "on",
                                                        "to",
                                                        "all",
                                                        "tan",
                                                        "teal",
                                                        "none",
                                                        "linen",
                                                        "toleft",
                                                        "rotate",
                                                        "orange",
                                                        "linear",
                                                        "data",
                                                        "red",
                                                        "rad",
                                                        "rotatez",
                                                        "alternate",
                                                        "translate",
                                                        "translatez",
                                                        "line-range",
                                                        "indigo",
                                                        "totop",
                                                        "lime",
                                                        "repeat",
                                                        "translate3d",
                                                        "orangered",
                                                        "normal",
                                                        "maroon",
                                                        "dotted",
                                                        "ease",
                                                        "tomato",
                                                        "em",
                                                        "inset",
                                                        "top",
                                                        "palegreen",
                                                        "url",
                                                        "indianred",
                                                        "transparent",
                                                        "rem",
                                                        "limegreen",
                                                        "sienna",
                                                        "true",
                                                        "turn",
                                                        "azure",
                                                        "ellipse",
                                                        "solid",
                                                        "auto",
                                                        "ease-in",
                                                        "s",
                                                        "round",
                                                        "green",
                                                        "salmon",
                                                        "sp",
                                                        "groove",
                                                        "mintcream",
                                                        "no-repeat",
                                                        "ridge",
                                                        "palevioletred",
                                                        "underline",
                                                        "grad",
                                                        "gold",
                                                        "bold",
                                                        "palegoldenrod",
                                                        "ms",
                                                        "ease-in-out",
                                                        "seashell",
                                                        "outset",
                                                        "medium",
                                                        "margin",
                                                        "step-end",
                                                        "steps",
                                                        "purple",
                                                        "magenta",
                                                        "saturate",
                                                        "goldenrod",
                                                        "ease-out",
                                                        "mediumorchid",
                                                        "blue",
                                                        "blur",
                                                        "step-start",
                                                        "seagreen",
                                                        "peru",
                                                        "plum",
                                                        "bottom",
                                                        "mistyrose",
                                                        "thin",
                                                        "border-area",
                                                        "rgba",
                                                        "deg",
                                                        "beige",
                                                        "double",
                                                        "linear-gradient",
                                                        "translatey",
                                                        "mediumseagreen",
                                                        "ease-in-ease-out",
                                                        "sandybrown",
                                                        "paused",
                                                        "blueviolet",
                                                        "toright",
                                                        "tobottom",
                                                        "margin-left",
                                                        "radial-gradient",
                                                        "saddlebrown",
                                                        "min-height",
                                                        "margin-right",
                                                        "mediumblue",
                                                        "mediumturquoise",
                                                        "hidden",
                                                        "polygon",
                                                        "hsl",
                                                        "gainsboro",
                                                        "darkgreen",
                                                        "darkorange",
                                                        "lawngreen",
                                                        "darkviolet",
                                                        "hsla",
                                                        "mediumpurple",
                                                        "padding-left",
                                                        "margin-top",
                                                        "aqua",
                                                        "darkred",
                                                        "dodgerblue",
                                                        "steelblue",
                                                        "slateblue",
                                                        "border-right-color",
                                                        "thistle",
                                                        "padding-top",
                                                        "padding",
                                                        "darksalmon",
                                                        "rgb",
                                                        "aquamarine",
                                                        "brightness",
                                                        "right",
                                                        "running",
                                                        "local",
                                                        "coral",
                                                        "height",
                                                        "color",
                                                        "dashed",
                                                        "padding-bottom",
                                                        "center",
                                                        "mediumslateblue",
                                                        "darkgoldenrod",
                                                        "darkblue",
                                                        "brown",
                                                        "contain",
                                                        "super-ellipse",
                                                        "text",
                                                        "springgreen",
                                                        "min-width",
                                                        "oldlace",
                                                        "mediumaquamarine",
                                                        "antiquewhite",
                                                        "darkslateblue",
                                                        "fr",
                                                        "darkmagenta",
                                                        "hue-rotate",
                                                        "turquoise",
                                                        "bisque",
                                                        "darkseagreen",
                                                        "left",
                                                        "scale",
                                                        "translatex",
                                                        "slategrey",
                                                        "slategray",
                                                        "crimson",
                                                        "border-bottom-color",
                                                        "filter",
                                                        "rotatey",
                                                        "blanchedalmond",
                                                        "lightsalmon",
                                                        "royalblue",
                                                        "contrast",
                                                        "midnightblue",
                                                        "space",
                                                        "infinite",
                                                        "paleturquoise",
                                                        "powderblue",
                                                        "lightseagreen",
                                                        "wheat",
                                                        "format",
                                                        "white",
                                                        "false",
                                                        "path",
                                                        "darkcyan",
                                                        "transform-origin",
                                                        "margin-bottom",
                                                        "lightgreen",
                                                        "width",
                                                        "matrix3d",
                                                        "transform",
                                                        "pink",
                                                        "from",
                                                        "fit-content",
                                                        "burlywood",
                                                        "mediumspringgreen",
                                                        "grey",
                                                        "gray",
                                                        "max-content",
                                                        "padding-right",
                                                        "border-right-width",
                                                        "repeat-y",
                                                        "farthest-corner",
                                                        "cyan",
                                                        "deeppink",
                                                        "lightsteelblue",
                                                        "farthest-side",
                                                        "both",
                                                        "rosybrown",
                                                        "forestgreen",
                                                        "snow",
                                                        "lightblue",
                                                        "var",
                                                        "conic-gradient",
                                                        "closest-side",
                                                        "border-color",
                                                        "aliceblue",
                                                        "square-bezier",
                                                        "olive",
                                                        "closest-corner",
                                                        "cadetblue",
                                                        "border-top-color",
                                                        "greenyellow",
                                                        "yellowgreen",
                                                        "violet",
                                                        "dimgrey",
                                                        "dimgray",
                                                        "orchid",
                                                        "whitesmoke",
                                                        "papayawhip",
                                                        "max-height",
                                                        "lavender",
                                                        "deepskyblue",
                                                        "border-bottom-width",
                                                        "lightpink",
                                                        "alternate-reverse",
                                                        "border-left-color",
                                                        "padding-box",
                                                        "lemonchiffon",
                                                        "darkturquoise",
                                                        "lightslategrey",
                                                        "lightslategray",
                                                        "silver",
                                                        "reverse",
                                                        "khaki",
                                                        "darkgrey",
                                                        "darkgray",
                                                        "black",
                                                        "chartreuse",
                                                        "lightgrey",
                                                        "lightgray",
                                                        "grayscale",
                                                        "circle",
                                                        "darkslategrey",
                                                        "darkslategray",
                                                        "yellow",
                                                        "line-through",
                                                        "lightcoral",
                                                        "skyblue",
                                                        "border-box",
                                                        "border-top-width",
                                                        "darkkhaki",
                                                        "hotpink",
                                                        "cornflowerblue",
                                                        "skewy",
                                                        "max-width",
                                                        "olivedrab",
                                                        "cubic-bezier",
                                                        "honeydew",
                                                        "lightgoldenrodyellow",
                                                        "border-left-width",
                                                        "px",
                                                        "mediumvioletred",
                                                        "rotatex",
                                                        "rpx",
                                                        "skew",
                                                        "thick",
                                                        "ivory",
                                                        "backwards",
                                                        "opacity",
                                                        "content-box",
                                                        "matrix",
                                                        "lightyellow",
                                                        "scaley",
                                                        "ppx",
                                                        "cornsilk",
                                                        "moccasin",
                                                        "forwards",
                                                        "darkorchid",
                                                        "infinity",
                                                        "lightskyblue",
                                                        "repeat-x",
                                                        "background-position",
                                                        "darkolivegreen",
                                                        "visibility",
                                                        "ghostwhite",
                                                        "lightcyan",
                                                        "floralwhite",
                                                        "skewx",
                                                        "border-width",
                                                        "cover",
                                                        "chocolate",
                                                        "navy",
                                                        "vh",
                                                        "firebrick",
                                                        "calc",
                                                        "offset-distance",
                                                        "vw",
                                                        "flex-basis",
                                                        "fuchsia",
                                                        "navajowhite",
                                                        "env",
                                                        "lavenderblush",
                                                        "scalex",
                                                        "background-color",
                                                        "flex-grow",
                                                        "scalexy",
                                                        "off",
                                                        "wavy",
                                                        "peachpuff"};
#define stringpool ((const char*)&stringpool_contents)

#if (defined __GNUC__ && __GNUC__ + (__GNUC_MINOR__ >= 6) > 4) || \
    (defined __clang__ && __clang_major__ >= 3)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
static const struct TokenValue token_list[] = {
#line 74 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str0, TokenType::AT},
#line 341 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str1, TokenType::ON},
#line 30 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str2, TokenType::TO},
#line 284 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str3,
     TokenType::ALL},
#line 248 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str4,
     TokenType::TAN},
#line 249 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str5,
     TokenType::TEAL},
#line 29 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str6,
     TokenType::NONE},
#line 196 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str7,
     TokenType::LINEN},
#line 96 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str8,
     TokenType::TOLEFT},
#line 259 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str9,
     TokenType::ROTATE},
#line 217 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str10,
     TokenType::ORANGE},
#line 315 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str11,
     TokenType::LINEAR},
#line 75 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str12,
     TokenType::DATA},
#line 231 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str13,
     TokenType::RED},
#line 48 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str14,
     TokenType::RAD},
#line 262 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str15,
     TokenType::ROTATE_Z},
#line 329 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str16,
     TokenType::ALTERNATE},
#line 263 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str17,
     TokenType::TRANSLATE},
#line 267 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str18,
     TokenType::TRANSLATE_Z},
#line 31 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str19,
     TokenType::LINE_RANGE},
#line 172 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str20,
     TokenType::INDIGO},
#line 98 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str21,
     TokenType::TOTOP},
#line 194 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str22,
     TokenType::LIME},
#line 55 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str23,
     TokenType::REPEAT},
#line 264 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str24,
     TokenType::TRANSLATE_3D},
#line 218 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str25,
     TokenType::ORANGERED},
#line 93 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str26,
     TokenType::NORMAL},
#line 198 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str27,
     TokenType::MAROON},
#line 80 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str28,
     TokenType::DOTTED},
#line 319 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str29,
     TokenType::EASE},
#line 251 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str30,
     TokenType::TOMATO},
#line 40 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str31,
     TokenType::EM},
#line 86 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str32,
     TokenType::INSET},
#line 33 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str33,
     TokenType::TOP},
#line 221 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str34,
     TokenType::PALEGREEN},
#line 28 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str35,
     TokenType::URL},
#line 171 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str36,
     TokenType::INDIANRED},
#line 111 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str37,
     TokenType::TRANSPARENT},
#line 39 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str38,
     TokenType::REM},
#line 195 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str39,
     TokenType::LIMEGREEN},
#line 239 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str40,
     TokenType::SIENNA},
#line 338 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str41,
     TokenType::TOKEN_TRUE},
#line 49 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str42,
     TokenType::TURN},
#line 116 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str43,
     TokenType::AZURE},
#line 71 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str44,
     TokenType::ELLIPSE},
#line 82 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str45,
     TokenType::SOLID},
#line 50 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str46,
     TokenType::AUTO},
#line 316 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str47,
     TokenType::EASE_IN},
#line 327 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str48,
     TokenType::SECOND},
#line 58 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str49,
     TokenType::ROUND},
#line 166 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str50,
     TokenType::GREEN},
#line 235 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str51,
     TokenType::SALMON},
#line 43 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str52,
     TokenType::SP},
#line 84 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str53,
     TokenType::GROOVE},
#line 209 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str54,
     TokenType::MINTCREAM},
#line 56 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str55,
     TokenType::NO_REPEAT},
#line 85 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str56,
     TokenType::RIDGE},
#line 223 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str57,
     TokenType::PALEVIOLETRED},
#line 88 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str58,
     TokenType::UNDERLINE},
#line 47 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str59,
     TokenType::GRAD},
#line 163 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str60,
     TokenType::GOLD},
#line 94 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str61,
     TokenType::BOLD},
#line 220 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str62,
     TokenType::PALEGOLDENROD},
#line 326 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str63,
     TokenType::MILLISECOND},
#line 320 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str64,
     TokenType::EASE_IN_OUT},
#line 238 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str65,
     TokenType::SEASHELL},
#line 87 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str66,
     TokenType::OUTSET},
#line 77 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str67,
     TokenType::MEDIUM},
#line 309 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str68,
     TokenType::MARGIN},
#line 322 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str69,
     TokenType::STEP_END},
#line 325 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str70,
     TokenType::STEPS},
#line 230 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str71,
     TokenType::PURPLE},
#line 197 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str72,
     TokenType::MAGENTA},
#line 106 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str73,
     TokenType::SATURATE},
#line 164 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str74,
     TokenType::GOLDENROD},
#line 317 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str75,
     TokenType::EASE_OUT},
#line 201 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str76,
     TokenType::MEDIUMORCHID},
#line 121 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str77,
     TokenType::BLUE},
#line 109 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str78,
     TokenType::BLUR},
#line 321 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str79,
     TokenType::STEP_START},
#line 237 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str80,
     TokenType::SEAGREEN},
#line 226 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str81,
     TokenType::PERU},
#line 228 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str82,
     TokenType::PLUM},
#line 35 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str83,
     TokenType::BOTTOM},
#line 210 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str84,
     TokenType::MISTYROSE},
#line 76 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str85,
     TokenType::THIN},
#line 63 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str86,
     TokenType::BORDER_AREA},
#line 25 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str87,
     TokenType::RGBA},
#line 46 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str88,
     TokenType::DEG},
#line 117 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str89,
     TokenType::BEIGE},
#line 83 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str90,
     TokenType::DOUBLE},
#line 64 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str91,
     TokenType::LINEAR_GRADIENT},
#line 266 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str92,
     TokenType::TRANSLATE_Y},
#line 203 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str93,
     TokenType::MEDIUMSEAGREEN},
#line 318 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str94,
     TokenType::EASE_IN_EASE_OUT},
#line 236 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str95,
     TokenType::SANDYBROWN},
#line 336 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str96,
     TokenType::PAUSED},
#line 122 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str97,
     TokenType::BLUEVIOLET},
#line 97 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str98,
     TokenType::TORIGHT},
#line 95 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str99,
     TokenType::TOBOTTOM},
#line 297 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str100,
     TokenType::MARGIN_LEFT},
#line 65 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str101,
     TokenType::RADIAL_GRADIENT},
#line 234 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str102,
     TokenType::SADDLEBROWN},
#line 288 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str103,
     TokenType::MIN_HEIGHT},
#line 298 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str104,
     TokenType::MARGIN_RIGHT},
#line 200 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str105,
     TokenType::MEDIUMBLUE},
#line 206 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str106,
     TokenType::MEDIUMTURQUOISE},
#line 79 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str107,
     TokenType::HIDDEN},
#line 73 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str108,
     TokenType::POLYGON},
#line 26 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str109,
     TokenType::HSL},
#line 161 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str110,
     TokenType::GAINSBORO},
#line 137 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str111,
     TokenType::DARKGREEN},
#line 142 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str112,
     TokenType::DARKORANGE},
#line 177 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str113,
     TokenType::LAWNGREEN},
#line 151 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str114,
     TokenType::DARKVIOLET},
#line 27 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str115,
     TokenType::HSLA},
#line 202 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str116,
     TokenType::MEDIUMPURPLE},
#line 301 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str117,
     TokenType::PADDING_LEFT},
#line 299 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str118,
     TokenType::MARGIN_TOP},
#line 114 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str119,
     TokenType::AQUA},
#line 144 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str120,
     TokenType::DARKRED},
#line 156 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str121,
     TokenType::DODGERBLUE},
#line 247 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str122,
     TokenType::STEELBLUE},
#line 242 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str123,
     TokenType::SLATEBLUE},
#line 294 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str124,
     TokenType::BORDER_RIGHT_COLOR},
#line 250 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str125,
     TokenType::THISTLE},
#line 303 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str126,
     TokenType::PADDING_TOP},
#line 310 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str127,
     TokenType::PADDING},
#line 145 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str128,
     TokenType::DARKSALMON},
#line 24 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str129,
     TokenType::RGB},
#line 115 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str130,
     TokenType::AQUAMARINE},
#line 104 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str131,
     TokenType::BRIGHTNESS},
#line 34 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str132,
     TokenType::RIGHT},
#line 337 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str133,
     TokenType::RUNNING},
#line 92 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str134,
     TokenType::LOCAL},
#line 128 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str135,
     TokenType::CORAL},
#line 279 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str136,
     TokenType::HEIGHT},
#line 281 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str137,
     TokenType::COLOR},
#line 81 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str138,
     TokenType::DASHED},
#line 304 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str139,
     TokenType::PADDING_BOTTOM},
#line 36 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str140,
     TokenType::CENTER},
#line 204 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str141,
     TokenType::MEDIUMSLATEBLUE},
#line 135 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str142,
     TokenType::DARKGOLDENROD},
#line 133 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str143,
     TokenType::DARKBLUE},
#line 123 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str144,
     TokenType::BROWN},
#line 52 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str145,
     TokenType::CONTAIN},
#line 100 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str146,
     TokenType::SUPER_ELLIPSE},
#line 62 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str147,
     TokenType::TEXT},
#line 246 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str148,
     TokenType::SPRINGGREEN},
#line 287 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str149,
     TokenType::MIN_WIDTH},
#line 214 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str150,
     TokenType::OLDLACE},
#line 199 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str151,
     TokenType::MEDIUMAQUAMARINE},
#line 113 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str152,
     TokenType::ANTIQUEWHITE},
#line 147 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str153,
     TokenType::DARKSLATEBLUE},
#line 340 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str154,
     TokenType::FR},
#line 140 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str155,
     TokenType::DARKMAGENTA},
#line 107 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str156,
     TokenType::HUE_ROTATE},
#line 252 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str157,
     TokenType::TURQUOISE},
#line 118 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str158,
     TokenType::BISQUE},
#line 146 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str159,
     TokenType::DARKSEAGREEN},
#line 32 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str160,
     TokenType::LEFT},
#line 268 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str161,
     TokenType::SCALE},
#line 265 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str162,
     TokenType::TRANSLATE_X},
#line 244 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str163,
     TokenType::SLATEGREY},
#line 243 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str164,
     TokenType::SLATEGRAY},
#line 131 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str165,
     TokenType::CRIMSON},
#line 296 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str166,
     TokenType::BORDER_BOTTOM_COLOR},
#line 311 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str167,
     TokenType::FILTER},
#line 261 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str168,
     TokenType::ROTATE_Y},
#line 120 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str169,
     TokenType::BLANCHEDALMOND},
#line 187 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str170,
     TokenType::LIGHTSALMON},
#line 233 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str171,
     TokenType::ROYALBLUE},
#line 105 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str172,
     TokenType::CONTRAST},
#line 208 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str173,
     TokenType::MIDNIGHTBLUE},
#line 57 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str174,
     TokenType::SPACE},
#line 334 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str175,
     TokenType::INFINITE},
#line 222 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str176,
     TokenType::PALETURQUOISE},
#line 229 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str177,
     TokenType::POWDERBLUE},
#line 188 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str178,
     TokenType::LIGHTSEAGREEN},
#line 254 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str179,
     TokenType::WHEAT},
#line 91 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str180,
     TokenType::FORMAT},
#line 255 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str181,
     TokenType::WHITE},
#line 339 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str182,
     TokenType::TOKEN_FALSE},
#line 99 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str183,
     TokenType::PATH},
#line 134 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str184,
     TokenType::DARKCYAN},
#line 313 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str185,
     TokenType::TRANSFORM_ORIGIN},
#line 300 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str186,
     TokenType::MARGIN_BOTTOM},
#line 184 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str187,
     TokenType::LIGHTGREEN},
#line 278 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str188,
     TokenType::WIDTH},
#line 275 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str189,
     TokenType::MATRIX_3D},
#line 283 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str190,
     TokenType::TRANSFORM},
#line 227 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str191,
     TokenType::PINK},
#line 343 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str192,
     TokenType::FROM},
#line 110 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str193,
     TokenType::FIT_CONTENT},
#line 124 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str194,
     TokenType::BURLYWOOD},
#line 205 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str195,
     TokenType::MEDIUMSPRINGGREEN},
#line 168 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str196,
     TokenType::GREY},
#line 165 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str197,
     TokenType::GRAY},
#line 45 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str198,
     TokenType::MAX_CONTENT},
#line 302 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str199,
     TokenType::PADDING_RIGHT},
#line 290 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str200,
     TokenType::BORDER_RIGHT_WIDTH},
#line 54 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str201,
     TokenType::REPEAT_Y},
#line 70 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str202,
     TokenType::FARTHEST_CORNER},
#line 132 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str203,
     TokenType::CYAN},
#line 152 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str204,
     TokenType::DEEPPINK},
#line 192 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str205,
     TokenType::LIGHTSTEELBLUE},
#line 69 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str206,
     TokenType::FARTHEST_SIDE},
#line 333 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str207,
     TokenType::BOTH},
#line 232 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str208,
     TokenType::ROSYBROWN},
#line 159 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str209,
     TokenType::FORESTGREEN},
#line 245 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str210,
     TokenType::SNOW},
#line 179 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str211,
     TokenType::LIGHTBLUE},
#line 108 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str212,
     TokenType::VAR},
#line 66 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str213,
     TokenType::CONIC_GRADIENT},
#line 67 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str214,
     TokenType::CLOSEST_SIDE},
#line 308 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str215,
     TokenType::BORDER_COLOR},
#line 112 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str216,
     TokenType::ALICEBLUE},
#line 323 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str217,
     TokenType::SQUARE_BEZIER},
#line 215 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str218,
     TokenType::OLIVE},
#line 68 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str219,
     TokenType::CLOSEST_CORNER},
#line 125 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str220,
     TokenType::CADETBLUE},
#line 295 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str221,
     TokenType::BORDER_TOP_COLOR},
#line 167 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str222,
     TokenType::GREENYELLOW},
#line 258 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str223,
     TokenType::YELLOWGREEN},
#line 253 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str224,
     TokenType::VIOLET},
#line 155 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str225,
     TokenType::DIMGREY},
#line 154 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str226,
     TokenType::DIMGRAY},
#line 219 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str227,
     TokenType::ORCHID},
#line 256 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str228,
     TokenType::WHITESMOKE},
#line 224 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str229,
     TokenType::PAPAYAWHIP},
#line 286 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str230,
     TokenType::MAX_HEIGHT},
#line 175 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str231,
     TokenType::LAVENDER},
#line 153 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str232,
     TokenType::DEEPSKYBLUE},
#line 292 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str233,
     TokenType::BORDER_BOTTOM_WIDTH},
#line 186 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str234,
     TokenType::LIGHTPINK},
#line 330 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str235,
     TokenType::ALTERNATE_REVERSE},
#line 293 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str236,
     TokenType::BORDER_LEFT_COLOR},
#line 60 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str237,
     TokenType::PADDING_BOX},
#line 178 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str238,
     TokenType::LEMONCHIFFON},
#line 150 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str239,
     TokenType::DARKTURQUOISE},
#line 191 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str240,
     TokenType::LIGHTSLATEGREY},
#line 190 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str241,
     TokenType::LIGHTSLATEGRAY},
#line 240 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str242,
     TokenType::SILVER},
#line 328 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str243,
     TokenType::REVERSE},
#line 174 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str244,
     TokenType::KHAKI},
#line 138 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str245,
     TokenType::DARKGREY},
#line 136 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str246,
     TokenType::DARKGRAY},
#line 119 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str247,
     TokenType::BLACK},
#line 126 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str248,
     TokenType::CHARTREUSE},
#line 185 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str249,
     TokenType::LIGHTGREY},
#line 183 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str250,
     TokenType::LIGHTGRAY},
#line 103 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str251,
     TokenType::GRAYSCALE},
#line 72 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str252,
     TokenType::CIRCLE},
#line 149 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str253,
     TokenType::DARKSLATEGREY},
#line 148 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str254,
     TokenType::DARKSLATEGRAY},
#line 257 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str255,
     TokenType::YELLOW},
#line 89 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str256,
     TokenType::LINE_THROUGH},
#line 180 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str257,
     TokenType::LIGHTCORAL},
#line 241 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str258,
     TokenType::SKYBLUE},
#line 59 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str259,
     TokenType::BORDER_BOX},
#line 291 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str260,
     TokenType::BORDER_TOP_WIDTH},
#line 139 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str261,
     TokenType::DARKKHAKI},
#line 170 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str262,
     TokenType::HOTPINK},
#line 129 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str263,
     TokenType::CORNFLOWERBLUE},
#line 273 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str264,
     TokenType::SKEW_Y},
#line 285 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str265,
     TokenType::MAX_WIDTH},
#line 216 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str266,
     TokenType::OLIVEDRAB},
#line 324 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str267,
     TokenType::CUBIC_BEZIER},
#line 169 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str268,
     TokenType::HONEYDEW},
#line 182 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str269,
     TokenType::LIGHTGOLDENRODYELLOW},
#line 289 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str270,
     TokenType::BORDER_LEFT_WIDTH},
#line 37 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str271,
     TokenType::PX},
#line 207 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str272,
     TokenType::MEDIUMVIOLETRED},
#line 260 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str273,
     TokenType::ROTATE_X},
#line 38 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str274,
     TokenType::RPX},
#line 271 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str275,
     TokenType::SKEW},
#line 78 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str276,
     TokenType::THICK},
#line 173 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str277,
     TokenType::IVORY},
#line 332 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str278,
     TokenType::BACKWARDS},
#line 276 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str279,
     TokenType::OPACITY},
#line 61 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str280,
     TokenType::CONTENT_BOX},
#line 274 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str281,
     TokenType::MATRIX},
#line 193 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str282,
     TokenType::LIGHTYELLOW},
#line 270 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str283,
     TokenType::SCALE_Y},
#line 44 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str284,
     TokenType::PPX},
#line 130 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str285,
     TokenType::CORNSILK},
#line 211 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str286,
     TokenType::MOCCASIN},
#line 331 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str287,
     TokenType::FORWARDS},
#line 143 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str288,
     TokenType::DARKORCHID},
#line 335 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str289,
     TokenType::INFINITY_TOKEN},
#line 189 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str290,
     TokenType::LIGHTSKYBLUE},
#line 53 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str291,
     TokenType::REPEAT_X},
#line 312 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str292,
     TokenType::BACKGROUND_POSITION},
#line 141 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str293,
     TokenType::DARKOLIVEGREEN},
#line 282 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str294,
     TokenType::VISIBILITY},
#line 162 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str295,
     TokenType::GHOSTWHITE},
#line 181 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str296,
     TokenType::LIGHTCYAN},
#line 158 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str297,
     TokenType::FLORALWHITE},
#line 272 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str298,
     TokenType::SKEW_X},
#line 307 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str299,
     TokenType::BORDER_WIDTH},
#line 51 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str300,
     TokenType::COVER},
#line 127 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str301,
     TokenType::CHOCOLATE},
#line 213 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str302,
     TokenType::NAVY},
#line 42 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str303,
     TokenType::VH},
#line 157 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str304,
     TokenType::FIREBRICK},
#line 101 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str305,
     TokenType::CALC},
#line 314 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str306,
     TokenType::OFFSET_DISTANCE},
#line 41 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str307,
     TokenType::VW},
#line 305 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str308,
     TokenType::FLEX_BASIS},
#line 160 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str309,
     TokenType::FUCHSIA},
#line 212 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str310,
     TokenType::NAVAJOWHITE},
#line 102 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str311,
     TokenType::ENV},
#line 176 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str312,
     TokenType::LAVENDERBLUSH},
#line 269 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str313,
     TokenType::SCALE_X},
#line 280 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str314,
     TokenType::BACKGROUND_COLOR},
#line 306 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str315,
     TokenType::FLEX_GROW},
#line 277 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str316,
     TokenType::SCALE_XY},
#line 342 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str317,
     TokenType::OFF},
#line 90 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str318,
     TokenType::WAVY},
#line 225 "css_keywords.tmpl"
    {(int)(size_t) & ((struct stringpool_t*)0)->stringpool_str319,
     TokenType::PEACHPUFF}};
#if (defined __GNUC__ && __GNUC__ + (__GNUC_MINOR__ >= 6) > 4) || \
    (defined __clang__ && __clang_major__ >= 3)
#pragma GCC diagnostic pop
#endif

static const short lookup[] = {
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  0,   -1,  -1,  -1,  1,   2,
    -1,  -1,  3,   -1,  4,   -1,  -1,  -1,  -1,  -1,  5,   -1,  -1,  -1,  6,
    -1,  7,   -1,  -1,  -1,  -1,  8,   -1,  9,   10,  11,  12,  -1,  -1,  -1,
    13,  14,  -1,  15,  -1,  -1,  -1,  -1,  16,  17,  -1,  18,  -1,  -1,  19,
    -1,  -1,  20,  -1,  21,  22,  -1,  23,  -1,  -1,  24,  25,  -1,  -1,  -1,
    26,  -1,  27,  28,  29,  30,  -1,  31,  -1,  32,  33,  34,  35,  -1,  36,
    37,  38,  -1,  39,  40,  41,  -1,  42,  43,  44,  45,  46,  47,  -1,  -1,
    -1,  -1,  -1,  48,  -1,  -1,  -1,  -1,  -1,  49,  -1,  -1,  50,  -1,  -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  51,  52,  -1,  53,  54,  55,  -1,  56,
    -1,  -1,  57,  -1,  -1,  58,  -1,  -1,  -1,  59,  60,  -1,  61,  -1,  -1,
    62,  -1,  -1,  63,  -1,  64,  -1,  -1,  65,  -1,  66,  -1,  67,  -1,  68,
    69,  70,  -1,  -1,  -1,  71,  72,  -1,  -1,  -1,  73,  74,  75,  -1,  -1,
    -1,  -1,  -1,  -1,  76,  -1,  77,  -1,  78,  -1,  -1,  -1,  -1,  79,  -1,
    -1,  80,  -1,  81,  -1,  -1,  82,  83,  84,  85,  -1,  -1,  -1,  86,  87,
    88,  89,  90,  91,  -1,  92,  93,  94,  95,  96,  97,  -1,  -1,  98,  99,
    100, -1,  101, -1,  102, 103, -1,  -1,  -1,  104, 105, -1,  -1,  -1,  106,
    -1,  107, 108, -1,  109, 110, -1,  111, -1,  112, -1,  113, -1,  114, 115,
    116, 117, 118, -1,  -1,  119, -1,  -1,  -1,  120, 121, 122, 123, -1,  -1,
    -1,  -1,  124, 125, -1,  -1,  -1,  -1,  -1,  -1,  -1,  126, -1,  -1,  127,
    128, -1,  -1,  -1,  -1,  -1,  129, 130, 131, 132, 133, -1,  -1,  -1,  134,
    135, 136, 137, -1,  138, 139, 140, -1,  141, -1,  -1,  142, -1,  -1,  143,
    -1,  -1,  -1,  -1,  144, -1,  145, 146, -1,  147, -1,  148, 149, 150, 151,
    -1,  -1,  -1,  152, 153, 154, -1,  -1,  -1,  155, -1,  -1,  156, -1,  -1,
    157, 158, -1,  159, 160, 161, -1,  -1,  -1,  -1,  162, 163, 164, 165, -1,
    -1,  166, -1,  -1,  167, -1,  168, -1,  -1,  169, -1,  -1,  -1,  -1,  -1,
    -1,  170, 171, 172, -1,  173, -1,  -1,  174, -1,  -1,  -1,  175, -1,  -1,
    176, 177, -1,  178, -1,  -1,  -1,  179, -1,  180, 181, -1,  -1,  -1,  -1,
    182, 183, 184, -1,  185, 186, 187, -1,  -1,  188, -1,  189, -1,  -1,  190,
    191, -1,  -1,  -1,  -1,  -1,  -1,  192, 193, -1,  194, -1,  195, -1,  -1,
    196, 197, 198, 199, -1,  -1,  -1,  -1,  -1,  -1,  200, 201, -1,  -1,  -1,
    -1,  -1,  -1,  -1,  202, -1,  -1,  203, -1,  204, -1,  205, -1,  206, -1,
    -1,  -1,  207, -1,  -1,  208, -1,  -1,  209, -1,  -1,  210, -1,  -1,  -1,
    211, 212, 213, -1,  214, -1,  215, 216, -1,  -1,  -1,  -1,  -1,  217, 218,
    219, -1,  220, -1,  221, 222, 223, 224, -1,  -1,  -1,  225, 226, -1,  227,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  228, -1,  -1,  -1,  229,
    -1,  230, -1,  231, -1,  232, -1,  -1,  -1,  233, 234, -1,  235, -1,  -1,
    236, -1,  -1,  -1,  -1,  -1,  -1,  -1,  237, -1,  238, 239, -1,  240, 241,
    242, -1,  -1,  243, -1,  -1,  -1,  244, -1,  -1,  245, 246, 247, 248, 249,
    250, -1,  -1,  251, -1,  -1,  -1,  -1,  252, -1,  -1,  -1,  -1,  -1,  253,
    254, -1,  255, 256, -1,  -1,  -1,  -1,  257, -1,  -1,  258, 259, -1,  -1,
    260, -1,  -1,  261, -1,  -1,  -1,  -1,  -1,  -1,  262, -1,  -1,  -1,  263,
    264, -1,  -1,  -1,  -1,  -1,  -1,  -1,  265, -1,  -1,  -1,  266, -1,  -1,
    -1,  -1,  -1,  267, -1,  268, -1,  269, -1,  -1,  270, 271, -1,  272, -1,
    -1,  273, -1,  -1,  -1,  274, 275, 276, -1,  -1,  277, -1,  -1,  -1,  -1,
    -1,  -1,  278, -1,  -1,  -1,  -1,  -1,  279, -1,  280, -1,  281, -1,  -1,
    -1,  -1,  282, 283, -1,  -1,  -1,  284, -1,  -1,  -1,  285, -1,  -1,  286,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  287, -1,  -1,  -1,  -1,  -1,  -1,
    288, -1,  289, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,  290, -1,  -1,  -1,  -1,  291, 292, -1,  -1,  -1,  293, -1,  -1,  -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  294, -1,  -1,  -1,  295, -1,  -1,
    296, -1,  297, -1,  -1,  298, -1,  -1,  -1,  299, -1,  300, -1,  -1,  -1,
    -1,  -1,  -1,  301, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  302, -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  303, -1,  -1,  -1,  -1,  -1,  -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  304, 305, 306, -1,  -1,  -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  307, -1,  -1,  -1,  -1,  -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  308, -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,  309, -1,  -1,  -1,  -1,  310, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  311, -1,  -1,  -1,
    -1,  -1,  -1,  312, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  313, -1,
    -1,  314, -1,  -1,  -1,  -1,  315, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  316, -1,  -1,  -1,
    -1,  -1,  -1,  -1,  317, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  318,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  319};

const struct TokenValue* CSSKeywordsHash::GetTokenValue(const char* str,
                                                        size_t len) {
  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH) {
    unsigned int key = hash(str, len);

    if (key <= MAX_HASH_VALUE) {
      int index = lookup[key];

      if (index >= 0) {
        const char* s = token_list[index].name + stringpool;

        if (*str == *s && !strcmp(str + 1, s + 1)) return &token_list[index];
      }
    }
  }
  return static_cast<struct TokenValue*>(0);
}
#line 344 "css_keywords.tmpl"

const struct TokenValue* GetTokenValue(const char* str, unsigned len) {
  return CSSKeywordsHash::GetTokenValue(str, len);
}
}  // namespace tasm
}  // namespace lynx
// NOLINTEND(modernize-use-nullptr)
