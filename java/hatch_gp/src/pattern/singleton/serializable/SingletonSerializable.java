/**
 * SingletonSerializable.java
 * senllang
 * 2019-3-31-上午11:54:30
 */
package pattern.singleton.serializable;

import java.io.Serializable;

/**
 * @author senllang
 * 
 */
public class SingletonSerializable implements Serializable {

	/**
	 * 
	 */
	private static final long serialVersionUID = -2905415464986261206L;

	private SingletonSerializable() {
		System.out.println("Construction");
	}

	private static final SingletonSerializable lazy = new SingletonSerializable();

	public static SingletonSerializable getInstance() {
		return lazy;
	}
	
	/**
	 * 增加这个方法，可以解决序列化破坏单例的问题
	 * 但是并没有从根本上解决实例被多次创建，只是在读对象时返回了原有的实例对象，把读的时候新创建的实例对象没有用而已
	 * 实际上实例化了两次，只不过新创建的对象没有被返回而已
	 * 这在高频读时存在问题,真正解决需要注册式单例
	 * @return
	 */
	private Object readResolve() {
		return lazy;
	}
}
