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
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.widget.TextView;

public class RestoreSystemActivity extends Activity {
	private final static String TAG = "Dig_RestoreSystemActivity";

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_restore_system);

		Log.i(TAG, "RestoreSystemActivity.onCreate");

		String strErrorFile = getIntent().getStringExtra("error_file_path");
		if (strErrorFile != null) {
			((TextView)findViewById(R.id.erorr_file_tv)).setText(strErrorFile);
		}

		findViewById(R.id.confirm_btn).setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				restoreSystem();
				//finish();
			}
		});
	}

	private void restoreSystem() {
		Log.i(TAG, "restoreSystem");
		sendBroadcast(new Intent("android.intent.action.SYSTEM_RESTORE"));
	}
}
