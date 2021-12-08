/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 */

package com.droidlogic.HdmiSwitch;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.Handler;
import android.os.Message;
import android.os.SystemClock;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Display;
import android.view.Menu;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.view.Window;
import android.view.WindowManager;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TextView;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;

import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.SystemControlManager.DisplayInfo;
import com.droidlogic.app.OutputModeManager;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.UserHandle ;

public class HdmiOutputModeManager {
    private static final String TAG = "HdmiOutputModeManager";
    private static final boolean DEBUG = true;

    private static Context mContext = null;

    public final static String SYSFS_DISPLAY_MODE = "/sys/class/display/mode";
    public final static String SYSFS_DISPLAY_AXIS = "/sys/class/display/axis";
    public static final String SYSFS_DISP_CAP = "/sys/class/amhdmitx/amhdmitx0/disp_cap";

    private final static String SYSFS_DISPLAY2_MODE = "/sys/class/display2/mode";
    private static final String SYSFS_DISPLAY2_AXIS = "/sys/class/display2/axis";
    private static final String SYSFS_VIDEO_DEV = "/sys/module/amvideo/parameters/cur_dev_idx";
    private static final String SYSFS_VIDEO_AXIS = "/sys/class/video/axis";
    private static final String SYSFS_DISABLE_VIDEO = "/sys/class/video/disable_video";
    private static final String SYSFS_VIDEO2_CLONE = "/sys/class/video2/clone";

    private static final String SYSFS_VFM_MAP = "/sys/class/vfm/map";
    private static final String SYSFS_VIDEO2_FRAME_WIDTH = "/sys/module/amvideo2/parameters/clone_frame_scale_width";
    private static final String SYSFS_VIDEO2_SCREEN_MODE = "/sys/class/video2/screen_mode";
    private static final String SYSFS_VIDEO2_ZOOM = "/sys/class/video2/zoom";

    private static final String SYSFS_PPMGR_DISP = "sys/class/ppmgr/disp";
    private static final String SYSFS_PPMGR_ANGLE = "sys/class/ppmgr/angle";
    private static final String SYSFS_PPMGR_PPSCALER = "/sys/class/ppmgr/ppscaler";
    private static final String SYSFS_PPMGR_PPSCALER_RECT = "/sys/class/ppmgr/ppscaler_rect";


    private static final String SYSFS_HDMITX0_DISP_MODE = "/sys/class/amhdmitx/amhdmitx0/disp_mode";
    private static final String SYSFS_BRIGHTNESS = "/sys/class/backlight/aml-bl/brightness";

    private static final String SYSFS_FB0_BLANK = "/sys/class/graphics/fb0/blank";
    private static final String SYSFS_FB0_FREESCALE = "/sys/class/graphics/fb0/free_scale";
    private static final String SYSFS_FB0_FREESCALE_MODE = "/sys/class/graphics/fb0/freescale_mode";
    private static final String SYSFS_FB0_FREESCALE_AXIS = "/sys/class/graphics/fb0/free_scale_axis";
    private static final String SYSFS_FB0_PROT_ON = "/sys/class/graphics/fb0/prot_on";
    private static final String SYSFS_FB0_PROT_ANGLE = "/sys/class/graphics/fb0/prot_angle";
    private static final String SYSFS_FB0_PROT_CANVAS = "/sys/class/graphics/fb0/prot_canvas";
    private static final String SYSFS_FB0_REQUEST2XSCALE = "/sys/class/graphics/fb0/request2XScale";
    private static final String SYSFS_FB0_WINDOW_AXIS = "/sys/class/graphics/fb0/window_axis";
    private static final String SYSFS_FB0_BLOCK_MODE = "/sys/class/graphics/fb0/block_mode";

    private static final String SYSFS_FB1_BLANK = "/sys/class/graphics/fb1/blank";
    private static final String SYSFS_FB1_FREESCALE = "/sys/class/graphics/fb1/free_scale";
    private static final String SYSFS_FB1_VER_CLONE = "/sys/class/graphics/fb1/ver_clone";
    private static final String SYSFS_FB1_VER_ANGLE = "/sys/class/graphics/fb1/ver_angle";
    private static final String SYSFS_FB1_VER_UPDATE_PAN = "/sys/class/graphics/fb1/ver_update_pan";
    private static final String SYSFS_FB1_FREESCALE_AXIS = "/sys/class/graphics/fb1/free_scale_axis";
    private static final String SYSFS_FB1_WINDOW_AXIS = "/sys/class/graphics/fb1/window_axis";

    private static final String SYSFS_FB2_BLANK = "/sys/class/graphics/fb2/blank";
    private static final String SYSFS_FB2_FREESCALE = "/sys/class/graphics/fb2/free_scale";
    private static final String SYSFS_FB2_FREESCALE_AXIS = "/sys/class/graphics/fb2/free_scale_axis";
    private static final String SYSFS_FB2_CLONE = "/sys/class/graphics/fb2/clone";
    private static final String SYSFS_FB2_ANGLE = "/sys/class/graphics/fb2/angle";
    private static final String SYSFS_FB2_VENC = "/sys/class/display2/venc_mux";

    private static final String SYSFS_FB_BLANK = "/sys/class/graphics/%s/blank";
    private static final String SYSFS_FB_FREESCALE = "/sys/class/graphics/%s/free_scale";
    private static final String SYSFS_FB_FREESCALE_AXIS = "/sys/class/graphics/%s/free_scale_axis";

    private static final String HDMI_OFF = "aaa";

    private static final String[] TABLET_COMMON_MODE_VALUE_LIST = {
        "panel",
        "480i", "480p",
        "720p", "720p50hz",
        "1080i", "1080p", "1080i50hz", "1080p50hz", "1080p24hz"
    };

    private static SystemControlManager mSystemControl = null;
    private static OutputModeManager mOutputModeManager = null;
    private static DisplayInfo mDisplayInfo = null;

    private static boolean mModeSetting = false;
    final Object mLock = new Object[0];

    private static boolean mSingleOutput = false;
    private static boolean mPortraitScreen = false;
    private static boolean mDualDisplay = false;
    private static boolean mDualDisplay2 = false;
    private static boolean mDualDisplay3 = false;
    private static boolean mDualDisplay4 = false;
    private static boolean mNeedPlayerExit = true;
    private static boolean mHdmiAllSwitch = false;
    private static boolean mRealExternalDisplay = false;
    private static boolean mDualScaler = false;
    private static boolean mScreenOrientation = false;
    private static int mHdmiVppRotation = 0;

    public HdmiOutputModeManager (Context context) {
        mContext = context;
        mSystemControl = new SystemControlManager (mContext);
        mOutputModeManager = new OutputModeManager (mContext);
        mDisplayInfo = mSystemControl.getDisplayInfo();

        mSingleOutput = mSystemControl.getPropertyBoolean ("ro.module.singleoutput", false);
        mPortraitScreen = mSystemControl.getPropertyBoolean ("ro.screen.portrait", false);
        mDualDisplay = mSystemControl.getPropertyBoolean ("ro.vout.dualdisplay", false);
        mDualDisplay2 = mSystemControl.getPropertyBoolean ("ro.vout.dualdisplay2", false);
        mDualDisplay3 = mSystemControl.getPropertyBoolean ("ro.vout.dualdisplay3", false);
        mDualDisplay4 = mSystemControl.getPropertyBoolean ("ro.vout.dualdisplay4", false);
        mNeedPlayerExit = mSystemControl.getPropertyBoolean ("ro.vout.player.exit", true);
        mHdmiAllSwitch = mSystemControl.getPropertyBoolean ("ro.app.hdmi.allswitch", false);
        mRealExternalDisplay = mSystemControl.getPropertyBoolean ("ro.real.externaldisplay", false);
        mDualScaler = mSystemControl.getPropertyBoolean ("ro.module.dualscaler", false);
        mScreenOrientation = mSystemControl.getPropertyBoolean ("ro.screen.orientation", false);
        mHdmiVppRotation = mSystemControl.getPropertyInt ("ro.vpp.hdmi.rotation", 0);
    }

    public void hdmiPlugged() {
        Log.d (TAG, "===== hdmiPlugged()");
        mOutputModeManager.setHdmiPlugged();
    }

    public void hdmiUnPlugged() {
        Log.d (TAG, "===== hdmiUnPlugged()");
        mOutputModeManager.setHdmiUnPlugged();
    }

    public boolean isHdmiPlugged() {
        return mOutputModeManager.isHDMIPlugged();
    }

    //TODO: donot used
    public boolean ifModeIsSetting() {
        return mModeSetting;
    }

    /**
    * Sleep for a period of time.
    * @param secs the number of seconds to sleep
    */
    private static void nap (int secs) {
        try {
            Thread.sleep (secs * 1000);
        } catch (InterruptedException ignore) {
        }
    }
    private static void napMs (int ms) {
        try {
            Thread.sleep (ms);
        } catch (InterruptedException ignore) {
        }
    }

    /** get current mode*/
    public String getCurMode() {
        String modeStr;

        if (mDualDisplay4) {
            modeStr = readSysfs (SYSFS_DISPLAY2_MODE);
            return ( (modeStr == null) || modeStr.equals ("") ) ? "null" : modeStr;
        }
        if (mDualDisplay) {
            modeStr = readSysfs (SYSFS_DISPLAY2_MODE);
            return (modeStr == null) ? "720p" : modeStr;
        }
        modeStr = readSysfs (SYSFS_DISPLAY_MODE);
        return (modeStr == null) ? "panel" : modeStr;
    }

    private String getFbSize (int fb) {
        String val = null;

        if (fb > 0) {
            val = readSysfs ("/sys/class/graphics/fb2/virtual_size");
        } else {
            val = readSysfs ("/sys/class/graphics/fb0/virtual_size");
        }

        if (val != null) {
            String widthStr = null;
            String heightStr = null;
            int width, height;
            widthStr = val.split (",") [0];
            heightStr = val.split (",") [1];
            if (widthStr != null && heightStr != null) {
                width = Integer.parseInt (widthStr);
                height = Integer.parseInt (heightStr);
                return new String ("" + width + " " + (height / 2) );
            }
        }
        return null;
    }

    private void switchFreescale (String mode, String fb) {
        boolean playerRunning = getPropertyBoolean ("vplayer.playing", false);
        boolean freescaleOff = !mNeedPlayerExit && playerRunning;
        //TODO: not complete yet
        String mOsdAxis = "";
        if (fb.equals ("fb0") ) {
            mOsdAxis = String.format ("0 0 %d %d", mDisplayInfo.fb0Width, mDisplayInfo.fb0Height);
        } else if (fb.equals ("fb1") ) {
            //TODO:1
        } else if (fb.equals ("fb2") ) {
            //TODO:2
        } else if (fb.equals ("fb0fb1") ) {
            //TODO:3
        }

        if (freescaleOff && (!mode.equals ("null") ) ) {
            disableFreescaleLocked (mode, mOsdAxis, fb);
        } else {
            enableFreescaleLocked (mode, mOsdAxis, fb);
        }
    }

    private void disableFreescaleLocked ( final String mode, String osd_str, String fb ) {
        File file = null;

        file = new File (SYSFS_PPMGR_PPSCALER);
        if (file.exists() ) {
            writeSysfs (SYSFS_PPMGR_PPSCALER, "0");
        }
        writeSysfs (String.format (SYSFS_FB_FREESCALE, fb), "0x0");
        //TODO: not complete
        /*
        480
        sprintf(daxis_str, "0 0 %d %d 0 0 18 18", vinfo.xres, vinfo.yres);

        720p
        sprintf(daxis_str, "%d %d %d %d %d %d 18 18", 1280>vinfo.xres ? (1280-vinfo.xres)/2 : 0,
        720>vinfo.yres ? (720-vinfo.yres)/2 : 0,
        vinfo.xres,
        vinfo.yres,
        1280>vinfo.xres ? (1280-vinfo.xres)/2 : 0,
        720>vinfo.yres ? (720-vinfo.yres)/2 : 0);

        1080p
        sprintf(daxis_str, "%d %d %d %d %d %d 18 18", 1920>vinfo.xres ? (1920-vinfo.xres)/2 : 0,
        1080>vinfo.yres ? (1080-vinfo.yres)/2 : 0,
        vinfo.xres,
        vinfo.yres,
        1920>vinfo.xres ? (1920-vinfo.xres)/2 : 0,
        1080>vinfo.yres ? (1080-vinfo.yres)/2 : 0);
        */

        if (fb.equals ("fb2") ) {
            writeSysfs (SYSFS_DISPLAY2_AXIS, osd_str + " 0 0 18 18");
        } else {
            writeSysfs (SYSFS_DISPLAY_AXIS, osd_str + " 0 0 18 18");
        }

        file = new File (SYSFS_PPMGR_PPSCALER_RECT);
        if (file.exists() ) {
            writeSysfs (SYSFS_PPMGR_PPSCALER_RECT, "0 0 0 0 1");
        }
        file = new File (SYSFS_VIDEO_AXIS);
        if (file.exists() ) {
            writeSysfs (SYSFS_VIDEO_AXIS, "0 0 0 0");
        }

    }

    private void enableFreescaleLocked ( final String mode, String osd_str, String fb) {
        File file = null;

        if (mode.equals ("null") ) {
            file = new File (SYSFS_PPMGR_PPSCALER);
            if (file.exists() ) {
                writeSysfs (SYSFS_PPMGR_PPSCALER, "0");
            }
            //TODO: not complete
            if (fb.equals ("fb2") ) {
                writeSysfs (SYSFS_DISPLAY2_AXIS, osd_str + " 0 0 18 18");
            } else {
                writeSysfs (SYSFS_DISPLAY_AXIS, osd_str + " 0 0 18 18");
            }
            writeSysfs (String.format (SYSFS_FB_FREESCALE, fb), "0x0");
            writeSysfs (String.format (SYSFS_FB_FREESCALE, fb), "0x1");
            writeSysfs (String.format (SYSFS_FB_FREESCALE, fb), "0x0");
            file = new File (SYSFS_VIDEO_AXIS);
            if (file.exists() ) {
                writeSysfs (SYSFS_VIDEO_AXIS, "0 0 0 0");
            }
        } else {
            String mWinAxis = getDefWindowAxis (mode);

            file = new File (SYSFS_PPMGR_PPSCALER);
            if (file.exists() ) {
                writeSysfs (SYSFS_PPMGR_PPSCALER, "0");
            }
            file = new File (SYSFS_DISABLE_VIDEO);
            if (file.exists() ) {
                writeSysfs (SYSFS_DISABLE_VIDEO, "1");
            }
            file = new File (SYSFS_PPMGR_PPSCALER);
            if (file.exists() ) {
                writeSysfs (SYSFS_PPMGR_PPSCALER, "1");
            }
            file = new File (SYSFS_PPMGR_PPSCALER_RECT);
            if (file.exists() ) {
                writeSysfs (SYSFS_PPMGR_PPSCALER_RECT, mWinAxis + " 0");
            } else {
                file = new File (SYSFS_VIDEO_AXIS);
                if (file.exists() ) {
                    writeSysfs (SYSFS_VIDEO_AXIS, mWinAxis);
                }
            }

            //TODO: not complete
            if (fb.equals ("fb2") ) {
                writeSysfs (SYSFS_DISPLAY2_AXIS, osd_str + " 0 0 18 18");
            } else if (fb.equals ("fb0fb1") ) {
                writeSysfs (SYSFS_DISPLAY_AXIS, osd_str + " 0 0 18 18");
                writeSysfs (SYSFS_FB0_FREESCALE, "0x0");
                writeSysfs (SYSFS_FB1_FREESCALE, "0x0");
                //TODO: not complete
                writeSysfs (SYSFS_FB0_FREESCALE_AXIS, osd_str);
                writeSysfs (SYSFS_FB1_FREESCALE_AXIS, osd_str);
                writeSysfs (SYSFS_FB0_FREESCALE, "0x1");
                writeSysfs (SYSFS_FB1_FREESCALE, "0x1");
                file = new File (SYSFS_PPMGR_PPSCALER);
                if (file.exists() ) {
                    file = new File (SYSFS_DISABLE_VIDEO);
                    if (file.exists() ) {
                        writeSysfs (SYSFS_DISABLE_VIDEO, "1");
                    }
                }
                return;
            } else {
                writeSysfs (SYSFS_DISPLAY_AXIS, osd_str + " 0 0 18 18");
            }
            writeSysfs (String.format (SYSFS_FB_FREESCALE, fb), "0x0");
            //TODO: not complete
            writeSysfs (String.format (SYSFS_FB_FREESCALE_AXIS, fb), osd_str);
            writeSysfs (String.format (SYSFS_FB_FREESCALE, fb), "0x1");
            /*++Do we really need this?
            file = new File(SYSFS_PPMGR_PPSCALER);
            if (file.exists()) {
                //TODO: not complete
                writeSysfs(SYSFS_VIDEO_AXIS, );
            }
            --Do we really need this?*/
        }
    }

    public void setDisplay2Mode (String mode) {
        Log.d (TAG, "setDisplay2Mode---------------------" + mode);
        boolean verPanel = getPropertyBoolean ("ro.vout.dualdisplay4.ver-panel", false);
        boolean verPanelReverse = getPropertyBoolean ("ro.ver-panel.reverse", false);

        if (!mode.equals ("null") ) {
            writeSysfs (SYSFS_VFM_MAP, "add dual_display osd_ext amvideo4osd");

            writeSysfs (SYSFS_FB2_BLANK, "1");
            writeSysfs (SYSFS_DISABLE_VIDEO, "1");

            writeSysfs (SYSFS_PPMGR_DISP, getFbSize (1) );
            if (verPanel) {
                writeSysfs (SYSFS_PPMGR_ANGLE, "0");
            }

            writeSysfs (SYSFS_FB2_CLONE, "0");
            writeSysfs (SYSFS_DISPLAY2_MODE, mode);
            switchFreescale (mode, "fb2");
            if (verPanel) {
                if (verPanelReverse) {
                    writeSysfs (SYSFS_FB2_ANGLE, "1");
                } else {
                    writeSysfs (SYSFS_FB2_ANGLE, "3");
                }
            } else {
                writeSysfs (SYSFS_FB2_ANGLE, "4");
            }
            writeSysfs (SYSFS_FB2_VENC, "0x8");
            writeSysfs (SYSFS_FB2_CLONE, "1");

            writeSysfs (SYSFS_VIDEO_DEV, "1");
            napMs (500);
            writeSysfs (SYSFS_DISABLE_VIDEO, "2");

            writeSysfs (SYSFS_FB2_BLANK, "0");

        } else {
            writeSysfs (SYSFS_FB2_BLANK, "1");
            writeSysfs (SYSFS_DISABLE_VIDEO, "1");

            writeSysfs (SYSFS_PPMGR_DISP, getFbSize (0) );
            if (verPanel) {
                if (verPanelReverse) {
                    writeSysfs (SYSFS_PPMGR_ANGLE, "3");
                } else {
                    writeSysfs (SYSFS_PPMGR_ANGLE, "1");
                }
            }

            writeSysfs (SYSFS_FB2_CLONE, "0");
            writeSysfs (SYSFS_DISPLAY2_MODE, "null");
            writeSysfs (SYSFS_FB2_VENC, "0x0");
            switchFreescale (mode, "fb2");

            writeSysfs (SYSFS_VIDEO_DEV, "0");
            napMs (300);
            writeSysfs (SYSFS_DISABLE_VIDEO, "2");
        }
    }

    /** disable Hdmi*/
    public int disableHdmi() {
        //Log.i(TAG, "--disableHdmi");
        writeSysfs (SYSFS_HDMITX0_DISP_MODE, HDMI_OFF + "\r\n");
        return 0;
    }

    /** get brightness*/
    public String getBrightness() {
        String briStr = "128";

        //Log.i(TAG, "--getBrightness");
        File file = new File (SYSFS_BRIGHTNESS);
        if (!file.exists() ) {
            return briStr;
        }

        briStr = readSysfs (SYSFS_BRIGHTNESS);
        return (briStr == null) ? "128" : briStr;
    }

    /** set osd blank*/
    public int setFbBlank (String fb, String blankStr) {
        String fbStr = String.format (SYSFS_FB_BLANK, fb);
        //Log.i(TAG, "----setFbBlank: " + blankStr);

        File file = new File (fbStr);
        if ( (!file.exists() ) || (writeSysfs (fbStr, blankStr) ) ) {
            return 0;
        }

        return 1;
    }

    /** set brightness*/
    public int setBrightness (String briStr) {

        //Log.i(TAG, "---setBrightness: " + briStr);
        File file = new File (SYSFS_BRIGHTNESS);
        if (!file.exists() ) {
            return 0;
        }

        writeSysfs (SYSFS_BRIGHTNESS, briStr);
        return 0;
    }

    /** video layer control */
    private int disableVideo (boolean disable) {

        //Log.i(TAG, "---disableVideo: " + disable);
        File file = new File (SYSFS_DISABLE_VIDEO);
        if (!file.exists() ) {
            return 0;
        }

        if (disable) {
            writeSysfs (SYSFS_DISABLE_VIDEO, "1");
        } else {
            writeSysfs (SYSFS_DISABLE_VIDEO, "2");
        }
        return 0;
    }

    private void shadowScreen (final String mode) {
        writeSysfs (SYSFS_FB0_BLANK, "1");
        Thread task = new Thread (new Runnable() {
            @Override
            public void run() {
                try {
                    mModeSetting = true;
                    Thread.sleep (1000);
                    writeSysfs (SYSFS_FB0_BLANK, "0");
                    mModeSetting = false;
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        });
        task.start();
    }

    public void setOutputMode ( final String mode ) {
        setOutputModeNowLocked (mode);
    }

    public void setOutputWithoutFreeScale ( final String mode ) {
        setOutputWithoutFreeScaleLocked (mode);
    }

    private void setOutputModeNowLocked (final String mode) {
        synchronized (mLock) {
            String curMode = readSysfs (SYSFS_DISPLAY_MODE);
            String newMode = mode;

            if (curMode == null || curMode.length() < 4) {
                if (DEBUG) {
                    Log.d (TAG, "===== something wrong !!!" );
                }
                curMode =  "panel";
            }
            if (DEBUG) {
                Log.d (TAG, "===== change mode form *" + curMode + "* to *" + newMode + "* ");
            }
            if (newMode.equals (curMode) ) {
                if (DEBUG) {
                    Log.d (TAG, "===== The same mode as current , do nothing !");
                }
                return ;
            }

            if (! (mSingleOutput && mPortraitScreen) ) {
                shadowScreen (curMode);
            }
            //TODO:++
            String mOsdAxis = getDefFreeScaleAxis (false);
            String mOsdPortraitAxis = getDefFreeScaleAxis (true);
            String mWinAxis = getDefWindowAxis (newMode);
            Log.d (TAG, "osd axis is " + mOsdAxis);
            Log.d (TAG, "osd portrait axis is " + mOsdPortraitAxis);
            Log.d (TAG, "window axis is " + mWinAxis);

            if ( mSingleOutput ) { //single display
                if ( mPortraitScreen ) { //portrait
                    Log.d (TAG, "hi,this is single portrait display!");
                    switchSinglePortraitDisplay (newMode, mOsdAxis, mWinAxis);
                } else { //landscape
                    Log.d (TAG, "hi,this is single landscape display!");
                    switchSingleLandscapeDisplay (newMode, mOsdAxis, mWinAxis);
                }
            } else { //dual displays
                if ( mPortraitScreen ) { //portrait
                    Log.d (TAG, "hi,this is dual portrait display!");
                    switchDualPortraitDisplay (newMode, mOsdAxis, mOsdPortraitAxis, mWinAxis);
                } else { //landscape
                    Log.d (TAG, "hi,this is dual landscape display!");
                    switchDualLandscapeDisplay (newMode, mOsdAxis, mWinAxis);
                }
            }
        }
        return;
    }

    private void setOutputWithoutFreeScaleLocked (final String mode) {
        synchronized (mLock) {
            String curMode = readSysfs (SYSFS_DISPLAY_MODE);
            String newMode = mode;

            if (curMode == null || curMode.length() < 4) {
                if (DEBUG) {
                    Log.d (TAG, "===== something wrong !!!" );
                }
                curMode =  "panel";
            }
            if (DEBUG) {
                Log.d (TAG, "===== change mode form *" + curMode + "* to *" + newMode + "* ");
            }
            if (newMode.equals (curMode) ) {
                if (DEBUG) {
                    Log.d (TAG, "===== The same mode as current , do nothing !");
                }
                return ;
            }

            shadowScreen (curMode);
            //TODO:++
            String mOsdAxis = getDefFreeScaleAxis (false);

            if (mode.contains ("480") ) { //x2panel
                writeSysfs (SYSFS_PPMGR_PPSCALER, "0");
                writeSysfs (SYSFS_FB0_FREESCALE, "0x0");
                writeSysfs (SYSFS_FB1_FREESCALE, "0x0");
                writeSysfs (SYSFS_DISPLAY_AXIS, mOsdAxis + " 0 0 18 18");
                writeSysfs (SYSFS_PPMGR_PPSCALER_RECT, "0 0 0 0 1");
                writeSysfs (SYSFS_VIDEO_AXIS, "0 0 0 0");
            } else if (mode.contains ("720") ) {
                Log.d (TAG, "----disableFreescale 720p-----");
                writeSysfs (SYSFS_PPMGR_PPSCALER, "0");
                writeSysfs (SYSFS_FB0_FREESCALE, "0x0");
                writeSysfs (SYSFS_FB1_FREESCALE, "0x0");
                //TODO: osd_str is not right
                writeSysfs (SYSFS_DISPLAY_AXIS, mOsdAxis + " 0 0 18 18");
                /*sprintf(daxis_str, "%d %d %d %d %d %d 18 18", 1280>vinfo.xres ? (1280-vinfo.xres)/2 : 0,
                720>vinfo.yres ? (720-vinfo.yres)/2 : 0,
                vinfo.xres,
                vinfo.yres,
                1280>vinfo.xres ? (1280-vinfo.xres)/2 : 0,
                720>vinfo.yres ? (720-vinfo.yres)/2 : 0);
                write(fd_daxis, daxis_str, strlen(daxis_str));*/
                writeSysfs (SYSFS_PPMGR_PPSCALER_RECT, "0 0 0 0 1");
                writeSysfs (SYSFS_VIDEO_AXIS, "0 0 0 0");
            } else if (mode.contains ("1080") ) {
                writeSysfs (SYSFS_PPMGR_PPSCALER, "0");
                writeSysfs (SYSFS_FB0_FREESCALE, "0x0");
                writeSysfs (SYSFS_FB1_FREESCALE, "0x0");
                //TODO:osd_str is not right
                writeSysfs (SYSFS_DISPLAY_AXIS, mOsdAxis + " 0 0 18 18");
                /*sprintf(daxis_str, "%d %d %d %d %d %d 18 18", 1920>vinfo.xres ? (1920-vinfo.xres)/2 : 0,
                1080>vinfo.yres ? (1080-vinfo.yres)/2 : 0,
                vinfo.xres,
                vinfo.yres,
                1920>vinfo.xres ? (1920-vinfo.xres)/2 : 0,
                1080>vinfo.yres ? (1080-vinfo.yres)/2 : 0);
                write(fd_daxis, daxis_str, strlen(daxis_str));*/
                writeSysfs (SYSFS_PPMGR_PPSCALER_RECT, "0 0 0 0 1");
                writeSysfs (SYSFS_VIDEO_AXIS, "0 0 0 0");
            }
        }
        return;
    }

    public static boolean isSinglePortraitDisplay() {
        return (mSingleOutput && mPortraitScreen);
    }

    private void switchSingleLandscapeDisplay ( final String mode, String osd_str,
            String win_str ) {
        if (mode.contains ("panel") ) { //x2panel
            writeSysfs (SYSFS_FB0_FREESCALE_MODE, "1");
            writeSysfs (SYSFS_FB0_FREESCALE, "0x0");
            writeSysfs (SYSFS_DISPLAY_MODE, mode);
        } else { //x2hdmi
            writeSysfs (SYSFS_FB0_FREESCALE_MODE, "1");
            writeSysfs (SYSFS_DISPLAY_MODE, mode);
            writeSysfs (SYSFS_FB0_FREESCALE_AXIS, osd_str);
            writeSysfs (SYSFS_FB0_WINDOW_AXIS, win_str);
            writeSysfs (SYSFS_VIDEO_AXIS, win_str);
            writeSysfs (SYSFS_FB0_FREESCALE, "0x10001");
        }
        writeSysfs (SYSFS_FB0_BLANK, "0");
    }

    private void switchSinglePortraitDisplay ( final String mode, String osd_str,
            String win_str ) {

        if (mode.contains ("panel") ) { // x2panel
            writeSysfs (SYSFS_FB1_BLANK, "1");
            if ( mRealExternalDisplay ) { // use external display implement
                setProperty ("sys.sf.hotplug", "false");
                writeSysfs (SYSFS_FB1_FREESCALE, "0x0");
            } else {
                writeSysfs (SYSFS_FB1_FREESCALE, "0x0");
                writeSysfs (SYSFS_FB1_VER_CLONE, "0");
            }
            writeSysfs (SYSFS_DISPLAY_MODE, mode);
            writeSysfs (SYSFS_FB0_BLANK, "0");
            return;
        } else { //x2hdmi
            writeSysfs (SYSFS_FB0_BLANK, "1");
            if ( mRealExternalDisplay ) {
                setProperty ("sys.sf.hotplug", "true");
                writeSysfs (SYSFS_FB1_FREESCALE, "0x0");
            } else {
                writeSysfs (SYSFS_FB1_FREESCALE, "0x0");
                writeSysfs (SYSFS_FB1_VER_CLONE, "0");
                writeSysfs (SYSFS_FB1_VER_ANGLE, "1");
                writeSysfs (SYSFS_FB1_VER_CLONE, "1");
            }
            writeSysfs (SYSFS_DISPLAY_MODE, mode);
            writeSysfs (SYSFS_FB1_FREESCALE_AXIS, osd_str);
            writeSysfs (SYSFS_FB1_WINDOW_AXIS, win_str);
            writeSysfs (SYSFS_VIDEO_AXIS, win_str);
            writeSysfs (SYSFS_FB1_FREESCALE, "0x10001");
            if ( !mRealExternalDisplay ) {
                writeSysfs (SYSFS_FB1_VER_UPDATE_PAN, "1");
            }
            writeSysfs (SYSFS_FB1_BLANK, "0");
        }

    }

    private void switchDualLandscapeDisplay ( final String mode, String osd_str,
            String win_str ) {

        if (mode.contains ("panel") ) { // x2panel
            if ( mRealExternalDisplay ) { //real external display
                setProperty ("sys.sf.hotplug", "false");
                writeSysfs (SYSFS_DISPLAY_MODE, mode);
                writeSysfs (SYSFS_DISPLAY2_MODE, "null");
                writeSysfs (SYSFS_FB2_VENC, "0x0");
                writeSysfs (SYSFS_FB0_FREESCALE, "0x0");
            } else {
                File file = new File (SYSFS_PPMGR_PPSCALER);
                if (file.exists() ) {
                    writeSysfs (SYSFS_PPMGR_PPSCALER, "0");
                }
                writeSysfs (SYSFS_DISPLAY_AXIS, osd_str + " 0 0 18 18");
                writeSysfs (SYSFS_DISPLAY_MODE, mode);
                writeSysfs (SYSFS_DISPLAY2_MODE, "null");
                writeSysfs (SYSFS_DISABLE_VIDEO, "1");
                writeSysfs (SYSFS_FB0_FREESCALE, "0");
                writeSysfs (SYSFS_FB0_BLANK, "0");
                writeSysfs (SYSFS_FB2_BLANK, "0");
                writeSysfs (SYSFS_DISABLE_VIDEO, "0");

                /*++we really need this?
                file = new File(SYSFS_PPMGR_PPSCALER);
                if (file.exists()) {
                    //TODO: not complete
                    writeSysfs(SYSFS_VIDEO_AXIS, );
                }
                --we really need this?*/
                return;
            }
        } else { //x2hdmi
            if (mRealExternalDisplay) {
                setProperty ("sys.sf.hotplug", "true");
            }
            writeSysfs (SYSFS_DISPLAY_MODE, mode);
            writeSysfs (SYSFS_DISPLAY2_MODE, "null");
            writeSysfs (SYSFS_DISPLAY2_MODE, "panel");
            //if (mRealExternalDisplay){
            writeSysfs (SYSFS_FB2_VENC, "0x2");
            //}
            writeSysfs (SYSFS_FB0_FREESCALE_MODE, "0x1");
            writeSysfs (SYSFS_FB0_FREESCALE_AXIS, osd_str);
            writeSysfs (SYSFS_FB0_WINDOW_AXIS, win_str);
            writeSysfs (SYSFS_VIDEO_AXIS, win_str);
            writeSysfs (SYSFS_FB0_FREESCALE, "0x10001");
        }
        writeSysfs (SYSFS_FB0_BLANK, "0");
        writeSysfs (SYSFS_FB2_BLANK, "0");
    }

    private void switchDualPortraitDisplay ( final String mode, String osd_str,
            String osd_str_portrait, String win_str ) {

        if (mode.contains ("panel") ) {
            if ( mRealExternalDisplay ) { //real external display
                setProperty ("sys.sf.hotplug", "false");
            } else {
                writeSysfs (SYSFS_FB2_CLONE, "0");
            }
            writeSysfs (SYSFS_FB0_PROT_ON, "0");
            writeSysfs (SYSFS_DISPLAY_MODE, mode);
            writeSysfs (SYSFS_DISPLAY2_MODE, "null");
            writeSysfs (SYSFS_FB2_VENC, "0x0");
            writeSysfs (SYSFS_FB0_FREESCALE, "0x0");

            {
                //TODO: [Stark] we really need this sleep?
                if ( !mRealExternalDisplay ) {
                    try {
                        Thread.sleep (60);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
            }
            writeSysfs (SYSFS_FB0_BLANK, "0");
            return;
        } else {
            if ( mRealExternalDisplay ) {
                setProperty ("sys.sf.hotplug", "true");
            } else {
                writeSysfs (SYSFS_FB2_CLONE, "1");
            }
            writeSysfs (SYSFS_DISPLAY_MODE, mode);
            writeSysfs (SYSFS_DISPLAY2_MODE, "null");
            writeSysfs (SYSFS_DISPLAY2_MODE, "panel");
            writeSysfs (SYSFS_FB2_VENC, "0x2");
            writeSysfs (SYSFS_FB0_PROT_CANVAS, osd_str);
            if ( mScreenOrientation ) {
                writeSysfs (SYSFS_FB0_PROT_ANGLE, "2");
            } else {
                writeSysfs (SYSFS_FB0_PROT_ANGLE, "1");
            }
            writeSysfs (SYSFS_FB0_PROT_ON, "1");
            writeSysfs (SYSFS_FB0_FREESCALE_MODE, "0x1");
            writeSysfs (SYSFS_FB0_FREESCALE_AXIS, osd_str_portrait);
            writeSysfs (SYSFS_FB0_WINDOW_AXIS, win_str);
            writeSysfs (SYSFS_VIDEO_AXIS, win_str);
            writeSysfs (SYSFS_FB0_FREESCALE, "0x10001");
        }
        writeSysfs (SYSFS_FB0_BLANK, "0");
        writeSysfs (SYSFS_FB2_BLANK, "0");
    }

    private String getDefFreeScaleAxis (boolean portrait) {
        String osdAxis;
        final boolean isSinglePortraitDisplay = isSinglePortraitDisplay();

        if (portrait) {
            if (isSinglePortraitDisplay) {
                if (mRealExternalDisplay) {
                    osdAxis = String.format ("0 0 %d %d", mDisplayInfo.fb1Height - 1, mDisplayInfo.fb1Width - 1);
                } else {
                    osdAxis = String.format ("0 0 %d %d", 719, 1279);
                }
            } else {
                osdAxis = String.format ("0 0 %d %d", mDisplayInfo.fb0Height - 1, mDisplayInfo.fb0Width - 1);
            }
        } else {
            if (isSinglePortraitDisplay) {
                if (mRealExternalDisplay) {
                    osdAxis = String.format ("0 0 %d %d", mDisplayInfo.fb1Width - 1, mDisplayInfo.fb1Height - 1);
                } else {
                    osdAxis = String.format ("0 0 %d %d", 1279, 719);
                }
            } else {
                osdAxis = String.format ("0 0 %d %d", mDisplayInfo.fb0Width - 1, mDisplayInfo.fb0Height - 1);
            }
        }
        return osdAxis;
    }


    private String getDefWindowAxis (String mode) {
        String sCurAxis = "40 15 1240 705";
        final boolean isSinglePortraitDisplay = isSinglePortraitDisplay();
        int index = 0; // panel

        for (int i = 0; i < TABLET_COMMON_MODE_VALUE_LIST.length; i++) {
            if (mode.equalsIgnoreCase (TABLET_COMMON_MODE_VALUE_LIST[i]) ) {
                index = i;
            }
        }

        switch (index) {
        case 0: //panel
            break;
        case 1:// 480i
        case 2:// 480p
            sCurAxis = "20 10 700 470";
            break;
        case 3: // 720p
        case 4: // 720p50hz
            if (isSinglePortraitDisplay) {
                sCurAxis = "40 15 1239 704";
            } else {
                sCurAxis = "40 15 1240 705";
            }
            break;
        case 5: // 1080i
        case 6: // 1080p
        case 7: // 1080i50hz
        case 8: // 1080p50hz
        case 9: //1080p24hz
            if (isSinglePortraitDisplay) {
                sCurAxis = "40 15 1879 1064";
            } else {
                sCurAxis = "40 20 1880 1060";
            }
            break;
        default: // 720p
            sCurAxis =  "40 15 1240 705";
            break;
        }

        return sCurAxis;
    }

    /** set mode */
    public int setMode (String modeStr) {
        Log.i (TAG, "Set mode 2 " + modeStr);
        if ( mDualDisplay4 ) {
            if (modeStr.contains ("panel") ) {
                modeStr = "null";
            }
            if (!modeStr.contains ("null") ) {
                if (!isHdmiPlugged() ) {
                    return 0;
                }
            }
            if (modeStr.contains (getCurMode() ) ) {
                return 0;
            }
            setDisplay2Mode (modeStr);
            return 0;
        }
        if (modeStr.contains (getCurMode() ) ) {
            return 0;
        }
        if (!modeStr.contains ("panel") && !isHdmiPlugged() ) {
            return 0;
        }
        if (modeStr.contains ("panel") && ! (isSinglePortraitDisplay() && mRealExternalDisplay) ) {
            //if mode is reset to 'panel',we need to reset window_axis to zero avoid libplayer changing video/axis
            writeSysfs (SYSFS_FB0_WINDOW_AXIS, "0 0 0 0");
        }

        if ( mDualDisplay ) {
            writeSysfs (SYSFS_DISPLAY2_MODE, modeStr + "\r\n");
            return 0;
        }

        String briStr = "128";
        if (modeStr.contains ("panel") ) {
            disableHdmi();
            briStr = getBrightness();
            setBrightness ("0");
            disableVideo (true);
        }

        boolean playerRunning = getPropertyBoolean ("vplayer.playing", false);
        boolean freescaleOff = !mNeedPlayerExit && playerRunning;
        //for android 4.3,we use new freescale mode ,and there is no need to disabele freescale when videoplay is running
        if ( mDualScaler ) {
            freescaleOff = false;
        }

        //do free_scale
        if (modeStr.contains ("panel") ) {
            setOutputMode (modeStr);
            disableVideo (false);
            setBrightness (briStr);
            if (! (isSinglePortraitDisplay() && mRealExternalDisplay) ) {
                writeSysfs (SYSFS_FB0_REQUEST2XSCALE, "2");
            }
        } else {
            if (freescaleOff) {
                setOutputWithoutFreeScale (modeStr);
            } else {
                setOutputMode (modeStr);
            }
        }

        return 0;
    }

    /** get all support mode*/
    public List<String> getAllMode() {
        List<String> list = new ArrayList<String>();
        String modeStr;

        WindowManager mWm = (WindowManager) mContext.getSystemService (Context.WINDOW_SERVICE);
        Display display = mWm.getDefaultDisplay();
        int mWScreenx = display.getWidth();
        int mWScreeny = display.getHeight();
        boolean skip480p = false;
        if ( ( (mWScreenx > 1920) && (mWScreeny > 1080) )
                || ( ( mWScreeny > 1920) && (mWScreenx > 1080) ) ) {
            skip480p = true;
        }

        if ( mDualDisplay ) {
            if (!skip480p) {
                list.add ("480p");
            }
            list.add ("720p50hz");
            list.add ("720p");
            list.add ("1080p24hz");
            list.add ("1080p50hz");
            list.add ("1080p");
            return list;
        }

        if ( mDualDisplay4 ) {
            list.add ("null");
        } else {
            list.add ("panel");
        }

        //list.add("480i");
        if (getPropertyBoolean ("ro.hdmi480p.enable", true) ) {
            if (!skip480p) {
                list.add ("480p");
            }
        }
        if (getPropertyBoolean ("ro.hdmi720p50Hz.enable", false) ) {
            list.add ("720p50hz");
        }
        if (getPropertyBoolean ("ro.hdmi720p60Hz.enable", true) ) {
            list.add ("720p");
        }
        if (getPropertyBoolean ("ro.hdmi1080p24Hz.enable", false) ) {
            list.add ("1080p24hz");
        }
        if (getPropertyBoolean ("ro.hdmi1080p50Hz.enable", false) ) {
            list.add ("1080p50hz");
        }
        if (getPropertyBoolean ("ro.hdmi1080p60Hz.enable", true) ) {
            list.add ("1080p");
        }

        return list;
    }

    public int onVideoPlayerCrashed() {
        setProperty ("vplayer.playing", "false");
        setProperty ("vplayer.hideStatusBar.enable", "false");
        writeSysfs (SYSFS_FB0_BLANK, "0");
        writeSysfs (SYSFS_FB0_BLOCK_MODE, "0");

        if (!isHdmiPlugged() ) {
            return 0;
        }

        if (mDualDisplay4) {
            if (!getCurMode().equals ("null") ) {
                switchFreescale (getCurMode(), "fb2");
            }
            return 0;
        }

        if (!getCurMode().equals ("panel") ) {
            switchFreescale (getCurMode(), "fb0fb1");
        }

        return 0;
    }

    public void setVout2OffStatic() {
        if (mDualDisplay2) {
            setFbBlank ("fb0", "1");
        } else if (mDualDisplay3) {
            setFbBlank ("fb2", "1");
        }
    }

    /** fastSwitch func for amlplayer*/
    public int fastSwitch() {
        /* check driver interface */
        File file = new File (SYSFS_DISP_CAP);
        if (!file.exists() ) {
            return 0;
        }
        file = new File (SYSFS_DISPLAY_MODE);
        if (!file.exists() ) {
            return 0;
        }
        file = new File (SYSFS_DISPLAY_AXIS);
        if (!file.exists() ) {
            return 0;
        }

        /* panel <-> TV*/
        if (getCurMode().equals ("panel") ) {
            setMode ("720p");
            Log.v (TAG, "fast switch to 720p");
            return 1;
        } else {
            setMode ("panel");
            Log.v (TAG, "fast switch to panel");
            return 1;
        }
    }

    public void setDualDisplay (boolean hdmiPlugged) {

        //we canot deal with camera when hdmi enable
        if (isCameraBusy() ) {
            Log.w (TAG, "setDualDisplay, camera is busy");
            return;
        }

        if ( mDualDisplay2 ) {
            if (hdmiPlugged) {
                writeSysfs (SYSFS_VIDEO2_CLONE, "0");
                writeSysfs (SYSFS_VFM_MAP, "rm default_ext");
                if (mHdmiVppRotation > 0) {
                    writeSysfs (SYSFS_VFM_MAP, "add default_ext vdin freescale amvideo2");
                } else {
                    writeSysfs (SYSFS_VFM_MAP, "add default_ext vdin amvideo2");
                }
                writeSysfs (SYSFS_VIDEO2_CLONE, "1");

                if (getCurMode().contains ("720") ) {
                    writeSysfs (SYSFS_VIDEO2_FRAME_WIDTH, "640");
                } else if (getCurMode().contains ("1080") ) {
                    writeSysfs (SYSFS_VIDEO2_FRAME_WIDTH, "800");
                } else {
                    writeSysfs (SYSFS_VIDEO2_FRAME_WIDTH, "0");
                }
                writeSysfs (SYSFS_VIDEO2_ZOOM, "105");

                if (getDualDisplayState() == 1) {
                    writeSysfs (SYSFS_VIDEO2_SCREEN_MODE, "1");
                    writeSysfs (SYSFS_DISPLAY2_MODE, "null");
                    writeSysfs (SYSFS_DISPLAY2_MODE, "panel");
                }
            } else {
                writeSysfs (SYSFS_VIDEO2_CLONE, "0");
                writeSysfs (SYSFS_VFM_MAP, "rm default_ext");
                writeSysfs (SYSFS_VFM_MAP, "add default_ext vdin vm amvideo");
                writeSysfs (SYSFS_DISPLAY2_MODE, "null");
            }
        } else if ( mDualDisplay3 ) {
            if ( mDualScaler ) {
                if (hdmiPlugged && (getDualDisplayState() == 1) ) {
                    setFbBlank ("fb2", "1");
                    writeSysfs (SYSFS_FB2_CLONE, "1");
                    setFbBlank ("fb2", "0");
                } else {
                    setFbBlank ("fb2", "1");
                    writeSysfs (SYSFS_FB2_CLONE, "0");
                    writeSysfs (SYSFS_DISPLAY2_MODE, "null");
                }
            }
        }
    }

    public int getDualDisplayState() {
        //always be dualdisplay
        return 1;//Settings.System.getInt(getContentResolver(), Settings.System.HDMI_DUAL_DISP, 1);
    }

    public void setDualDisplayStatic (boolean hdmiPlugged, boolean dualEnabled) {

        //we canot deal with camera when hdmi enable
        if ( (isCameraBusy() ) ) {
            Log.w (TAG, "setDualDisplay, camera is busy");
            return;
        }

        if ( mDualDisplay2 ) {
            if (hdmiPlugged) {
                writeSysfs (SYSFS_VIDEO2_CLONE, "0");
                writeSysfs (SYSFS_VFM_MAP, "rm default_ext");
                if (getPropertyInt ("ro.vpp.hdmi.rotation", 0) > 0) {
                    writeSysfs (SYSFS_VFM_MAP, "add default_ext vdin freescale amvideo2");
                } else {
                    writeSysfs (SYSFS_VFM_MAP, "add default_ext vdin amvideo2");
                }
                writeSysfs (SYSFS_VIDEO2_CLONE, "1");

                if (getCurMode().contains ("720") ) {
                    writeSysfs (SYSFS_VIDEO2_FRAME_WIDTH, "640");
                } else if (getCurMode().contains ("1080") ) {
                    writeSysfs (SYSFS_VIDEO2_FRAME_WIDTH, "800");
                } else {
                    writeSysfs (SYSFS_VIDEO2_FRAME_WIDTH, "0");
                }
                writeSysfs (SYSFS_VIDEO2_ZOOM, "105");

                if (dualEnabled) {
                    writeSysfs (SYSFS_VIDEO2_SCREEN_MODE, "1");
                    writeSysfs (SYSFS_DISPLAY2_MODE, "null");
                    writeSysfs (SYSFS_DISPLAY2_MODE, "panel");
                }
            } else {
                writeSysfs (SYSFS_VIDEO2_CLONE, "0");
                writeSysfs (SYSFS_VFM_MAP, "rm default_ext");
                writeSysfs (SYSFS_VFM_MAP, "add default_ext vdin vm amvideo");
                writeSysfs (SYSFS_DISPLAY2_MODE, "null");
            }
        } else if ( mDualDisplay3 ) {
            if (hdmiPlugged && dualEnabled) {
                if ( !mPortraitScreen ) {
                    setFbBlank ("fb2", "1");
                    writeSysfs (SYSFS_FB2_CLONE, "1");
                    writeSysfs (SYSFS_DISPLAY2_MODE, "null");
                    writeSysfs (SYSFS_DISPLAY2_MODE, "panel");
                    setFbBlank ("fb2", "0");
                } else {
                    Log.v (TAG, "setDualDisplayStatic 2 connect");
                }
            } else {
                if ( !mPortraitScreen ) {
                    setFbBlank ("fb2", "1");
                    writeSysfs (SYSFS_FB2_CLONE, "0");
                    writeSysfs (SYSFS_DISPLAY2_MODE, "null");
                } else {
                    Log.v (TAG, "setDualDisplayStatic  2 disconnect");
                }
            }
        }
    }

    public void resetFreescaleStatus() {
        //when system power up, we need to reset freescale status in case the screen is crash
        if (readSysfs (SYSFS_FB0_FREESCALE).contains ("0x1") ) {
            //Log.d(TAG, "freescale has open,which means hdmi is plugging in .So don't set it");
            return;
        } else {
            writeSysfs (SYSFS_FB0_FREESCALE_MODE, "0");
            writeSysfs (SYSFS_FB0_FREESCALE, "0");
        }
    }

    public String getProperty (String key) {
        if (DEBUG) {
            Log.i (TAG, "getProperty key:" + key);
        }
        return mSystemControl.getProperty (key);
    }

    public String getPropertyString (String key, String def) {
        if (DEBUG) {
            Log.i (TAG, "getPropertyString key:" + key + " def:" + def);
        }
        return mSystemControl.getPropertyString (key, def);
    }

    public int getPropertyInt (String key, int def) {
        if (DEBUG) {
            Log.i (TAG, "getPropertyInt key:" + key + " def:" + def);
        }
        return mSystemControl.getPropertyInt (key, def);
    }

    public long getPropertyLong (String key, long def) {
        if (DEBUG) {
            Log.i (TAG, "getPropertyLong key:" + key + " def:" + def);
        }
        return mSystemControl.getPropertyLong (key, def);
    }

    public boolean getPropertyBoolean (String key, boolean def) {
        if (DEBUG) {
            Log.i (TAG, "getPropertyBoolean key:" + key + " def:" + def);
        }
        return mSystemControl.getPropertyBoolean (key, def);
    }

    public void setProperty (String key, String value) {
        if (DEBUG) {
            Log.i (TAG, "setProperty key:" + key + " value:" + value);
        }
        mSystemControl.setProperty (key, value);
    }

    public String getBootenv (String key, String value) {
        if (DEBUG) {
            Log.i (TAG, "getBootenv key:" + key + " value:" + value);
        }
        return mSystemControl.getBootenv (key, value);
    }

    public int getBootenvInt (String key, String value) {
        if (DEBUG) {
            Log.i (TAG, "getBootenvInt key:" + key + " value:" + value);
        }
        return Integer.parseInt (mSystemControl.getBootenv (key, value) );
    }

    public void setBootenv (String key, String value) {
        if (DEBUG) {
            Log.i (TAG, "setBootenv key:" + key + " value:" + value);
        }
        mSystemControl.setBootenv (key, value);
    }

    public String readSysfsTotal (String path) {
        return mSystemControl.readSysFs (path).replaceAll ("\n", "");
    }

    public String readSysfs (String path) {
        return mSystemControl.readSysFs (path).replaceAll ("\n", "");
    }

    public boolean writeSysfs (String path, String value) {
        if (DEBUG) {
            Log.i (TAG, "writeSysfs path:" + path + " value:" + value);
        }

        return mSystemControl.writeSysFs (path, value);
    }

    public static boolean getPropDualDisplay() {
        return mDualDisplay;
    }

    public static boolean getPropDualDisplay2() {
        return mDualDisplay2;
    }

    public static boolean getPropDualDisplay3() {
        return mDualDisplay3;
    }

    public static boolean getPropDualDisplay4() {
        return mDualDisplay4;
    }

    public static boolean getPropRealExternalDisplay() {
        return mRealExternalDisplay;
    }

    public boolean isCameraBusy() {
        return !getPropertyString ("camera.busy", "0").equals ("0");
    }

}

