package com.jc.searchengine;
import java.util.Random;
import com.jc.searchengine.po.Person;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.SQLException;
import java.sql.Statement;

import org.apache.ignite.Ignite;
import org.apache.ignite.IgniteCache;
import org.apache.ignite.IgniteException;
import org.apache.ignite.Ignition;
import org.apache.ignite.IgniteJdbcThinDriver;

/**
 * @Author: wangjie
 * @Description: put data and get data
 * @Date: Created in 13:38 2018/3/22
 */
public class HelloWorld
{
    public static void main(String[] args) {
        /*try (Ignite ignite = Ignition.start("/home/ssd/apache-ignite-2.7.0-bin/examples/config/example-cache.xml")) {
            // Put values in cache.
            IgniteCache<Integer, Person> cache = ignite.getOrCreateCache("myCache");

            cache.put(1, new Person(1,"Hello",1));
            cache.put(2, new Person(2,"World!",2));

            // Get values from cache and broadcast 'Hello World' on all the nodes in the cluster.
            ignite.compute().broadcast(() -> {
                Person s1 = cache.get(1);
                Person s2 = cache.get(2);

                System.out.println(s1.toString() + " " + s2.toString());*/
    	
    	
    	// Register JDBC driver.
    	try {
			Class.forName("org.apache.ignite.IgniteJdbcThinDriver");
		} catch (ClassNotFoundException e1) {
			// TODO 自动生成的 catch 块
			System.out.println("class failure.");
			e1.printStackTrace();
		}

    	// Open JDBC connection.
    	Connection conn = null;
		try {
			conn = DriverManager.getConnection("jdbc:ignite:thin://127.0.0.1/");
		} catch (SQLException e1) {
			// TODO 自动生成的 catch 块
			System.out.println("connection failure.");
			e1.printStackTrace();
		}

	String tbl_sql = "create table  tbl_employ(id LONG primary key, name varchar, etime timestamp,info varchar, salary real);";
	String insert_sql = "insert into tbl_employ(id,name,etime,info,salary) values(";
		
    	// Create database tables.
    	try (Statement stmt = conn.createStatement()) {

    	    // Create table based on REPLICATED template.
    	    /*stmt.executeUpdate("CREATE TABLE town (" + 
    	    " id LONG PRIMARY KEY, name VARCHAR) " +
    	    " WITH \"template=replicated\"");*/

    	    // Create table based on PARTITIONED template with one backup.
/*    	    stmt.executeUpdate("CREATE TABLE people (" +
    	    " id LONG, name VARCHAR, town_id LONG, " +
    	    " PRIMARY KEY (id, town_id)) " +
    	    " WITH \"backups=1, affinityKey=town_id\"");
    	  
    	    // Create an index on the City table.
    	    stmt.executeUpdate("CREATE INDEX idx_town_name ON town (name)");

    	    // Create an index on the Person table.
    	    stmt.executeUpdate("CREATE INDEX idx_people_name ON people (name)");*/
    		    		   
    		// test sql
    		// insert into tbl_employ(id,name,etime,info,salary) 
    		// values(1,'test',current_timestamp(),'hello teest',8.4);
    		String tval = insert_sql;
    		for(int i = 20; i < 100000; i++)
    		{
    			tval = insert_sql;
    			tval += i + ","; // id
    			tval += "'" + getRandomString(16) +"',"; // name
    			tval += "current_timestamp(),"; // etime
    			tval += "'" + getRandomString(48) +"',"; // info
    			tval += getRandomFloat(10.0f, 100.0f); // salary
    			tval += ");"; // end
    			System.out.println(tval);
    			stmt.executeUpdate(tval);    			
    		}
    		
    		// stmt.executeUpdate("select count(*) from tbl_employ;");
    	} catch (SQLException e) {
			// TODO 自动生成的 catch 块
    		System.out.println("statement failure.");
			e.printStackTrace();
		}
    }
 public static String getRandomString(int length){
     String str="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
     Random random=new Random();
     StringBuffer sb=new StringBuffer();
     for(int i=0;i<length;i++){
       int number=random.nextInt(62);
       sb.append(str.charAt(number));
     }
     return sb.toString();
  }
 
 public static float getRandomFloat(float min, float max) {
		float floatBounded = min + new Random().nextFloat() * (max - min);
		return floatBounded;
 }
}
