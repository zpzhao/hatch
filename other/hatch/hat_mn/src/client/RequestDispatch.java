/**
 * 
 */
package client;

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
		while(isRunning) {
			System.out.println("ThreadID:"+Thread.currentThread().getId()+" ThreadName:"+Thread.currentThread().getName()+" running");
			
			udp sock = new udp(port++);
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
