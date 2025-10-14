/* C++ code produced by gperf version 3.1 */
/* Command-line: gperf -m 100 --output-file=clay/gfx/style/color_gen.inl clay/gfx/style/color.tmpl  */
/* Computed positions: -k'1,3,6-8,12-13' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gperf@gnu.org>."
#endif

#line 10 "clay/gfx/style/color.tmpl"

#line 13 "clay/gfx/style/color.tmpl"
struct __attribute__((packed)) TokenValue {
  short name;
  unsigned int color;
};

static constexpr unsigned int RGBColor(uint8_t r, uint8_t g, uint8_t b) {
  return 0xFF000000 | (r << 16) | (g << 8) | b;
};

#define TOTAL_KEYWORDS 148
#define MIN_WORD_LENGTH 3
#define MAX_WORD_LENGTH 20
#define MIN_HASH_VALUE 10
#define MAX_HASH_VALUE 301
/* maximum key range = 292, duplicates = 0 */

class ColorHash
{
private:
  static inline unsigned int hash (const char *str, size_t len);
public:
  static const struct __attribute__((packed)) TokenValue *find (const char *str, size_t len);
};

inline unsigned int
ColorHash::hash (const char *str, size_t len)
{
  static const unsigned short asso_values[] =
    {
      302, 302, 302, 302, 302, 302, 302, 302, 302, 302,
      302, 302, 302, 302, 302, 302, 302, 302, 302, 302,
      302, 302, 302, 302, 302, 302, 302, 302, 302, 302,
      302, 302, 302, 302, 302, 302, 302, 302, 302, 302,
      302, 302, 302, 302, 302, 302, 302, 302, 302, 302,
      302, 302, 302, 302, 302, 302, 302, 302, 302, 302,
      302, 302, 302, 302, 302, 302, 302, 302, 302, 302,
      302, 302, 302, 302, 302, 302, 302, 302, 302, 302,
      302, 302, 302, 302, 302, 302, 302, 302, 302, 302,
      302, 302, 302, 302, 302, 302, 302,   5,  55,   3,
       26,   3,  51,   6,  21,   3, 302, 102,   4,  53,
       14,  29,  52, 115,   3,   9,   4,  58,  54,  95,
       69,  70, 302,   5, 302, 302, 302, 302, 302, 302,
      302, 302, 302, 302, 302, 302, 302, 302, 302, 302,
      302, 302, 302, 302, 302, 302, 302, 302, 302, 302,
      302, 302, 302, 302, 302, 302, 302, 302, 302, 302,
      302, 302, 302, 302, 302, 302, 302, 302, 302, 302,
      302, 302, 302, 302, 302, 302, 302, 302, 302, 302,
      302, 302, 302, 302, 302, 302, 302, 302, 302, 302,
      302, 302, 302, 302, 302, 302, 302, 302, 302, 302,
      302, 302, 302, 302, 302, 302, 302, 302, 302, 302,
      302, 302, 302, 302, 302, 302, 302, 302, 302, 302,
      302, 302, 302, 302, 302, 302, 302, 302, 302, 302,
      302, 302, 302, 302, 302, 302, 302, 302, 302, 302,
      302, 302, 302, 302, 302, 302, 302, 302, 302, 302,
      302, 302, 302, 302, 302, 302, 302, 302
    };
  unsigned int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[static_cast<unsigned char>(str[12])];
      /*FALLTHROUGH*/
      case 12:
        hval += asso_values[static_cast<unsigned char>(str[11])];
      /*FALLTHROUGH*/
      case 11:
      case 10:
      case 9:
      case 8:
        hval += asso_values[static_cast<unsigned char>(str[7])];
      /*FALLTHROUGH*/
      case 7:
        hval += asso_values[static_cast<unsigned char>(str[6])];
      /*FALLTHROUGH*/
      case 6:
        hval += asso_values[static_cast<unsigned char>(str[5])];
      /*FALLTHROUGH*/
      case 5:
      case 4:
      case 3:
        hval += asso_values[static_cast<unsigned char>(str[2]+2)];
      /*FALLTHROUGH*/
      case 2:
      case 1:
        hval += asso_values[static_cast<unsigned char>(str[0])];
        break;
    }
  return hval;
}

struct StringPool_t
  {
    char StringPool_str10[sizeof("cyan")];
    char StringPool_str11[sizeof("teal")];
    char StringPool_str12[sizeof("coral")];
    char StringPool_str13[sizeof("gray")];
    char StringPool_str16[sizeof("grey")];
    char StringPool_str17[sizeof("green")];
    char StringPool_str24[sizeof("gold")];
    char StringPool_str26[sizeof("sienna")];
    char StringPool_str28[sizeof("lightgrey")];
    char StringPool_str29[sizeof("lightgreen")];
    char StringPool_str30[sizeof("lightgray")];
    char StringPool_str31[sizeof("seashell")];
    char StringPool_str32[sizeof("silver")];
    char StringPool_str33[sizeof("slategrey")];
    char StringPool_str35[sizeof("slategray")];
    char StringPool_str36[sizeof("lightsalmon")];
    char StringPool_str37[sizeof("lime")];
    char StringPool_str39[sizeof("springgreen")];
    char StringPool_str40[sizeof("seagreen")];
    char StringPool_str41[sizeof("orange")];
    char StringPool_str43[sizeof("salmon")];
    char StringPool_str45[sizeof("lightslategrey")];
    char StringPool_str47[sizeof("lightslategray")];
    char StringPool_str48[sizeof("darkgreen")];
    char StringPool_str50[sizeof("orangered")];
    char StringPool_str51[sizeof("limegreen")];
    char StringPool_str52[sizeof("lightcoral")];
    char StringPool_str54[sizeof("lightseagreen")];
    char StringPool_str55[sizeof("darkmagenta")];
    char StringPool_str57[sizeof("red")];
    char StringPool_str58[sizeof("turquoise")];
    char StringPool_str59[sizeof("tan")];
    char StringPool_str60[sizeof("peru")];
    char StringPool_str61[sizeof("linen")];
    char StringPool_str62[sizeof("darkorange")];
    char StringPool_str63[sizeof("black")];
    char StringPool_str64[sizeof("orchid")];
    char StringPool_str65[sizeof("purple")];
    char StringPool_str66[sizeof("darkred")];
    char StringPool_str67[sizeof("darkorchid")];
    char StringPool_str68[sizeof("tomato")];
    char StringPool_str69[sizeof("fuchsia")];
    char StringPool_str70[sizeof("darkseagreen")];
    char StringPool_str72[sizeof("magenta")];
    char StringPool_str73[sizeof("firebrick")];
    char StringPool_str75[sizeof("goldenrod")];
    char StringPool_str76[sizeof("darkviolet")];
    char StringPool_str77[sizeof("maroon")];
    char StringPool_str78[sizeof("transparent")];
    char StringPool_str79[sizeof("forestgreen")];
    char StringPool_str80[sizeof("chartreuse")];
    char StringPool_str82[sizeof("skyblue")];
    char StringPool_str83[sizeof("indianred")];
    char StringPool_str84[sizeof("palegreen")];
    char StringPool_str85[sizeof("lightpink")];
    char StringPool_str86[sizeof("lemonchiffon")];
    char StringPool_str87[sizeof("navy")];
    char StringPool_str89[sizeof("indigo")];
    char StringPool_str90[sizeof("moccasin")];
    char StringPool_str92[sizeof("lawngreen")];
    char StringPool_str93[sizeof("oldlace")];
    char StringPool_str94[sizeof("lightcyan")];
    char StringPool_str95[sizeof("lightyellow")];
    char StringPool_str98[sizeof("lightgoldenrodyellow")];
    char StringPool_str99[sizeof("lightsteelblue")];
    char StringPool_str100[sizeof("greenyellow")];
    char StringPool_str102[sizeof("darksalmon")];
    char StringPool_str103[sizeof("darkblue")];
    char StringPool_str104[sizeof("aqua")];
    char StringPool_str105[sizeof("azure")];
    char StringPool_str106[sizeof("wheat")];
    char StringPool_str108[sizeof("pink")];
    char StringPool_str110[sizeof("khaki")];
    char StringPool_str111[sizeof("darkolivegreen")];
    char StringPool_str113[sizeof("lavender")];
    char StringPool_str114[sizeof("darkgrey")];
    char StringPool_str116[sizeof("darkgray")];
    char StringPool_str117[sizeof("darkslateblue")];
    char StringPool_str120[sizeof("thistle")];
    char StringPool_str121[sizeof("aquamarine")];
    char StringPool_str122[sizeof("bisque")];
    char StringPool_str123[sizeof("ivory")];
    char StringPool_str124[sizeof("cornsilk")];
    char StringPool_str125[sizeof("mintcream")];
    char StringPool_str127[sizeof("darkcyan")];
    char StringPool_str128[sizeof("snow")];
    char StringPool_str129[sizeof("darkslategrey")];
    char StringPool_str131[sizeof("darkslategray")];
    char StringPool_str132[sizeof("saddlebrown")];
    char StringPool_str133[sizeof("lightblue")];
    char StringPool_str134[sizeof("royalblue")];
    char StringPool_str135[sizeof("dimgrey")];
    char StringPool_str136[sizeof("olive")];
    char StringPool_str137[sizeof("dimgray")];
    char StringPool_str138[sizeof("slateblue")];
    char StringPool_str140[sizeof("chocolate")];
    char StringPool_str141[sizeof("steelblue")];
    char StringPool_str144[sizeof("palevioletred")];
    char StringPool_str148[sizeof("lavenderblush")];
    char StringPool_str149[sizeof("dodgerblue")];
    char StringPool_str150[sizeof("midnightblue")];
    char StringPool_str151[sizeof("plum")];
    char StringPool_str154[sizeof("blue")];
    char StringPool_str155[sizeof("crimson")];
    char StringPool_str157[sizeof("darkgoldenrod")];
    char StringPool_str158[sizeof("sandybrown")];
    char StringPool_str159[sizeof("deeppink")];
    char StringPool_str161[sizeof("mistyrose")];
    char StringPool_str162[sizeof("beige")];
    char StringPool_str165[sizeof("blanchedalmond")];
    char StringPool_str167[sizeof("darkkhaki")];
    char StringPool_str174[sizeof("olivedrab")];
    char StringPool_str175[sizeof("brown")];
    char StringPool_str179[sizeof("violet")];
    char StringPool_str180[sizeof("cadetblue")];
    char StringPool_str185[sizeof("yellow")];
    char StringPool_str186[sizeof("papayawhip")];
    char StringPool_str189[sizeof("mediumseagreen")];
    char StringPool_str193[sizeof("palegoldenrod")];
    char StringPool_str194[sizeof("powderblue")];
    char StringPool_str196[sizeof("blueviolet")];
    char StringPool_str197[sizeof("rosybrown")];
    char StringPool_str198[sizeof("hotpink")];
    char StringPool_str199[sizeof("yellowgreen")];
    char StringPool_str202[sizeof("white")];
    char StringPool_str203[sizeof("lightskyblue")];
    char StringPool_str204[sizeof("gainsboro")];
    char StringPool_str205[sizeof("honeydew")];
    char StringPool_str211[sizeof("cornflowerblue")];
    char StringPool_str221[sizeof("burlywood")];
    char StringPool_str225[sizeof("peachpuff")];
    char StringPool_str226[sizeof("mediumblue")];
    char StringPool_str227[sizeof("mediumorchid")];
    char StringPool_str230[sizeof("antiquewhite")];
    char StringPool_str231[sizeof("darkturquoise")];
    char StringPool_str233[sizeof("aliceblue")];
    char StringPool_str236[sizeof("mediumvioletred")];
    char StringPool_str239[sizeof("navajowhite")];
    char StringPool_str244[sizeof("mediumslateblue")];
    char StringPool_str247[sizeof("mediumspringgreen")];
    char StringPool_str250[sizeof("ghostwhite")];
    char StringPool_str266[sizeof("mediumturquoise")];
    char StringPool_str267[sizeof("paleturquoise")];
    char StringPool_str270[sizeof("deepskyblue")];
    char StringPool_str282[sizeof("mediumpurple")];
    char StringPool_str297[sizeof("floralwhite")];
    char StringPool_str298[sizeof("whitesmoke")];
    char StringPool_str301[sizeof("mediumaquamarine")];
  };
static const struct StringPool_t StringPool_contents =
  {
    "cyan",
    "teal",
    "coral",
    "gray",
    "grey",
    "green",
    "gold",
    "sienna",
    "lightgrey",
    "lightgreen",
    "lightgray",
    "seashell",
    "silver",
    "slategrey",
    "slategray",
    "lightsalmon",
    "lime",
    "springgreen",
    "seagreen",
    "orange",
    "salmon",
    "lightslategrey",
    "lightslategray",
    "darkgreen",
    "orangered",
    "limegreen",
    "lightcoral",
    "lightseagreen",
    "darkmagenta",
    "red",
    "turquoise",
    "tan",
    "peru",
    "linen",
    "darkorange",
    "black",
    "orchid",
    "purple",
    "darkred",
    "darkorchid",
    "tomato",
    "fuchsia",
    "darkseagreen",
    "magenta",
    "firebrick",
    "goldenrod",
    "darkviolet",
    "maroon",
    "transparent",
    "forestgreen",
    "chartreuse",
    "skyblue",
    "indianred",
    "palegreen",
    "lightpink",
    "lemonchiffon",
    "navy",
    "indigo",
    "moccasin",
    "lawngreen",
    "oldlace",
    "lightcyan",
    "lightyellow",
    "lightgoldenrodyellow",
    "lightsteelblue",
    "greenyellow",
    "darksalmon",
    "darkblue",
    "aqua",
    "azure",
    "wheat",
    "pink",
    "khaki",
    "darkolivegreen",
    "lavender",
    "darkgrey",
    "darkgray",
    "darkslateblue",
    "thistle",
    "aquamarine",
    "bisque",
    "ivory",
    "cornsilk",
    "mintcream",
    "darkcyan",
    "snow",
    "darkslategrey",
    "darkslategray",
    "saddlebrown",
    "lightblue",
    "royalblue",
    "dimgrey",
    "olive",
    "dimgray",
    "slateblue",
    "chocolate",
    "steelblue",
    "palevioletred",
    "lavenderblush",
    "dodgerblue",
    "midnightblue",
    "plum",
    "blue",
    "crimson",
    "darkgoldenrod",
    "sandybrown",
    "deeppink",
    "mistyrose",
    "beige",
    "blanchedalmond",
    "darkkhaki",
    "olivedrab",
    "brown",
    "violet",
    "cadetblue",
    "yellow",
    "papayawhip",
    "mediumseagreen",
    "palegoldenrod",
    "powderblue",
    "blueviolet",
    "rosybrown",
    "hotpink",
    "yellowgreen",
    "white",
    "lightskyblue",
    "gainsboro",
    "honeydew",
    "cornflowerblue",
    "burlywood",
    "peachpuff",
    "mediumblue",
    "mediumorchid",
    "antiquewhite",
    "darkturquoise",
    "aliceblue",
    "mediumvioletred",
    "navajowhite",
    "mediumslateblue",
    "mediumspringgreen",
    "ghostwhite",
    "mediumturquoise",
    "paleturquoise",
    "deepskyblue",
    "mediumpurple",
    "floralwhite",
    "whitesmoke",
    "mediumaquamarine"
  };
#define StringPool ((const char *) &StringPool_contents)

static const struct __attribute__((packed)) TokenValue wordlist[] =
  {
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1},
#line 44 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str10, RGBColor(0, 255, 255)},
#line 161 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str11, RGBColor(0, 128, 128)},
#line 40 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str12, RGBColor(255, 127, 80)},
#line 77 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str13, RGBColor(128, 128, 128)},
    {-1}, {-1},
#line 80 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str16, RGBColor(128, 128, 128)},
#line 78 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str17, RGBColor(0, 128, 0)},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 75 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str24, RGBColor(255, 215, 0)},
    {-1},
#line 151 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str26, RGBColor(160, 82, 45)},
    {-1},
#line 97 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str28, RGBColor(211, 211, 211)},
#line 96 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str29, RGBColor(144, 238, 144)},
#line 95 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str30, RGBColor(211, 211, 211)},
#line 150 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str31, RGBColor(255, 245, 238)},
#line 152 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str32, RGBColor(192, 192, 192)},
#line 156 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str33, RGBColor(112, 128, 144)},
    {-1},
#line 155 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str35, RGBColor(112, 128, 144)},
#line 99 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str36, RGBColor(255, 160, 122)},
#line 106 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str37, RGBColor(0, 255, 0)},
    {-1},
#line 158 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str39, RGBColor(0, 255, 127)},
#line 149 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str40, RGBColor(46, 139, 87)},
#line 129 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str41, RGBColor(255, 165, 0)},
    {-1},
#line 147 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str43, RGBColor(250, 128, 114)},
    {-1},
#line 103 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str45, RGBColor(119, 136, 153)},
    {-1},
#line 102 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str47, RGBColor(119, 136, 153)},
#line 49 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str48, RGBColor(0, 100, 0)},
    {-1},
#line 130 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str50, RGBColor(255, 69, 0)},
#line 107 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str51, RGBColor(50, 205, 50)},
#line 92 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str52, RGBColor(240, 128, 128)},
    {-1},
#line 100 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str54, RGBColor(32, 178, 170)},
#line 52 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str55, RGBColor(139, 0, 139)},
    {-1},
#line 143 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str57, RGBColor(255, 0, 0)},
#line 164 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str58, RGBColor(64, 224, 208)},
#line 160 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str59, RGBColor(210, 180, 140)},
#line 138 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str60, RGBColor(205, 133, 63)},
#line 108 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str61, RGBColor(250, 240, 230)},
#line 54 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str62, RGBColor(255, 140, 0)},
#line 31 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str63, RGBColor(0, 0, 0)},
#line 131 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str64, RGBColor(218, 112, 214)},
#line 142 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str65, RGBColor(128, 0, 128)},
#line 56 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str66, RGBColor(139, 0, 0)},
#line 55 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str67, RGBColor(153, 50, 204)},
#line 163 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str68, RGBColor(255, 99, 71)},
#line 72 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str69, RGBColor(255, 0, 255)},
#line 58 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str70, RGBColor(143, 188, 143)},
    {-1},
#line 109 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str72, RGBColor(255, 0, 255)},
#line 69 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str73, RGBColor(178, 34, 34)},
    {-1},
#line 76 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str75, RGBColor(218, 165, 32)},
#line 63 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str76, RGBColor(148, 0, 211)},
#line 110 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str77, RGBColor(128, 0, 0)},
#line 23 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str78, 0},
#line 71 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str79, RGBColor(34, 139, 34)},
#line 38 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str80, RGBColor(127, 255, 0)},
    {-1},
#line 153 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str82, RGBColor(135, 206, 235)},
#line 83 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str83, RGBColor(205, 92, 92)},
#line 133 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str84, RGBColor(152, 251, 152)},
#line 98 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str85, RGBColor(255, 182, 193)},
#line 90 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str86, RGBColor(255, 250, 205)},
#line 125 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str87, RGBColor(0, 0, 128)},
    {-1},
#line 84 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str89, RGBColor(75, 0, 130)},
#line 123 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str90, RGBColor(255, 228, 181)},
    {-1},
#line 89 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str92, RGBColor(124, 252, 0)},
#line 126 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str93, RGBColor(253, 245, 230)},
#line 93 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str94, RGBColor(224, 255, 255)},
#line 105 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str95, RGBColor(255, 255, 224)},
    {-1}, {-1},
#line 94 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str98, RGBColor(250, 250, 210)},
#line 104 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str99, RGBColor(176, 196, 222)},
#line 79 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str100, RGBColor(173, 255, 47)},
    {-1},
#line 57 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str102, RGBColor(233, 150, 122)},
#line 45 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str103, RGBColor(0, 0, 139)},
#line 26 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str104, RGBColor(0, 255, 255)},
#line 28 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str105, RGBColor(240, 255, 255)},
#line 166 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str106, RGBColor(245, 222, 179)},
    {-1},
#line 139 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str108, RGBColor(255, 192, 203)},
    {-1},
#line 86 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str110, RGBColor(240, 230, 140)},
#line 53 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str111, RGBColor(85, 107, 47)},
    {-1},
#line 87 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str113, RGBColor(230, 230, 250)},
#line 50 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str114, RGBColor(169, 169, 169)},
    {-1},
#line 48 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str116, RGBColor(169, 169, 169)},
#line 59 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str117, RGBColor(72, 61, 139)},
    {-1}, {-1},
#line 162 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str120, RGBColor(216, 191, 216)},
#line 27 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str121, RGBColor(127, 255, 212)},
#line 30 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str122, RGBColor(255, 228, 196)},
#line 85 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str123, RGBColor(255, 255, 240)},
#line 42 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str124, RGBColor(255, 248, 220)},
#line 121 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str125, RGBColor(245, 255, 250)},
    {-1},
#line 46 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str127, RGBColor(0, 139, 139)},
#line 157 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str128, RGBColor(255, 250, 250)},
#line 61 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str129, RGBColor(47, 79, 79)},
    {-1},
#line 60 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str131, RGBColor(47, 79, 79)},
#line 146 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str132, RGBColor(139, 69, 19)},
#line 91 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str133, RGBColor(173, 216, 230)},
#line 145 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str134, RGBColor(65, 105, 225)},
#line 67 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str135, RGBColor(105, 105, 105)},
#line 127 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str136, RGBColor(128, 128, 0)},
#line 66 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str137, RGBColor(105, 105, 105)},
#line 154 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str138, RGBColor(106, 90, 205)},
    {-1},
#line 39 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str140, RGBColor(210, 105, 30)},
#line 159 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str141, RGBColor(70, 130, 180)},
    {-1}, {-1},
#line 135 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str144, RGBColor(219, 112, 147)},
    {-1}, {-1}, {-1},
#line 88 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str148, RGBColor(255, 240, 245)},
#line 68 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str149, RGBColor(30, 144, 255)},
#line 120 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str150, RGBColor(25, 25, 112)},
#line 140 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str151, RGBColor(221, 160, 221)},
    {-1}, {-1},
#line 33 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str154, RGBColor(0, 0, 255)},
#line 43 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str155, RGBColor(220, 20, 60)},
    {-1},
#line 47 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str157, RGBColor(184, 134, 11)},
#line 148 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str158, RGBColor(244, 164, 96)},
#line 64 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str159, RGBColor(255, 20, 147)},
    {-1},
#line 122 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str161, RGBColor(255, 228, 225)},
#line 29 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str162, RGBColor(245, 245, 220)},
    {-1}, {-1},
#line 32 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str165, RGBColor(255, 235, 205)},
    {-1},
#line 51 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str167, RGBColor(189, 183, 107)},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 128 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str174, RGBColor(107, 142, 35)},
#line 35 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str175, RGBColor(165, 42, 42)},
    {-1}, {-1}, {-1},
#line 165 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str179, RGBColor(238, 130, 238)},
#line 37 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str180, RGBColor(95, 158, 160)},
    {-1}, {-1}, {-1}, {-1},
#line 169 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str185, RGBColor(255, 255, 0)},
#line 136 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str186, RGBColor(255, 239, 213)},
    {-1}, {-1},
#line 115 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str189, RGBColor(60, 179, 113)},
    {-1}, {-1}, {-1},
#line 132 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str193, RGBColor(238, 232, 170)},
#line 141 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str194, RGBColor(176, 224, 230)},
    {-1},
#line 34 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str196, RGBColor(138, 43, 226)},
#line 144 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str197, RGBColor(188, 143, 143)},
#line 82 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str198, RGBColor(255, 105, 180)},
#line 170 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str199, RGBColor(154, 205, 50)},
    {-1}, {-1},
#line 167 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str202, RGBColor(255, 255, 255)},
#line 101 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str203, RGBColor(135, 206, 250)},
#line 73 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str204, RGBColor(220, 220, 220)},
#line 81 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str205, RGBColor(240, 255, 240)},
    {-1}, {-1}, {-1}, {-1}, {-1},
#line 41 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str211, RGBColor(100, 149, 237)},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 36 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str221, RGBColor(222, 184, 135)},
    {-1}, {-1}, {-1},
#line 137 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str225, RGBColor(255, 218, 185)},
#line 112 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str226, RGBColor(0, 0, 205)},
#line 113 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str227, RGBColor(186, 85, 211)},
    {-1}, {-1},
#line 25 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str230, RGBColor(250, 235, 215)},
#line 62 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str231, RGBColor(0, 206, 209)},
    {-1},
#line 24 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str233, RGBColor(240, 248, 255)},
    {-1}, {-1},
#line 119 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str236, RGBColor(199, 21, 133)},
    {-1}, {-1},
#line 124 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str239, RGBColor(255, 222, 173)},
    {-1}, {-1}, {-1}, {-1},
#line 116 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str244, RGBColor(123, 104, 238)},
    {-1}, {-1},
#line 117 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str247, RGBColor(0, 250, 154)},
    {-1}, {-1},
#line 74 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str250, RGBColor(248, 248, 255)},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 118 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str266, RGBColor(72, 209, 204)},
#line 134 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str267, RGBColor(175, 238, 238)},
    {-1}, {-1},
#line 65 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str270, RGBColor(0, 191, 255)},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1},
#line 114 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str282, RGBColor(147, 112, 219)},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1},
#line 70 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str297, RGBColor(255, 250, 240)},
#line 168 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str298, RGBColor(245, 245, 245)},
    {-1}, {-1},
#line 111 "clay/gfx/style/color.tmpl"
    {(short)(size_t)&((struct StringPool_t *)0)->StringPool_str301, RGBColor(102, 205, 170)}
  };

const struct __attribute__((packed)) TokenValue *
ColorHash::find (const char *str, size_t len)
{
  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      unsigned int key = hash (str, len);

      if (key <= MAX_HASH_VALUE)
        {
          int o = wordlist[key].name;
          if (o >= 0)
            {
              const char *s = o + StringPool;

              if (*str == *s && !strcmp (str + 1, s + 1))
                return &wordlist[key];
            }
        }
    }
  return 0;
}
#line 171 "clay/gfx/style/color.tmpl"

