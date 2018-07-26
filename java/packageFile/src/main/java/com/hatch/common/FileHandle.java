/**
 * @author 
 * @version 2018年7月21日
 */
package com.hatch.common;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;

import com.hatch.master.Master;

/**
 * @author zpzhao
 *
 */
public class FileHandle {
	private File hfile = null;
	
	private BufferedOutputStream dataOut = null;
	
	private BufferedInputStream dataIn = null;
	
	/**
	 * 
	 */
	public FileHandle() {
		// TODO Auto-generated constructor stub
	}
	
	/**
	 * @return the hfile
	 */
	public File getHfile() {
		return hfile;
	}

	/**
	 * @param hfile the hfile to set
	 */
	public void setHfile(File hfile) {
		this.hfile = hfile;
	}

	/**
	 * @return the dataOut
	 */
	public BufferedOutputStream getDataOut() {
		return dataOut;
	}

	/**
	 * @param dataOut the dataOut to set
	 */
	public void setDataOut(BufferedOutputStream dataOut) {
		this.dataOut = dataOut;
	}

	/**
	 * @return the dataIn
	 */
	public BufferedInputStream getDataIn() {
		return dataIn;
	}

	/**
	 * @param dataIn the dataIn to set
	 */
	public void setDataIn(BufferedInputStream dataIn) {
		this.dataIn = dataIn;
	}

	/*
	 * 
	 */
	public int OpenFile(String filePath)
	{
		hfile = new File(filePath);
		
		return 0;
	}
	
	public int OpenReadFile(String filePath)
	{
		Master.getAplog().InfoLog("Open " + filePath, "FileHandle", "OpenReadFile");
		hfile = new File(filePath);
		
		InputStream filein = null;
		try {
			filein = new FileInputStream(hfile);
		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
		dataIn = new BufferedInputStream(filein);
		
		
		return 0;
	}
	
	/**
	 * get file size
	 * @return
	 */
	public long GetFileSize() {
		return hfile.length();
	}
	
	public int OpenWriteFile(String filePath)
	{
		Master.getAplog().InfoLog("Open " + filePath, "FileHandle", "OpenWriteFile");
		hfile = new File(filePath);
		
		OutputStream fileOut = null;
		try {
			fileOut = new FileOutputStream(hfile);
		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		dataOut = new BufferedOutputStream(fileOut);
		
		return 0;
	}
	
	/**
	 * read file
	 * return : readlen, error -1 
	 */
	public int ReadFile(byte[] context, int off, int rlen) throws IOException
	{
		return dataIn.read(context, off, rlen);		
	}
	
	/**
	 * write file
	 * @throws IOException 
	 */
	public int WriteFile(byte[] context, int off, int len) throws IOException
	{
		dataOut.write(context, off, len);
		
		return 0;
	}
	
	public void CloseFile() throws IOException
	{
		if(null != dataOut)
			dataOut.close();
		
		if(null != dataIn)
			dataIn.close();
		
		dataOut = null;
		dataIn = null;
	}
	
	/**
	 * read directory, return file list
	 */
	public ArrayList<String> ReadDir(String path, String base, boolean isTop) {
		ArrayList<String> files = new ArrayList<String>();
		String basePath = "";
		
		File file = new File(path);
		if(!file.exists())
		{
			Master.getAplog().SevereLog(path+" not exits.", "FileHandle", "ReadDir");
			return null;
		}
		
		if(isTop)
			basePath = file.getName();
		else
			basePath = base + "/" + file.getName();
		
		File[] tempList = file.listFiles();
		if(null == tempList)
		{
			Master.getAplog().WarnLog(path+" is empty.", "FileHandle", "ReadDir");
			return null;
		}
		
		for(File f: tempList)
		{
			/* "/" need to windows or linux */
			files.add(basePath+"/"+f.getName());
			
			if(f.isDirectory())
			{
				files.addAll(ReadDir(f.getAbsolutePath(), basePath, false));
			}
		}
		
		return files;
	}
	
	public void finalize() {
		if(null != dataOut)
			try {
				dataOut.close();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		
		if(null != dataIn)
			try {
				dataIn.close();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
	}
}
