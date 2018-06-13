package com.hatch.common;

/**
 * 
 * @author zhaozongpeng
 * @datetime 2018年6月12日下午1:47:56
 */
public class Employee
{

	private int no;
	private String name;
	private double salary;
	
	/**
	 * Initial block
	 */
	{
		no = 2;
		name = "e1";
	}
	
	public Employee()
	{
		salary = 0;
		MyPrint("Default Employee construction: no:"+no+";name:"+name);
	}
	
	public Employee(int ino, String sname)
	{	
		MyPrint("Employee2 construction");
		
		no = ino;
		name =sname;
		salary = 0;
		MyPrint("Employee2 construction: no:"+no+";name:"+name);
		this.MyPrintElements();
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
		MyPrint("no:"+no+";name:"+name+";salary:"+salary);
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
	
	public double GetSalary()
	{
		return salary;
	}
	
	public void SetSalary(double dsalary)
	{
		salary = dsalary;
	}
	
}