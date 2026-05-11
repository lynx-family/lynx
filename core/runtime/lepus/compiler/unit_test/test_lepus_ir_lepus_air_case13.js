// [TEST_TARGET: IR]
// Focus: Hot aggregate rematerialization path (kHotAggregate priority)
// Strategy: 90 prefix vars + 180 unique constants (3001-3180)
// 4 groups of 30 objects each reuse the SAME 180 constants
// After CSE, each constant has 4 aggregate users (>=4) with span >= 64
// This triggers kHotAggregate priority (vs kLongTailAggregate in case11)

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

// GROUP A: First use of constants 3001-3180 (30 objects × 6 properties)
let gA0 = { a: 3001, b: 3002, c: 3003, d: 3004, e: 3005, f: 3006 };
let gA1 = { a: 3007, b: 3008, c: 3009, d: 3010, e: 3011, f: 3012 };
let gA2 = { a: 3013, b: 3014, c: 3015, d: 3016, e: 3017, f: 3018 };
let gA3 = { a: 3019, b: 3020, c: 3021, d: 3022, e: 3023, f: 3024 };
let gA4 = { a: 3025, b: 3026, c: 3027, d: 3028, e: 3029, f: 3030 };
let gA5 = { a: 3031, b: 3032, c: 3033, d: 3034, e: 3035, f: 3036 };
let gA6 = { a: 3037, b: 3038, c: 3039, d: 3040, e: 3041, f: 3042 };
let gA7 = { a: 3043, b: 3044, c: 3045, d: 3046, e: 3047, f: 3048 };
let gA8 = { a: 3049, b: 3050, c: 3051, d: 3052, e: 3053, f: 3054 };
let gA9 = { a: 3055, b: 3056, c: 3057, d: 3058, e: 3059, f: 3060 };
let gA10 = { a: 3061, b: 3062, c: 3063, d: 3064, e: 3065, f: 3066 };
let gA11 = { a: 3067, b: 3068, c: 3069, d: 3070, e: 3071, f: 3072 };
let gA12 = { a: 3073, b: 3074, c: 3075, d: 3076, e: 3077, f: 3078 };
let gA13 = { a: 3079, b: 3080, c: 3081, d: 3082, e: 3083, f: 3084 };
let gA14 = { a: 3085, b: 3086, c: 3087, d: 3088, e: 3089, f: 3090 };
let gA15 = { a: 3091, b: 3092, c: 3093, d: 3094, e: 3095, f: 3096 };
let gA16 = { a: 3097, b: 3098, c: 3099, d: 3100, e: 3101, f: 3102 };
let gA17 = { a: 3103, b: 3104, c: 3105, d: 3106, e: 3107, f: 3108 };
let gA18 = { a: 3109, b: 3110, c: 3111, d: 3112, e: 3113, f: 3114 };
let gA19 = { a: 3115, b: 3116, c: 3117, d: 3118, e: 3119, f: 3120 };
let gA20 = { a: 3121, b: 3122, c: 3123, d: 3124, e: 3125, f: 3126 };
let gA21 = { a: 3127, b: 3128, c: 3129, d: 3130, e: 3131, f: 3132 };
let gA22 = { a: 3133, b: 3134, c: 3135, d: 3136, e: 3137, f: 3138 };
let gA23 = { a: 3139, b: 3140, c: 3141, d: 3142, e: 3143, f: 3144 };
let gA24 = { a: 3145, b: 3146, c: 3147, d: 3148, e: 3149, f: 3150 };
let gA25 = { a: 3151, b: 3152, c: 3153, d: 3154, e: 3155, f: 3156 };
let gA26 = { a: 3157, b: 3158, c: 3159, d: 3160, e: 3161, f: 3162 };
let gA27 = { a: 3163, b: 3164, c: 3165, d: 3166, e: 3167, f: 3168 };
let gA28 = { a: 3169, b: 3170, c: 3171, d: 3172, e: 3173, f: 3174 };
let gA29 = { a: 3175, b: 3176, c: 3177, d: 3178, e: 3179, f: 3180 };

let nodeA = __CreateView($currentComponentId);
__SetAttribute(nodeA, "style", gA0);
__AppendElement($root, nodeA);

// GROUP B: Second use of same constants 3001-3180
let gB0 = { a: 3001, b: 3002, c: 3003, d: 3004, e: 3005, f: 3006 };
let gB1 = { a: 3007, b: 3008, c: 3009, d: 3010, e: 3011, f: 3012 };
let gB2 = { a: 3013, b: 3014, c: 3015, d: 3016, e: 3017, f: 3018 };
let gB3 = { a: 3019, b: 3020, c: 3021, d: 3022, e: 3023, f: 3024 };
let gB4 = { a: 3025, b: 3026, c: 3027, d: 3028, e: 3029, f: 3030 };
let gB5 = { a: 3031, b: 3032, c: 3033, d: 3034, e: 3035, f: 3036 };
let gB6 = { a: 3037, b: 3038, c: 3039, d: 3040, e: 3041, f: 3042 };
let gB7 = { a: 3043, b: 3044, c: 3045, d: 3046, e: 3047, f: 3048 };
let gB8 = { a: 3049, b: 3050, c: 3051, d: 3052, e: 3053, f: 3054 };
let gB9 = { a: 3055, b: 3056, c: 3057, d: 3058, e: 3059, f: 3060 };
let gB10 = { a: 3061, b: 3062, c: 3063, d: 3064, e: 3065, f: 3066 };
let gB11 = { a: 3067, b: 3068, c: 3069, d: 3070, e: 3071, f: 3072 };
let gB12 = { a: 3073, b: 3074, c: 3075, d: 3076, e: 3077, f: 3078 };
let gB13 = { a: 3079, b: 3080, c: 3081, d: 3082, e: 3083, f: 3084 };
let gB14 = { a: 3085, b: 3086, c: 3087, d: 3088, e: 3089, f: 3090 };
let gB15 = { a: 3091, b: 3092, c: 3093, d: 3094, e: 3095, f: 3096 };
let gB16 = { a: 3097, b: 3098, c: 3099, d: 3100, e: 3101, f: 3102 };
let gB17 = { a: 3103, b: 3104, c: 3105, d: 3106, e: 3107, f: 3108 };
let gB18 = { a: 3109, b: 3110, c: 3111, d: 3112, e: 3113, f: 3114 };
let gB19 = { a: 3115, b: 3116, c: 3117, d: 3118, e: 3119, f: 3120 };
let gB20 = { a: 3121, b: 3122, c: 3123, d: 3124, e: 3125, f: 3126 };
let gB21 = { a: 3127, b: 3128, c: 3129, d: 3130, e: 3131, f: 3132 };
let gB22 = { a: 3133, b: 3134, c: 3135, d: 3136, e: 3137, f: 3138 };
let gB23 = { a: 3139, b: 3140, c: 3141, d: 3142, e: 3143, f: 3144 };
let gB24 = { a: 3145, b: 3146, c: 3147, d: 3148, e: 3149, f: 3150 };
let gB25 = { a: 3151, b: 3152, c: 3153, d: 3154, e: 3155, f: 3156 };
let gB26 = { a: 3157, b: 3158, c: 3159, d: 3160, e: 3161, f: 3162 };
let gB27 = { a: 3163, b: 3164, c: 3165, d: 3166, e: 3167, f: 3168 };
let gB28 = { a: 3169, b: 3170, c: 3171, d: 3172, e: 3173, f: 3174 };
let gB29 = { a: 3175, b: 3176, c: 3177, d: 3178, e: 3179, f: 3180 };

let nodeB = __CreateView($currentComponentId);
__SetAttribute(nodeB, "layout", gB0);
__AppendElement($root, nodeB);

// GROUP C: Third use of same constants 3001-3180
let gC0 = { a: 3001, b: 3002, c: 3003, d: 3004, e: 3005, f: 3006 };
let gC1 = { a: 3007, b: 3008, c: 3009, d: 3010, e: 3011, f: 3012 };
let gC2 = { a: 3013, b: 3014, c: 3015, d: 3016, e: 3017, f: 3018 };
let gC3 = { a: 3019, b: 3020, c: 3021, d: 3022, e: 3023, f: 3024 };
let gC4 = { a: 3025, b: 3026, c: 3027, d: 3028, e: 3029, f: 3030 };
let gC5 = { a: 3031, b: 3032, c: 3033, d: 3034, e: 3035, f: 3036 };
let gC6 = { a: 3037, b: 3038, c: 3039, d: 3040, e: 3041, f: 3042 };
let gC7 = { a: 3043, b: 3044, c: 3045, d: 3046, e: 3047, f: 3048 };
let gC8 = { a: 3049, b: 3050, c: 3051, d: 3052, e: 3053, f: 3054 };
let gC9 = { a: 3055, b: 3056, c: 3057, d: 3058, e: 3059, f: 3060 };
let gC10 = { a: 3061, b: 3062, c: 3063, d: 3064, e: 3065, f: 3066 };
let gC11 = { a: 3067, b: 3068, c: 3069, d: 3070, e: 3071, f: 3072 };
let gC12 = { a: 3073, b: 3074, c: 3075, d: 3076, e: 3077, f: 3078 };
let gC13 = { a: 3079, b: 3080, c: 3081, d: 3082, e: 3083, f: 3084 };
let gC14 = { a: 3085, b: 3086, c: 3087, d: 3088, e: 3089, f: 3090 };
let gC15 = { a: 3091, b: 3092, c: 3093, d: 3094, e: 3095, f: 3096 };
let gC16 = { a: 3097, b: 3098, c: 3099, d: 3100, e: 3101, f: 3102 };
let gC17 = { a: 3103, b: 3104, c: 3105, d: 3106, e: 3107, f: 3108 };
let gC18 = { a: 3109, b: 3110, c: 3111, d: 3112, e: 3113, f: 3114 };
let gC19 = { a: 3115, b: 3116, c: 3117, d: 3118, e: 3119, f: 3120 };
let gC20 = { a: 3121, b: 3122, c: 3123, d: 3124, e: 3125, f: 3126 };
let gC21 = { a: 3127, b: 3128, c: 3129, d: 3130, e: 3131, f: 3132 };
let gC22 = { a: 3133, b: 3134, c: 3135, d: 3136, e: 3137, f: 3138 };
let gC23 = { a: 3139, b: 3140, c: 3141, d: 3142, e: 3143, f: 3144 };
let gC24 = { a: 3145, b: 3146, c: 3147, d: 3148, e: 3149, f: 3150 };
let gC25 = { a: 3151, b: 3152, c: 3153, d: 3154, e: 3155, f: 3156 };
let gC26 = { a: 3157, b: 3158, c: 3159, d: 3160, e: 3161, f: 3162 };
let gC27 = { a: 3163, b: 3164, c: 3165, d: 3166, e: 3167, f: 3168 };
let gC28 = { a: 3169, b: 3170, c: 3171, d: 3172, e: 3173, f: 3174 };
let gC29 = { a: 3175, b: 3176, c: 3177, d: 3178, e: 3179, f: 3180 };

let nodeC = __CreateView($currentComponentId);
__SetAttribute(nodeC, "data", gC0);
__AppendElement($root, nodeC);

// GROUP D: Fourth use of same constants 3001-3180
let gD0 = { a: 3001, b: 3002, c: 3003, d: 3004, e: 3005, f: 3006 };
let gD1 = { a: 3007, b: 3008, c: 3009, d: 3010, e: 3011, f: 3012 };
let gD2 = { a: 3013, b: 3014, c: 3015, d: 3016, e: 3017, f: 3018 };
let gD3 = { a: 3019, b: 3020, c: 3021, d: 3022, e: 3023, f: 3024 };
let gD4 = { a: 3025, b: 3026, c: 3027, d: 3028, e: 3029, f: 3030 };
let gD5 = { a: 3031, b: 3032, c: 3033, d: 3034, e: 3035, f: 3036 };
let gD6 = { a: 3037, b: 3038, c: 3039, d: 3040, e: 3041, f: 3042 };
let gD7 = { a: 3043, b: 3044, c: 3045, d: 3046, e: 3047, f: 3048 };
let gD8 = { a: 3049, b: 3050, c: 3051, d: 3052, e: 3053, f: 3054 };
let gD9 = { a: 3055, b: 3056, c: 3057, d: 3058, e: 3059, f: 3060 };
let gD10 = { a: 3061, b: 3062, c: 3063, d: 3064, e: 3065, f: 3066 };
let gD11 = { a: 3067, b: 3068, c: 3069, d: 3070, e: 3071, f: 3072 };
let gD12 = { a: 3073, b: 3074, c: 3075, d: 3076, e: 3077, f: 3078 };
let gD13 = { a: 3079, b: 3080, c: 3081, d: 3082, e: 3083, f: 3084 };
let gD14 = { a: 3085, b: 3086, c: 3087, d: 3088, e: 3089, f: 3090 };
let gD15 = { a: 3091, b: 3092, c: 3093, d: 3094, e: 3095, f: 3096 };
let gD16 = { a: 3097, b: 3098, c: 3099, d: 3100, e: 3101, f: 3102 };
let gD17 = { a: 3103, b: 3104, c: 3105, d: 3106, e: 3107, f: 3108 };
let gD18 = { a: 3109, b: 3110, c: 3111, d: 3112, e: 3113, f: 3114 };
let gD19 = { a: 3115, b: 3116, c: 3117, d: 3118, e: 3119, f: 3120 };
let gD20 = { a: 3121, b: 3122, c: 3123, d: 3124, e: 3125, f: 3126 };
let gD21 = { a: 3127, b: 3128, c: 3129, d: 3130, e: 3131, f: 3132 };
let gD22 = { a: 3133, b: 3134, c: 3135, d: 3136, e: 3137, f: 3138 };
let gD23 = { a: 3139, b: 3140, c: 3141, d: 3142, e: 3143, f: 3144 };
let gD24 = { a: 3145, b: 3146, c: 3147, d: 3148, e: 3149, f: 3150 };
let gD25 = { a: 3151, b: 3152, c: 3153, d: 3154, e: 3155, f: 3156 };
let gD26 = { a: 3157, b: 3158, c: 3159, d: 3160, e: 3161, f: 3162 };
let gD27 = { a: 3163, b: 3164, c: 3165, d: 3166, e: 3167, f: 3168 };
let gD28 = { a: 3169, b: 3170, c: 3171, d: 3172, e: 3173, f: 3174 };
let gD29 = { a: 3175, b: 3176, c: 3177, d: 3178, e: 3179, f: 3180 };

let nodeD = __CreateView($currentComponentId);
__SetAttribute(nodeD, "config", gD0);
__AppendElement($root, nodeD);

// --- Checksum: verify all 4 groups ---
// Expected per-group sum: sum of 3001..3180 = (3001+3180)*180/2 = 6181*90 = 556290
let sumA = 0;
sumA = sumA + gA0.a + gA0.b + gA0.c + gA0.d + gA0.e + gA0.f;
sumA = sumA + gA1.a + gA1.b + gA1.c + gA1.d + gA1.e + gA1.f;
sumA = sumA + gA2.a + gA2.b + gA2.c + gA2.d + gA2.e + gA2.f;
sumA = sumA + gA3.a + gA3.b + gA3.c + gA3.d + gA3.e + gA3.f;
sumA = sumA + gA4.a + gA4.b + gA4.c + gA4.d + gA4.e + gA4.f;
sumA = sumA + gA5.a + gA5.b + gA5.c + gA5.d + gA5.e + gA5.f;
sumA = sumA + gA6.a + gA6.b + gA6.c + gA6.d + gA6.e + gA6.f;
sumA = sumA + gA7.a + gA7.b + gA7.c + gA7.d + gA7.e + gA7.f;
sumA = sumA + gA8.a + gA8.b + gA8.c + gA8.d + gA8.e + gA8.f;
sumA = sumA + gA9.a + gA9.b + gA9.c + gA9.d + gA9.e + gA9.f;
sumA = sumA + gA10.a + gA10.b + gA10.c + gA10.d + gA10.e + gA10.f;
sumA = sumA + gA11.a + gA11.b + gA11.c + gA11.d + gA11.e + gA11.f;
sumA = sumA + gA12.a + gA12.b + gA12.c + gA12.d + gA12.e + gA12.f;
sumA = sumA + gA13.a + gA13.b + gA13.c + gA13.d + gA13.e + gA13.f;
sumA = sumA + gA14.a + gA14.b + gA14.c + gA14.d + gA14.e + gA14.f;
sumA = sumA + gA15.a + gA15.b + gA15.c + gA15.d + gA15.e + gA15.f;
sumA = sumA + gA16.a + gA16.b + gA16.c + gA16.d + gA16.e + gA16.f;
sumA = sumA + gA17.a + gA17.b + gA17.c + gA17.d + gA17.e + gA17.f;
sumA = sumA + gA18.a + gA18.b + gA18.c + gA18.d + gA18.e + gA18.f;
sumA = sumA + gA19.a + gA19.b + gA19.c + gA19.d + gA19.e + gA19.f;
sumA = sumA + gA20.a + gA20.b + gA20.c + gA20.d + gA20.e + gA20.f;
sumA = sumA + gA21.a + gA21.b + gA21.c + gA21.d + gA21.e + gA21.f;
sumA = sumA + gA22.a + gA22.b + gA22.c + gA22.d + gA22.e + gA22.f;
sumA = sumA + gA23.a + gA23.b + gA23.c + gA23.d + gA23.e + gA23.f;
sumA = sumA + gA24.a + gA24.b + gA24.c + gA24.d + gA24.e + gA24.f;
sumA = sumA + gA25.a + gA25.b + gA25.c + gA25.d + gA25.e + gA25.f;
sumA = sumA + gA26.a + gA26.b + gA26.c + gA26.d + gA26.e + gA26.f;
sumA = sumA + gA27.a + gA27.b + gA27.c + gA27.d + gA27.e + gA27.f;
sumA = sumA + gA28.a + gA28.b + gA28.c + gA28.d + gA28.e + gA28.f;
sumA = sumA + gA29.a + gA29.b + gA29.c + gA29.d + gA29.e + gA29.f;

let sumD = 0;
sumD = sumD + gD0.a + gD0.b + gD0.c + gD0.d + gD0.e + gD0.f;
sumD = sumD + gD1.a + gD1.b + gD1.c + gD1.d + gD1.e + gD1.f;
sumD = sumD + gD2.a + gD2.b + gD2.c + gD2.d + gD2.e + gD2.f;
sumD = sumD + gD3.a + gD3.b + gD3.c + gD3.d + gD3.e + gD3.f;
sumD = sumD + gD4.a + gD4.b + gD4.c + gD4.d + gD4.e + gD4.f;
sumD = sumD + gD5.a + gD5.b + gD5.c + gD5.d + gD5.e + gD5.f;
sumD = sumD + gD6.a + gD6.b + gD6.c + gD6.d + gD6.e + gD6.f;
sumD = sumD + gD7.a + gD7.b + gD7.c + gD7.d + gD7.e + gD7.f;
sumD = sumD + gD8.a + gD8.b + gD8.c + gD8.d + gD8.e + gD8.f;
sumD = sumD + gD9.a + gD9.b + gD9.c + gD9.d + gD9.e + gD9.f;
sumD = sumD + gD10.a + gD10.b + gD10.c + gD10.d + gD10.e + gD10.f;
sumD = sumD + gD11.a + gD11.b + gD11.c + gD11.d + gD11.e + gD11.f;
sumD = sumD + gD12.a + gD12.b + gD12.c + gD12.d + gD12.e + gD12.f;
sumD = sumD + gD13.a + gD13.b + gD13.c + gD13.d + gD13.e + gD13.f;
sumD = sumD + gD14.a + gD14.b + gD14.c + gD14.d + gD14.e + gD14.f;
sumD = sumD + gD15.a + gD15.b + gD15.c + gD15.d + gD15.e + gD15.f;
sumD = sumD + gD16.a + gD16.b + gD16.c + gD16.d + gD16.e + gD16.f;
sumD = sumD + gD17.a + gD17.b + gD17.c + gD17.d + gD17.e + gD17.f;
sumD = sumD + gD18.a + gD18.b + gD18.c + gD18.d + gD18.e + gD18.f;
sumD = sumD + gD19.a + gD19.b + gD19.c + gD19.d + gD19.e + gD19.f;
sumD = sumD + gD20.a + gD20.b + gD20.c + gD20.d + gD20.e + gD20.f;
sumD = sumD + gD21.a + gD21.b + gD21.c + gD21.d + gD21.e + gD21.f;
sumD = sumD + gD22.a + gD22.b + gD22.c + gD22.d + gD22.e + gD22.f;
sumD = sumD + gD23.a + gD23.b + gD23.c + gD23.d + gD23.e + gD23.f;
sumD = sumD + gD24.a + gD24.b + gD24.c + gD24.d + gD24.e + gD24.f;
sumD = sumD + gD25.a + gD25.b + gD25.c + gD25.d + gD25.e + gD25.f;
sumD = sumD + gD26.a + gD26.b + gD26.c + gD26.d + gD26.e + gD26.f;
sumD = sumD + gD27.a + gD27.b + gD27.c + gD27.d + gD27.e + gD27.f;
sumD = sumD + gD28.a + gD28.b + gD28.c + gD28.d + gD28.e + gD28.f;
sumD = sumD + gD29.a + gD29.b + gD29.c + gD29.d + gD29.e + gD29.f;

SoftAssert(sumA === 556290, "case13: sumA mismatch, got " + sumA);
SoftAssert(sumD === 556290, "case13: sumD mismatch, got " + sumD);
SoftAssert(sumA === sumD, "case13: sumA/sumD mismatch");

// Verify prefix variables: sum 0..89 = 89*90/2 = 4005
let varSum = v0 + v1 + v2 + v3 + v4 + v5 + v6 + v7 + v8 + v9;
varSum = varSum + v10 + v11 + v12 + v13 + v14 + v15 + v16 + v17 + v18 + v19;
varSum = varSum + v20 + v21 + v22 + v23 + v24 + v25 + v26 + v27 + v28 + v29;
varSum = varSum + v30 + v31 + v32 + v33 + v34 + v35 + v36 + v37 + v38 + v39;
varSum = varSum + v40 + v41 + v42 + v43 + v44 + v45 + v46 + v47 + v48 + v49;
varSum = varSum + v50 + v51 + v52 + v53 + v54 + v55 + v56 + v57 + v58 + v59;
varSum = varSum + v60 + v61 + v62 + v63 + v64 + v65 + v66 + v67 + v68 + v69;
varSum = varSum + v70 + v71 + v72 + v73 + v74 + v75 + v76 + v77 + v78 + v79;
varSum = varSum + v80 + v81 + v82 + v83 + v84 + v85 + v86 + v87 + v88 + v89;
SoftAssert(varSum === 4005, "case13: varSum mismatch, got " + varSum);

__FlushElementTree($page);
print("case13 sumA=" + sumA + " sumD=" + sumD + " varSum=" + varSum);
Assert(__assert_fail_count === 0);
