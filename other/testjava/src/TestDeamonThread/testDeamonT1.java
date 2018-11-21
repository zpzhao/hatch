package TestDeamonThread;

class MyThread extends Thread {
	public void run() {
		while(true) {
			System.out.println("deamon thread alive.");
			try {
				Thread.sleep(10000);
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
	}
}

public class testDeamonT1 {

	public static void main(String[] args) {
		// TODO Auto-generated method stub
		MyThread th1 = new MyThread();
		// MyThread th2 = new MyThread();
		th1.setDaemon(true);
		th1.start();
		//th2.start();
		System.out.println("Main thread end.");
	}

}
