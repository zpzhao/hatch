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
import pattern.singleton.serializable.SingletonSerializable;
import pattern.singleton.register.*;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.ObjectOutputStream;
import java.io.ObjectInputStream;
import java.lang.Thread;
import java.lang.reflect.*;
import java.util.List;
import java.util.ArrayList;

/**
 * @author senllang 2019.3.30
 */
public class SingletonTest {

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub

		// LazyTest();
		// LazyThreadTest();
		// LazySycThreadTest();
		// LazyInnerTest();
		// LazyReflectTest();
		// LazyReflect1Test();
		// LazySerializable();
		// singletonRegEnumTest();
		// SingletonEnumReflect();
		SingletonContainerTest();
	}

	/**
	 * @author senllang 懒汉模式测试
	 */
	public static void LazyTest() {
		LazySingleton lzsimple = LazySingleton.getInstance();
		LazySingleton lzsimple1 = LazySingleton.getInstance();

		System.out.print("instance1 :" + lzsimple + "\ninstance2:" + lzsimple1);
	}

	/**
	 * @author senllang
	 * @category 在多线程下测试 懒汉模式在是线程不安全的，如果只是简单单例，每个线程可能得到的实例是不同的
	 */
	public static void LazyThreadTest() {
		Thread th1 = new Thread(new LazySingletonThread());
		Thread th2 = new Thread(new LazySingletonThread());

		th1.start();
		th2.start();
	}

	/**
	 * @author senllang
	 * @category 在多线程下测试 懒汉模式,获取实例方法增加synchronized
	 *           ，保证线程安全，但是在调用并发高频繁时效率不高，cpu占用多
	 */
	public static void LazySycThreadTest() {
		List<Thread> thlist = new ArrayList<Thread>();

		for (int i = 0; i < 20; i++) {
			thlist.add(new Thread(new LazySingletonSycThread()));
		}

		for (int i = 0; i < 20; i++) {
			Thread th = thlist.get(i);
			th.start();
		}
	}

	/**
	 * 静态内部类实现懒汉单例 并观察内部类加载时机，和单例创建时机
	 */
	public static void LazyInnerTest() {
		// 可以将无参构造改为public，可以更好观察加载的时机，可以看到外部类实例创建了，内部类还没有加载
		// LazySingletonInnercls ly1 = new LazySingletonInnercls();
		LazySingletonInnercls ly1 = null;
		System.out.println("ly1 : " + ly1);

		LazySingletonInnercls ly2 = LazySingletonInnercls.getInstance();

		ly1 = LazySingletonInnercls.getInstance();

		System.out.println("ly1 : " + ly1 + "\nly2" + ly2);
	}

	/**
	 * 静态内部类实现懒汉单例 通过反射破坏单例，可以创建多个实例
	 */
	public static void LazyReflectTest() {
		try {
			Class<?> clazz = LazySingletonInnercls.class;
			// 通过反射拿到私有的构造方法
			Constructor<?> c = clazz.getDeclaredConstructor(null);
			// 强制访问，设置访问权限
			c.setAccessible(true);
			// 强制构造，初始化实例
			Object o1 = c.newInstance();

			// 调用两次构造，初始化两个实例
			// 犯了原则性问题，破坏了单例
			Object o2 = c.newInstance();

			System.out.println("o1 : " + o1 + "\no2 : " + o2);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	/**
	 * 静态内部类实现懒汉单例 通过反射破坏单例，改造构造函数后就可以防止破坏
	 */
	public static void LazyReflect1Test() {
		try {
			Class<?> clazz = LazysingletonReflect.class;
			// 通过反射拿到私有的构造方法
			Constructor<?> c = clazz.getDeclaredConstructor(null);
			// 强制访问，设置访问权限
			c.setAccessible(true);
			// 强制构造，初始化实例
			Object o1 = c.newInstance();

			// 调用两次构造，初始化两个实例
			// 犯了原则性问题，破坏了单例
			Object o2 = c.newInstance();

			System.out.println("o1 : " + o1 + "\no2 : " + o2);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	/**
	 * 测试序列化对单例的破坏 不管是懒汉还是饿汉都会存在序列化破化的情况，这里只是测试序列化破坏的情况
	 */
	public static void LazySerializable() {
		SingletonSerializable s1 = null;
		SingletonSerializable s2 = SingletonSerializable.getInstance();
		System.out.println("getinstance");
		
		FileOutputStream fout = null;
		FileInputStream fin = null;
		
		try {
			fout = new FileOutputStream("serializableInstance.obj");
			ObjectOutputStream oos = new ObjectOutputStream(fout);
			
			oos.writeObject(s2);
			oos.flush();
			oos.close();
			
			System.out.println("write Object end");
			
			fin = new FileInputStream("serializableInstance.obj");
			ObjectInputStream ois = new ObjectInputStream(fin);
			s1 = (SingletonSerializable)ois.readObject();
			ois.close();
			System.out.println("read Object end");
			System.out.println(s1);
			System.out.println(s2);
			System.out.println(s1 == s2);
			
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	
	/**
	 * 枚举注册式单例 ，可以避免序列化时创建新的实例
	 */
	public static void singletonRegEnumTest() {
		SingletonRegEnum s1 = null;
		SingletonRegEnum s2 = SingletonRegEnum.getInstance();
		// 枚举为什么只能在这里赋值，在后面赋值都不会生效
		s2.setData(new Object());
		System.out.println("getinstance");
		
		FileOutputStream fout = null;
		FileInputStream fin = null;
		
		try {
			fout = new FileOutputStream("serializableInstance.obj");
			ObjectOutputStream oos = new ObjectOutputStream(fout);
			
			oos.writeObject(s2);
			oos.flush();
			oos.close();
			
			System.out.println("write Object end");
			
			fin = new FileInputStream("serializableInstance.obj");
			ObjectInputStream ois = new ObjectInputStream(fin);
			s1 = (SingletonRegEnum)ois.readObject();
			ois.close();
			System.out.println("read Object end");
			System.out.println(s1.getData());
			System.out.println(s2.getData());
			
			// s2.setData(new Object());
			System.out.println(s1.getData() == s2.getData());
			
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	/**
	 * 枚举式单例的反射测试
	 * 不能被反射，在JAVA代码中对枚举做了特殊处理，保证枚举只被加载一次，只有一个实例；
	 */
	public static void SingletonEnumReflect() {
		Class<?> clazz = SingletonRegEnum.class;
		Constructor c;
		try {
			c = clazz.getDeclaredConstructor(String.class, int.class);
			c.setAccessible(true);
			SingletonRegEnum s = (SingletonRegEnum)c.newInstance("temp",200);
		} catch (InstantiationException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IllegalAccessException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IllegalArgumentException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (InvocationTargetException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (NoSuchMethodException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
		} catch (SecurityException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
		}
	}
	
	/**
	 * 测试容器注册式单例
	 * 是线程不安全的，基本上是每个线程内保持单例
	 */
	public static void SingletonContainerTest() {
		long startTime = System.currentTimeMillis();
		
		ConcurrentExcutor.execute(new ConcurrentExcutor.RunHandler() {
			
			@Override
			public void handle() {
				// TODO Auto-generated method stub
				Object obj = SingletonContainer.getBean("pattern.singleton.register.Pojo");
				System.out.println(System.currentTimeMillis()+":"+Thread.currentThread().getName()+":"+obj);
			}
		}, 1000, 2);
		
		long endTime = System.currentTimeMillis();
		
		System.out.println("总耗时:"+(endTime-startTime)+" ms.");
	}
}
