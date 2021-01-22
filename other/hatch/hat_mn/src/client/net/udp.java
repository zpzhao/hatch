/**
 * 
 */
package client.net;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.UnknownHostException;

/**
 * @author zpzhao
 *
 */
public class udp {
	public final static String serverIP = "192.71.0.2";
	private int port = 8871;
	/**
	 * 
	 */
	public udp() {
		// TODO Auto-generated constructor stub
	}
	
	public udp(int port) {
		// TODO Auto-generated constructor stub
		this.port = port;
	}
	public void test() {
			/*try {
			// 客服端建立连接，ip地址是本地，端口号是10000
			Socket s = new Socket("localhost", 10000);
			// 建立输出流，目的是为了向服务器端写数据
			PrintStream out = new PrintStream(s.getOutputStream());
			// 建立输出流，目的是为了向服务器端读取数据
			BufferedReader in = new BufferedReader(new InputStreamReader(
					s.getInputStream()));
			// 从键盘写东西
			Scanner scan = new Scanner(System.in);
			while (true) {
				System.out.println("请输入：");
				String str = scan.nextLine();
				// 写数据
				out.println(str);
				// 读数据
				String line = in.readLine();
				System.out.println(line);
				scan.close();
				s.close();
			}
		} catch (UnknownHostException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}*/
		try {		 
				// 创建接收端的socket对象
				DatagramSocket ds = new DatagramSocket(port);
		 
				// 创建一个包裹
				byte[] bys = "hello udp server!".getBytes();
				DatagramPacket dp = new DatagramPacket(bys, bys.length,
						InetAddress.getByName(serverIP),12300);
				
				// send packet data
				ds.send(dp);
				
				DatagramPacket p = new DatagramPacket(new byte[8192],8192);
				ds.receive(p);
				// 释放资源
				ds.close();
		}catch(UnknownHostException e) {
			e.printStackTrace();
		} catch(IOException e) {
			e.printStackTrace();
		}
	}

}
