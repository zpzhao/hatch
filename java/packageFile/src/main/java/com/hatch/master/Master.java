/**
 * @author zpzhao
 * @version 2018年7月21日
 * @desc package file with distribute processors 
 */
package com.hatch.master;

import java.io.IOException;

import com.hatch.common.FileHandle;
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
		
		FileHandle in = new FileHandle();
		FileHandle out = new FileHandle();
		
		in.OpenReadFile("test.log");
		out.OpenWriteFile("out.log");
		
		int rlen = 0;
		byte[] buf = new byte[1024];
		int off = 0;

		
		try {
			Master.getAplog().InfoLog("off:"+off, "Master", "Main");
			
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
