/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

package com.droidlogic.dig.connector;

import android.os.Parcel;

/**
 * An exception that indicates there was an error with a
 * {@link NativeDaemonConnector} operation.
 */
@SuppressWarnings("serial")
public class NativeDaemonConnectorException extends Exception {
	private String mCmd;
	private NativeDaemonEvent mEvent;

	public NativeDaemonConnectorException(String detailMessage) {
		super(detailMessage);
	}

	public NativeDaemonConnectorException(String detailMessage, Throwable throwable) {
		super(detailMessage, throwable);
	}

	public NativeDaemonConnectorException(String cmd, NativeDaemonEvent event) {
		super("command '" + cmd + "' failed with '" + event + "'");
		mCmd = cmd;
		mEvent = event;
	}

	public int getCode() {
		return mEvent.getCode();
	}

	public String getCmd() {
		return mCmd;
	}

	/**
	 * Rethrow as a {@link RuntimeException} subclass that is handled by
	 * {@link Parcel#writeException(Exception)}.
	 */
	public IllegalArgumentException rethrowAsParcelableException() {
		throw new IllegalStateException(getMessage(), this);
	}
}

