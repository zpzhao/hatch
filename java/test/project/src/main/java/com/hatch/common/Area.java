package com.hatch.common;


/**
 * final class
 * @author zhaozongpeng
 *
 */
public final class Area 
{
	private double x;
	private double y;
	
	public Area()
	{
		x = 0;
		y = 0;
	}
	
	public Area(double dx, double dy)
	{
		x = dx;
		y = dy;
	}
	
	public double GetX()
	{
		return x;
	}
	
	public double GetY()
	{
		return y;
	}
	
	public void SetX(double dx)
	{
		x = dx;
	}
	
	public void SetY(double dy)
	{
		y = dy;
	}
}