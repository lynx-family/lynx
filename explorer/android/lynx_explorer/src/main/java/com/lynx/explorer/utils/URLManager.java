// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.utils;

import android.content.Context;
import android.content.SharedPreferences;
import android.util.Log;
import java.util.List;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.Map;
import java.util.HashMap;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;

public class URLManager {
    private static final String TAG = "URLManager";
    private static final String PREFS_NAME = "URLManagerPrefs";
    private static final String KEY_RECENT_URLS = "recent_urls";
    private static final int MAX_SIZE = 5;
    
    private static URLManager instance;
    private Context context;
    private SharedPreferences prefs;
    private List<Map<String, String>> recentUrls;
    
    private URLManager(Context context) {
        this.context = context.getApplicationContext();
        this.prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
        loadRecentUrls();
    }
    
    public static synchronized URLManager getInstance(Context context) {
        if (instance == null) {
            instance = new URLManager(context);
        }
        return instance;
    }
    
    public void addToHistory(String url) {
        if (!isValidUrl(url)) {
            return;
        }

        LocalDateTime unformattedDate = LocalDateTime.now();
        DateTimeFormatter myFormatObj = DateTimeFormatter.ofPattern("dd-MM-yyyy HH:mm:ss");
        String date = unformattedDate.format(myFormatObj);
        Map<String, String> urlEntry = new HashMap<>();
        urlEntry.put("url", url);
        urlEntry.put("time", date);
        // Add to front of lists
        for (int ind = 0; ind < recentUrls.size(); ind++){
            if (url.equals(recentUrls.get(ind).get("url"))){
                recentUrls.remove(ind);
                break;
            }
        }
        
        recentUrls.add(0, urlEntry);
        
        //Remove extra urls
        while (recentUrls.size() > MAX_SIZE) {
            recentUrls.remove(recentUrls.size() - 1);
        }
        
        saveRecentUrls();
        
        Log.d(TAG, "Added URL to history: " + url);
    }
    
    public List<Map<String, String>> getRecentUrls() {
        return new ArrayList<>(recentUrls);
    }
    
    public boolean isValidUrl(String url) {
        if (url == null || url.trim().isEmpty()) {
            return false;
        }
        
        // Check supported URL schemes
        return url.startsWith("file://lynx?local://") ||
               url.startsWith("assets://") ||
               url.startsWith("http://") ||
               url.startsWith("https://");
    }
    
    public void clearHistory() {
        recentUrls.clear();
        saveRecentUrls();
    }
    
    private void loadRecentUrls() {
        String urls_in_recent = this.prefs.getString(KEY_RECENT_URLS, "");
        recentUrls = new ArrayList<Map<String, String>>();
        if (!urls_in_recent.isEmpty()){
            String[] fileOutputs = urls_in_recent.split(",");
            for (int ind = 0; ind < fileOutputs.length; ind++){
                String[] lineOutputs = fileOutputs[ind].split("\\|");
                Map<String, String> entry = new HashMap<>();
                if (lineOutputs.length >= 2){
                    entry.put("url", lineOutputs[0]);
                    entry.put("time", lineOutputs[1]);
                    recentUrls.add(entry);
                }
            }

        }
    }
    
    private void saveRecentUrls() {
        ArrayList<String> url_inputs = new ArrayList<String>();
        for (int ind = 0; ind < recentUrls.size(); ind++){
            String urlAndDate = recentUrls.get(ind).get("url")+ "\\|" + recentUrls.get(ind).get("time");
            url_inputs.add(urlAndDate);
        }
        String saving_urls = String.join(",", url_inputs);
        SharedPreferences.Editor editor = this.prefs.edit();
        editor.putString(KEY_RECENT_URLS, saving_urls);
        editor.apply();
        Log.d(TAG, "Recent URLs saved:" + saving_urls);
    }
}
