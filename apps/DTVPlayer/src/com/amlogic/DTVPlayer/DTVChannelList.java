package com.amlogic.DTVPlayer;

import android.util.Log;
import android.os.Bundle;
import android.os.AsyncTask;
import com.amlogic.tvutil.TVMessage;
import com.amlogic.tvutil.TVConst;
import com.amlogic.tvutil.TVProgram;
import com.amlogic.tvutil.TVProgramNumber;
import com.amlogic.tvactivity.TVActivity;
import com.amlogic.tvutil.TVChannelParams;
import com.amlogic.tvutil.TVScanParams;
import com.amlogic.tvutil.TVSatellite;
import com.amlogic.tvutil.TVConst;
import com.amlogic.tvutil.TVGroup;

import java.text.SimpleDateFormat;
import java.util.*;
import android.text.*;
import android.text.method.*;
import android.view.*;
import android.view.View.*;
import android.view.animation.*;
import android.util.DisplayMetrics;
import android.widget.*;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.AbsListView.OnScrollListener;
import android.app.*;
import android.content.*;
import android.graphics.Color;
import com.amlogic.widget.Rotate3D;
import com.amlogic.widget.CustomDialog;
import com.amlogic.widget.CustomDialog.ICustomDialog;
import com.amlogic.DTVPlayer.R;

public class DTVChannelList extends DTVActivity{
	private static final String TAG="DTVChannelList";
	ListView ListView_channel=null;
	TextView Text_title=null;
	ProgressBar ProgressBar_channel=null;
	private int class_total=0;
	private int cur_class_no=-1;
	private int cur_select_item=0;
	private IconAdapter myAdapter=null;
	private TVProgram[] mTVProgramList=null;
	private TVProgram[] mRadioProgramList=null;
	private TVProgram[] mFavProgramList=null;
	private TVProgram[] mSearchProgramList=null;
	private DTVSettings mDTVSettings=null;

	int db_id=-1;
	private int service_type=TVProgram.TYPE_TV;
	private boolean favor=false;
	private static final int LEFT = 0;
	private static final int RIGHT = 1;

	private  synchronized void getListData(int type){
		if(type == -1){
			mFavProgramList=TVProgram.selectByFavorite(this,true);	
		}
		else {
			if(type == TVProgram.TYPE_TV){
				if(mTVProgramList==null)
					mTVProgramList = TVProgram.selectByType(this, type, true);
			}	
			else{
				if(mRadioProgramList==null)
					mRadioProgramList = TVProgram.selectByType(this, type, true);
			}	
		}
	}

	TVGroup[] mTVGroup=null;
	private int getListProgramClass(){
		mTVGroup=DTVProgramManagerGetGroupList();
		if(mTVGroup!=null)
			return mTVGroup.length;
		else
			return 0;
	}
	
	private LinearLayout LinearLayoutListView=null;
	private void DTVChannelListUIInit(){

		Bundle bundle = this.getIntent().getExtras();
		if(bundle!=null){
	    	db_id = bundle.getInt("db_id");
			service_type=getCurrentProgramType();
		}	

	    	//LinearLayoutListView = (LinearLayout)findViewById(R.id.LinearLayoutListView);
		//initAnimation();

		ProgressBar_channel = (ProgressBar)findViewById(R.id.ProgressBar_channel);
		Text_title=(TextView)findViewById(R.id.Text_title);
		Text_title.setTextColor(Color.YELLOW);
		class_total = getListProgramClass();
		if(service_type == TVProgram.TYPE_RADIO){
			Text_title.setText(R.string.radio);
			myAdapter = new IconAdapter(DTVChannelList.this,mRadioProgramList);
		}	
		else{
			service_type = TVProgram.TYPE_TV;
			Text_title.setText(R.string.tv);
			myAdapter = new IconAdapter(DTVChannelList.this,mTVProgramList);
		}

		MyAsyncTask mTask = new MyAsyncTask();  
		mTask.execute(service_type);  
		
		ListView_channel = (ListView) findViewById(R.id.ListView_channel);
		
		ListView_channel.setOnItemSelectedListener(mOnSelectedListener);
		ListView_channel.setOnScrollListener(new listOnScroll()); 
		ListView_channel.setOnItemClickListener(mOnItemClickListener);
		ListView_channel.setAdapter(myAdapter);
		setFocusPosition();

		findViewById(R.id.arrow_left).setOnClickListener(
			new View.OnClickListener(){	  
				public void onClick(View v) {		
					// TODO Auto-generated method stub	
					DTVListDealLeftAndRightKey(LEFT);
				}
			}
		);

		findViewById(R.id.arrow_right).setOnClickListener(
			new View.OnClickListener(){	  
				public void onClick(View v) {		
					// TODO Auto-generated method stub	
					DTVListDealLeftAndRightKey(RIGHT);
				}
			}
		);

		if (!mDTVSettings.getScanRegion().contains("DVBS")) {
			findViewById(R.id.ok_icon).setOnClickListener(new View.OnClickListener() {
				@Override
				public void onClick(View v) {
					chooseItem(cur_select_item);
				}
			});
		}
		findViewById(R.id.return_icon).setOnClickListener(
			new View.OnClickListener(){	  
				public void onClick(View v) {		
					// TODO Auto-generated method stub	
					finish();
				}
			}
		);	

		myAdapter.notifyDataSetChanged();
	}

	public void setFocusPosition(){
		int i = 0;
		switch(service_type){
			case TVProgram.TYPE_TV:
				if(mTVProgramList!=null){
					ListView_channel.setFocusableInTouchMode(true);
				  	ListView_channel.requestFocus();
				  	ListView_channel.requestFocusFromTouch();
					for(i=0;i<mTVProgramList.length;i++){
						if(db_id == mTVProgramList[i].getID()){	
			        		ListView_channel.setSelection(i);
							cur_select_item = i;
							break;
						}
					}	
				}
				break;
			case TVProgram.TYPE_RADIO:
				if(mRadioProgramList!=null){
					ListView_channel.setFocusableInTouchMode(true);
				  	ListView_channel.requestFocus();
				  	ListView_channel.requestFocusFromTouch();
					for(i=0;i<mRadioProgramList.length;i++){
						if(db_id == mRadioProgramList[i].getID()){	
			        		ListView_channel.setSelection(i);
							cur_select_item = i;
							break;
						}
					}	
				}
				break;
			case -1:
				if(mFavProgramList!=null){
					ListView_channel.setFocusableInTouchMode(true);
				  	ListView_channel.requestFocus();
				  	ListView_channel.requestFocusFromTouch();
					for(i=0;i<mFavProgramList.length;i++){
						if(db_id == mFavProgramList[i].getID()){	
			        		ListView_channel.setSelection(i);
							cur_select_item = i;
							break;
						}
					}	
				}
				break;	
		}	
	}
	
	public void onCreate(Bundle savedInstanceState){
		Log.d(TAG, "onCreate");
		super.onCreate(savedInstanceState);
	}

	public void onConnected(){
		Log.d(TAG, "connected");
		super.onConnected();
		mDTVSettings = new DTVSettings(this);
		if(mDTVSettings.getScanRegion().contains("DVBS")==true){
			Log.d(TAG,"DVBS");
			setContentView(R.layout.dtvchannellist_dvbs); 
		}
		else{
			setContentView(R.layout.dtvchannellist); 
		}
		DTVChannelListUIInit();
	}

	public void onDisconnected(){
		Log.d(TAG, "disconnected");
		super.onDisconnected();
	}

	public void onMessage(TVMessage msg){
		Log.d(TAG, "message "+msg.getType());
		switch (msg.getType()) {
			case TVMessage.TYPE_SCAN_PROGRESS:
				break;
			case TVMessage.TYPE_SCAN_STORE_BEGIN:
				Log.d(TAG, "Storing ...");
				break;
			case TVMessage.TYPE_SCAN_STORE_END:
				Log.d(TAG, "Store Done !");
				break;
			case TVMessage.TYPE_SCAN_END:
				Log.d(TAG, "Scan End");
				break;
			default:
				break;
		}
	}
  
  	private AdapterView.OnItemSelectedListener mOnSelectedListener = new AdapterView.OnItemSelectedListener(){
		public void onItemSelected(AdapterView<?> parent, View v, int position, long id){
			//parent.setSelection(position);
			//v.requestFocus();
			if(ListView_channel.hasFocus() == true){
				if(mDTVSettings.getScanRegion().contains("DVBS")==true){
					TextView info= (TextView) findViewById(R.id.channel_info);
					if(myAdapter!=null){
						TVProgram[] list = myAdapter.getDate();
						if(list[position].getChannel()!=null){
							int fre = list[position].getChannel().getParams().getFrequency();
							int sym = list[position].getChannel().getParams().getSymbolRate();
							String pol="";
							if(list[position].getChannel().getParams().getPolarisation()==0)
								pol="V";
							else
								pol="H";
							int sat_id = list[position].getChannel().getParams().getSatId();
							String sat_name = TVSatellite.tvSatelliteSelect(DTVChannelList.this,sat_id).getSatelliteName();
							
							info.setText(String.valueOf(fre/1000)+"MHz   "+pol+"   "+String.valueOf(sym/1000)+"KS/s   "+sat_name);
						}
					}
				}
			}
			cur_select_item = position;
		}
		public void onNothingSelected(AdapterView<?> parent){
		}
	};

	private void chooseItem(int position) {
		int db_id = -1;
		int serviceType = 0;
		switch(service_type){
			case TVProgram.TYPE_TV:
				db_id = mTVProgramList[position].getID();
				serviceType = mTVProgramList[position].getType();
				break;
			case TVProgram.TYPE_RADIO:
				db_id = mRadioProgramList[position].getID();
				serviceType = mRadioProgramList[position].getType();
				break;
			case -1:
				db_id = mFavProgramList[position].getID();
				serviceType = mFavProgramList[position].getType();
				break;	
		}
		
		if (DTVPlayerGetCurrentProgramType() != serviceType) {
			if (serviceType == TVProgram.TYPE_RADIO) {
				setProgramType(TVProgram.TYPE_RADIO);
			} else {
				setProgramType(TVProgram.TYPE_TV);
			}
		}
		if (DTVPlayerGetCurrentProgramID() != db_id) {
			Bundle bundle = new Bundle();
			bundle.putInt("db_id", db_id);
			bundle.putInt("service_type", serviceType);
			bundle.putString("activity_tag", "DTVChannelList");
			Intent intent = new Intent();
			intent.setClass(DTVChannelList.this, DTVPlayer.class);
			intent.putExtras(bundle);
			startActivity(intent);
		}
		DTVChannelList.this.finish();
	}

	private AdapterView.OnItemClickListener mOnItemClickListener =new AdapterView.OnItemClickListener(){
		public void onItemClick(AdapterView<?> parent, View v, int position, long id){
			v.requestFocus();
			parent.setSelection(position);
			chooseItem(position);
		}
	};

	private class IconAdapter extends BaseAdapter {
		private LayoutInflater mInflater;
		private Context cont;
		private TVProgram[] listItems;
		private int selectItem;
		
		class ViewHolder {
			TextView prono;
			TextView text;	
			ImageView icon_scrambled;
			ImageView icon_fav;
			ImageView icon;
		}
		
		public IconAdapter(Context context, TVProgram[] list) {
			super();
			cont = context;
			listItems = list;
			mInflater=LayoutInflater.from(context);			  
		}

		public void fillData(TVProgram[] list){
			listItems = list;
		}

		public TVProgram[] getDate(){
			return listItems;
		}

		public int getCount() {
			if(listItems==null)
				return 0;
			else
				return listItems.length;
		}

		public Object getItem(int position) {
			return position;
		}
		
		public long getItemId(int position) {
			return position;
		}

		public void setSelectItem(int position){
			this.selectItem = position;
		}
        
	        public int getSelectItem(){
				return this.selectItem;
	        }
		
		public View getView(int position, View convertView, ViewGroup parent) {
			ViewHolder holder;	
			if (convertView == null){    
				convertView = mInflater.inflate(R.layout.dtvchannellist_item, null);
				
				holder = new ViewHolder();
				holder.prono = (TextView)convertView.findViewById(R.id.prono);
				holder.text = (TextView) convertView.findViewById(R.id.ItemText);
				holder.icon = (ImageView) convertView.findViewById(R.id.icon);
				holder.icon_scrambled = (ImageView)convertView.findViewById(R.id.icon_scrambled);
				holder.icon_fav = (ImageView)convertView.findViewById(R.id.icon_fav);
				convertView.setTag(holder);
			}
			else {
				// Get the ViewHolder back to get fast access to the TextView
				// and the ImageView.
				holder = (ViewHolder) convertView.getTag();
			}
		
			// Bind the data efficiently with the holder.
			if(listItems!=null&&position<listItems.length){
				if(mDTVSettings.getScanRegion().contains("ATSC")==false){
					holder.prono.setText(Integer.toString(listItems[position].getNumber().getNumber()));
				}
				else{
					holder.prono.setText(Integer.toString(listItems[position].getNumber().getNumber())+"-"+Integer.toString(listItems[position].getNumber().getMinor()));
				}

				holder.text.setText(listItems[position].getName());

				if(db_id == listItems[position].getID()){  
					//convertView.setBackgroundColor(Color.RED);  
					holder.text.setTextColor(Color.YELLOW);
				}	
				else{
					//convertView.setBackgroundColor(Color.TRANSPARENT); 
					holder.text.setTextColor(Color.WHITE);
				}	
			
				if(listItems[position].getLockFlag()){
					holder.icon.setBackgroundResource(R.drawable.dtvplayer_icon_lock); 
				}	
				else{
					holder.icon.setBackgroundResource(Color.TRANSPARENT);
				}

				if(listItems[position].getFavoriteFlag()){
					holder.icon_fav.setBackgroundResource(R.drawable.dtvplayer_icon_fav); 
				}	
				else{
					holder.icon_fav.setBackgroundResource(Color.TRANSPARENT);
				}	

				if(listItems[position].getScrambledFlag()){
					holder.icon_scrambled.setBackgroundResource(R.drawable.dtvplayer_icon_scrambled); 
				}	
				else{
					holder.icon_scrambled.setBackgroundResource(Color.TRANSPARENT);
				}			  
			}
			return convertView;
		}
	}	
		
	class listOnScroll implements OnScrollListener{
		public void onScroll(AbsListView view, int firstVisibleItem,int visibleItemCount, int totalItemCount) {
			//reset_timer();	
		}
		public void onScrollStateChanged(AbsListView view, int scrollState) {
			//reset_timer();
		}
    }	
	
	class MouseClick implements OnClickListener{    
		public void onClick(View v) {
				// TODO Auto-generated method stub	
		    	//reset_timer();
				switch (v.getId()) {
					case R.id.arrow_left:
						break;
				}
			}
	     }
	
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		// TODO Auto-generated method stub
		//reset_timer();
		if(!connected){
			return true;
			}
		switch (keyCode) {
			case KeyEvent.KEYCODE_DPAD_LEFT:
				DTVListDealLeftAndRightKey(LEFT);
				break;		
			case KeyEvent.KEYCODE_DPAD_RIGHT:	
				DTVListDealLeftAndRightKey(RIGHT);
				break;
			case KeyEvent.KEYCODE_DPAD_DOWN:
				if(ListView_channel!=null){
					if(cur_select_item== ListView_channel.getCount()-1){
				    	ListView_channel.setSelection(0); 	
						return true;
					}	
				}	
				break;
			case KeyEvent.KEYCODE_DPAD_UP:
				if(ListView_channel!=null){
					if(cur_select_item== 0){
						ListView_channel.setSelection(ListView_channel.getCount()-1); 
						return true;
					}	
				}	
				break;
			case DTVActivity.KEYCODE_AUDIO_LANGUAGE:
				if(ListView_channel.getChildCount()>cur_select_item)
					ListView_channel.setSelection(0);
				else{
					ListView_channel.setSelection(cur_select_item-ListView_channel.getChildCount());
				}
				myAdapter.notifyDataSetChanged();
				break;
			case DTVActivity.KEYCODE_SUBTITLE:
				int p=0;				
				p = cur_select_item+ListView_channel.getChildCount();
				if(p<ListView_channel.getCount())
					ListView_channel.setSelection(p-1);
				else{
					ListView_channel.setSelection(ListView_channel.getCount()-1);
				}
				myAdapter.notifyDataSetChanged();			
				break;		
			case DTVActivity.KEYCODE_REC:
				//if(myAdapter!=null&&myAdapter.getCount()>0)
					//showPvrTimeSetDialog(DTVChannelList.this);
				return true;
			case DTVActivity.KEYCODE_GOTO_BUTTON:
				if(mDTVSettings!=null&&(mDTVSettings.getScanRegion().contains("DVBS")))
					showSatellitesList();
				return true;
			case DTVActivity.KEYCODE_RED_BUTTON: 			
				if(myAdapter!=null&&myAdapter.getCount()>0){
					showBookAddDialog();
				}
				return true;
			case DTVActivity.KEYCODE_YELLOW_BUTTON:
				if(mDTVSettings!=null&&(mDTVSettings.getScanRegion().contains("DVBS")))
					showProgramSearchDialog();
				return true;
		}
		return super.onKeyDown(keyCode, event);
	}	  

	
	Rotate3D lQuest1Animation=null;		
	Rotate3D lQuest2Animation =null;	
 	Rotate3D rQuest1Animation=null;	
    Rotate3D rQuest2Animation=null;	
	public void initAnimation() {
		DisplayMetrics dm = new DisplayMetrics();
		dm = getResources().getDisplayMetrics();
		int mCenterX = dm.widthPixels / 2;
		int mCenterY = dm.heightPixels / 2;
		
		int duration = 300;
		lQuest1Animation = new Rotate3D(0, -90, mCenterX, mCenterY);	
		lQuest1Animation.setFillAfter(true);
		lQuest1Animation.setDuration(duration);

		lQuest2Animation = new Rotate3D(90, 0, mCenterX, mCenterY);		
		lQuest2Animation.setFillAfter(true);
		lQuest2Animation.setDuration(duration);

		rQuest1Animation = new Rotate3D(0, 90, mCenterX, mCenterY);		
		rQuest1Animation.setFillAfter(true);
		rQuest1Animation.setDuration(duration);

		rQuest2Animation = new Rotate3D(-90, 0, mCenterX, mCenterY);	
		rQuest2Animation.setFillAfter(true);
		rQuest2Animation.setDuration(duration);
	}

	MyAsyncTask mTask = null;
	private void setProgramList(int type){
		if(mTask == null)
			mTask = new MyAsyncTask();  
		else{
			mTask.cancel(true);
			mTask = new MyAsyncTask();  
		}	
		switch(type){
			case TVProgram.TYPE_TV:
				{
					Text_title.setText(R.string.tv);
					favor = false;
					service_type = TVProgram.TYPE_TV;
					mTask.execute(service_type);  
				}
				break;
			case TVProgram.TYPE_RADIO:
				{
					Text_title.setText(R.string.radio);
					favor = false;
					service_type = TVProgram.TYPE_RADIO;
					
					mTask.execute(service_type);  
				}
				break;
			case -1:
				{
					Text_title.setText(R.string.favorite);
					favor = true;
					service_type = -1;
					mTask.execute(service_type);  
				}
				break;	
		}
	}

	private void setTVGroupList() {
		Text_title.setText(mTVGroup[cur_class_no].getName());
		favor = false;
		service_type = -1;
		myAdapter.notifyDataSetChanged();
	}

	private void DTVListDealLeftAndRightKey(int direction) {
		switch (direction) {
			case LEFT:
				//LinearLayoutListView.startAnimation(lQuest1Animation); 
				if ((service_type == TVProgram.TYPE_RADIO) && !favor) {
					if (class_total > 0) {
						cur_class_no = class_total - 1;
						setTVGroupList();
					} else
						setProgramList(TVProgram.TYPE_TV);
				} else if ((service_type != TVProgram.TYPE_TV) && (service_type != TVProgram.TYPE_RADIO)
					&& !favor) {
					if (cur_class_no > 0 && class_total > 0) {
						cur_class_no--;
						setTVGroupList();
					} else
						setProgramList(TVProgram.TYPE_TV);
				} else if ((service_type == TVProgram.TYPE_TV) && !favor) {
					setProgramList(-1);
				} else if (favor) {
					setProgramList(TVProgram.TYPE_RADIO);
				}
				//LinearLayoutListView.startAnimation(lQuest2Animation); 
				break;
			case RIGHT:
				//LinearLayoutListView.startAnimation(rQuest1Animation);  
				if ((service_type == TVProgram.TYPE_TV) && !favor) {
					if (class_total > 0) {
						cur_class_no = 0;
						setTVGroupList();
					} else
						setProgramList(TVProgram.TYPE_RADIO);
				} else if ((service_type != TVProgram.TYPE_TV) && (service_type != TVProgram.TYPE_RADIO)
					&& !favor) {
					if (cur_class_no < class_total - 1) {
						cur_class_no++;
						setTVGroupList();
					} else
						setProgramList(TVProgram.TYPE_RADIO);
				} else if ((service_type == TVProgram.TYPE_RADIO) && !favor) {
					setProgramList(-1);
				} else if (favor) {
					setProgramList(TVProgram.TYPE_TV);
				}
				//LinearLayoutListView.startAnimation(rQuest2Animation);  
				break;
		}
		setFocusPosition();
	}	

	private int mYear;
	private int mMonth;
	private int mDay;
	private int mHour;
	private int mMinute; 
	private void pvr_time_init(){				
		Date date = new Date(getUTCTime());
		SimpleDateFormat sdf = new SimpleDateFormat("yyyy MM dd HH mm ss"); 
		String today = sdf.format(date); 
		Log.d(TAG,"####Today"+today);
		
		String[] para = today.split(" ", 6);
		if (para.length >= 6){
			mYear = Integer.valueOf(para[0]).intValue();
			mMonth= Integer.valueOf(para[1]).intValue()-1;
			mDay = Integer.valueOf(para[2]).intValue();
			mHour = Integer.valueOf(para[3]).intValue();
			mMinute = Integer.valueOf(para[4]).intValue();
		}

		Log.d(TAG,"mYear="+mYear+"  mMonth="+mMonth+"  mDay="+mDay+"  mHour="+mHour+" mMinute="+mMinute);

	}

	long time_test(int y,int m, int d,int h,int min,int s){	
		/*Calendar cal = new GregorianCalendar(TimeZone.getTimeZone("GMT")); 
		cal.set(y, m, d, h, min, s); 
		return cal.getTime().getTime(); 
		 */
		Calendar Calendar_sys=Calendar.getInstance();
		//Date date = new Date( y, m,  d, h, min, s);
		//Calendar_sys.setTime(date);
		Calendar_sys.set(y, m, d, h, min, s); 
	    	return Calendar_sys.getTime().getTime();
	}

	private void showPvrTimeSetDialog(Context context){
		final Context mContext = context;
		final CustomDialog mCustomDialog = new CustomDialog(mContext,R.style.MyDialog);
		mCustomDialog.showDialog(R.layout.dtv_pvr_time_set_dialog, new ICustomDialog(){
				public boolean onKeyDown(int keyCode, KeyEvent event){
					if(keyCode == KeyEvent.KEYCODE_BACK)
						mCustomDialog.dismissDialog();
					return false;
				}
				public void showWindowDetail(Window window){
					TextView title = (TextView)window.findViewById(R.id.title);
					title.setText(R.string.dtvplayer_pvr_duration_time);

					TimePicker tp;
  					DatePicker dp;	
					EditText editText;
					
					editText = (EditText)window.findViewById(R.id.edittext_time_duration);
					editText.setFilters(new  android.text.InputFilter[]{ new  android.text.InputFilter.LengthFilter(5)});

					dp=(DatePicker)window.findViewById(R.id.dPicker);
					pvr_time_init();
					dp.init(mYear, mMonth, mDay, new DatePicker.OnDateChangedListener()    {
			 	  
					public void onDateChanged(DatePicker view, int year, int monthOfYear,
						int dayOfMonth) {
							mYear=year;
							mMonth= monthOfYear;
							mDay=dayOfMonth;
						
						}
					});
		    
					tp=(TimePicker)window.findViewById(R.id.tPicker);
					tp.setIs24HourView(true);
					tp.setCurrentHour(mHour); 
					tp.setCurrentMinute(mMinute); 
					tp.setOnTimeChangedListener(new TimePicker.OnTimeChangedListener() {
						public void onTimeChanged(TimePicker view,int hourOfDay,int minute)      {
							mHour=hourOfDay;
							mMinute=minute;
						}    
					});	


					final int time_init = (int)(time_test(mYear,mMonth,mDay,mHour,mMinute,0)/1000);
					
					Button no = (Button)window.findViewById(R.id.no);
					no.setText(R.string.no);
					no.setTextColor(Color.WHITE);
					Button yes = (Button)window.findViewById(R.id.yes);
					yes.setText(R.string.yes);
					yes.setTextColor(Color.WHITE);
					no.setOnClickListener(new OnClickListener(){
						public void onClick(View v) {
							mCustomDialog.dismissDialog();
						}
					});	 
					yes.setOnClickListener(new OnClickListener(){
						public void onClick(View v) {	
							//DTVPlayerStartRecording();
							//showPvrIcon();
							mCustomDialog.dismissDialog();
						}
					});	    
				}
			}	
		);		
	}

	private  class EventAddAdapter extends BaseAdapter {
		private LayoutInflater mInflater;

		
		private Context cont;
		private String[] DATA;

	 	class ViewHolder {
			
			TextView text;
			ImageView icon;
			TextView  info;
			ImageView icon1;
		}
	
		public EventAddAdapter(Context context) {
			super();
			cont = context;
			mInflater=LayoutInflater.from(context);

			DATA = new String[4];
			//DATA[0]= cont.getResources().getString(R.string.event_start_date);			
			DATA[0]= cont.getResources().getString(R.string.event_start_time);
			DATA[1]= cont.getResources().getString(R.string.duration);		
			DATA[2]= cont.getResources().getString(R.string.repeat);			
			DATA[3]= cont.getResources().getString(R.string.mode);	
		}

		public int getCount() {	
			return DATA.length;
		}

		public Object getItem(int position) {

			return position;
		}
	
		public long getItemId(int position) {
			return position;
		}

		public View getView(int position, View convertView, ViewGroup parent) {
			ViewHolder holder=null;

			if (convertView == null) {
				convertView = mInflater.inflate(R.layout.epg_event_add_list, null);
				holder = new ViewHolder();
				holder.text = (TextView) convertView.findViewById(R.id.text);
				holder.info = (TextView)convertView.findViewById(R.id.info);
				holder.icon = (ImageView)convertView.findViewById(R.id.icon);
				holder.icon1 = (ImageView)convertView.findViewById(R.id.icon1);
				convertView.setTag(holder);
			}else {
				// Get the ViewHolder back to get fast access to the TextView
				// and the ImageView.
				holder = (ViewHolder) convertView.getTag();
			}

			Date dt_start =  new Date(getUTCTime());

			SimpleDateFormat sdf = new SimpleDateFormat("HH:mm"); 
			SimpleDateFormat sdf_date = new SimpleDateFormat("yyyy-MM-dd HH:mm"); 
			String str_start = sdf.format(dt_start); 
			String str_date  = sdf_date.format(dt_start);
	
			holder.text.setText(DATA[position]);
			holder.icon.setBackgroundResource(R.drawable.arrow_down2); 
			holder.info.setVisibility(View.VISIBLE);
			holder.icon.setVisibility(View.VISIBLE);
			holder.icon1.setVisibility(View.INVISIBLE);
			holder.info.setTextColor(Color.YELLOW);	
			switch(position){
				case 0:
					holder.info.setText(str_date);
					break;
				//case 1:
					//holder.info.setText(str_start);
					//break;
				case 1:
					//holder.info.setText("0 "+getString(R.string.dtvplayer_pvr_rec_min));
					holder.info.setText("");
					break;
				case 2:
					holder.info.setText(cont.getResources().getString(R.string.once));
					break;
				case 3:
					holder.info.setText(cont.getResources().getString(R.string.view));
					break;	
			
			} 
			
			return convertView;
		}
	}

	class BookAdd{
		long start_date;
		long start_time;
		long duration;
		int repeat;
		int mode;
	}
	
    BookAdd mBookAdd =null;
	private void showBookAddDialog(){
		if(mBookAdd==null){
			mBookAdd = new BookAdd();	
		}	
	
		mBookAdd.start_time=getUTCTime();
		mBookAdd.duration=0;
		mBookAdd.repeat=0;
		mBookAdd.mode=0;

		final Dialog mDialog = new AlertDialog(this){
			@Override
			public boolean onKeyDown(int keyCode, KeyEvent event){
				 switch (keyCode) {
					case KeyEvent.KEYCODE_BACK:	
						//if(mDialog!=null&& mDialog.isShowing()){
							dismiss();
						//}
						break;
				}
				return super.onKeyDown(keyCode, event);
			}
			
		};
		
		mDialog.setCancelable(false);
		mDialog.setCanceledOnTouchOutside(false);

		if(mDialog == null){
			return;
		}

		mDialog.setOnShowListener(new DialogInterface.OnShowListener(){
			public void onShow(DialogInterface dialog) {
				
			}         
		}); 	
		mDialog.show();
		mDialog.setContentView(R.layout.book_add_dialog);
		Window window = mDialog.getWindow();
		WindowManager.LayoutParams lp=mDialog.getWindow().getAttributes();
		
		lp.dimAmount=0.0f;
		mDialog.getWindow().setAttributes(lp);
		mDialog.getWindow().addFlags(WindowManager.LayoutParams.FLAG_DIM_BEHIND);

		Button no = (Button)window.findViewById(R.id.no);
		no.setText(R.string.no);
		no.setTextColor(Color.WHITE);
		Button yes = (Button)window.findViewById(R.id.yes);
		yes.setText(R.string.yes);
		yes.setTextColor(Color.WHITE);
		TextView title = (TextView)window.findViewById(R.id.title);
		title.setTextColor(Color.YELLOW);
		//title.setText(getString(R.string.scan_mode));
		title.setText(getString(R.string.epg_event_add));
	
		final TextView text_channel_name= (TextView) window.findViewById(R.id.text_channel_name);
		
		Date dt_start =  new Date(getUTCTime());
		
		SimpleDateFormat sdf = new SimpleDateFormat("HH:mm"); 
		SimpleDateFormat sdf_date = new SimpleDateFormat("yyyy-MMMM-dd"); 
		String str_start = sdf.format(dt_start); 
		String str_date  = sdf_date.format(dt_start);
		
		if(myAdapter!=null){
			TVProgram[] list = myAdapter.getDate();
			if(list!=null)
				text_channel_name.setText(list[cur_select_item].getName());
		}
		
		final ListView LimitListView = (ListView)window.findViewById(R.id.set_list); 	
		LimitListView.setAdapter(new EventAddAdapter(this));
		
		LimitListView.setOnItemSelectedListener(new OnItemSelectedListener() {

		public void onItemSelected(AdapterView<?> parent, View view,
			int position, long id) {
				Log.d(TAG,"sat_list setOnItemSelectedListener " + position);
				//SetLimitItemSelected = position;
			}

			public void onNothingSelected(AdapterView<?> parent) {
				Log.d(TAG,"<<sat_list onNothingSelected>> ");
			}
		});
		LimitListView.setOnItemClickListener(new AdapterView.OnItemClickListener()
        	{
			public void onItemClick(AdapterView<?> arg0, View arg1,
					int arg2, long arg3) {
				
				final TextView text =(TextView) arg1.findViewById(R.id.info);
				final ImageView icon=(ImageView)arg1.findViewById(R.id.icon);	
				final ImageView icon1=(ImageView)arg1.findViewById(R.id.icon1);
				
				// TODO Auto-generated method stub
				System.out.println("onItemSelected arg0 " + arg0);
				System.out.println("onItemSelected arg1 " + arg1);
				System.out.println("onItemSelected arg2 " + arg2);
				System.out.println("onItemSelected arg3 " + arg3);
				
				switch(arg2){
					case 0:
						showPvrDateSetDialog(text);
						break;
					//case 1:
						//showTimeDateSetDialog(text);
						//break;
					case 1:
						showEditPvrTimeDialog(text);
						break;
					case 2:  
						if(text.getText().equals(getString((R.string.once)))){
							text.setText(getString(R.string.daily));
							if(mBookAdd!=null)
								mBookAdd.repeat=1;
						}
						else if(text.getText().equals(getString((R.string.daily)))){
							text.setText(getString(R.string.weekly));
							if(mBookAdd!=null)
								mBookAdd.repeat=2;
						}
						else{
							text.setText(getString(R.string.once));
							if(mBookAdd!=null)
								mBookAdd.repeat=0;
						}
						
						break;
					case 3:
						if(text.getText().equals(getString((R.string.view)))){
							text.setText(getString(R.string.pvr));
							if(mBookAdd!=null)
								mBookAdd.mode=1;
						}
						else{
							text.setText(getString(R.string.view));
							if(mBookAdd!=null)
								mBookAdd.mode=0;
						}
						break;					
				}
			}
        	    
        });

		//no.setFocusable(true);     
     	no.setFocusableInTouchMode(true);   
		no.setOnClickListener(new OnClickListener(){
		          public void onClick(View v) {				  	 
		        	 //onSetNegativeButton();
					if(mDialog!=null&& mDialog.isShowing()){
						mDialog.dismiss();
					}
		          }});	 
		yes.setOnClickListener(new OnClickListener(){
	          public void onClick(View v) {
					if(mBookAdd!=null){
						int db_id=-1;
						if(myAdapter!=null){
							TVProgram[] list = myAdapter.getDate();
							if(list!=null)
								db_id=list[cur_select_item].getID();	
						}
						Log.d(TAG,"db_id="+db_id+",mBookAdd.mode="+mBookAdd.mode+",mBookAdd.start_time="+mBookAdd.start_time+",mBookAdd.duration="+mBookAdd.duration+",mBookAdd.repeat="+mBookAdd.repeat);
						DTVPlayerAddBook(db_id,mBookAdd.mode,mBookAdd.start_time,mBookAdd.duration,mBookAdd.repeat);
					}
					if(mDialog!=null&& mDialog.isShowing()){
						mDialog.dismiss();
					}

					DTVChannelList.this.finish();
			}});
		

		mDialog.setOnShowListener(new DialogInterface.OnShowListener(){
						public void onShow(DialogInterface dialog) {
								
							}         
							}); 	

		mDialog.setOnDismissListener(new DialogInterface.OnDismissListener(){
						public void onDismiss(DialogInterface dialog) {
							
						}         
						});	
	
	}


	private AlertDialog.Builder editBuilder=null;
	public void showPvrDateSetDialog(View v){
		final TextView text_info = (TextView)v;

		final CustomDialog mCustomDialog = new CustomDialog(DTVChannelList.this,R.style.MyDialog);
		mCustomDialog.showDialog(R.layout.date_piker_dialog, new ICustomDialog(){
			public boolean onKeyDown(int keyCode, KeyEvent event){
				if(keyCode == KeyEvent.KEYCODE_BACK)
					mCustomDialog.dismissDialog();
				return false;
			}
			public void showWindowDetail(Window window){
				TextView title = (TextView)window.findViewById(R.id.title);
				title.setText(R.string.event_start_time);

				DatePicker dp=(DatePicker)window.findViewById(R.id.dPicker);
				pvr_time_init();
				dp.init(mYear, mMonth, mDay, new DatePicker.OnDateChangedListener()    {
		 	  
				public void onDateChanged(DatePicker view, int year, int monthOfYear,
					int dayOfMonth) 
					{
						mYear=year;
						mMonth= monthOfYear;
						mDay=dayOfMonth;
					
					}
				});
				TimePicker tp=(TimePicker)window.findViewById(R.id.tPicker);		
				tp.setIs24HourView(true);
				tp.setCurrentHour(mHour); 
				tp.setCurrentMinute(mMinute); 
				tp.setOnTimeChangedListener(new TimePicker.OnTimeChangedListener() {
				  	public void onTimeChanged(TimePicker view,int hourOfDay,int minute)      {
					  	mHour=hourOfDay;
						mMinute=minute;
					  
					}    
				  });	 
				
				Button no = (Button)window.findViewById(R.id.no);
				no.setText(R.string.no);
				no.setTextColor(Color.WHITE);
				Button yes = (Button)window.findViewById(R.id.yes);
				yes.setText(R.string.yes);
				yes.setTextColor(Color.WHITE);
				no.requestFocus();
				no.setOnClickListener(new OnClickListener(){
					public void onClick(View v) {
						mCustomDialog.dismissDialog();
					}
				});	 
				yes.setOnClickListener(new OnClickListener(){
					public void onClick(View v) {	
						long now=0;	
						int end=0;

						now = time_test(mYear,mMonth,mDay,mHour,mMinute,0);
			 			Date date =  new Date(now);
				
			
						SimpleDateFormat sdf_date = new SimpleDateFormat("yyyy-MM-dd HH:mm"); 
						String str_date  = sdf_date.format(date);
					
						text_info.setText(str_date);
						if(mBookAdd!=null)
							mBookAdd.start_time=now;
						
						mCustomDialog.dismissDialog();
					}
				});	    
			}
		});

	}

	public void showTimeDateSetDialog(View v){
		final TextView text_info = (TextView)v;
		AlertDialog alert_password=null;	

		LinearLayout pvr_time_layout = null;
		if(editBuilder==null)
		editBuilder = new AlertDialog.Builder(this);

		pvr_time_layout=(LinearLayout) getLayoutInflater().inflate(R.layout.time_piker_dialog, null);

		TimePicker tp=(TimePicker)pvr_time_layout.findViewById(R.id.tPicker);
		pvr_time_init();
		
		  tp=(TimePicker)pvr_time_layout.findViewById(R.id.tPicker);
		  tp.setIs24HourView(true);
          tp.setCurrentHour(mHour); 
		  tp.setCurrentMinute(mMinute); 
		  tp.setOnTimeChangedListener(new TimePicker.OnTimeChangedListener() {
		  	public void onTimeChanged(TimePicker view,int hourOfDay,int minute)      {
		  	mHour=hourOfDay;
			mMinute=minute;
			  
			}    
		  });	
			
		  final int time_init = (int)(time_test(mYear,mMonth,mDay,mHour,mMinute,0)/1000);
		  
		  //editBuilder.setTitle(R.string.pvr_set);
			editBuilder.setView(pvr_time_layout); 

			editBuilder.setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {

				public void onClick(DialogInterface dialog, int which) {

				int now=0;	

				now = (int)(time_test(mYear,mMonth,mDay,mHour,mMinute,0)/1000);
				Date date =  new Date((long)now*1000);


				SimpleDateFormat sdf = new SimpleDateFormat("HH:mm"); 
				String str_date  = sdf.format(date);

				text_info.setText(str_date);
				if(mBookAdd!=null)
					mBookAdd.start_time=now;

			}
		});

		editBuilder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int id) {
				
			}        
		}); 
		 
		  alert_password = editBuilder.create();
		  alert_password.setOnShowListener(new DialogInterface.OnShowListener(){
								public void onShow(DialogInterface dialog) {

									}         
									}); 	

		   alert_password.setOnDismissListener(new DialogInterface.OnDismissListener(){
								public void onDismiss(DialogInterface dialog) {
								
								}         
								});	
		  
		  alert_password.show();
		  WindowManager m = getWindowManager();   
		  Display d = m.getDefaultDisplay();  	
		 	
		  WindowManager.LayoutParams lp=alert_password.getWindow().getAttributes();
		  lp.dimAmount=0.0f;
		  alert_password.getWindow().setAttributes(lp);
		  alert_password.getWindow().addFlags(WindowManager.LayoutParams.FLAG_DIM_BEHIND);

	}

	private AlertDialog.Builder builder;
	Toast toast;
	public void showEditPvrTimeDialog(View v){
		builder = new AlertDialog.Builder(this);
		
		final EditText editText = new EditText(this);
		editText.setFilters(new  android.text.InputFilter[]{ new  android.text.InputFilter.LengthFilter(4)});
		editText.setInputType(InputType.TYPE_CLASS_NUMBER);
		builder.setTitle(R.string.edit_title);
		final TextView text_time =(TextView) v;
		editText.setText("");
		builder.setView(editText); 

		AlertDialog alert = builder.create();

		alert.setOnKeyListener( new DialogInterface.OnKeyListener(){
			public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
				// TODO Auto-generated method stub
				switch(keyCode)
				{	
					case KeyEvent.KEYCODE_DPAD_CENTER:
					case KeyEvent.KEYCODE_ENTER:
						String time = editText.getText().toString();
						if(time==null||time.equals("")){
							editText.setText(null);
							/*
							toast = Toast.makeText(DTVChannelList.this, 
						    	R.string.invalid_input,
						    	Toast.LENGTH_SHORT);
							toast.setGravity(Gravity.CENTER, 0, 0);
							toast.show();
							*/
							if(mBookAdd!=null)
									mBookAdd.duration=0;	
							text_time.setText("");
						}
						else{
							if(Integer.parseInt(time)==0){
								editText.setText(null);
								
								toast = Toast.makeText(DTVChannelList.this, 
							    	R.string.invalid_input,
							    	Toast.LENGTH_SHORT);
								toast.setGravity(Gravity.CENTER, 0, 0);
								toast.show();
								
								mBookAdd.duration=0;
								text_time.setText("");
							}
							else{
								text_time.setText(editText.getText().toString()+getString(R.string.dtvplayer_pvr_rec_min));
								if(mBookAdd!=null)
									mBookAdd.duration=Long.valueOf(editText.getText().toString())*60*1000;	
								dialog.cancel();
							}	
						}
						dialog.cancel();
						return true;
					case  KeyEvent.KEYCODE_BACK:
						dialog.cancel();
						return true;
				}
				
				return false;
			}
		});	

		
		alert.setOnShowListener(new DialogInterface.OnShowListener(){
						public void onShow(DialogInterface dialog) {
			
							}         
							}); 	

		alert.setOnDismissListener(new DialogInterface.OnDismissListener(){
						public void onDismiss(DialogInterface dialog) {
						}
						});
		alert.show();
		//alert.getWindow().setLayout(500, -200);
		//alert.getWindow().setLayout(500, 200);
		WindowManager.LayoutParams lp=alert.getWindow().getAttributes();
		lp.dimAmount=0.0f;
		lp.x = 0;
		lp.y = -500 ;
		lp.width = 500;
		lp.height = 200 ;
		alert.getWindow().setAttributes(lp);
		alert.getWindow().addFlags(WindowManager.LayoutParams.FLAG_DIM_BEHIND);
	}

	
	private static class mySatListAdapter extends BaseAdapter {
		private LayoutInflater mInflater;
		private Context cont;
		private List<String> listItems;
		private int selectItem;
		
		static class ViewHolder {
			TextView text;	
			ImageView icon;
		}
		
		public mySatListAdapter(Context context, List<String> list) {
			super();
			cont = context;
			listItems = list;
			mInflater=LayoutInflater.from(context);			  
		}

		public int getCount() {
			if(list_sat==null)
				return 0;
			else	
				return list_sat.length;
		}

		public Object getItem(int position) {
		
			return position;
		}
		
		public long getItemId(int position) {
			return position;
		}

		public void setSelectItem(int position)
		{
			this.selectItem = position;
		}

	        public int getSelectItem()
	        {
	       	 return this.selectItem;
	        }
		
		
		public View getView(int position, View convertView, ViewGroup parent) {
		ViewHolder holder;	
		if (convertView == null) {
		   
		   convertView = mInflater.inflate(R.layout.listitem, null);
			
		   holder = new ViewHolder();
		   holder.text = (TextView) convertView.findViewById(R.id.ItemText);
		   holder.icon = (ImageView) convertView.findViewById(R.id.icon);
		   convertView.setTag(holder);
		}else {
		  // Get the ViewHolder back to get fast access to the TextView
		  // and the ImageView.
		  holder = (ViewHolder) convertView.getTag();
		  }
		
		  // Bind the data efficiently with the holder.
		  if(list_sat!=null)
		  	holder.text.setText(list_sat[position].getSatelliteName());
		  
		  return convertView;
		}
	  }
	
	private AlertDialog.Builder diaBuilder;
	private void showSatellitesList(){

		getSatellitesListData();

		final Dialog mDialog = new AlertDialog(this){
			@Override
			public boolean onKeyDown(int keyCode, KeyEvent event){
				 switch (keyCode) {
					case KeyEvent.KEYCODE_BACK:	
						dismiss();
						break;
				}
				return super.onKeyDown(keyCode, event);
			}
			
		};
		
		mDialog.setCancelable(false);
		mDialog.setCanceledOnTouchOutside(false);

		if(mDialog == null){
			return;
		}

		mDialog.show();
		mDialog.setContentView(R.layout.dvbs_show_sat_dia);

		Window window = mDialog.getWindow();
		window.setGravity(Gravity.CENTER);
		WindowManager.LayoutParams lp=mDialog.getWindow().getAttributes();
		//WindowManager m = getWindowManager();
		//Display d = m.getDefaultDisplay();
		lp.dimAmount=0.0f;
		lp.x=600;	

		mDialog.getWindow().setAttributes(lp);
		mDialog.getWindow().addFlags(WindowManager.LayoutParams.FLAG_DIM_BEHIND);

		TextView title = (TextView)window.findViewById(R.id.title);
		title.setTextColor(Color.YELLOW);
		title.setText(getString(R.string.satellites_info_list));
		
		ListView LimitListView =(ListView)window.findViewById(R.id.set_list);
		
		LimitListView.setOnItemClickListener(new AdapterView.OnItemClickListener(){

			public void onItemClick(AdapterView<?> arg0, View arg1,
					int arg2, long arg3) {
				// TODO Auto-generated method stub
				System.out.println("onItemSelected arg0 " + arg0);
				System.out.println("onItemSelected arg1 " + arg1);
				System.out.println("onItemSelected arg2 " + arg2);
				System.out.println("onItemSelected arg3 " + arg3);
				Log.d(TAG,"id=="+list_sat[arg2].getSatelliteId());
				getProgBySatIdAndType(list_sat[arg2].getSatelliteId(),getCurrentProgramType());
				Text_title.setText(list_sat[arg2].getSatelliteName());
				myAdapter.notifyDataSetChanged();
				mDialog.dismiss();

			}
        	    
        });
		LimitListView.setAdapter(new mySatListAdapter(this,null));		
	}


	private static TVSatellite[] list_sat=null;
	private  void  getSatellitesListData(){
	 	list_sat = TVSatellite.tvSatelliteList(this);
	}
	
	private void getProgBySatIdAndType(int sat_id,int type){
		switch(type){
			case TVProgram.TYPE_TV:
				mTVProgramList=TVProgram.selectBySatIDAndType(this,sat_id,type);
				break;
			case TVProgram.TYPE_RADIO:
				mRadioProgramList=TVProgram.selectBySatIDAndType(this,sat_id,type);
				break;
		}
	}
	
	private void showProgramSearchDialog(){
		final Dialog mDialog = new Dialog(this,R.style.MyDialog){
			@Override
			public boolean onKeyDown(int keyCode, KeyEvent event){
				 switch (keyCode) {
					case KeyEvent.KEYCODE_BACK:	
						dismiss();
						
						break;
				}
				return super.onKeyDown(keyCode, event);
			}
			
		};
		
		mDialog.setCancelable(false);
		mDialog.setCanceledOnTouchOutside(false);

		if(mDialog == null){
			return;
		}

		mDialog.setOnShowListener(new DialogInterface.OnShowListener(){
			public void onShow(DialogInterface dialog) {
				
			}         
		}); 	
		mDialog.show();
		mDialog.setContentView(R.layout.program_search_dialog);
		Window window = mDialog.getWindow();
		WindowManager.LayoutParams lp=mDialog.getWindow().getAttributes();
		
		lp.dimAmount=0.0f;
		//lp.width = (int) (mDialog.getWidth() * 0.50);
		lp.x=600;
		lp.y=-250;
		mDialog.getWindow().setAttributes(lp);
		mDialog.getWindow().addFlags(WindowManager.LayoutParams.FLAG_DIM_BEHIND);

		Button no = (Button)window.findViewById(R.id.no);
		no.setText(R.string.no);
		no.setTextColor(Color.WHITE);
		Button yes = (Button)window.findViewById(R.id.yes);
		yes.setText(R.string.yes);
		yes.setTextColor(Color.WHITE);
		TextView title = (TextView)window.findViewById(R.id.title);
		title.setTextColor(Color.YELLOW);
		title.setText(getString(R.string.find));

      		EditText editText = (EditText)window.findViewById(R.id.edittext_name);

		editText.addTextChangedListener(new TextWatcher() {
		   @Override
		   public void onTextChanged(CharSequence s, int start, int before, int count) {
				// TODO Auto-generated method stub	
				getListDataByStringKeyAndType(s,getCurrentProgramType());
				//Title.setText(R.string.search_program);
				if(myAdapter!=null){
					myAdapter.fillData(mSearchProgramList);
		 			myAdapter.notifyDataSetChanged();
				}
		   }
		   
		   @Override
		   public void beforeTextChanged(CharSequence s, int start, int count,int after) {
				// TODO Auto-generated method stub
		    
		   }
		   
		   @Override
		   public void afterTextChanged(Editable s) {
				// TODO Auto-generated method stub
		    
		   }
		});
		
		no.setFocusable(true);   
     	//no.requestFocus();   
     	no.setFocusableInTouchMode(true);   
		no.setOnClickListener(new OnClickListener(){
		          public void onClick(View v) {				  	 
					//onSetNegativeButton();
					if (service_type != TVProgram.TYPE_RADIO)
						service_type = TVProgram.TYPE_TV;
					getListData(service_type);
					myAdapter.notifyDataSetChanged();
					if(mDialog!=null&& mDialog.isShowing()){
						mDialog.dismiss();
					}
		          }});	 
		yes.setOnClickListener(new OnClickListener(){
	          public void onClick(View v) {
					//myAdapter.notifyDataSetChanged();
					if(mDialog!=null&& mDialog.isShowing()){
						mDialog.dismiss();
					}
			}});
		
		mDialog.setOnShowListener(new DialogInterface.OnShowListener(){
						public void onShow(DialogInterface dialog) {
							
							}         
							}); 	

		mDialog.setOnDismissListener(new DialogInterface.OnDismissListener(){
						public void onDismiss(DialogInterface dialog) {
							
						}         
						});	


	}

	private void getListDataByStringKeyAndType(CharSequence key,int type){
		String pro_name = key.toString();
		Log.d(TAG,"program="+pro_name);
		
		mSearchProgramList=TVProgram.selectByNameAndType(this,pro_name,type);
	}

	public class MyAsyncTask extends AsyncTask<Integer, Integer, String>{ 
		private volatile boolean running = true;
		private int cmd = 0;
		@Override
		protected void onCancelled() {
			running = false;
    		}	
		
		@Override
		protected void onPreExecute() {  
			super.onPreExecute();  
			ProgressBar_channel.setVisibility(View.VISIBLE);		
		}  
	          
	        @Override
	        protected String doInBackground(Integer... params) {  
			while(running){
				cmd = params[0];
				switch(cmd){
					case TVProgram.TYPE_TV:
						getListData(TVProgram.TYPE_TV);
						break;
					case TVProgram.TYPE_RADIO:
						getListData(TVProgram.TYPE_RADIO);
						break;
					case -1:
						getListData(-1);
						break;	
				}
				break;
			}	
			return null;  
	        }  
	  
	        @Override
	        protected void onProgressUpdate(Integer... progress) { 
			super.onProgressUpdate(progress);  
	        }  
	  
	        @Override
	        protected void onPostExecute(String result) {  
			super.onPostExecute(result);  
			ProgressBar_channel.setVisibility(View.INVISIBLE);	
			if(myAdapter!=null){
				switch(cmd){
					case TVProgram.TYPE_TV:
						myAdapter.fillData(mTVProgramList);
						break;
					case TVProgram.TYPE_RADIO:
						myAdapter.fillData(mRadioProgramList);
						break;
					case -1:
						myAdapter.fillData(mFavProgramList);
						break;	
				}
				
				myAdapter.notifyDataSetChanged();
			}
			setFocusPosition();
	        }  
	  
	}  

}

