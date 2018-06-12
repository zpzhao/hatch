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
		MyPrint("Default Myapplication construction");
	}

	public static void main(String[] args)
	{
		Myapplication ap = new Myapplication();

		System.out.println("hello world !");
		System.out.println("index:"+Myapplication.index+",name:"+ap.name+",el.name:"+ap.el.GetName());
	}

	/**
	 * operator
	 */
	public void MyPrint(String format)
	{
		System.out.println(format);
	}
}

/**
 * 
 * @author zhaozongpeng
 * @datetime 2018年6月12日下午1:47:56
 */
class Employee
{

	private int no;
	private String name;
	
	/**
	 * Initial block
	 */
	{
		no = 2;
		name = "e1";
	}
	
	public Employee()
	{
		MyPrint("Default Employee construction: no:"+no+";name:"+name);
	}
	
	public Employee(int ino, String sname)
	{		
		MyPrint("Employee2 construction: no:"+no+";name:"+name);
		no = ino;
		name =sname;
		MyPrint("Employee2 construction: no:"+no+";name:"+name);
	}
	
	/**
	 * operators
	 */
	public void MyPrint(String format)
	{
		System.out.println(format);
	}
	
	public void MyPrintElements()
	{
		MyPrint("no:"+no+";name:"+name);
	}
	
	/**
	 * get/set operators
	 */
	public int GetNo()
	{
		return no;
	}
	
	public void SetNo(int ino)
	{
		no = ino;
	}
	
	public String GetName()
	{
		return name;
	}
	
	public void SetName(String sname)
	{
		name = sname;
	}
}

