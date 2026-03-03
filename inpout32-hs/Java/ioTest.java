//package jnpout32;
import hardware.inpout32.*;
//import java.lang.Integer;

public class ioTest
{
          static short datum;
          static short Addr;
	  static pPort lpt;
 static ioLpt mylpt;
 
    static void do_read()
    {
          // Read from the port
          datum = (short) lpt.input(Addr);

          // Notify the console
          System.out.println("Read Port: " + Integer.toHexString(Addr) +
                              " = " +  Integer.toHexString(datum));
     }

     static void do_write()
     {
          // Notify the console
          System.out.println("Write to Port: " + Integer.toHexString(Addr) +
                              " with data = " +  Integer.toHexString(datum));
          //Write to the port
          lpt.output(Addr,datum);
     }


     static void do_read_range()
     {
          // Try to read 0x378..0x37F, LPT1:
          for (Addr=0x378; (Addr<0x380); Addr++) {

               //Read from the port
               datum = (short) lpt.input(Addr);

               // Notify the console
               System.out.println("Port: " + Integer.toHexString(Addr) +
                                   " = " +  Integer.toHexString(datum));
          }
     }


     public static void main( String args[] )
     {

	    lpt = new pPort();
        try{
	  mylpt=new ioLpt(0);
	  System.out.println("ioLpt::ioLpt(0) liefert " + mylpt.isOpen());
	  mylpt.queueDelay(1000);
	  byte[] µcode=new byte[10];
	  byte[] result=new byte[10];
	  for(int i=0;i<10;i++)µcode[i]=0x10;
	  mylpt.inOut(µcode,result);
	  mylpt.close();
	  mylpt=null;
	  for(int i=0;i<10;i++) System.out.print(µcode[i]+" "+result[i]+"  ");
	}catch (Exception e) {
	  System.out.println("Murks");
	}
	  
 
          // Try to read 0x378..0x37F, LPT1:

           do_read_range();


     //  Write the data register

     Addr=(short)ioLpt.getAddr(0);
     System.out.println("Adresse von LPT1 ist "+Integer.toHexString(Addr));
     datum=0x77;
     do_write();


     // And read back to verify
     do_read();

     //  One more time, different value
     datum=0xAA;
     do_write();

     // And read back to verify
     do_read();

     // etc...

     Addr++;
     do_read();

     Addr--;
     do_read();

     do_read_range();

    }


}
