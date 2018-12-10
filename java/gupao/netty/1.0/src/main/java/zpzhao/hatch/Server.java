package zpzhao.hatch;

import java.net.ServerSocket;
import java.net.Socket;

public class Server  {

	final static int SERVER_PORT = 7777;
	final String SERVER_IP = "127.0.0.1";
	
	// single instance
	static ServerSocket ser;
	
	public Server() {
		// TODO Auto-generated constructor stub
	}

	public static void start() {
		start(SERVER_PORT);
	}
	
	public static void start(int port) {
		if(null != null)
			return ;
		try {
			ser = new ServerSocket(port);
			System.out.println("Sever listen to "+port);
			
			while(true) {
				Socket sock = ser.accept();
				new Thread(new SocketHandle(sock)).start();
			}
			
		} catch(Exception e) {
			e.printStackTrace();
		} finally {
			
		}
		
	}
	
}
