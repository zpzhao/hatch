/**
 * 
 */
package client;

import client.cpu_test.cpuTestCase;
import client.net.udp;

/**
 * @author zpzhao
 *
 */
public class RequestDispatch extends Thread{

	public static boolean isRunning = false;
	public static int port = 8890;
	/**
	 * 
	 */
	public RequestDispatch() {
		// TODO Auto-generated constructor stub
		Thread.currentThread().setName("hatch_mn");
	}

	@Override
	public void run() {
		// TODO Auto-generated method stub
		udp sock = new udp(port++);
		while(isRunning) {
			System.out.println("ThreadID:"+Thread.currentThread().getId()+" ThreadName:"+Thread.currentThread().getName()+" running");
			
			new cpuTestCase().TestCaseUtil();
			sock.test();
		}
	}
	
	public void startup() {
		isRunning = true;
		this.start();
	}

	public void stopping() {
		isRunning = false;
	}
}
