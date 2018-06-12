/**
 *	@author zhaozongpeng
 *	@datetime 2018年6月12日下午1:48:06
 */

package com.hatch;
public class Myapplication
{
	public static int index;
	public final String name = "abc";
	public Employee el;
	
	/**
	 * Initial block
	 */
	{
		el = new Employee();
	}
	
	/**
	 * 
	 */
	public Myapplication()
	{
		
	}

	public static void main(String[] args)
	{
		Myapplication ap = new Myapplication();

		System.out.println("hello world !");
		System.out.println("index:"+ap.index+",name:"+ap.name+",el.name:"+ap.el.name);
	}

}

/**
 * 
 * @author zhaozongpeng
 * @datetime 2018年6月12日下午1:47:56
 */
class Employee
{

	public int no;
	public String name;
	
	/**
	 * Initial block
	 */
	{
		no = 2;
		name = "e1";
	}
	
}

