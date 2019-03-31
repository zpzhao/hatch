/**
 * LazySingletonSycDoubleCheck.java
 * senllang
 * 2019-3-30-下午11:06:31
 */
package pattern.singleton.lazy;

/**
 * @author senllang
 *
 */
public class LazySingletonSycDoubleCheck {
	private static LazySingletonSycDoubleCheck lazy = null;
	
	private LazySingletonSycDoubleCheck() {}
	
	public static LazySingletonSycDoubleCheck getInstance() {
		if(null == lazy) {
			/*
			 * 1.线程安全；
			 * 2.提高高频调用性能，使用synchronized 关键字，并进行两次检查
			 */
			synchronized(LazySingletonSycDoubleCheck.class) {
				if(null == lazy) {
					lazy = new LazySingletonSycDoubleCheck();
				}
			}
		}
		return lazy;
	}
}
