/**
 * 单例模式测试
 * 1.饿汉式
 * 2.懒汉式
 * 		简单实现
 * 		多线程安全：加锁，双检查方式加锁
 * 		序列化安全
 * 		反射安全
 * 
 * 3.注册式   
 * 		容器式注册
 * 		枚举式注册
 * 4.单例问题
 * 		多线程破坏单例
 * 		反射破坏单例
 * 		序列化破坏单例
 * 
 * 
 * @author senllang
 */
package pattern.singleton;
import pattern.singleton.lazy.*;
import java.lang.Thread;
import java.util.List;
import java.util.ArrayList;

/**
 * @author senllang
 * 2019.3.30
 */
public class SingletonTest {

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub
		
		// LazyTest();
		//LazyThreadTest();
		//LazySycThreadTest();
		LazyInnerTest();
		
	}
	
	/**
	 * @author senllang
	 * 懒汉模式测试
	 */
	public static void LazyTest() {
		LazySingleton lzsimple = LazySingleton.getInstance();	
		LazySingleton lzsimple1 = LazySingleton.getInstance();
		
		System.out.print("instance1 :"+lzsimple+"\ninstance2:"+lzsimple1);
	}
	
	/**
	 * @author senllang
	 * @category 在多线程下测试
	 * 懒汉模式在是线程不安全的，如果只是简单单例，每个线程可能得到的实例是不同的
	 */
	public static void LazyThreadTest() {
		Thread th1 = new Thread(new LazySingletonThread());
		Thread th2 = new Thread(new LazySingletonThread());
		
		th1.start();
		th2.start();
	}
	
	/**
	 * @author senllang
	 * @category 在多线程下测试
	 * 懒汉模式,获取实例方法增加synchronized ，保证线程安全，但是在调用并发高频繁时效率不高，cpu占用多
	 */
	public static void LazySycThreadTest() {
		List<Thread> thlist = new ArrayList<Thread>();
		
		for(int i=0; i < 20; i++) {
			thlist.add(new Thread(new LazySingletonSycThread()));
		}
		
		
		for(int i=0; i < 20; i++) {
			Thread th = thlist.get(i);
			th.start();
		}
	}
	
	/**
	 * 静态内部类实现懒汉单例
	 * 并观察内部类加载时机，和单例创建时机
	 */
	public static void LazyInnerTest() {
		// 可以将无参构造改为public，可以更好观察加载的时机，可以看到外部类实例创建了，内部类还没有加载
		//LazySingletonInnercls ly1 = new LazySingletonInnercls();
		LazySingletonInnercls ly1 = null;
		System.out.println("ly1 : "+ly1);
		
		LazySingletonInnercls ly2 = LazySingletonInnercls.getInstance();
		
		ly1 = LazySingletonInnercls.getInstance();
		
		System.out.println("ly1 : "+ly1+"\nly2"+ly2);
	}
	
	
	/**
	 * 静态内部类实现懒汉单例
	 * 通过反射破坏单例，可以创建多个实例
	 */
	public static void LazyReflectTest() {
		
		LazySingletonInnercls ly2 = LazySingletonInnercls.getInstance();
		
		ly1 = LazySingletonInnercls.getInstance();
		
		System.out.println("ly1 : "+ly1+"\nly2"+ly2);
	}
}
