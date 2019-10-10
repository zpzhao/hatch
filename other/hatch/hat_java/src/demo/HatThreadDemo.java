/**
 * 
 */
package demo;

/**
 * @author zhaozongpeng
 * @see thread test demo
 */
public class HatThreadDemo {

	/**
	 * 
	 */
	public HatThreadDemo() {
		// TODO Auto-generated constructor stub
	}

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub
		Demo d = new Demo();
        d.start();
        d.waitForStartup();
        for(int i=0;i<1;i++){
            System.out.println(Thread.currentThread().getName()+i);
        }
	}

}


class Demo extends Thread{
    public void run(){
        for(int i=0;i<60;i++){
            System.out.println(Thread.currentThread().getName()+i);
        }
    }

	public void waitForStartup() {
		// TODO Auto-generated method stub
		System.out.println("wait end");
	}
}