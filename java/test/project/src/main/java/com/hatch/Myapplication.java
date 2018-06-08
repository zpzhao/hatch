/**
 *
 */

package com.hatch;
public class Myapplication
{
	public static int index;
	public final String name = "abc";
	public Employee el;

	public static void main(String[] args)
	{
		Myapplication ap = new Myapplication();

		System.out.println("hello world !");
		System.out.println("index:"+ap.index+",name:"+ap.name+",el.name:"+ap.el.name);
	}
	{
		el = new Employee();
	}
}

class Employee
{

	public int no;
	public String name;
	
	{
		no = 2;
		name = "e1";
	}	
}

