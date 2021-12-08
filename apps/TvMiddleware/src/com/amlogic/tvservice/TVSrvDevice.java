package com.amlogic.tvservice;

import com.amlogic.tvservice.TVConfig.TVConfigEntry;
import com.amlogic.tvutil.TVChannelParams;
import com.amlogic.tvutil.TVConfigValue;
import com.amlogic.tvutil.TVConfigValue.TypeException;
import com.amlogic.tvutil.TvinInfo.tvin_sig_fmt_e;
import com.amlogic.tvutil.TvinInfo.tvin_sig_status_t;
import com.amlogic.tvutil.TvinInfo.tvin_trans_fmt;
import com.amlogic.tvutil.TVProgram;
import com.amlogic.tvutil.TVConst;
import com.amlogic.tvutil.DTVPlaybackParams;
import com.amlogic.tvutil.DTVRecordParams;
import com.amlogic.tvutil.TvinInfo;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileWriter;

import android.util.Log;
import android.amlogic.Tv;
import android.amlogic.Tv.*;
import android.amlogic.Tv.Frontend_Para;
import android.amlogic.Tv.SigInfoChangeListener;
import android.amlogic.Tv.SourceSwitchListener;
import android.amlogic.Tv.SrcInput;
import android.amlogic.Tv.StatusTVChangeListener;
import android.amlogic.Tv.VGAAdjustChangeListener;
import android.amlogic.Tv.tvin_info_t;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;

public abstract class TVDeviceImpl extends TVDevice implements StatusTVChangeListener, SourceSwitchListener
                                                            , VGAAdjustChangeListener,SigInfoChangeListener
{
    private boolean destroy;
    private String TAG = "TVDeviceImpl";
    public static Tv tv = null;
    private Looper mylooper = null;
    public static final int NATVIVE_EVENT_FRONTEND = 1;
    public static final int NATVIVE_EVENT_PLAYER = 2;
    public static final int NATVIVE_EVENT_SIGNAL_OK = 1;
    public static final int NATVIVE_EVENT_SIGNAL_NOT_OK = 0;

    public static final int EVENT_FRONTEND = 1 << 1;
    public static final int EVENT_SOURCE_SWITCH = 1 << 2;
    deviceHandler myHandler = null;
    private int mFreq = 0;
    public TVDeviceImpl()
    {
        super();
        tv = SingletonTv.getTvInstance();
        tv.SetStatusTVChangeListener(this);
        tv.SetSourceSwitchListener(this);
        tv.SetSigInfoChangeListener(this);
        tv.SetVGAChangeListener(this);
        // tv.INIT_TV();

    }

    public TVDeviceImpl(Looper looper)
    {
        super();
        mylooper = looper;

        tv = SingletonTv.getTvInstance();
        tv.SetStatusTVChangeListener(this);
        tv.SetSourceSwitchListener(this);
        tv.SetSigInfoChangeListener(this);
        tv.SetVGAChangeListener(this);
        // tv.INIT_TV();
        myHandler = new deviceHandler(looper);
    }

    public class TVException extends Exception
    {
    }

    private class deviceHandler extends Handler
    {

        public deviceHandler(Looper looper)
        {
            super(looper);

        }

        @Override
        public void handleMessage(Message msg)
        {
            switch (msg.what)
            {
                case 1:

                    Log.v(TAG, "deviceHandler handleMessage");
                    Bundle bundle = msg.getData();
                    int type = bundle.getInt("type");
                    int state = bundle.getInt("state");
                    int mode = bundle.getInt("mode");
                    int freq = bundle.getInt("freq");
                    int para1 = bundle.getInt("para1");
                    int para2 = bundle.getInt("para2");

                    if (type == NATVIVE_EVENT_FRONTEND)
                    { // frontEnd
                        Log.v(TAG, "handleMessage onStatusDTVChange:  " + type + "  " + state + "  " + mode + "  " + freq + "  " + para1 + "  "
                                + para2);
                        Event myEvent = new Event(Event.EVENT_FRONTEND);
                        if (state == NATVIVE_EVENT_SIGNAL_OK)
                        {
                            Log.v(TAG, "NATVIVE_EVENT_SIGNAL_OK");
                            myEvent.feStatus = TVChannelParams.FE_HAS_LOCK;
                        }
                        else if (state == NATVIVE_EVENT_SIGNAL_NOT_OK)
                        {
                            Log.v(TAG, "NATVIVE_EVENT_SIGNAL_NOT_OK");
                            myEvent.feStatus = TVChannelParams.FE_TIMEDOUT;
                        }
                        myEvent.feParams = fpara2chanpara(mode, freq, para1, para2);
                        
                        onEvent(myEvent);
                    }

                    break;
            }
        }
    }

    public void setInputSource(TVConst.SourceInput source)
    {
        // native_set_input_source(source.ordinal());
        Log.v(TAG, "setInputSource " + source.toString());
        Log.v(TAG, "^^^^^^^^^^^^^^^^^ & ^------^^ & ^^^^^^^^^^^^^^^^^^^^^^^^^^^");
        if (source == TVConst.SourceInput.SOURCE_DTV)
            tv.SetSourceInput(Tv.SrcInput.DTV);
        else if (source == TVConst.SourceInput.SOURCE_ATV)
            tv.SetSourceInput(Tv.SrcInput.TV);
        else if (source == TVConst.SourceInput.SOURCE_AV1)
            tv.SetSourceInput(Tv.SrcInput.AV1);
        else if (source == TVConst.SourceInput.SOURCE_AV2)
            tv.SetSourceInput(Tv.SrcInput.AV2);
        else if (source == TVConst.SourceInput.SOURCE_YPBPR1)
            tv.SetSourceInput(Tv.SrcInput.YPBPR1);
        else if (source == TVConst.SourceInput.SOURCE_YPBPR2)
            tv.SetSourceInput(Tv.SrcInput.YPBPR2);
        else if (source == TVConst.SourceInput.SOURCE_HDMI1)
            tv.SetSourceInput(Tv.SrcInput.HDMI1);
        else if (source == TVConst.SourceInput.SOURCE_HDMI2)
            tv.SetSourceInput(Tv.SrcInput.HDMI2);
        else if (source == TVConst.SourceInput.SOURCE_HDMI3)
            tv.SetSourceInput(Tv.SrcInput.HDMI3);
        else if (source == TVConst.SourceInput.SOURCE_VGA)
            tv.SetSourceInput(Tv.SrcInput.VGA);
        else if (source == TVConst.SourceInput.SOURCE_MPEG)
            tv.SetSourceInput(Tv.SrcInput.MPEG);
        else if (source == TVConst.SourceInput.SOURCE_SVIDEO)
            tv.SetSourceInput(Tv.SrcInput.SVIDEO);
		else if (source == TVConst.SourceInput.SOURCE_HDMI_4K2K)
            tv.SetSourceInput(Tv.SrcInput.HDMI4K2K);
		else if (source == TVConst.SourceInput.SOURCE_USB_4K2K)
            tv.SetSourceInput(Tv.SrcInput.USB4K2K);
        // **********************temp************************
        // Event myEvent = new Event(Event.EVENT_SET_INPUT_SOURCE_OK);
        // this.onEvent(myEvent);
        // *********************finish************************

    }

    public TVConst.SourceInput getCurInputSource()
    {

        int val = tv.GetCurrentSourceInput();
        TVConst.SourceInput source = TVConst.SourceInput.SOURCE_ATV;

        Log.v(TAG, "^^^^^^^^^^^^^^^^getCurInputSource^^^^^^^^^^^^^^^^^^^^^^^^^^^" + val);

        if (val == Tv.SrcInput.DTV.toInt())
        {
            source = TVConst.SourceInput.SOURCE_DTV;
        }
        else if (val == Tv.SrcInput.TV.toInt())
        {
            source = TVConst.SourceInput.SOURCE_ATV;
        }
        else if (val == Tv.SrcInput.AV1.toInt())
        {
            source = TVConst.SourceInput.SOURCE_AV1;
        }
        else if (val == Tv.SrcInput.AV2.toInt())
        {
            source = TVConst.SourceInput.SOURCE_AV2;
        }
        else if (val == Tv.SrcInput.YPBPR1.toInt())
        {
            source = TVConst.SourceInput.SOURCE_YPBPR1;
        }
        else if (val == Tv.SrcInput.YPBPR2.toInt())
        {
            source = TVConst.SourceInput.SOURCE_YPBPR2;
        }
        else if (val == Tv.SrcInput.HDMI1.toInt())
        {
            source = TVConst.SourceInput.SOURCE_HDMI1;
        }
        else if (val == Tv.SrcInput.HDMI2.toInt())
        {
            source = TVConst.SourceInput.SOURCE_HDMI2;
        }
        else if (val == Tv.SrcInput.HDMI3.toInt())
        {
            source = TVConst.SourceInput.SOURCE_HDMI3;
        }
        else if (val == Tv.SrcInput.VGA.toInt())
        {
            source = TVConst.SourceInput.SOURCE_VGA;
        }
        else if (val == Tv.SrcInput.MPEG.toInt())
        {
            source = TVConst.SourceInput.SOURCE_MPEG;
        }
        else if (val == Tv.SrcInput.SVIDEO.toInt())
        {
            source = TVConst.SourceInput.SOURCE_SVIDEO;
        }
		 else if (val == Tv.SrcInput.HDMI4K2K.toInt())
        {
            source = TVConst.SourceInput.SOURCE_HDMI_4K2K;
        }
		  else if (val == Tv.SrcInput.USB4K2K.toInt())
        {
            source = TVConst.SourceInput.SOURCE_USB_4K2K;
        }

        return source;
    }

    public void setVideoWindow(int x, int y, int w, int h)
    {
        SetWindowSize(1,x ,y ,w ,h);
    }

    public void switchDTVAudio(int pid, int afmt)
    {
	    Log.d(TAG, "switchDTVAudio "+pid + " " + afmt);
	    tv.SwitchDTVAudio(pid,afmt);
    }

    public void setFrontend(TVChannelParams params)
    {
        // native_set_frontend(params);
        tv.INIT_TV();
        Log.d(TAG, ""+params.mode);
        if (params.mode == TVChannelParams.MODE_QAM)
            tv.SetFrontEnd(params.mode, params.frequency, params.symbolRate, params.modulation);
        else if (params.mode == TVChannelParams.MODE_ANALOG)
        {
            mFreq = params.frequency;
            tv.SetFrontEnd(params.mode, params.frequency, params.standard,params.afc_data/500);
        }
        else if (params.mode == TVChannelParams.MODE_OFDM)
            tv.SetFrontEnd(params.mode, params.frequency, params.bandwidth, 0);
        else if (params.mode == TVChannelParams.MODE_ATSC)
            tv.SetFrontEnd(params.mode, params.frequency, params.modulation, 0);
	    else if (params.mode == TVChannelParams.MODE_DTMB)
            tv.SetFrontEnd(params.mode, params.frequency,  params.bandwidth , 0);
        
    }

    public TVChannelParams getFrontend()
    {
        TVChannelParams params;
        // return native_get_frontend();
        // tv.INIT_TV();
        Tv.Frontend_Para fpara = tv.GetFrontEnd();
        params = fpara2chanpara(fpara.mode, fpara.frequency, fpara.para1, fpara.para2);

        return params;
    }

    public int getFrontendStatus()
    {
        // return native_get_frontend_status();
        Log.v(TAG, "getfrontendstatus is" + tv.Get_FrontendStatus());
        return tv.Get_FrontendStatus();
    }

    public int getFrontendSignalStrength()
    {
        // return native_get_frontend_signal_strength();
        Log.v(TAG, "getfrontendsignalstrength is" + tv.Get_FrontendSignalStrength());
        return tv.Get_FrontendSignalStrength();
    }

    public int getFrontendSNR()
    {
        // return native_get_frontend_snr();
        Log.v(TAG, "getfrontendsnr" + tv.Get_FrontendSNR());
        return tv.Get_FrontendSNR();
    }

    public int getFrontendBER()
    {
        // return native_get_frontend_ber();
        Log.v(TAG, "getfrontendber" + tv.Get_FrontendBER());
        return tv.Get_FrontendBER();
    }

    public void freeFrontend()
    {
        // native_free_frontend();
        Log.v(TAG, "freeFrontend");
        tv.FreeFrontEnd();

    }

    public void startVBI(int flags)
    {
        Log.e(TAG, "*********startVBI have not realize");
        // native_start_vbi(flags);
    }

    public void stopVBI(int flags)
    {
        Log.e(TAG, "*********stopVBI have not realize");
        // native_stop_vbi(flags);
    }

    public void playATV()
    {
        // native_play_atv();
        tv.StartTV((int) TVConst.SourceInput.SOURCE_ATV.ordinal(), 0, 0, 0, 0);
    }

    public void resetATVFormat(TVChannelParams params)
    {
        Log.v(TAG, "resetATVFormat");
        if (params.mode == TVChannelParams.MODE_ANALOG)
            tv.SetFrontEnd(params.mode, params.frequency, params.standard, 0);
    }

    public void stopATV()
    {
        // native_stop_atv();
        Log.v(TAG, "temp not handle stop atv");
        // tv.StopTV((int)TVConst.SourceInput.SOURCE_ATV.ordinal());
    }

    public void playDTV(int vpid, int vfmt, int apid, int afmt, int pcr)
    {
        // native_play_dtv(vpid, vfmt, apid, afmt);
        Log.v(TAG, "SourceInput SOURCE_DTV" + (int) TVConst.SourceInput.SOURCE_DTV.ordinal());
        tv.StartTV((int) TVConst.SourceInput.SOURCE_DTV.ordinal(), vpid, apid, vfmt, afmt);
    }

    public void stopDTV()
    {
        // native_stop_dtv();
        Log.v(TAG, "stopDTV");
        tv.StopTV((int) TVConst.SourceInput.SOURCE_DTV.ordinal());
    }

	public void ATVChannelFineTune(int fre)
    {
        Log.d(TAG, "*********ATVChannelFineTune" + fre);
        tv.Set_Atv_Channel_Fine_Tune(fre);
    }
    
    public void SetCvbsAmpOut(int amp)
    {
        Log.d(TAG, "*********SetCvbsAmpOut" + amp);
        tv.SetCvbsAmpOut(amp);
    }

    public void startRecording(DTVRecordParams params)
    {
        Log.e(TAG, "*********startRecording have not realize");
        // native_start_recording(params);
    }

    public void stopRecording()
    {
        Log.e(TAG, "*********stopRecording have not realize");
        // return native_stop_recording();
    }
    
    public DTVRecordParams getRecordingParams()
    {
    	return null;
    }

    public void startTimeshifting(DTVPlaybackParams params)
    {
    }

    public void stopTimeshifting()
    {
        // return native_stop_timeshifting();
        Log.e(TAG, "*********stopTimeshifting have not realize");
    }

    public void startPlayback(DTVPlaybackParams params)
    {
        // native_start_playback(params);
        Log.e(TAG, "*********startPlayback have not realize");
    }

    public void stopPlayback()
    {
        // native_stop_playback();
        Log.e(TAG, "*********stopPlayback have not realize");
    }
    
    public DTVPlaybackParams getPlaybackParams()
    {
    	return null;
    }

    public void pause()
    {
        // native_pause();
        Log.e(TAG, "*********pause have not realize");
    }

    public void resume()
    {
        // native_resume();
        Log.e(TAG, "*********resume have not realize");
    }

    public void fastForward(int speed)
    {
        // native_fast_forward(speed);
        Log.e(TAG, "*********fastForward have not realize");
    }

    public void fastBackward(int speed)
    {
        // native_fast_backward(speed);
        Log.e(TAG, "*********fastBackward have not realize");
    }

    public void seekTo(int pos)
    {
        // native_seek_to(pos);
        Log.e(TAG, "*********seekTo have not realize");
    }

    public void setVGAAutoAdjust()
    {
        Log.d(TAG, "*********setVGAAutoAdjust");
        tv.RunVGAAutoAdjust();
    }

    public int GetSrcInputType()
    {
        Log.d(TAG, "*********GetSrcInputType");
        return tv.GetSrcInputType().ordinal();
    }
    
    public TvinInfo GetCurrentSignalInfo()
    {
        Log.d(TAG, "*********GetCurrentSignalInfo");
        TvinInfo mytvinfo = new TvinInfo();
        tvin_info_t tvin_info = tv.GetCurrentSignalInfo();
        mytvinfo.fmt = tvin_sig_fmt_e.values()[tvin_info.fmt.ordinal()];
        mytvinfo.trans_fmt = tvin_trans_fmt.values()[tvin_info.trans_fmt.ordinal()];
        mytvinfo.status = tvin_sig_status_t.values()[tvin_info.status.ordinal()];
        mytvinfo.reserved = tvin_info.reserved;
        return mytvinfo;
    }
    
    
  
    
    protected void finalize() throws Throwable
    {
        if (!destroy)
        {
            destroy = false;
            // native_device_destroy();
            Log.e(TAG, "*********finalize have not realize");
        }
    }

    public void onStatusTVChange(int type, int state, int mode, int freq, int para1, int para2)
    {
        // TODO Auto-generated method stub
        Log.v(TAG, "onStatusDTVChange:  " + type + "  " + state + "  " + mode + "  " + freq + "  " + para1 + "  " + para2);
        if (type == NATVIVE_EVENT_FRONTEND)
        {
            // frontEnd
            if (mode == TVChannelParams.MODE_ANALOG)
            {
                myHandler.removeMessages(1);
                Message msg = myHandler.obtainMessage(1);
                Bundle bundle = new Bundle();
                bundle.putInt("type", type);
                bundle.putInt("state", state);
                bundle.putInt("mode", mode);
                /*callback old freq*/
//                bundle.putInt("freq", freq);
                bundle.putInt("freq", mFreq);
                bundle.putInt("para1", para1);
                bundle.putInt("para2", para2);
                msg.setData(bundle);
                myHandler.sendMessageDelayed(msg, 1000);
            }
            else
            {
                Event myEvent = new Event(Event.EVENT_FRONTEND);
                if (state == NATVIVE_EVENT_SIGNAL_OK)
                {
                    Log.v(TAG, "NATVIVE_EVENT_SIGNAL_OK");
                    myEvent.feStatus = TVChannelParams.FE_HAS_LOCK;
                }
                else if (state == NATVIVE_EVENT_SIGNAL_NOT_OK)
                {
                    Log.v(TAG, "NATVIVE_EVENT_SIGNAL_NOT_OK");
                    myEvent.feStatus = TVChannelParams.FE_TIMEDOUT;
                }

                myEvent.feParams = fpara2chanpara(mode, freq, para1, para2);
                onEvent(myEvent);
            }
        }

    }

    public void onSourceSwitchStatusChange(SrcInput input, int state)
    {
        Log.v(TAG, "onSourceSwitchStatusChange:  " + input.toString() + Integer.toString(state));
        
        if(input == SrcInput.TV)
        {
            Event myEvent = new Event(Event.EVENT_SET_INPUT_SOURCE_OK);
            myEvent.source = input.ordinal();
            onEvent(myEvent);
            return;
        }
        if (state == 0)
        {
            Event myEvent = new Event(Event.EVENT_SET_INPUT_SOURCE_OK);
            myEvent.source = input.ordinal();
            onEvent(myEvent);
        }else
        {
            Event myEvent = new Event(Event.EVENT_SET_INPUT_SOURCE_FAILED);
            myEvent.source = input.ordinal();
            onEvent(myEvent);
        }
    }

    @Override
    public void onVGAAdjustChange(int state)
    {
        // TODO Auto-generated method stub
        Log.d(TAG, "onVGAAdjustChange status:" + state);
        Event myEvent = new Event(Event.EVENT_VGA_ADJUST_STATUS);
        myEvent.vga_adjust_status = null;

        if (state < 0)
            myEvent.vga_adjust_status = TVConst.VGA_ADJUST_STATUS.CC_TV_VGA_ADJUST_FAILED;
        else if (state == 0)
            myEvent.vga_adjust_status = TVConst.VGA_ADJUST_STATUS.CC_TV_VGA_ADJUST_DOING;
        else if (state == 1)
            myEvent.vga_adjust_status = TVConst.VGA_ADJUST_STATUS.CC_TV_VGA_ADJUST_SUCCESS;
        onEvent(myEvent);
    }
    
    @Override
    public void onSigChange(tvin_info_t arg0)
    {
        // TODO Auto-generated method stub
        Log.d(TAG, "onSigChange ");
    //tmp   if(tv.GetCurrentSourceInput()  != Tv.SrcInput.TV.toInt())
        {
            TvinInfo tvinfo = new TvinInfo();
            tvinfo.fmt = tvin_sig_fmt_e.values()[arg0.fmt.ordinal()];
            tvinfo.trans_fmt = tvin_trans_fmt.values()[arg0.trans_fmt.ordinal()];
            tvinfo.status = tvin_sig_status_t.values()[arg0.status.ordinal()];
            tvinfo.reserved = arg0.reserved;
            Event myEvent = new Event(Event.EVENT_SIG_CHANGE,tvinfo);
       //     tvin_info_t tvin_info = tv.GetCurrentSignalInfo();
       /*     myEvent.tvin_info.fmt = tvin_sig_fmt_e.values()[arg0.fmt.ordinal()];
            myEvent.tvin_info.trans_fmt = tvin_trans_fmt.values()[arg0.trans_fmt.ordinal()];
            myEvent.tvin_info.status = tvin_sig_status_t.values()[arg0.status.ordinal()];
            myEvent.tvin_info.reserved = tvin_info.reserved;*/
            if(myEvent.tvin_info == null)
                Log.d(TAG,"*************tvin_info is null..................");
            onEvent(myEvent);
        }
    }

    

    TVChannelParams fpara2chanpara(int mode, int freq, int para1, int para2)
    {
        Log.v(TAG, "mode freq para1 para2:" + mode + "," + freq + "," + para1 + "," + para2);
        TVChannelParams tvChannelPara = null;
        switch (mode)
        {
            case TVChannelParams.MODE_OFDM:
                Log.v(TAG, " MODE_OFDM");
                tvChannelPara = TVChannelParams.dvbtParams(freq, para1);
                break;
            case TVChannelParams.MODE_QAM:
                Log.v(TAG, " MODE_QAM");
                tvChannelPara = TVChannelParams.dvbcParams(freq, para2, para1);
                break;
            case TVChannelParams.MODE_ANALOG:

                Log.v(TAG, " MODE_ANALOG");
                tvChannelPara = TVChannelParams.analogParams(freq, para1, 0,para2);
                break;
            case TVChannelParams.MODE_ATSC:
                Log.v(TAG, " MODE_ATSC");
                tvChannelPara = TVChannelParams.atscParams(freq, para1);
                break;
			 case TVChannelParams.MODE_DTMB:
                Log.v(TAG, " MODE_DTMB");
                tvChannelPara = TVChannelParams.dtmbParams(freq, para1);
                break;
        }

        return tvChannelPara;
    }

    @Override
    public void onUpdate(String name, TVConfigValue value)
    {
        // TODO Auto-generated method stub
        /*****************tv setting***********************/
        Log.v(TAG, "SET " + name + ": " + value);
        String configType = "Set";
        String save = "SaveCur";
        String saveName = "";
        if (TVDeviceImpl.tv == null)
            try
            {
                throw new TVException();
            }
            catch (TVException e)
            {
                e.printStackTrace();
            }

        int pos = name.indexOf(":");
        if (pos == -1)
            Log.e(TAG, "set name fail");
        name = name.substring(pos + 1);
        
    
        
        pos = name.indexOf(":");
        if (pos == -1)
            Log.e(TAG, "set name fail");
        
        String source = name.substring(0, pos);
                
        int    mysource = Integer.valueOf(source);   
        Log.d(TAG, "Source Input Type" + mysource);
        
        name = name.substring(pos + 1);
       
        saveName = save + name;
        name = configType + name;
        
        
        
        
        Log.v(TAG, "SET NEW " + name);

        int type = value.getType();
        switch (type)
        {
            case TVConfigValue.TYPE_INT:
                int userValue = 0;
                try
                {
                    userValue = value.getInt();
                }
                catch (TypeException e)
                {
                    e.printStackTrace();
                }
                int status3D = TVDeviceImpl.tv.Get3DMode();
                tvin_info_t sig_fmt = TVDeviceImpl.tv.GetCurrentSignalInfo();

                int fmt = sig_fmt.fmt.ordinal();
                int ON = 1;
                if (name.equals("SetSharpness"))
                {
                    TVDeviceImpl.tv.TvITFExecute(name, userValue, mysource, ON, status3D);
                }
                else if (name.equals("SetSaturation") || name.equals("SetDisplayMode") || name.equals("SetHue"))
                {
                	if(name.equals("SetDisplayMode"))//For DisPlay Mode 2.19
                	{
                		userValue = get_screen_mode(userValue);
						if(userValue == 5 && IsFmtHighResolution())
						{
							TVDeviceImpl.tv.SetDisplayModeWithoutSave(Tv.Dis_Mode.DISPLAY_MODE_169,Tv.Source_Input_Type.values()[mysource],Tv.tvin_sig_fmt_e.values()[fmt]);
							TVDeviceImpl.tv.SaveDisplay(Tv.Dis_Mode.values()[userValue],Tv.Source_Input_Type.values()[mysource],Tv.tvin_sig_fmt_e.values()[fmt]);
						}							
						else//add for 1920x1200 in HDMI 6.13
						{
							TVDeviceImpl.tv.TvITFExecute(name, userValue, mysource, fmt);
						}
                	}
					else
                    TVDeviceImpl.tv.TvITFExecute(name, userValue, mysource, fmt);
                }
                else if (name.equals("SetVGAPhase") || name.equals("SetVGAClock") || name.equals("SetVGAHPos") || name.equals("SetVGAVPos"))
                {
                    TVDeviceImpl.tv.TvITFExecute(name, userValue, fmt);
                }
                else if (name.equals("SetAudioSoundMode") || name.equals("SetAudioBalance") || name.equals("SetAudioTrebleVolume")
                        || name.equals("SetAudioBassVolume") || name.equals("SetAudioSupperBassVolume") || name.equals("SetAudioSRSSurround")
                        || name.equals("SetAudioSrsDialogClarity") || name.equals("SetAudioSrsTruBass") || name.equals("SetBaseColorMode")
                        || name.equals("SetAudioSupperBassSwitch")||name.equals("SetAudioSPDIFMode"))
                {
                    TVDeviceImpl.tv.TvITFExecute(name, userValue);
                    TVDeviceImpl.tv.TvITFExecute(saveName, userValue);
                }
                else if (name.equals("SetPowerOnOffChannel")) 
                {
                    TVDeviceImpl.tv.TvITFExecute("SSMSavePowerOnOffChannel", userValue);
                }
				else if(name.equals("SetParentControlSwitch"))
				{
					TVDeviceImpl.tv.TvITFExecute("SSMSaveParentalControlSwitch", userValue);
				}
				else if(name.equals("SetPowerOnMusicSwitch"))
				{
				    TVDeviceImpl.tv.TvITFExecute("SSMSavePowerOnMusicSwitch", userValue);
				}
                else if (name.equals("SetRGBOGOGainR100")||name.equals("SetRGBOGOGainG100")||name.equals("SetRGBOGOGainB100"))
                {
                    TVDeviceImpl.tv.TvITFExecute(name, userValue);
                }
                else if(name.equals("SetAudioMasterVolume") || name.equals("SetAudioVolumeCompensationVal"))
                {
                    TVDeviceImpl.tv.TvITFExecute(name, userValue);
                }
                else
                {
                    TVDeviceImpl.tv.TvITFExecute(name, userValue, mysource);
                }

                break;
        }

    }

	private boolean IsFmtHighResolution()
	{
		if(tv.GetSrcInputType() != Tv.Source_Input_Type.SOURCE_TYPE_HDMI)
			return false;
		tvin_info_t tv_info = tv.GetCurrentSignalInfo();
		String[] items = null;
		if(tv_info != null){
			items = tv_info.fmt.toString().split("_");
			if(items[4].equals("1920X1200") || items[4].equals("1920x1200"))
				return true;
		}
		return false;
    }

	
	private int get_screen_mode( int value )
		{
			int index = value ;
			switch( value )
			{
			case 0:
				value = 0;//full
				break;
			case 1:
				value = 4;//4:3
				break;
			case 2:
				value = 5;//panorama
				break;
			case 3:
				value = 2;//zoom1
				break;
			case 4:
				value = 10;//zoom2
				break;
			case 5:
				value = 7;//Ptop
				break;
			case 6:
				break;
			case 7:
				break;
			default:
				break;
			}
			Log.d(TAG, " reset index : " + index + " to DisplayMode : " + value );
			return value;
		}

    @Override
    public TVConfigValue read(String name, TVConfigEntry entry)
    {
        // TODO Auto-generated method stub
        /*****************tv setting***********************/
        Log.v(TAG, "GET : " + name);
        String configType = "Get";
    
   
        if (TVDeviceImpl.tv == null)
            try
            {
                throw new TVException();
            }
            catch (TVException e)
            {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        int pos = name.indexOf(":");
        if (pos == -1)
            Log.e(TAG, "set name fail");
        name = name.substring(pos + 1);
        
    
        
        pos = name.indexOf(":");
        if (pos == -1)
            Log.e(TAG, "set name fail");
        
        String source = name.substring(0, pos);
                
        int    mysource = Integer.valueOf(source);   
        Log.d(TAG, "Source Input Type" + mysource);
        
        name = name.substring(pos + 1);
              
        name = configType + name;
        Log.v(TAG, "GET NEW " + name);
        TVConfigValue myvalue = null;
//        int val = TVDeviceImpl.tv.GetSrcInputType().ordinal();

        tvin_info_t sig_fmt = TVDeviceImpl.tv.GetCurrentSignalInfo();
        int fmt = sig_fmt.fmt.ordinal();
        if (name.equals("GetAudioSoundMode") || name.equals("GetAudioSupperBassVolume") || name.equals("GetAudioSRSSurround")
                || name.equals("GetAudioSrsDialogClarity") || name.equals("GetAudioSrsTruBass") || name.equals("GetBaseColorMode")
                || name.equals("GetAudioSupperBassSwitch")||name.equals("GetAudioSPDIFMode"))
        {

            myvalue = new TVConfigValue(TVDeviceImpl.tv.TvITFExecute(name));
			entry.setCacheable(false);
            return myvalue;
        }
        else if (name.equals("GetAudioTrebleVolume") || name.equals("GetAudioBassVolume") || name.equals("GetAudioBalance"))
        {
            myvalue = new TVConfigValue(TVDeviceImpl.tv.TvITFExecute(name));
            entry.setCacheable(false);
            return myvalue;
        }
        else if (name.equals("GetVGAPhase") || name.equals("GetVGAClock") || name.equals("GetVGAHPos") || name.equals("GetVGAVPos"))
        {
            myvalue = new TVConfigValue(TVDeviceImpl.tv.TvITFExecute(name, fmt));
            entry.setCacheable(false);
            return myvalue;
        }
        else if (name.equals("GetRGBOGOGainR100") || name.equals("GetRGBOGOGainG100") || name.equals("GetRGBOGOGainB100"))
        {
            myvalue = new TVConfigValue(TVDeviceImpl.tv.TvITFExecute(name));
            entry.setCacheable(false);
            try
            {
                Log.d(TAG, "get" + name + ":" + myvalue.getInt());
            }
            catch (TypeException e)
            {
                e.printStackTrace();
            }
            return myvalue;
        }
				else if (name.equals("GetPowerOnOffChannel")) 
        {
            myvalue = new TVConfigValue(TVDeviceImpl.tv.TvITFExecute("SSMReadPowerOnOffChannel"));
            entry.setCacheable(false);
            return myvalue;
        }
        else if (name.equals("GetParentControlSwitch")) 
        {
            myvalue = new TVConfigValue(TVDeviceImpl.tv.TvITFExecute("SSMReadParentalControlSwitch"));
            return myvalue;
        }
        else if(name.equals("GetPowerOnMusicSwitch"))
        {
            myvalue = new TVConfigValue(TVDeviceImpl.tv.TvITFExecute("SSMReadPowerOnMusicSwitch"));
			entry.setCacheable(false);
            try
            {
                Log.d(TAG, "*******************myvalue=" + myvalue.getInt());
            }
            catch (Exception e)
            {
            }
            return myvalue;
        }
        else if(name.equals("GetAudioMasterVolume") || name.equals("GetAudioVolumeCompensationVal"))
        {
            myvalue = new TVConfigValue(TVDeviceImpl.tv.TvITFExecute(name));
            entry.setCacheable(false);
            return myvalue;
        }
        else
        {
            myvalue = new TVConfigValue(TVDeviceImpl.tv.TvITFExecute(name, mysource));
            entry.setCacheable(false);
            return myvalue;
        }
    }

	public void setSecRequest(int secType, TVChannelParams secCurParams, int secPositionerMoveUnit)
	{
	}


    private void SetWindowSize(int mode ,int x , int y , int w , int h) {
        Log.d(TAG,"SetWindowSize========================"+mode+"x="+x+"y="+y+"w="+w+"h="+h);
        //w=w+x;
        //h=h+y;
        /*String hole=null;
        if(mode==1)
           hole = ""+x+" "+y+" "+w+" "+h+" "+mode;
        else
           hole = ""+x+" "+y+" "+w+" "+h+" "+mode;
        try {
            BufferedWriter writer = new BufferedWriter(new FileWriter("/sys/class/graphics/fb0/video_hole"));
            try {
                writer.write(hole);
                } finally {
                    writer.close();
                }
        }catch (FileNotFoundException e) {
            e.printStackTrace();
        }catch (Exception e) {
                Log.e(TAG,"set  /sys/class/graphics/fb0/video_hole ERROR!",e);
        } 
    //  SystemProperties.set("hw.videohole.enable", "true");
        */
    }
}
class SingletonTv
{
    static Tv instance = null;

    public synchronized static Tv getTvInstance()
    {
        if (instance == null)
        {
            instance = Tv.open();
        }
        return instance;
    }
}
