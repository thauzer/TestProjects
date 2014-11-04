package javassh;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;

import ch.ethz.ssh2.ChannelCondition;
import ch.ethz.ssh2.Connection;
import ch.ethz.ssh2.ServerHostKeyVerifier;
import ch.ethz.ssh2.Session;
import ch.ethz.ssh2.StreamGobbler;

public class MainSsh{

	public static void main(String[] args) {
		// TODO Auto-generated method stub

		try {
//			String hostname = "193.77.24.121";
//			String username = "admin";
//			String password = "zhone";
			
			String hostname = "192.168.0.85";
			String username = "root";
			String password = "";
			
			String command = "ls -ltrh";

			Connection conn = new Connection(hostname);

			conn.connect(null, 300000, 300000);

			boolean isAuthenticated = conn.authenticateWithPassword(username, password);

			if (isAuthenticated == false)
				throw new IOException("Authentication failed.");

			InputStream inpStr;
			OutputStream outStr;
			
			Session sess = conn.openSession();
			sess.requestPTY("dumb", 132, 24, 0, 0, null);
			sess.startShell();
			outStr = sess.getStdin();
		    inpStr = sess.getStdout();
		    
		    int timeout = 5000;
		    
//		    sess.execCommand("slots 1");
		    
		    System.out.println("Reading first command line");
		    StringBuffer result = new StringBuffer();
		    byte[] buffer = new byte[4096];
		    long startTime = System.currentTimeMillis();
		    int status = sess.waitForCondition(ChannelCondition.STDOUT_DATA, timeout);
		    
		    String end = null;
		    String end2 = null;
		    String s, sum = "";
		    boolean timeOutDetected = true;
		    while (status != 1) {
	            int read = inpStr.read(buffer);
	            if(read != -1){
	                s = new String(buffer, 0, read);
	                result.append(s);
	            } else{
	                break;
	            }
	            sum = result.toString();
	            if(end != null && sum.contains(end)){
	                timeOutDetected = false;
	                break;
	            }
	            if(end2 != null && sum.contains(end2)){
	                timeOutDetected = false;
	                break;
	            }
	            if(sum.contains("seconds by IP Address") && sum.contains("OPERATION FAILED...Database is locked")){
	                timeOutDetected = false;
	                break;
	            }
	            status = sess.waitForCondition(ChannelCondition.STDOUT_DATA, timeout);
	        }
		    long endTime = System.currentTimeMillis();
		    long elapsedTime = endTime - startTime;
		    System.out.println(result.toString());
		    System.out.println("\nElapsed time: " + elapsedTime);
		    
		    System.out.println("Writing to ssh server command: " + command);
		    
		    outStr.write(command.getBytes());
		    outStr.write('\n');
		    outStr.flush();
		    
		    System.out.println("Reading response from ssh server command: " + command);
		    startTime = System.currentTimeMillis();
		    status = sess.waitForCondition(ChannelCondition.STDOUT_DATA, timeout);
		     
		    sum = "";
		    timeOutDetected = true;
		    while (status != 1) {
	            int read = inpStr.read(buffer);
	            if(read != -1){
	                s = new String(buffer, 0, read);
	                result.append(s);
	            } else{
	                break;
	            }
	            sum = result.toString();
	            if(end != null && sum.contains(end)){
	                timeOutDetected = false;
	                break;
	            }
	            if(end2 != null && sum.contains(end2)){
	                timeOutDetected = false;
	                break;
	            }
	            if(sum.contains("seconds by IP Address") && sum.contains("OPERATION FAILED...Database is locked")){
	                timeOutDetected = false;
	                break;
	            }
	            status = sess.waitForCondition(ChannelCondition.STDOUT_DATA, timeout);
	        }
		    endTime = System.currentTimeMillis();
		    elapsedTime = endTime - startTime;
		    System.out.println(result.toString());
		    System.out.println("\nElapsed time: " + elapsedTime);
		    System.out.println("");

			System.out.println("Closing");

			sess.close();
			conn.close();

		}
		catch (IOException e)
		{
			e.printStackTrace(System.err);
			System.exit(2);
		}
	}

}
