/**
 * 单例模式学习
 * 简单的懒汉模式，在调用的时候才会创建对对象
 * 
 * @author senllang
 * 2019.3.30
 */
package pattern.singleton.lazy;

/**
 * @author senllang
 * 2019.3.30
 */
public class LazySingleton {
	private static LazySingleton LazyInstance = null;
	
	private LazySingleton() {}
	
	public static LazySingleton getInstance() {
		if(null == LazyInstance) {
			LazyInstance = new LazySingleton();
		}
		
		return LazyInstance;
	}
	
}
