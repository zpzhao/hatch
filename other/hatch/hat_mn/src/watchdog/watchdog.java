/**
 * date 2020/12/30
 */
package watchdog;

/**
 * @author zpzhao
 *
 */
public class watchdog {

	public static long heartbeatTimeout = 10000000; // unit is millis second; default 10s
	public static int heartbeatRetryCnt = 3;  // default try again three times.
	
	/**
	 * 
	 */
	public watchdog() {
		// TODO Auto-generated constructor stub
	}

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub
		long start = System.currentTimeMillis();
		int retryCnt = 0;
		long now = start;
		
		while(true) {
			if((now - start) > heartbeatTimeout) {
				retryCnt ++;
				
				DoRetry();
			}
			
			if(retryCnt > heartbeatRetryCnt) {
				System.out.println("try "+heartbeatRetryCnt+" times later, along failure, then exit");
				break;
			}
			
			if(IsHealth() > 0) {
				retryCnt = 0;
			}
			
			try {
				Thread.sleep(1000000);
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}			
		}
	}

	public static void DoRetry() {
		
	}
	
	public static int IsHealth() {
		return 0;
	}
}
