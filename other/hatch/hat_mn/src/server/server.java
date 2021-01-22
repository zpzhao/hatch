/**
 * udp communication programmer
 * date 2020/12/30
 */
package server;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.SocketException;
import java.nio.channels.ServerSocketChannel;

/**
 * @author zpzhao
 *
 */
public class server {
	
	public final static int recv_buf_max = 8192;
	
	/**
	 * 
	 */
	public server() {
		// TODO Auto-generated constructor stub
	}

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub
		try {
			// 1.构建DatagramSocket实例，指定本地端口
			DatagramSocket socket = new DatagramSocket(12300);
			            
			byte[] buf = new byte[recv_buf_max];
			// 2.构建需要收发的DatagramPacket报文--构造 DatagramPacket，用来接收长度为 length 的数据包。
			DatagramPacket packet = new DatagramPacket(buf, buf.length);
			while (true) {
				// 收报文---从此套接字接收数据报包，此方法在接收到数据报前一直阻塞
				// TODO: packet再次使用时是否需要清空
				socket.receive(packet);
				System.out.println("ip:" + packet.getAddress().getHostAddress()
						+"; port:"+packet.getPort()+ "; len:"+packet.getLength());
				// 接收信息
				@SuppressWarnings("deprecation")
				String receive = new String(packet.getData(),
						packet.getLength());
				System.out.println("接收信息" + receive);
				// 发报文
				String info = receive + "client 您好！";
				// 构造数据报包，用来将长度为 length 的包发送到指定主机上的指定端口号。
				DatagramPacket dp = new DatagramPacket(info.getBytes(),
						info.getBytes().length, packet.getAddress(), packet.getPort());
				// 为此包设置数据缓冲区。将此 DatagramPacket 的偏移量设置为 0，长度设置为 buf 的长度
				dp.setData(info.getBytes());
				// 从此套接字发送数据报包
				socket.send(dp);
			}
		} catch (SocketException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

}

