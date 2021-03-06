package com.subtitleview;

import android.R.bool;
import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.util.Log;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.FrameLayout;
import android.view.View;
import android.view.Gravity;
import com.subtitleparser.*;
import android.graphics.Matrix;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.widget.LinearLayout;
import android.os.Handler;
import android.os.Message;
import java.util.Timer;
import java.util.TimerTask;

public class SubtitleView extends FrameLayout {
        private static final String TAG = SubtitleView.class.getSimpleName();
        private boolean needSubTitleShow = true;
        private int timeoffset = 400;
        private SubData data = null;
        private int graphicViewMode = 0;
        private float wscale = 1.000f;
        private float hscale = 1.000f;
        private int wmax = 0;
        private int hmax = 0;
        private ImageView mImageView = null;
        private TextView mTextView = null;

        //for font
        Context mContext;
        private String mFont = null;
        Typeface mSimSun;
        Typeface mSimHei;

        //add for pgs show
        private SubData dataPgsA = null;
        private SubData dataPgsB = null;
        private boolean dataPgsAValid = false;
        private boolean dataPgsBValid = false;
        private boolean dataPgsAShowed = false;
        private boolean dataPgsBShowed = false;
        private final int SUBTITLE_PGS = 2;
        private final int SUBTITLE_DVB = 6;
        private final int SUBTITLE_TMD_TXT = 7;
        public void setGraphicSubViewMode (int flag) {
            graphicViewMode = flag;
        }

        public SubtitleView (Context context) {
            super (context);
            init (context);
        }

        public SubtitleView (Context context, AttributeSet attrs, int defStyle) {
            super (context, attrs, defStyle);
            init (context);
        }

        public SubtitleView (Context context, AttributeSet attrs) {
            super (context, attrs);
            init (context);
        }

        private void init (Context context) {
            mContext = context;
            mImageView = new ImageView (context);
            mTextView = new TextView (context);
            if (mTextView != null) {
                mTextView.setTextColor (0);
                mTextView.setTextSize (12);
                //mTextView.setTypeface (null, Typeface.BOLD);
                mTextView.setGravity (Gravity.CENTER);
            }
            wscale = 1.000f;
            hscale = 1.000f;
            SubManager.getinstance();
        }

        public void setImgSubRatio (float ratioW, float ratioH, int maxW, int maxH) {
            wscale = ratioW;
            hscale = ratioH;
            wmax = maxW;
            hmax = maxH;
        }

        public void setImgSubRatio(float ratio) {
            wscale = hscale = ratio;
        }

        public void setTextColor (int color) {
            if (mTextView != null) {
                mTextView.setTextColor (color);
            }
        }

        public void setTextSize (int size) {
            if (mTextView != null) {
                mTextView.setTextSize (size);
            }
        }

        public void setTextStyle (int style) {
            if (mTextView != null) {
                mTextView.setTypeface (null, style);
            }
        }

        public void setGravity (int gravity) {
            if (mTextView != null) {
                mTextView.setGravity (gravity);
            }
        }

        public void setViewStatus (boolean flag) {
            needSubTitleShow = flag;
            if (flag == false) {
                setVisibility (INVISIBLE);
            } else {
                setVisibility (VISIBLE);
            }
        }

        public void clear() {
            data = null;
            SubManager.getinstance().clear();
            this.removeAllViews();
            this.requestLayout();
        }

        public int getSubType() {
            int ret = 0; // text default
            if (data != null) {
                //Log.i(TAG,"[getSubType]data.subSize():"+data.subSize()+",data.gettype():"+data.gettype());
                if (data.subSize() > 0) {
                    if (data.gettype() == 1) {
                        ret = 1; //bitmap
                    }
                }
            }
            //Log.i(TAG,"[getSubType]ret:"+ret);
            return ret;
        }

        public String getSubTypeStr() {
            String typeStr = "null";
            Subtitle.SUBTYPE type = SubManager.getinstance().getSubType();
            switch (type) {
                case SUB_INVALID:
                    typeStr = "SUB_INVALID";
                    break;
                case SUB_MICRODVD:
                    typeStr = "MICRODVD";
                    break;
                case SUB_SUBRIP:
                    typeStr = "SUBRIP";
                    break;
                case SUB_SUBVIEWER:
                    typeStr = "SUBVIEWER";
                    break;
                case SUB_SAMI:
                    typeStr = "SAMI";
                    break;
                case SUB_VPLAYER:
                    typeStr = "VPLAYER";
                    break;
                case SUB_RT:
                    typeStr = "RT";
                    break;
                case SUB_SSA:
                    typeStr = "SSA";
                    break;
                case SUB_PJS:
                    typeStr = "PJS";
                    break;
                case SUB_MPSUB:
                    typeStr = "MPSUB";
                    break;
                case SUB_AQTITLE:
                    typeStr = "AQTITLE";
                    break;
                case SUB_SUBVIEWER2:
                    typeStr = "SUBVIEWER";
                    break;
                case SUB_SUBVIEWER3:
                    typeStr = "SUBVIEWER";
                    break;
                case SUB_SUBRIP09:
                    typeStr = "SUBRIP";
                    break;
                case SUB_JACOSUB:
                    typeStr = "JACOSUB";
                    break;
                case SUB_MPL1:
                    typeStr = "SUB_MPL";
                    break;
                case SUB_MPL2:
                    typeStr = "SUB_MPL";
                    break;
                case SUB_DIVX:
                    typeStr = "DIVX";
                    break;
                case SUB_IDXSUB:
                    typeStr = "IDX+SUB";
                    break;
                case SUB_COMMONTXT:
                    typeStr = "COMMONTXT";
                    break;
                case SUB_LRC:
                    typeStr = "LRC";
                    break;
                case INSUB:
                    typeStr = "INSUB";
                    break;
            }
            return typeStr;
        }

        public void redraw (SubData data) {
            this.removeAllViews();
            stopOsdTimeout();
            if (data != null) {
                if (data.subSize() > 0) {
                    if (data.gettype() == 1) {
                        //evaluteScale (data.getSubBitmap() );
                        Bitmap inter_bitmap = creatBitmapByScale (data.getSubBitmap(), wscale, hscale, wmax, hmax);
                        if ( (inter_bitmap != null) && (mImageView != null) ) {
                            mImageView.setImageBitmap (inter_bitmap);
                            this.addView (mImageView);
                        }
                    } else {
                        String sttmp = data.getSubString();
                        if (sttmp != null) {
                            sttmp = sttmp.replaceAll ("\r", "");
                            byte sttmp_2[] = sttmp.getBytes();
                            if (sttmp_2.length > 0 && 0 == sttmp_2[ sttmp_2.length - 1]) {
                                sttmp_2[ sttmp_2.length - 1] = 0x20;
                            }
                            if (mTextView != null) {
                                mTextView.setText (new String (sttmp_2) );
                                this.addView (mTextView);
                            }
                        }
                    }
                }
            }
            startSubShowTimeout();
            this.requestLayout();
        }

        public void redraw() {
            this.removeAllViews();
            stopOsdTimeout();
            if (data != null) {
                if (data.gettype() == 1) {
                    //evaluteScale (data.getSubBitmap() );
                    Bitmap inter_bitmap = creatBitmapByScale (data.getSubBitmap(), wscale, hscale, wmax, hmax);
                    if ( (inter_bitmap != null) && (mImageView != null) ) {
                        mImageView.setImageBitmap (inter_bitmap);
                        this.addView (mImageView);
                    }
                } else {
                    String sttmp = data.getSubString();
                    sttmp = sttmp.replaceAll ("\r", "");
                    byte sttmp_2[] = sttmp.getBytes();
                    if (sttmp_2.length > 0 && 0 == sttmp_2[ sttmp_2.length - 1]) {
                        sttmp_2[ sttmp_2.length - 1] = 0x20;
                    }
                    if (mTextView != null) {
                        if (mFont != null && (mFont.indexOf("hei") >= 0
                            || mFont.indexOf("Hei") >= 0)) {
                            mTextView.setTypeface(mSimHei);
                        }
                        else if (mFont != null && (mFont.indexOf("sun") >= 0
                            || mFont.indexOf("Sun") >= 0)) {
                            mTextView.setTypeface(mSimSun);
                        }
                        mTextView.setText (new String (sttmp_2) );
                        this.addView (mTextView);
                    }
                }
            }
            startSubShowTimeout();
            this.requestLayout();
        }

        private Timer timer = new Timer();
        private static final int MSG_SUB_SHOW_TIME_OUT = 0xd1;
        protected void startSubShowTimeout() {
            final Handler handler = new Handler() {
                public void handleMessage (Message msg) {
                    switch (msg.what) {
                        case MSG_SUB_SHOW_TIME_OUT:
                            SubtitleView.this.removeAllViews();
                            SubtitleView.this.requestLayout();
                            //stopSubThread();
                            break;
                    }
                    super.handleMessage (msg);
                }
            };

            TimerTask task = new TimerTask() {
                public void run() {
                    Message message = Message.obtain();
                    message.what = MSG_SUB_SHOW_TIME_OUT;
                    handler.sendMessage (message);
                }
            };

            stopOsdTimeout();
            if (timer == null) {
                timer = new Timer();
            }
            if (timer != null) {
                timer.schedule (task, 5 * 1000); //5s
            }
        }

        private void stopOsdTimeout() {
            if (timer != null) {
                timer.cancel();
            }
            timer = null;
        }

        public void setDisplayResolution (int width, int height) {
            SubManager.getinstance().setDisplayResolution (width, height);
        }

        public void setVideoResolution (int width, int height) {
            SubManager.getinstance().setVideoResolution (width, height);
        }

        public void evaluteScale (Bitmap bitmap) {
            float w_scale = 1.000f;
            float h_scale = 1.000f;
            int display_width = 0;
            int display_height = 0;
            int video_width = 0;
            int video_height = 0;
            int bitmap_width = 0;
            int bitmap_height = 0;
            int max_width = 0;
            int max_height = 0;
            //wscale = 1.000f;
            //hscale = 1.000f;
            display_width = SubManager.getinstance().getDisplayWidth();
            display_height = SubManager.getinstance().getDisplayHeight();
            video_width = SubManager.getinstance().getVideoWidth();
            video_height = SubManager.getinstance().getVideoHeight();
            if (bitmap != null) {
                bitmap_width = bitmap.getWidth();
                bitmap_height = bitmap.getHeight();
            }

            //Log.d(TAG, "disply width: " + display_width + ", height: " + display_height);
            //Log.d(TAG, "video width: " + video_width + ", height: " + video_height);
            //Log.d(TAG, "bitmap width: " + bitmap_width + ", height: " + bitmap_height);
            max_width = (video_width > bitmap_width) ? video_width : bitmap_width;
            max_height = (video_height > max_height) ? video_height : max_height;
            if ( (display_width <= 0) || (display_height <= 0)
                    || (max_width <= 0) || (max_height <= 0) ) {
                return;
            }
            if ( (max_width <= display_width) && (max_height <= display_height) ) {
                return;
            }
            if (this.getWidth() == display_width) {
                w_scale = ( (float) display_width) / max_width;
                h_scale = ( (float) display_height) / max_height;
            } else if (this.getWidth() == display_height) {
                w_scale = ( (float) display_height) / max_width;
                h_scale = ( (float) display_width) / max_height;
            }
            //Log.d(TAG, "w_scale: " + Float.toString(w_scale));
            //Log.d(TAG, "h_scale: " + Float.toString(h_scale));
            if ( (w_scale < 0.000f) || (h_scale < 0.000f) ) {
                return;
            }
            wscale = w_scale;
            hscale = h_scale;
        }

        public static Bitmap creatBitmapByScale (Bitmap bitmap, float w_scale, float h_scale, int wmax, int hmax) {
            if (bitmap == null) {
                return null;
            }
            int w = bitmap.getWidth();
            int h = bitmap.getHeight();
            /*
            if (wmax > 0 && w * w_scale > wmax) {
                w_scale = ((float)wmax) / w;
            }

            if (hmax > 0 && h * h_scale > hmax) {
                h_scale = ((float)hmax) / h;
            }

            Log.d(TAG, "bitmap width: " + w + ", height: " + h
                    + ", w_scale: " + Float.toString(w_scale)
                    + ", h_scale: " + Float.toString(h_scale));
            */
            Matrix matrix = new Matrix();
            matrix.postScale (w_scale, h_scale);
            Bitmap resizedBitmap = Bitmap.createBitmap (bitmap, 0, 0, w, h, matrix, true);
            return resizedBitmap;
        }

        /*
        suppose pgs data as follow:
        dataPgsA: size>0 startpts>0 delay=0 (show subtitle)
        dataPgsB: size=0 startpts>0 delay=0 (show blank)
        */
        private void getDataForPsgA (int curTime) {
            data = SubManager.getinstance().getSubData (curTime);
            if (data != null) {
                if ( (data.subSize() > 0) && (data.beginTime() > 0) ) {
                    dataPgsA = data;
                    dataPgsAValid = true;
                    dataPgsBValid = false; //after get dataPgsA, should get dataPgsB then
                }
            }
        }

        private void getDataForPsgB (int curTime) {
            data = SubManager.getinstance().getSubData (curTime);
            if (data != null) {
                //if((data.subSize() == 0) && (data.beginTime() > 0)) {
                if (data.beginTime() > 0) {
                    dataPgsB = data;
                    dataPgsBValid = true;
                }
            }
        }

        public int getSubTypeDetial() {
            return SubManager.getinstance().getSubTypeDetial();
        }

        private boolean resetForSeek = false;
        public void resetForSeek() {
            resetForSeek = true;
        }

        public void tick (int millisec) {
            if (needSubTitleShow == false) {
                return;
            }
            //add for pgs
        SubData datatmp = null;
        if (resetForSeek) {
            this.removeAllViews();
            this.requestLayout();
            data = null;
            dataPgsAValid = false;
            dataPgsBValid = false;
            dataPgsAShowed = false;
            dataPgsBShowed = false;
            resetForSeek = false;
            SubManager.getinstance().resetForSeek();
        }

        int modifytime = millisec + timeoffset;
        /*if((getSubTypeDetial() == 0) ||(getSubTypeDetial() == -1)) {
            return;
        }
        else */if (getSubTypeDetial() == SUBTITLE_PGS || getSubTypeDetial() == SUBTITLE_DVB || getSubTypeDetial() == SUBTITLE_TMD_TXT) {
            Log.i(TAG,"[tick]data:"+data+",dataPgsAValid:"+dataPgsAValid+",dataPgsBValid:"+dataPgsBValid+",modifytime:"+modifytime);
            if (data == null) {
                    if (dataPgsAValid == false) {
                        getDataForPsgA (modifytime);
                    } else if (dataPgsBValid == false) {
                        getDataForPsgB (modifytime);
                    } else {
                        // TODO:
                        //dataPgsAValid = true
                        //dataPgsBValid = true
                        //data = null
                        //error status
                        Log.i (TAG, "dataPgsAValid = true, dataPgsBValid = true, data = null, error status");
                    }
                } else { //data != null
                    if ( (dataPgsBValid == true) && (dataPgsAShowed == true) ) {
                        Log.i (TAG, "[tick]dataPgsB.beginTime():" + dataPgsB.beginTime() );
                        if (modifytime >= dataPgsB.beginTime() ) {
                            redraw (dataPgsB);
                            dataPgsAShowed = false;
                            //dataPgsBShowed = true;
                            dataPgsAValid = false; //current showing dataPgsB, reset dataPgsAValid and get dataPgsA in any case
                            getDataForPsgA(modifytime);
                        }
                    }
                    else if (dataPgsAValid == true) {
                        //enter this case means dataPgsBValid=fasle, should get dataPgsB
                        Log.i(TAG,"[tick]dataPgsA.beginTime():"+dataPgsA.beginTime());
                        if (modifytime >= dataPgsA.beginTime()) {
                            redraw(dataPgsA);
                            dataPgsAShowed = true;
                        }

                        if (dataPgsBValid == false) {
                            getDataForPsgB(modifytime);
                        }
                    } else {
                        //dataPgsAValid = false, dataPgsBValid = false, should get dataPgsA immediately
                        getDataForPsgA (modifytime);
                    }
                }
            } else {
                if (data != null) {
                    if ((modifytime >= data.beginTime()) && (modifytime <= data.endTime())) {
                        if (getVisibility() == View.GONE) {
                            return ;
                        }
                    } else {
                        data = SubManager.getinstance().getSubData (modifytime);
                    }
                } else {
                    data = SubManager.getinstance().getSubData (modifytime);
                }
                redraw();
            }
        }

        public void setDelay (int milsec) {
            timeoffset = milsec;
        }

        @Override
        public void onDraw (Canvas canvas) {
            super.onDraw (canvas);
        }

        public void closeSubtitle() {
            data = null;
            dataPgsA = null;
            dataPgsB = null;
            dataPgsAValid = false;
            dataPgsBValid = false;
            dataPgsAShowed = false;
            dataPgsBShowed = false;
            resetForSeek = false;
            wscale = 1.000f;
            hscale = 1.000f;
            SubManager.getinstance().closeSubtitle();
        }

        public void startSubThread() {
            Log.i (TAG, "startSubThread");
            SubManager.getinstance().startSubThread();
        }

        public void stopSubThread() {
            SubManager.getinstance().stopSubThread();
        }

        public void startSocketServer() {
            SubManager.getinstance().startSocketServer();
        }

        public void stopSocketServer() {
            SubManager.getinstance().stopSocketServer();
        }

        public void setIOType(int type) {
            SubManager.getinstance().setIOType(type);
        }

        public String getPcrscr() {
            return SubManager.getinstance().getPcrscr();
        }

        public void loadSubtitleFile (String path, String enc) throws Exception {
            SubManager.getinstance().loadSubtitleFile (path, enc);
        }

        public Subtitle.SUBTYPE setFile (SubID file, String enc) throws Exception {
            data = null;
            dataPgsA = null;
            dataPgsB = null;
            dataPgsAValid = false;
            dataPgsBValid = false;
            dataPgsAShowed = false;
            dataPgsBShowed = false;
            Subtitle.SUBTYPE tmp = SubManager.getinstance().setFile (file, enc);
            mFont = SubManager.getinstance().getFont();
            AssetManager mgr = mContext.getAssets();
            mSimSun = Typeface.createFromAsset(mgr, "fonts/simsun.ttf");
            mSimHei = Typeface.createFromAsset(mgr, "fonts/simhei.ttf");
            return tmp;
        }

        public SubtitleApi getSubtitleFile() {
            return SubManager.getinstance().getSubtitleFile();
        }

        public int getOriginW() {
            if (data != null) {
                return data.getOriginW();
            }
            return 0;
        }

        public int getOriginH() {
            if (data != null) {
                return data.getOriginH();
            }
            return 0;
        }
}
