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
	}
}
