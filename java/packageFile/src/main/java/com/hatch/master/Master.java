/**
 * @author zpzhao
 * @version 2018年7月21日
 * @desc package file with distribute processors 
 */
package com.hatch.master;

import static org.junit.Assert.assertTrue;

import java.io.BufferedInputStream;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;

import com.hatch.common.FileHandle;
import com.hatch.common.packFile;
import com.hatch.common.packFileHeader;
import com.hatch.log.hatchLog;

/**
 * @author zpzhao
 *
 */
public class Master {
	private static hatchLog aplog = null;

	/**
	 * 
	 */
	public Master() {
		// TODO Auto-generated constructor stub
	}

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub
		Master app = new Master();
		
		/*
		 *  log initial 
		 */
		app.setAplog(new hatchLog());
		Master.getAplog().InitDefaultLog();
				
		Master.getAplog().InfoLog("Application starting ...", "Master", "main");
		
		/*
		 * test file read and write
		 */
		FileHandle in = new FileHandle();
		FileHandle out = new FileHandle();
		
		in.OpenReadFile("test.log");
		out.OpenWriteFile("out.log");
		
		int rlen = 0;
		byte[] buf = new byte[1024];
		int off = 0;

		
		try {	
			while((rlen = in.ReadFile(buf, 0, 1024)) > 0)
			{
				Master.getAplog().InfoLog("off:"+off, "Master", "Main");
				out.WriteFile(buf, 0, rlen);
				off += rlen;
			}
			
		} catch (IOException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
		}
		finally {
			try {
				out.CloseFile();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			try {
				in.CloseFile();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}			
		}
		
		/*
		 * test pack file header write and read
		 */
		packFile pf = new packFile(Master.getAplog());
		pf.OpenpackFile("pack1.tar");
		
	   packFileHeader pfh = new packFileHeader();
	   pfh.setPackctime(20181616);
	   pfh.setPackutime(20180616);
	   //pfh.setPackVer();
	   pfh.setPackFileSize(36);
	   pfh.setPackFlag(1);
	   pfh.setPackHeaderSize(36);
	   pfh.setPackNum(0);
	   pfh.setPackreserver1(0);
	   pfh.setPackreserver2(0);
	   
		pf.setPfh(pfh);
		try {
			pf.WritePackHeader();
			pf.closePackFile();
		} catch(IOException e)
		{
			e.printStackTrace();
		}
		
		packFile pf1 = new packFile();
		pf1.OpenReadFile("pack1.tar");
		
		pf1.ReadPackHeader();
		if(pf1.getPfh().equals(pf.getPfh()))
		{
			Master.getAplog().InfoLog("equal", "Master", "main");			
		}
		else
		{
			Master.getAplog().InfoLog("no equal", "Master", "main");	
		}
		
		/* write pack header context */
		Master.getAplog().InfoLog(pf1.getPfh().toString(),  "Master", "main");
		/* read pack header context */
		Master.getAplog().InfoLog(pf.getPfh().toString(),  "Master", "main");
		
		/* readdir test */
		ArrayList<String> filelist = pf.getpFile().ReadDir(".", null, true);
		for(String s : filelist)
		{
			System.out.println(s);
		}
		
	
		/* end of app */
		Master.getAplog().InfoLog("Application end ...", "Master", "main");
	}

	/**
	 * @return the aplog
	 */
	public static hatchLog getAplog() {
		return aplog;
	}

	/**
	 * @param aplog the aplog to set
	 */
	public void setAplog(hatchLog aplog) {
		Master.aplog = aplog;
	}

}
