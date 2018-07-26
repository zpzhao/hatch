/**
 * @author 
 * @version 2018年7月23日
 */
package com.hatch.common;

import java.io.IOException;
import java.io.UnsupportedEncodingException;

import com.hatch.log.hatchLog;

/**
 * @author zpzhao
 * @version 2018年7月23日
 */
public class packFile {
	private packFileHeader pfh;
	private fileHeader fh;
	private FileHandle pFile;
	private FileHandle inFile;
	
	private long packFileLimit = 1024*1024*1024;
	private int packFileNum = 0;
	private hatchLog aplog;
		
	/**
	 * 
	 */
	public packFile() {
		// TODO Auto-generated constructor stub
	}

	public packFile(FileHandle pack) {
		pFile = pack;
	}
	
	public packFile(hatchLog packlog) {
		aplog = packlog;
	}
	
	/**
	 * open pack file
	 * @param pfile
	 */
	public int OpenpackFile(String pfile) {
		if(null != pFile)
		{
			aplog.WarnLog("pack file not null", "packFile", "OpenpackFile");
			return -1;
		}
		
		pFile = new FileHandle();
		pFile.OpenWriteFile(pfile);
		pfh = new packFileHeader();
		return 0;
	}
	
	/**
	 * open read file to packed
	 */
	public void OpenReadFile(String rd) {
		inFile = new FileHandle();
		inFile.OpenReadFile(rd);
		
		/* file header infomation */
		fh = new fileHeader();
		fh.setFileName(rd);
		fh.setFilesize(inFile.GetFileSize());
		
		if(fh.getFilesize()+pfh.getPackFileSize() > packFileLimit)
		{
			fh.setFileFlag(2);
			fh.setFileNum(1);
		}
		fh.AdustHeader();
	}
	
	/**
	 * update information to memory
	 * @param ph
	 */
	public void UpdatePackHeader(packFileHeader ph) {
		pfh.setPackctime(ph.getPackctime());
		pfh.setPackFileSize(ph.getPackFileSize());
		pfh.setPackFlag(ph.getPackFlag());
		pfh.setPackHeaderSize(ph.getPackHeaderSize());
		
		pfh.setPackNum(ph.getPackNum());
		pfh.setPackutime(ph.getPackutime());
		pfh.setPackVer(ph.getPackVer());
		
		pfh.setPackreserver1(ph.getPackreserver1());		
		pfh.setPackreserver2(ph.getPackreserver2());		
	}
	
	public void closePackFile() throws IOException {
		pFile.CloseFile();
	}
	
	/**
	 * 
	 */
	public int WritePackHeader() {
		if(null == pfh)
		{
			aplog.SevereLog("pack file header is null.", "packFile", "OpenpackFile");
			return -1;
		}
		
		try {
			int len = Integer.SIZE/8;
			pFile.WriteFile(packFile.intToBytes(pfh.getPackHeaderSize()), 0, len);
			pFile.WriteFile(packFile.intToBytes(pfh.getPackVer()), 0, len);
			pFile.WriteFile(packFile.intToBytes(pfh.getPackFlag()), 0, len);
			pFile.WriteFile(packFile.intToBytes(pfh.getPackNum()), 0, len);
			pFile.WriteFile(packFile.intToBytes(pfh.getPackctime()), 0, len);
			pFile.WriteFile(packFile.intToBytes(pfh.getPackutime()), 0, len);
			pFile.WriteFile(packFile.intToBytes(pfh.getPackFileSize()), 0, len);
			pFile.WriteFile(packFile.intToBytes(pfh.getPackreserver1()), 0, len);
			pFile.WriteFile(packFile.intToBytes(pfh.getPackreserver2()), 0, len);
			
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
		return 0;
	}
	
	public int[] ReadPackHeader() {
		pfh = new packFileHeader();
		
		byte[] bt = new byte[4];
		int[] v = new int[9];
		try {
			for(int i = 0; i < 9; i++)
			{
				inFile.ReadFile(bt, 0, 4);
				v[i] = packFile.bytesToInt(bt);
			}
			
			pfh.setPackHeaderSize(v[0]);
			pfh.setPackVer(v[1]);
			pfh.setPackFlag(v[2]);
			pfh.setPackNum(v[3]);
			pfh.setPackctime(v[4]);
			pfh.setPackutime(v[5]);
			pfh.setPackFileSize(v[6]);
			pfh.setPackreserver1(v[7]);
			pfh.setPackreserver1(v[8]);
			
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
				
		return v;
	}
	
	public int WriteFileHeader() {
		return 0;
	}
	
	public static int bytesToInt(byte[] b) {
		return b[3] & 0xFF |
				(b[2] & 0xFF) << 8 |
				(b[1] & 0xFF) << 16 |
				(b[0] & 0xFF) << 24;
	}
	
	public static byte[] intToBytes(int a) {
		return new byte[] {
				(byte) ((a >> 24) & 0xFF),
				(byte) ((a >> 16) & 0xFF),
				(byte) ((a >> 8) & 0xFF),
				(byte) (a & 0xFF)
		};
	}
	
	/**
	 * @return the pfh
	 */
	public packFileHeader getPfh() {
		return pfh;
	}

	/**
	 * @param pfh the pfh to set
	 */
	public void setPfh(packFileHeader pfh) {
		this.pfh = pfh;
	}

	/**
	 * @return the fh
	 */
	public fileHeader getFh() {
		return fh;
	}

	/**
	 * @param fh the fh to set
	 */
	public void setFh(fileHeader fh) {
		this.fh = fh;
	}

	/**
	 * @return the pFile
	 */
	public FileHandle getpFile() {
		return pFile;
	}

	/**
	 * @param pFile the pFile to set
	 */
	public void setpFile(FileHandle pFile) {
		this.pFile = pFile;
	}

	/**
	 * @return the inFile
	 */
	public FileHandle getInFile() {
		return inFile;
	}

	/**
	 * @param inFile the inFile to set
	 */
	public void setInFile(FileHandle inFile) {
		this.inFile = inFile;
	}

	/**
	 * @return the packFileLimit
	 */
	public long getPackFileLimit() {
		return packFileLimit;
	}

	/**
	 * @param packFileLimit the packFileLimit to set
	 */
	public void setPackFileLimit(long packFileLimit) {
		this.packFileLimit = packFileLimit;
	}

	/**
	 * @return the packFileNum
	 */
	public int getPackFileNum() {
		return packFileNum;
	}

	/**
	 * @param packFileNum the packFileNum to set
	 */
	public void setPackFileNum(int packFileNum) {
		this.packFileNum = packFileNum;
	}

	
	public void finalize() {
		if(null != pFile)
			try {
				pFile.CloseFile();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		if(null != inFile)
			try {
				inFile.CloseFile();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
	}

}



/**
 * @author zpzhao
 * @version 2018年7月25日
 */
class fileHeader {
	/**
	 * fileheader
	 * 	 fileHeaderVer
	 *  fileHeaderSize
	 *  fileSize
	 *  fileFlag	= in one packfile 1, in multi packfile 2
	 *  fileNum (fileFlag = 2)
	 *  fileName
	 *  fileCode = bytes UTF-8 little ending 1,bytes UTF-8 big ending
	 *  reserver1
	 */
	private int fileHeaderver = 0x0101;
	private int fileHeaderSize;	
	private long filesize;
	private int fileFlag = 1; 
	private int fileNum;
	private String fileName;
	private int fileCode = 1;
	private int reserve1;
	
	public fileHeader() {
		fileHeaderSize = 0;
		//fileFlag = 1;
		fileName = "";
		//fileCode = 0;
		reserve1 = 0;
	}
	
	public void AdustHeader() {
		try {
			fileHeaderSize = 4 + 4 + 8 + 4 + 4 + fileName.getBytes("utf-8").length + 4 + 4;
			reserve1 = 0;
		} catch (UnsupportedEncodingException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	
	@Override
	public int hashCode() {
		final int prime = 31;
		int result = 1;
		result = prime * result + fileCode;
		result = prime * result + fileFlag;
		result = prime * result + fileHeaderSize;
		result = prime * result + fileHeaderver;
		result = prime * result + ((fileName == null) ? 0 : fileName.hashCode());
		result = prime * result + reserve1;
		return result;
	}
	@Override
	public boolean equals(Object obj) {
		if (this == obj)
			return true;
		if (obj == null)
			return false;
		if (getClass() != obj.getClass())
			return false;
		fileHeader other = (fileHeader) obj;
		if (fileCode != other.fileCode)
			return false;
		if (fileFlag != other.fileFlag)
			return false;
		if (fileHeaderSize != other.fileHeaderSize)
			return false;
		if (fileHeaderver != other.fileHeaderver)
			return false;
		if (fileName == null) {
			if (other.fileName != null)
				return false;
		} else if (!fileName.equals(other.fileName))
			return false;
		if (reserve1 != other.reserve1)
			return false;
		return true;
	}
	
	/* (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	@Override
	public String toString() {
		return "fileHeader [fileHeaderver=" + fileHeaderver + ", fileHeaderSize=" + fileHeaderSize + ", fileFlag="
				+ fileFlag + ", fileName=" + fileName + ", fileCode=" + fileCode + ", reserve1=" + reserve1 + "]";
	}
	
	public int getFileHeaderver() {
		return fileHeaderver;
	}
	public void setFileHeaderver(int fileHeaderver) {
		this.fileHeaderver = fileHeaderver;
	}
	public int getFileHeaderSize() {
		return fileHeaderSize;
	}
	public void setFileHeaderSize(int fileHeaderSize) {
		this.fileHeaderSize = fileHeaderSize;
	}
	public int getFileFlag() {
		return fileFlag;
	}
	public void setFileFlag(int fileFlag) {
		this.fileFlag = fileFlag;
	}
	public String getFileName() {
		return fileName;
	}
	public void setFileName(String fileName) {
		this.fileName = fileName;
	}
	public int getFileCode() {
		return fileCode;
	}
	public void setFileCode(int fileCode) {
		this.fileCode = fileCode;
	}
	public int getReserve1() {
		return reserve1;
	}
	public void setReserve1(int reserve1) {
		this.reserve1 = reserve1;
	}

	/**
	 * @return the fileNum
	 */
	public int getFileNum() {
		return fileNum;
	}

	/**
	 * @param fileNum the fileNum to set
	 */
	public void setFileNum(int fileNum) {
		this.fileNum = fileNum;
	}

	/**
	 * @return the filesize
	 */
	public long getFilesize() {
		return filesize;
	}

	/**
	 * @param filesize the filesize to set
	 */
	public void setFilesize(long filesize) {
		this.filesize = filesize;
	}
	
}
