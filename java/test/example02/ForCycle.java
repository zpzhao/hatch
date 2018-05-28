/*
 * 两种for循环的表示
 * 这里要注意，String类型赋值涉及到深拷贝和浅拷贝
 */
public class ForCycle
{
	public static void main(String[] args)
	{
		String[] name = new String[4];
		int num = 0;
		for(num = 0; num < 4 ; num++)
		{
			name[num] = "" + num;
		}
		for(String i : name)
			System.out.println(i);
	}
}
