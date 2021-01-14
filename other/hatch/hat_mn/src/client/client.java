/**
 * date 2020/12/30
 */
package client;
import org.apache.log4j.*;

import client.net.udp;

/**
 * @author zpzhao
 *
 */
public class client {
	private static org.apache.log4j.Logger log = Logger.getLogger(client.class);
	/**
	 * 
	 */
	public client() {
		// TODO Auto-generated constructor stub
	}

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub
		//PropertyConfigurator.configure("./run/log4j.properties");
		log.info("log test");
		
		Thread.currentThread().setName("main");
		
		RequestDispatch rqdispatch ;
		//try {
			//RequestDispatch.isRunning = true;
		for(int i = 0; i < 8; i++) {
			rqdispatch = new RequestDispatch();
			rqdispatch.startup();
		}
			
			//rqdispatch.wait();
		//} catch (InterruptedException e) {
			// TODO Auto-generated catch block
		//	e.printStackTrace();
		//}
		try {
			Thread.sleep(10000000);
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		log.info("end");
		System.out.println("client end");
	} 
	
}

