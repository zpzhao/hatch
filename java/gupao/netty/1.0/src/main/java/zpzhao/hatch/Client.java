package zpzhao.hatch;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;
import java.net.UnknownHostException;

public class Client {

	final static String SERVER_IP = "127.0.0.1";
	final static int SERVER_PORT = 7777;
	
	public static void send(String expression) {
		send(SERVER_PORT,expression);
	}
	
	public static void send(int port , String expression)  {
		Socket sock = null;
		BufferedReader in = null;
		PrintWriter out = null;
		
		try {
			sock = new Socket(SERVER_IP, port);
			in = new BufferedReader(new InputStreamReader(sock.getInputStream()));
			out = new PrintWriter(sock.getOutputStream(),true);
			
			System.out.println("client send:"+expression);
			out.println(expression);
			
			System.out.println("client recv:"+in.readLine());
			
		} catch (UnknownHostException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} finally {
			if(null != sock)
				try {
					sock.close();
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			sock = null;
			
			if(null != in)
				try {
					in.close();
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			in = null;
			
			if(null != out)
				out.close();
			out = null;
		}
		
	}
}
