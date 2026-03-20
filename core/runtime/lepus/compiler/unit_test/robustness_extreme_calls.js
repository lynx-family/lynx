
function target(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20) {
    return a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9 + a10 + 
           a11 + a12 + a13 + a14 + a15 + a16 + a17 + a18 + a19 + a20;
}

function extreme_calls(base) {
    // Large argument count call
    let res1 = target(base, base + 1, base + 2, base + 3, base + 4, 
                      base + 5, base + 6, base + 7, base + 8, base + 9,
                      base + 10, base + 11, base + 12, base + 13, base + 14,
                      base + 15, base + 16, base + 17, base + 18, base + 19);

    // Call result as argument
    let res2 = target(res1, res1, res1, res1, res1, res1, res1, res1, res1, res1,
                      res1, res1, res1, res1, res1, res1, res1, res1, res1, res1);

    // Mixed logic
    let f = target;
    let x = base * 2;
    let y = base * 3;
    let res3 = f(x, y, x, y, x, y, x, y, x, y, x, y, x, y, x, y, x, y, x, y);

    return res2 + res3;
}

var res = extreme_calls(1);
Assert(res != 0);
