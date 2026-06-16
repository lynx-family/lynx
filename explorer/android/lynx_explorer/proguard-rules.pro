# Add project specific ProGuard rules here.
# You can control the set of applied configuration files using the
# proguardFiles setting in build.gradle.
#
# For more details, see
#   http://developer.android.com/guide/developing/tools/proguard.html

# If your project uses WebView with JS, uncomment the following
# and specify the fully qualified class name to the JavaScript interface
# class:
#-keepclassmembers class fqcn.of.javascript.interface.for.webview {
#   public *;
#}

# Uncomment this to preserve the line number information for
# debugging stack traces.
#-keepattributes SourceFile,LineNumberTable

# If you keep the line number information, uncomment this to
# hide the original source file name.
#-renamesourcefileattribute SourceFile

-keepclasseswithmembers class * {
    @com.lynx.trace.CalledByNative <methods>;
}

-keepclasseswithmembers class * {
    @com.lynx.base.CalledByNative <methods>;
}

-keep class androidx.activity.** { *; }
-keep class androidx.fragment.** { *; }
-keep class androidx.lifecycle.** { *; }
-keep class androidx.savedstate.** { *; }

# Sparkling navigation router methods are registered through Sparkling's IDL
# reflection path and must keep their class names and annotations in minified
# enabled builds.
-keepattributes *Annotation*,Signature,InnerClasses,EnclosingMethod
-keep class com.lynx.explorer.sparkling.SparklingNavigationRegistrar { *; }
-keep class com.tiktok.sparkling.method.router.** { *; }
-keep class com.tiktok.sparkling.method.registry.core.annotation.** { *; }

# Sparkling SDK's bundled lint-check JARs reference com.android.tools.lint.client.api.Vendor
# which is only available in AGP 7+. This project uses AGP 4.1.0; suppress the class warning
# at the R8 level (the lint task itself is suppressed via lintOptions.checkReleaseBuilds=false).
-dontwarn com.android.tools.lint.**
