/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 */

package com.droidlogic.HdmiSwitch;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
//import android.view.WindowManagerPolicy;

import android.util.Log;

public class HdmiDelayedService extends Service {
    private static final String TAG = "HdmiDelayedService";

    private Handler mProgressHandler;
    private Context mContext;

    private HdmiOutputModeManager mHdmiOutputManager = null;

    @Override
    public void onCreate() {
        mContext = this;
        mProgressHandler = new DelayedHandler();
        mHdmiOutputManager = new HdmiOutputModeManager (mContext);
    }

    @Override
    public int onStartCommand (Intent intent, int flags, int startId) {
        /* start after DELAY */
        if (mProgressHandler != null) {
            mProgressHandler.sendEmptyMessageDelayed (HdmiSwitch.MSG_CONFIRM_DIALOG,
                    3 * HdmiSwitch.ONE_SEC_DELAY);
        }
        return super.onStartCommand (intent, flags, startId);
    }

    @Override
    public void onDestroy() {
    }

    @Override
    public IBinder onBind (Intent intent) {
        return null;
    }

    private class DelayedHandler extends Handler {
        @Override
        public void handleMessage (Message msg) {
            super.handleMessage (msg);
            onDelayedProcess();
        }
    }

    private void sendUnpluggedIntent ( Context context ) {
        final boolean playerNotPause = ( HdmiOutputModeManager.isSinglePortraitDisplay()
                                         && HdmiOutputModeManager.getPropRealExternalDisplay() );

        Intent it = new Intent (HdmiBroadcastReceiver.WINDOW_MANAGER_ACTION_HDMI_PLUGGED);
        it.putExtra (HdmiBroadcastReceiver.WINDOW_MANAGER_EXTRA_HDMI_PLUGGED_STATE, false);
        if (playerNotPause) {
            it.putExtra ("videoplayer.need.pause", false);
        }
        context.sendStickyBroadcast (it);
    }

    private void onDelayedProcess() {
        mHdmiOutputManager.setMode ("panel");
        if ( (HdmiOutputModeManager.getPropDualDisplay2() || HdmiOutputModeManager.getPropDualDisplay3() )
                && !HdmiOutputModeManager.getPropRealExternalDisplay() ) {
            mHdmiOutputManager.setDualDisplayStatic (false, (mHdmiOutputManager.getDualDisplayState() == 1) );
        }
        sendUnpluggedIntent (mContext);
    }

}
