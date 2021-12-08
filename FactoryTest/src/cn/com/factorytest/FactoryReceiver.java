package cn.com.factorytest;

import java.io.File;
import java.io.IOException;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Environment;
import android.util.Log;

public class FactoryReceiver extends BroadcastReceiver{
	private static final String TAG = Tools.TAG;
	//检测U盘 udiskfile 启动产测apk
	private static final String udiskfile = "khadas_test.xml";
	private static final String rebootfile = "khadas_reboot.xml";
	@Override
	public void onReceive(Context context, Intent intent) {
		// TODO Auto-generated method stub
		String action = intent.getAction();
		Uri uri = intent.getData();
		if (uri.getScheme().equals("file")) {
			String path = uri.getPath();
            String externalStoragePath = Environment.getExternalStorageDirectory().getPath();
            String legacyPath = Environment.getLegacyExternalStorageDirectory().getPath();

            try {
                path = new File(path).getCanonicalPath();
            } catch (IOException e) {
                Log.e(TAG, "couldn't canonicalize " + path);
                return;
            }
            if (path.startsWith(legacyPath)) {
                path = externalStoragePath + path.substring(legacyPath.length());                                                          
            }

			if (Intent.ACTION_MEDIA_MOUNTED.equals(action)) {
				String rebootfullpath = path+"/"+rebootfile;
				File rebootfile = new File(rebootfullpath);
				if(rebootfile.exists() && rebootfile.isFile()){
					try {
					Thread.sleep(10000);
					Process proc = Runtime.getRuntime().exec(new String[]{"reboot"});
					proc.waitFor();
					} catch (Exception e){
						e.printStackTrace();
					}
					return;
				}
				String fullpath = path+"/"+udiskfile;
				File file = new File(fullpath);
				 if(file.exists() && file.isFile()){
					 try {
						Thread.sleep(5000);
					 } catch (InterruptedException e) {
						 e.printStackTrace();
					 }
					 Intent i = new Intent();
					 i.setClassName("cn.com.factorytest", "cn.com.factorytest.MainActivity");
					 i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
					 context.startActivity(i);
				 }
			}
		}
		
	}
	
}
