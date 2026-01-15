# Napi Lynx shim layer

Currently on Android we are depending on Napi source files directly while on iOS
we depend on published Napi Pod. This shim layer allows for including the same
header file in Lynx code.

This should be fixed soon as Android is adopting libc++_shared.so.
