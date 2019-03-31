/**
 * LazysingletonReflect.java
 * senllang
 * 2019-3-31-上午10:53:18
 */
package pattern.singleton.lazy;

/**
 * @author senllang 懒汉式单例，用反射会破坏单例,为了防止这种情况发生，需要在构造函数中增加限制判断
 */
public class LazysingletonReflect {
	static {
		System.out.println("load LazysingletonReflect");
	}

	private LazysingletonReflect() {
		System.out.println("constructor LazysingletonReflect");

		// 内部类在这里调用，也时在这里会加载，加载时就会创建实例，所以内部类加载后实例就不为空，也就不会发生再次创建实例的情况
		if (null != LazysingletonReflectInner.lazy)
			throw new RuntimeException("不允许创建多个实例");
	}

	public static LazysingletonReflect getInstance() {
		return LazysingletonReflectInner.lazy;
	}

	public static final class LazysingletonReflectInner {
		static {
			System.out.println("load LazysingletonReflectInner");
		}

		private static final LazysingletonReflect lazy = new LazysingletonReflect();
	}
}
