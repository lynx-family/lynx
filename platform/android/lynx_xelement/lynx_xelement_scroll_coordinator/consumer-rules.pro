-keep class android.support.design.widget.AppBarLayout$BaseBehavior{
*;
}

-keep class android.support.design.widget.HeaderBehavior{
*;
}

-keep class com.google.android.material.appbar.AppBarLayout$BaseBehavior{
*;
}

-keep class com.google.android.material.appbar.HeaderBehavior{
*;
}

-keepclassmembers class com.google.android.material.appbar.** {
    void dispatchOffsetUpdates(int);
}
