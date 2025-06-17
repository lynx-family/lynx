// platform/android/lynx_explorer/src/main/java/com/lynx/explorer/modules/ImageGeneratorModule.java
package com.lynx.explorer.modules;

import android.util.Log;

import com.lynx.jsbridge.LynxModule;
import com.lynx.jsbridge.LynxMethod;
import com.lynx.jsbridge.Callback; // For JS callbacks
import com.lynx.jsbridge.Promise; // For JS Promises

// Changed to extend LynxModule directly
public class ImageGeneratorModule extends LynxModule {
    private static final String TAG = "ImageGeneratorModule";

    // Constructor required for LynxModule
    public ImageGeneratorModule() {
        super();
    }

    @LynxMethod
    public void generateImage(String prompt, Promise promise) {
        Log.d(TAG, "Native Android: generateImage called with prompt: " + prompt);
        // In a real module, you might interact with an Android AI SDK here.
        // For now, we just simulate a delay and return a predefined success message or a dummy URL.

        // Simulate some work
        try {
            Thread.sleep(1000);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            promise.reject("Error", "Image generation interrupted: " + e.getMessage());
            return;
        }

        String simulatedImageUrl = "https://dummyjson.com/image/400x300/ff0000/fff?text=Native+Android+" + encodeURIComponent(prompt);
        promise.resolve(simulatedImageUrl);
    }

    @LynxMethod
    public void logPrompt(String prompt, Callback callback) {
        Log.i(TAG, "Native Android: logPrompt called with: " + prompt);
        // This method is for testing basic native module communication
        callback.invoke("Prompt logged successfully by Android Native Module: " + prompt);
    }

    // Helper method for URL encoding, as it's used in generateImage
    private String encodeURIComponent(String s) {
        if (s == null) {
            return "null_prompt";
        }
        try {
            return java.net.URLEncoder.encode(s, "UTF-8")
                .replaceAll("\+", "%20")
                .replaceAll("\%21", "!")
                .replaceAll("\%27", "'")
                .replaceAll("\%28", "(")
                .replaceAll("\%29", ")")
                .replaceAll("\%7E", "~");
        } catch (java.io.UnsupportedEncodingException e) {
            Log.e(TAG, "URL encoding failed", e);
            return s; // fallback to original string
        }
    }
}
