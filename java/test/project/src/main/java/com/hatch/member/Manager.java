package com.hatch.member;

import com.hatch.common.Area;
import com.hatch.common.Employee;

/**
 * 
 * @author zhaozongpeng
 * @datetime 2018年6月12日下午2:55:22
 */
public class Manager extends Employee
{
	private Area range;
	private int level;
	
	public Manager()
	{
		setRange(new Area(0,0));
		level = 1;
		MyPrint("default Manager construction");
		MyPrintElements();
	}
	
	public Manager(int ino, String sname, Area ar, int ilevel)
	{
		super(ino,sname);
		setRange(ar);
		level = ilevel;
		MyPrint("Manager2 construction");
		MyPrintElements();
	}
	
	public double GetSalary()
	{
		double baseline = super.GetSalary();
		return level * baseline;
	}

	public Area getRange() {
		return clone(range);
	}
	
	public Area getRangeNC() {
		return range;
	}

	private Area clone(Area rg) {
		// TODO Auto-generated method stub
		if(null == rg)
			return null;
		Area ra = new Area(rg.GetX(),rg.GetY());
		return ra;
	}

	public void setRange(Area range) {
		if(null == range)
			MyPrint("setRange: rang null");
		this.range = range;
	}
	
	/**
	 * operators
	 */
	public void MyPrintElements()
	{
		// super.MyPrintElements();
		if(null == range)
		{
			MyPrint("name:"+this.GetName()+"; range null to MyPrintElements.");
			return;
		}
		MyPrint("name:"+this.GetName()+";level:"+level+";range:"+range.GetX()+":"+range.GetY());
	}
}
