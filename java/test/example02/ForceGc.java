/**
 * 不断动态申请，引发GC
 */
import java.io.*;
import java.util.AbstractMap;
import java.util.Arrays;
import java.util.Date;
import java.util.Map;
import java.util.Scanner;

/**
 * @author zhaozongpeng
 *
 */
public class ForceGc {

	private static final int FAULTLENGTH = 0;


	/**
	 * 
	 */
	public ForceGc() {
		// TODO Auto-generated constructor stub
	}

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub	
		ForceGc fc = new ForceGc();
	
		// current time infomation
		System.out.printf("%tc\n", new Date());
		
		// current directory
		String dir = System.getProperty("user.dir");
		System.out.println("CurrentDir:"+dir);
		
		fc.Test();
	}
	
	public void Test()
	{
		try {
			FileOp();
			Arrayop();
		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	public void Arrayop()
	{
		int[] num = new int[5000000];
		
		for(int i=0; i < 5000000 ; i++)
		{
			num[i] = (int) (Math.random() * 5000000);
		}
		
		Arrays.sort(num);
		
		for(int i = 0; i< 5000000; i++)
			System.out.println(num.toString());
	}
	
	/**
	 * 文件操作
	 * @throws FileNotFoundException 
	 */
	public void FileOp() throws FileNotFoundException
	{
		PrintWriter ofile = new PrintWriter("in.txt");
		ofile.println("test file ");
		//ofile.close();
		
		Scanner sfile = new Scanner(new File("in.txt"));
		String tx = sfile.nextLine();
		System.out.println(tx);
		//sfile.close();
	}
	
	/**
	 * 使用大量动态内存产生GC
	 */
	public void ForceGcT()
	{
		while(true)
		{
			String[] a = new String[1024];
			for(int i = 0; i < 1024; i++)
				a[i] = "abc" + i;		
			System.out.println("run");
		}
	}
	
	/**
	 * StringBuilder 测试
	 */
	public void StringBuildTest()
	{
		StringBuilder strb = new StringBuilder();
		strb.append("first");
		strb.append('a');
		strb.append(56);
		
		int l = strb.length();
		System.out.println(strb.toString() + " length:"+l);
	}

	/**
	 * 输入输出测试
	 */
	public void InOrOut()
	{
		Scanner in = new Scanner(System.in);
		
		System.out.println("you name:");
		String name = in.nextLine();
		System.out.println("age");
		int age = in.nextInt();
		
		System.out.println(name + ": age:"+age);
		
	}
	
	public void ConsoleT()
	{
		Console cons = System.console();
		if(cons == null)
		{
			System.out.println("console null");
			return;
		}
		
		char[] passwd;
		
		String name1 = cons.readLine("Username:");
		passwd = cons.readPassword("[%s]", "Password:");
		
		java.util.Arrays.fill(passwd, ' ');
		System.out.println(name1 + "pw:" + passwd);
	}
	


	public long[] readCpu(final Process proc) { 
        long[] retn = new long[2]; 
        try { 
            proc.getOutputStream().close(); 
            InputStreamReader ir = new InputStreamReader(proc.getInputStream()); 
            LineNumberReader input = new LineNumberReader(ir); 
            String line = input.readLine(); 
            if (line == null || line.length() < FAULTLENGTH) { 
                return null; 
            } 
            int capidx = line.indexOf("Caption"); 
            int cmdidx = line.indexOf("CommandLine"); 
            int rocidx = line.indexOf("ReadOperationCount"); 
            int umtidx = line.indexOf("UserModeTime"); 
            int kmtidx = line.indexOf("KernelModeTime"); 
            int wocidx = line.indexOf("WriteOperationCount"); 
            long idletime = 0; 
            long kneltime = 0; 
            long usertime = 0; 
            while ((line = input.readLine()) != null) { 
                if (line.length() < wocidx) { 
                    continue; 
                } 
                // 字段出现顺序：Caption,CommandLine,KernelModeTime,ReadOperationCount, 
                // ThreadCount,UserModeTime,WriteOperation 
                String caption =substring(line, capidx, cmdidx - 1).trim(); 
                String cmd = substring(line, cmdidx, kmtidx - 1).trim(); 
                if (cmd.indexOf("wmic.exe") >= 0) { 
                    continue; 
                } 
                String s1 = substring(line, kmtidx, rocidx - 1).trim(); 
                String s2 = substring(line, umtidx, wocidx - 1).trim(); 
                if (caption.equals("System Idle Process") || caption.equals("System")) { 
                    if (s1.length() > 0) 
                        idletime += Long.valueOf(s1).longValue(); 
                    if (s2.length() > 0) 
                        idletime += Long.valueOf(s2).longValue(); 
                    continue; 
                } 
                if (s1.length() > 0) 
                    kneltime += Long.valueOf(s1).longValue(); 
                if (s2.length() > 0) 
                    usertime += Long.valueOf(s2).longValue(); 
            } 
            retn[0] = idletime; 
            retn[1] = kneltime + usertime; 
            return retn; 
        } catch (Exception ex) { 
            ex.printStackTrace(); 
        } finally { 
            try { 
                proc.getInputStream().close(); 
            } catch (Exception e) { 
                e.printStackTrace(); 
            } 
        } 
        return null; 
    }

private static String substring(String line, int capidx, int i) {
	// TODO Auto-generated method stub
	return null;
} 

}



