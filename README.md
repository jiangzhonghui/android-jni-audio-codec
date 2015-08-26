# android-jni-audio-codec

Android JNI audio codec from android source, now include amr , pcma, pcmu codec.

Android JNI 层的音频编解码库，现在已经合入的有 AMR, PCMA, PCMU 编码.

### 说明

本文参考了 [opencore-amr-android](https://github.com/kevinho/opencore-amr-android), 表示感谢. 他所使用的是 [opencore-amr](http://sourceforge.net/projects/opencore-amr/files/opencore-amr/) 项目， 我后来发现 Android 系统源码内部包含有修改过的 opencore-amr, 只不过一般的 App 是无法调用的， 我从 Android 系统源码中将该部分提取出来编成 JNI 动态库， 普通 App 可以直接使用， 同时还将 PCMA， PCMU 编码也提取出来了。


### 相关 Android 源码目录
 
 - [Android-4.4-src/frameworks/opt/net/voip/src/jni/rtp]
 - [Android-4.4-src/frameworks/av/media/libstagefright/codecs]


### 编译
直接在根目录执行 ndk-build 即可.

### 备注
实际使用建议修改 JNI 动态注册的方法类名和 java 文件包名.
