//lepusNullPropAsUndef
let i = undefined;
let j = null;


Assert(!(i == j));
Assert(i == undefined);
Assert(j == null);
Assert((i? "true" : "false") == "false");
Assert((!i? "true" : "false") == "true");

let obj = {};
Assert(obj.x == undefined);