
function nested_cfg(a, b, c, d) {
    let result = 0;
    if (a > 0) {
        if (b > 0) {
            for (let i = 0; i < 10; i++) {
                if (c > 5) {
                    if (d < 100) {
                        result += a + b + c + d;
                    } else {
                        result -= 1;
                    }
                } 
            }
        } else {
            result = a * 2;
        }
    } else {
        if (c < 0) {
            result = b - c;
        } else {
            let j = 0;
            while (j < 5) {
                result += d;
                j++;
            }
        }
    }

    // Dead blocks and instructions
    if (false) {
        let dead = a + b + c + d;
        console.log(dead);
        if (true) {
            let more_dead = dead * 2;
        }
    }

    return result;
}

Assert(nested_cfg(1, 1, 10, 50) == 620);
Assert(nested_cfg(-1, 0, -1, 0) == 1);
Assert(nested_cfg(1, -1, 0, 0) == 2);
