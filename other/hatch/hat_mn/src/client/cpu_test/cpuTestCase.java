/**
 * 
 */
package client.cpu_test;

import java.math.BigDecimal;
import java.util.Calendar;
import java.util.Date;

/**
 * @author zpzhao
 *
 */
public class cpuTestCase {

	/**
	 * 
	 */
	public cpuTestCase() {
		// TODO Auto-generated constructor stub
	}
	
	public void TestCaseUtil() {
		long maxCount = 500000000;

		int warp_count = 0;
		long count = 0;
		long b_second, l_second;
		long rawtime;

		while (true) {

			b_second = System.currentTimeMillis() / 1000; // 排序前取得当前时间
			l_second = b_second + 1;

			/*
			 * number is counted per second,which testcase is call. 
			 */
			while ((b_second = (System.currentTimeMillis() / 1000)) < l_second) {

				// TODO: to call cpu test case here!
				SimpleTest();
				DoubleTestCase();
				GetPaiN(500000);
				count++;
			}

			rawtime = System.currentTimeMillis();

			Calendar c = Calendar.getInstance();
			c.setTimeInMillis(rawtime);

			System.out.println("耗时: " + c.get(Calendar.MINUTE) + "分 " + c.get(Calendar.SECOND) + "秒 "
					+ c.get(Calendar.MILLISECOND) + " 微秒");

			System.out.println("count: " + count);

			count = 0;
			warp_count++;
			if (warp_count == maxCount)
				break;
		}
	}

	public void SimpleTest() {
		int i, j, l, k, m, jj;
		j = 32156;
		jj = 2342;
		k = 31455;
		l = 16452;
		m = 9823;
		i = 1000000;

		m = m ^ l;
		k = (k / m * jj) % i;
		l = j * m * k;
		i = (j * k) ^ m;
		k = (k / m * jj) % i;
		m = m ^ l;
		m = m ^ l;
		i = (j * k) ^ m;
		k = (k / m * jj) % i;
		m = i * i * i * i * i * i * i; // m=k*l*jj*l;
		m = m ^ l;
		k = (k / m * jj) % i;
		l = j * m * k;
		i = (j * k) ^ m;
		l = (k / m * jj) % i;
		m = m ^ l;
		m = m ^ l;
		i = (j * k) ^ m;
		k = (k / m * jj) % i;
		m = k * k * k * k * k - m / i;
	}
	
	
	public void DoubleTestCase( ) {
		long STEPS = 500000;
		int i;
		double p = 2;
		for (i = 0; i < STEPS; i++) {
			p *= ((double) (((int) ((i + 2) / 2)) * 2)) / (((int) ((i + 1) / 2)) * 2 + 1);
		}	
	}

	

	/**
	 * 
	 *
	 *@大致思路：利用马青公式与Java的BigDecimal对结果计算，
	 *理论上可以精确到π的十万位以后，
	 *
	 *程序优化思路：
	 *1，可以使用文本文件存储输出结果
	 *
	 *2，马青公式分为两部分，可以使用多线程同时运算，提高时间效率。
	 */
	public void GetPai() {
			// 马青公式：π=16(arctan1/5−4arctan1/239 )
			//初始公式变量相关
			BigDecimal fz = BigDecimal.ONE;
			BigDecimal fz1 = new BigDecimal("5");
			BigDecimal fz2 = new BigDecimal("239");
			
			BigDecimal fm1 = new BigDecimal("25");
			BigDecimal fm2 = new BigDecimal("57121");
			BigDecimal a = new BigDecimal("4");
			BigDecimal b = new BigDecimal("1");
			BigDecimal c = new BigDecimal("2");
			//公式符号改变标志
			int flag = 1;
			//结果存储
			//分结果
			BigDecimal result1;
			BigDecimal r1;
			BigDecimal r2;
			//总结果
			BigDecimal result = new BigDecimal("0");
			//精确迭代次数10000次
			int n=10000;
			//start
			int i = 1;
			long time1=new Date().getTime();
			while (i < n) {
				i++;
				//分式子结果，计算1
				r1=fz.divide(fz1.multiply(b),n,BigDecimal.ROUND_DOWN);
				r2=fz.divide(fz2.multiply(b),n,BigDecimal.ROUND_DOWN);
				//分式子结果，计算1
				if (flag == 1) {
					result1=a.multiply(r1).subtract(r2);
					//标志改变
					flag = -1;
				} else {
					result1=r2.subtract(a.multiply(r1));
					flag = 1;
				}
				
				//总计算
				result = result.add(result1);
				//变量迭达变换
				b=b.add(c);
				fz1 = fz1.multiply(fm1);
				fz2 = fz2.multiply(fm2);

			}
			//end
			long time2=new Date().getTime();
			//输出pi1000与计算时间
			System.out.println("pi计算的结果：\n" + result.multiply(a).toString().substring(0, 1000));
			System.out.println("pi计算的时间：\n" +(time2-time1)+"毫秒" );
	}
	
	
	public void GetPaiN(long digitNum) {
		// 马青公式：π=16(arctan1/5−4arctan1/239 )
		//初始公式变量相关
		BigDecimal fz = BigDecimal.ONE;
		BigDecimal fz1 = new BigDecimal("5");
		BigDecimal fz2 = new BigDecimal("239");
		
		BigDecimal fm1 = new BigDecimal("25");
		BigDecimal fm2 = new BigDecimal("57121");
		BigDecimal a = new BigDecimal("4");
		BigDecimal b = new BigDecimal("1");
		BigDecimal c = new BigDecimal("2");
		//公式符号改变标志
		int flag = 1;
		//结果存储
		//分结果
		BigDecimal result1;
		BigDecimal r1;
		BigDecimal r2;
		//总结果
		BigDecimal result = new BigDecimal("0");
		//精确迭代次数10000次
		int n= (int)digitNum;
		//start
		int i = 1;
		long time1=new Date().getTime();
		while (i < n) {
			i++;
			//分式子结果，计算1
			r1=fz.divide(fz1.multiply(b),n,BigDecimal.ROUND_DOWN);
			r2=fz.divide(fz2.multiply(b),n,BigDecimal.ROUND_DOWN);
			//分式子结果，计算1
			if (flag == 1) {
				result1=a.multiply(r1).subtract(r2);
				//标志改变
				flag = -1;
			} else {
				result1=r2.subtract(a.multiply(r1));
				flag = 1;
			}
			
			//总计算
			result = result.add(result1);
			//变量迭达变换
			b=b.add(c);
			fz1 = fz1.multiply(fm1);
			fz2 = fz2.multiply(fm2);

		}
		//end
		long time2=new Date().getTime();
		//输出pi1000与计算时间
		//System.out.println("pi计算的结果：\n" + result.multiply(a).toString().substring(0, 1000));
		System.out.println("pi计算的时间：\n" +(time2-time1)+"毫秒" );
}


}
