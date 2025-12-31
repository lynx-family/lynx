let topVar = "original";

function createClosure() {
    return function() {
        return topVar;
    };
}

let closure = createClosure();
Assert(closure() === "original");

topVar = "changed";
Assert(closure() === "changed");
Assert(globalThis.topVar === "changed");

// Test with shadow variable
function shadowTest() {
    let topVar = "shadowed"; // This shadows the top-level topVar
    Assert(topVar === "shadowed");
    Assert(globalThis.topVar === "changed");
}
shadowTest();
Assert(topVar === "changed");

print("toplevel_closure_sync_test passed");
