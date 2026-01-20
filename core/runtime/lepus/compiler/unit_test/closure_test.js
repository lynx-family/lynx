let arams = {"h":10, "e":11, "x":12};
let SystemInfo = null;
let __globalProps = null;
let keys = ["h", "e", "x"];
function renderer() {
  let func1 = {};
  (function m(exports) {
    "use strict";

    function add(aram) {
      let absParams = aram;
      return keys
        .filter(function yy(absTestKey) {
          console.log("filter", absParams, absTestKey);
          return absParams[absTestKey];
        })
        .join(" ");
    }

    exports.add = add;
  })(func1);
  let func2 = {};
  (function xx(exports) {
    "use strict";
    function nothing(aram) {
      let margs = aram;
      return keys
        .filter(function clo(absTestKey) {
          console.log("filter", margs, absTestKey);
          return margs[absTestKey];
        })
        .join(" ");
    }
    exports.nothing = nothing;
  })(func2);

  let ret = func2.nothing(arams);
  Assert(ret);
}
