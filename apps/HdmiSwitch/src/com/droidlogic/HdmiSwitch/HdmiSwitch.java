/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 */

package com.droidlogic.HdmiSwitch;

import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.droidlogic.HdmiSwitch.R;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.ObjectAnimator;
import android.app.Activity;
import android.app.ActivityManager;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.util.Log;
import android.view.Display;
import android.view.Menu;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.view.Window;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;
import android.view.animation.AccelerateInterpolator;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.SimpleAdapter;
import android.widget.TextView;
import android.widget.Toast;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;

public class HdmiSwitch extends Activity {
    private static final String TAG = "HdmiSwitch";

    //Cling Related
    public static final String PRESS_KEY = "com.amlogic.HdmiSwitch.prefs";
    private static String SHOW_CLING = "com.amlogic.HdmiSwitch.action.SHOW_CLING";

    public static final int MSG_CONFIRM_DIALOG = 0;
    public static final int ONE_SEC_DELAY = 1000;
    private static final int MSG_CHECK_HDMI = 1;
    private static final int MSG_SETMODE_FINISH_CONFIRM_DIALOG = 2;
    private static final int MSG_SETMODE_FINISH = 3;
    private static final int MSG_DELAY_PANEL_ON = 4;

    private static final int CONFIRM_DIALOG_ID = 0;
    private static final int MAX_PROGRESS = 15;
    private static final int STOP_PROGRESS = -1;
    private int mProgress;
    private int mProgress2;
    private Handler mProgressHandler;

    private static final boolean HDMI_CONNECTED = true;
    private static final boolean HDMI_DISCONNECTED = false;
    private static boolean hdmi_stat = HDMI_DISCONNECTED;
    private static boolean hdmi_stat_old = HDMI_DISCONNECTED;

    private AlertDialog confirm_dialog;
    private static String old_mode = "panel";

    private ListView lv;

    private HdmiOutputModeManager mHdmiOutputManager = null;

    /** Called when the activity is first created. */
    @Override
    public void onCreate (Bundle savedInstanceState) {
        super.onCreate (savedInstanceState);
        setContentView (R.layout.main);

        mHdmiOutputManager = new HdmiOutputModeManager (this);
        /* set window size */
        WindowManager wm = getWindowManager();
        Display display = wm.getDefaultDisplay();
        LayoutParams lp = getWindow().getAttributes();
        if (display.getHeight() > display.getWidth() ) {
            //lp.height = (int) (display.getHeight() * 0.5);
            lp.width = (int) (display.getWidth() * 1.0);
        } else {
            //lp.height = (int) (display.getHeight() * 0.75);
            lp.width = (int) (display.getWidth() * 0.5);
        }
        getWindow().setAttributes (lp);

        /* check driver interface */
        TextView tv = (TextView) findViewById (R.id.hdmi_state_str);
        File file = new File (HdmiOutputModeManager.SYSFS_DISP_CAP);
        if (!file.exists() ) {
            tv.setText (getText (R.string.driver_api_err) + "[001]");
            return;
        }
        file = new File (HdmiOutputModeManager.SYSFS_DISPLAY_MODE);
        if (!file.exists() ) {
            tv.setText (getText (R.string.driver_api_err) + "[010]");
            return;
        }
        file = new File (HdmiOutputModeManager.SYSFS_DISPLAY_AXIS);
        if (!file.exists() ) {
            tv.setText (getText (R.string.driver_api_err) + "[100]");
            return;
        }

        /* update hdmi_state_str*/
        if (isHdmiPlugged() ) {
            tv.setText (getText (R.string.hdmi_state_str1) );
        } else {
            tv.setText (getText (R.string.hdmi_state_str2) );
        }

        /* setup video mode list */
        lv = (ListView) findViewById (R.id.listview);
        SimpleAdapter adapter = new SimpleAdapter (this, getListData(), R.layout.list_item,
                new String[] {"item_text", "item_img"},
                new int[] {R.id.item_text, R.id.item_img});
        lv.setAdapter (adapter);

        /* mode select listener */
        lv.setOnItemClickListener (new OnItemClickListener() {
            public void onItemClick (AdapterView<?> parent, View view, int pos, long id) {
                Map<String, Object> item = (Map<String, Object>) parent.getItemAtPosition (pos);
                if (item.get ("item_img").equals (R.drawable.item_img_unsel) ) {
                    old_mode = getCurMode();
                    if ( (HdmiOutputModeManager.getPropDualDisplay2()
                            || HdmiOutputModeManager.getPropDualDisplay3() )
                    && (mHdmiOutputManager.isCameraBusy() ) ) {
                        Log.w (TAG, "setDualDisplay, camera is busy");
                        Toast.makeText (HdmiSwitch.this,
                                        getText (R.string.Toast_msg_camera_busy),
                                        Toast.LENGTH_LONG).show();
                        return;
                    }
                    final String mode = (String) item.get ("mode");

                    Log.v (TAG, "handle creat mode " + mode);
                    new Thread ("setMode") {
                        @Override
                        public void run() {
                            Log.v (TAG, "handle creat mode " + mode);
                            setMode (mode);
                            mProgressHandler.sendEmptyMessage (MSG_SETMODE_FINISH_CONFIRM_DIALOG);
                        }
                    } .start();
                }
            }
        });

        /* progress handler*/
        mProgressHandler = new HdmiSwitchProgressHandler();
    }

    /** onResume() */
    @Override
    public void onResume() {
        super.onResume();

        /* check driver interface */
        File file = new File (HdmiOutputModeManager.SYSFS_DISP_CAP);
        if (!file.exists() ) {
            return;
        }
        file = new File (HdmiOutputModeManager.SYSFS_DISPLAY_MODE);
        if (!file.exists() ) {
            return;
        }
        file = new File (HdmiOutputModeManager.SYSFS_DISPLAY_AXIS);
        if (!file.exists() ) {
            return;
        }

        mProgress2 = 0;
        mProgressHandler.sendEmptyMessageDelayed (MSG_CHECK_HDMI, ONE_SEC_DELAY / 10);
    }

    /** onPause() */
    @Override
    public void onPause() {
        super.onPause();

        hdmi_stat_old = isHdmiPlugged();
        mProgress = STOP_PROGRESS;
        mProgress2 = STOP_PROGRESS;
    }

    /** Confirm Dialog */
    @Override
    protected Dialog onCreateDialog (int id) {
        if (id == CONFIRM_DIALOG_ID) {
            confirm_dialog =  new AlertDialog.Builder (HdmiSwitch.this)
            //.setIcon(R.drawable.dialog_icon)
            .setTitle (R.string.dialog_title)
            .setPositiveButton (R.string.dialog_str_ok, new DialogInterface.OnClickListener() {
                public void onClick (DialogInterface dialog, int whichButton) {
                    mProgress = STOP_PROGRESS;
                    finish();
                    /* User clicked OK so do some stuff */
                }
            })
            .setNegativeButton (R.string.dialog_str_cancel, new DialogInterface.OnClickListener() {
                public void onClick (DialogInterface dialog, int whichButton) {
                    mProgress = STOP_PROGRESS;
                    final String mode = old_mode;

                    Log.v (TAG, "handle click mode " + mode);
                    new Thread ("setMode") {
                        @Override
                        public void run() {
                            Log.v (TAG, "handle click mode " + mode);
                            setMode (mode);
                            mProgressHandler.sendEmptyMessage (MSG_SETMODE_FINISH);
                        }
                    } .start();
                    /* User clicked Cancel so do some stuff */
                }
            })
            .setOnCancelListener (new DialogInterface.OnCancelListener() {
                public void onCancel (DialogInterface dialog) {
                    mProgress = STOP_PROGRESS;
                }
            })
            .create();

            return confirm_dialog;
        }

        return null;
    }

    @Override
    protected void onPrepareDialog (int id, Dialog dialog) {
        if (id == CONFIRM_DIALOG_ID) {
            WindowManager wm = getWindowManager();
            Display display = wm.getDefaultDisplay();
            LayoutParams lp = dialog.getWindow().getAttributes();
            if (display.getHeight() > display.getWidth() ) {
                lp.width = (int) (display.getWidth() * 1.0);
            } else {
                lp.width = (int) (display.getWidth() * 0.5);
            }
            dialog.getWindow().setAttributes (lp);

            ( (AlertDialog) dialog).getButton (AlertDialog.BUTTON_NEGATIVE)
            .setText (getText (R.string.dialog_str_cancel)
                      + " (" + MAX_PROGRESS + ")");

            mProgress = 0;
            mProgressHandler.sendEmptyMessageDelayed (MSG_CONFIRM_DIALOG, ONE_SEC_DELAY);
        }
    }

    /** updateListDisplay */
    private void updateListDisplay() {
        Map<String, Object> list_item;

        for (int i = 0; i < lv.getAdapter().getCount(); i++) {
            list_item = (Map<String, Object>) lv.getAdapter().getItem (i);
            if (list_item.get ("mode").equals (getCurMode() ) ) {
                list_item.put ("item_img", R.drawable.item_img_sel);
            } else {
                list_item.put ("item_img", R.drawable.item_img_unsel);
            }
        }
        ( (BaseAdapter) lv.getAdapter() ).notifyDataSetChanged();
    }

    /** updateActivityDisplay */
    private void updateActivityDisplay() {
        /* update hdmi_state_str*/
        TextView tv = (TextView) findViewById (R.id.hdmi_state_str);
        if (isHdmiPlugged() ) {
            tv.setText (getText (R.string.hdmi_state_str1) );
        } else {
            tv.setText (getText (R.string.hdmi_state_str2) );
        }

        /* update video mode list */
        lv = (ListView) findViewById (R.id.listview);
        SimpleAdapter adapter = new SimpleAdapter (this, getListData(), R.layout.list_item,
                new String[] {"item_text", "item_img"},
                new int[] {R.id.item_text, R.id.item_img});
        lv.setAdapter (adapter);

        ( (BaseAdapter) lv.getAdapter() ).notifyDataSetChanged();
    }

    /** sendTvOutIntent **/
    private void sendTvOutIntent ( boolean plugged ) {
        Intent intent = new Intent (HdmiBroadcastReceiver.WINDOW_MANAGER_ACTION_HDMI_PLUGGED);
        intent.putExtra (HdmiBroadcastReceiver.WINDOW_MANAGER_EXTRA_HDMI_PLUGGED_STATE, plugged);
        sendStickyBroadcast (intent);
    }

    private void notifyModeChanged() {
        if (HdmiOutputModeManager.getPropDualDisplay() ) {
            return;
        }

        if (HdmiOutputModeManager.getPropDualDisplay4() ) {
            sendTvOutIntent (getCurMode().equals ("null") ? false : true);
            return;
        }

        if (getCurMode().equals ("panel") ) {
            sendTvOutIntent (false);
        } else {
            sendTvOutIntent (true);
        }
    }

    private void onSetModeFinished ( int switch_case ) {
        if (HdmiOutputModeManager.getPropDualDisplay4() ) {
            //notifyModeChanged();
            updateListDisplay();
            if (MSG_SETMODE_FINISH_CONFIRM_DIALOG == switch_case) {
                finish();
            }
            return;
        }
        {
            boolean plug = isHdmiPlugged();
            if (plug) {
                if (HdmiOutputModeManager.getPropDualDisplay2() ) {
                    setFbBlank ("fb0", "1");
                }
                if (HdmiOutputModeManager.getPropDualDisplay3() ) {
                    setFbBlank ("fb2", "1");
                }
            }
            if (!HdmiOutputModeManager.getPropRealExternalDisplay() ) {
                setDualDisplay (plug);
            }
            if (plug) {
                mProgressHandler.sendEmptyMessageDelayed (MSG_DELAY_PANEL_ON, ONE_SEC_DELAY);
            }
        }
        //notifyModeChanged();
        updateListDisplay();
        if ( (MSG_SETMODE_FINISH_CONFIRM_DIALOG == switch_case)
                && !HdmiOutputModeManager.getPropDualDisplay() ) {
            if (isHdmiPlugged() ) {
                showDialog (CONFIRM_DIALOG_ID);
            } else {
                finish();
            }
        }
    }

    /** process handler */
    private class HdmiSwitchProgressHandler extends Handler {
        @Override
        public void handleMessage (Message msg) {
            super.handleMessage (msg);
            switch (msg.what) {
                // confirm dialog
            case MSG_CONFIRM_DIALOG:
                if (mProgress == STOP_PROGRESS) {
                    return;
                }

                if (mProgress >= MAX_PROGRESS) {
                    final String mode = old_mode;
                    Log.v (TAG, "handle message mode " + mode);
                    new Thread ("setMode") {
                        @Override
                        public void run() {
                            Log.v (TAG, "handle message mode " + mode);
                            setMode (mode);
                            mProgressHandler.sendEmptyMessage (MSG_SETMODE_FINISH);
                        }
                    } .start();

                    confirm_dialog.dismiss();
                } else {
                    mProgress++;
                    confirm_dialog.getButton (AlertDialog.BUTTON_NEGATIVE)
                    .setText (getText (R.string.dialog_str_cancel)
                              + " (" + (MAX_PROGRESS - mProgress) + ")");

                    mProgressHandler.sendEmptyMessageDelayed (MSG_CONFIRM_DIALOG, ONE_SEC_DELAY);
                }
                break;
                // hdmi check
            case MSG_CHECK_HDMI:
                if (mProgress2 == STOP_PROGRESS) {
                    return;
                }

                hdmi_stat = isHdmiPlugged();
                if (hdmi_stat_old == HDMI_DISCONNECTED) {
                    if (hdmi_stat == HDMI_CONNECTED) {
                        hdmi_stat_old = hdmi_stat;

                        if (confirm_dialog != null) {
                            mProgress = STOP_PROGRESS;
                            confirm_dialog.dismiss();
                        }
                        updateActivityDisplay();
                    }
                } else {
                    if (hdmi_stat == HDMI_DISCONNECTED) {
                        hdmi_stat_old = hdmi_stat;

                        if (confirm_dialog != null) {
                            mProgress = STOP_PROGRESS;
                            confirm_dialog.dismiss();
                        }
                        updateActivityDisplay();
                    }
                }
                mProgressHandler.sendEmptyMessageDelayed (MSG_CHECK_HDMI, 3 * ONE_SEC_DELAY);
                break;
                // setMode finish, show confirm dialog
            case MSG_SETMODE_FINISH_CONFIRM_DIALOG:
                onSetModeFinished (MSG_SETMODE_FINISH_CONFIRM_DIALOG);
                break;
                // setMode finish
            case MSG_SETMODE_FINISH:
                onSetModeFinished (MSG_SETMODE_FINISH);
                break;
                // delayed panel on
            case MSG_DELAY_PANEL_ON:
                if (HdmiOutputModeManager.getPropDualDisplay2() ) {
                    setFbBlank ("fb0", "0");
                }
                if (HdmiOutputModeManager.getPropDualDisplay3() ) {
                    setFbBlank ("fb0", "0");
                    setFbBlank ("fb2", "0");
                }
                break;
            }
        }
    }

    /** mode <-> mode_str/axis */
    private static final Map<String, Object> MODE_STR_TABLE = new HashMap<String, Object>();
    private static final Map<String, String> MODE_AXIS_TABLE = new HashMap<String, String>();
    static {
        MODE_STR_TABLE.put ("panel", R.string.mode_str_panel);
        MODE_STR_TABLE.put ("null", R.string.mode_str_panel);
        //MODE_STR_TABLE.put("480i", R.string.mode_str_480i);
        MODE_STR_TABLE.put ("480p", R.string.mode_str_480p);
        //MODE_STR_TABLE.put("576i", R.string.mode_str_576i);
        //MODE_STR_TABLE.put("576p", R.string.mode_str_576p);
        MODE_STR_TABLE.put ("720p50hz", R.string.mode_str_720p50hz);
        MODE_STR_TABLE.put ("720p", R.string.mode_str_720p);
        //MODE_STR_TABLE.put("1080i", R.string.mode_str_1080i);
        MODE_STR_TABLE.put ("1080p24hz", R.string.mode_str_1080p24hz);
        MODE_STR_TABLE.put ("1080p50hz", R.string.mode_str_1080p50hz);
        MODE_STR_TABLE.put ("1080p", R.string.mode_str_1080p);

        MODE_AXIS_TABLE.put ("panel", "0 0 800 480 0 0 18 18");
        MODE_AXIS_TABLE.put ("480i", "0 0 800 480 0 0 18 18");
        MODE_AXIS_TABLE.put ("480p", "0 0 800 480 0 0 18 18");
        MODE_AXIS_TABLE.put ("576i", "0 48 800 480 0 48 18 18");
        MODE_AXIS_TABLE.put ("576p", "0 48 800 480 0 48 18 18");
        MODE_AXIS_TABLE.put ("720p", "240 120 800 480 240 120 18 18");
        MODE_AXIS_TABLE.put ("1080i", "560 300 800 480 560 300 18 18");
        //MODE_AXIS_TABLE.put("1080p", "560 300 800 480 560 300 18 18");
        MODE_AXIS_TABLE.put ("1080p", "160 60 1600 960 160 60 36 36");	//2x scale
    }

    //option menu
    public boolean onCreateOptionsMenu (Menu menu) {
        String ver_str = null;
        try {
            ver_str = getPackageManager().getPackageInfo ("com.droidlogic.HdmiSwitch", 0).versionName;
        } catch (NameNotFoundException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        menu.add (0, 0, 0, getText (R.string.app_name) + " v" + ver_str);
        return true;
    }

    private boolean isClingsEnabled() {
        // disable clings when running in a test harness
        if (ActivityManager.isRunningInTestHarness() ) {
            return false;
        }
        return true;
    }

    private void removeCling (int id) {
        final View cling = findViewById (id);
        if (cling != null) {
            final ViewGroup parent = (ViewGroup) cling.getParent();
            parent.post (new Runnable() {
                public void run() {
                    parent.removeView (cling);
                }
            });
        }
    }

    public int showFirstRunHdmiCling (String mode) {
        // chose the key according to the mod
        String key;
        if (mode.equals ("first") ) {
            key = HdmiCling.CLING_DISMISS_FIRST;
        } else {
            if (!isHdmiPlugged() ) {
                return -1;
            }
            if (mode.equals ("480p") ) {
                key = HdmiCling.CLING_DISMISS_KEY_480P;
            } else if (mode.equals ("720p") ) {
                key = HdmiCling.CLING_DISMISS_KEY_720P;
            } else if (mode.equals ("1080i") ) {
                key = HdmiCling.CLING_DISMISS_KEY_1080I;
            } else if (mode.equals ("1080p") ) {
                key = HdmiCling.CLING_DISMISS_KEY_1080P;
            } else {
                return -1;
            }
        }

        if (mode.equals (getCurMode() ) ) {
            return -1;
        }

        return 1;
    }


    private void setDualDisplay (boolean hdmiPlugged) {
        mHdmiOutputManager.setDualDisplay (hdmiPlugged);
    }

    private int setMode (String modeStr) {
        return mHdmiOutputManager.setMode (modeStr);
    }

    private int setFbBlank (String fb, String blankStr) {
        return mHdmiOutputManager.setFbBlank (fb, blankStr);
    }

    private boolean isHdmiPlugged() {
        return mHdmiOutputManager.isHdmiPlugged();
    }

    private List<String> getAllMode() {
        return mHdmiOutputManager.getAllMode();
    }

    private String getCurMode() {
        return mHdmiOutputManager.getCurMode();
    }

    /** getListData */
    private List<Map<String, Object>> getListData() {
        List<Map<String, Object>> list = new ArrayList<Map<String, Object>>();

for (String modeStr : getAllMode() ) {
            Map<String, Object> map = new HashMap<String, Object>();
            map.put ("mode", modeStr);
            map.put ("item_text", getText ( (Integer) MODE_STR_TABLE.get (modeStr) ) );
            if (modeStr.equals (getCurMode() ) ) {
                map.put ("item_img", R.drawable.item_img_sel);
            } else {
                map.put ("item_img", R.drawable.item_img_unsel);
            }
            list.add (map);
        }

        return list;
    }

}
