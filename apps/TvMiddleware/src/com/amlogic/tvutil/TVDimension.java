package com.amlogic.tvutil;

import android.database.Cursor;
import android.content.Context;
import android.util.Log;
import com.amlogic.tvdataprovider.TVDataProvider;

/**
 *TV ATSC rating dimension
 */
public class TVDimension{
	private static final String TAG="TVDimension";

	/**Rating regions*/
	public static final int REGION_US = 0x1;
	public static final int REGION_CANADA = 0x2;
	public static final int REGION_TAIWAN = 0x3;
	public static final int REGION_SOUTHKOREA = 0x4;
	
	private Context context;
	private int id;
	private int indexj;
	private int ratingRegion;
	private int graduatedScale;
	private int[] lockValues;
	private String name;
	private String ratingRegionName;
	private String[] abbrevValues;
	private String[] textValues;
	private boolean isPGAll;
	/*set tv none block, all programe is block*/
	public boolean isNoneBlock;

	TVDimension(Context context, Cursor c){
		this.context = context;

		int col;

		col = c.getColumnIndex("db_id");
		this.id = c.getInt(col);

		col = c.getColumnIndex("index_j");
		this.indexj = c.getInt(col);
		
		col = c.getColumnIndex("rating_region");
		this.ratingRegion = c.getInt(col);
		
		col = c.getColumnIndex("graduated_scale");
		this.graduatedScale = c.getInt(col);
		
		col = c.getColumnIndex("name");
		this.name = c.getString(col);
		
		col = c.getColumnIndex("rating_region_name");
		this.ratingRegionName = c.getString(col);
		
		col = c.getColumnIndex("values_defined");
		int valuesDefined = c.getInt(col);
		this.lockValues = new int[valuesDefined];
		this.abbrevValues = new String[valuesDefined];
		this.textValues = new String[valuesDefined];
		for (int i=0; i<valuesDefined; i++){
			col = c.getColumnIndex("abbrev"+i);
			this.abbrevValues[i] = c.getString(col);
			col = c.getColumnIndex("text"+i);
			this.textValues[i] = c.getString(col);
			col = c.getColumnIndex("locked"+i);
			this.lockValues[i] = c.getInt(col);
		}
		
		if (ratingRegion == REGION_US && name.equals("All")){
			isPGAll = true;
			if(this.lockValues[0] == 1){
				isNoneBlock = true;
			}
		}else{
			isPGAll = false;
			isNoneBlock = false;
		}
	}
	
	public class VChipRating{
		private int region;
		private int dimension;
		private int value;
		
		public VChipRating(int region, int dimension, int value){
			this.region = region;
			this.dimension = dimension;
			this.value = value;
		}
		
		public int getRegion(){
			return region;
		}
		
		public int getDimension(){
			return dimension;
		}
		
		public int getValue(){
			return value;
		}
	}
	
	public TVDimension(){
	
	}

	/* 'All' is a very special case, it links to dimension0 & dimension5 */
	private int getUSPGAllLockStatus(String abbrev){
		TVDimension dm0 = selectByIndex(context, REGION_US, 0);
		TVDimension dm5 = selectByIndex(context, REGION_US, 5);
		String[] dm0Abbrev = dm0.getAbbrev();
		String[] dm5Abbrev = dm5.getAbbrev();
		
		for (int j=0; j<dm0Abbrev.length; j++){
			Log.d(TAG, dm0Abbrev[j]);
			if (abbrev.equals(dm0Abbrev[j])){
				return dm0.getLockStatus(j+1);
			}
		}
		for (int j=0; j<dm5Abbrev.length; j++){
			Log.d(TAG, dm0Abbrev[j]);
			if (abbrev.equals(dm5Abbrev[j])){
				return dm5.getLockStatus(j+1);
			}
		}
		
		return -1;
	}
	
	private void setUSPGAllLockStatus(String abbrev, int lock){
		TVDimension dm0 = selectByIndex(context, REGION_US, 0);
		TVDimension dm5 = selectByIndex(context, REGION_US, 5);
		String[] dm0Abbrev = dm0.getAbbrev();
		String[] dm5Abbrev = dm5.getAbbrev();

		for (int j=0; j<dm0Abbrev.length; j++){
			if (abbrev.equals(dm0Abbrev[j])){
				dm0.setLockStatus(j+1, lock);
				return;
			}
		}
		for (int j=0; j<dm5Abbrev.length; j++){
			if (abbrev.equals(dm5Abbrev[j])){
				dm5.setLockStatus(j+1, lock);
				return;
			}
		}
		
		return;
	}
	
	private int[] getUSPGAllLockStatus(String[] abbrevs){
		int[] lockAll = new int[abbrevs.length];
		
		for (int i=0; i<abbrevs.length; i++){
			lockAll[i] = getUSPGAllLockStatus(abbrevs[i]);
		}
		
		return lockAll;
	}
	
	private void setUSPGAllLockStatus(String[] abbrevs, int[] lock){
		for (int i=0; i<abbrevs.length; i++){
			setUSPGAllLockStatus(abbrevs[i], lock[i]);
		}
	}
	/**
	 *????????????ID???????????????TVDimension
	 *@param context ??????Context
	 *@param id ??????ID
	 *@return ??????ID?????????TVDimension??????
	 */
	public static TVDimension selectByID(Context context, int id){
		TVDimension e = null;

		Cursor c = context.getContentResolver().query(TVDataProvider.RD_URL,
				null,
				"select * from dimension_table where evt_table.db_id = " + id,
				null, null);
		if(c != null){
			if(c.moveToFirst()){
				e = new TVDimension(context, c);
			}
			c.close();
		}

		return e;
	}
	
	/**
	 *????????????ID???????????????TVDimension
	 *@param context ??????Context
	 *@param ratingRegionID rating region ID
	 *@return ??????ID?????????TVDimension??????
	 */
	public static TVDimension[] selectByRatingRegion(Context context, int ratingRegionID){
		TVDimension[] d = null;

		Cursor c = context.getContentResolver().query(TVDataProvider.RD_URL,
				null,
				"select * from dimension_table where rating_region = " + ratingRegionID,
				null, null);
		if(c != null){
			if(c.moveToFirst()){
				int id = 0;
				d = new TVDimension[c.getCount()];
				do{
					d[id++] = new TVDimension(context, c);
				}while(c.moveToNext());
			}
			c.close();
		}

		return d;
	}
	
	/**
	 *????????????ID???????????????TVDimension
	 *@param context ??????Context
	 *@param ratingRegionID rating region ID
	 *@param index RRT????????????index_j
	 *@return ???????????????TVDimension??????
	 */
	public static TVDimension selectByIndex(Context context, int ratingRegionID, int index){
		TVDimension d = null;
		String cmd = "select * from dimension_table where rating_region = " + ratingRegionID;
		cmd += " and index_j=" + index;
		Cursor c = context.getContentResolver().query(TVDataProvider.RD_URL,
				null, cmd, null, null);
		if(c != null){
			if(c.moveToFirst()){
				d = new TVDimension(context, c);
			}
			c.close();
		}

		return d;
	}
	
	/**
	 *??????ID????????????????????????TVDimension
	 *@param context ??????Context
	 *@param ratingRegionID rating region ID
	 *@param dimensionName dimension?????????
	 *@return ???????????????TVDimension??????
	 */
	public static TVDimension selectByName(Context context, int ratingRegionID, String dimensionName){
		TVDimension d = null;
		String cmd = "select * from dimension_table where rating_region = " + ratingRegionID;
		cmd += " and name='" + dimensionName + "'";
		Cursor c = context.getContentResolver().query(TVDataProvider.RD_URL,
				null, cmd, null, null);
		if(c != null){
			if(c.moveToFirst()){
				d = new TVDimension(context, c);
			}
			c.close();
		}

		return d;
	}
	
	/**
	 *??????US downloadable TVDimension
	 *@return ???????????????TVDimension????????????
	 */
	public static TVDimension[] selectUSDownloadable(Context context){
		TVDimension[] d = null;
		String cmd = "select * from dimension_table where rating_region >= 5";
		Cursor c = context.getContentResolver().query(TVDataProvider.RD_URL,
				null, cmd, null, null);
		if(c != null){
			if(c.moveToFirst()){
				int id = 0;
				d = new TVDimension[c.getCount()];
				do{
					d[id++] = new TVDimension(context, c);
				}while(c.moveToNext());
			}
			c.close();
		}

		return d;
	}
	
	/**
	 *????????????rating_value????????????block
	 *@param context ??????Context
	 *@param definedRating content_advisory_descr????????????????????????
	 *@return ??????block
	 */
	public static boolean isBlocked(Context context, VChipRating definedRating){

		TVDimension dmNone = selectByName(context, TVDimension.REGION_US, "All");

		if(dmNone.getNoneLockStatus() == true){
			/*is set tv none, all programe is block*/
			Log.d(TAG,"isBlocked isNoneBlock:"+true);
			return true;
		} else {
			Log.d(TAG,"isBlocked false isNoneBlock:"+false);
		}

		if (definedRating != null){
			Log.d(TAG, "isBlocked: REG:"+definedRating.getRegion()+", DIMEN "+definedRating.getDimension()+", getValue "+definedRating.getValue());
			TVDimension dm = selectByIndex(context, definedRating.getRegion(), definedRating.getDimension());
			if (dm != null){
				return (dm.getLockStatus(definedRating.getValue()) == 1);
			}
		} else {
			Log.d(TAG,"isBlocked definedRating is null");
		}
		
		return false;
	}
	
	/**
	 *???????????????ID
	 *@return ???????????????ID
	 */
	public int getID(){
		return id;
	}

	/**
	 *?????? rating region ID
	 *@return ?????? rating region ID
	 */
	public int getRatingRegion(){
		return ratingRegion;
	}

	/**
	 *?????? rating region ??????
	 *@return ?????? rating region ??????
	 */
	public String getRatingRegionName(){
		return ratingRegionName;
	}
	
	/**
	 *??????Dimension??????
	 *@return ??????Dimension??????
	 */
	public String getName(){
		return name;
	}
	
	/**
	 *??????graduated scale??????
	 *@return ??????graduated scale??????
	 */
	public int getGraduatedScale(){
		return graduatedScale;
	}
	
	public boolean getNoneLockStatus(){
		int[] Rating_status_ALL={1,1,1,1,1,1,1,};
		String[] abb={"TV-NONE","TV-Y","TV-Y7","TV-G","TV-PG","TV-14","TV-MA"};
		Rating_status_ALL = getLockStatus(abb);
		if(Rating_status_ALL[0]==1){
			isNoneBlock = true;
		} else {
			isNoneBlock = false;
		}
		Log.d(TAG,"getNoneLockStatus isNoneBlock:"+isNoneBlock);
		return isNoneBlock;
	}
	/**
	 *?????????dimension?????????values???????????????
	 *@return ????????????values??????????????????0-????????????-1-???????????????????????????????????????????????????-?????????
	 */
	public int[] getLockStatus(){
		if (lockValues.length > 1){
			if (isPGAll){
				return getUSPGAllLockStatus(abbrevValues);
			}else{
				int[] l = new int[lockValues.length - 1];
				System.arraycopy(lockValues, 1, l, 0, l.length);
				return l;
			}
		}else{
			return null;
		}
	}
	
	/**
	 *?????????dimension?????????value???????????????
	 *@param valueIndex value??????
	 *@return ????????????value??????????????????0-????????????-1-???????????????????????????????????????????????????-?????????
	 */
	public int getLockStatus(int valueIndex){
		if (valueIndex >= lockValues.length){
			return -1;
		}else{
			if (isPGAll){
				return getUSPGAllLockStatus(abbrevValues[valueIndex]);
			}else{
				return lockValues[valueIndex];
			}
		}
	}
	
	/**
	 *?????????dimension???????????????values???????????????
	 *@param abbrevs ???????????????value???abbrev??????
	 *@return ????????????values??????????????????0-????????????-1-???????????????????????????????????????????????????-?????????
	 */
	public int[] getLockStatus(String[] abbrevs){
		int l[] = null;
		
		if (abbrevs != null){
			if (isPGAll){
				return getUSPGAllLockStatus(abbrevs);
			}else{
				l = new int[abbrevs.length];
				for (int i=0; i<abbrevs.length; i++){
					l[i] = -1;
					for (int j=0; j<abbrevValues.length; j++){
						if (abbrevs[i].equals(abbrevValues[j])){
							l[i] = lockValues[j];
							break;
						}
					}
				}
			}
		}
		
		return l;
	}

	/**
	 *?????????dimension?????????values???abbrev text
	 *@return ????????????values???abbrev text
	 */
	public String[] getAbbrev(){
		/* the first rating_value must be not visible to user */
		if (abbrevValues.length > 1){
			String[] a = new String[abbrevValues.length - 1];
			System.arraycopy(abbrevValues, 1, a, 0, a.length);
			return a;
		}else{
			return null;
		}
	}
	
	/**
	 *?????????dimension??????value???abbrev text
	 *@return ??????abbrev text
	 */
	public String getAbbrev(int valueIndex){
		if (valueIndex >= abbrevValues.length)
			return null;
		else
			return abbrevValues[valueIndex];
	}

	/**
	 *?????????dimension?????????values???value text
	 *@return ????????????values???value text
	 */
	public String[] getText(){
		if (textValues.length > 1){
			String[] t = new String[textValues.length - 1];
			System.arraycopy(textValues, 1, t, 0, t.length);
			return t;
		}else{
			return null;
		}
	}
	
	/**
	 *?????????dimension??????value???value text
	 *@return ??????value text
	 */
	public String getText(int valueIndex){
		if (valueIndex >= textValues.length)
			return null;
		else
			return textValues[valueIndex];
	}
	
	/**
	 *????????????value???????????????
	 *@param valueIndex value??????
	 *@param status ????????????
	 */
	public void setLockStatus(int valueIndex, int status){
		if (valueIndex >= lockValues.length)
			return;
			
		if (isPGAll){
			setUSPGAllLockStatus(abbrevValues[valueIndex], lockValues[valueIndex]);
		}else{
			if (lockValues[valueIndex] != -1 && lockValues[valueIndex] != status){
				lockValues[valueIndex] = status;
				String cmd = "update dimension_table set locked" + valueIndex;
				cmd += "=" + status + " where db_id = " + id;
				context.getContentResolver().query(TVDataProvider.WR_URL,
					null, cmd, null, null);
			}
		}
	}
	
	/**
	 *?????????dimension??????values???????????????
	 *@param status ????????????
	 */
	public void setLockStatus(int[] status){
		if (status == null || status.length != (lockValues.length-1)){
			Log.d(TAG, "Cannot set lock status, invalid param");
			return;
		}
		
		if (isPGAll){
			setUSPGAllLockStatus(abbrevValues, status);
		}else{
			for (int i=0; i<status.length; i++){
				setLockStatus(i+1, status[i]);
			}
		}
	}
	
	/**
	 *????????????values???????????????
	 *@param abbrevs abbrev??????
	 *@param locks ??????????????????abbrev???????????????????????????
	 */
	public void setLockStatus(String[] abbrevs, int[] locks){
		if (abbrevs == null || locks == null)
			return;
		if (abbrevs.length != locks.length){
			Log.d(TAG, "Invalid abbrevs or locks, length must be equal");
			return;
		}
		
		if (isPGAll){
			setUSPGAllLockStatus(abbrevs, locks);
		}else{
			for (int i=0; i<abbrevs.length; i++){
				for (int j=0; j<abbrevValues.length; j++){
					if (abbrevs[i].equals(abbrevValues[j])){
						setLockStatus(j, locks[i]);
						break;
					}
				}
			}
		}
	}
}
