function build_big_obj() {
  // Keep this test light: huge literals may exceed the VM bytecode register
  // limit (max 256), which is not what this test is about.
  var x = 0;

  var o = {};
  o["k0"] = 1000;
  o["k1"] = 1001;
  o["k2"] = 1002;
  o["k3"] = 1003;
  o["k4"] = 1004;
  o["k5"] = 1005;
  o["k6"] = 1006;
  o["k7"] = 1007;
  o["k8"] = 1008;
  o["k9"] = 1009;
  o["k10"] = 1010;
  o["k11"] = 1011;
  o["k12"] = 1012;
  o["k13"] = 1013;
  o["k14"] = 1014;
  o["k15"] = 1015;

  return o["k15"] + x;
}

Assert(build_big_obj() === 1015);
