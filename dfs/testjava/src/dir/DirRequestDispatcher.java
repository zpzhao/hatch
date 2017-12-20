package dir;

import rpc.TestObject;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

/*
 * describe: manage all threads, and dispatch message
 * date : 2017/12/13
 * author: zpzhao
 */
public class DirRequestDispatcher extends Thread {
	private final String dirdb = "dirdb";
	public volatile boolean quit;
	public Object oblock;
	
	private BlockingQueue<TestObject> queue;
	
	public DirRequestDispatcher() {
		/* default construction */
		quit = false;
		oblock = new Object();
		this.queue = new LinkedBlockingQueue<TestObject>();
	}
	
	/*
	 * (non-Javadoc)
	 * @see java.lang.Thread#run()
	 */
	public void run() {
		try {
			while(!quit) {
				final TestObject rq = this.queue.take();
				
				synchronized(oblock) {
					ProcessRequest(rq);
				}				
			}
		} 
		catch(InterruptedException ex) {
			//quit = true;
			ex.printStackTrace();
			System.out.println("InterruptedException");
		}
		catch(Throwable ex) {
			quit = true;
			ex.printStackTrace();
			/*
			 * Maybe process crash.
			 */
			System.out.println("Throwable");
		}
	}
	
	/*
	 * start all threads here
	 */
	public void startup() {
		this.start();
		
	}
	
	public void shutdown() {
		quit = true;
		this.interrupt();
	}
	
	/*
	 * receive message
	 */
	public void ReceiveRecord(TestObject rq) {
		this.queue.add(rq);
	}
	
	/*
	 * process request message
	 */
	public void ProcessRequest(TestObject rq) {
		System.out.println("processRequest: " + rq.GetObjectName());
	}
}
