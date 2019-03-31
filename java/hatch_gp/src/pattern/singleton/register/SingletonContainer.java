/**
 * SingletonContainer.java
 * senllang
 * 2019年3月31日-下午8:59:26
 */
package pattern.singleton.register;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

/**
 * @author senllang
 *	容器注册式单例
 */
public class SingletonContainer {
	private static Map<String,Object> ioc = new ConcurrentHashMap<String,Object>();
	
	private SingletonContainer() {
		
	}
	
	public static Object getBean(String className) {
		synchronized(ioc) {
			if(!ioc.containsKey(className)) {
				Object obj = null;
				try {
					obj = Class.forName(className).newInstance();
					
					ioc.put(className, obj);
					
				} catch (InstantiationException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				} catch (IllegalAccessException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				} catch (ClassNotFoundException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
				
				return obj;
			} else {
				return ioc.get(className);
			}
		}
	}
}
