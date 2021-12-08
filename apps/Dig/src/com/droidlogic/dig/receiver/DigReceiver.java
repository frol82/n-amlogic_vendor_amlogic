/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

package com.droidlogic.dig.receiver;

import com.droidlogic.dig.activity.RebootActivity;
import com.droidlogic.dig.activity.RestoreSystemActivity;
import com.droidlogic.dig.activity.WipeActivity;
import com.droidlogic.dig.connector.DigConnector;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class DigReceiver extends BroadcastReceiver {
	private static final String TAG = "DigReceiver";

	@Override
	public void onReceive (Context context, Intent intent) {
		Log.d(TAG, "onReceive[" + intent.getAction() + "]");

		if ("android.intent.action.BOOT_COMPLETED".equals(intent.getAction())) {
			startDigConnector(context);
		} else if ("com.droidlogic.dig.DATA_READ_ONLY".equals(intent.getAction())) {
			Intent i = new Intent(context, RebootActivity.class);
			i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
			context.startActivity(i);
		} else if ("com.droidlogic.dig.DATA_CRASH".equals(intent.getAction())) {
			Intent i = new Intent(context, WipeActivity.class);
			i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
			context.startActivity(i);
		} else if ("com.droidlogic.dig.SYSTEM_CHANGED".equals(intent.getAction())) {
			String path = intent.getExtras().getString("error_file_path");
			Intent i = new Intent(context, RestoreSystemActivity.class);
			i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
			i.putExtra("error_file_path", path);
			context.startActivity(i);
		}
	}

	private void startDigConnector(Context context) {
		new DigConnector(context).startConnector();
	}
}