/**
 * LazysingletonReflect.java
 * senllang
 * 2019-3-31-上午10:53:18
 */
package pattern.singleton.lazy;

/**
 * @author senllang
 * 懒汉式单例，用反射会破坏单例,为了防止这种情况发生，需要在构造函数中增加限制判断
 */
public class LazysingletonReflect {
	
	private LazysingletonReflect() {}
	
	public static LazysingletonReflect getInstance() {
		return LazysingletonReflectInner.lazy;
	}
	
	public static class LazysingletonReflectInner {
		private static final LazysingletonReflect lazy = new LazysingletonReflect();
	}
}
