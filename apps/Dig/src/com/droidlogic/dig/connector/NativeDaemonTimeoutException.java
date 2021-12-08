/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

package com.droidlogic.dig.connector;

/**
 * An exception that indicates there was a timeout with a
 * {@link NativeDaemonConnector} operation.
 */
@SuppressWarnings("serial")
public class NativeDaemonTimeoutException extends NativeDaemonConnectorException {
	public NativeDaemonTimeoutException(String command, NativeDaemonEvent event) {
		super(command, event);
	}
}
