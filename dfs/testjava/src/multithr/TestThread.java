/**
 * test threads class
 * add 2017/12/7
 */
package multithr;

/**
 * @author zhaozongpeng
 *
 */
public class TestThread extends Thread {

	private String pname = "default";
	private int pno = 0;
	
	/**
	 * 
	 */
	public TestThread() {
		// TODO Auto-generated constructor stub
	}
	
	/**
	 * user define
	 */
	public TestThread(String name, int no) {
		// TODO Auto-generated constructor stub
		pname = name;
		pno = no;
	}

	/**
	 * @param arg0
	 */
	public TestThread(Runnable arg0) {
		super(arg0);
		// TODO Auto-generated constructor stub
	}

	/**
	 * @param arg0
	 */
	public TestThread(String arg0) {
		super(arg0);
		// TODO Auto-generated constructor stub
	}

	/**
	 * @param arg0
	 * @param arg1
	 */
	public TestThread(ThreadGroup arg0, Runnable arg1) {
		super(arg0, arg1);
		// TODO Auto-generated constructor stub
	}

	/**
	 * @param arg0
	 * @param arg1
	 */
	public TestThread(ThreadGroup arg0, String arg1) {
		super(arg0, arg1);
		// TODO Auto-generated constructor stub
	}

	/**
	 * @param arg0
	 * @param arg1
	 */
	public TestThread(Runnable arg0, String arg1) {
		super(arg0, arg1);
		// TODO Auto-generated constructor stub
	}

	/**
	 * @param arg0
	 * @param arg1
	 * @param arg2
	 */
	public TestThread(ThreadGroup arg0, Runnable arg1, String arg2) {
		super(arg0, arg1, arg2);
		// TODO Auto-generated constructor stub
	}

	/**
	 * @param arg0
	 * @param arg1
	 * @param arg2
	 * @param arg3
	 */
	public TestThread(ThreadGroup arg0, Runnable arg1, String arg2, long arg3) {
		super(arg0, arg1, arg2, arg3);
		// TODO Auto-generated constructor stub
	}

	public void run() {
		/*
		 * compute number
		 */
		pno ++;
		System.out.println(pname+":"+pno);
	}
}
