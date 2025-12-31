// E2E: string indexing + trim + manual tokenize + array join.

function tokenize(s) {
  s = s.trim();
  let out = [];
  let cur = "";
  for (let i = 0; i < s.length; i++) {
    let ch = s[i];
    if (ch === ',' || ch === ';') {
      out.push(cur);
      cur = "";
    } else {
      cur = cur + ch;
    }
  }
  out.push(cur);
  return out;
}

let arr = tokenize("  a,b;cc,dd;  ");
Assert(arr.length === 5);
Assert(arr[0] === "a");
Assert(arr[1] === "b");
Assert(arr[2] === "cc");
Assert(arr[3] === "dd");
Assert(arr[4] === "");

let joined = arr.join("|");
Assert(joined === "a|b|cc|dd|");

// extra: compare + charAt
let s = "HELLO";
Assert(s.charAt(0) === "H");
Assert(s[4] === "O");
Assert("z" > "a");

