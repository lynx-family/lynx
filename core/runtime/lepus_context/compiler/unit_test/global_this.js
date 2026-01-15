let top1 = "hello";

function f() {
  Assert(globalThis.top1 == "hello");
}

function ret() {
  return "call ret";
}

Assert(globalThis.ret() == "call ret");
