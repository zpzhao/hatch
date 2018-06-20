/**
 * 
 */
package com.hatch.common;

/**
 * @author zhaozongpeng
 *
 */
public abstract class Person {

	String name;
	
	/**
	 * 
	 */
	public Person() {
		// TODO Auto-generated constructor stub
	}

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub

	}
	
	public abstract String getDescribe();
	
	/**
	 * @return the name
	 */
	public String getName() {
		return name;
	}

	/**
	 * @param name the name to set
	 */
	public void setName(String name) {
		this.name = name;
	}
	
	public boolean equals(Object other)
	{
		if(null == other)
			return false;
		
		if(this == other)
			return true;
		
		if(getClass() != other.getClass())
			return false;
		
		Person f = (Person)other;
		return name.equals(f.name);		
	}

}
