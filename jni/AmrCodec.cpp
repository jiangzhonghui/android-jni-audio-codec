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

#include "gsmamr_dec.h"
#include "gsmamr_enc.h"

#ifndef NELEM
# define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

namespace {

const int gFrameBits[8] = {95, 103, 118, 134, 148, 159, 204, 244};

//------------------------------------------------------------------------------

// See RFC 4867 for the encoding details.

class AmrCodec{
public:
    AmrCodec() {
        if (AMREncodeInit(&mEncoder, &mSidSync, false)) {
            mEncoder = NULL;
        }
        if (GSMInitDecode(&mDecoder, (Word8 *)"RTP")) {
            mDecoder = NULL;
        }
    }

    ~AmrCodec() {
        if (mEncoder) {
            AMREncodeExit(&mEncoder, &mSidSync);
        }
        if (mDecoder) {
            GSMDecodeFrameExit(&mDecoder);
        }
    }

    int set(int sampleRate, const char *fmtp);
    int encode(int mode, void *payload, int16_t *samples);
    int decode(int16_t *samples, int count, void *payload, int length);

public:
    void *mEncoder;
    void *mSidSync;
    void *mDecoder;

    bool mOctetAligned;
};

int AmrCodec::set(int sampleRate, const char *fmtp)
{
    // These parameters are not supported.
    if (strcasestr(fmtp, "crc=1") || strcasestr(fmtp, "robust-sorting=1") ||
        strcasestr(fmtp, "interleaving=")) {
        return -1;
    }

    // Handle mode-set and octet-align.
    // const char *modes = strcasestr(fmtp, "mode-set=");
    // if (modes) {
        // mMode = 0;
        // mModeSet = 0;
        // for (char c = *modes; c && c != ' '; c = *++modes) {
            // if (c >= '0' && c <= '7') {
                // int mode = c - '0';
                // if (mode > mMode) {
                    // mMode = mode;
                // }
                // mModeSet |= 1 << mode;
            // }
        // }
    // } else {
        // mMode = 7;
        // mModeSet = 0xFF;
    // }
    mOctetAligned = (strcasestr(fmtp, "octet-align=1") != NULL);

    // TODO: handle mode-change-*.

    return (sampleRate == 8000 && mEncoder && mDecoder) ? 160 : -1;
}

int AmrCodec::encode(int mode, void *payload, int16_t *samples)
{
    unsigned char *bytes = (unsigned char *)payload;
    Frame_Type_3GPP type;

    int length = AMREncode(mEncoder, mSidSync, (Mode)mode,
        samples, bytes + 1, &type, AMR_TX_WMF);

    if (type != mode || length != (8 + gFrameBits[mode] + 7) >> 3) {
        return -1;
    }

    if (mOctetAligned) {
        bytes[0] = 0xF0;
        bytes[1] = (mode << 3) | 0x04;
        ++length;
    } else {
        // CMR = 15 (4-bit), F = 0 (1-bit), FT = mode (4-bit), Q = 1 (1-bit).
        bytes[0] = 0xFF;
        bytes[1] = 0xC0 | (mode << 1) | 1;

        // Shift left 6 bits and update the length.
        bytes[length + 1] = 0;
        for (int i = 0; i <= length; ++i) {
            bytes[i] = (bytes[i] << 6) | (bytes[i + 1] >> 2);
        }
        length = (10 + gFrameBits[mode] + 7) >> 3;
    }
    return length;
}

int AmrCodec::decode(int16_t *samples, int count, void *payload, int length)
{
    unsigned char *bytes = (unsigned char *)payload;
    Frame_Type_3GPP type;
    if (length < 2) {
        return -1;
    }
    int request = bytes[0] >> 4;

    if (mOctetAligned) {
        if ((bytes[1] & 0xC4) != 0x04) {
            return -1;
        }
        type = (Frame_Type_3GPP)(bytes[1] >> 3);
        if (length != (16 + gFrameBits[type] + 7) >> 3) {
            return -1;
        }
        length -= 2;
        bytes += 2;
    } else {
        if ((bytes[0] & 0x0C) || !(bytes[1] & 0x40)) {
            return -1;
        }
        type = (Frame_Type_3GPP)((bytes[0] << 1 | bytes[1] >> 7) & 0x07);
        if (length != (10 + gFrameBits[type] + 7) >> 3) {
            return -1;
        }

        // Shift left 2 bits and update the length.
        --length;
        for (int i = 1; i < length; ++i) {
            bytes[i] = (bytes[i] << 2) | (bytes[i + 1] >> 6);
        }
        bytes[length] <<= 2;
        length = (gFrameBits[type] + 7) >> 3;
        ++bytes;
    }

    if (AMRDecode(mDecoder, type, bytes, samples, MIME_IETF) != length) {
        return -1;
    }

    // Handle CMR
    // if (request < 8 && request != mMode) {
        // for (int i = request; i >= 0; --i) {
            // if (mModeSet & (1 << i)) {
                // mMode = request;
                // break;
            // }
        // }
    // }

    return 160;
}

//------------------------------------------------------------------------------

// See RFC 3551 for the encoding details.

class GsmEfrCodec 
{
public:
    GsmEfrCodec() {
        if (AMREncodeInit(&mEncoder, &mSidSync, false)) {
            mEncoder = NULL;
        }
        if (GSMInitDecode(&mDecoder, (Word8 *)"RTP")) {
            mDecoder = NULL;
        }
    }

    ~GsmEfrCodec() {
        if (mEncoder) {
            AMREncodeExit(&mEncoder, &mSidSync);
        }
        if (mDecoder) {
            GSMDecodeFrameExit(&mDecoder);
        }
    }

    int set(int sampleRate, const char *fmtp) {
        return (sampleRate == 8000 && mEncoder && mDecoder) ? 160 : -1;
    }

    int encode(void *payload, int16_t *samples);
    int decode(int16_t *samples, int count, void *payload, int length);

private:
    void *mEncoder;
    void *mSidSync;
    void *mDecoder;
};

int GsmEfrCodec::encode(void *payload, int16_t *samples)
{
    unsigned char *bytes = (unsigned char *)payload;
    Frame_Type_3GPP type;

    int length = AMREncode(mEncoder, mSidSync, MR122,
        samples, bytes, &type, AMR_TX_WMF);

    if (type == AMR_122 && length == 32) {
        bytes[0] = 0xC0 | (bytes[1] >> 4);
        for (int i = 1; i < 31; ++i) {
            bytes[i] = (bytes[i] << 4) | (bytes[i + 1] >> 4);
        }
        return 31;
    }
    return -1;
}

int GsmEfrCodec::decode(int16_t *samples, int count, void *payload, int length)
{
    unsigned char *bytes = (unsigned char *)payload;
    int n = 0;
    while (n + 160 <= count && length >= 31 && (bytes[0] >> 4) == 0x0C) {
        for (int i = 0; i < 30; ++i) {
            bytes[i] = (bytes[i] << 4) | (bytes[i + 1] >> 4);
        }
        bytes[30] <<= 4;

        if (AMRDecode(mDecoder, AMR_122, bytes, &samples[n], MIME_IETF) != 31) {
            break;
        }
        n += 160;
        length -= 31;
        bytes += 31;
    }
    return n;
}

} // namespace

extern "C" {

jint amr_init
  (JNIEnv *env, jclass)
{
    AmrCodec* codec = new AmrCodec();
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
jint amr_encode
  (JNIEnv *env, jclass, jint handle, jint mode, jshortArray samples, jbyteArray encoded)
{
    AmrCodec* codec =  (AmrCodec*)handle;

    jsize inLen = env->GetArrayLength(samples);
    jshort inBuf[inLen]; 
    env->GetShortArrayRegion(samples, 0, inLen, inBuf);

    jsize outLen = env->GetArrayLength(encoded);
    jbyte outBuf[outLen];
    int encodeLength;

    encodeLength = codec->encode(mode, (void*)outBuf, (int16_t*)inBuf);

    env->SetByteArrayRegion(encoded, 0, encodeLength, outBuf);
    
    return encodeLength;
}

/*
 * Class:     
 * Method:    decode
 * Signature: ([BI[SI)I
 */
jint amr_decode
  (JNIEnv *env, jclass, jint handle, jbyteArray encoded, jint encodedSize, jshortArray samples)
{
    AmrCodec* codec =  (AmrCodec*)handle;

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
void amr_close
  (JNIEnv *, jclass, jint handle)
{
    delete (AmrCodec*)handle;
}

}

static JNINativeMethod gMethods[] = {
	{ "amr_init", "()I", (void *)amr_init },
	{ "amr_encode", "(II[S[B)I", (void *)amr_encode },
	{ "amr_decode", "(I[BI[S)I", (void *)amr_decode },
	{ "amr_close", "(I)V", (void *)amr_close },
};



int registerAmrCodec(JNIEnv *env)
{
	jclass clazz;
    clazz = env->FindClass("com/csipsimple/codecs/JniCodec");
	
    env->RegisterNatives(clazz, gMethods, NELEM(gMethods));

    return 0;
}
