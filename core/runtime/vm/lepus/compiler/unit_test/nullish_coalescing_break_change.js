setFlag("lepusNullPropAsUndef", true);

let a = {};
let b = a?.test ?? [];
Assert(b == []);


let c = null ?? [];
Assert(c == []);


let d = undefined ?? [];
Assert(d == []);

let m = undefined;
let e = m?.forEach(function(item){});