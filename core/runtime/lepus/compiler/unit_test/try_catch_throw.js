let catch_info = '';
let finally_info = '';
let catch_info2 = '';
let finally_info2 = '';
try {
  try {
    let a = 1;
    a();
  } catch (e) {
    catch_info = e;
  } finally {
    finally_info = 'finally';
  }

  let b = 1;
  b();
} catch (e) {
  catch_info2 = e;
} finally {
  finally_info2 = 'finally2';
}

try {
  function errorFunction() {
    let a = 1;
    a();
  }
  errorFunction();
} catch {
  console.log('error');
} finally {
  console.log('finally');
}

let global1 = 'global';
try {
  throw (global1);
} catch (e) {
  console.log(e);
} finally {
  console.log('finally');
}

function my() {
  let a = [1.1, 1.3];
  throw (a);
}
try {
  my();
} catch (e) {
  console.log(e);
}
