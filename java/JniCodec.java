package com.csipsimple.codecs;


public class JniCodec {
	
	// amr 
    public static native int amr_init();
	
	public static native int amr_encode(int handle, int mode, short[] samples, byte[] encoded);
    public static native int amr_decode(int handle, byte[] encoded, int encodedSize, short[] samples);
	
	public static native void amr_close(int handle);
	
	
	// alaw 
    public static native int alaw_init();
	
	public static native int alaw_encode(int handle, short[] samples, byte[] encoded);
    public static native int alaw_decode(int handle, byte[] encoded, int encodedSize, short[] samples);
	
	public static native void alaw_close(int handle);
	
	
	// ulaw 
    public static native int ulaw_init();
	
	public static native int ulaw_encode(int handle, short[] samples, byte[] encoded);
    public static native int ulaw_decode(int handle, byte[] encoded, int encodedSize, short[] samples);
	
	public static native void ulaw_close(int handle);
	
	

    static {
        System.loadLibrary("jni-codec");
    }
}
