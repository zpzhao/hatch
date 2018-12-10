package zpzhao.hatch;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;

public class SocketHandle implements Runnable {

	public static Socket sock = null;
	public SocketHandle(Socket s) {
		sock = s;
	}
	
	public void run() {
		// TODO Auto-generated method stub
		BufferedReader in = null;
		PrintWriter out = null;
		
		try {
			in = new BufferedReader(new InputStreamReader(sock.getInputStream()));
			out = new PrintWriter(sock.getOutputStream(),true);
			
			String expression = null;
			String result = null;
			
			while(true) {
				if((expression = in.readLine()) == null)
					break;
				
				result = Calculator.cal(expression);
				out.println(result);
			}			
			
		} catch(Exception e) {
			e.printStackTrace();
			
		} finally {
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
			
			if(null != sock)
				try {
					sock.close();
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			sock = null;
		}
	}

}
