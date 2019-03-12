/**
 * @author zpzhao
 * @version 2019年3月12日
 */
package hatch.bench;

import java.io.*;
import java.sql.*;
import java.util.*;

/**
 * @author zpzhao
 *
 */
public class Bench {

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// TODO 自动生成的方法存根
		Connection conn = null;
		Statement stmt = null;
		StringBuffer sql = new StringBuffer();

		try {

			Properties ini = new Properties();
			ini.load(new FileInputStream(System.getProperty("prop")));

			// Register jdbcDriver
			Class.forName(ini.getProperty("driver"));

			// make connection
			conn = DriverManager.getConnection(ini.getProperty("conn"), ini.getProperty("user"),
					ini.getProperty("password"));
			conn.setAutoCommit(true);

			// Create Statement
			stmt = conn.createStatement();
						
			
		} catch (IOException ie) {
			System.out.println(ie.getMessage());

		} catch (SQLException se) {
			System.out.println(se.getMessage());

		} catch (Exception e) {
			e.printStackTrace();

			// exit Cleanly
		} finally {
			try {
				if (conn != null)
					conn.close();
			} catch (SQLException se) {
				se.printStackTrace();
			} // end finally

		} // end try
	}
	
	public static int InitTables(Statement stmt) {
		
		return 0;
	}
	
	public static int DropTables(Statement stmt) {
		return 0;
	}
	
	public static int InsertData() {
		return 0;
	}
	
	static void execJDBC(Statement stmt, String query) {

	    System.out.println(query + ";");

	    try {
	      stmt.execute(query);
	    }catch(SQLException se) {
	      System.out.println(se.getMessage());
	    } // end try

	  } // end execJDBCCommand

	public static String getRandomString(int length) {
		String str = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
		Random random = new Random();
		StringBuffer sb = new StringBuffer();
		for (int i = 0; i < length; i++) {
			int number = random.nextInt(62);
			sb.append(str.charAt(number));
		}
		return sb.toString();
	}

	public static float getRandomFloat(float min, float max) {
		float floatBounded = min + new Random().nextFloat() * (max - min);
		return floatBounded;
	}

}
