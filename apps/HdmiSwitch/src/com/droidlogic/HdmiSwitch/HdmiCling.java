/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 */

package com.droidlogic.HdmiSwitch;

import com.droidlogic.HdmiSwitch.R;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.util.Log;
import android.widget.FrameLayout;

public class HdmiCling extends FrameLayout {
    static final String CLING_DISMISS_KEY_480P = "cling.480p.dismissed";
    static final String CLING_DISMISS_KEY_720P = "cling.720p.dismissed";
    static final String CLING_DISMISS_KEY_1080I = "cling.1080i.dismissed";
    static final String CLING_DISMISS_KEY_1080P = "cling.1080p.dismissed";
    public static final String CLING_DISMISS_FIRST = "cling_first.dismissed";

    // used to identify the screen size
    private String mDrawIdentifier;

    private ShowCling mSwitch;
    private Drawable mBackground;
    private Drawable mPunchThroughGraphic;
    private Drawable mHandTouchGraphic;
    private int mPunchThroughGraphicCenterRadius;
    private boolean mIsInitialized;
    private int[] mPositionData;
    private int mAppIconSize;
    private int mButtonBarHeight;
    private float mRevealRadius;

    private Paint mErasePaint;

    public HdmiCling (Context context) {
        super (context, null, 0);
    }

    public HdmiCling (Context context, AttributeSet attrs) {
        super (context, attrs, 0);
        // TODO Auto-generated constructor stub
    }

    public HdmiCling (Context context, AttributeSet attrs, int defStyle) {
        super (context, attrs, defStyle);
        TypedArray a = context.obtainStyledAttributes (attrs, R.styleable.Cling, defStyle, 0);
        mDrawIdentifier = a.getString (R.styleable.Cling_drawIdentifier);
        a.recycle();
    }

    public void init (ShowCling showCling, int[] positionData) {
        // TODO Auto-generated method stub
        if (!mIsInitialized) {
            mSwitch = showCling;
            mPositionData = positionData;
            Resources r = getContext().getResources();
            mPunchThroughGraphic = r.getDrawable (R.drawable.cling);
            mPunchThroughGraphicCenterRadius =
                r.getDimensionPixelSize (R.dimen.clingPunchThroughGraphicCenterRadius);
            mAppIconSize = r.getDimensionPixelSize (R.dimen.app_icon_size);
            mRevealRadius = mAppIconSize * 1f;
            mButtonBarHeight = r.getDimensionPixelSize (R.dimen.button_bar_height);

            mErasePaint = new Paint();
            mErasePaint.setXfermode (new PorterDuffXfermode (PorterDuff.Mode.MULTIPLY) );
            mErasePaint.setColor (0xFFFFFF);
            mErasePaint.setAlpha (0);

            mIsInitialized = true;
        }
    }

    public void cleanup() {
        // TODO Auto-generated method stub
        mBackground = null;
        mPunchThroughGraphic = null;
        mHandTouchGraphic = null;
        mIsInitialized = false;
    }

    @Override
    public boolean onTouchEvent (android.view.MotionEvent event) {
        int[] pos = getPunchThroughPosition();
        double diff = Math.sqrt (Math.pow (event.getX() - pos[0], 2) +
                                 Math.pow (event.getY() - pos[1], 2) );
        if (diff < mRevealRadius) {
            return false;
        }
        return true;
    };

    @SuppressLint ("NewApi")
    @Override
    protected void dispatchDraw (Canvas canvas) {
        if (mIsInitialized) {
            DisplayMetrics metrics = new DisplayMetrics();
            mSwitch.getWindowManager().getDefaultDisplay().getRealMetrics (metrics);

            int height = metrics.heightPixels;
            int width = metrics.widthPixels;

            //        	// initailize the draw buffer
            //        	Bitmap b = Bitmap.createBitmap(getMeasuredWidth(), getMeasuredHeight(),
            //			    Bitmap.Config.ARGB_8888);
            Bitmap b = Bitmap.createBitmap (width, height,
                                            Bitmap.Config.ARGB_8888);

            Canvas c = new Canvas (b);

            mBackground = getResources().getDrawable (R.drawable.bg_cling1);
            mBackground.draw (c);


            canvas.drawBitmap (b, 0, 0, null);
            c.setBitmap (null);
            b = null;
        }
        // Draw the rest of the cling
        super.dispatchDraw (canvas);
    };

    private int[] getPunchThroughPosition() {
        return new int[] {getMeasuredWidth() / 2, getMeasuredHeight() - (mButtonBarHeight / 2) };
    }
}
