/**
 * 
 */
package com.hatch.common;

/**
 * @author zhaozongpeng
 *
 */
public class Factory {
	private String fname;
	private Area range;
	
	/**
	 * 
	 */
	public Factory() {
		// TODO Auto-generated constructor stub
		fname = "factory default";
		range = null;
	}

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub

	}

	/**
	 * @return the fname
	 */
	public String getFname() {
		return fname;
	}

	/**
	 * @param fname the fname to set
	 */
	public void setFname(String fname) {
		this.fname = fname;
	}

	/**
	 * @return the range
	 */
	public Area getRange() {
		return range;
	}

	/**
	 * @param range the range to set
	 */
	public void setRange(Area range) {
		this.range = new Area(range.GetX(),range.GetY());
	}

}
