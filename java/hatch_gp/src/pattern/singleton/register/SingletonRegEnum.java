/**
 * SingletonRegEnum.java
 * senllang
 * 2019-3-31-下午1:24:22
 */
package pattern.singleton.register;

/**
 * @author senllang
 *
 */
public enum SingletonRegEnum {
	INSTANCE;
	private Object data;
	
	public Object getData() {
		return data;
	}
	
	public void setData(Object data) {
		this.data = data;
	}
	
	public static SingletonRegEnum getInstance() {
		return INSTANCE;
	}
}
