/**
 * zpzhao
 * ByteFileProc.java
 * 2020年10月22日
 */
package core.security.storage;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

/**
 * @author zpzhao
 *
 */
public class ByteFileProc {
	public BufferedOutputStream bos;
	public BufferedInputStream bis;
	
	public FileOutputStream fos;
	public FileInputStream fis;
	
	/**
	 * 
	 */
	public ByteFileProc() {
		// TODO Auto-generated constructor stub
	}

	public void CreateWriteFile(String filePath) throws IOException {
		fos = new FileOutputStream(filePath);
		bos = new BufferedOutputStream(fos);
	}
	
	
	public void CloseWriteFile() throws IOException {
		bos.close();
		fos.close();
	}
	
	
	public void CreateReadFile(String filePath) throws IOException {
		fis = new FileInputStream(filePath);
		bis = new BufferedInputStream(fis);
	}
	
	
	public void CloseReadFile() throws IOException {
		bis.close();
		fis.close();
	}
	
	public int ReadBytes(byte[] value, int readlen, int off) throws IOException {
		return bis.read(value, off, readlen);			
	}
	
	public void WriteBytes(byte[] value) throws IOException {
		bos.write(value);			
	}
}
