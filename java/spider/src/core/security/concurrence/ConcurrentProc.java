/**
 * zpzhao
 * ConcurrentProc.java
 * 2020年10月22日
 */
package core.security.concurrence;

import java.util.concurrent.ArrayBlockingQueue;

/**
 * @author zpzhao
 *
 */
public class ConcurrentProc {
	private ArrayBlockingQueue<Byte> bq_blocklist;
	public static Thread worker;
	/**
	 * 
	 */
	public ConcurrentProc() {
		// TODO Auto-generated constructor stub
	}
	
	
	/*
	 * worker type : 1 -- readfile
	 * 				 2 -- writefile
	 * 				 3 -- encrypt block
	 * 				 4 -- descrypt block
	 * 				 5 -- send block
	 * 	 			 6 -- recv block
	 */
	public static void CreateWorker(int workerType) {
		switch(workerType) {
		case 1 :
			worker = new Thread(()->{
				System.out.println("readfile worker");
			});
			break;
		case 2 :
			worker = new Thread(()->{
				System.out.println("writefile worker");
			});
			break;
		case 3 :
			worker = new Thread(()->{
				System.out.println("encrypt block worker");
			});
			break;
		case 4 :
			worker = new Thread(()->{
				System.out.println("descrypt block worker");
			});
			break;
		case 5 :
			worker = new Thread(()->{
				System.out.println("send block worker");
			});
			break;
		case 6 :
			worker = new Thread(()->{
				System.out.println("recv block worker");
			});
			break;
			default:
				break;
		}
	}
	
	

}
