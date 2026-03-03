// Java support, with self-registration
// Originally "jnpout" made by Douglas Beattie Jr.
// <beattidp@ieee.org> http://www.hytherion.com/beattidp/
#include <jni.h>
#include "inpout32.h"

// These functions are not DLL-exported. This is done by JNI_OnLoad().
// This approach leaves some freedom to the package choosen.

static void JNICALL jOut32(JNIEnv*, jclass, jshort addr, jshort data) {
 Out32(addr,(BYTE)data);
}

static jshort JNICALL jInp32(JNIEnv*, jclass, jshort addr) {
 return Inp32(addr);
}


static jint JNICALL getAddr(JNIEnv*, jclass, jint n) {
 return (jint)LptGetAddr(NULL,n);
}

static jint JNICALL getAddrs(JNIEnv* env, jclass, jintArray a) {
 jint*u=env->GetIntArrayElements(a,0);
 int ret=(jint)LptGetAddr((DWORD*)u,env->GetArrayLength(a));
 env->ReleaseIntArrayElements(a,u,0);
 return ret;
}

// All the wrappers use jlong for HQUEUE to ease transition to inpoutx64.dll,
// the Java code can remain the same but should System.loadLibrary("inpoutx64") instead.
// The example class does this by exception catching.
// A better solution would be checking the bitness of the used Java VM.
static jlong JNICALL open(JNIEnv*, jclass, jint n, jint flags) {
 return (jlong)LptOpen(n,flags);
}

static int JNICALL inOut(JNIEnv*env, jclass, jlong q, jbyteArray ucode, jbyteArray result) {
 jbyte*u=env->GetByteArrayElements(ucode,0);
 jbyte*r=env->GetByteArrayElements(result,0);
 int ret=LptInOut((HQUEUE)q,(const BYTE*)u,env->GetArrayLength(ucode),(BYTE*)r,env->GetArrayLength(result));
 env->ReleaseByteArrayElements(result,r,0);
 env->ReleaseByteArrayElements(ucode,u,JNI_ABORT);
 return ret;
}

static void JNICALL out(JNIEnv*, jclass, jlong q, jint o, jint b) {
 LptOut((HQUEUE)q,(BYTE)o,(BYTE)b);
}

static jbyte JNICALL in(JNIEnv*, jclass, jlong q, jint o) {
 return LptIn((HQUEUE)q,(BYTE)o);
}

static void JNICALL delay(JNIEnv*, jclass, jlong q, jint us) {
 LptDelay((HQUEUE)q,us);
}

static int JNICALL flush(JNIEnv*env, jclass, jlong q, jbyteArray result) {
 jbyte*r=env->GetByteArrayElements(result,0);
 int ret=LptFlush((HQUEUE)q,(BYTE*)r,env->GetArrayLength(result));
 env->ReleaseByteArrayElements(result,r,0);
 return ret;
}

static jboolean JNICALL close(JNIEnv*, jclass, jlong q) {
 return LptClose((HQUEUE)q);
}

static jbyte JNICALL pass(JNIEnv*, jclass, jlong q, jbyte b) {
 return LptPass((HQUEUE)q, b);
}


static jclass myFindClass(JNIEnv *env, char*fullPackage) {
 jclass ret;
 for(;;){
  ret=env->FindClass(fullPackage);
  if (ret) return ret;
  fullPackage=strchr(fullPackage,'/');
  if (!fullPackage) return 0;
  fullPackage++;
 }
}


jint JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved) {
 jclass cls;
 JNIEnv *env;

 if (jvm->GetEnv((void **)&env, JNI_VERSION_1_2)) return JNI_ERR; // JNI Version not supported!

 // Grammar notation for JNINativeMethod .signature field is according to
 // "Java Virtual Machine Specification Second Edition," Section 4.3
 static const JNINativeMethod nm[12]= {	//Array to hold JNINativeMethod structs
  {"Out32",	"(SS)V",	jOut32},	//Java method name, method descriptor, pointer to implementation.
  {"Inp32",	"(S)S",		jInp32},
  {"getAddr",	"(I)I",		getAddr},
  {"getAddrs",	"([I)I",	getAddrs},
  {"open",	"(II)J",	open},
  {"inOut",	"(J[B[B)I",	inOut},
  {"out",	"(JII)V",	out},
  {"in",	"(JI)B",	in},
  {"delay",	"(JI)V",	delay},
  {"queueFlush","(J[B)I",	flush},
  {"close",	"(J)Z",		close},
  {"pass",	"(JB)B",	pass}};

// Used packages were "hardware.jnpout32", "jnpout32", and no package.
// So try to find the right package for current Java bytecode,
// and let the Java developer leave some choice ...
// Maybe I should replace "inpout32" with the current DLL file name?
 cls=myFindClass(env,"hardware/inpout32/ioPort");
 if (!cls) cls=myFindClass(env,"hardware/jnpout32/ioPort");
 if (cls) {
  if (env->RegisterNatives(cls,nm,2)) return JNI_ERR; // JNI Version not supported!
 }				//debug.print("RegisterNatives failed...");
// New code should use "hardware.inpout32" package because it's much more obvious.
 cls=myFindClass(env,"hardware/inpout32/ioLpt");
 if (cls) {
  if (env->RegisterNatives(cls,nm+2,10)) return JNI_ERR; // JNI Version not supported!
 }

 return JNI_VERSION_1_2;
}
