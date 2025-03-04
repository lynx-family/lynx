let obj = {
  prop: 42,
};

let result = Object.freeze(obj);
Assert(result == obj);
