
function test_string_concat() {
    let name = "Lepus";
    let version = 2.0;
    
    let str1 = "Hello, " + name + "!";
    Assert(str1 == "Hello, Lepus!");
    
    let str2 = "Version is " + (version + 0.6);
    Assert(str2 == "Version is 2.6");
    
    let str3 = "Multline\ntest";
    // Check length instead of indexOf
    Assert(str3.length > 10);

    let nested = "Nested: " + ("val: " + (1 + 1));
    Assert(nested == "Nested: val: 2");
}

test_string_concat();
