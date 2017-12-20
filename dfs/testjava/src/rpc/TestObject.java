package rpc;

public class TestObject extends Object {
	private String name;
	
	public TestObject() {
		name = "default";
	}
	
	public TestObject(String n) {
		name = n;
	}
	
	public String GetObjectName() {
		return name;
	}
	
	public void SetObjectName(String n) {
		name =n;
	}
}
