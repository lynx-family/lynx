// [TEST_TARGET: IR]
// Focus: Hot aggregate rematerialization path
// Strategy: Use 180+ unique constant values each appearing in two groups of objects
// (early and late). After CSE, all 180 LoadConst live simultaneously -> overflow > 256
// Rematerialization pass clones them closer to usage to reduce live ranges.

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

// GROUP EARLY: 30 objects using unique constants 1001-1180
// Each object uses 6 unique constants, total 180 unique values
let earlyA0 = { a: 1001, b: 1002, c: 1003, d: 1004, e: 1005, f: 1006 };
let earlyA1 = { a: 1007, b: 1008, c: 1009, d: 1010, e: 1011, f: 1012 };
let earlyA2 = { a: 1013, b: 1014, c: 1015, d: 1016, e: 1017, f: 1018 };
let earlyA3 = { a: 1019, b: 1020, c: 1021, d: 1022, e: 1023, f: 1024 };
let earlyA4 = { a: 1025, b: 1026, c: 1027, d: 1028, e: 1029, f: 1030 };
let earlyA5 = { a: 1031, b: 1032, c: 1033, d: 1034, e: 1035, f: 1036 };
let earlyA6 = { a: 1037, b: 1038, c: 1039, d: 1040, e: 1041, f: 1042 };
let earlyA7 = { a: 1043, b: 1044, c: 1045, d: 1046, e: 1047, f: 1048 };
let earlyA8 = { a: 1049, b: 1050, c: 1051, d: 1052, e: 1053, f: 1054 };
let earlyA9 = { a: 1055, b: 1056, c: 1057, d: 1058, e: 1059, f: 1060 };
let earlyA10 = { a: 1061, b: 1062, c: 1063, d: 1064, e: 1065, f: 1066 };
let earlyA11 = { a: 1067, b: 1068, c: 1069, d: 1070, e: 1071, f: 1072 };
let earlyA12 = { a: 1073, b: 1074, c: 1075, d: 1076, e: 1077, f: 1078 };
let earlyA13 = { a: 1079, b: 1080, c: 1081, d: 1082, e: 1083, f: 1084 };
let earlyA14 = { a: 1085, b: 1086, c: 1087, d: 1088, e: 1089, f: 1090 };
let earlyA15 = { a: 1091, b: 1092, c: 1093, d: 1094, e: 1095, f: 1096 };
let earlyA16 = { a: 1097, b: 1098, c: 1099, d: 1100, e: 1101, f: 1102 };
let earlyA17 = { a: 1103, b: 1104, c: 1105, d: 1106, e: 1107, f: 1108 };
let earlyA18 = { a: 1109, b: 1110, c: 1111, d: 1112, e: 1113, f: 1114 };
let earlyA19 = { a: 1115, b: 1116, c: 1117, d: 1118, e: 1119, f: 1120 };
let earlyA20 = { a: 1121, b: 1122, c: 1123, d: 1124, e: 1125, f: 1126 };
let earlyA21 = { a: 1127, b: 1128, c: 1129, d: 1130, e: 1131, f: 1132 };
let earlyA22 = { a: 1133, b: 1134, c: 1135, d: 1136, e: 1137, f: 1138 };
let earlyA23 = { a: 1139, b: 1140, c: 1141, d: 1142, e: 1143, f: 1144 };
let earlyA24 = { a: 1145, b: 1146, c: 1147, d: 1148, e: 1149, f: 1150 };
let earlyA25 = { a: 1151, b: 1152, c: 1153, d: 1154, e: 1155, f: 1156 };
let earlyA26 = { a: 1157, b: 1158, c: 1159, d: 1160, e: 1161, f: 1162 };
let earlyA27 = { a: 1163, b: 1164, c: 1165, d: 1166, e: 1167, f: 1168 };
let earlyA28 = { a: 1169, b: 1170, c: 1171, d: 1172, e: 1173, f: 1174 };
let earlyA29 = { a: 1175, b: 1176, c: 1177, d: 1178, e: 1179, f: 1180 };

let nodeE = __CreateView($currentComponentId);
__SetAttribute(nodeE, "style", earlyA0);
__AppendElement($root, nodeE);

// MIDDLE: some DOM operations + arithmetic to widen the gap
let midSum = 0;
midSum = midSum + v0 + v1 + v2 + v3 + v4 + v5 + v6 + v7 + v8 + v9;
midSum = midSum + v10 + v11 + v12 + v13 + v14 + v15 + v16 + v17 + v18 + v19;
midSum = midSum + v20 + v21 + v22 + v23 + v24 + v25 + v26 + v27 + v28 + v29;
midSum = midSum + v30 + v31 + v32 + v33 + v34 + v35 + v36 + v37 + v38 + v39;
midSum = midSum + v40 + v41 + v42 + v43 + v44 + v45 + v46 + v47 + v48 + v49;
midSum = midSum + v50 + v51 + v52 + v53 + v54 + v55 + v56 + v57 + v58 + v59;
midSum = midSum + v60 + v61 + v62 + v63 + v64 + v65 + v66 + v67 + v68 + v69;
midSum = midSum + v70 + v71 + v72 + v73 + v74 + v75 + v76 + v77 + v78 + v79;
midSum = midSum + v80 + v81 + v82 + v83 + v84 + v85 + v86 + v87 + v88 + v89;

let midNode0 = __CreateView($currentComponentId);
let midNode1 = __CreateView($currentComponentId);
let midNode2 = __CreateView($currentComponentId);
__AppendElement($root, midNode0);
__AppendElement($root, midNode1);
__AppendElement($root, midNode2);
__SetAttribute(midNode0, "data-sum", midSum);

// GROUP LATE: 30 objects using THE SAME unique constants 1001-1180
// After CSE, each constant is merged with the early group -> long live range
let lateB0 = { a: 1001, b: 1002, c: 1003, d: 1004, e: 1005, f: 1006 };
let lateB1 = { a: 1007, b: 1008, c: 1009, d: 1010, e: 1011, f: 1012 };
let lateB2 = { a: 1013, b: 1014, c: 1015, d: 1016, e: 1017, f: 1018 };
let lateB3 = { a: 1019, b: 1020, c: 1021, d: 1022, e: 1023, f: 1024 };
let lateB4 = { a: 1025, b: 1026, c: 1027, d: 1028, e: 1029, f: 1030 };
let lateB5 = { a: 1031, b: 1032, c: 1033, d: 1034, e: 1035, f: 1036 };
let lateB6 = { a: 1037, b: 1038, c: 1039, d: 1040, e: 1041, f: 1042 };
let lateB7 = { a: 1043, b: 1044, c: 1045, d: 1046, e: 1047, f: 1048 };
let lateB8 = { a: 1049, b: 1050, c: 1051, d: 1052, e: 1053, f: 1054 };
let lateB9 = { a: 1055, b: 1056, c: 1057, d: 1058, e: 1059, f: 1060 };
let lateB10 = { a: 1061, b: 1062, c: 1063, d: 1064, e: 1065, f: 1066 };
let lateB11 = { a: 1067, b: 1068, c: 1069, d: 1070, e: 1071, f: 1072 };
let lateB12 = { a: 1073, b: 1074, c: 1075, d: 1076, e: 1077, f: 1078 };
let lateB13 = { a: 1079, b: 1080, c: 1081, d: 1082, e: 1083, f: 1084 };
let lateB14 = { a: 1085, b: 1086, c: 1087, d: 1088, e: 1089, f: 1090 };
let lateB15 = { a: 1091, b: 1092, c: 1093, d: 1094, e: 1095, f: 1096 };
let lateB16 = { a: 1097, b: 1098, c: 1099, d: 1100, e: 1101, f: 1102 };
let lateB17 = { a: 1103, b: 1104, c: 1105, d: 1106, e: 1107, f: 1108 };
let lateB18 = { a: 1109, b: 1110, c: 1111, d: 1112, e: 1113, f: 1114 };
let lateB19 = { a: 1115, b: 1116, c: 1117, d: 1118, e: 1119, f: 1120 };
let lateB20 = { a: 1121, b: 1122, c: 1123, d: 1124, e: 1125, f: 1126 };
let lateB21 = { a: 1127, b: 1128, c: 1129, d: 1130, e: 1131, f: 1132 };
let lateB22 = { a: 1133, b: 1134, c: 1135, d: 1136, e: 1137, f: 1138 };
let lateB23 = { a: 1139, b: 1140, c: 1141, d: 1142, e: 1143, f: 1144 };
let lateB24 = { a: 1145, b: 1146, c: 1147, d: 1148, e: 1149, f: 1150 };
let lateB25 = { a: 1151, b: 1152, c: 1153, d: 1154, e: 1155, f: 1156 };
let lateB26 = { a: 1157, b: 1158, c: 1159, d: 1160, e: 1161, f: 1162 };
let lateB27 = { a: 1163, b: 1164, c: 1165, d: 1166, e: 1167, f: 1168 };
let lateB28 = { a: 1169, b: 1170, c: 1171, d: 1172, e: 1173, f: 1174 };
let lateB29 = { a: 1175, b: 1176, c: 1177, d: 1178, e: 1179, f: 1180 };

let nodeL = __CreateView($currentComponentId);
__SetAttribute(nodeL, "style", lateB0);
__AppendElement($root, nodeL);

// --- Checksum: verify early and late groups have correct values ---
let earlySum = 0;
earlySum = earlySum + earlyA0.a + earlyA0.b + earlyA0.c + earlyA0.d + earlyA0.e + earlyA0.f;
earlySum = earlySum + earlyA1.a + earlyA1.b + earlyA1.c + earlyA1.d + earlyA1.e + earlyA1.f;
earlySum = earlySum + earlyA2.a + earlyA2.b + earlyA2.c + earlyA2.d + earlyA2.e + earlyA2.f;
earlySum = earlySum + earlyA3.a + earlyA3.b + earlyA3.c + earlyA3.d + earlyA3.e + earlyA3.f;
earlySum = earlySum + earlyA4.a + earlyA4.b + earlyA4.c + earlyA4.d + earlyA4.e + earlyA4.f;
earlySum = earlySum + earlyA5.a + earlyA5.b + earlyA5.c + earlyA5.d + earlyA5.e + earlyA5.f;
earlySum = earlySum + earlyA6.a + earlyA6.b + earlyA6.c + earlyA6.d + earlyA6.e + earlyA6.f;
earlySum = earlySum + earlyA7.a + earlyA7.b + earlyA7.c + earlyA7.d + earlyA7.e + earlyA7.f;
earlySum = earlySum + earlyA8.a + earlyA8.b + earlyA8.c + earlyA8.d + earlyA8.e + earlyA8.f;
earlySum = earlySum + earlyA9.a + earlyA9.b + earlyA9.c + earlyA9.d + earlyA9.e + earlyA9.f;
earlySum = earlySum + earlyA10.a + earlyA10.b + earlyA10.c + earlyA10.d + earlyA10.e + earlyA10.f;
earlySum = earlySum + earlyA11.a + earlyA11.b + earlyA11.c + earlyA11.d + earlyA11.e + earlyA11.f;
earlySum = earlySum + earlyA12.a + earlyA12.b + earlyA12.c + earlyA12.d + earlyA12.e + earlyA12.f;
earlySum = earlySum + earlyA13.a + earlyA13.b + earlyA13.c + earlyA13.d + earlyA13.e + earlyA13.f;
earlySum = earlySum + earlyA14.a + earlyA14.b + earlyA14.c + earlyA14.d + earlyA14.e + earlyA14.f;
earlySum = earlySum + earlyA15.a + earlyA15.b + earlyA15.c + earlyA15.d + earlyA15.e + earlyA15.f;
earlySum = earlySum + earlyA16.a + earlyA16.b + earlyA16.c + earlyA16.d + earlyA16.e + earlyA16.f;
earlySum = earlySum + earlyA17.a + earlyA17.b + earlyA17.c + earlyA17.d + earlyA17.e + earlyA17.f;
earlySum = earlySum + earlyA18.a + earlyA18.b + earlyA18.c + earlyA18.d + earlyA18.e + earlyA18.f;
earlySum = earlySum + earlyA19.a + earlyA19.b + earlyA19.c + earlyA19.d + earlyA19.e + earlyA19.f;
earlySum = earlySum + earlyA20.a + earlyA20.b + earlyA20.c + earlyA20.d + earlyA20.e + earlyA20.f;
earlySum = earlySum + earlyA21.a + earlyA21.b + earlyA21.c + earlyA21.d + earlyA21.e + earlyA21.f;
earlySum = earlySum + earlyA22.a + earlyA22.b + earlyA22.c + earlyA22.d + earlyA22.e + earlyA22.f;
earlySum = earlySum + earlyA23.a + earlyA23.b + earlyA23.c + earlyA23.d + earlyA23.e + earlyA23.f;
earlySum = earlySum + earlyA24.a + earlyA24.b + earlyA24.c + earlyA24.d + earlyA24.e + earlyA24.f;
earlySum = earlySum + earlyA25.a + earlyA25.b + earlyA25.c + earlyA25.d + earlyA25.e + earlyA25.f;
earlySum = earlySum + earlyA26.a + earlyA26.b + earlyA26.c + earlyA26.d + earlyA26.e + earlyA26.f;
earlySum = earlySum + earlyA27.a + earlyA27.b + earlyA27.c + earlyA27.d + earlyA27.e + earlyA27.f;
earlySum = earlySum + earlyA28.a + earlyA28.b + earlyA28.c + earlyA28.d + earlyA28.e + earlyA28.f;
earlySum = earlySum + earlyA29.a + earlyA29.b + earlyA29.c + earlyA29.d + earlyA29.e + earlyA29.f;

let lateSum = 0;
lateSum = lateSum + lateB0.a + lateB0.b + lateB0.c + lateB0.d + lateB0.e + lateB0.f;
lateSum = lateSum + lateB1.a + lateB1.b + lateB1.c + lateB1.d + lateB1.e + lateB1.f;
lateSum = lateSum + lateB2.a + lateB2.b + lateB2.c + lateB2.d + lateB2.e + lateB2.f;
lateSum = lateSum + lateB3.a + lateB3.b + lateB3.c + lateB3.d + lateB3.e + lateB3.f;
lateSum = lateSum + lateB4.a + lateB4.b + lateB4.c + lateB4.d + lateB4.e + lateB4.f;
lateSum = lateSum + lateB5.a + lateB5.b + lateB5.c + lateB5.d + lateB5.e + lateB5.f;
lateSum = lateSum + lateB6.a + lateB6.b + lateB6.c + lateB6.d + lateB6.e + lateB6.f;
lateSum = lateSum + lateB7.a + lateB7.b + lateB7.c + lateB7.d + lateB7.e + lateB7.f;
lateSum = lateSum + lateB8.a + lateB8.b + lateB8.c + lateB8.d + lateB8.e + lateB8.f;
lateSum = lateSum + lateB9.a + lateB9.b + lateB9.c + lateB9.d + lateB9.e + lateB9.f;
lateSum = lateSum + lateB10.a + lateB10.b + lateB10.c + lateB10.d + lateB10.e + lateB10.f;
lateSum = lateSum + lateB11.a + lateB11.b + lateB11.c + lateB11.d + lateB11.e + lateB11.f;
lateSum = lateSum + lateB12.a + lateB12.b + lateB12.c + lateB12.d + lateB12.e + lateB12.f;
lateSum = lateSum + lateB13.a + lateB13.b + lateB13.c + lateB13.d + lateB13.e + lateB13.f;
lateSum = lateSum + lateB14.a + lateB14.b + lateB14.c + lateB14.d + lateB14.e + lateB14.f;
lateSum = lateSum + lateB15.a + lateB15.b + lateB15.c + lateB15.d + lateB15.e + lateB15.f;
lateSum = lateSum + lateB16.a + lateB16.b + lateB16.c + lateB16.d + lateB16.e + lateB16.f;
lateSum = lateSum + lateB17.a + lateB17.b + lateB17.c + lateB17.d + lateB17.e + lateB17.f;
lateSum = lateSum + lateB18.a + lateB18.b + lateB18.c + lateB18.d + lateB18.e + lateB18.f;
lateSum = lateSum + lateB19.a + lateB19.b + lateB19.c + lateB19.d + lateB19.e + lateB19.f;
lateSum = lateSum + lateB20.a + lateB20.b + lateB20.c + lateB20.d + lateB20.e + lateB20.f;
lateSum = lateSum + lateB21.a + lateB21.b + lateB21.c + lateB21.d + lateB21.e + lateB21.f;
lateSum = lateSum + lateB22.a + lateB22.b + lateB22.c + lateB22.d + lateB22.e + lateB22.f;
lateSum = lateSum + lateB23.a + lateB23.b + lateB23.c + lateB23.d + lateB23.e + lateB23.f;
lateSum = lateSum + lateB24.a + lateB24.b + lateB24.c + lateB24.d + lateB24.e + lateB24.f;
lateSum = lateSum + lateB25.a + lateB25.b + lateB25.c + lateB25.d + lateB25.e + lateB25.f;
lateSum = lateSum + lateB26.a + lateB26.b + lateB26.c + lateB26.d + lateB26.e + lateB26.f;
lateSum = lateSum + lateB27.a + lateB27.b + lateB27.c + lateB27.d + lateB27.e + lateB27.f;
lateSum = lateSum + lateB28.a + lateB28.b + lateB28.c + lateB28.d + lateB28.e + lateB28.f;
lateSum = lateSum + lateB29.a + lateB29.b + lateB29.c + lateB29.d + lateB29.e + lateB29.f;

// Expected: sum of 1001..1180 = (1001+1180)*180/2 = 2181*90 = 196290
// Both groups should be equal since they use the same constants
SoftAssert(earlySum === 196290, "case11: earlySum mismatch, got " + earlySum);
SoftAssert(lateSum === 196290, "case11: lateSum mismatch, got " + lateSum);
SoftAssert(earlySum === lateSum, "case11: early/late mismatch");

// Verify prefix variables: sum 0..89 = 89*90/2 = 4005
SoftAssert(midSum === 4005, "case11: midSum mismatch, got " + midSum);

__FlushElementTree($page);
print("case11 earlySum=" + earlySum + " lateSum=" + lateSum);
Assert(__assert_fail_count === 0);
