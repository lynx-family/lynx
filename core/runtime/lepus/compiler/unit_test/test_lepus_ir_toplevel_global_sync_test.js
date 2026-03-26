// 1. Test variable modification -> globalThis access (READ SYNC)
var a = 100;
Assert(a === 100);
Assert(globalThis.a === 100);

a = 200;
Assert(a === 200);
Assert(globalThis.a === 200);

// 2. Complex interaction
function modifyA(val) {
    a = val;
}
modifyA(400);
Assert(globalThis.a === 400);

// 3. Multiple variables
var b = "hello";
var c = { x: 1 };
Assert(globalThis.b === "hello");
Assert(globalThis.c.x === 1);

b = "world";
Assert(globalThis.b === "world");

// 4. Nested scopes
(function() {
    a = 600;
    Assert(globalThis.a === 600);
})();

print("toplevel_global_sync_test passed");
