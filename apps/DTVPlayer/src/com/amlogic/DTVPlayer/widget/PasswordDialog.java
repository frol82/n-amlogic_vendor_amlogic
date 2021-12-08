package com.amlogic.widget;

import android.util.Log;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.view.KeyEvent;
import android.view.Window;

import android.view.*;
import android.view.View.*;
import android.view.animation.*;
import android.widget.*;
import android.app.*;
import android.content.*;
import android.os.*;
import android.text.*;
import android.text.method.*;
import android.graphics.Color;
import android.view.inputmethod.InputMethodManager;
import com.amlogic.DTVPlayer.R;
import com.amlogic.DTVPlayer.DTVSettings;

abstract public class PasswordDialog {
	private static final String TAG="PasswordDialog";
	Dialog mDialog = null;
	private Context mContext = null;
	private Toast toast=null;
	/* ImageButton pin0 ;
    ImageButton pin1 ;
    ImageButton pin2 ;
    ImageButton pin3 ; */
	TextView mPinTextView = null;
       
    // private static int cur_button_position=0;
    
    /* private  String pin_char_0=null;
    private  String pin_char_1=null;
    private  String pin_char_2=null;
    private  String pin_char_3=null; */
	
	public PasswordDialog(Context context) {
		mContext = context;
		// cur_button_position=0;	
		mDialog = new Dialog(mContext,R.style.transparentDialog);
		//mDialog = new Dialog(mContext);
		if(mDialog == null){
			return;
		}

		//mDialog.setCancelable(false);
		mDialog.setCanceledOnTouchOutside(true);
		mDialog.setOnKeyListener( new DialogInterface.OnKeyListener(){
			public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
				
				switch (keyCode) {
					/* case KeyEvent.KEYCODE_0:
					case KeyEvent.KEYCODE_1:
					case KeyEvent.KEYCODE_2:
					case KeyEvent.KEYCODE_3:
					case KeyEvent.KEYCODE_4:
					case KeyEvent.KEYCODE_5:
					case KeyEvent.KEYCODE_6:
					case KeyEvent.KEYCODE_7:
					case KeyEvent.KEYCODE_8:
					case KeyEvent.KEYCODE_9:
						switch(cur_button_position){
							case 0:
								pin_char_0=Integer.toString(keyCode - KeyEvent.KEYCODE_0);
								pin0.setBackgroundResource(R.drawable.button_pin_in);
								pin1.requestFocus();
								break;
							case 1:
								pin_char_1=Integer.toString(keyCode - KeyEvent.KEYCODE_0);
								pin1.setBackgroundResource(R.drawable.button_pin_in);
								pin2.requestFocus();
								break;
							case 2:
								pin_char_2=Integer.toString(keyCode - KeyEvent.KEYCODE_0);
								pin2.setBackgroundResource(R.drawable.button_pin_in);
								pin3.requestFocus();
								break;
							case 3:
								pin_char_3=Integer.toString(keyCode - KeyEvent.KEYCODE_0);
								pin3.setBackgroundResource(R.drawable.button_pin_in);
								if(check_pin()){
									    dismissDialog();
										onCheckPasswordIsRight();
								}
								else{
									pin0.setBackgroundResource(R.drawable.button_pin_null);
									pin1.setBackgroundResource(R.drawable.button_pin_null);
									pin2.setBackgroundResource(R.drawable.button_pin_null);
									pin3.setBackgroundResource(R.drawable.button_pin_null);
									pin0.requestFocus();
									onCheckPasswordIsFalse();
									pin_char_0=null;
								    pin_char_1=null;
								    pin_char_2=null;
								    pin_char_3=null;
								}	
								break;	
						}
						return true; */
					case KeyEvent.KEYCODE_MENU:	
					case KeyEvent.KEYCODE_BACK:	
						if(onDealUpDownKey()){
							((Activity)mContext).onKeyDown(keyCode,event);
							return true;
						}
						break;
					case KeyEvent.KEYCODE_DPAD_UP:	
					case KeyEvent.KEYCODE_DPAD_DOWN:					
						if(onDealUpDownKey()){
							((Activity)mContext).onKeyDown(keyCode,event);
							dismissDialog();
							return true;
						}
						break;
					case KeyEvent.KEYCODE_DPAD_CENTER:
					case KeyEvent.KEYCODE_ENTER:	
						checkPassword();
						return true;
				}
				return false;
			}
		});
		

		mDialog.show();
		mDialog.setContentView(R.layout.vchip_pin);
		Window window = mDialog.getWindow();
		((PasswordLayout)window.findViewById(R.id.password_layout)).setPasswordDialog(this);

		/* WindowManager.LayoutParams lp=mDialog.getWindow().getAttributes();
		lp.dimAmount=0.0f;
		mDialog.getWindow().setAttributes(lp);
		mDialog.getWindow().addFlags(WindowManager.LayoutParams.FLAG_DIM_BEHIND); */

		pin_button_init(window);

	}

	private void pin_button_init(Window window ){
		/* pin0 = (ImageButton)window.findViewById(R.id.pin_button0);
		pin1 = (ImageButton)window.findViewById(R.id.pin_button1);
		pin2 = (ImageButton)window.findViewById(R.id.pin_button2);
		pin3 = (ImageButton)window.findViewById(R.id.pin_button3);
  	    	
	     	pin0.setFocusable(true);   
	     	pin0.requestFocus();   
	     	pin0.setFocusableInTouchMode(true);   

		pin0.setOnClickListener(new OnClickListener(){
			public void onClick(View v) {				  	 
				pin0.requestFocus(); 
		}});

		pin1.setOnClickListener(new OnClickListener(){
			public void onClick(View v) {
				pin1.requestFocus(); 
		}});

		pin2.setOnClickListener(new OnClickListener(){
			public void onClick(View v) {
				cur_button_position=2;
				pin2.requestFocus(); 
		}});	

		pin3.setOnClickListener(new OnClickListener(){
			public void onClick(View v) {				  	 
				pin3.requestFocus(); 
		}});	

	
            
	     	pin0.setOnFocusChangeListener(new ImageButton.OnFocusChangeListener(){   
	             public void onFocusChange(View v, boolean hasFocus) {   
	                 ImageButton imageButton = (ImageButton) v;   
	                 if (hasFocus) {   
	                 	cur_button_position=0;
	                      
	                 } else {   
	                     
	                 }   
	             }});   
    	
	    	pin1.setOnFocusChangeListener(new ImageButton.OnFocusChangeListener(){   
	           public void onFocusChange(View v, boolean hasFocus) {   
	                ImageButton imageButton = (ImageButton) v;   
	                if (hasFocus) {   
	                	cur_button_position=1;                     
	                } else {
	                }   
	            }});   
     	
	     	pin2.setOnFocusChangeListener(new ImageButton.OnFocusChangeListener(){   
	            public void onFocusChange(View v, boolean hasFocus) {   
	                ImageButton imageButton = (ImageButton) v;   
	                if (hasFocus) {   
	                	cur_button_position=2;
	                     
	                } else {   
	                    
	                }   
	            }});  
     	
		pin3.setOnFocusChangeListener(new ImageButton.OnFocusChangeListener(){   
			public void onFocusChange(View v, boolean hasFocus) {   
				ImageButton imageButton = (ImageButton) v;   
				if (hasFocus) {   				 
				cur_button_position=3;
				 
				} else {   

				}   
			}});   

		ImageView number = (ImageView)window.findViewById(R.id.number);
		number.setFocusable(false);     
 		number.setFocusableInTouchMode(false);   
		number.setOnClickListener(new OnClickListener(){
			public void onClick(View v) {				  	 
				InputMethodManager imm = (InputMethodManager)mContext.getSystemService(Context.INPUT_METHOD_SERVICE);  	
				imm.toggleSoftInput(0,InputMethodManager.HIDE_NOT_ALWAYS);
				//imm.showSoftInput(v, InputMethodManager.SHOW_IMPLICIT);  
		}}); */
		mPinTextView = (TextView)window.findViewById(R.id.pin_textview);
		// mPinTextView.requestFocus();

		window.findViewById(R.id.ok_icon).setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				/*
				if (check_pin()) {
					dismissDialog();
					onCheckPasswordIsRight();
				} else {
					onCheckPasswordIsFalse();
				}
				*/
				checkPassword();
			}
		});
		window.findViewById(R.id.return_icon).setOnClickListener(
			new View.OnClickListener(){	  
				public void onClick(View v) {		
					// TODO Auto-generated method stub	
					dismissDialog();
				}
			}
		);
	}

	public void checkPassword() {
		if (check_pin()) {
			dismissDialog();
			onCheckPasswordIsRight();
		} else {
			onCheckPasswordIsFalse();

			if(toast!=null)
				toast.cancel(); 
			toast = Toast.makeText(	mContext, 
	    		R.string.invalid_password,
	    		Toast.LENGTH_SHORT);
			toast.setGravity(Gravity.CENTER, 0, 0);
			toast.show();
		}
	}

	public void setDialogContent(String c){
		if(mDialog!=null){
			Window window = mDialog.getWindow();
			TextView t = (TextView)window.findViewById(R.id.content);
			t.setText(c);
		}	
	}
	
    private boolean check_pin(){
    	// String cur_pin = null;
    	// cur_pin=pin_char_0+pin_char_1+pin_char_2+pin_char_3;
		String cur_pin = mPinTextView.getText().toString();
		
    	String database_pin = "0000";
		
		DTVSettings mDTVSettings = new DTVSettings(mContext);
		database_pin=mDTVSettings.getPassWord();

		Log.d(TAG,"cur_pin = "+cur_pin+"----"+"database_ping = "+database_pin);
    	if(database_pin==null){
    		if(cur_pin.equals("0000"))
    			return true;
    	}
    	else{
    		if(cur_pin.equals(database_pin)){
    			return true;
    		}	
    	}	
    	return false;
    }
	
	public void dismissDialog(){
		if(toast!=null)
			toast.cancel(); 
		if(mDialog!=null&& mDialog.isShowing()){
			mDialog.dismiss();
		}
	}

	public void cancelDialog(){
		if(toast!=null)
			toast.cancel(); 
		if(mDialog!=null&& mDialog.isShowing()){
			mDialog.cancel();
		}
	}

	public void showDialog(){
		if(mDialog!=null&& mDialog.isShowing()==false){
			/* pin_char_0=null;
			pin_char_1=null;
			pin_char_2=null;
			pin_char_3=null; */
			mDialog.show();
		}
	}


	public boolean isShowing(){
		if(mDialog!=null&&mDialog.isShowing()){
			return mDialog.isShowing();
		}
		return false;
	}

	abstract public void onCheckPasswordIsRight();
	abstract public void onCheckPasswordIsFalse();
	abstract public boolean onDealUpDownKey();
}

 
