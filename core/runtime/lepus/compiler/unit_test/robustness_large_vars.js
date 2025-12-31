
function large_vars(start) {
    let v0 = start + 0;
    let v1 = start + 1;
    let v2 = start + 2;
    let v3 = start + 3;
    let v4 = start + 4;
    let v5 = start + 5;
    let v6 = start + 6;
    let v7 = start + 7;
    let v8 = start + 8;
    let v9 = start + 9;
    let v10 = start + 10;
    let v11 = start + 11;
    let v12 = start + 12;
    let v13 = start + 13;
    let v14 = start + 14;
    let v15 = start + 15;
    let v16 = start + 16;
    let v17 = start + 17;
    let v18 = start + 18;
    let v19 = start + 19;
    let v20 = start + 20;
    let v21 = start + 21;
    let v22 = start + 22;
    let v23 = start + 23;
    let v24 = start + 24;
    let v25 = start + 25;
    let v26 = start + 26;
    let v27 = start + 27;
    let v28 = start + 28;
    let v29 = start + 29;
    let v30 = start + 30;

    // Use them in multiple ways to create complex live intervals
    let sum1 = v0 + v1 + v2 + v3 + v4 + v5 + v6 + v7 + v8 + v9;
    let sum2 = v10 + v11 + v12 + v13 + v14 + v15 + v16 + v17 + v18 + v19;
    let sum3 = v20 + v21 + v22 + v23 + v24 + v25 + v26 + v27 + v28 + v29;

    let total = sum1 + sum2 + sum3;

    // Interleave usage
    let mixed = (v0 * v29) + (v1 * v28) + (v2 * v27) + (v3 * v26) + (v4 * v25);
    
    if (total > 1000) {
        return total + mixed;
    }

    return total - mixed;
}

var res = large_vars(10);
Assert(res != 0);
