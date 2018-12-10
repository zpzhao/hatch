package zpzhao.hatch;

import java.util.Random;

/**
 * Hello world!
 *
 */
public class App 
{
    public static void main( String[] args )
    {
        System.out.println( "Hello World!" );
        
        
        new Thread(new Runnable() {

			public void run() {
				// TODO Auto-generated method stub
				Server.start();
			}        	
        }).start();
        
        // wait for server start complete
      try {
		Thread.sleep(1000);
	} catch (InterruptedException e) {
		// TODO Auto-generated catch block
		e.printStackTrace();
	}
      
		final char op[] = {'+','-','*','/'};
		final Random rd = new Random(System.currentTimeMillis());
		
      new Thread(new Runnable() {

		public void run() {
			// TODO Auto-generated method stub
			String expression = null ;
			while(true) {
				expression = rd.nextInt(10) + "" + op[rd.nextInt(4)] + "" + rd.nextInt(10);
				Client.send(expression);
				
				try {
					Thread.sleep(rd.nextInt(100));
				} catch (InterruptedException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
			
		}
    	  
      }).start();
      
    }
}
