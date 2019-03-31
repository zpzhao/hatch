/**
 * LazySingletonInnercls.java
 * senllang
 * 2019-3-30-下午11:28:34
 */
package pattern.singleton.lazy;

/**
 * @author senllang
 *	用内部类的方式实现单例懒汉式，避够线程安全的加锁
 *	内部类的加载时机，在使用时才加载内部类并创建对象
 *  内部类是延迟加载的，不管是静态内部类不是非静态内部类，都是在第一次使用时加载，而不是在外部类加载时；
 *  用静态内部类实现，因为在内部类加载时，单实例就会创建，内部类相当于饿汉模式创建实例，所以是线程安全的，这个由JAVA机制保证
 */
public class LazySingletonInnercls {
	static {
		System.out.println("load LazySingletonInnercls");
	}
	
	private LazySingletonInnercls() {
		System.out.println("Create LazySingletonInnercls instance");
	}
	public static LazySingletonInnercls getInstance() {
		return LazyInstance.lazy;
	}
	
	private static final class LazyInstance {
		static {
			System.out.println("load LazyInstance");
		}
		
		private static final LazySingletonInnercls lazy = new LazySingletonInnercls();
	}
}
