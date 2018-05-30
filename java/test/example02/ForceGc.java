

/**
 * 不断动态申请，引发GC
 */

/**
 * @author zhaozongpeng
 *
 */
public class ForceGc {

	/**
	 * 
	 */
	public ForceGc() {
		// TODO Auto-generated constructor stub
	}

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub
		
		while(true)
		{
			String[] a = new String[1024];
			for(int i = 0; i < 1024; i++)
				a[i] = "abc";			
		}
	}

}
