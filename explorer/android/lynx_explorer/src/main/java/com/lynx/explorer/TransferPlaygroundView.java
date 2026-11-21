// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer;

import android.content.Context;
import android.text.InputType;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.LynxViewClient;
import com.lynx.tasm.transfer.LynxTransferredView;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

final class TransferPlaygroundView extends LinearLayout {
  private final LynxView mOwner;
  private final EditText mComponentName;
  private final EditText mProps;
  private final Spinner mSelection;
  private final ArrayAdapter<String> mLabels;
  private final LinearLayout mCards;
  private final List<LynxTransferredView> mViews = new ArrayList<>();
  private final Button mSetProps;
  private final Button mRemove;
  private int mNextInstance;

  TransferPlaygroundView(Context context, LynxView owner) {
    super(context);
    mOwner = owner;
    setOrientation(VERTICAL);
    addView(owner, new LayoutParams(LayoutParams.MATCH_PARENT, 1));

    mComponentName = new EditText(context);
    mComponentName.setSingleLine();
    mComponentName.setHint("Component name");
    addView(mComponentName);

    mProps = new EditText(context);
    mProps.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_MULTI_LINE
        | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
    mProps.setMaxLines(3);
    mProps.setHint("Props (JSON object)");
    addView(mProps);

    mLabels = new ArrayAdapter<>(context, android.R.layout.simple_spinner_item);
    mLabels.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
    mSelection = new Spinner(context);
    mSelection.setContentDescription("Transfer instance");
    mSelection.setAdapter(mLabels);
    addView(mSelection);

    LinearLayout actions = new LinearLayout(context);
    Button create = addAction(actions, "Create", this::createTransfer);
    create.setEnabled(false);
    mSetProps = addAction(actions, "Set props", this::setTransferProps);
    mRemove = addAction(actions, "Remove", this::removeTransfer);
    mSetProps.setEnabled(false);
    mRemove.setEnabled(false);
    addView(actions);

    TextView status = new TextView(context);
    status.setText("Loading bundle…");
    addView(status);

    ScrollView scroll = new ScrollView(context);
    mCards = new LinearLayout(context);
    mCards.setOrientation(VERTICAL);
    scroll.addView(mCards);
    addView(scroll, new LayoutParams(LayoutParams.MATCH_PARENT, 0, 1));

    owner.addLynxViewClient(new LynxViewClient() {
      @Override
      public void onLoadSuccess() {
        post(() -> {
          create.setEnabled(true);
          status.setVisibility(View.GONE);
        });
      }

      @Override
      public void onLoadFailed(String message) {
        post(() -> {
          create.setEnabled(false);
          status.setText(message);
          status.setVisibility(View.VISIBLE);
        });
      }
    });
  }

  private Button addAction(LinearLayout actions, String title, Runnable action) {
    Button button = new Button(getContext());
    button.setText(title);
    button.setOnClickListener(view -> action.run());
    actions.addView(button, new LayoutParams(0, LayoutParams.WRAP_CONTENT, 1));
    return button;
  }

  private void createTransfer() {
    String name = mComponentName.getText().toString().trim();
    if (name.isEmpty()) {
      mComponentName.setError("Enter a registered component name");
      return;
    }
    JavaOnlyMap props = readProps();
    if (props == null) {
      return;
    }
    LynxTransferredView view;
    try {
      view = mOwner.loadTransfer(name, props);
    } catch (IllegalStateException error) {
      mComponentName.setError(error.getMessage());
      return;
    }
    mCards.addView(view, new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
    mViews.add(view);
    mLabels.add(name + " #" + ++mNextInstance);
    mSelection.setSelection(mViews.size() - 1);
    mSetProps.setEnabled(true);
    mRemove.setEnabled(true);
  }

  private void setTransferProps() {
    int index = mSelection.getSelectedItemPosition();
    JavaOnlyMap props = readProps();
    if (index >= 0 && index < mViews.size() && props != null) {
      mViews.get(index).setProps(props);
    }
  }

  private void removeTransfer() {
    int index = mSelection.getSelectedItemPosition();
    if (index < 0 || index >= mViews.size()) {
      return;
    }
    LynxTransferredView view = mViews.remove(index);
    view.release();
    mCards.removeView(view);
    mLabels.remove(mLabels.getItem(index));
    mSetProps.setEnabled(!mViews.isEmpty());
    mRemove.setEnabled(!mViews.isEmpty());
  }

  @Nullable
  private JavaOnlyMap readProps() {
    String text = mProps.getText().toString().trim();
    try {
      return text.isEmpty() ? new JavaOnlyMap() : readObject(new JSONObject(text));
    } catch (JSONException error) {
      mProps.setError("Enter a JSON object");
      return null;
    }
  }

  private static JavaOnlyMap readObject(JSONObject object) throws JSONException {
    JavaOnlyMap map = new JavaOnlyMap();
    for (Iterator<String> keys = object.keys(); keys.hasNext();) {
      String key = keys.next();
      map.put(key, readValue(object.get(key)));
    }
    return map;
  }

  @Nullable
  private static Object readValue(Object value) throws JSONException {
    if (value == JSONObject.NULL) {
      return null;
    }
    if (value instanceof JSONObject) {
      return readObject((JSONObject) value);
    }
    if (value instanceof JSONArray) {
      JSONArray json = (JSONArray) value;
      JavaOnlyArray array = new JavaOnlyArray();
      for (int index = 0; index < json.length(); index++) {
        array.add(readValue(json.get(index)));
      }
      return array;
    }
    return value;
  }
}
