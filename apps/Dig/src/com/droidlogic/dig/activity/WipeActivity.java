/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

package com.droidlogic.dig.activity;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.Window;

import com.droidlogic.dig.R;

public class WipeActivity extends Activity {
	private final static String TAG = "Dig_WipeActivity";

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_wipe);

		Log.i(TAG, "WipeActivity.onCreate");
		findViewById(R.id.confirm_btn).setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				wipeData();
				//finish();
			}
		});
	}

	private void wipeData() {
		Log.i(TAG, "wipeData");
		sendBroadcast(new Intent("android.intent.action.MASTER_CLEAR"));
	}
}
