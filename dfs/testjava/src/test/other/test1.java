package test.other;

import java.util.ArrayList;
import java.util.Iterator;

public class test1 {

	public test1() {
		// TODO Auto-generated constructor stub
	}
	
	public static void main(String[] args) {
		////////////////////////////////////////
		/*
		 * 测试代码，与DFS无关
		 */
		test1 t = new test1();
		t.Compute_max_profit(0,1);
		
		////////////////////////////////////////
	}
	
	/*
	 * 计算洗车金额与洗车次数最优值
	 * 一次性交300, 每次洗车10元，两年期限，洗多少次车最优
	 */
	public void Compute_max_profit(int s, int d) {		
		int perpay=10;
		int limitday=365*2;
		int count=1;
		int actualperpay = 0;
		int totalpay=0;
		
		ArrayList<Integer> al = new ArrayList<Integer>();
		
		/*
		 * 得到所有值列表 
		 * y : 总金额
		 * x : 次数
		 * y = 300 + 10 * x
		 * 这样看来，没有最优。如果以总金额最少来看，次数越少越好。随意吧还是。
		 */
		for (; count < limitday; count++) {
			totalpay = actualperpay = 300 + 10 * count ;
			actualperpay = actualperpay / count;
			
			al.add(actualperpay);
			
			if(d > 0) {
				System.out.println(count +","+ actualperpay + "," + totalpay);
			}
		}
		
		/*
		 * 排优，找到最优的10个
		 */
		
	}

	private ArrayList ArrayList() {
		// TODO Auto-generated method stub
		return null;
	}
}
