/**
 *	@author zhaozongpeng
 *	@datetime 2018年6月12日下午1:48:06
 */

package com.hatch;


import com.hatch.common.*;
import com.hatch.member.*;

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

	/*
	 * add final limit, that is permit
	 */
	@SuppressWarnings("unused")
	final public static void main(String[] args)
	{
		Myapplication ap = new Myapplication();

		System.out.println("hello world !");
		System.out.println("index:"+Myapplication.index+",name:"+ap.name+",el.name:"+ap.el.GetName());
		
		
		Employee[] staff = new Employee[3];
		
		Manager boss = new Manager(1,"noffer", new Area(1.2,3.4), 3);
		
		staff[0] = new Employee(1,"001");
		staff[1] = new Employee(2, "002");
		staff[2] = boss;
		
		staff[0].SetSalary(1000);
		staff[1].SetSalary(1500);
		staff[2].SetSalary(4000);
			
		for(Employee e:staff)
			System.out.println("name:"+e.GetName()+";salary:"+e.GetSalary());
		
		Manager[] staff1 = new Manager[10];
		Employee[] em_staff = staff1;
		
		//em_staff[0] = new Employee(10, "nmgr");
		
		//System.out.println("name: "+ staff1[0].GetName());
	}

	/**
	 * operator
	 */
	public void MyPrint(String format)
	{
		System.out.println(format);
	}
}







