// E2E: recursion + memo table + mixed numeric/string keys.

function fib(n, memo) {
  let key = "k" + n;
  let cached = memo[key];
  if (cached !== undefined) {
    return cached;
  }
  let v;
  if (n <= 1) {
    v = n;
  } else {
    v = fib(n - 1, memo) + fib(n - 2, memo);
  }
  memo[key] = v;
  return v;
}

let memo = {};
let a = fib(10, memo);  // 55
let b = fib(11, memo);  // 89

// validate memo filled (access through computed keys)
Assert(memo.k10 === 55);
Assert(memo["k11"] === 89);
Assert(a === 55);
Assert(b === 89);

