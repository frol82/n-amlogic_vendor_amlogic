package com.amlogic.tvservice;

import com.amlogic.tvutil.TVConfigValue;
import com.amlogic.tvutil.TVConfigValue.TypeException;
import com.amlogic.tvutil.TVChannelParams;
import com.amlogic.tvutil.TVProgram;
import com.amlogic.tvutil.TVConst;
import com.amlogic.tvutil.DTVPlaybackParams;
import com.amlogic.tvutil.DTVRecordParams;
import java.io.File;
import android.util.Log;
import android.os.Looper;

abstract public class TVDeviceImpl extends TVDevice{

	private long native_handle;
	private boolean destroy;

	private native void native_device_init();
	private native void native_device_destroy();
	private native void native_set_input_source(int src);
	private native void native_set_video_window(int x, int y, int w, int h);
	private native void native_set_frontend(TVChannelParams params);
	private native void native_set_frontend_prop(int cmd, int val);
	private native TVChannelParams native_get_frontend();
	private native int  native_get_frontend_status();
	private native int  native_get_frontend_signal_strength();
	private native int  native_get_frontend_snr();
	private native int  native_get_frontend_ber();
	private native void native_free_frontend();
	private native void native_start_vbi(int flags);
	private native void native_stop_vbi(int flags);
	private native void native_play_atv();
	private native void native_stop_atv();
	private native void native_play_dtv(int vpid, int vfmt, int apid, int afmt, int pcr);
	private native void native_switch_dtv_audio(int apid, int afmt);
	private native void native_ad_start(int apid, int afmt);
	private native void native_ad_stop();
	private native void native_stop_dtv();
	private native void native_start_recording(DTVRecordParams params);
	private native void native_stop_recording();
	private native DTVRecordParams native_get_recording_params();
	private native void native_start_timeshifting(DTVPlaybackParams params);
	private native void native_stop_timeshifting();
	private native void native_start_playback(DTVPlaybackParams params);
	private native void native_stop_playback();
	private native DTVPlaybackParams native_get_playback_params();
	private native void native_fast_forward(int speed);
	private native void native_fast_backward(int speed);
	private native void native_pause();
	private native void native_resume();
	private native void native_seek_to(int pos);
	private native void native_setSecRequest(int secType, TVChannelParams secCurParams, int secPositionerMoveUnit);
	private native void native_switch_video_blackout(int val);
	private native String native_am_read_sysfile(String name);
	private native int native_am_write_sysfile(String name,String value);

	

	static{
		System.loadLibrary("am_adp");
		System.loadLibrary("am_mw");
		System.loadLibrary("zvbi");
		System.loadLibrary("jnitvmboxdevice");
	}

	public TVDeviceImpl(){
		super();
		destroy = false;
		native_device_init();
	}
	
	 public TVDeviceImpl(Looper looper)
    {
        super();
		destroy = false;
		native_device_init();
    }
	

	private TVConst.SourceInput curInputSource = TVConst.SourceInput.SOURCE_DTV;

	public void setInputSource(TVConst.SourceInput source){
		native_set_input_source(source.ordinal());
		curInputSource = source;
	}

	public TVConst.SourceInput getCurInputSource(){
		return curInputSource;
	}

    public void setVideoWindow(int x, int y, int w, int h){
    	native_set_video_window(x, y, w, h);
	}

	public void setFrontend(TVChannelParams params){
		native_set_frontend(params);
	}

	public void setFrontendProp(int cmd,int val){
		native_set_frontend_prop(cmd,val);	
	}

	public TVChannelParams getFrontend(){
		return native_get_frontend();
	}

	public int getFrontendStatus(){
		return native_get_frontend_status();
	}

	public int getFrontendSignalStrength(){
		return native_get_frontend_signal_strength();
	}

	public int getFrontendSNR(){
		return native_get_frontend_snr();
	}

	public int getFrontendBER(){
		return native_get_frontend_ber();
	}

	public void freeFrontend(){
		native_free_frontend();
	}

	public void startVBI(int flags){
		native_start_vbi(flags);
	}

	public void stopVBI(int flags){
		native_stop_vbi(flags);
	}

	public void playATV(){
		native_play_atv();
	}

	public void resetATVFormat(TVChannelParams params){
	}

	public void stopATV(){
		native_stop_atv();
	}

	public void playDTV(int vpid, int vfmt, int apid, int afmt, int pcr){
		native_play_dtv(vpid, vfmt, apid, afmt, pcr);
	}

  	public void switchDTVAudio(int apid, int afmt){
		native_switch_dtv_audio(apid, afmt);
	}

	public void startAD(int apid,int afmt){
		native_ad_start(apid,afmt);
	}

	public void stopAD(){
		native_ad_stop();
	}

	public void stopDTV(){
		native_stop_dtv();
	}

	public void startRecording(DTVRecordParams params){
		native_start_recording(params);
	}

	public void stopRecording(){
		native_stop_recording();
	}
	
	public DTVRecordParams getRecordingParams(){
		return native_get_recording_params();
	}

	public void startTimeshifting(DTVPlaybackParams params){
		native_start_timeshifting(params);
	}

	public void stopTimeshifting(){
		native_stop_timeshifting();
	}

	public void startPlayback(DTVPlaybackParams params){
		native_start_playback(params);
	}

	public void stopPlayback(){
		native_stop_playback();
	}
	
	public DTVPlaybackParams getPlaybackParams(){
		return native_get_playback_params();
	}

	public void pause(){
		native_pause();
	}

	public void resume(){
		native_resume();
	}

	public void fastForward(int speed){
		native_fast_forward(speed);
	}

	public void fastBackward(int speed){
		native_fast_backward(speed);
	}

	public void seekTo(int pos){
		native_seek_to(pos);
	}

	public void switch_video_blackout(int val){
		native_switch_video_blackout(val);
	}

	//add sys file read api
	public String am_read_sysfile(String name){
		return native_am_read_sysfile(name);
	}
	//add sys file write api
	public void am_write_sysfile(String name,String value){
		native_am_write_sysfile(name,value);
	}

	public void dispose(){
		if(!destroy){
			destroy = false;
			native_device_destroy();
		}
	}

    @Override
    public void onUpdate(String name, TVConfigValue value)
    {
	}

	@Override
    public TVConfigValue read(String name, TVConfig.TVConfigEntry entry)
	{
		return null;
	}
	
	@Override
    public void ATVChannelFineTune(int fre)
	{
		
	}
    
    @Override
    public void SetCvbsAmpOut(int amp)
    {
    }
	
	public void setVGAAutoAdjust()
    {
    }

	public void setSecRequest(int secType, TVChannelParams secCurParams, int secPositionerMoveUnit)
	{
		native_setSecRequest(secType, secCurParams, secPositionerMoveUnit);
	}
}

