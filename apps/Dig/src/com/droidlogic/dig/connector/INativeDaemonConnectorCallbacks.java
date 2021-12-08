/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

package com.droidlogic.dig.connector;

interface INativeDaemonConnectorCallbacks {
	void onDaemonConnected();
	boolean onCheckHoldWakeLock(int code);
	boolean onEvent(int code, String raw, String[] cooked);
}
