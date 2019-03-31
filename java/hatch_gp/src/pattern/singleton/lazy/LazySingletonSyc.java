/**
 * 单例模式学习
 * 懒汉模式，为了线程安全，将获取方法加同步关键字
 * 
 * @author senllang
 * 2019.3.30
 */
package pattern.singleton.lazy;

public class LazySingletonSyc {
	private static LazySingletonSyc instance = null;
	private LazySingletonSyc() {}
	
	public synchronized static LazySingletonSyc getInstance() {
		if(null == instance) {
			instance = new LazySingletonSyc();
		}
		return instance;
	}
}
