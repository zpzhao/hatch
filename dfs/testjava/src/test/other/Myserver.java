package test.other;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.ServerSocket;
import java.net.Socket;

public class Myserver {

    public static void main(String[] args) throws Exception{
        ServerSocket ss = null;
        Socket socket = null;
        BufferedReader in = null;
        PrintWriter out = null;
        BufferedReader input = null;
        
        // 监听3333端口
        ss = new ServerSocket(3333);
        // 等待接收客户端的请求
        socket = ss.accept();
        while(true){
            // 获取连接对象的输入流
            in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
            // 获取客户端的输入信息
            String str = in.readLine();
            
            //input = new BufferedReader(new InputStreamReader(System.in));
            
            System.out.println("服务器显示-->客户端输入数据：" + str);
            
            out = new PrintWriter(socket.getOutputStream(),true);
            // 将数据输出到客户端
            // out.println("hehe");
            String info = "defaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaadef";
            
            out.println(info);
        }
       // in.close();
       // out.flush();
       // out.close();
       // ss.close();
    }

}