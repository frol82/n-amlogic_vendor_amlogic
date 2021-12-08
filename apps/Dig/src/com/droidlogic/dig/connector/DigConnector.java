/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

package com.droidlogic.dig.connector;

import android.content.Context;
import android.content.Intent;
import android.os.HandlerThread;
import android.util.Log;

public class DigConnector {
	private final static String TAG = "DigConnector";

	/**
	 * Dig series from dig report
	 */
	private final static int DIG_REPORT_DATA_READ_ONLY = 680;
	private final static int DIG_REPORT_DATA_CRASH = 681;
	private final static int DIG_REPORT_SYSTEM_CHANGED = 682;

	/**
	 * Dig socket
	 */
	private static final String SOCKET_NAME = "dig";
	private static final int MAX_CONTAINERS = 250;
	private static final String SOCKET_COMMAND = "dig";

	private Context mContext;
	private static NativeDaemonConnector mConnector;

	public DigConnector(Context context) {
		mContext = context;
	}

	public void startConnector() {
		Log.i(TAG, "startConnector");
		HandlerThread thread = new HandlerThread("DigConnectorThread");
		thread.start();

		mConnector = new NativeDaemonConnector(
				new NativeCallbackReceiver(),
				SOCKET_NAME, MAX_CONTAINERS * 2,
				TAG,
				25,
				null,
				thread.getLooper());
		new Thread(mConnector, TAG).start();
	}

	public static NativeDaemonEvent sendCommand(Object... args) {
		if (mConnector != null &&
				args != null &&
				args.length >= 1) {
			try {
				return mConnector.execute(SOCKET_COMMAND, args);
			} catch (NativeDaemonConnectorException e) {
				e.printStackTrace();
			}
		}
		return null;
	}

	class NativeCallbackReceiver implements INativeDaemonConnectorCallbacks {
		private boolean mDoSend;

		NativeCallbackReceiver() {
			mDoSend = false;
		}

		/**
		 * Callback from NativeDaemonConnector
		 */
		@Override
		public void onDaemonConnected() {
			// Connect success. notity socket server
			if (!mDoSend) {
				mDoSend = true;
				new Thread() {
					public void run() {
						Log.i(TAG, "send command: connect ok");
						sendCommand("connect", "ok");
					};
				}.start();
			}
		}

		/**
		 * Callback from NativeDaemonConnector
		 */
		@Override
		public boolean onCheckHoldWakeLock(int code) {
			return false;
		}

		/**
		 * Callback from NativeDaemonConnector
		 */
		@Override
		public boolean onEvent(int code, String raw, String[] cooked) {
			final Intent intent;
			switch (code) {
			case DIG_REPORT_DATA_READ_ONLY:
				Log.i(TAG, "DIG_REPORT_DATA_READ_ONLY.");
				intent = new Intent("com.droidlogic.dig.DATA_READ_ONLY");
				mContext.sendBroadcast(intent);
				break;

			case DIG_REPORT_DATA_CRASH:
				Log.i(TAG, "DIG_REPORT_DATA_CRASH.");
				intent = new Intent("com.droidlogic.dig.DATA_CRASH");
				mContext.sendBroadcast(intent);
				break;

			case DIG_REPORT_SYSTEM_CHANGED:
				intent = new Intent("com.droidlogic.dig.SYSTEM_CHANGED");
				final String path = (cooked.length > 3) ? cooked[3] : null;
				if (path != null) intent.putExtra("error_file_path", path);
				Log.i(TAG, "DIG_REPORT_SYSTEM_CHANGED. " + path);
				mContext.sendBroadcast(intent);
				break;
			}
			return true;
		}
	}
}
