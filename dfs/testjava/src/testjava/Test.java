/*
 * test java programme
 * add 2017/8/10
 * senllang
 */
package testjava;

import multithr.TestThread;
import rpc.TestObject;
import dir.DirRequestDispatcher;

public class Test {

	private int versionH = 1;
	private int versionL = 0;
	
	public Test() {
		// TODO Auto-generated constructor stub
	}

	public static void main(String[] args) {
		// TODO Auto-generated method stub
		System.out.println("Test start:");
		/*
		TestThread thr1 = new TestThread("thr1", 1);
		TestThread thr2 = new TestThread("thr2", 2);
		
		thr1.start();
		thr2.start();
	*/
		
		
		DirRequestDispatcher dirRequestDispatcher = new DirRequestDispatcher();
		
		dirRequestDispatcher.startup();
		
		for(int i = 0; i < 1; i++)
		{
			//for(int j = 0; j < i; j++)
			{
				TestObject to = new TestObject(i+"n");
				System.out.println(to.GetObjectName());
				dirRequestDispatcher.ReceiveRecord(to);
			}
				
			//System.out.println(i+"main");
		}
		
		// dirRequestDispatcher.shutdown();
		
		/* 
		 * add shutdown hook here
		 */
		try {
			Runtime.getRuntime().addShutdownHook(new Thread() {
				/* @Override */
				public void run() {
					try {
						/* It's excuted when shutdown */
						System.out.println("Test end!");
					} catch(Exception ex) {
						ex.printStackTrace();
					}					
				}
			});
		}
		catch(Exception ex) {
			System.out.println(ex.getMessage());
		}
	}
	
}
