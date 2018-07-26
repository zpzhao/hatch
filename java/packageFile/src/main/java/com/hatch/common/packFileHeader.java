package com.hatch.common;

import com.hatch.master.Master;

/**
 * @author zpzhao
 * @version 2018年7月23日
 */
public class packFileHeader
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
	private int packHeaderSize;
	private int packVer = 0x1001;
	private int packFlag;
	private int packNum;
	private int packctime;
	private int packutime;
	private int packFileSize;
	private int packreserver1;
	private int packreserver2;
	
	
	public packFileHeader() {
		packHeaderSize = 0;
		packFlag = 0;
		packNum = 0;
		packctime = 0;
		packutime = 0;
		packFileSize = 0;
		packreserver1 = 0;
		packreserver2 = 0;
	}
	
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


	public int getPackctime() {
		return packctime;
	}


	public void setPackctime(int packctime) {
		if(0 == this.packctime)
			this.packctime = packctime;
	}


	public int getPackutime() {
		return packutime;
	}


	public void setPackutime(int packutime) {
		this.packutime = packutime;
	}


	public int getPackFileSize() {
		return packFileSize;
	}


	public void setPackFileSize(int packFileSize) {
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
	public int getPackHeaderSize() {
		return packHeaderSize;
	}


	/**
	 * @param packHeaderSize the packHeaderSize to set
	 */
	public void setPackHeaderSize(int packHeaderSize) {
		this.packHeaderSize = packHeaderSize;
	}


	/* (non-Javadoc)
	 * @see java.lang.Object#hashCode()
	 */
	@Override
	public int hashCode() {
		final int prime = 31;
		int result = 1;
		result = prime * result + packFileSize;
		result = prime * result + packFlag;
		result = prime * result + packHeaderSize;
		result = prime * result + packNum;
		result = prime * result + packVer;
		result = prime * result + packctime;
		result = prime * result + packreserver1;
		result = prime * result + packreserver2;
		result = prime * result + packutime;
		return result;
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#equals(java.lang.Object)
	 */
	@Override
	public boolean equals(Object obj) {
		if (this == obj) {
			return true;
		}
		if (obj == null) {
			Master.getAplog().InfoLog("obj == null", "packFileHeader", "equals");
			return false;
		}
		if (!(obj instanceof packFileHeader)) {
			Master.getAplog().InfoLog("not instance of", "packFileHeader", "equals");
			return false;
		}
		packFileHeader other = (packFileHeader) obj;
		if (packFileSize != other.packFileSize) {
			Master.getAplog().InfoLog("packFileSize["+packFileSize+"<>"+other.packFileSize+"]", "packFileHeader", "equals");
			return false;
		}
		if (packFlag != other.packFlag) {
			Master.getAplog().InfoLog("packFlag["+packFlag+"<>"+other.packFlag+"]", "packFileHeader", "equals");
			return false;
		}
		if (packHeaderSize != other.packHeaderSize) {
			Master.getAplog().InfoLog("packHeaderSize["+packHeaderSize+"<>"+other.packHeaderSize+"]", "packFileHeader", "equals");
			return false;
		}
		if (packNum != other.packNum) {
			Master.getAplog().InfoLog("packNum["+packNum+"<>"+other.packNum+"]", "packFileHeader", "equals");
			return false;
		}
		if (packVer != other.packVer) {
			Master.getAplog().InfoLog("packVer["+packVer+"<>"+other.packVer+"]", "packFileHeader", "equals");
			return false;
		}
		if (packctime != other.packctime) {
			Master.getAplog().InfoLog("packctime["+packctime+"<>"+other.packctime+"]", "packFileHeader", "equals");
			return false;
		}
		if (packreserver1 != other.packreserver1) {
			Master.getAplog().InfoLog("packreserver1["+packreserver1+"<>"+other.packreserver1+"]", "packFileHeader", "equals");
			return false;
		}
		if (packreserver2 != other.packreserver2) {
			Master.getAplog().InfoLog("packreserver2["+packreserver2+"<>"+other.packreserver2+"]", "packFileHeader", "equals");
			return false;
		}
		if (packutime != other.packutime) {
			Master.getAplog().InfoLog("packutime["+packutime+"<>"+other.packutime+"]", "packFileHeader", "equals");
			return false;
		}
		return true;
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	@Override
	public String toString() {
		return "packFileHeader [packHeaderSize=" + packHeaderSize + ", packVer=" + packVer + ", packFlag=" + packFlag
				+ ", packNum=" + packNum + ", packctime=" + packctime + ", packutime=" + packutime + ", packFileSize="
				+ packFileSize + ", packreserver1=" + packreserver1 + ", packreserver2=" + packreserver2 + "]";
	}
	
}
