/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.droidlogic.miracast;

import android.annotation.TargetApi;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Configuration;
import android.content.SharedPreferences;
import android.net.NetworkInfo;
import android.net.wifi.p2p.WifiP2pConfig;
import android.net.wifi.p2p.WifiP2pDevice;
import android.net.wifi.p2p.WifiP2pManager;
import android.net.wifi.p2p.WifiP2pWfdInfo;
import android.net.wifi.p2p.WifiP2pManager.ActionListener;
import android.net.wifi.p2p.WifiP2pManager.Channel;
import android.net.wifi.p2p.WifiP2pManager.ChannelListener;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.Looper;
import android.os.UserHandle;
import android.os.PowerManager;
import android.os.SystemProperties;
import android.os.FileObserver;
import android.os.FileUtils;
import android.provider.Settings;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ImageButton;
import android.widget.Toast;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.Timer;
import java.util.TimerTask;
import java.util.StringTokenizer;
import java.util.Enumeration;
import java.net.NetworkInterface;
import java.net.InetAddress;

import com.droidlogic.app.SystemControlManager;
import com.droidlogic.miracast.WiFiDirectMainActivity;
public class SinkActivity extends Activity
{
    public static final String TAG                  = "amlSink";

    public static final String KEY_IP               = "ip";
    public static final String KEY_PORT             = "port";

    private final int OSD_TIMER                     = 5000;//ms

    private final String FB0_BLANK                  = "/sys/class/graphics/fb0/blank";
    //private final String CLOSE_GRAPHIC_LAYER      = "echo 1 > /sys/class/graphics/fb0/blank";
    //private final String OPEN_GRAPHIC_LAYER       = "echo 0 > /sys/class/graphics/fb0/blank";
    //private final String WIFI_DISPLAY_CMD         = "wfd -c";
    //private static final int MAX_DELAY_MS         = 3000;
// Miracast cert begin
    private File mFolder = new File("/data/data/com.amlogic.miracast");
    private String strSessionID = null;
    private String strIP = null;
// Miracast cert end
    private final int MSG_CLOSE_OSD             = 2;
    private String mCueEnable;
    private String mBypassDynamic;
    private String mBypassProg;

    private String mIP;
    private String mPort;
    private boolean mMiracastRunning = false;
    private PowerManager.WakeLock mWakeLock;
    private MiracastThread mMiracastThread = null;
    private Handler mMiracastHandler = null;
    private boolean isHD = false;
    private SurfaceView mSurfaceView;

    private View mRootView;

    private SystemControlManager mSystemControl = new SystemControlManager (this);
    private int certBtnState = 0; // 0: none oper, 1:play, 2:pause

    static
    {
        System.loadLibrary ("wfd_jni");
    }

    private FileObserver mFileObserver = new FileObserver(mFolder.getPath(), FileObserver.MODIFY)
    {
        public void onEvent(int event, String path) {
            Log.d(TAG, "File changed : path=" + path + " event=" + event);
            if (null == path)
            {
                return;
            }

            if (path.equals(new String("sessionId")))
            {
                File ipFile = new File(mFolder, path);
                String fullName = ipFile.getPath();
                parseSessionId(fullName);
            }
        }
    };

    private final BroadcastReceiver mReceiver = new BroadcastReceiver()
    {
        @Override
        public void onReceive (Context context, Intent intent)
        {
            String action = intent.getAction();
            if (action.equals (WifiP2pManager.WIFI_P2P_CONNECTION_CHANGED_ACTION) )
            {
                NetworkInfo networkInfo = (NetworkInfo) intent
                                          .getParcelableExtra (WifiP2pManager.EXTRA_NETWORK_INFO);

                Log.d (TAG, "P2P connection changed isConnected:" + networkInfo.isConnected() );
                if (!networkInfo.isConnected() )
                {
                    finishView();
                }
            }
        }
    };

    private void finishView()
    {
        Intent homeIntent = new Intent (SinkActivity.this, WiFiDirectMainActivity.class);
        homeIntent.setFlags (Intent.FLAG_ACTIVITY_BROUGHT_TO_FRONT);
        SinkActivity.this.startActivity (homeIntent);
        SinkActivity.this.finish();
    }

    @Override
    public void onCreate (Bundle savedInstanceState)
    {
        super.onCreate (savedInstanceState);

        //no title and no status bar
        requestWindowFeature (Window.FEATURE_NO_TITLE);
        getWindow().setFlags (WindowManager.LayoutParams.FLAG_FULLSCREEN,
                              WindowManager.LayoutParams.FLAG_FULLSCREEN);

        setContentView (R.layout.sink);

        mSurfaceView = (SurfaceView) findViewById (R.id.wifiDisplaySurface);

        //full screen test
        mRootView = (View) findViewById (R.id.rootView);

        mSurfaceView.getHolder().addCallback (new SurfaceCallback() );
        mSurfaceView.getHolder().setKeepScreenOn (true);
        mSurfaceView.getHolder().setType (SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
        mSurfaceView.setVisibility(View.GONE);

        Window window=getWindow();
        WindowManager.LayoutParams wl = window.getAttributes();
        wl.flags=WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON;
        wl.alpha=0.0f;
        window.setAttributes(wl);

        Intent intent = getIntent();
        Bundle bundle = intent.getExtras();
        mPort = bundle.getString (KEY_PORT);
        mIP = bundle.getString (KEY_IP);
        isHD = bundle.getBoolean(WiFiDirectMainActivity.HRESOLUTION_DISPLAY);
        MiracastThread mMiracastThread = new MiracastThread();
        new Thread (mMiracastThread).start();
        synchronized (mMiracastThread)
        {
            while (null == mMiracastHandler)
            {
                try
                {
                    mMiracastThread.wait();
                }
                catch (InterruptedException e) {}
            }
        }
        mFileObserver.startWatching();

        PowerManager pm = (PowerManager) getSystemService (Context.POWER_SERVICE);
        mWakeLock = pm.newWakeLock (PowerManager.SCREEN_BRIGHT_WAKE_LOCK |
                                    PowerManager.ON_AFTER_RELEASE, TAG);
        mWakeLock.acquire();

        IntentFilter intentFilter = new IntentFilter (WifiP2pManager.WIFI_P2P_CONNECTION_CHANGED_ACTION);
        registerReceiver (mReceiver, intentFilter);

        setSinkParameters(true);
        startMiracast(mIP, mPort);
        strIP = getlocalip();
    }

    protected void onDestroy()
    {
        super.onDestroy();

        certBtnState = 0;
        stopMiracast (true);
        unregisterReceiver (mReceiver);
        mWakeLock.release();
        setSinkParameters (false);

        quitLoop();
        mFileObserver.stopWatching();
    }

    /** register the BroadcastReceiver with the intent values to be matched */
    @Override
    public void onResume()
    {
        super.onResume();
    }

    private boolean parseSessionId(String fileName)
    {
        File file = new File(fileName);
        BufferedReader reader = null;
        String info = new String();
        try {
            reader = new BufferedReader(new FileReader(file));
            info = reader.readLine();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (reader != null) {
                try {
                    reader.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }

                if (info == null) {
                    Log.d (TAG, "parseSessionId info is NULL");
                    return false;
                } else {
                    StringTokenizer strToke = new StringTokenizer(info," ");
                    strSessionID = strToke.nextToken();
                    String sourceInfo = "IP address: " + strIP + ",  session Id: " + strSessionID;
                    Log.e("wpa_supplicant",  sourceInfo);
                    return true;
                }
            }
            else
                return false;
        }
    }

    private String getlocalip()
    {
        StringBuilder IFCONFIG = new StringBuilder();
        String ipAddr = "192.168.43.1";
        try {
            for (Enumeration<NetworkInterface> en = NetworkInterface
                .getNetworkInterfaces(); en.hasMoreElements();) {
                    NetworkInterface intf = en.nextElement();
                    for (Enumeration<InetAddress> enumIpAddr = intf
                    .getInetAddresses(); enumIpAddr.hasMoreElements();) {
                        InetAddress inetAddress = enumIpAddr.nextElement();
                        if (!inetAddress.isLoopbackAddress()
                            && !inetAddress.isLinkLocalAddress()
                            && inetAddress.isSiteLocalAddress()) {
                            IFCONFIG.append(inetAddress.getHostAddress().toString());
                            ipAddr = IFCONFIG.toString();
                        }
                    }
            }
        } catch (Exception ex) {
        }
        return ipAddr;
    }
    @Override
    public void onPause()
    {
        super.onPause();
    }

    @Override
    public boolean onKeyDown (int keyCode, KeyEvent event)
    {
        Log.d (TAG, "onKeyDown miracast running:" + mMiracastRunning + " keyCode:" + keyCode + " event:"  + event);
        if (mMiracastRunning)
        {
            switch (keyCode)
            {
            case KeyEvent.KEYCODE_VOLUME_UP:
            case KeyEvent.KEYCODE_VOLUME_DOWN:
            case KeyEvent.KEYCODE_VOLUME_MUTE:
                //openOsd();
                break;

            case KeyEvent.KEYCODE_BACK:
                //openOsd();
                return true;
            }
        }

        return super.onKeyDown (keyCode, event);
    }

    @Override
    public boolean onKeyUp (int keyCode, KeyEvent event)
    {
        Log.d (TAG, "onKeyUp miracast running:" + mMiracastRunning + " keyCode:" + keyCode + " event:"  + event);
        if (mMiracastRunning)
        {
            switch (keyCode)
            {
            case KeyEvent.KEYCODE_VOLUME_UP:
            case KeyEvent.KEYCODE_VOLUME_DOWN:
            case KeyEvent.KEYCODE_VOLUME_MUTE:
                break;

            case KeyEvent.KEYCODE_BACK:
                exitMiracastDialog();
                return true;
            }
        }
        return super.onKeyUp (keyCode, event);
    }

    @Override
    public void onConfigurationChanged (Configuration config)
    {
        super.onConfigurationChanged (config);

        //Log.d(TAG, "onConfigurationChanged: " + config);
        /*
        try {
        if (getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE) {
        }
        else if (getResources().getConfiguration().orientation == Configuration.ORIENTATION_PORTRAIT) {
        }
        }
        catch (Exception ex) {}
        */
    }

    private void openOsd()
    {
        switchGraphicLayer (true);
    }

    private void closeOsd()
    {
        switchGraphicLayer (false);
    }

    /**
     * open or close graphic layer
     *
     */
    public void switchGraphicLayer (boolean open)
    {
        //Log.d(TAG, (open?"open":"close") + " graphic layer");
        writeSysfs (FB0_BLANK, open ? "0" : "1");
    }

    private int writeSysfs (String path, String val)
    {
        if (!new File (path).exists() )
        {
            Log.e (TAG, "File not found: " + path);
            return 1;
        }

        try
        {
            FileWriter fw = new FileWriter(path);
            BufferedWriter writer = new BufferedWriter (fw, 64);
            try
            {
                writer.write (val);
            } finally
            {
                writer.close();
                fw.close();
            }
            return 0;

        }
        catch (IOException e)
        {
            Log.e (TAG, "IO Exception when write: " + path, e);
            return 1;
        }
    }

    private String readSysfs(String path) {
        if (!new File(path).exists()) {
            Log.e(TAG, "File not found: " + path);
            return null;
        }

        String str = null;
        StringBuilder value = new StringBuilder();

        try {
            FileReader fr = new FileReader(path);
            BufferedReader br = new BufferedReader(fr);
            try {
                while ((str = br.readLine()) != null) {
                    if (str != null)
                        value.append(str);
                }
                br.close();
                fr.close();
                if (value != null)
                    return value.toString();
                return null;
            } catch (IOException e) {
                e.printStackTrace();
                return null;
            }
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            return null;
        }
    }

    private void exitMiracastDialog()
    {
        new AlertDialog.Builder(this).setTitle(R.string.dlg_title)
            .setItems(new String[]{getString(R.string.disconnect_sink), getString(R.string.play_sink), getString(R.string.pause_sink)},
                new DialogInterface.OnClickListener()
               {
                   @Override
                   public void onClick(DialogInterface arg0, int arg1)
                   {
                       switch (arg1)
                       {
                            case 0:
                                nativeSetTeardown();
                                finishView();
                                break;
                            case 1:
                                if (certBtnState == 2)
                                {
                                    nativeSetPlay();
                                    certBtnState = 1;
                                }
                                break;
                            case 2:
                                if (certBtnState != 2)
                               {
                                    nativeSetPause();
                                    certBtnState = 2;
                                }
                                break;
                            default:
                                break;
                       }
                   }
                })
        .setNegativeButton("cancel", null).show();
    }

    public void startMiracast (String ip, String port)
    {
        Log.d (TAG, "start miracast isRunning:" + mMiracastRunning + " IP:" + ip + ":" + port);
        mMiracastRunning = true;

        Message msg = Message.obtain();
        msg.what = CMD_MIRACAST_START;
        Bundle data = msg.getData();
        data.putString (KEY_IP, ip);
        data.putString (KEY_PORT, port);
        if (mMiracastHandler != null)
        {
            mMiracastHandler.sendMessage(msg);
        }
    }

    /**
     * client or owner stop miracast
     * client stop miracast, only need open graphic layer
     */
    public void stopMiracast (boolean owner)
    {
        Log.d (TAG, "stop miracast running:" + mMiracastRunning);

        if (mMiracastRunning)
        {
            mMiracastRunning = false;
            nativeDisconnectSink();

            //Message msg = Message.obtain();
            //msg.what = CMD_MIRACAST_STOP;
            //mMiracastThreadHandler.sendMessage(msg);
        }
    }

    private boolean checkPathActive () {
        String defaultNode = null;

        String vfm_map = readSysfs("/sys/class/vfm/map");
        if (vfm_map == null) {
            Log.d(TAG, "readSysfs /sys/class/vfm/map is null");
            return false;
        }
        Log.d(TAG, "readSysfs /sys/class/vfm/map : " + vfm_map);

        int defaultStartIndex = vfm_map.indexOf("default {");
        Log.d(TAG, "defaultStartIndex : " + defaultStartIndex);
        if (defaultStartIndex >= 0) {
            int defaultEndIndex = vfm_map.indexOf("}", defaultStartIndex);
            Log.d(TAG, "defaultEndIndex : " + defaultEndIndex);
            if (defaultEndIndex >= 0) {
                defaultNode = vfm_map.substring(defaultStartIndex, defaultEndIndex);
                Log.d(TAG, "defaultNode : " + defaultNode);
                if (defaultNode.contains("(1)")) {
                    Log.d(TAG, "contains defaultNode");
                    return true;
                }
            }
        }

        Log.d(TAG, "uncontains defaultNode");
        return false;
    }

    private void waitForVideoUnreg() {
        boolean ret = false;
        int i = 0;
        for ( i = 0; i<50; i++ ) {
            ret = checkPathActive ();
            if (ret == false) {
                Log.d(TAG, "defaultNode inactive, return");
                return;
            } else {
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                Log.d(TAG, "defaultNode active, wait");
            }
        }

        Log.d(TAG, "defaultNode check timeout, return");
        return;
    }

    public void setSinkParameters (boolean start)
    {
        if (start)
        {
            if (null != mSystemControl)
            {
                mCueEnable = mSystemControl.readSysFs("/sys/module/di/parameters/cue_enable");
                mBypassDynamic = mSystemControl.readSysFs("/sys/module/di/parameters/bypass_dynamic");
                mBypassProg = mSystemControl.readSysFs("/sys/module/di/parameters/bypass_prog");

                mSystemControl.writeSysFs("/sys/module/di/parameters/cue_enable", "0");
                mSystemControl.writeSysFs("/sys/module/di/parameters/bypass_dynamic", "0");
                mSystemControl.writeSysFs("/sys/module/di/parameters/bypass_prog", "1");

                mSystemControl.writeSysFs("/sys/class/vfm/map", "rm default");
                mSystemControl.writeSysFs("/sys/class/vfm/map", "add default decoder deinterlace amvideo");
                mSystemControl.writeSysFs("/sys/module/amvdec_h264/parameters/error_recovery_mode", "1");
            }
            else
            {
                mCueEnable = readSysfs("/sys/module/di/parameters/cue_enable");
                mBypassDynamic = readSysfs("/sys/module/di/parameters/bypass_dynamic");
                mBypassProg = readSysfs("/sys/module/di/parameters/bypass_prog");

                writeSysfs("/sys/module/di/parameters/cue_enable", "0");
                writeSysfs("/sys/module/di/parameters/bypass_dynamic", "0");
                writeSysfs("/sys/module/di/parameters/bypass_prog", "1");

                writeSysfs("/sys/class/vfm/map", "rm default");
                writeSysfs("/sys/class/vfm/map", "add default decoder deinterlace amvideo");
                writeSysfs("/sys/module/amvdec_h264/parameters/error_recovery_mode", "1");
            }
        }
        else
        {
            waitForVideoUnreg();
            StringBuilder b = new StringBuilder(60);
            String vfmdefmap = SystemProperties.get("media.decoder.vfm.defmap");
            if (vfmdefmap == null) {
                b.append("add default decoder ppmgr amvideo");
            }
            else {
                b.append("add default ");
                b.append(vfmdefmap);
            }

            if (null != mSystemControl) {
                mSystemControl.writeSysFs("/sys/module/di/parameters/cue_enable", mCueEnable);
                mSystemControl.writeSysFs("/sys/module/di/parameters/bypass_dynamic", mBypassDynamic);
                mSystemControl.writeSysFs("/sys/module/di/parameters/bypass_prog", mBypassProg);
                mSystemControl.writeSysFs("/sys/class/video/vsync_pts_inc_upint", "0");

                mSystemControl.writeSysFs("/sys/class/vfm/map", "rm default");
                mSystemControl.writeSysFs("/sys/class/vfm/map", b.toString());
                mSystemControl.writeSysFs("/sys/module/amvdec_h264/parameters/error_recovery_mode", "0");
            } else {
                writeSysfs("/sys/module/di/parameters/cue_enable", mCueEnable);
                writeSysfs("/sys/module/di/parameters/bypass_dynamic", mBypassDynamic);
                writeSysfs("/sys/module/di/parameters/bypass_prog", mBypassProg);
                writeSysfs("/sys/class/video/vsync_pts_inc_upint", "0");

                writeSysfs("/sys/class/vfm/map", "rm default");
                writeSysfs("/sys/class/vfm/map", b.toString());
                writeSysfs("/sys/module/amvdec_h264/parameters/error_recovery_mode", "0");
            }
        }
    }
    private native void nativeConnectWifiSource (SinkActivity sink, String ip, int port);
    private native void nativeDisconnectSink();
    private native void nativeResolutionSettings (boolean isHD);
    private native void nativeSetPlay();
    private native void nativeSetPause();
    private native void nativeSetTeardown();
    //private native void nativeSourceStart(String ip);
    //private native void nativeSourceStop();
    // Native callback.
    private void notifyWfdError()
    {
         Log.d(TAG, "notifyWfdError received!!!");
         finishView();
    }

    private final int CMD_MIRACAST_START      = 10;
    private final int CMD_MIRACAST_STOP         = 11;
    class MiracastThread implements Runnable
    {
        public void run()
        {
            Looper.prepare();

            Log.v (TAG, "miracast thread run");

            mMiracastHandler = new Handler()
            {
                public void handleMessage (Message msg)
                {
                    switch (msg.what)
                    {
                        case CMD_MIRACAST_START:
                            {
                                Bundle data = msg.getData();
                                String ip = data.getString (KEY_IP);
                                String port = data.getString (KEY_PORT);

                                nativeConnectWifiSource (SinkActivity.this, ip, Integer.parseInt (port) );
                            }
                            break;

                        default:
                            break;
                    }
                }
            };

            synchronized (this)
            {
                notifyAll();
            }
            Looper.loop();
         }
      };

      public void quitLoop()
      {
          if (mMiracastHandler != null && mMiracastHandler.getLooper() != null)
          {
              Log.v(TAG, "miracast thread quit");
              mMiracastHandler.getLooper().quit();
          }
      }

    private class SurfaceCallback implements SurfaceHolder.Callback
    {
        @Override
        public void surfaceChanged (SurfaceHolder holder, int format, int width, int height)
        {
            // TODO Auto-generated method stub
            Log.v (TAG, "surfaceChanged");
        }

        @Override
        public void surfaceCreated (SurfaceHolder holder)
        {
            // TODO Auto-generated method stub
            Log.v (TAG, "surfaceCreated");
            nativeResolutionSettings (isHD);
        }

        @Override
        public void surfaceDestroyed (SurfaceHolder holder)
        {
            // TODO Auto-generated method stub
            Log.v (TAG, "surfaceDestroyed");
            if (getAndroidSDKVersion() == 17)
            {
                writeSysfs ("/sys/class/graphics/fb0/free_scale", "0");
                writeSysfs ("/sys/class/graphics/fb0/free_scale", "1");
            }
        }
    }

    private int getAndroidSDKVersion()
    {
        int version = 0;
        try
        {
            version = Integer.valueOf (android.os.Build.VERSION.SDK);
        }
        catch (NumberFormatException e)
        {
        }
        return version;
    }
}
