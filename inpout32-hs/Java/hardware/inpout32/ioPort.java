package hardware.inpout32;

public class ioPort {
  // Output a value to a specified port address.
  // Only the lower 8 bits of data is used.
  // Upper 8 bits are ignored.
  // Use a negative value for PortAddress if greater than 0x7FFF!
  public native void Out32(short PortAddress, short data);

  // Input a value from a specified port address
  // Only the lower 8 bits of return value are used.
  // Upper 8 bits are set to zero.
  public native short Inp32(short PortAddress);

  // load 'jnpout32reg.dll' for package
  static{
    try{
      System.loadLibrary("inpout32");
    }catch (UnsatisfiedLinkError e) {
      System.loadLibrary("jnpout32reg");
    }
  }
}
