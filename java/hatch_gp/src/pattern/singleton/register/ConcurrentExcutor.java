/**
 * ConcurrentExcutor.java
 * senllang
 * 2019年3月31日-下午9:23:23
 */
package pattern.singleton.register;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Semaphore;

/**
 * @author senllang
 * 并发执行handle方法
 * 
 */
public class ConcurrentExcutor {
	/**
	 * 并发执行
	 * @param runHandle	并发执行的方法
	 * @param executeCount 并发执行请求次数
	 * @param concurrentCount	并发线程数量
	 */
	public static void execute(final RunHandler runHandle, int executeCount, int concurrentCount) {
		ExecutorService es = Executors.newCachedThreadPool();
		//信号量用于控制线程并发数
		final Semaphore sem = new Semaphore(concurrentCount);
		//用于闭锁，计数量递减
		final CountDownLatch cdl = new CountDownLatch(executeCount);
		
		for(int i=0; i < executeCount; i++) {
			es.execute(new Runnable() {
				public void run() {
					// 获得线程执行许可，当总计末释放的许可不超过executeCount时，则允许同时并发；
					//否则阻塞等待许可,至到有可用许可
					try {
						sem.acquire();
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
					//System.out.println(Thread.currentThread().getName());
					runHandle.handle();
					sem.release();
					
					cdl.countDown();
				}
			});
			
		}
		
		try {
			// 当闭锁为0时，才会往下走,否则会阻塞
			cdl.await();
			System.out.println("latch count 0");
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		es.shutdown();
	}
	
	public interface RunHandler {
		public void handle();
	}
}
