package test.other;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;

public class Myclient {

    public static void main(String[] args) throws Exception {
        
        Socket socket = null;
        BufferedReader in = null;
        PrintWriter out = null;
        
        BufferedReader input = null;
        // 请求指定ip和端口号的服务器
        socket = new Socket("198.13.61.196",3333);
        
        while(true){

            in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
            
            out = new PrintWriter(socket.getOutputStream(), true);
            // 接收控制台的输入
            //input = new BufferedReader(new InputStreamReader(System.in));
            // out.println("this is client info!");
            String info ="abcdeadfdsfdasfsafdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddabc";
            
            out.println(info);
            
            String str = in.readLine();
            
            System.out.println("客户端显示--》服务器的信息：" + str);
        }
        //in.close();
        //out.close();
    }

}