/**
 * @author 
 * @version 2018年7月21日
 */
package com.hatch.log;

import java.io.IOException;
import java.util.logging.ConsoleHandler;
import java.util.logging.FileHandler;
import java.util.logging.Handler;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.logging.SimpleFormatter;

/**
 * @author zpzhao
 *
 */
public class hatchLog {

	private String logPath = "";
	private String logFileName = "javaLog";
	private int loglimit = 10000;
	private int lognum = 3;
	private Level logLevel = java.util.logging.Level.INFO;
	
	private Handler logHandler = null;
	private Logger aplog = null;
	
	/**
	 * 
	 */
	public hatchLog() {
		// TODO Auto-generated constructor stub
	}

	/*
	 * log name init
	 */
	public int InitLog(String logName)
	{
		return 0;
	}
	
	public int InitHandle()
	{
		try {
			setLogHandler(new FileHandler(logPath + logFileName +"%g.log",loglimit,lognum,true));
		} catch (SecurityException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		return 0;
	}
	
	public int InitDefaultLog()
	{
		Handler handle = null;
		try {
			handle = new FileHandler(logPath + logFileName +"%g.log",loglimit,lognum,true);
		} catch (SecurityException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();					
		}
		
		handle.setLevel(logLevel);
		handle.setFormatter(new SimpleFormatter());
		
		aplog = Logger.getLogger("com.hatch.packageFile");
		aplog.addHandler(handle);
		
		return 0;
	}

	/**
	 * warning log
	 */
	public void WarnLog(String msg, String sourceClass, String sourceMethod)
	{
		getAplog().logp(java.util.logging.Level.WARNING, sourceClass, sourceMethod, msg);
	}
	
	/**
	 * info log
	 */
	public void InfoLog(String msg, String sourceClass, String sourceMethod)
	{
		getAplog().logp(java.util.logging.Level.INFO, sourceClass, sourceMethod, msg);
	}
	
	/**
	 * severe log
	 */
	public void SevereLog(String msg, String sourceClass, String sourceMethod)
	{
		getAplog().logp(java.util.logging.Level.SEVERE, sourceClass, sourceMethod, msg);
	}
	
	/**
	 * @return the logPath
	 */
	public String getLogPath() {
		return logPath;
	}

	/**
	 * @param logPath the logPath to set
	 */
	public void setLogPath(String logPath) {
		this.logPath = logPath;
	}

	/**
	 * @return the logFileName
	 */
	public String getLogFileName() {
		return logFileName;
	}

	/**
	 * @param logFileName the logFileName to set
	 */
	public void setLogFileName(String logFileName) {
		this.logFileName = logFileName;
	}

	/**
	 * @return the logLevel
	 */
	public Level getLogLevel() {
		return logLevel;
	}

	/**
	 * @param logLevel the logLevel to set
	 */
	public void setLogLevel(Level logLevel) {
		this.logLevel = logLevel;
	}

	/**
	 * @return the logHandler
	 */
	public Handler getLogHandler() {
		return logHandler;
	}

	/**
	 * @param logHandler the logHandler to set
	 */
	public void setLogHandler(Handler logHandler) {
		this.logHandler = logHandler;
	}

	/**
	 * @return the loglimit
	 */
	public int getLoglimit() {
		return loglimit;
	}

	/**
	 * @param loglimit the loglimit to set
	 */
	public void setLoglimit(int loglimit) {
		this.loglimit = loglimit;
	}

	/**
	 * @return the lognum
	 */
	public int getLognum() {
		return lognum;
	}

	/**
	 * @param lognum the lognum to set
	 */
	public void setLognum(int lognum) {
		this.lognum = lognum;
	}

	/**
	 * @return the aplog
	 */
	public Logger getAplog() {
		return aplog;
	}

	/**
	 * @param aplog the aplog to set
	 */
	public void setAplog(Logger aplog) {
		this.aplog = aplog;
	}
}
