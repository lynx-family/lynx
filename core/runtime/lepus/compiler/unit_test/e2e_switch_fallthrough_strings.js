// E2E: switch over strings + fallthrough-like behavior + default.

function score(tag) {
  let s = 0;
  if (tag === "A") {
    s += 1;
    // emulate fallthrough by explicit code path
    s += 10;
  } else if (tag === "B") {
    s += 2;
  } else if (tag === "C") {
    s += 3;
  } else {
    s += 99;
  }
  return s;
}

Assert(score("A") === 11);
Assert(score("B") === 2);
Assert(score("C") === 3);
Assert(score("Z") === 99);
