package com.amlogic.DTVPlayer;

import android.util.Log;
import android.os.Bundle;
import com.amlogic.tvutil.TVMessage;
import com.amlogic.tvutil.TVConst;
import com.amlogic.tvutil.TVProgram;
import com.amlogic.tvutil.TVProgramNumber;
import com.amlogic.tvactivity.TVActivity;
import com.amlogic.tvutil.TVChannelParams;
import com.amlogic.tvutil.TVScanParams;
import com.amlogic.tvutil.TVConst;
import com.amlogic.tvutil.TVGroup;

import java.util.*;
import java.text.*;
import android.view.*;
import android.view.View.*;
import android.view.animation.*;
import android.widget.*;
import android.app.*;
import android.app.AlertDialog.*;
import android.content.*;
import android.graphics.*;
import android.view.ViewGroup.*;
import android.text.*;
import android.text.method.*;
import android.database.*;
import android.os.*;
import android.widget.AdapterView.OnItemLongClickListener;
import android.view.View.OnLongClickListener;
import android.widget.AbsListView.OnScrollListener;
import java.lang.reflect.Field;

import com.amlogic.widget.PasswordDialog;
import com.amlogic.widget.SureDialog;
import com.amlogic.widget.SingleChoiseDialog;
import com.amlogic.widget.MutipleChoiseDialog;
import com.amlogic.widget.CustomDialog;
import com.amlogic.widget.CustomDialog.ICustomDialog;

public class DTVProgramManager extends DTVActivity{
	private static final String TAG="DTVProgramManager";
 
	private DTVSettings mDTVSettings=null;
	private TextView mTextview=null;
	ListView ListView_programmanager=null;
	TextView Text_title=null;
	private int cur_select_item=0;
	private IconAdapter myAdapter=null;
	private TVProgram[]  mTVProgramList=null;

	int db_id=-1;
	private int service_type=TVProgram.TYPE_TV;
	private int TVProgramCurrentId = -1;
	private int TabIndex = TVProgramCurrentId;

	private Button mButtonTv=null;
	private Button mButtonRadio=null;
	private Button mButtonFav=null;

	private Button getGroupButtonById(int id){
		Button temp=null;
		if(id==-1)
			temp=mButtonTv;
		else if(id==-2)
			temp=mButtonRadio;
		else if(id==-3)
			temp=mButtonFav;
		else {
			LinearLayout mLinearLayout = (LinearLayout)findViewById(R.id.LinearLayoutGroupButton) ;
			temp = (Button) mLinearLayout.findViewById(id);
		}
		return temp;
	}

	private int getProgramCurrentIndex(){
		if(TVProgramCurrentId!=-1){
			for(int i=0;i<mProgramGroup.length;i++){
				if(TVProgramCurrentId==mProgramGroup[i].getID())
					return i;
			}
		}

		return -1;
	}	
	private void getListData(int type){
		if(type==0)
			mTVProgramList = TVProgram.selectByType(this,TVProgram.TYPE_TV,true);
		else if(type==1)
			mTVProgramList = TVProgram.selectByType(this,TVProgram.TYPE_RADIO,true);
	}

	private void getListFavorite(){
		mTVProgramList=DTVProgramManagerGetFav();
	}

	private void getListGroupById(int id){
		mTVProgramList=DTVProgramManagerGetProByGroup(id);
	}

	private void deleteCurrentGroup(){
		if(TVProgramCurrentId!=-1)
			DTVProgramManagerDeleteGroup(TVProgramCurrentId);
	}

	private void editCurrentGroupName(String name){
		if(TVProgramCurrentId!=-1)
			DTVProgramManagerEditGroupName(TVProgramCurrentId,name);
	}

	private void dealFav(int pos){
		if(mTVProgramList[pos].getFavoriteFlag()){
			mTVProgramList[pos].setFavoriteFlag(false);
			if(TabIndex==-3){
				mTVProgramList=removeProgramFromList(mTVProgramList,pos);	
			}
		}
		else
			mTVProgramList[pos].setFavoriteFlag(true);
	}

	private void dealLock(int pos){
		if(mTVProgramList[pos].getLockFlag())
			mTVProgramList[pos].setLockFlag(false);
		else
			mTVProgramList[pos].setLockFlag(true);
	}

	private void deleteProgramFromDB(int index){
		mTVProgramList[index].deleteFromDb();
		mTVProgramList = removeProgramFromList(mTVProgramList,index);
	}

	private TVProgram[] removeProgramFromList(TVProgram[] a,int index){
	    int len=a.length;
	    if(index<0||index>=len){
	        return a;
	    }
	    TVProgram[] result=new TVProgram[len-1];
	    System.arraycopy(a,0,result,0,index);
	    System.arraycopy(a,index+1,result,index,len-index-1);
	    return result;
	}

	private void addIntoGroup(int pos,int group_id){
		mTVProgramList[pos].addProgramToGroup(group_id);
	}	

	private void deleteProgramFromGroup(int pos,int group_id){
		mTVProgramList[pos].deleteFromGroup(group_id);	
	}


	private TVGroup[] mProgramGroup=null;
	private int TVGroupCount = 0;
	private void DTVProgramManagerGroupButtonData(){
		TVGroup[] group = DTVProgramManagerGetGroupList();
		int len = 0;
		if(group!=null){
			len = group.length;
		}

		TVGroupCount = len+3;
		mProgramGroup = new TVGroup[TVGroupCount];
		for(int m=0;m<TVGroupCount;m++){
			mProgramGroup[m]=new TVGroup();
		}
		mProgramGroup[0].setID(-1);
		mProgramGroup[0].setName(getString(R.string.tv));
		mProgramGroup[1].setID(-1);
		mProgramGroup[1].setName(getString(R.string.radio));
		mProgramGroup[2].setID(-1);
		mProgramGroup[2].setName(getString(R.string.favorite));
		for(int i=0;i<len;i++){
			mProgramGroup[i+3] = group[i];
		}

		for(int j=0;j<mProgramGroup.length;j++){
			Log.d(TAG,"mProgramGroup="+mProgramGroup[j].getName());
		}

	}

	
	private void DTVProgramManagerUIInit(){
		DTVProgramManagerGroupButtonData();
		//init list data
		getListFavorite();
		mTextview = (TextView) findViewById(R.id.ProgramManagerDescription);
		ListView_programmanager = (ListView) findViewById(R.id.list_content);
		myAdapter = new IconAdapter(DTVProgramManager.this,null);
		ListView_programmanager.setOnItemSelectedListener(mOnSelectedListener);
		ListView_programmanager.setOnKeyListener(new listOnKeyListener());
		ListView_programmanager.setOnScrollListener(new listOnScroll()); 
		ListView_programmanager.setOnItemClickListener(mOnItemClickListener);
		ListView_programmanager.setOnItemLongClickListener(new OnItemLongClickListener() {
			@Override
			public boolean onItemLongClick(AdapterView<?> parent, View view,int position, long id) {
				Log.d(TAG,"long Click");
				createMenuChoiceDialog(DTVProgramManager.this ,position);
				return false;
			}
		});

		ListView_programmanager.setAdapter(myAdapter);
		create_group_button();

		if(mButtonTv!=null)	
			mButtonTv.requestFocus();
	}

	public void onCreate(Bundle savedInstanceState){
		Log.d(TAG, "onCreate");
		super.onCreate(savedInstanceState);
		setContentView(R.layout.dtvprogrammanager); 
		/*get list data*/
		
	}

	public void onConnected(){
		Log.d(TAG, "connected");
		super.onConnected();		
		mDTVSettings = new DTVSettings(this);
		DTVProgramManagerUIInit();
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
			ListView_programmanager = (ListView) findViewById(R.id.list_content);
			if(ListView_programmanager.hasFocus() == true){
			}
			cur_select_item = position;
		}
		public void onNothingSelected(AdapterView<?> parent){
		}
	};

	private boolean needUpdate = false;
	private boolean move_mode=false;
	private int moveItemPos = -1;
	private int cur_pos = -1;
	private int temp_pos = 0;
	private AdapterView.OnItemClickListener mOnItemClickListener =new AdapterView.OnItemClickListener(){
		public void onItemClick(AdapterView<?> parent, View v, int position, long id){
				int db_id=mTVProgramList[position].getID();
				Log.d(TAG,"mOnItemClickListener pos="+position);

		}
	};

	public void setMoveMode(boolean mode){
		this.move_mode = mode;
	}

	public boolean getMoveMode(){
		return this.move_mode;
	}

	public void setMoveItemPos(int pos){
		this.moveItemPos = pos;
	}

	public int getMoveItemPos(){
		return this.moveItemPos;
	}

	public void exchageItem(int first, int second){
		Log.d(TAG,"cur_pos="+first+"-----"+"temp_pos="+second);
		TVProgram mTemp = mTVProgramList[second];
		mTVProgramList[second]=mTVProgramList[first];
		mTVProgramList[first]=mTemp;
	}

	class listOnKeyListener implements OnKeyListener{
		public boolean onKey(View arg0, int arg1, KeyEvent arg2) {
			// TODO Auto-generated method stub
			Log.d(TAG, "enter key press");

			if (arg2.getAction() == KeyEvent.ACTION_DOWN){
				switch(arg1)
				{
					case KeyEvent.KEYCODE_DPAD_CENTER:
					case KeyEvent.KEYCODE_ENTER:
						if (getMoveMode()){	
							Log.d(TAG,"switch itme " + getMoveItemPos() + "   "+myAdapter.getSelectItem());
							//saveChange();
							setMoveItemPos(-1);
							setMoveMode(false);
							myAdapter.notifyDataSetChanged();
						}
						break;
					case KeyEvent.KEYCODE_DPAD_UP:
						if (getMoveMode() && getMoveItemPos() > 0){
							needUpdate = false;
							exchageItem(getMoveItemPos(), getMoveItemPos() - 1);
							setMoveItemPos(getMoveItemPos() - 1);
							myAdapter.setSelectItem(getMoveItemPos());
							myAdapter.notifyDataSetChanged();
							Log.d(TAG, "press up");
						}
						else if (!getMoveMode() && myAdapter.getSelectItem()== 0){
							needUpdate = true;
							Log.d(TAG, "press up to last item");
						}
						else{
					 		needUpdate = false;
					 	}
						break;
					case KeyEvent.KEYCODE_DPAD_DOWN:
						if (getMoveMode() && getMoveItemPos() < (mTVProgramList.length - 1))
						{
							needUpdate = false;
							exchageItem(getMoveItemPos(), getMoveItemPos() + 1);
							setMoveItemPos(getMoveItemPos() + 1);
							myAdapter.setSelectItem(getMoveItemPos());
							myAdapter.notifyDataSetChanged();
							Log.d(TAG, "press down");
						}
						else if (!getMoveMode() && myAdapter.getSelectItem() == (mTVProgramList.length - 1))
						{
							needUpdate = true;
							Log.d(TAG, "press down to first item");
						}
						 else
					 	{
					 		needUpdate = false;
					 	}
						break;
				}
			}

			if (arg2.getAction() == KeyEvent.ACTION_UP){
				switch(arg1)
				{
					case KeyEvent.KEYCODE_ZOOM_IN:
						if (getMoveMode()){
							for (int i = getMoveItemPos(); i > myAdapter.getSelectItem(); i--){
								exchageItem(i, i - 1);
							}
							setMoveItemPos(myAdapter.getSelectItem());
							myAdapter.setSelectItem(myAdapter.getSelectItem());
							myAdapter.notifyDataSetChanged();
							Log.d(TAG, "press page up");
						}
						break;
					case KeyEvent.KEYCODE_ZOOM_OUT:
						if (getMoveMode())
						{
							for (int i = getMoveItemPos(); i < myAdapter.getSelectItem(); i++)
							{
								exchageItem(i, i + 1);
							}
							setMoveItemPos(myAdapter.getSelectItem());
							myAdapter.setSelectItem(myAdapter.getSelectItem());
							myAdapter.notifyDataSetChanged();
							Log.d(TAG, "press page down");
						}
						break;
						/*
					case KeyEvent.KEYCODE_DPAD_UP:
						if (needUpdate)
						{
							needUpdate = false;
							myAdapter.setSelectItem(mTVProgramList.length - 1);
							ListView_programmanager.setSelection(mTVProgramList.length - 1);
							myAdapter.notifyDataSetChanged();
							Log.d(TAG, "press up to last item");
						}
						 break;
					case KeyEvent.KEYCODE_DPAD_DOWN:
						if (needUpdate)
						{
							needUpdate = false;
							myAdapter.setSelectItem(0);
							ListView_programmanager.setSelection(0);
							myAdapter.notifyDataSetChanged();
 							Log.d(TAG, "###ss up to last item");
						}
						break;
						*/
				   }
			}
        	      	
			return false;
			
		}
    	 
    }

	private void DTVListDealLeftAndRightKey(int mode){
		switch(mode){
			case 0://left
			case 1://right
				Button mButton=getGroupButtonById(TabIndex);
				if(mButton!=null)
					mButton.requestFocus();
				break;
		}
		
	}

	private void DTVListDealUpAndDownKey(int mode){
		switch(mode){
			case 0://up
				
				cur_pos=temp_pos;
				myAdapter.notifyDataSetChanged();
				break;
			case 1://down

				break;
		}
	}
	
	private class IconAdapter extends BaseAdapter {
		private LayoutInflater mInflater;
		private Context cont;
		private List<String> listItems;
		private int selectItem;
		
		class ViewHolder {
			ImageView icon_move;
			TextView prono;
			TextView text;	
			ImageView icon_scrambled;
			ImageView icon_fav;
			ImageView icon;
		}
		
		public IconAdapter(Context context, List<String> list) {
			super();
			cont = context;
			mInflater=LayoutInflater.from(context);			  
		}

		public int getCount() {
			if(mTVProgramList==null)
				return 0;
			else
				return mTVProgramList.length;
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
				convertView = mInflater.inflate(R.layout.dtv_programmanager_list_item, null);
				
				holder = new ViewHolder();
				holder.icon_move = (ImageView)convertView.findViewById(R.id.icon_move);
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
			
			String region = mDTVSettings.getScanRegion();
			if(region.contains("ATSC")){
				holder.prono.setText(Integer.toString(mTVProgramList[position].getNumber().getMajor())+"-"+Integer.toString(mTVProgramList[position].getNumber().getMinor()));
			}
			else
				holder.prono.setText(Integer.toString(mTVProgramList[position].getNumber().getNumber()));

			holder.text.setText(mTVProgramList[position].getName());
			if(db_id == mTVProgramList[position].getID()){  
				//convertView.setBackgroundColor(Color.RED);  
				holder.text.setTextColor(Color.YELLOW);
			}	
			else{
				//convertView.setBackgroundColor(Color.TRANSPARENT); 
				holder.text.setTextColor(Color.WHITE);
			}	
		
			if(mTVProgramList[position].getLockFlag()){
				holder.icon.setBackgroundResource(R.drawable.dtvplayer_icon_lock); 
			}	
			else{
				holder.icon.setBackgroundResource(Color.TRANSPARENT);
			}

			if(mTVProgramList[position].getFavoriteFlag()){
				holder.icon_fav.setBackgroundResource(R.drawable.dtvplayer_icon_fav); 
			}	
			else{
				holder.icon_fav.setBackgroundResource(Color.TRANSPARENT);
			}	

			if(mTVProgramList[position].getScrambledFlag()){
				holder.icon_scrambled.setBackgroundResource(R.drawable.dtvplayer_icon_scrambled); 
			}	
			else{
				holder.icon_scrambled.setBackgroundResource(Color.TRANSPARENT);
			}

			if(getMoveMode()&&position==getMoveItemPos()){
				holder.icon_move.setBackgroundResource(R.drawable.dtv_programmanager_movecursor); 
			}	
			else{
				holder.icon_move.setBackgroundResource(Color.TRANSPARENT);
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
		if(!connected){
			return true;
			}
		switch (keyCode) {
			case KeyEvent.KEYCODE_DPAD_LEFT:
				DTVListDealLeftAndRightKey(0);
				break;		
			case KeyEvent.KEYCODE_DPAD_RIGHT:
				DTVListDealLeftAndRightKey(1);
				break;
			case KeyEvent.KEYCODE_DPAD_DOWN:
				if(cur_select_item == ListView_programmanager.getCount()-1){
					ListView_programmanager.setSelection(0); 	
					return true;
				}	
				break;
			case KeyEvent.KEYCODE_DPAD_UP:		
				if(cur_select_item== 0){
					Button mButton=getGroupButtonById(TabIndex);
					if(mButton!=null)
						mButton.requestFocus();
				}
				break;
			case KeyEvent.KEYCODE_BACK:
				if(move_mode)
					move_mode=false;
				break;
		}
		return super.onKeyDown(keyCode, event);
	}	  
	
	private boolean [] b=null;
	public void programGroupOperate(int pos){
		final int p = pos;
		
		final TVGroup[] group = DTVProgramManagerGetGroupList();
		if(group==null)
			return;
		if(group.length<=0)
			return;

		Log.d(TAG,"group.length="+group.length);	 
		String[] items = new String[group.length];        	 
		b = new boolean[group.length];

		for (int j = 0; j < group.length; j++){
			items[j] = group[j].getName();
			b[j]= mTVProgramList[pos].checkGroup(group[j].getID());
			Log.d(TAG,">>>"+j+"item="+items[j]+"----"+b[j]);
		}
		
		new MutipleChoiseDialog(DTVProgramManager.this,items,b,0){
			public void onSetMessage(View v){
				((TextView)v).setText(getString(R.string.add));
			}
			public void onSetNegativeButton(){
				
			}
			public void onSetPositiveButton(int which,boolean[] b){
				if(b!=null)
				for(int index = 0;index < group.length;index++){
					Log.d(TAG,"position: "+index+"is "+ b[index]);
					if(b[index])
						addIntoGroup(p,group[index].getID());
				}
			}
		};
	}
	
	void createMenuChoiceDialogForGroup(Context context){
		final Context mContext = context;
		final String[] itemChoices = {
			getString(R.string.add),
			getString(R.string.edit),
			getString(R.string.delete)
		};

		final CustomDialog mCustomDialog = new CustomDialog(mContext);
		mCustomDialog.showDialog(R.layout.list_menu, new ICustomDialog(){
				public boolean onKeyDown(int keyCode, KeyEvent event){
					if(keyCode == KeyEvent.KEYCODE_BACK)
						mCustomDialog.dismissDialog();
					return false;
				}
				public void showWindowDetail(Window window){
					TextView title = (TextView)window.findViewById(R.id.title);
					title.setTextColor(Color.YELLOW);
					title.setText("Group Operations");
					
					ListView list_item = (ListView)window.findViewById(R.id.list_item);
					ArrayAdapter<String> adapter = new ArrayAdapter<String>(mContext,R.layout.menu_list_item,itemChoices);
					list_item.setAdapter(adapter);  
					list_item.setOnItemClickListener(new AdapterView.OnItemClickListener(){
						public void onItemClick(AdapterView<?> parent, View v, int position, long id){			
							switch(position){
								case 0: //add
									final CustomDialog mAddCustomDialog = new CustomDialog(mContext);
										mAddCustomDialog.showDialog(R.layout.edit_dialog, new ICustomDialog(){
											public boolean onKeyDown(int keyCode, KeyEvent event){
												if(keyCode == KeyEvent.KEYCODE_BACK)
													mAddCustomDialog.dismissDialog();
												return false;
											}
											public void showWindowDetail(Window window){
												TextView title = (TextView)window.findViewById(R.id.title);
												title.setText(R.string.add);
												final EditText mAddText = (EditText)window.findViewById(R.id.edit);
												mAddText.setText(null);
												Button no = (Button)window.findViewById(R.id.no);
												no.setText(R.string.no);
												no.setTextColor(Color.WHITE);
												Button yes = (Button)window.findViewById(R.id.yes);
												yes.setText(R.string.yes);
												yes.setTextColor(Color.WHITE);
												no.setOnClickListener(new OnClickListener(){
													public void onClick(View v) {
														mAddCustomDialog.dismissDialog();
													}
												});	 
												yes.setOnClickListener(new OnClickListener(){
													public void onClick(View v) {	
														DTVProgramManagerAddGroup(mAddText.getText().toString());
														DTVProgramManagerGroupButtonData();
														refreshGroupButton();
														mAddCustomDialog.dismissDialog();
													}
												});	    
											}
										});
									break;
								case 1: //edit
									if(TVProgramCurrentId!=-1){
										final CustomDialog mEditCustomDialog = new CustomDialog(mContext);
										mEditCustomDialog.showDialog(R.layout.edit_dialog, new ICustomDialog(){
											public boolean onKeyDown(int keyCode, KeyEvent event){
												if(keyCode == KeyEvent.KEYCODE_BACK)
													mEditCustomDialog.dismissDialog();
												return false;
											}
											public void showWindowDetail(Window window){
												TextView title = (TextView)window.findViewById(R.id.title);
												title.setText(R.string.edit);
												final EditText mEditText = (EditText)window.findViewById(R.id.edit);
												mEditText.setText(mProgramGroup[getProgramCurrentIndex()].getName());
												Button no = (Button)window.findViewById(R.id.no);
												no.setText(R.string.no);
												no.setTextColor(Color.WHITE);
												Button yes = (Button)window.findViewById(R.id.yes);
												yes.setText(R.string.yes);
												yes.setTextColor(Color.WHITE);
												no.setOnClickListener(new OnClickListener(){
													public void onClick(View v) {
														mEditCustomDialog.dismissDialog();
													}
												});	 
												yes.setOnClickListener(new OnClickListener(){
													public void onClick(View v) {	
														editCurrentGroupName(mEditText.getText().toString());
														DTVProgramManagerGroupButtonData();
														mEditCustomDialog.dismissDialog();
													}
												});	    
											}
										});
									}	
									break;
								case 2: //delete
									deleteCurrentGroup();
									DTVProgramManagerGroupButtonData();
									refreshGroupButton();
									break;
							}
						}
					}	
					);
				}
			}	
		);		
	}

	void createMenuChoiceDialog(Context context, int position){
		final Context mContext = context;
		final int pos = position;
		boolean fav = false; 
		boolean lock = false;
		boolean skip = false;

		if(mTVProgramList!=null){
			fav = mTVProgramList[position].getFavoriteFlag();
			lock = mTVProgramList[position].getLockFlag();
			//skip = mTVProgramList[pos].get
		}
		
		final String[] itemChoices = {
			getString(R.string.edit),
			getString(R.string.delete),
			(fav==false)?getString(R.string.add_fav):getString(R.string.del_fav),
			//(skip==false)?getString(R.string.add_skip):getString(R.string.del_skip),
			(lock==false)?getString(R.string.add_lock):getString(R.string.del_lock),		
			getString(R.string.move),
			getString(R.string.add_into_group)
		};

		final CustomDialog mCustomDialog = new CustomDialog(mContext);
		mCustomDialog.showDialog(R.layout.list_menu, new ICustomDialog(){
				public boolean onKeyDown(int keyCode, KeyEvent event){
					if(keyCode == KeyEvent.KEYCODE_BACK)
						mCustomDialog.dismissDialog();
					return false;
				}
				public void showWindowDetail(Window window){
					TextView title = (TextView)window.findViewById(R.id.title);
					title.setTextColor(Color.YELLOW);
					title.setText(R.string.program_operation);
					
					ListView list_item = (ListView)window.findViewById(R.id.list_item);
					ArrayAdapter<String> adapter = new ArrayAdapter<String>(mContext,R.layout.menu_list_item,itemChoices);
					//ArrayAdapter<String> adapter = new ArrayAdapter<String>(mContext,R.layout.dtvsettings_list_item,itemChoices);
					list_item.setAdapter(adapter);  
					list_item.setOnItemClickListener(new AdapterView.OnItemClickListener(){
						public void onItemClick(AdapterView<?> parent, View v, int position, long id){			
							switch(position){
								case 0: //edit
									final CustomDialog mEditCustomDialog = new CustomDialog(mContext);
									mEditCustomDialog.showDialog(R.layout.edit_dialog, new ICustomDialog(){
										public boolean onKeyDown(int keyCode, KeyEvent event){
											if(keyCode == KeyEvent.KEYCODE_BACK)
												mEditCustomDialog.dismissDialog();
											return false;
										}
										public void showWindowDetail(Window window){
											TextView title = (TextView)window.findViewById(R.id.title);
											title.setText(R.string.edit);
											final EditText mEditText = (EditText)window.findViewById(R.id.edit);
											mEditText.setText(mTVProgramList[pos].getName());
											Button no = (Button)window.findViewById(R.id.no);
											no.setText(R.string.no);
											no.setTextColor(Color.WHITE);
											Button yes = (Button)window.findViewById(R.id.yes);
											yes.setText(R.string.yes);
											yes.setTextColor(Color.WHITE);
											no.setOnClickListener(new OnClickListener(){
												public void onClick(View v) {
													mEditCustomDialog.dismissDialog();
												}
											});	 
											yes.setOnClickListener(new OnClickListener(){
												public void onClick(View v) {	
													mTVProgramList[pos].setProgramName(mEditText.getText().toString());
													myAdapter.notifyDataSetChanged();
													mEditCustomDialog.dismissDialog();
													mCustomDialog.dismissDialog();
												}
											});	    
										}
									});
									break;
								case 1: //delete
									deleteProgramFromDB(pos);
									myAdapter.notifyDataSetChanged();
									mCustomDialog.dismissDialog();
									break;
								case 2: //fav
									dealFav(pos);
									myAdapter.notifyDataSetChanged();
									mCustomDialog.dismissDialog();
									break;
								case 3: //lock
									dealLock(pos);
									myAdapter.notifyDataSetChanged();
									mCustomDialog.dismissDialog();
									break;
								case 4: //move
									setMoveMode(true);
									setMoveItemPos(pos);
									myAdapter.notifyDataSetChanged();
									mCustomDialog.dismissDialog();
									break;
								case 5: //add into group
									programGroupOperate(pos);
									mCustomDialog.dismissDialog();
									break;
								default:
									break;
							}
						}
					}	
					);
				}
			}	
		);		
	}

	private void create_group_button() {
		LinearLayout mLinearLayout = (LinearLayout)findViewById(R.id.LinearLayoutGroupButton) ;
		for(int i=0; i<TVGroupCount; i++){
			Log.d(TAG,"create_group_button="+i);
			LinearLayout.LayoutParams TempLP = 
				new LinearLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);					
			TempLP.leftMargin = 2;
			TempLP.bottomMargin = 1;

			Button TempButton;
			TempButton = new Button(this);

			switch(i){
				case 0:
					mButtonTv = TempButton;
					break;
				case 1:
					mButtonRadio= TempButton;
					break;
				case 2:
					mButtonFav= TempButton;
					break;
			}
						
			TempButton.setId(mProgramGroup[i].getID());
			TempButton.setTextColor(Color.WHITE);
			TempButton.setTextSize(22F);
			TempButton.setLayoutParams(TempLP);
			//TempButton.setVisibility(View.GONE);
			
			TempButton.setFocusableInTouchMode(true);
			TempButton.setOnFocusChangeListener(new GroupButtonItemOnFocusChange());
			TempButton.setOnClickListener(new GroupButtonItemOnClick());
			TempButton.setOnLongClickListener(new OnLongClickListener() {
				public boolean onLongClick(View view) {
					Log.d(TAG,"long Click");
					createMenuChoiceDialogForGroup(DTVProgramManager.this);
					return false;
				}
			});

			TempButton.setOnKeyListener(new OnKeyListener() {
				@Override
				public boolean onKey(View v, int keyCode, KeyEvent event) {
					switch(keyCode)
					{
						case KeyEvent.KEYCODE_DPAD_DOWN:
							if (event.getAction() == KeyEvent.ACTION_DOWN) {
								ListView_programmanager.requestFocus();
							}
							break;
					} 	
					return false;
				}
				
			});
			
			TempButton.setSingleLine(true);
			TempButton.setEllipsize(TextUtils.TruncateAt.valueOf("MARQUEE"));

			TempButton.setWidth(128);
			TempButton.setHeight(50);
			TempButton.setHint(" "+mProgramGroup[i].getID());
			TempButton.setBackgroundColor(GROUP_TIEM_UNFOCUSCOLOR_EVEN);	
			TempButton.setText(mProgramGroup[i].getName());
			TempButton.setVisibility(View.VISIBLE);
			
			((LinearLayout)mLinearLayout).addView(TempButton); 
		}
	}

	private void refreshGroupButton(){
		LinearLayout mLinearLayout = (LinearLayout)findViewById(R.id.LinearLayoutGroupButton) ;
		mLinearLayout.removeAllViews();
		for(int i=0; i<TVGroupCount; i++){
			Log.d(TAG,"create_group_button="+i);
			LinearLayout.LayoutParams TempLP = 
				new LinearLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);					
			TempLP.leftMargin = 2;
			TempLP.bottomMargin = 1;
			
			Button TempButton;
			TempButton = new Button(this);

			switch(i){
				case 0:
					mButtonTv = TempButton;
					break;
				case 1:
					mButtonRadio= TempButton;
					break;
				case 2:
					mButtonFav= TempButton;
					break;
			}
			
			TempButton.setId(mProgramGroup[i].getID());
			TempButton.setTextColor(Color.WHITE);
			TempButton.setTextSize(22F);
			TempButton.setLayoutParams(TempLP);
			//TempButton.setVisibility(View.GONE);
			
			TempButton.setFocusableInTouchMode(true);
			TempButton.setOnFocusChangeListener(new GroupButtonItemOnFocusChange());
			TempButton.setOnClickListener(new GroupButtonItemOnClick());
			TempButton.setOnLongClickListener(new OnLongClickListener() {
				public boolean onLongClick(View view) {
					Log.d(TAG,"long Click");
					createMenuChoiceDialogForGroup(DTVProgramManager.this);
					
					return false;
				}
			});

			TempButton.setOnKeyListener(new OnKeyListener() {
				@Override
				public boolean onKey(View v, int keyCode, KeyEvent event) {
					switch(keyCode)
					{
						case KeyEvent.KEYCODE_DPAD_DOWN:
							if (event.getAction() == KeyEvent.ACTION_DOWN) {
								ListView_programmanager.requestFocus();
								ListView_programmanager.setSelection(0); 
								return true;
							}
							break;
					} 	
					return false;
				}
				
			});
			
			TempButton.setSingleLine(true);
			TempButton.setEllipsize(TextUtils.TruncateAt.valueOf("MARQUEE"));

			TempButton.setWidth(128);
			TempButton.setHeight(50);
			TempButton.setHint(" "+mProgramGroup[i].getID());
			TempButton.setBackgroundColor(GROUP_TIEM_UNFOCUSCOLOR_EVEN);	
			TempButton.setText(mProgramGroup[i].getName());
			TempButton.setVisibility(View.VISIBLE);
			
			((LinearLayout)mLinearLayout).addView(TempButton); 
		}
	}


	class GroupButtonItemOnClick  implements OnClickListener{
		public void onClick(View v) {
			//final int i=Integer.valueOf(((Button)v).getHint().toString());
		}
	}

	private final static int    GROUP_TIEM_FOCUSCOLOR             = Color.argb(200, 255, 180, 0);
	private final static int    GROUP_TIEM_UNFOCUSCOLOR_ODD       = Color.argb(200, 75, 75, 75);
	private final static int    GROUP_TIEM_UNFOCUSCOLOR_EVEN      = Color.argb(200, 42, 42, 42);
	class GroupButtonItemOnFocusChange  implements OnFocusChangeListener{
		public void onFocusChange(View v, boolean isFocused){
			if (isFocused==true){
				((Button)v).setBackgroundColor(GROUP_TIEM_FOCUSCOLOR);
				TVProgramCurrentId = ((Button)v).getId();
				mTextview.setText(((Button)v).getText());
				if(TVProgramCurrentId!=-1){
					getListGroupById(TVProgramCurrentId);
					TabIndex = TVProgramCurrentId;
				}
				else{
					String name = ((Button)v).getText().toString();
					if(name.equals(getString(R.string.tv))){
						getListData(0);
						TabIndex = -1;
					}
					else if(name.equals(getString(R.string.radio))){
						getListData(1);
						TabIndex = -2;
					}
					else if(name.equals(getString(R.string.favorite))){
						getListFavorite();
						TabIndex = -3;
					}
				}

				myAdapter.notifyDataSetChanged();
			}
			else {				  
				((Button)v).setBackgroundColor(GROUP_TIEM_UNFOCUSCOLOR_EVEN);	
			}
	    }	
	}

	
}

