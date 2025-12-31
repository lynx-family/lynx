// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// Test unary operator register allocation
// This test verifies that unary operators correctly allocate registers
// and that src and dst use the same register for in-place operations

console.log("=== Unary Operator Register Allocation Tests ===");

// Test 1: Unary Negation (-)
function testUnaryNeg() {
    console.log("\n[Test 1] Unary Negation (-)");
    let x = 42;
    let result = -x;
    Assert(result === -42);
    console.log("  -42 = " + result + " ✓");

    let y = -10;
    let result2 = -y;
    Assert(result2 === 10);
    console.log("  -(-10) = " + result2 + " ✓");

    let z = 0;
    let result3 = -z;
    Assert(result3 === 0);
    console.log("  -0 = " + result3 + " ✓");
}

// Test 2: Unary Plus (+)
function testUnaryPos() {
    console.log("\n[Test 2] Unary Plus (+)");
    let x = 42;
    let result = +x;
    Assert(result === 42);
    console.log("  +42 = " + result + " ✓");

    let y = -10;
    let result2 = +y;
    Assert(result2 === -10);
    console.log("  +(-10) = " + result2 + " ✓");

    let str = "123";
    let result3 = +str;
    Assert(result3 === 123);
    console.log("  +'123' = " + result3 + " ✓");
}

// Test 3: Logical NOT (!)
function testUnaryNot() {
    console.log("\n[Test 3] Logical NOT (!)");
    let x = true;
    let result = !x;
    Assert(result === false);
    console.log("  !true = " + result + " ✓");

    let y = false;
    let result2 = !y;
    Assert(result2 === true);
    console.log("  !false = " + result2 + " ✓");

    let z = 0;
    let result3 = !z;
    Assert(result3 === true);
    console.log("  !0 = " + result3 + " ✓");

    let w = 42;
    let result4 = !w;
    Assert(result4 === false);
    console.log("  !42 = " + result4 + " ✓");
}

// Test 4: Bitwise NOT (~)
function testUnaryBitNot() {
    let x = 5;
    let result = ~x;
    Assert(result === -6);

    let y = -1;
    let result2 = ~y;
    Assert(result2 === 0);
    console.log("  ~(-1) = " + result2 + " ✓");

    let z = 0;
    let result3 = ~z;
    Assert(result3 === -1);
    console.log("  ~0 = " + result3 + " ✓");
}

// Test 5: Increment (++)
function testUnaryInc() {
    console.log("\n[Test 5] Increment (++)");

    // Pre-increment
    let x = 10;
    let result = ++x;
    Assert(x === 11);
    Assert(result === 11);
    console.log("  ++10 = " + result + ", x = " + x + " ✓");

    // Post-increment
    let y = 20;
    let result2 = y++;
    Assert(y === 21);
    Assert(result2 === 20);
    console.log("  20++ = " + result2 + ", y = " + y + " ✓");

    // Increment negative
    let z = -5;
    ++z;
    Assert(z === -4);
    console.log("  ++(-5) = " + z + " ✓");
}

// Test 6: Decrement (--)
function testUnaryDec() {
    console.log("\n[Test 6] Decrement (--)");

    // Pre-decrement
    let x = 10;
    let result = --x;
    Assert(x === 9);
    Assert(result === 9);
    console.log("  --10 = " + result + ", x = " + x + " ✓");

    // Post-decrement
    let y = 20;
    let result2 = y--;
    Assert(y === 19);
    Assert(result2 === 20);
    console.log("  20-- = " + result2 + ", y = " + y + " ✓");

    // Decrement to zero
    let z = 1;
    --z;
    Assert(z === 0);
    console.log("  --1 = " + z + " ✓");
}

// Test 7: Typeof operator
function testUnaryTypeof() {
    console.log("\n[Test 7] Typeof operator");
    let x = 42;
    let result = typeof x;
    Assert(result === "number");
    console.log("  typeof 42 = '" + result + "' ✓");

    let y = "hello";
    let result2 = typeof y;
    Assert(result2 === "string");
    console.log("  typeof 'hello' = '" + result2 + "' ✓");

    let z = true;
    let result3 = typeof z;
    Assert(result3 === "boolean");
    console.log("  typeof true = '" + result3 + "' ✓");
}

// Test 8: Parameter as source operand
function testUnaryWithParameter(x) {
    console.log("\n[Test 8] Unary with parameter");
    let result1 = -x;
    Assert(result1 === -100);
    console.log("  -param(100) = " + result1 + " ✓");

    let result2 = !x;
    Assert(result2 === false);
    console.log("  !param(100) = " + result2 + " ✓");

    let result3 = ~x;
    Assert(result3 === -101);
    console.log("  ~param(100) = " + result3 + " ✓");
}

// Test 9: Multiple uses of source operand
function testUnaryWithMultipleUses() {
    console.log("\n[Test 9] Unary with multiple uses");
    let x = 10;
    let neg = -x;      // x is negated
    let sum = x + 5;   // x is used again
    let result = neg + sum;
    Assert(neg === -10);
    Assert(sum === 15);
    Assert(result === 5);
    console.log("  -x + (x + 5) where x=10: " + result + " ✓");

    let y = 7;
    let bitnot = ~y;   // y is bit-negated
    let double = y * 2; // y is used again
    let result2 = bitnot + double;
    Assert(bitnot === -8);
    Assert(double === 14);
    Assert(result2 === 6);
    console.log("  ~y + (y * 2) where y=7: " + result2 + " ✓");
}

// Test 10: Nested unary operators
function testNestedUnary() {
    console.log("\n[Test 10] Nested unary operators");
    let x = 5;
    let result1 = --x;
    Assert(result1 === 4);
    console.log("  --5 = " + result1 + " ✓");

    let y = 10;
    let result2 = -(-y);
    Assert(result2 === 10);
    console.log("  -(-10) = " + result2 + " ✓");

    let z = true;
    let result3 = !!z;
    Assert(result3 === true);
    console.log("  !!true = " + result3 + " ✓");

    let w = 0;
    let result4 = !!w;
    Assert(result4 === false);
    console.log("  !!0 = " + result4 + " ✓");

    let v = 5;
    let result5 = ~(~v);
    Assert(result5 === 5);
    console.log("  ~~5 = " + result5 + " ✓");
}

// Test 11: Unary in expressions
function testUnaryInExpressions() {
    console.log("\n[Test 11] Unary in expressions");
    let x = 10;
    let y = 20;

    let result1 = -x + y;
    Assert(result1 === 10);
    console.log("  -10 + 20 = " + result1 + " ✓");

    let result2 = x + -y;
    Assert(result2 === -10);
    console.log("  10 + (-20) = " + result2 + " ✓");

    let result3 = ~x & y;
    Assert(result3 === 20);
    console.log("  ~10 & 20 = " + result3 + " ✓");

    let a = 5;
    let result4 = (++a) * 2;
    Assert(result4 === 12);
    Assert(a === 6);
    console.log("  (++5) * 2 = " + result4 + " ✓");
}

// Test 12: Unary in control flow
function testUnaryInControlFlow() {
    console.log("\n[Test 12] Unary in control flow");

    let x = 10;
    let result;
    if (-x < 0) {
        result = "negative";
    } else {
        result = "positive";
    }
    Assert(result === "negative");
    console.log("  if (-10 < 0): '" + result + "' ✓");

    let y = 0;
    let result2;
    if (!y) {
        result2 = "falsy";
    } else {
        result2 = "truthy";
    }
    Assert(result2 === "falsy");
    console.log("  if (!0): '" + result2 + "' ✓");

    let z = 5;
    let result3 = z > 0 ? -z : z;
    Assert(result3 === -5);
    console.log("  (5 > 0) ? -5 : 5 = " + result3 + " ✓");
}

// Test 13: Unary with different types
function testUnaryWithDifferentTypes() {
    console.log("\n[Test 13] Unary with different types");

    // Number
    let num = 42;
    Assert(-num === -42);
    console.log("  -(number) ✓");

    // String to number
    let str = "123";
    Assert(+str === 123);
    console.log("  +(string) ✓");

    // Boolean
    let bool = true;
    Assert(!bool === false);
    console.log("  !(boolean) ✓");

    // Null
    let nul = null;
    Assert(!nul === true);
    console.log("  !(null) ✓");
}

// Test 14: Edge cases
function testUnaryEdgeCases() {
    console.log("\n[Test 14] Edge cases");

    // Very large number
    let large = 1000000;
    let result1 = -large;
    Assert(result1 === -1000000);
    console.log("  -1000000 = " + result1 + " ✓");

    // Very small number
    let small = -1000000;
    let result2 = -small;
    Assert(result2 === 1000000);
    console.log("  -(-1000000) = " + result2 + " ✓");

    // Zero
    let zero = 0;
    Assert(-zero === 0);
    Assert(+zero === 0);
    Assert(!zero === true);
    Assert(~zero === -1);
    console.log("  Unary operations on 0 ✓");

    // Negative zero (JavaScript quirk)
    let negZero = -0;
    Assert(negZero === 0);
    console.log("  -0 === 0 ✓");
}

// Test 15: Unary in loops
function testUnaryInLoops() {
    console.log("\n[Test 15] Unary in loops");

    let sum = 0;
    for (let i = 0; i < 5; i++) {
        sum += -i;
    }
    Assert(sum === -10);
    console.log("  Sum of -i for i=0..4: " + sum + " ✓");

    let count = 0;
    let x = 10;
    while (x-- > 0) {
        count++;
    }
    Assert(count === 10);
    Assert(x === -1);
    console.log("  While loop with x--: count=" + count + " ✓");

    let product = 1;
    for (let i = 1; i <= 5; ++i) {
        product *= i;
    }
    Assert(product === 120);
    console.log("  Factorial with ++i: " + product + " ✓");
}

// Test 16: Complex nested expressions
function testComplexNestedExpressions() {
    console.log("\n[Test 16] Complex nested expressions");

    let a = 5;
    let b = 10;
    let result1 = -(a + b);
    Assert(result1 === -15);
    console.log("  -(5 + 10) = " + result1 + " ✓");

    let result2 = ~(a & b);
    Assert(result2 === -1);
    console.log("  ~(5 & 10) = " + result2 + " ✓");

    let result3 = !(a > b);
    Assert(result3 === true);
    console.log("  !(5 > 10) = " + result3 + " ✓");

    let x = 3;
    let result4 = -(++x) * 2 + ~x;
    Assert(result4 === -13);
    console.log("  -(++3) * 2 + ~4 = " + result4 + " ✓");
}

// Test 17: No clobber after copy propagation
// Sensitive to incorrect src==dst forcing and unsafe MOV propagation.
function testUnaryNoClobberAfterCopy() {
    console.log("\n[Test 17] Unary no-clobber after copy");

    // ++ on a copy must not change the original.
    let x = 10;
    let y = x;
    let z = ++y;
    Assert(x === 10);
    Assert(y === 11);
    Assert(z === 11);

    // -- on a copy must not change the original.
    let a = 20;
    let b = a;
    let c = --b;
    Assert(a === 20);
    Assert(b === 19);
    Assert(c === 19);

    // typeof must not overwrite the operand value.
    let n = 42;
    let t = typeof n;
    let sum = n + 1;
    Assert(t === "number");
    Assert(sum === 43);
}

// Run all tests
testUnaryNeg();
testUnaryPos();
testUnaryNot();
testUnaryBitNot();
testUnaryInc();
testUnaryDec();
testUnaryTypeof();
testUnaryWithParameter(100);
testUnaryWithMultipleUses();
testNestedUnary();
testUnaryInExpressions();
testUnaryInControlFlow();
testUnaryWithDifferentTypes();
testUnaryEdgeCases();
testUnaryInLoops();
testComplexNestedExpressions();
testUnaryNoClobberAfterCopy();

console.log("\n=== All Unary Register Allocation Tests Passed! ===");
