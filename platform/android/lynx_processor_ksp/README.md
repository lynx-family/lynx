# Lynx KSP Processor

`lynx-processor-ksp` is the KSP equivalent of `lynx-processor`. It generates the
same Java registration sources for Lynx behaviors, props, UI methods, JSI
properties, and library providers without requiring KAPT.

```kotlin
plugins {
  id("com.google.devtools.ksp")
}

dependencies {
  ksp("org.lynxsdk.lynx:lynx-processor-ksp:<version>")
}
```

Projects using the Lynx library plugin do not need to configure
`lynx.library.packageName` manually. For direct processor use, pass it through
the KSP extension:

```kotlin
ksp {
  arg("lynx.library.packageName", "com.example.generated")
}
```

The original `lynx-processor` artifact remains available for javac and KAPT
users.
