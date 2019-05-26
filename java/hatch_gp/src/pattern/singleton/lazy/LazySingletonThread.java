package pattern.singleton.lazy;

import pattern.singleton.threadlocal.SingletonThreadlocal;

/**
 * 测试单例懒汉式的线程安全性 简单方式是线程不安全的
 * 
 * @author senllang
 * 
 */
public class LazySingletonThread implements Runnable {

	@Override
	public void run() {
		// TODO Auto-generated method stub

		/*
		 * 简单的方式，在每个线程中有可能是不一样的实例
		 */
		// LazySingleton lz1 = LazySingleton.getInstance();
		
		/*
		 * 这里复用一下这个类，注释掉上面的代码，测试一下threadlocal实现的单例
		 */
		SingletonThreadlocal lz1 = SingletonThreadlocal.getInstance();
		
		System.out.println(Thread.currentThread().getName() + ":" + lz1);
	}

}
