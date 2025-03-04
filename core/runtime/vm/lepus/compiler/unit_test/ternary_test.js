let items = [];
for (let i = 0; i < 100; i++) {
  items == null
    ? undefined
    : items.push === null
    ? undefined
    : items.push('!!!');
}

let items2 = [];
for (let i = 0; i < 100; i++) {
  items2.push('!!!');
}
Assert(items == items2);

function getFee(isMember) {
  return isMember ? '$2.00' : '$10.00';
}

Assert(getFee(true) == '$2.00');

Assert(getFee(false) == '$10.00');

Assert(getFee(null) == '$10.00');

function example(condition1, condition2, condition3) {
  return condition1 ? 1 : condition2 ? 2 : condition3 ? 3 : 4;
}

Assert(example(1, 0, 0) == 1);
Assert(example(1, 0, 1) == 1);
Assert(example(0, 1, 0) == 2);
Assert(example(0, 0, 1) == 3);
Assert(example(0, 0, 0) == 4);


