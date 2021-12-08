/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 */

package com.droidlogic.HdmiSwitch;

import com.droidlogic.HdmiSwitch.R;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.ObjectAnimator;
import android.app.Activity;
import android.app.ActivityManager;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.TranslateAnimation;
import android.widget.Button;
import android.widget.ImageView;

public class ShowCling extends Activity {
    private static final String TAG = "Show cling activity";
    private int[] location = new int[2];
    private int[] location2 = new int[2];
    @Override
    protected void onCreate (Bundle savedInstanceState) {
        super.onCreate (savedInstanceState);

        // Turn the title off
        requestWindowFeature (Window.FEATURE_NO_TITLE);
        // full screen
        //		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
        //				WindowManager.LayoutParams.FLAG_FULLSCREEN);
        //
        String value = "first";
        Bundle extras = getIntent().getExtras();
        if (extras != null) {
            value = extras.getString ("which_cling");
        }

        if (value.equals ("first") ) {
            setContentView (R.layout.hdmi_cling);
            initCling (R.id.hdmi_cling, null, false, 0);

            ImageView hand = (ImageView) findViewById (R.id.hdmi_hand);
            hand.getLocationOnScreen (location);

            TranslateAnimation anim = new TranslateAnimation (location[0] + 50, location[0] + 300,
                    location[1], location[1]);
            anim.setDuration (3000);
            anim.setRepeatCount (1);
            //anim.setRepeatMode(2);
            anim.setFillAfter (true);
            hand.startAnimation (anim);

            Button bt = (Button) findViewById (R.id.cling_next);
            bt.setText ("next");

        } else if (value.equals ("second") ) {
            setContentView (R.layout.hdmi_cling2);
            initCling (R.id.hdmi_cling2, null, false, 0);

            ImageView hand2 = (ImageView) findViewById (R.id.hdmi_hand2);
            hand2.getLocationOnScreen (location2);
            TranslateAnimation anim2 = new TranslateAnimation (location2[0] + 50, location2[0] + 300,
                    location2[1], location2[1]);
            anim2.setDuration (3000);
            anim2.setRepeatCount (1);
            anim2.setFillAfter (true);
            hand2.startAnimation (anim2);

        }
        //setContentView(R.layout.hdmi_cling);

        Log.d (TAG, "hdmi cling added");
    }

    @Override
    protected void onPause() {
        super.onPause();


    }
    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.d (TAG, "Destroying...");
    }

    @Override
    protected void onStop() {
        super.onStop();
        Log.d (TAG, "Stopping...");
    }

    //	private static final String PRESS_KEY = "com.amlogic.HdmiSwitch.prefs";
    private static final int DISMISS_CLING_DURATION = 250;
    private static final int SHOW_CLING_DURATION = 550;

    public void nextHdmiCling (View v) {
        String key = HdmiCling.CLING_DISMISS_KEY_720P;

        // show the cling when it auto connected
        Bundle extras = getIntent().getExtras();
        if (extras != null) {
            key = extras.getString ("on_which");
        }
        Intent i = new Intent (this, ShowCling.class);
        i.putExtra ("on_which", key);

        Log.d ("next", key);

        i.putExtra ("which_cling", "second");
        startActivity (i);
        finish();
    }
    public void dismissHdmiCling (View v) {
        String value = HdmiCling.CLING_DISMISS_KEY_720P ;
        HdmiCling cling = (HdmiCling) findViewById (R.id.hdmi_cling2);
        Bundle extras = getIntent().getExtras();
        if (extras != null) {
            value = extras.getString ("on_which");
        }

        Log.d ("dismiss", value);
        // dismissCling(cling, HdmiCling.CLING_DISMISS_KEY_720P, DISMISS_CLING_DURATION);
        dismissCling (cling, value, DISMISS_CLING_DURATION);
        finish();
    }
    private void dismissCling (final HdmiCling cling, final String flag, int duration) {
        if (cling != null) {
            ObjectAnimator anim = ObjectAnimator.ofFloat (cling, "alpha", 0f);
            anim.setDuration (duration);
            anim.addListener (new AnimatorListenerAdapter() {
                public void onAnimationEnd (Animator animation) {
                    cling.setVisibility (View.GONE);
                    cling.cleanup();
                    SharedPreferences prefs =
                        getSharedPreferences (HdmiSwitch.PRESS_KEY, Context.MODE_PRIVATE);
                    SharedPreferences.Editor editor = prefs.edit();
                    editor.putBoolean (flag, true);
                    editor.commit();
                };
            });
            anim.start();
        }
    }
    private HdmiCling initCling (int clingId, int[] positionData, boolean animate, int delay) {
        HdmiCling cling = (HdmiCling) findViewById (clingId);
        if (cling != null) {
            cling.init (this, positionData);
            cling.setVisibility (View.VISIBLE);
            cling.setLayerType (View.LAYER_TYPE_HARDWARE, null);

            if (animate) {
                cling.buildLayer();
                cling.setAlpha (0f);
                cling.animate()
                .alpha (1f)
                .setInterpolator (new AccelerateInterpolator() )
                .setDuration (SHOW_CLING_DURATION)
                .setStartDelay (delay)
                .start();
            } else {
                cling.setAlpha (1f);
            }
        }
        return cling;
    }

}
