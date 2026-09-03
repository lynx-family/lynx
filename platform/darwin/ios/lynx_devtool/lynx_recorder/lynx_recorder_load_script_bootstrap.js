// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

(function(global){
var replayContext=__LYNX_RECORDER_REPLAY_CONTEXT__;
var allSources=replayContext&&replayContext.scripts&&typeof replayContext.scripts==='object'?
    replayContext.scripts:{};
var adapterPrefix='lynx-recorder://replay-adapter/';
var sources=Object.create(null);
var adapterSources=Object.create(null);
var owns=Object.prototype.hasOwnProperty;
Object.keys(allSources).forEach(function(url){
  if(String(url).indexOf(adapterPrefix)===0){adapterSources[url]=allSources[url];}
  else{sources[url]=allSources[url];}
});
var loaded=Object.create(null);
var adapters=[];
var currentAdapterURL='';
var replayErrors=[];
global.__LYNX_RECORDER_REPLAY_ERRORS__=replayErrors;
function reportError(adapter,stage,error){
  var entry={
    adapter:String(adapter&&adapter.id||adapter||currentAdapterURL||'unknown'),
    stage:String(stage||'unknown'),
    message:String(error&&error.message||error)
  };
  replayErrors.push(entry);
  if(global.console&&typeof global.console.error==='function'){
    global.console.error('[TestBench] Replay adapter error',entry);
  }
}
function resolve(url){
  var raw=String(url||'');
  if(owns.call(sources,raw)){return raw;}
  var path=raw.split('?')[0];
  if(owns.call(sources,path)){return path;}
  var relativePath=path.replace(/^(?:\.\/|\/)+/,'');
  var fileKey='ttfile:///'+relativePath;
  if(owns.call(sources,fileKey)){return fileKey;}
  if(!/\.[^/]+$/.test(relativePath)){
    if(owns.call(sources,fileKey+'.js')){return fileKey+'.js';}
    if(owns.call(sources,fileKey+'/index.js')){return fileKey+'/index.js';}
    if(owns.call(sources,relativePath+'.js')){return relativePath+'.js';}
  }
  var name=path.slice(path.lastIndexOf('/')+1);
  if(owns.call(sources,name)){return name;}
  var matched=null;
  Object.keys(sources).some(function(sourceKey){
    var sourcePath=sourceKey.split('?')[0];
    if(sourcePath.slice(sourcePath.lastIndexOf('/')+1)!==name){return false;}
    if(matched!==null){matched=false;return true;}
    matched=sourceKey;return false;
  });
  return matched||null;
}
function resolveWithParent(url,parentURL){
  var raw=String(url||'');
  if(!/^\.\.?\//.test(raw)){return resolve(raw);}
  var parent=String(parentURL||'').split('?')[0];
  var scheme='';
  var match=parent.match(/^([a-zA-Z][a-zA-Z0-9+.-]*:\/\/\/)/);
  if(match){scheme=match[1];parent=parent.slice(scheme.length);}
  var parts=parent.split('/');parts.pop();
  raw.split('/').forEach(function(part){
    if(!part||part==='.'){return;}
    if(part==='..'){parts.pop();}else{parts.push(part);}
  });
  var key=scheme+parts.join('/');
  return owns.call(sources,key)?key:resolve(key);
}
var originalLoadScript=typeof global.loadScript==='function'?global.loadScript:null;
function runHook(adapter,stage,args){
  var hook=adapter&&adapter[stage];
  if(typeof hook!=='function'){return undefined;}
  try{return hook.apply(adapter,args||[]);}catch(error){reportError(adapter,stage,error);}
}
function executeSource(source,url){
  Function(String(source)+'\n//# sourceURL='+String(url))();
}
function loadScript(url){
  var key=resolve(url);
  if(key===null){
    var missingHandled=adapters.some(function(adapter){
      return runHook(adapter,'handleMissingScript',[String(url||''),runtime])===true;
    });
    if(missingHandled){return true;}
    return originalLoadScript?originalLoadScript.apply(global,arguments):false;
  }
  if(loaded[key]){return true;}
  loaded[key]=true;
  var source=String(sources[key]||'');
  try{
    adapters.forEach(function(adapter){
      var transformed=runHook(adapter,'transformScript',[key,source,runtime]);
      if(typeof transformed==='string'){source=transformed;}
    });
    var handled=false;var failed=false;
    adapters.some(function(adapter){
      var result=runHook(adapter,'executeScript',[key,source,runtime]);
      handled=result===true;failed=result===false;
      return handled||failed;
    });
    if(failed){delete loaded[key];return false;}
    if(!handled){executeSource(source,key);}
    adapters.forEach(function(adapter){runHook(adapter,'afterScript',[key,runtime]);});
    return true;
  }catch(error){
    delete loaded[key];
    throw error;
  }
}
global.loadScript=loadScript;
if(typeof global.loadScriptAsync!=='function'){
  global.loadScriptAsync=function(url,callback){
    var error=null;
    try{if(!loadScript(url)){error={code:1,errMsg:'recorded script not found: '+String(url)};}}
    catch(exception){error={code:1,errMsg:String(exception&&exception.message||exception)};}
    if(typeof callback==='function'){callback(error);}
  };
}
var runtime={
  context:replayContext,
  sources:sources,
  resolveScript:resolveWithParent,
  loadScript:loadScript,
  schedule:function(callback,delay){
    return typeof global.setTimeout==='function'?global.setTimeout(callback,Math.max(0,Number(delay)||0)):callback();
  },
  reportError:function(stage,error){reportError(currentAdapterURL,stage,error);}
};
global.__LYNX_RECORDER_REPLAY_RUNTIME__=runtime;
global.__LYNX_RECORDER_REGISTER_REPLAY_ADAPTER__=function(adapter){
  if(!adapter||typeof adapter.id!=='string'||!adapter.id){
    reportError(currentAdapterURL,'register',new Error('adapter id is required'));return;
  }
  adapter.__recordedURL=currentAdapterURL;
  adapters.push(adapter);
};
Object.keys(adapterSources).sort().forEach(function(url){
  currentAdapterURL=url;
  try{executeSource(adapterSources[url],url);}catch(error){reportError(url,'load',error);}
  currentAdapterURL='';
});
adapters.forEach(function(adapter){runHook(adapter,'beforeReplay',[runtime]);});
adapters.forEach(function(adapter){runHook(adapter,'start',[runtime]);});
})(typeof globalThis!=='undefined'?globalThis:Function('return this')());
