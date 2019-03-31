/**
 * LazySingletonSycThread.java
 * senllang
 * 2019-3-30-下午10:43:21
 *
 */
package pattern.singleton.lazy;

/**
 * @author senllang
 *
 */
public class LazySingletonSycThread implements Runnable {

	/* (non-Javadoc)
	 * @see java.lang.Runnable#run()
	 */
	@Override
	public void run() {
		// TODO Auto-generated method stub
		LazySingletonSyc lz1 = LazySingletonSyc.getInstance();
		
		System.out.println(Thread.currentThread().getName()+":"+lz1);
	}

}
