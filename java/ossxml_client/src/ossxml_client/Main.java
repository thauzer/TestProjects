package ossxml_client;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;

import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.TransformerException;
import javax.xml.transform.TransformerFactoryConfigurationError;

import org.xml.sax.SAXException;

public class Main {

	public static void main(String[] args) throws IOException, ParserConfigurationException, TransformerFactoryConfigurationError, TransformerException, SAXException {
		
		String inputFileName = args[0];
		String ip = args[1]; 
		
		Socket socket = null;
		socket = new Socket(ip, 28882);
		BufferedReader in = null;
		PrintWriter out = null;
		in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
		out = new PrintWriter(socket.getOutputStream(), true);
		
		BufferedReader inputFile = null;
		inputFile = new BufferedReader(new FileReader(inputFileName));
		
		StringBuffer tempBuffer = new StringBuffer();
		String fileContent = "";
		while ((fileContent = inputFile.readLine()) != null) {
			tempBuffer.append(fileContent);
		}
		
		String requestToServer = tempBuffer.toString();
		
		System.out.println(requestToServer);
		
		final long startTime = System.currentTimeMillis();
		out.println(requestToServer);
		inputFile.close();
		
		StringBuffer responseFromServer = new StringBuffer();
//		int length = in.read();
		String str = null;
		while(true) {
			str = in.readLine();
			if (str == null || str=="" || str.equals("")) {
				break;
			}
			responseFromServer.append(str);
		}
		
		final long endTime = System.currentTimeMillis();
		long elapsedTime = endTime - startTime;
		
        String responseStr = responseFromServer.toString();
		
        System.out.println("\n\nResponse from server:");
		System.out.println(responseStr.replaceAll("(</.*?>)", "$1\n").replaceAll("(<.*?/>)", "$1\n"));
		
		System.out.println("\nElapsed time: " + elapsedTime);
		

	}

}
