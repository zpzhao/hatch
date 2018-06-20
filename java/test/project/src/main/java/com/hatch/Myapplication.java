/**
 *	@author zhaozongpeng
 *	@datetime 2018年6月12日下午1:48:06
 */

package com.hatch;


import java.util.ArrayList;

import com.hatch.common.Area;
import com.hatch.common.Employee;
import com.hatch.member.Manager;

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
		System.out.println("index:"+Myapplication.index+",name:"+ap.name+",el.name:"+ap.el.getName());
		
		
		Employee[] staff = new Employee[3];
		
		Manager boss = new Manager(1,"noffer", new Area(1.2,3.4), 3);
		
		staff[0] = new Employee(1,"001");
		staff[1] = new Employee(2,"002");
		staff[2] = boss;
		
		staff[0].SetSalary(1000);
		staff[1].SetSalary(1500);
		staff[2].SetSalary(4000);
			
		for(Employee e:staff)
			System.out.println("name:"+e.getName()+";salary:"+e.GetSalary());
		
		Manager[] staff1 = new Manager[10];
		Employee[] em_staff = staff1;
		
		for(Employee e:staff)
			if(e instanceof Manager)
			{
				Manager mg = (Manager)e;
				System.out.println(e.getName()+ " is manager");
			}
			else
				System.out.println(e.getName()+ " is'nt manager");
		
		for(Employee e:staff)
		{
			if(e.equals(staff[0]))
			{
				System.out.println("equal("+e.getName()+":"+staff[0].getName()+")");
			}
			else
			{
				System.out.println("no equal("+e.getName()+":"+staff[0].getName()+")");
			}
		}
		
		//em_staff[0] = new Employee(10, "nmgr");
		
		//System.out.println("name: "+ staff1[0].getName());
		
		/*
		 * hashcode
		 */
		String st = "中国";
		String st1 = "DE";
		
		System.out.println(st.hashCode()+":"+st1.hashCode());
		
		int hash = 0;
		for(int i=0; i < st.length(); i++)
			hash = 31 * hash + st.charAt(i);
		System.out.println(st+":hashcode:"+hash);
		
		for(int i=0; i < st1.length(); i++)
			System.out.println(st1.charAt(i));
		
		ap.ArrayTest();
		
		//ap.finalize();
		
		System.out.println("run program end.");
	}

	/**
	 * operator
	 */
	public void MyPrint(String format)
	{
		System.out.println(format);
	}
	
	
	public void ArrayTest()
	{
		long num = 1024;
		ArrayList<Employee> staff = new ArrayList<Employee>((int) num);
		
		for(long i = 0; i < num ; i++)
		{
			staff.add(new Manager());
		}
		
		
	}
	
	public void finalize()
	{
		System.out.println("application finalize");
	}
}








