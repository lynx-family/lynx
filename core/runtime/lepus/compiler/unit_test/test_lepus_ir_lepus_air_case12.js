// [TEST_TARGET: IR]
// Focus: Rematerialization with conditional verification paths
// Strategy: 90 prefix vars + 180 unique constants (2001-2180) used in 3 groups
// of 30 objects each, ALL in straight-line (entry block). Verification logic uses
// if-blocks to validate results through different conditional paths.
// After CSE: each constant has 3 aggregate users, span >= 64 → kLongTailAggregate
// The if-blocks test that rematerialization output is correct around branch boundaries.

let $currentComponentId = 1;
let __assert_fail_count = 0;

function SoftAssert(v, msg) {
  if (!v) {
    __assert_fail_count = __assert_fail_count + 1;
    print(msg);
  }
}

function __CreatePage(a, b) { return {}; }
function __CreateView(a) { return {}; }
function __CreateText(a) { return {}; }
function __AppendElement(a, b) {}
function __SetAttribute(a, b, c) {}
function __FlushElementTree(a) {}
function __AddEvent(a, b, c, d) {}

// --- 90 toplevel let variables to consume prefix registers ---
let v0 = 0; let v1 = 1; let v2 = 2; let v3 = 3; let v4 = 4;
let v5 = 5; let v6 = 6; let v7 = 7; let v8 = 8; let v9 = 9;
let v10 = 10; let v11 = 11; let v12 = 12; let v13 = 13; let v14 = 14;
let v15 = 15; let v16 = 16; let v17 = 17; let v18 = 18; let v19 = 19;
let v20 = 20; let v21 = 21; let v22 = 22; let v23 = 23; let v24 = 24;
let v25 = 25; let v26 = 26; let v27 = 27; let v28 = 28; let v29 = 29;
let v30 = 30; let v31 = 31; let v32 = 32; let v33 = 33; let v34 = 34;
let v35 = 35; let v36 = 36; let v37 = 37; let v38 = 38; let v39 = 39;
let v40 = 40; let v41 = 41; let v42 = 42; let v43 = 43; let v44 = 44;
let v45 = 45; let v46 = 46; let v47 = 47; let v48 = 48; let v49 = 49;
let v50 = 50; let v51 = 51; let v52 = 52; let v53 = 53; let v54 = 54;
let v55 = 55; let v56 = 56; let v57 = 57; let v58 = 58; let v59 = 59;
let v60 = 60; let v61 = 61; let v62 = 62; let v63 = 63; let v64 = 64;
let v65 = 65; let v66 = 66; let v67 = 67; let v68 = 68; let v69 = 69;
let v70 = 70; let v71 = 71; let v72 = 72; let v73 = 73; let v74 = 74;
let v75 = 75; let v76 = 76; let v77 = 77; let v78 = 78; let v79 = 79;
let v80 = 80; let v81 = 81; let v82 = 82; let v83 = 83; let v84 = 84;
let v85 = 85; let v86 = 86; let v87 = 87; let v88 = 88; let v89 = 89;

// --- Template rendering simulation ---
let $page = __CreatePage("0", 0);
let $root = __CreateView($currentComponentId);
__AppendElement($page, $root);

// GROUP A: 30 objects with unique constants 2001-2180 (first use, straight-line)
let objA0 = { a: 2001, b: 2002, c: 2003, d: 2004, e: 2005, f: 2006 };
let objA1 = { a: 2007, b: 2008, c: 2009, d: 2010, e: 2011, f: 2012 };
let objA2 = { a: 2013, b: 2014, c: 2015, d: 2016, e: 2017, f: 2018 };
let objA3 = { a: 2019, b: 2020, c: 2021, d: 2022, e: 2023, f: 2024 };
let objA4 = { a: 2025, b: 2026, c: 2027, d: 2028, e: 2029, f: 2030 };
let objA5 = { a: 2031, b: 2032, c: 2033, d: 2034, e: 2035, f: 2036 };
let objA6 = { a: 2037, b: 2038, c: 2039, d: 2040, e: 2041, f: 2042 };
let objA7 = { a: 2043, b: 2044, c: 2045, d: 2046, e: 2047, f: 2048 };
let objA8 = { a: 2049, b: 2050, c: 2051, d: 2052, e: 2053, f: 2054 };
let objA9 = { a: 2055, b: 2056, c: 2057, d: 2058, e: 2059, f: 2060 };
let objA10 = { a: 2061, b: 2062, c: 2063, d: 2064, e: 2065, f: 2066 };
let objA11 = { a: 2067, b: 2068, c: 2069, d: 2070, e: 2071, f: 2072 };
let objA12 = { a: 2073, b: 2074, c: 2075, d: 2076, e: 2077, f: 2078 };
let objA13 = { a: 2079, b: 2080, c: 2081, d: 2082, e: 2083, f: 2084 };
let objA14 = { a: 2085, b: 2086, c: 2087, d: 2088, e: 2089, f: 2090 };
let objA15 = { a: 2091, b: 2092, c: 2093, d: 2094, e: 2095, f: 2096 };
let objA16 = { a: 2097, b: 2098, c: 2099, d: 2100, e: 2101, f: 2102 };
let objA17 = { a: 2103, b: 2104, c: 2105, d: 2106, e: 2107, f: 2108 };
let objA18 = { a: 2109, b: 2110, c: 2111, d: 2112, e: 2113, f: 2114 };
let objA19 = { a: 2115, b: 2116, c: 2117, d: 2118, e: 2119, f: 2120 };
let objA20 = { a: 2121, b: 2122, c: 2123, d: 2124, e: 2125, f: 2126 };
let objA21 = { a: 2127, b: 2128, c: 2129, d: 2130, e: 2131, f: 2132 };
let objA22 = { a: 2133, b: 2134, c: 2135, d: 2136, e: 2137, f: 2138 };
let objA23 = { a: 2139, b: 2140, c: 2141, d: 2142, e: 2143, f: 2144 };
let objA24 = { a: 2145, b: 2146, c: 2147, d: 2148, e: 2149, f: 2150 };
let objA25 = { a: 2151, b: 2152, c: 2153, d: 2154, e: 2155, f: 2156 };
let objA26 = { a: 2157, b: 2158, c: 2159, d: 2160, e: 2161, f: 2162 };
let objA27 = { a: 2163, b: 2164, c: 2165, d: 2166, e: 2167, f: 2168 };
let objA28 = { a: 2169, b: 2170, c: 2171, d: 2172, e: 2173, f: 2174 };
let objA29 = { a: 2175, b: 2176, c: 2177, d: 2178, e: 2179, f: 2180 };

// GROUP B: Same 180 constants (second use, straight-line, creating long live ranges)
let objB0 = { a: 2001, b: 2002, c: 2003, d: 2004, e: 2005, f: 2006 };
let objB1 = { a: 2007, b: 2008, c: 2009, d: 2010, e: 2011, f: 2012 };
let objB2 = { a: 2013, b: 2014, c: 2015, d: 2016, e: 2017, f: 2018 };
let objB3 = { a: 2019, b: 2020, c: 2021, d: 2022, e: 2023, f: 2024 };
let objB4 = { a: 2025, b: 2026, c: 2027, d: 2028, e: 2029, f: 2030 };
let objB5 = { a: 2031, b: 2032, c: 2033, d: 2034, e: 2035, f: 2036 };
let objB6 = { a: 2037, b: 2038, c: 2039, d: 2040, e: 2041, f: 2042 };
let objB7 = { a: 2043, b: 2044, c: 2045, d: 2046, e: 2047, f: 2048 };
let objB8 = { a: 2049, b: 2050, c: 2051, d: 2052, e: 2053, f: 2054 };
let objB9 = { a: 2055, b: 2056, c: 2057, d: 2058, e: 2059, f: 2060 };
let objB10 = { a: 2061, b: 2062, c: 2063, d: 2064, e: 2065, f: 2066 };
let objB11 = { a: 2067, b: 2068, c: 2069, d: 2070, e: 2071, f: 2072 };
let objB12 = { a: 2073, b: 2074, c: 2075, d: 2076, e: 2077, f: 2078 };
let objB13 = { a: 2079, b: 2080, c: 2081, d: 2082, e: 2083, f: 2084 };
let objB14 = { a: 2085, b: 2086, c: 2087, d: 2088, e: 2089, f: 2090 };
let objB15 = { a: 2091, b: 2092, c: 2093, d: 2094, e: 2095, f: 2096 };
let objB16 = { a: 2097, b: 2098, c: 2099, d: 2100, e: 2101, f: 2102 };
let objB17 = { a: 2103, b: 2104, c: 2105, d: 2106, e: 2107, f: 2108 };
let objB18 = { a: 2109, b: 2110, c: 2111, d: 2112, e: 2113, f: 2114 };
let objB19 = { a: 2115, b: 2116, c: 2117, d: 2118, e: 2119, f: 2120 };
let objB20 = { a: 2121, b: 2122, c: 2123, d: 2124, e: 2125, f: 2126 };
let objB21 = { a: 2127, b: 2128, c: 2129, d: 2130, e: 2131, f: 2132 };
let objB22 = { a: 2133, b: 2134, c: 2135, d: 2136, e: 2137, f: 2138 };
let objB23 = { a: 2139, b: 2140, c: 2141, d: 2142, e: 2143, f: 2144 };
let objB24 = { a: 2145, b: 2146, c: 2147, d: 2148, e: 2149, f: 2150 };
let objB25 = { a: 2151, b: 2152, c: 2153, d: 2154, e: 2155, f: 2156 };
let objB26 = { a: 2157, b: 2158, c: 2159, d: 2160, e: 2161, f: 2162 };
let objB27 = { a: 2163, b: 2164, c: 2165, d: 2166, e: 2167, f: 2168 };
let objB28 = { a: 2169, b: 2170, c: 2171, d: 2172, e: 2173, f: 2174 };
let objB29 = { a: 2175, b: 2176, c: 2177, d: 2178, e: 2179, f: 2180 };

// GROUP C: Same 180 constants (third use, straight-line, kLongTailAggregate → 3 users)
let objC0 = { a: 2001, b: 2002, c: 2003, d: 2004, e: 2005, f: 2006 };
let objC1 = { a: 2007, b: 2008, c: 2009, d: 2010, e: 2011, f: 2012 };
let objC2 = { a: 2013, b: 2014, c: 2015, d: 2016, e: 2017, f: 2018 };
let objC3 = { a: 2019, b: 2020, c: 2021, d: 2022, e: 2023, f: 2024 };
let objC4 = { a: 2025, b: 2026, c: 2027, d: 2028, e: 2029, f: 2030 };
let objC5 = { a: 2031, b: 2032, c: 2033, d: 2034, e: 2035, f: 2036 };
let objC6 = { a: 2037, b: 2038, c: 2039, d: 2040, e: 2041, f: 2042 };
let objC7 = { a: 2043, b: 2044, c: 2045, d: 2046, e: 2047, f: 2048 };
let objC8 = { a: 2049, b: 2050, c: 2051, d: 2052, e: 2053, f: 2054 };
let objC9 = { a: 2055, b: 2056, c: 2057, d: 2058, e: 2059, f: 2060 };
let objC10 = { a: 2061, b: 2062, c: 2063, d: 2064, e: 2065, f: 2066 };
let objC11 = { a: 2067, b: 2068, c: 2069, d: 2070, e: 2071, f: 2072 };
let objC12 = { a: 2073, b: 2074, c: 2075, d: 2076, e: 2077, f: 2078 };
let objC13 = { a: 2079, b: 2080, c: 2081, d: 2082, e: 2083, f: 2084 };
let objC14 = { a: 2085, b: 2086, c: 2087, d: 2088, e: 2089, f: 2090 };
let objC15 = { a: 2091, b: 2092, c: 2093, d: 2094, e: 2095, f: 2096 };
let objC16 = { a: 2097, b: 2098, c: 2099, d: 2100, e: 2101, f: 2102 };
let objC17 = { a: 2103, b: 2104, c: 2105, d: 2106, e: 2107, f: 2108 };
let objC18 = { a: 2109, b: 2110, c: 2111, d: 2112, e: 2113, f: 2114 };
let objC19 = { a: 2115, b: 2116, c: 2117, d: 2118, e: 2119, f: 2120 };
let objC20 = { a: 2121, b: 2122, c: 2123, d: 2124, e: 2125, f: 2126 };
let objC21 = { a: 2127, b: 2128, c: 2129, d: 2130, e: 2131, f: 2132 };
let objC22 = { a: 2133, b: 2134, c: 2135, d: 2136, e: 2137, f: 2138 };
let objC23 = { a: 2139, b: 2140, c: 2141, d: 2142, e: 2143, f: 2144 };
let objC24 = { a: 2145, b: 2146, c: 2147, d: 2148, e: 2149, f: 2150 };
let objC25 = { a: 2151, b: 2152, c: 2153, d: 2154, e: 2155, f: 2156 };
let objC26 = { a: 2157, b: 2158, c: 2159, d: 2160, e: 2161, f: 2162 };
let objC27 = { a: 2163, b: 2164, c: 2165, d: 2166, e: 2167, f: 2168 };
let objC28 = { a: 2169, b: 2170, c: 2171, d: 2172, e: 2173, f: 2174 };
let objC29 = { a: 2175, b: 2176, c: 2177, d: 2178, e: 2179, f: 2180 };

let nodeA = __CreateView($currentComponentId);
__SetAttribute(nodeA, "style", objA0);
__AppendElement($root, nodeA);

// --- Conditional verification paths (tests correctness around branch boundaries) ---
let condA = true;
let condB = true;
let sumA = 0;
let sumB = 0;
let sumC = 0;

// Verify group A through condA branch
if (condA) {
  sumA = sumA + objA0.a + objA0.b + objA0.c + objA0.d + objA0.e + objA0.f;
  sumA = sumA + objA1.a + objA1.b + objA1.c + objA1.d + objA1.e + objA1.f;
  sumA = sumA + objA2.a + objA2.b + objA2.c + objA2.d + objA2.e + objA2.f;
  sumA = sumA + objA3.a + objA3.b + objA3.c + objA3.d + objA3.e + objA3.f;
  sumA = sumA + objA4.a + objA4.b + objA4.c + objA4.d + objA4.e + objA4.f;
  sumA = sumA + objA5.a + objA5.b + objA5.c + objA5.d + objA5.e + objA5.f;
  sumA = sumA + objA6.a + objA6.b + objA6.c + objA6.d + objA6.e + objA6.f;
  sumA = sumA + objA7.a + objA7.b + objA7.c + objA7.d + objA7.e + objA7.f;
  sumA = sumA + objA8.a + objA8.b + objA8.c + objA8.d + objA8.e + objA8.f;
  sumA = sumA + objA9.a + objA9.b + objA9.c + objA9.d + objA9.e + objA9.f;
  sumA = sumA + objA10.a + objA10.b + objA10.c + objA10.d + objA10.e + objA10.f;
  sumA = sumA + objA11.a + objA11.b + objA11.c + objA11.d + objA11.e + objA11.f;
  sumA = sumA + objA12.a + objA12.b + objA12.c + objA12.d + objA12.e + objA12.f;
  sumA = sumA + objA13.a + objA13.b + objA13.c + objA13.d + objA13.e + objA13.f;
  sumA = sumA + objA14.a + objA14.b + objA14.c + objA14.d + objA14.e + objA14.f;
  sumA = sumA + objA15.a + objA15.b + objA15.c + objA15.d + objA15.e + objA15.f;
  sumA = sumA + objA16.a + objA16.b + objA16.c + objA16.d + objA16.e + objA16.f;
  sumA = sumA + objA17.a + objA17.b + objA17.c + objA17.d + objA17.e + objA17.f;
  sumA = sumA + objA18.a + objA18.b + objA18.c + objA18.d + objA18.e + objA18.f;
  sumA = sumA + objA19.a + objA19.b + objA19.c + objA19.d + objA19.e + objA19.f;
  sumA = sumA + objA20.a + objA20.b + objA20.c + objA20.d + objA20.e + objA20.f;
  sumA = sumA + objA21.a + objA21.b + objA21.c + objA21.d + objA21.e + objA21.f;
  sumA = sumA + objA22.a + objA22.b + objA22.c + objA22.d + objA22.e + objA22.f;
  sumA = sumA + objA23.a + objA23.b + objA23.c + objA23.d + objA23.e + objA23.f;
  sumA = sumA + objA24.a + objA24.b + objA24.c + objA24.d + objA24.e + objA24.f;
  sumA = sumA + objA25.a + objA25.b + objA25.c + objA25.d + objA25.e + objA25.f;
  sumA = sumA + objA26.a + objA26.b + objA26.c + objA26.d + objA26.e + objA26.f;
  sumA = sumA + objA27.a + objA27.b + objA27.c + objA27.d + objA27.e + objA27.f;
  sumA = sumA + objA28.a + objA28.b + objA28.c + objA28.d + objA28.e + objA28.f;
  sumA = sumA + objA29.a + objA29.b + objA29.c + objA29.d + objA29.e + objA29.f;
}

// Verify group B through condB branch
if (condB) {
  sumB = sumB + objB0.a + objB0.b + objB0.c + objB0.d + objB0.e + objB0.f;
  sumB = sumB + objB1.a + objB1.b + objB1.c + objB1.d + objB1.e + objB1.f;
  sumB = sumB + objB2.a + objB2.b + objB2.c + objB2.d + objB2.e + objB2.f;
  sumB = sumB + objB3.a + objB3.b + objB3.c + objB3.d + objB3.e + objB3.f;
  sumB = sumB + objB4.a + objB4.b + objB4.c + objB4.d + objB4.e + objB4.f;
  sumB = sumB + objB5.a + objB5.b + objB5.c + objB5.d + objB5.e + objB5.f;
  sumB = sumB + objB6.a + objB6.b + objB6.c + objB6.d + objB6.e + objB6.f;
  sumB = sumB + objB7.a + objB7.b + objB7.c + objB7.d + objB7.e + objB7.f;
  sumB = sumB + objB8.a + objB8.b + objB8.c + objB8.d + objB8.e + objB8.f;
  sumB = sumB + objB9.a + objB9.b + objB9.c + objB9.d + objB9.e + objB9.f;
  sumB = sumB + objB10.a + objB10.b + objB10.c + objB10.d + objB10.e + objB10.f;
  sumB = sumB + objB11.a + objB11.b + objB11.c + objB11.d + objB11.e + objB11.f;
  sumB = sumB + objB12.a + objB12.b + objB12.c + objB12.d + objB12.e + objB12.f;
  sumB = sumB + objB13.a + objB13.b + objB13.c + objB13.d + objB13.e + objB13.f;
  sumB = sumB + objB14.a + objB14.b + objB14.c + objB14.d + objB14.e + objB14.f;
  sumB = sumB + objB15.a + objB15.b + objB15.c + objB15.d + objB15.e + objB15.f;
  sumB = sumB + objB16.a + objB16.b + objB16.c + objB16.d + objB16.e + objB16.f;
  sumB = sumB + objB17.a + objB17.b + objB17.c + objB17.d + objB17.e + objB17.f;
  sumB = sumB + objB18.a + objB18.b + objB18.c + objB18.d + objB18.e + objB18.f;
  sumB = sumB + objB19.a + objB19.b + objB19.c + objB19.d + objB19.e + objB19.f;
  sumB = sumB + objB20.a + objB20.b + objB20.c + objB20.d + objB20.e + objB20.f;
  sumB = sumB + objB21.a + objB21.b + objB21.c + objB21.d + objB21.e + objB21.f;
  sumB = sumB + objB22.a + objB22.b + objB22.c + objB22.d + objB22.e + objB22.f;
  sumB = sumB + objB23.a + objB23.b + objB23.c + objB23.d + objB23.e + objB23.f;
  sumB = sumB + objB24.a + objB24.b + objB24.c + objB24.d + objB24.e + objB24.f;
  sumB = sumB + objB25.a + objB25.b + objB25.c + objB25.d + objB25.e + objB25.f;
  sumB = sumB + objB26.a + objB26.b + objB26.c + objB26.d + objB26.e + objB26.f;
  sumB = sumB + objB27.a + objB27.b + objB27.c + objB27.d + objB27.e + objB27.f;
  sumB = sumB + objB28.a + objB28.b + objB28.c + objB28.d + objB28.e + objB28.f;
  sumB = sumB + objB29.a + objB29.b + objB29.c + objB29.d + objB29.e + objB29.f;
}

// Verify group C in straight-line (after branches, in merge block)
sumC = sumC + objC0.a + objC0.b + objC0.c + objC0.d + objC0.e + objC0.f;
sumC = sumC + objC1.a + objC1.b + objC1.c + objC1.d + objC1.e + objC1.f;
sumC = sumC + objC2.a + objC2.b + objC2.c + objC2.d + objC2.e + objC2.f;
sumC = sumC + objC3.a + objC3.b + objC3.c + objC3.d + objC3.e + objC3.f;
sumC = sumC + objC4.a + objC4.b + objC4.c + objC4.d + objC4.e + objC4.f;
sumC = sumC + objC5.a + objC5.b + objC5.c + objC5.d + objC5.e + objC5.f;
sumC = sumC + objC6.a + objC6.b + objC6.c + objC6.d + objC6.e + objC6.f;
sumC = sumC + objC7.a + objC7.b + objC7.c + objC7.d + objC7.e + objC7.f;
sumC = sumC + objC8.a + objC8.b + objC8.c + objC8.d + objC8.e + objC8.f;
sumC = sumC + objC9.a + objC9.b + objC9.c + objC9.d + objC9.e + objC9.f;
sumC = sumC + objC10.a + objC10.b + objC10.c + objC10.d + objC10.e + objC10.f;
sumC = sumC + objC11.a + objC11.b + objC11.c + objC11.d + objC11.e + objC11.f;
sumC = sumC + objC12.a + objC12.b + objC12.c + objC12.d + objC12.e + objC12.f;
sumC = sumC + objC13.a + objC13.b + objC13.c + objC13.d + objC13.e + objC13.f;
sumC = sumC + objC14.a + objC14.b + objC14.c + objC14.d + objC14.e + objC14.f;
sumC = sumC + objC15.a + objC15.b + objC15.c + objC15.d + objC15.e + objC15.f;
sumC = sumC + objC16.a + objC16.b + objC16.c + objC16.d + objC16.e + objC16.f;
sumC = sumC + objC17.a + objC17.b + objC17.c + objC17.d + objC17.e + objC17.f;
sumC = sumC + objC18.a + objC18.b + objC18.c + objC18.d + objC18.e + objC18.f;
sumC = sumC + objC19.a + objC19.b + objC19.c + objC19.d + objC19.e + objC19.f;
sumC = sumC + objC20.a + objC20.b + objC20.c + objC20.d + objC20.e + objC20.f;
sumC = sumC + objC21.a + objC21.b + objC21.c + objC21.d + objC21.e + objC21.f;
sumC = sumC + objC22.a + objC22.b + objC22.c + objC22.d + objC22.e + objC22.f;
sumC = sumC + objC23.a + objC23.b + objC23.c + objC23.d + objC23.e + objC23.f;
sumC = sumC + objC24.a + objC24.b + objC24.c + objC24.d + objC24.e + objC24.f;
sumC = sumC + objC25.a + objC25.b + objC25.c + objC25.d + objC25.e + objC25.f;
sumC = sumC + objC26.a + objC26.b + objC26.c + objC26.d + objC26.e + objC26.f;
sumC = sumC + objC27.a + objC27.b + objC27.c + objC27.d + objC27.e + objC27.f;
sumC = sumC + objC28.a + objC28.b + objC28.c + objC28.d + objC28.e + objC28.f;
sumC = sumC + objC29.a + objC29.b + objC29.c + objC29.d + objC29.e + objC29.f;

// Use prefix vars
let prefixSum = 0;
prefixSum = prefixSum + v0 + v1 + v2 + v3 + v4 + v5 + v6 + v7 + v8 + v9;
prefixSum = prefixSum + v10 + v11 + v12 + v13 + v14 + v15 + v16 + v17 + v18 + v19;
prefixSum = prefixSum + v20 + v21 + v22 + v23 + v24 + v25 + v26 + v27 + v28 + v29;
prefixSum = prefixSum + v30 + v31 + v32 + v33 + v34 + v35 + v36 + v37 + v38 + v39;
prefixSum = prefixSum + v40 + v41 + v42 + v43 + v44 + v45 + v46 + v47 + v48 + v49;
prefixSum = prefixSum + v50 + v51 + v52 + v53 + v54 + v55 + v56 + v57 + v58 + v59;
prefixSum = prefixSum + v60 + v61 + v62 + v63 + v64 + v65 + v66 + v67 + v68 + v69;
prefixSum = prefixSum + v70 + v71 + v72 + v73 + v74 + v75 + v76 + v77 + v78 + v79;
prefixSum = prefixSum + v80 + v81 + v82 + v83 + v84 + v85 + v86 + v87 + v88 + v89;

// Expected: sum of 2001..2180 = (2001+2180)*180/2 = 4181*90 = 376290
SoftAssert(sumA === 376290, "case12: sumA mismatch, got " + sumA);
SoftAssert(sumB === 376290, "case12: sumB mismatch, got " + sumB);
SoftAssert(sumC === 376290, "case12: sumC mismatch, got " + sumC);
SoftAssert(sumA === sumB, "case12: sumA/sumB should be equal");
SoftAssert(sumB === sumC, "case12: sumB/sumC should be equal");
SoftAssert(prefixSum === 4005, "case12: prefixSum mismatch, got " + prefixSum);

__FlushElementTree($page);
print("case12 sumA=" + sumA + " sumB=" + sumB + " sumC=" + sumC + " prefixSum=" + prefixSum);
Assert(__assert_fail_count === 0);
