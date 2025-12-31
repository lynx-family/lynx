function test_reg_alloc_complex(a, b, c, d, e) {
    let x1 = a + b;
    let x2 = c + d;
    let x3 = x1 * x2;
    let x4 = e - x3;
    let x5 = x1 / x4;
    let x6 = x2 + x5;
    
    if (x6 > 0) {
        let y1 = x1 + x2 + x3 + x4 + x5 + x6;
        return y1;
    } else {
        let y2 = x1 * x2 * x3 * x4 * x5 * x6;
        return y2;
    }
}

let res = test_reg_alloc_complex(1, 2, 3, 4, 5);
// x1 = 3, x2 = 7, x3 = 21, x4 = 5 - 21 = -16, x5 = 3 / -16 = -0.1875, x6 = 7 - 0.1875 = 6.8125
// x6 > 0, so y1 = 3 + 7 + 21 - 16 - 0.1875 + 6.8125 = 21.625
Assert(res === 21.625);
