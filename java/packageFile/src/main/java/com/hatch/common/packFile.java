/**
 * @author 
 * @version 2018年7月23日
 */
package com.hatch.common;

/**
 * @author zpzhao
 * @version 2018年7月23日
 */
public class packFile {
	
	
	/**
	 * 
	 */
	public packFile() {
		// TODO Auto-generated constructor stub
	}


	

}

/**
 * @author zpzhao
 * @version 2018年7月23日
 */
class packFileHeader
{
	/**
	 * pack header
	 *  packheader size
	 *  packVer
	 *  packflag = onlyfile 1/slidfile 2
	 *  packnum = (packflag = slidfile)
	 *  packctime= create time
	 *  packutime= update time
	 *  packfilesize = except header
	 *  packreserve1
	 *  packreserve2
	 *  
	 *   
	 */
	private long packHeaderSize;
	private int packVer = 0x1001;
	private int packFlag;
	private int packNum;
	private long packctime;
	private long packutime;
	private long packFileSize;
	private int packreserver1;
	private int packreserver2;
	
	
	public int getPackFlag() {
		return packFlag;
	}


	public void setPackFlag(int packFlag) {
		this.packFlag = packFlag;
	}


	public int getPackNum() {
		return packNum;
	}


	public void setPackNum(int packNum) {
		this.packNum = packNum;
	}


	public long getPackctime() {
		return packctime;
	}


	public void setPackctime(long packctime) {
		this.packctime = packctime;
	}


	public long getPackutime() {
		return packutime;
	}


	public void setPackutime(long packutime) {
		this.packutime = packutime;
	}


	public long getPackFileSize() {
		return packFileSize;
	}


	public void setPackFileSize(long packFileSize) {
		this.packFileSize = packFileSize;
	}


	public int getPackreserver1() {
		return packreserver1;
	}


	public void setPackreserver1(int packreserver1) {
		this.packreserver1 = packreserver1;
	}


	public int getPackreserver2() {
		return packreserver2;
	}


	public void setPackreserver2(int packreserver2) {
		this.packreserver2 = packreserver2;
	}


	/**
	 * @return the packVer
	 */
	public int getPackVer() {
		return packVer;
	}


	/**
	 * @param packVer the packVer to set
	 */
	public void setPackVer(int packVer) {
		this.packVer = packVer;
	}


	/**
	 * @return the packHeaderSize
	 */
	public long getPackHeaderSize() {
		return packHeaderSize;
	}


	/**
	 * @param packHeaderSize the packHeaderSize to set
	 */
	public void setPackHeaderSize(long packHeaderSize) {
		this.packHeaderSize = packHeaderSize;
	}
	
}
