package com.amlogic.tvutil;

import java.util.Locale;
import android.database.Cursor;
import android.content.Context;
import android.util.Log;
import com.amlogic.tvdataprovider.TVDataProvider;


/**
 *TVMultilingualText
 *multilingual text parsing
 */
public class TVMultilingualText{
	private static final String TAG="TVMultilingualText";

	private class MultilingualText{
		private String language;
		private String text;

		public MultilingualText(Context context, String formatString){
			if (formatString != null && formatString.length() >= 3){
				language = formatString.substring(0, 3);
				/*there is no iso-descr in service_descr/SDT, xxx indicate it*/
				if (language.equalsIgnoreCase("xxx")){
					language = TVConfigResolver.getConfig(context, 
								"tv:scan:dtv:default_text_language", 
								"eng");
				}
				if (formatString.length() > 3){
						text = formatString.substring(3, formatString.length());
				}else{
					text = "";
				}
			}else{
				language = "";
				text     = "";
			}

		}

		public String getLangage(){
			return language;
		}

		public String getText(){
			return text;
		}
	}

	public static String getText(Context context, String formatText, String lang){
		String ret = "";
		boolean useFirst = false;
		String split;
		
		if (formatText == null || lang == null || formatText.isEmpty())
			return ret;

		/* special case for 'local' and 'first' */
		if (lang.equalsIgnoreCase("local")){
			Locale defaultLocale = Locale.getDefault();
			/* recover lang by the current local Android language */
			if (defaultLocale.equals(Locale.SIMPLIFIED_CHINESE)){
				lang = "chs";
			}else if (defaultLocale.equals(Locale.TRADITIONAL_CHINESE)){
				lang = "chi";
			}else{
				lang = Locale.getDefault().getISO3Language();
			}			
		}else if (lang.equalsIgnoreCase("first")){
			useFirst = true;
		}
		
		if (formatText.contains(new String(new byte[]{(byte)0x80}))){
			split = new String(new byte[]{(byte)0x80});
		}else{
			split = new String(new char[]{(char)0x80});
		}
		String[] langText = formatText.split(split);
		for (int i=0; langText!=null && i<langText.length; i++){
			TVMultilingualText inst = new TVMultilingualText();
			MultilingualText text = inst.new MultilingualText(context, langText[i]);

			if (useFirst || text.getLangage().equalsIgnoreCase(lang)){
				ret = text.getText();
				break;
			}
		}
		
		return ret;
	}
	
	public static String getText(Context context, String formatText){
		String ret = "";

		/* read the current config */
		String configLangs = TVConfigResolver.getConfig(context, 
			"tv:scan:dtv:ordered_text_languages", 
			"local first");

		if (configLangs == null || configLangs.isEmpty()){
			return ret;
		}

		/* get the correct text according to config */
		String[] langs = configLangs.split(" ");
		if (langs != null && langs.length > 0){
			for (int i=0; i<langs.length; i++){
				ret = getText(context, formatText, langs[i]);
				if (ret != null && !ret.isEmpty()){
					break;
				}
			}
		}

		return ret;
	}
}

