setFlag("lepusNullPropAsUndef", true);
let i = undefined;
let j = null;


Assert(!(i == j));
Assert(i == undefined);
Assert(j == null);
Assert((i? "true" : "false") == "false");
Assert((!i? "true" : "false") == "true");

let obj = {};
Assert(obj.x == undefined);

let uf;
Assert(uf.x == undefined);
setFlag("lepusNullPropAsUndef", false);

let ni = undefined;
Assert(ni == null);

let nobj;
Assert(nobj == null);
Assert(nobj.x == null);
