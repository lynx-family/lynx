
function test_sharing() {
  let x = 10;
  function f1() { return x; }
  function f2() { x = 20; return x; }
  Assert(f1() === 10);
  Assert(f2() === 20);
  Assert(f1() === 20);
}

function test_loop() {
  let x = 0;
  function f() { x++; return x; }
  for (let i = 0; i < 10; i++) {
    f();
  }
  Assert(x === 10);
}

function test_nested() {
  let x = 1;
  function outer() {
    let y = 2;
    function inner() {
      return x + y;
    }
    return inner();
  }
  Assert(outer() === 3);
}

function test_capture_and_modify() {
  let a = 1;
  let b = 2;
  function mod() {
    a = 10;
    b = 20;
  }
  function read() {
    return a + b;
  }
  Assert(read() === 3);
  mod();
  Assert(read() === 30);
}

test_sharing();
test_loop();
test_nested();
test_capture_and_modify();
