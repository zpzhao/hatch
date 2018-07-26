import static org.junit.Assert.*;

import java.io.IOException;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

import com.hatch.common.packFile;
import com.hatch.common.packFileHeader;
import com.hatch.log.hatchLog;
import com.hatch.master.Master;

/**
 * @author 
 * @version 2018年7月25日
 */

/**
 * @author zpzhao
 *
 */
public class packFileTest {
	private Master mp ;
	packFile pf;
	
	/**
	 * @throws java.lang.Exception
	 */
	@BeforeClass
	public static void setUpBeforeClass() throws Exception {
		
	}

	/**
	 * @throws java.lang.Exception
	 */
	@AfterClass
	public static void tearDownAfterClass() throws Exception {
	}

	/**
	 * @throws java.lang.Exception
	 */
	@Before
	public void setUp() throws Exception {
		mp = new Master();
		
		/*
		 *  log initial 
		 */
		mp.setAplog(new hatchLog());
		Master.getAplog().InitDefaultLog();
		
		pf = new packFile(Master.getAplog());
		pf.OpenpackFile("pack1.tar");
		
	   packFileHeader pfh = new packFileHeader();
	   pfh.setPackctime(20181616);
	   pfh.setPackutime(20180616);
	   //pfh.setPackVer();
	   pfh.setPackFileSize(36);
	   pfh.setPackFlag(1);
	   pfh.setPackHeaderSize(36);
	   pfh.setPackNum(0);
	   pfh.setPackreserver1(0);
	   pfh.setPackreserver2(0);
	   
		pf.setPfh(pfh);
		try {
			pf.WritePackHeader();
			pf.closePackFile();
		} catch(IOException e)
		{
			e.printStackTrace();
		}
	}

	/**
	 * @throws java.lang.Exception
	 */
	@After
	public void tearDown() throws Exception {
	}

	@Test
	public void packHeaderFileW() {
		packFile pf1 = new packFile();
		pf1.OpenpackFile("pack1.tar");
		pf1.ReadPackHeader();
		assertTrue(pf1.getPfh().equals(pf));
	}

}
