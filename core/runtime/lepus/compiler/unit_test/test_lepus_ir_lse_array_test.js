
function test_lse_array_methods() {
    var arr = [1, 2, 3];
    var obj = { x: 10 };
    
    var v1 = obj.x;
    arr.push(4); // Side effect on heap
    var v2 = obj.x; // Should be re-loaded
    
    if (v1 != 10 || v2 != 10) { print("Fail array 1"); Assert(false); }
    
    var v3 = arr[0];
    arr[0] = 100;
    var v4 = arr[0]; // Forwarding from store
    if (v3 != 1 || v4 != 100) { print("Fail array 2"); Assert(false); }
    
    var v5 = arr[1];
    arr.pop(); // Side effect on array, potentially affects indices
    var v6 = arr[1]; // Should be re-loaded
    if (v5 != 2 || v6 != 2) { print("Fail array 3"); Assert(false); }
}

test_lse_array_methods();
print("test_lse_array_methods passed");
