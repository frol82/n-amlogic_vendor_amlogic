/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 */

package com.droidlogic.HdmiSwitch;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.KeyguardManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.widget.Toast;

import android.view.KeyEvent;
import android.os.PowerManager;
import android.os.RemoteException;
import android.os.SystemClock;

import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import android.content.SharedPreferences;

import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.OutputModeManager;
import com.droidlogic.HdmiSwitch.R;

public class HdmiBroadcastReceiver extends BroadcastReceiver {
    private static final String TAG = "HdmiBroadcastReceiver";

    // Use a layout id for a unique identifier
    private static final int HDMI_NOTIFICATIONS = R.layout.main;

    private static final String ACTION_PLAYER_CRASHED = "com.farcore.videoplayer.PLAYER_CRASHED";

    /**
    * Sticky broadcast of the current HDMI plugged state.
    */
    public final static String WINDOW_MANAGER_ACTION_HDMI_PLUGGED = "android.intent.action.HDMI_PLUGGED";
    /**
     * Extra in {@link #ACTION_HDMI_PLUGGED} indicating the state: true if
     * plugged in to HDMI, false if not.
     */
    public final static String WINDOW_MANAGER_EXTRA_HDMI_PLUGGED_STATE = "state";

    private static boolean mSystemReady = false;
    private HdmiOutputModeManager mHdmiOutputManager = null;
    private SystemControlManager mSystemControl = null;

    private static boolean isSingleOutput = false;
    private static boolean isDualDisplay = false;
    private static boolean isDualDisplay2 = false;
    private static boolean isDualDisplay3 = false;
    private static boolean isDualDisplay4 = false;
    private static boolean isNeedPlayerExit = true;
    private static boolean isHdmiAllSwitch = false;
    private static boolean isRealExternalDisplay = false;

    @Override
    public void onReceive (Context context, Intent intent) {
        Log.d (TAG, "onReceive: " + intent.getAction() );
        mHdmiOutputManager = new HdmiOutputModeManager (context);
        mSystemControl = new SystemControlManager (context);

        isSingleOutput = mSystemControl.getPropertyBoolean ("ro.module.singleoutput", false);
        isDualDisplay = mSystemControl.getPropertyBoolean ("ro.vout.dualdisplay", false);
        isDualDisplay2 = mSystemControl.getPropertyBoolean ("ro.vout.dualdisplay2", false);
        isDualDisplay3 = mSystemControl.getPropertyBoolean ("ro.vout.dualdisplay3", false);
        isDualDisplay4 = mSystemControl.getPropertyBoolean ("ro.vout.dualdisplay4", false);
        isNeedPlayerExit = mSystemControl.getPropertyBoolean ("ro.vout.player.exit", true);
        isHdmiAllSwitch = mSystemControl.getPropertyBoolean ("ro.app.hdmi.allswitch", false);
        isRealExternalDisplay = mSystemControl.getPropertyBoolean ("ro.real.externaldisplay", false);

        if (Intent.ACTION_BOOT_COMPLETED.equals (intent.getAction() ) ) {
            mSystemReady = true;
            mHdmiOutputManager.resetFreescaleStatus();
            if (mHdmiOutputManager.isHdmiPlugged() ) {
                NotificationManager nM = (NotificationManager) context.getSystemService (context.NOTIFICATION_SERVICE);

                CharSequence text = context.getText (R.string.hdmi_state_str1);
                Notification notification = new Notification (R.drawable.stat_connected, text, System.currentTimeMillis() );

                Intent it = new Intent (context, HdmiSwitch.class);
                it.setFlags (Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
                PendingIntent contentIntent = PendingIntent.getActivity (context, 0, it, 0);
                notification.setLatestEventInfo (context, context.getText (R.string.app_name), text, contentIntent);

                nM.notify (HDMI_NOTIFICATIONS, notification);
                onHdmiPlugged (context);
            }
        } else if (WINDOW_MANAGER_ACTION_HDMI_PLUGGED.equals (intent.getAction() ) ) {

            boolean plugged = intent.getBooleanExtra (WINDOW_MANAGER_EXTRA_HDMI_PLUGGED_STATE, false);
            if (plugged) {
                NotificationManager nM = (NotificationManager) context.getSystemService (context.NOTIFICATION_SERVICE);

                CharSequence text = context.getText (R.string.hdmi_state_str1);
                Notification notification = new Notification (R.drawable.stat_connected, text, System.currentTimeMillis() );
                Intent it = new Intent (context, HdmiSwitch.class);
                it.setFlags (Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
                PendingIntent contentIntent = PendingIntent.getActivity (context, 0, it, 0);
                notification.setLatestEventInfo (context, context.getText (R.string.app_name), text, contentIntent);

                nM.notify (HDMI_NOTIFICATIONS, notification);
                onHdmiPlugged (context);
            } else {
                Log.d (TAG, "onReceive: 00" );
                int repeatCount = 3;
                while (mHdmiOutputManager.isHdmiPlugged() && repeatCount <= 0) {
                    Log.d (TAG, "onReceive: 01   " + mHdmiOutputManager.isHdmiPlugged() );
                    repeatCount--;
                    try {
                        Thread.currentThread().sleep (1000);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
                Log.d (TAG, "onReceive: 02" );

                onHdmiUnplugged (context);

                NotificationManager nM = (NotificationManager) context.getSystemService (context.NOTIFICATION_SERVICE);
                nM.cancel (HDMI_NOTIFICATIONS);
            }
        } else if (ACTION_PLAYER_CRASHED.equals (intent.getAction() ) ) {
            mHdmiOutputManager.onVideoPlayerCrashed();
        } else if (Intent.ACTION_USER_PRESENT.equals (intent.getAction() ) ) {
            mSystemReady = true;
            if (isHdmiAllSwitch) {
                if (mHdmiOutputManager.isHdmiPlugged() ) {
                    onHdmiPlugged (context);
                }
            }
        }
    }

    private void onHdmiPlugged (Context context) {
        final String curMode = mHdmiOutputManager.getCurMode();
        final boolean playerNotPause = (mHdmiOutputManager.isSinglePortraitDisplay() && isRealExternalDisplay);
        SharedPreferences prefs = context.getSharedPreferences (HdmiSwitch.PRESS_KEY, Context.MODE_PRIVATE);
        int autoSwitchEnabled = 1; //Settings.System.getInt(context.getContentResolver(), Settings.System.HDMI_AUTO_SWITCH, 1);

        Log.w (TAG, "onHdmiPlugged isDualDisplay: " + isDualDisplay + " isDualDisplay4: " + isDualDisplay4 +
               " mSystemReady: " + mSystemReady);
        if (isDualDisplay && !isDualDisplay4) {
            return;
        }
        if (!mSystemReady ) {
            return;
        }
        if (autoSwitchEnabled != 1) {
            return;
        }
        Log.w (TAG, "onHdmiPlugged curMode: " + curMode);
        if (curMode.equals ("null") || curMode.equals ("panel") ) {
            // screen on
            PowerManager powerManager = (PowerManager) context.getSystemService (context.POWER_SERVICE);
            if (!powerManager.isScreenOn() ) {
                Log.w (TAG, "onHdmiPlugged, screen is off");
                return;
            }

            // camera in-use
            if (mHdmiOutputManager.isCameraBusy() ) {
                Log.w (TAG, "onHdmiPlugged, camera is busy");
                Toast.makeText (context,
                                context.getText (R.string.Toast_msg_camera_busy),
                                Toast.LENGTH_LONG).show();
                return;
            }

            // keyguard on
            boolean mNotCheckKygd = mHdmiOutputManager.getPropertyBoolean ("ro.module.dualscaler", false);
            KeyguardManager mKeyguardManager = (KeyguardManager) context.getSystemService (context.KEYGUARD_SERVICE);
            if ( mKeyguardManager != null && mKeyguardManager.inKeyguardRestrictedInputMode() && !mNotCheckKygd ) {
                Log.w (TAG, "onHdmiPlugged, keyguard on");
                return;
            }

            if (isNeedPlayerExit) {
                /// send BACK key to stop other player
                sendKeyEvent (context);
            }

            // show the cling when it auto connected
            if (isSingleOutput) {
                Log.v (TAG, "singleoutput ok, dualdispaly false");
                /*if (!prefs.getBoolean(HdmiCling.CLING_DISMISS_KEY_720P, false)) {
                    Intent i = new Intent(context, ShowCling.class);
                    i.putExtra("on_which", HdmiCling.CLING_DISMISS_KEY_720P);
                    i.putExtra("which_cling", "first");
                    i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    context.startActivity(i);
                }*/
            }
            mHdmiOutputManager.setMode ("720p");
            //sendPluggedIntent(context);

            //real external display:
            if (isRealExternalDisplay) {
                return;
            }
            //dual display 4:
            if (isDualDisplay4) {
                return;
            }

            if (isDualDisplay2 || isDualDisplay3) {
                mHdmiOutputManager.setDualDisplayStatic (true, (mHdmiOutputManager.getDualDisplayState() == 1) );
            }
            mHdmiOutputManager.setFbBlank ("fb0", "0");
        }
        return;
    }

    private void onHdmiUnplugged (Context context) {
        final String curMode = mHdmiOutputManager.getCurMode();

        Log.d (TAG, "onHdmiUnplugged" );
        Log.d (TAG, "isDualDisplay: " + isDualDisplay);
        Log.d (TAG, "isDualDisplay4: " + isDualDisplay4 );
        Log.d (TAG, "isDualDisplay2: " + isDualDisplay2 );
        Log.d (TAG, "isDualDisplay3: " + isDualDisplay3 );

        if ( isDualDisplay && !isDualDisplay4 ) {
            return;
        }
        if ( isDualDisplay4 && curMode.equals ("null") ) {
            return;
        }
        if ( !isDualDisplay4 && curMode.equals ("panel") ) {
            return;
        }

        if ( isDualDisplay2 || isDualDisplay3 ) {
            mHdmiOutputManager.setVout2OffStatic();
        }

        if ( isNeedPlayerExit ) {
            /// 2. send BACK key to stop player
            sendKeyEvent (context);
        }

        if ( isDualDisplay4 ) {
            mHdmiOutputManager.setMode ("null");
            //sendUnpluggedIntent(context);
            return;
        } else {
            //if ( isDualDisplay2 || isDualDisplay3 ) {
            if ( isNeedPlayerExit ) {
                context.startService (new Intent (context, HdmiDelayedService.class) );
            } else {
                mHdmiOutputManager.setMode ("panel");
                //sendUnpluggedIntent(context);

                //real external display
                if ( isRealExternalDisplay ) {
                    return;
                }

                mHdmiOutputManager.setDualDisplayStatic (false, (mHdmiOutputManager.getDualDisplayState() == 1) );
            }
        }
    }

    private void sendPluggedIntent ( Context context ) {
        Intent it = new Intent (WINDOW_MANAGER_ACTION_HDMI_PLUGGED);
        it.putExtra (WINDOW_MANAGER_EXTRA_HDMI_PLUGGED_STATE, true);
        context.sendStickyBroadcast (it);
    }

    private void sendUnpluggedIntent ( Context context ) {
        final boolean playerNotPause = (mHdmiOutputManager.isSinglePortraitDisplay() && isRealExternalDisplay );

        Intent it = new Intent (WINDOW_MANAGER_ACTION_HDMI_PLUGGED);
        it.putExtra (WINDOW_MANAGER_EXTRA_HDMI_PLUGGED_STATE, false);
        if (playerNotPause) {
            it.putExtra ("videoplayer.need.pause", false);
        }
        context.sendStickyBroadcast (it);
    }

    /**
    * Send a single key event.
    *
    * @param event is a string representing the keycode of the key event you
    * want to execute.
    */
    private void sendKeyEvent (Context context) {
        Intent intent = new Intent();
        intent.setAction (Intent.ACTION_MAIN);
        intent.addCategory (Intent.CATEGORY_HOME);
        context.startActivity (intent);
    }
}

