// Copyright 2024 The Primjs Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
//
// clang-format off
#ifdef DEFTAG

#ifndef deftag
#define deftag(type, description) DEFTAG(type, description)
#endif
// LEPUSValue tag
DEFTAG(LEPUSObject, "LEPUSObject")
DEFTAG(LEPUSLepusRef, "LEPUSLepusRef")
DEFTAG(JSString, "JSString")
DEFTAG(JSShape, "JSShape")
DEFTAG(LEPUSFunctionBytecode, "LEPUSFunctionBytecode")
DEFTAG(JSTypedArray, "JSTypedArray")
DEFTAG(JSMapState, "JSMapState")
DEFTAG(JSMapIteratorData, "JSMapIteratorData")
DEFTAG(JSFunctionDef, "JSFunctionDef")
DEFTAG(JSArrayBuffer, "JSArrayBuffer")
DEFTAG(LEPUSScriptSource, "LEPUSScriptSource")
DEFTAG(LEPUSModuleDef, "LEPUSModuleDef")
DEFTAG(JSGeneratorData, "JSGeneratorData")
DEFTAG(JSAsyncFunctionData, "JSAsyncFunctionData")
DEFTAG(JSVarRef, "JSVarRef")
// LEPUSObject, class_id, finalizer
DEFTAG(JSBoundFunction, "JSBoundFunction")
DEFTAG(JSCFunctionDataRecord, "JSCFunctionDataRecord")
DEFTAG(JSForInIterator, "JSForInIterator")
DEFTAG(JSSeparableString, "JSSeparableString")
DEFTAG(JSArrayIteratorData, "JSArrayIteratorData")
DEFTAG(JSRegExpStringIteratorData, "JSRegExpStringIteratorData")
DEFTAG(JSProxyData, "JSProxyData")
DEFTAG(JSPromiseData, "JSPromiseData")
DEFTAG(JSPromiseReactionData, "JSPromiseReactionData")
DEFTAG(JSPromiseFunctionData, "JSPromiseFunctionData")
DEFTAG(JSAsyncFromSyncIteratorData, "JSAsyncFromSyncIteratorData")
DEFTAG(JSAsyncGeneratorData, "JSAsyncGeneratorData")
// other
DEFTAG(LEPUSPropertyEnum, "LEPUSPropertyEnum")
DEFTAG(JSMapRecord, "JSMapRecord")
DEFTAG(ValueSlot, "ValueSlot")
DEFTAG(LEPUSDebuggerInfo, "DebuggerInfo")

DEFTAG(FinalizationRegistryData, "FinalizationRegistryData")
DEFTAG(WeakRefData, "WeakRefData")

#ifdef CONFIG_BIGNUM
// big number
DEFTAG(JSBigFloat, "JSBigFloat")
#endif

DEFTAG(JSOSRWHandler, "JSOSRWHandler")
DEFTAG(JSOSSignalHandler, "JSOSSignalHandler")
DEFTAG(JSOSTimer, "JSOSTimer")
DEFTAG(JSSTDFile, "JSSTDFile")

deftag(JSSymbol, "JSSymbol")
deftag(JSValueArray, "JSValueArray")
deftag(JSConstString, "JSConstString")
deftag(JsonStrArray, "JsonStrArray")
#undef deftag
#endif
