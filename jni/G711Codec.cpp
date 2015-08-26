/*
 * Copyrightm (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <jni.h>
#include <android/log.h>
#include <string.h>
#include <stdint.h>

#ifndef NELEM
# define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

namespace {

const int8_t gExponents[128] = {
    0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
};

//------------------------------------------------------------------------------

class UlawCodec
{
public:
    int set(int sampleRate, const char *fmtp) {
        mSampleCount = sampleRate / 50;
        return mSampleCount;
    }
    int encode(void *payload, int16_t *samples);
    int decode(int16_t *samples, int count, void *payload, int length);
private:
    int mSampleCount;
};

int UlawCodec::encode(void *payload, int16_t *samples)
{
    int8_t *ulaws = (int8_t *)payload;
    for (int i = 0; i < mSampleCount; ++i) {
        int sample = samples[i];
        int sign = (sample >> 8) & 0x80;
        if (sample < 0) {
            sample = -sample;
        }
        sample += 132;
        if (sample > 32767) {
            sample = 32767;
        }
        int exponent = gExponents[sample >> 8];
        int mantissa = (sample >> (exponent + 3)) & 0x0F;
        ulaws[i] = ~(sign | (exponent << 4) | mantissa);
    }
    return mSampleCount;
}

int UlawCodec::decode(int16_t *samples, int count, void *payload, int length)
{
    int8_t *ulaws = (int8_t *)payload;
    if (length > count) {
        length = count;
    }
    for (int i = 0; i < length; ++i) {
        int ulaw = ~ulaws[i];
        int exponent = (ulaw >> 4) & 0x07;
        int mantissa = ulaw & 0x0F;
        int sample = (((mantissa << 3) + 132) << exponent) - 132;
        samples[i] = (ulaw < 0 ? -sample : sample);
    }
    return length;
}

//------------------------------------------------------------------------------

class AlawCodec
{
public:
    int set(int sampleRate, const char *fmtp) {
        mSampleCount = sampleRate / 50;
        return mSampleCount;
    }
    int encode(void *payload, int16_t *samples);
    int decode(int16_t *samples, int count, void *payload, int length);
private:
    int mSampleCount;
};

int AlawCodec::encode(void *payload, int16_t *samples)
{
    int8_t *alaws = (int8_t *)payload;
    for (int i = 0; i < mSampleCount; ++i) {
        int sample = samples[i];
        int sign = (sample >> 8) & 0x80;
        if (sample < 0) {
            sample = -sample;
        }
        if (sample > 32767) {
            sample = 32767;
        }
        int exponent = gExponents[sample >> 8];
        int mantissa = (sample >> (exponent == 0 ? 4 : exponent + 3)) & 0x0F;
        alaws[i] = (sign | (exponent << 4) | mantissa) ^ 0xD5;
    }
    return mSampleCount;
}

int AlawCodec::decode(int16_t *samples, int count, void *payload, int length)
{
    int8_t *alaws = (int8_t *)payload;
    if (length > count) {
        length = count;
    }
    for (int i = 0; i < length; ++i) {
        int alaw = alaws[i] ^ 0x55;
        int exponent = (alaw >> 4) & 0x07;
        int mantissa = alaw & 0x0F;
        int sample = (exponent == 0 ? (mantissa << 4) + 8 :
            ((mantissa << 3) + 132) << exponent);
        samples[i] = (alaw < 0 ? sample : -sample);
    }
    return length;
}

} // namespace


extern "C" {
	
jint alaw_init
  (JNIEnv *env, jclass)
{
    AlawCodec* codec = new AlawCodec();
    if (codec == NULL) {
    }
    
    codec->set(8000, "");

    return (jint)codec;
}

/*
 * Class:     
 * Method:    encode
 * Signature: (I[S[B)I
 */
jint alaw_encode
  (JNIEnv *env, jclass, jint handle, jshortArray samples, jbyteArray encoded)
{
    AlawCodec* codec =  (AlawCodec*)handle;

    jsize inLen = env->GetArrayLength(samples);
    jshort inBuf[inLen]; 
    env->GetShortArrayRegion(samples, 0, inLen, inBuf);

    jsize outLen = env->GetArrayLength(encoded);
    jbyte outBuf[outLen];
    int encodeLength;

    encodeLength = codec->encode((void*)outBuf, (int16_t*)inBuf);

    env->SetByteArrayRegion(encoded, 0, encodeLength, outBuf);
    
    return encodeLength;
}

/*
 * Class:     
 * Method:    decode
 * Signature: ([BI[SI)I
 */
jint alaw_decode
  (JNIEnv *env, jclass, jint handle, jbyteArray encoded, jint encodedSize, jshortArray samples)
{
    AlawCodec* codec =  (AlawCodec*)handle;

    jbyte inBuf[encodedSize];
    env->GetByteArrayRegion(encoded, 0, encodedSize, inBuf);

    short outBuf[160];
    codec->decode((int16_t*)outBuf, 160, (void*)inBuf, encodedSize);

    env->SetShortArrayRegion(samples, 0, 160, outBuf);

    return 160;
}


/*
 * Class:     
 * Method:    close
 * Signature: (I)V
 */
void alaw_close
  (JNIEnv *, jclass, jint handle)
{
    delete (AlawCodec*)handle;
}


jint ulaw_init
  (JNIEnv *env, jclass)
{
    UlawCodec* codec = new UlawCodec();
    if (codec == NULL) {
    }
    
    codec->set(8000, "");

    return (jint)codec;
}

/*
 * Class:     
 * Method:    encode
 * Signature: (I[S[B)I
 */
jint ulaw_encode
  (JNIEnv *env, jclass, jint handle, jshortArray samples, jbyteArray encoded)
{
    UlawCodec* codec =  (UlawCodec*)handle;

    jsize inLen = env->GetArrayLength(samples);
    jshort inBuf[inLen]; 
    env->GetShortArrayRegion(samples, 0, inLen, inBuf);

    jsize outLen = env->GetArrayLength(encoded);
    jbyte outBuf[outLen];
    int encodeLength;

    encodeLength = codec->encode((void*)outBuf, (int16_t*)inBuf);

    env->SetByteArrayRegion(encoded, 0, encodeLength, outBuf);
    
    return encodeLength;
}

/*
 * Class:     
 * Method:    decode
 * Signature: ([BI[SI)I
 */
jint ulaw_decode
  (JNIEnv *env, jclass, jint handle, jbyteArray encoded, jint encodedSize, jshortArray samples)
{
    UlawCodec* codec =  (UlawCodec*)handle;

    jbyte inBuf[encodedSize];
    env->GetByteArrayRegion(encoded, 0, encodedSize, inBuf);

    short outBuf[160];
    codec->decode((int16_t*)outBuf, 160, (void*)inBuf, encodedSize);

    env->SetShortArrayRegion(samples, 0, 160, outBuf);

    return 160;
}


/*
 * Class:     
 * Method:    close
 * Signature: (I)V
 */
void ulaw_close
  (JNIEnv *, jclass, jint handle)
{
    delete (UlawCodec*)handle;
}
	
	
	
}

static JNINativeMethod gMethods[] = {
	{ "alaw_init", "()I", (void *)alaw_init },
	{ "alaw_encode", "(I[S[B)I", (void *)alaw_encode },
	{ "alaw_decode", "(I[BI[S)I", (void *)alaw_decode },
	{ "alaw_close", "(I)V", (void *)alaw_close },
	
	{ "ulaw_init", "()I", (void *)ulaw_init },
	{ "ulaw_encode", "(I[S[B)I", (void *)ulaw_encode },
	{ "ulaw_decode", "(I[BI[S)I", (void *)ulaw_decode },
	{ "ulaw_close", "(I)V", (void *)ulaw_close },
};


int registerG711Codec(JNIEnv *env)
{
	jclass clazz;
    clazz = env->FindClass("com/csipsimple/codecs/JniCodec");
	
    env->RegisterNatives(clazz, gMethods, NELEM(gMethods));

    return 0;
}

