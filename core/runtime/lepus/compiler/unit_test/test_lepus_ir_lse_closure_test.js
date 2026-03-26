// Test LSE for closure variables (Context Slots)

function testRedundantGet() {
    let x = 1;
    function inner() {
        // Redundant GetContextSlot
        let a = x;
        let b = x; 
        return a + b;
    }
    return inner();
}

function testForwarding() {
    let x = 1;
    function inner() {
        // SetContextSlot followed by GetContextSlot
        x = 42;
        let a = x;
        return a;
    }
    return inner();
}

function testInvalidationByCall() {
    let x = 1;
    function modify() {
        x = 100;
    }
    function inner() {
        let a = x;
        modify(); // Should invalidate cache for x
        let b = x;
        return a + b;
    }
    return inner();
}

function testNestedClosure() {
    let x = 1;
    function outer() {
        let y = 2;
        function inner() {
            let a = x;
            let b = y;
            let c = x; // Redundant
            let d = y; // Redundant
            return a + b + c + d;
        }
        return inner();
    }
    return outer();
}

function testMixedAllocations() {
    let x = 1;
    function inner() {
        let a = x;
        let obj = {foo: 1}; // NewTable should not invalidate x
        let arr = [1, 2];   // NewArray should not invalidate x
        let b = x;          // Redundant
        return a + b + obj.foo + arr[0];
    }
    return inner();
}

console.log(testRedundantGet());      // Expected: 2
console.log(testForwarding());        // Expected: 42
console.log(testInvalidationByCall());// Expected: 101
console.log(testNestedClosure());     // Expected: 6
console.log(testMixedAllocations());  // Expected: 4
