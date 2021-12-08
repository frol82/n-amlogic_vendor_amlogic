/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

package com.droidlogic.dig.activity;

import com.droidlogic.dig.R;

import android.app.Activity;
import android.os.Bundle;
import android.os.PowerManager;
import android.util.Log;
import android.view.View;
import android.view.Window;

public class RebootActivity extends Activity {
	private final static String TAG = "Dig_RbootActivity";

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_reboot);

		Log.i(TAG, "RebootActivity.onCreate");
		findViewById(R.id.confirm_btn).setOnClickListener(new View.OnClickListener() {
			public void onClick(View view) {
				reboot();
			}
		});
	}

	private void reboot() {
		Log.i(TAG, "reboot");
		PowerManager pm = (PowerManager) getSystemService(POWER_SERVICE);
		pm.reboot("");
	}
}
