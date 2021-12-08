package com.amlogic.tvservice;
import android.util.Log;
import com.amlogic.tvutil.TVChannelParams;

abstract public class TVEpgScanner{
	private static final String TAG = "TVEpgScanner";
	public static final int MODE_ADD    = 0;
	public static final int MODE_REMOVE = 1;
	public static final int MODE_SET    = 2;

	public static final int SCAN_PAT = 0x01;
	public static final int SCAN_PMT = 0x02;
	public static final int SCAN_CAT = 0x04;
	public static final int SCAN_SDT = 0x08;
	public static final int SCAN_NIT = 0x10;
	public static final int SCAN_TDT = 0x20;
	public static final int SCAN_EIT_PF_ACT = 0x40;
	public static final int SCAN_EIT_PF_OTH = 0x80;
	public static final int SCAN_EIT_SCHE_ACT = 0x100;
	public static final int SCAN_EIT_SCHE_OTH = 0x200;
	public static final int SCAN_MGT = 0x400;
	public static final int SCAN_VCT = 0x800;
	public static final int SCAN_STT = 0x1000;
	public static final int SCAN_RRT = 0x2000;
	public static final int SCAN_PSIP_EIT   = 0x4000;
	public static final int SCAN_EIT_PF_ALL = SCAN_EIT_PF_ACT | SCAN_EIT_PF_OTH;
	public static final int SCAN_EIT_SCHE_ALL = SCAN_EIT_SCHE_ACT | SCAN_EIT_SCHE_OTH;
	public static final int SCAN_EIT_ALL = SCAN_EIT_PF_ALL | SCAN_EIT_SCHE_ALL;
	public static final int SCAN_ALL = SCAN_PAT | SCAN_PMT | SCAN_CAT | SCAN_SDT | SCAN_NIT | SCAN_TDT | SCAN_EIT_ALL |
			SCAN_MGT | SCAN_VCT | SCAN_STT | SCAN_RRT | SCAN_PSIP_EIT;

    public class Event
    {
        public static final int EVENT_PF_EIT_END            = 1;
        public static final int EVENT_SCH_EIT_END           = 2;
        public static final int EVENT_PMT_END               = 3;
        public static final int EVENT_SDT_END               = 4;
        public static final int EVENT_TDT_END               = 5;
        public static final int EVENT_NIT_END               = 6;
        public static final int EVENT_PROGRAM_AV_UPDATE     = 7;
        public static final int EVENT_PROGRAM_NAME_UPDATE   = 8;
        public static final int EVENT_PROGRAM_EVENTS_UPDATE = 9;
        public static final int EVENT_CHANNEL_UPDATE        = 10;

        public int type;
        public int channelID;
        public int programID;
        public int dvbOrigNetID;
        public int dvbTSID;
        public int dvbServiceID;
        public long time;
        public int dvbVersion;
	 	
        public Event(int type){
			this.type = type;
		}
    }

	private long native_handle;
	private native void native_epg_create(int fend_id, int dmx_id, int src, String textLangs);
	private native void native_epg_destroy();
	private native void native_epg_change_mode(int op, int mode);
	private native void native_epg_monitor_service(int src_id);
	private native void native_epg_set_dvb_text_coding(String coding);

	/** Load native library*/
	static{
		System.loadLibrary("jnitvepgscanner");
	}

	private int fend_dev_id = -1;
	private int dmx_dev_id  = -1;
	private int fend_type   = -1;
	private boolean created = false;
	private int channel_id  = -1;
	private int program_id  = -1;

	/*Start scan the sections.*/
	private void startScan(int mode){
		Log.d(TAG, "startScan  start");
		if(!created)
			return;

		native_epg_change_mode(MODE_ADD, mode);
	}

	/*Stop scan the sections.*/
	private void stopScan(int mode){
		Log.d(TAG, "stopScan  start");
		if(!created)
			return;

		native_epg_change_mode(MODE_REMOVE, mode);
	}

	public TVEpgScanner(){
	}

	public void setSource(int fend, int dmx, int src, String textLanguages){
		if(created)
			destroy();
		
		fend_dev_id = fend;
		dmx_dev_id  = dmx;
		fend_type   = src;
		native_epg_create(fend, dmx, src, textLanguages);
		created = true;
	}

	public void destroy(){
		if(created){
			native_epg_destroy();
			created = false;
		}
	}

	/*Enter a channel.*/
	public void enterChannel(int chan_id){
		if(chan_id == channel_id)
			return;

		if(!created)
			return;

		if(channel_id != -1){
			leaveChannel();
		}
		
		if(fend_type == TVChannelParams.MODE_ATSC){
			startScan(SCAN_PSIP_EIT|SCAN_MGT|SCAN_VCT|SCAN_RRT|SCAN_STT);
		}else{
			startScan(SCAN_EIT_ALL|SCAN_SDT|SCAN_NIT|SCAN_TDT|SCAN_CAT);
		}
		
		channel_id = chan_id;
	}

	/*Leave the channel.*/
	public void leaveChannel(){
		if(channel_id == -1)
			return;

		if(!created)
			return;

		stopScan(SCAN_ALL);
		channel_id = -1;
	}

	/*Enter the program.*/
	public void enterProgram(int prog_id){
		if(prog_id == program_id)
			return;

		if(!created)
			return;

		if(program_id != -1){
			leaveProgram();
		}

		native_epg_monitor_service(prog_id);
		startScan(SCAN_PAT|SCAN_PMT);
		program_id = prog_id;
	}

	/*Leave the program.*/
	public void leaveProgram(){
		if(program_id == -1)
			return;

		if(!created)
			return;

		stopScan(SCAN_PAT|SCAN_PMT);
		native_epg_monitor_service(-1);
		program_id = -1;
	}

	/*Set the default dvb text coding, 'standard' indicates using DVB text coding standard*/
	public void setDvbTextCoding(String coding){
		native_epg_set_dvb_text_coding(coding);
	}

    abstract void onEvent(Event event);
}

