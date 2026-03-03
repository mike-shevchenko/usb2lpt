package hardware.inpout32;

import java.io.*; 

public class ioLpt implements Closeable {
  private long hQueue;
// constructors, n == number of LPT port -1, flags == unused
  public ioLpt(int n)		{ hQueue=open(n,0); }
  public void ioLpt(int n, int flags)	{ hQueue=open(n,flags); }
// query state
  public boolean isOpen()		{ return hQueue!=0; }
// access
  public int inOut(byte µcode[], byte result[]) throws IOException {
    if (hQueue==0) throw new NullPointerException();
    int i = inOut(hQueue, µcode, result);
    if (i<0) throw new IOException();
    return i;
  }
  public void out(int offset, int data) throws IOException {
    if (hQueue==0) throw new NullPointerException();
    out(hQueue, offset, data);
  }
  public byte in(int offset) throws IOException {
    if (hQueue==0) throw new NullPointerException();
    return in(hQueue, offset);
  }
  public void delay(int µs) throws IOException {
    if (hQueue==0) throw new NullPointerException();
    delay(hQueue, µs);
  }
  public int flush(byte result[]) throws IOException {
    if (hQueue==0) throw new NullPointerException();
    int i = flush(hQueue, result);
    if (i<0) throw new IOException();
    return i;
  }
  public void close() {
    close(hQueue);
    hQueue = 0;
  }
  public void pass(byte n) {
    return pass(hQueue, n);
  }
  protected void finalize() throws Throwable {close();}

// native routines
  public native static int getAddr(int n);
  public native static int getAddrs(int a[]);
  private native static long open(int n, int flags);
  private native static int inOut(long q, byte µcode[], byte result[]);
  private native static void out(long q, int offset, int data);
  private native static byte in(long q, int offset);
  private native static void delay(long q, int µs);
  private native static int flush(long q, byte result[]);
  private native static boolean close(long q);
  private native static byte pass(long q, byte n);

  static{
    try{
      System.loadLibrary("inpout32");
    }catch (UnsatisfiedLinkError e) {
      System.loadLibrary("inpoutx64");
    }
  }
}
