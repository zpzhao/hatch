/**
 * @author zpzhao
 * @date 2020/10/14
 */
package spider;

import core.security.sm4.*;

/**
 * @author zpzhao
 *
 */
class Spider {

	/**
	 * 
	 */
	public Spider() {
		// TODO Auto-generated constructor stub
	}

	public static void ShowBytes(byte[] values) {
		for(int i = 0; i < values.length; i++) {
			System.out.printf("%#x  ",values[i]);
		}
		System.out.println(" ");
	}
	
	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub
		System.out.println("hello");
		
		byte[] key = "1234567890123456".getBytes();
		byte[] value = "hello1234567890123456".getBytes(); // new byte[16];
		byte[] crypt_value;
		byte[] decrypt_value = new byte[8192];
		
		Sm4 hsm4 = new Sm4();
		
		System.out.println("origin: ");
		ShowBytes(value);
		
		crypt_value = Sm4.encodeSMS4(value,  key);
		System.out.println("encrypt: ");
		ShowBytes(crypt_value);
				
		decrypt_value = Sm4.decodeSMS4(crypt_value, key);
		System.out.println("decrypt: ");
		ShowBytes(decrypt_value);
	}

}
