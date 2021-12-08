package com.droidlogic.videoplayer;

import android.os.storage.*;
import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import com.droidlogic.videoplayer.R;

import android.app.AlertDialog;
import android.app.ListActivity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.Manifest;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;
import android.view.Menu;
import android.view.MenuItem;
import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.Method;
import java.lang.System;
import android.database.Cursor;
import android.provider.MediaStore;
import android.widget.ProgressBar;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import android.os.Handler;
import java.util.Collections;
import java.util.Comparator;
import java.util.Timer;
import java.util.TimerTask;
import android.os.Message;
import com.droidlogic.app.SystemControlManager;

public class FileList extends ListActivity {
        private static final String ROOT_PATH           = "/storage";
        private static final String SHEILD_EXT_STOR     = Environment.getExternalStorageDirectory().getPath() + "/external_storage";//"/storage/sdcard0/external_storage";
        private static final String NAND_PATH           = Environment.getExternalStorageDirectory().getPath();//"/storage/sdcard0";
        private static final String SD_PATH             = "/storage/external_storage/sdcard1";
        private static final String USB_PATH            = "/storage/external_storage";
        private static final String ASEC_PATH           = "/mnt/asec";
        private static final String SECURE_PATH         = "/mnt/secure";
        private static final String OBB_PATH            = "/mnt/obb";
        private static final String USB_DRIVE_PATH      = "/mnt/usbdrive";
        private static final String SHELL_PATH          = "/mnt/shell";
        private static final String SURPORT_BIN         = ",bin";
        private static final String NOT_SURPORT         = "";

        private Context mContext;
        private ApplicationInfo mAppInfo;

        private boolean listAllFiles = true;
        private boolean mFileFlag = false;
        private List<File> listFiles = null;
        private List<File> listVideos = null;
        private List<String> items = null;
        private List<String> paths = null;
        private List<String> currentlist = null;
        private String currenturl = null;
        private String root_path = ROOT_PATH;
        private String extensions ;
        private static String ISOpath = null;

        private TextView tileText;
        private TextView nofileText;
        private TextView searchText;
        private ProgressBar sp;
        private boolean isScanning = false;
        private boolean isQuerying = false;
        private int scanCnt = 0;
        private File file;
        private static String TAG = "FileList";
        Timer timer = new Timer();
        Timer timerScan = new Timer();

        private int item_position_selected, item_position_first, fromtop_piexl;
        private ArrayList<Integer> fileDirectory_position_selected;
        private ArrayList<Integer> fileDirectory_position_piexl;
        private int pathLevel = 0;
        private final String iso_mount_dir = "/mnt/loop";
        private static final String iso_mount_dir_s = "/mnt/loop";
        private Uri uri;
        private static SystemControlManager mSystemControl;
        private StorageManager mStorageManager;

        private void waitForBrowserIsoFile() {
            final Handler handler = new Handler() {
                public void handleMessage (Message msg) {
                    switch (msg.what) {
                        case 0x4c:
                            BrowserFile (iso_mount_dir);
                            break;
                    }
                    super.handleMessage (msg);
                }
            };
            TimerTask task = new TimerTask() {
                public void run() {
                    Message message = Message.obtain();
                    message.what = 0x4c;
                    handler.sendMessage (message);
                }
            };
            timer.cancel();
            timer = new Timer();
            timer.schedule (task, 100);//add 100ms delay to wait fuse finish
        }

        private void waitForRescan() {
            final Handler handler = new Handler() {
                public void handleMessage (Message msg) {
                    switch (msg.what) {
                        case 0x5c:
                            isScanning = false;
                            prepareFileForList();
                            timerScan.cancel();
                            break;
                    }
                    super.handleMessage (msg);
                }
            };
            TimerTask task = new TimerTask() {
                public void run() {
                    Message message = Message.obtain();
                    message.what = 0x5c;
                    handler.sendMessage (message);
                }
            };
            timer.cancel();
            timer = new Timer();
            timer.schedule (task, 500);
        }

        private void waitForScanFinish() {
            final Handler handler = new Handler() {
                public void handleMessage (Message msg) {
                    switch (msg.what) {
                        case 0x6c:
                            scanCnt--;
                            isScanning = false;
                            prepareFileForList();
                            break;
                    }
                    super.handleMessage (msg);
                }
            };
            TimerTask task = new TimerTask() {
                public void run() {
                    Message message = Message.obtain();
                    message.what = 0x6c;
                    handler.sendMessage (message);
                }
            };
            timerScan.cancel();
            timerScan = new Timer();
            timerScan.schedule (task, 20000);
        }

        private BroadcastReceiver mListener = new BroadcastReceiver() {
            @Override
            public void onReceive (Context context, Intent intent) {
                String action = intent.getAction();
                Uri uri = intent.getData();
                String path = uri.getPath();

                if (action == null || path == null) {
                    return;
                }

                if (action.equals(Intent.ACTION_MEDIA_EJECT)
                    || action.equals(Intent.ACTION_MEDIA_UNMOUNTED)) {
                    if (listAllFiles) {
                        prepareFileForList();
                    }
                    else {
                        if (PlayList.getinstance().rootPath.startsWith (path)
                            || PlayList.getinstance().rootPath.equals (root_path)
                            || (PlayList.getinstance().rootPath.startsWith (SD_PATH) && path.equals ("/storage/sdcard1"))) {
                            BrowserFile(root_path);
                        }
                    }
                }
                else if (action.equals(Intent.ACTION_MEDIA_MOUNTED)) {
                    if (PlayList.getinstance().rootPath == null
                        || PlayList.getinstance().rootPath.equals(root_path)) {
                        BrowserFile (root_path);
                    }
                }
                else if (action.equals(Intent.ACTION_MEDIA_SCANNER_STARTED)) {
                    if (!isScanning) {
                        isScanning = true;
                        setListAdapter (null);
                        showSpinner();
                        scanCnt++;
                        waitForScanFinish();
                    }
                }
                else if (action.equals(Intent.ACTION_MEDIA_SCANNER_FINISHED)) {
                    if (isScanning && (scanCnt == 1)) {
                        scanCnt--;
                        waitForRescan();
                    }
                }
            }
        };

        @Override
        public void onResume() {
            super.onResume();
            IntentFilter f = new IntentFilter();
            f.addAction (Intent.ACTION_MEDIA_EJECT);
            f.addAction (Intent.ACTION_MEDIA_MOUNTED);
            f.addAction (Intent.ACTION_MEDIA_UNMOUNTED);
            f.addDataScheme ("file");

            if (!listAllFiles) {
                File file = null;
                if (PlayList.getinstance().rootPath != null) {
                    file = new File (PlayList.getinstance().rootPath);
                }
                if ( (file != null) && file.exists()) {
                    File[] the_Files;
                    the_Files = file.listFiles (new MyFilter (extensions));
                    if (the_Files == null || the_Files.length <= 0) {
                        PlayList.getinstance().rootPath = root_path;
                    }
                    BrowserFile (PlayList.getinstance().rootPath);
                }
                else {
                    PlayList.getinstance().rootPath = root_path;
                    BrowserFile (PlayList.getinstance().rootPath);
                }
                getListView().setSelectionFromTop (item_position_selected, fromtop_piexl);
            }
            else {
                f.addAction (Intent.ACTION_MEDIA_SCANNER_STARTED);
                f.addAction (Intent.ACTION_MEDIA_SCANNER_FINISHED);
            }

            registerReceiver(mListener, f);
        }

        public void onDestroy() {
            super.onDestroy();
        }

        @Override
        public void onPause() {
            super.onPause();
            if (listAllFiles) {
                isScanning = false;
                isQuerying = false;
                scanCnt = 0;
                timer.cancel();
                timerScan.cancel();
            }
            unregisterReceiver(mListener);
        }

        @Override
        protected void onCreate (Bundle icicle) {
            super.onCreate (icicle);
            extensions = getResources().getString (R.string.support_video_extensions);
            requestWindowFeature (Window.FEATURE_NO_TITLE);
            setContentView (R.layout.file_list);
            mSystemControl = new SystemControlManager(this);
            if (!mSystemControl.getPropertyBoolean("sys.videoplayer.surportbin", false)) {
                extensions = extensions.replaceAll(SURPORT_BIN, NOT_SURPORT);
            }
            mStorageManager = (StorageManager)getSystemService(Context.STORAGE_SERVICE);
            mContext = this.getApplicationContext();
            mAppInfo = mContext.getApplicationInfo();
            PlayList.setContext (this);
            listAllFiles = mSystemControl.getPropertyBoolean("vplayer.listall.enable", false);
            currentlist = new ArrayList<String>();
            if (!listAllFiles) {
                try {
                    Bundle bundle = new Bundle();
                    bundle = this.getIntent().getExtras();
                    if (bundle != null) {
                        item_position_selected = bundle.getInt ("item_position_selected");
                        item_position_first = bundle.getInt ("item_position_first");
                        fromtop_piexl = bundle.getInt ("fromtop_piexl");
                        fileDirectory_position_selected = bundle.getIntegerArrayList ("fileDirectory_position_selected");
                        fileDirectory_position_piexl = bundle.getIntegerArrayList ("fileDirectory_position_piexl");
                        pathLevel = fileDirectory_position_selected.size();
                    }
                }
                catch (Exception e) {
                    e.printStackTrace();
                }
                if (PlayList.getinstance().rootPath == null) {
                    PlayList.getinstance().rootPath = root_path;
                }
                //BrowserFile (PlayList.getinstance().rootPath);
            }
            Button home = (Button) findViewById (R.id.Button_home);
            home.setOnClickListener (new View.OnClickListener() {
                public void onClick (View v) {
                    if (listAllFiles) {
                        if (!isScanning) {
                            reScanVideoFiles();
                        }
                    }
                    else {
                        FileList.this.finish();
                        PlayList.getinstance().rootPath = null;
                    }
                }
            });
            Button exit = (Button) findViewById (R.id.Button_exit);
            exit.setOnClickListener (new View.OnClickListener() {
                public void onClick (View v) {
                    if (listAllFiles) {
                        FileList.this.finish();
                        return;
                    }
                    if (paths == null) {
                        FileList.this.finish();
                        PlayList.getinstance().rootPath = null;
                    }
                    else {
                        if (paths.isEmpty() || paths.get (0) == null) {
                            FileList.this.finish();
                            PlayList.getinstance().rootPath = null;
                            return;
                        }
                        file = new File (paths.get (0).toString());
                        if (file.getParent().compareTo (iso_mount_dir) == 0 && ISOpath != null) {
                            file = new File (ISOpath + "/VIRTUAL_CDROM");
                            ISOpath = null;
                        }
                        currenturl = file.getParentFile().getParent();
                        if ( (file.getParent().compareToIgnoreCase (root_path) != 0) && (pathLevel > 0)) {
                            String path = file.getParent();
                            String parent_path = file.getParentFile().getParent();
                            if ( (path.equals (NAND_PATH) || path.equals (SD_PATH) || parent_path.equals (USB_PATH)) && (pathLevel > 0)) {
                                pathLevel = 0;
                                BrowserFile (ROOT_PATH);
                            }
                            else {
                                BrowserFile (currenturl);
                                pathLevel--;
                                getListView().setSelectionFromTop (fileDirectory_position_selected.get (pathLevel), fileDirectory_position_piexl.get (pathLevel));
                                fileDirectory_position_selected.remove (pathLevel);
                                fileDirectory_position_piexl.remove (pathLevel);
                            }
                        }
                        else {
                            FileList.this.finish();
                            PlayList.getinstance().rootPath = null;
                        }
                    }
                }
            });
            nofileText = (TextView) findViewById (R.id.TextView_nofile);
            searchText = (TextView) findViewById (R.id.TextView_searching);
            sp = (ProgressBar) findViewById (R.id.spinner);
            if (listAllFiles) {
                prepareFileForList();
            }
        }

        private void showSpinner() {
            if (listAllFiles) {
                if ( (isScanning) || (isQuerying)) {
                    sp.setVisibility (View.VISIBLE);
                    searchText.setVisibility (View.VISIBLE);
                    nofileText.setVisibility (View.INVISIBLE);
                }
                else {
                    sp.setVisibility (View.INVISIBLE);
                    searchText.setVisibility (View.INVISIBLE);
                    int total = paths.size();
                    if (total == 0) {
                        nofileText.setVisibility (View.VISIBLE);
                    }
                    else if (total > 0) {
                        nofileText.setVisibility (View.INVISIBLE);
                    }
                }
            }
            else {
                sp.setVisibility (View.GONE);
                nofileText.setVisibility (View.GONE);
                searchText.setVisibility (View.GONE);
            }
        }


        private void prepareFileForList() {
            if (listAllFiles) {
                //Intent intent = getIntent();
                //uri = intent.getData();
                String[] mCursorCols = new String[] {
                    MediaStore.Video.Media._ID,
                    MediaStore.Video.Media.DATA,
                    MediaStore.Video.Media.TITLE,
                    MediaStore.Video.Media.SIZE,
                    MediaStore.Video.Media.DURATION,
                    //              MediaStore.Video.Media.BOOKMARK,
                    //              MediaStore.Video.Media.PLAY_TIMES
                };
                String patht = null;
                String namet = null;
                paths = new ArrayList<String>();
                items = new ArrayList<String>();
                paths.clear();
                items.clear();
                setListAdapter (null);
                isQuerying = true;
                showSpinner();
                uri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI;
                Cursor cursor = getContentResolver().query (uri, mCursorCols, null, null, null);
                cursor.moveToFirst();
                int colidx = cursor.getColumnIndexOrThrow (MediaStore.Video.Media.DATA);
                for (int i = 0; i < cursor.getCount(); i++) {
                    patht = cursor.getString (colidx);
                    //Log.e("wxl", "cursor["+colidx+"]:"+patht);
                    int index = patht.lastIndexOf ("/");
                    if (index >= 0) {
                        namet = patht.substring (index);
                    }
                    items.add (namet);
                    paths.add (patht);
                    cursor.moveToNext();
                }
                tileText = (TextView) findViewById (R.id.TextView_path);
                tileText.setText (R.string.all_file);
                if (paths.size() > 0) {
                    setListAdapter (new MyAdapter (this, items, paths));
                }
                isQuerying = false;
                showSpinner();
                if (cursor != null) {
                    cursor.close();
                }
            }
        }

        private void BrowserFile (String filePath) {
            int i = 0;
            int dev_usb_count = 0;
            int dev_cd_count = 0;
            file = new File (filePath);
            listFiles = new ArrayList<File>();
            items = new ArrayList<String>();
            paths = new ArrayList<String>();
            String[] files = file.list();

            listFiles.clear();
            searchFile (file);
            if (listFiles.isEmpty()) {
                Toast.makeText (FileList.this, R.string.str_no_file, Toast.LENGTH_SHORT).show();
                //paths =currentlist;
                paths.clear();
                paths.addAll (currentlist);
                return;
            }

            PlayList.getinstance().rootPath = filePath;
            //Log.d(TAG, "BrowserFile() listFiles.size():"+listFiles.size());
            File [] fs = new File[listFiles.size()];
            for (i = 0; i < listFiles.size(); i++) {
                fs[i] = listFiles.get (i);
                //Log.d(TAG, "BrowserFile() fs[i]:"+fs[i]);
            }

            try {
                Arrays.sort (fs, new MyComparator (MyComparator.NAME_ASCEND));
            }
            catch (IllegalArgumentException ex) {
            }

            for (i = 0; i < fs.length; i++) {
                File tempF = fs[i];
                String tmppath = tempF.getName();
                //Log.d(TAG, "BrowserFile() tmppath:"+tmppath + ", filePath:" + filePath + ", ROOT_PATH:" + ROOT_PATH);
                //change device name;
                if (filePath.equals (ROOT_PATH)) {
                    //shield for android 6.0 support
                    /*String tpath = tempF.getAbsolutePath();
                    if (tpath.equals (NAND_PATH)) {
                        tmppath = "sdcard";
                    }
                    else if (tpath.equals (SD_PATH)) {
                        tmppath = "external_sdcard";
                    }
                    else if ( ( (!tpath.equals (SD_PATH)) && tpath.startsWith (USB_PATH + "/sd")) || tpath.startsWith (ROOT_PATH + "/udisk")) {
                        dev_usb_count++;
                        char data = (char) ('A' + dev_usb_count - 1);
                        tmppath =  getText (R.string.usb_device_str) + "(" + data + ":)" ;
                        //tmppath = "usb"+" "+tpath.substring(5);//5 is the len of "/mnt/"
                    }
                    else if ( (!tpath.equals (SD_PATH)) && tpath.startsWith (USB_PATH + "/sr")) {
                        dev_cd_count++;
                        char data = (char) ('A' + dev_cd_count - 1);
                        tmppath =  getText (R.string.cdrom_device_str) + "(" + data + ":)" ;
                    }
                    //delete used folder
                    if ( (!tpath.equals (ASEC_PATH)) && (!tpath.equals (SECURE_PATH)) &&
                            (!tpath.equals (OBB_PATH)) && (!tpath.equals (USB_DRIVE_PATH))
                            && (!tpath.equals (SHELL_PATH))) {
                        String path = changeDevName (tmppath);
                        //Log.d(TAG, "BrowserFile() items.add path:"+path);
                        //String stateStr = Environment.getStorageState(new File(path));
                        //if (stateStr.equals(Environment.MEDIA_MOUNTED)) {
                            items.add (path);
                            paths.add (tempF.getPath());
                        //}
                    }*/

                    //change Device name for android 6.0
                    //internal storage
                    String tpath = tempF.getAbsolutePath();
                    if (tpath.equals (NAND_PATH)) {
                        items.add (getString(R.string.sdcard_device_str));
                        paths.add (tempF.getPath());
                    }

                    //external storage
                    Class<?> volumeInfoClazz = null;
                    Method getDescriptionComparator = null;
                    Method getBestVolumeDescription = null;
                    Method getVolumes = null;
                    Method isMountedReadable = null;
                    Method getType = null;
                    Method getPath = null;
                    List<?> volumes = null;
                    try {
                        volumeInfoClazz = Class.forName("android.os.storage.VolumeInfo");
                        getDescriptionComparator = volumeInfoClazz.getMethod("getDescriptionComparator");
                        getBestVolumeDescription = StorageManager.class.getMethod("getBestVolumeDescription", volumeInfoClazz);
                        getVolumes = StorageManager.class.getMethod("getVolumes");
                        isMountedReadable = volumeInfoClazz.getMethod("isMountedReadable");
                        getType = volumeInfoClazz.getMethod("getType");
                        getPath = volumeInfoClazz.getMethod("getPath");
                        volumes = (List<?>)getVolumes.invoke(mStorageManager);

                        for (Object vol : volumes) {
                            if (vol != null && (boolean)isMountedReadable.invoke(vol) && (int)getType.invoke(vol) == 0) {
                                File path = (File)getPath.invoke(vol);
                                //Log.d(TAG, "BrowserFile() tmppath:"+tmppath + ", path.getName():" + path.getName() + ", path.getPath():" + path.getPath());
                                if (tmppath.equals(path.getName())) {
                                    items.add((String)getBestVolumeDescription.invoke(mStorageManager, vol));
                                    paths.add(path.getPath());
                                }
                            }
                        }
                    }catch (Exception ex) {
                        ex.printStackTrace();
                    }
                }
                else {
                    //Log.d(TAG, "BrowserFile() 1 items.add tmppath:"+tmppath);
                    items.add (tmppath);
                    paths.add (tempF.getPath());
                }
            }
            tileText = (TextView) findViewById (R.id.TextView_path);
            tileText.setText (catShowFilePath (filePath));
            setListAdapter (new MyAdapter (this, items, paths));
        }

        private String changeDevName (String tmppath) {
            String path = "";
            String internal = getString (R.string.memory_device_str);
            String sdcard = getString (R.string.sdcard_device_str);
            String usb = getString (R.string.usb_device_str);
            String cdrom = getString (R.string.cdrom_device_str);
            String sdcardExt = getString (R.string.ext_sdcard_device_str);
            //Log.i("wxl","[changeDevName]tmppath:"+tmppath);
            if (tmppath.equals ("flash")) {
                path = internal;
            }
            else if (tmppath.equals ("sdcard")) {
                path = sdcard;
            }
            else if (tmppath.equals ("usb")) {
                path = usb;
            }
            else if (tmppath.equals ("cd-rom")) {
                path = cdrom;
            }
            else if (tmppath.equals ("external_sdcard")) {
                path = sdcardExt;
            }
            else {
                path = tmppath;
            }
            //Log.i("wxl","[changeDevName]path:"+path);
            return path;
        }

        private String catShowFilePath (String path) {
            String text = null;
            if (path.startsWith ("/mnt/flash")) {
                text = path.replaceFirst ("/mnt/flash", "/mnt/nand");
            }
            else if (path.startsWith ("/mnt/sda")) {
                text = path.replaceFirst ("/mnt/sda", "/mnt/usb sda");
            }
            else if (path.startsWith ("/mnt/sdb")) {
                text = path.replaceFirst ("/mnt/sdb", "/mnt/usb sdb");
            }
            //else if(path.startsWith("/mnt/sdcard"))
            //text=path.replaceFirst("/mnt/sdcard","sdcard");
            return text;
        }

        public void searchFile (File file) {
            File filetmp;
            File[] the_Files;
            the_Files = file.listFiles (new MyFilter (extensions));
            if (the_Files == null) {
                Toast.makeText (FileList.this, R.string.str_no_file, Toast.LENGTH_SHORT).show();
                return;
            }
            String curPath = file.getPath();
            if (curPath.equals (root_path)) {
                //internal storage
                File dir = new File (NAND_PATH);
                if (dir.exists() && dir.isDirectory()) {
                    filetmp = new File (NAND_PATH);
                    if (filetmp.listFiles() != null && filetmp.listFiles().length > 0) {
                        listFiles.add (dir);
                    }
                }

                //external storage
                Class<?> volumeInfoClazz = null;
                Method getDescriptionComparator = null;
                Method getBestVolumeDescription = null;
                Method getVolumes = null;
                Method isMountedReadable = null;
                Method getType = null;
                Method getPath = null;
                List<?> volumes = null;
                try {
                    volumeInfoClazz = Class.forName("android.os.storage.VolumeInfo");
                    getDescriptionComparator = volumeInfoClazz.getMethod("getDescriptionComparator");
                    getBestVolumeDescription = StorageManager.class.getMethod("getBestVolumeDescription", volumeInfoClazz);
                    getVolumes = StorageManager.class.getMethod("getVolumes");
                    isMountedReadable = volumeInfoClazz.getMethod("isMountedReadable");
                    getType = volumeInfoClazz.getMethod("getType");
                    getPath = volumeInfoClazz.getMethod("getPath");
                    volumes = (List<?>)getVolumes.invoke(mStorageManager);

                    for (Object vol : volumes) {
                        if (vol != null && (boolean)isMountedReadable.invoke(vol) && (int)getType.invoke(vol) == 0) {
                            File path = (File)getPath.invoke(vol);
                            listFiles.add(path);
                        }
                    }
                }catch (Exception ex) {
                    ex.printStackTrace();
                }

                //shield for android 6.0 support
                /*File dir = new File (NAND_PATH);
                if (dir.exists() && dir.isDirectory()) {
                    filetmp = new File (NAND_PATH);
                    if (filetmp.listFiles() != null && filetmp.listFiles().length > 0) {
                        listFiles.add (dir);
                    }
                }
                dir = new File (SD_PATH);
                if (dir.exists() && dir.isDirectory()) {
                    filetmp = new File (SD_PATH);
                    //Log.i(TAG,"[searchFile]filetmp.listFiles():"+filetmp.listFiles());
                    //Log.i(TAG,"[searchFile]================");
                    //if (filetmp.listFiles() != null && filetmp.listFiles().length > 0) {
                    String stateStr = Environment.getStorageState(filetmp);
                    if (stateStr.equals(Environment.MEDIA_MOUNTED)) {
                        listFiles.add (dir);
                    }
                }
                dir = new File (USB_PATH);
                if (dir.exists() && dir.isDirectory()) {
                    if (dir.listFiles() != null) {
                    for (File pfile : dir.listFiles()) {
                            if (pfile.isDirectory()) {
                                String path = pfile.getAbsolutePath();
                                if ( (path.startsWith (USB_PATH + "/sd") || path.startsWith (USB_PATH + "/sr")) && !path.equals (SD_PATH)) {
                                    filetmp = new File (path);
                                    if (filetmp.listFiles() != null && filetmp.listFiles().length > 0) {
                                        listFiles.add (pfile);
                                    }
                                }
                            }
                        }
                    }
                }
                dir = new File (ROOT_PATH);
                if (dir.exists() && dir.isDirectory()) {
                    if (dir.listFiles() != null) {
                    for (File qfile : dir.listFiles()) {
                            if (qfile.isDirectory()) {
                                String path = qfile.getAbsolutePath();
                                if (path.startsWith (ROOT_PATH + "/udisk")) {
                                    //filetmp = new File (path);
                                    //if (filetmp.listFiles() != null && filetmp.listFiles().length > 0) {
                                    String stateStr = Environment.getStorageState(new File(path));
                                    if (stateStr.equals(Environment.MEDIA_MOUNTED)) {
                                        listFiles.add (qfile);
                                    }
                                }
                            }
                        }
                    }
                }*/
                return;
            }
            for (int i = 0; i < the_Files.length; i++) {
                File tempF = the_Files[i];
                if (tempF.isDirectory()) {
                    if (!tempF.isHidden()) {
                        listFiles.add (tempF);
                    }
                    //shield some path
                    if ( (tempF.toString()).equals (SHEILD_EXT_STOR)) {
                        listFiles.remove (tempF);
                        continue;
                    }
                }
                else {
                    try {
                        listFiles.add (tempF);
                    }
                    catch (Exception e) {
                        return;
                    }
                }
            }
        }

        private static void mount(String path) {
               mSystemControl.loopMountUnmount(false, null);
               mSystemControl.loopMountUnmount(true, path);
        }

        public static boolean isISOFile (File file) {
            String fname = file.getName();
            String sname = ".iso";
            if (fname == "") {
                Log.e (TAG, "NULL file");
                return false;
            }
            if (file.isFile() && fname.toLowerCase().endsWith (sname)) {
                return true;
            }
            return false;
        }

        private static boolean isHasDir(File[] files, String name) {
           for (File file : files) {
               if (name != null && name.equals(file.getName()) && file.isDirectory())
                   return true;
           }
           return false;
        }

        public static boolean isBDFile(File file)
        {
            if (file.isDirectory()) {
                File[] rootFiles = file.listFiles();
                if (rootFiles != null && rootFiles.length >= 1 && isHasDir(rootFiles, "BDMV")) {
                    File bdDir = new File(file.getPath(), "BDMV");
                    String[] files = bdDir.list();
                    ArrayList<String> names = new ArrayList<String>();
                    for (int i = 0; i < files.length; i++)
                        names.add(files[i]);
                    if (names.contains("index.bdmv") && names.contains("PLAYLIST")
                        && names.contains("CLIPINF") && names.contains("STREAM"))
                        return true;
                }
            } else if (isISOFile(file)) {
                ISOpath = file.getPath();
                mount(ISOpath);
                File isofile = new File(iso_mount_dir_s);
                if (isofile.exists() && isofile.isDirectory()) {
                    File[] rootFiles = isofile.listFiles();
                    if (rootFiles != null && rootFiles.length >= 1 && isHasDir(rootFiles, "BDMV")) {
                        File bdfiles = new File(iso_mount_dir_s, "BDMV");
                        String[] bdmvFiles = bdfiles.list();
                        ArrayList<String> names = new ArrayList<String>();
                        for (int i = 0; i < bdmvFiles.length; i++)
                            names.add(bdmvFiles[i]);
                        if (names.contains("index.bdmv") && names.contains("PLAYLIST")
                            && names.contains("CLIPINF") && names.contains("STREAM"))
                            return true;
                    }
                }
            }
            return false;
        }

        public static void execCmd (String cmd) {
            int ch;
            Process p = null;
            Log.d (TAG, "exec command: " + cmd);
            try {
                p = Runtime.getRuntime().exec (cmd);
                InputStream in = p.getInputStream();
                InputStream err = p.getErrorStream();
                StringBuffer sb = new StringBuffer (512);
                while ( (ch = in.read()) != -1) {
                    sb.append ( (char) ch);
                }
                if (sb.toString() != "") {
                    Log.d (TAG, "exec out:" + sb.toString());
                }
                while ( (ch = err.read()) != -1) {
                    sb.append ( (char) ch);
                }
                if (sb.toString() != "") {
                    Log.d (TAG, "exec error:" + sb.toString());
                }
            }
            catch (IOException e) {
                Log.d (TAG, "IOException: " + e.toString());
            }
        }

        @Override
        protected void onListItemClick (ListView l, View v, int position, long id) {
            mFileFlag = true;
            File file = new File (paths.get (position));
            currentlist.clear();
            currentlist.addAll (paths);
            //currentlist =paths;
            if (fileDirectory_position_selected == null) {
                fileDirectory_position_selected = new ArrayList<Integer>();
            }
            if (fileDirectory_position_piexl == null) {
                fileDirectory_position_piexl = new ArrayList<Integer>();
            }
            if (isBDFile(file)) {
                mFileFlag = true;
            } else {
                if (file.isDirectory()) {
                    item_position_selected = getListView().getSelectedItemPosition();
                    item_position_first = getListView().getFirstVisiblePosition();
                    View cv = getListView().getChildAt (item_position_selected - item_position_first);
                    if (cv != null) {
                        fromtop_piexl = cv.getTop();
                    }
                    BrowserFile (paths.get (position));
                    if (!listFiles.isEmpty()) {
                        fileDirectory_position_selected.add (item_position_selected);
                        fileDirectory_position_piexl.add (fromtop_piexl);
                        pathLevel++;
                    }
                    mFileFlag = false;
                } else if (isISOFile(file)) {
                    waitForBrowserIsoFile();
                    fileDirectory_position_selected.add (item_position_selected);
                    fileDirectory_position_piexl.add (fromtop_piexl);
                    pathLevel++;
                    mFileFlag = false;
                } else
                    mFileFlag = true;
            }
            if (mFileFlag) {
                if (!listAllFiles) {
                    int pos = filterDir (file);
                    if (pos < 0) {
                        return;
                    }
                    PlayList.getinstance().rootPath = file.getParent();
                    PlayList.getinstance().setlist (paths, pos);
                    item_position_selected = getListView().getSelectedItemPosition();
                    item_position_first = getListView().getFirstVisiblePosition();
                    if (!listAllFiles) {
                        View cv = getListView().getChildAt (item_position_selected - item_position_first);
                        if (cv != null) {
                            fromtop_piexl = cv.getTop();
                        }
                    }
                }
                else {
                    PlayList.getinstance().setlist (paths, position);
                }
                showvideobar();
            }
        }
        public boolean onKeyDown (int keyCode, KeyEvent event) {
            if (keyCode == KeyEvent.KEYCODE_BACK) {
                if (listAllFiles) {
                    FileList.this.finish();
                    return true;
                }
                if (paths == null) {
                    FileList.this.finish();
                    PlayList.getinstance().rootPath = null;
                }
                else {
                    if (paths.isEmpty() || paths.get (0) == null) {
                        FileList.this.finish();
                        PlayList.getinstance().rootPath = null;
                        return true;
                    }
                    file = new File (paths.get (0).toString());
                    if (file.getParent().compareTo (iso_mount_dir) == 0 && ISOpath != null) {
                        file = new File (ISOpath + "/VIRTUAL_CDROM");//"/VIRTUAL_CDROM" random add for path level reduce
                        ISOpath = null;
                    }
                    currenturl = file.getParentFile().getParent();
                    if ( (file.getParent().compareToIgnoreCase (root_path) != 0) && (pathLevel > 0)) {
                        String path = file.getParent();
                        String parent_path = file.getParentFile().getParent();
                        if ( (path.equals (NAND_PATH) || path.equals (SD_PATH) || parent_path.equals (USB_PATH)) && (pathLevel > 0)) {
                            pathLevel = 0;
                            BrowserFile (ROOT_PATH);
                        }
                        else {
                            BrowserFile (currenturl);
                            pathLevel--;
                            getListView().setSelectionFromTop (fileDirectory_position_selected.get (pathLevel), fileDirectory_position_piexl.get (pathLevel));
                            fileDirectory_position_selected.remove (pathLevel);
                            fileDirectory_position_piexl.remove (pathLevel);
                        }
                    }
                    else {
                        FileList.this.finish();
                        PlayList.getinstance().rootPath = null;
                    }
                }
                return true;
            }
            return super.onKeyDown (keyCode, event);
        }
        private void showvideobar() {
            //* new an Intent object and ponit a class to start
            Intent intent = new Intent();
            Bundle bundle = new Bundle();
            if (!listAllFiles) {
                bundle.putInt ("item_position_selected", item_position_selected);
                bundle.putInt ("item_position_first", item_position_first);
                bundle.putInt ("fromtop_piexl", fromtop_piexl);
                bundle.putIntegerArrayList ("fileDirectory_position_selected", fileDirectory_position_selected);
                bundle.putIntegerArrayList ("fileDirectory_position_piexl", fileDirectory_position_piexl);
            }
            bundle.putBoolean ("backToOtherAPK", false);
            intent.setClass (FileList.this, VideoPlayer.class);
            intent.putExtras (bundle);
            ///wxl delete
            /*SettingsVP.setSystemWrite(sw);
            if (SettingsVP.chkEnableOSD2XScale() == true)
                this.setVisible(false);*/
            /*if (mAppInfo.targetSdkVersion >= Build.VERSION_CODES.M &&
                (PackageManager.PERMISSION_DENIED == ContextCompat.checkSelfPermission(mContext, Manifest.permission.READ_EXTERNAL_STORAGE))) {
                ActivityCompat.requestPermissions(FileList.this,
                    new String[]{Manifest.permission.READ_EXTERNAL_STORAGE},
                    //MY_PERMISSIONS_REQUEST_READ_EXTERNAL_STORAGE//0);
            }
            else {
                startActivity (intent);
                FileList.this.finish();
            }*/
            startActivity (intent);
            FileList.this.finish();
        }

        public int filterDir (File file) {
            int pos = -1;
            File[] the_Files;
            File parent = new File (file.getParent());
            the_Files = parent.listFiles (new MyFilter (extensions));
            if (the_Files == null) {
                return pos;
            }
            pos = 0;
            listVideos = new ArrayList<File>();
            for (int i = 0; i < the_Files.length; i++) {
                File tempF = the_Files[i];
                if (isBDFile(tempF) || (tempF.isFile() && !isISOFile(tempF))) {
                    listVideos.add (tempF);
                }
            }
            paths = new ArrayList<String>();
            File [] fs = new File[listVideos.size()];
            for (int i = 0; i < listVideos.size(); i++) {
                fs[i] = listVideos.get (i);
            }
            try {
                Arrays.sort (fs, new MyComparator (MyComparator.NAME_ASCEND));
            }
            catch (IllegalArgumentException ex) {
            }
            for (int i = 0; i < fs.length; i++) {
                File tempF = fs[i];
                if (tempF.getPath().equals (file.getPath())) {
                    pos = i;
                }
                paths.add (tempF.getPath());
            }
            return pos;
        }

        //option menu
        private final int MENU_ABOUT = 0;
        public boolean onCreateOptionsMenu (Menu menu) {
            menu.add (0, MENU_ABOUT, 0, R.string.str_about);
            return true;
        }

        public boolean onOptionsItemSelected (MenuItem item) {
            switch (item.getItemId()) {
                case MENU_ABOUT:
                    try {
                        Toast.makeText (FileList.this, " VideoPlayer \n Version: " +
                                        FileList.this.getPackageManager().getPackageInfo ("com.droidlogic.videoplayer", 0).versionName,
                                        Toast.LENGTH_SHORT)
                        .show();
                    }
                    catch (NameNotFoundException e) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                    }
                    return true;
            }
            return false;
        }

        public void reScanVideoFiles() {
            Intent intent = new Intent (Intent.ACTION_MEDIA_MOUNTED, Uri.parse ("file://" + ROOT_PATH));
            this.sendBroadcast (intent);
        }

        public void stopMediaPlayer() { //stop the backgroun music player
            Intent intent = new Intent();
            intent.setAction ("com.android.music.musicservicecommand.pause");
            intent.putExtra ("command", "stop");
            this.sendBroadcast (intent);
        }
}
