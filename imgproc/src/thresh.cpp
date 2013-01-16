/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#include "precomp.hpp"

namespace cv
{

static void
thresh_8u( const Mat& _src, Mat& _dst, uchar thresh, uchar maxval, int type )
{
    int i, j, j_scalar = 0;
    uchar tab[256];
    Size roi = _src.size();
    roi.width *= _src.channels();

    if( _src.isContinuous() && _dst.isContinuous() )
    {
        roi.width *= roi.height;
        roi.height = 1;
    }

#ifdef HAVE_TEGRA_OPTIMIZATION
    if (tegra::thresh_8u(_src, _dst, roi.width, roi.height, thresh, maxval, type))
        return;
#endif

    switch( type )
    {
    case THRESH_BINARY:
        for( i = 0; i <= thresh; i++ )
            tab[i] = 0;
        for( ; i < 256; i++ )
            tab[i] = maxval;
        break;
    case THRESH_BINARY_INV:
        for( i = 0; i <= thresh; i++ )
            tab[i] = maxval;
        for( ; i < 256; i++ )
            tab[i] = 0;
        break;
    case THRESH_TRUNC:
        for( i = 0; i <= thresh; i++ )
            tab[i] = (uchar)i;
        for( ; i < 256; i++ )
            tab[i] = thresh;
        break;
    case THRESH_TOZERO:
        for( i = 0; i <= thresh; i++ )
            tab[i] = 0;
        for( ; i < 256; i++ )
            tab[i] = (uchar)i;
        break;
    case THRESH_TOZERO_INV:
        for( i = 0; i <= thresh; i++ )
            tab[i] = (uchar)i;
        for( ; i < 256; i++ )
            tab[i] = 0;
        break;
    default:
        CV_Error( CV_StsBadArg, "Unknown threshold type" );
    }

#if CV_NEON
	#pragma mark ANUVIS team debugging NEON version
    
    if (cv::useOptimized()) {
        
    #ifdef ANUVIS_TEST
    CV_Error( CV_StsBadArg, "thresh_8u using Neon was called!" );
    #endif
	    
    //set the 16 signed 8-bit integer values to '\x80' (128)
    int8x16_t _x80 = vdupq_n_s8('\x80');
    int8x16_t thresh_u = vdupq_n_s8(thresh);
    int8x16_t thresh_s = vdupq_n_s8(thresh ^ 0x80);
	int8x16_t maxval_ = vdupq_n_s8(maxval);
    
    //set the 8 signed 8-bit integer value constants
    //we need these shorter equivilents for the last chunk of 32 to avoid overflow
    int8x8_t h_x80 = vdup_n_s8('\x80');
    int8x8_t hthresh_u = vdup_n_s8(thresh);
    int8x8_t hthresh_s = vdup_n_s8(thresh ^ 0x80);
	int8x8_t hmaxval_ = vdup_n_s8(maxval);
    
    //set the 16 signed 8-bit integer values to '\x80' (128)
    uint8x16_t uthresh_u = vdupq_n_u8(thresh);
    
    //set the 8 signed 8-bit integer value constants
    //we need these shorter equivilents for the last chunk of 32 to avoid overflow
    
    uint8x8_t uhthresh_u = vdup_n_u8(thresh);
    
    
    j_scalar = roi.width & -8;
    
    //process it row
    for( i = 0; i < roi.height; i++ )
    {
        const uchar* src = (const uchar*)(_src.data + _src.step*i);
        uchar* dst = (uchar*)(_dst.data + _dst.step*i);

        switch( type )
        {
        case THRESH_BINARY://~2x
            #ifdef ANUVIS_TEST
            CV_Error( CV_StsBadArg, "THRESH_BINARY in thresh_8u using Neon was called!" );
            #endif
            
            //whilst treating 32 signed 8 bit ints at once (except for the last 32)
            
            for (j = 0; j < roi.width - 32; j += 32) {
                int8x16_t v0, v1;
                
                //load 128-bit value
                v0 = vld1q_s8((const int8_t *)(src + j));
                v1 = vld1q_s8((const int8_t *) (src + j + 16));
                
                //compares the 16 signed 8-bit integers between a > b
                //computes the bitwise XOR of the 128-bit value in a and b
                v0 = (int8x16_t)vcgtq_s8(veorq_s8(v0, _x80), thresh_s);
                v1 = (int8x16_t)vcgtq_s8(veorq_s8(v1, _x80), thresh_s);
                
                //computes the bitwise AND of the 128-bit value in a and b
                v0 = vandq_s8(v0, maxval_);
                v1 = vandq_s8(v1, maxval_);
                
                //store it
                vst1q_s8((int8_t *)(dst + j), v0);
                vst1q_s8((int8_t *)(dst + j + 16), v1);
            }
            
			//treat the last 32
            for ( ; j <= roi.width - 8; j += 8) {
                int8x8_t v0 = vld1_s8((const int8_t *)(src + j));
                v0 = (int8x8_t)vcgt_s8(veor_s8(v0, h_x80), hthresh_s);
                v0 = vand_s8(v0, hmaxval_);
                vst1_s8((int8_t *)(dst + j), v0);
            }
               
			break;
			
        case THRESH_BINARY_INV://~2x
             
            //whilst treating 32 signed 8 bit ints at once (except for the last 32)
            
            for (j = 0; j < roi.width - 32; j += 32) {
                int8x16_t v0, v1;
                
                //load 128-bit value
                v0 = vld1q_s8((const int8_t *)(src + j));
                v1 = vld1q_s8((const int8_t *) (src + j + 16));
                
                //compares the 16 signed 8-bit integers between a > b
                //computes the bitwise XOR of the 128-bit value in a and b
                v0 = (int8x16_t)vcgtq_s8(veorq_s8(v0, _x80), thresh_s);
                v1 = (int8x16_t)vcgtq_s8(veorq_s8(v1, _x80), thresh_s);
                
                //computes the bitwise AND of the 128-bit value in a and b
                //note a is negated using the bitwise not
                v0 = vandq_s8(vmvnq_s8(v0), maxval_);
                v1 = vandq_s8(vmvnq_s8(v1), maxval_);
                
                //store it
                vst1q_s8((int8_t *)(dst + j), v0);
                vst1q_s8((int8_t *)(dst + j + 16), v1);
            }
            
            //treat the last 32
            for ( ; j <= roi.width - 8; j += 8) {
                int8x8_t v0 = vld1_s8((const int8_t *)(src + j));
                v0 = (int8x8_t)vcgt_s8(veor_s8(v0, h_x80), hthresh_s);
                v0 = vand_s8(vmvn_s8(v0), hmaxval_);
                vst1_s8((int8_t *)(dst + j), v0);
            }
            
            break;

        case THRESH_TRUNC:
            
            //begin
            for (j = 0; j < roi.width - 32; j += 32) {
                uint8x16_t v0, v1;
                
                //load 128-bit value
                v0 = vld1q_u8((const uint8_t *)(src + j));
                v1 = vld1q_u8((const uint8_t *) (src + j + 16));
                
                //Subtracts the 16 unsigned 8-bit integers of b from a and saturates
                v0 = vqsubq_u8(v0, vqsubq_u8(v0, uthresh_u));
                v1 = vqsubq_u8(v1, vqsubq_u8(v1, uthresh_u));
                
                //store it
                vst1q_u8((uint8_t *)(dst + j), v0);
                vst1q_u8((uint8_t *)(dst + j + 16), v1);
            }
            
            //treat the last 32
            for ( ; j <= roi.width - 8; j += 8) {
                uint8x8_t v0 = vld1_u8((const uint8_t *)(src + j));
                v0 = vqsub_u8(v0, vqsub_u8(v0, uhthresh_u));
                vst1_u8((uint8_t *)(dst + j), v0);
            }
            break;

        case THRESH_TOZERO:
                
            for (j = 0; j < roi.width - 32; j += 32) {
                int8x16_t v0, v1;
                
                //load 128-bit value
                v0 = vld1q_s8((const int8_t *)(src + j));
                v1 = vld1q_s8((const int8_t *) (src + j + 16));
                
                //computes the bitwise AND of the 128-bit value in a and b
                //compares the 16 signed 8-bit integers between a > b
                //computes the bitwise XOR of the 128-bit value in a and b
                v0 = vandq_s8(v0,(int8x16_t)vcgtq_s8(veorq_s8(v0, _x80), thresh_s));
                v1 = vandq_s8(v1, (int8x16_t)vcgtq_s8(veorq_s8(v1, _x80), thresh_s));
                
                //store it
                vst1q_s8((int8_t *)(dst + j), v0);
                vst1q_s8((int8_t *)(dst + j + 16), v1);
            }
            
            //treat the last 32
            for ( ; j <= roi.width - 8; j += 8) {
                int8x8_t v0 = vld1_s8((const int8_t *)(src + j));
                v0 = vand_s8(v0, (int8x8_t)vcgt_s8(veor_s8(v0, h_x80), hthresh_s));
                vst1_s8((int8_t *)(dst + j), v0);
            }
                
            break;

        case THRESH_TOZERO_INV:
            
            for (j = 0; j < roi.width - 32; j += 32) {
                int8x16_t v0, v1;
                
                //load 128-bit value
                v0 = vld1q_s8((const int8_t *)(src + j));
                v1 = vld1q_s8((const int8_t *) (src + j + 16));
                
                //bitwise NOT on a
                //computes the bitwise AND of the 128-bit value in a and b
                //compares the 16 signed 8-bit integers between a > b
                //computes the bitwise XOR of the 128-bit value in a and b
                v0 = vandq_s8(vmvnq_s8((int8x16_t)vcgtq_s8(veorq_s8(v0, _x80), thresh_s)),v0);
                v1 = vandq_s8(vmvnq_s8((int8x16_t)vcgtq_s8(veorq_s8(v1, _x80), thresh_s)),v1);
                
                //store it
                vst1q_s8((int8_t *)(dst + j), v0);
                vst1q_s8((int8_t *)(dst + j + 16), v1);
            }
            
            //treat the last 32
            for ( ; j <= roi.width - 8; j += 8) {
                int8x8_t v0 = vld1_s8((const int8_t *)(src + j));
                v0 = vand_s8(vmvn_s8((int8x8_t)vcgt_s8(veor_s8(v0, h_x80), hthresh_s)),v0);
                vst1_s8((int8_t *)(dst + j), v0);
            }
            
            break;
        }
    }
    }
#endif

#if CV_SSE2
    if( checkHardwareSupport(CV_CPU_SSE2) )
    {
        __m128i _x80 = _mm_set1_epi8('\x80');
        __m128i thresh_u = _mm_set1_epi8(thresh);
        __m128i thresh_s = _mm_set1_epi8(thresh ^ 0x80);
        __m128i maxval_ = _mm_set1_epi8(maxval);
        j_scalar = roi.width & -8;

        for( i = 0; i < roi.height; i++ )
        {
            const uchar* src = (const uchar*)(_src.data + _src.step*i);
            uchar* dst = (uchar*)(_dst.data + _dst.step*i);

            switch( type )
            {
            case THRESH_BINARY:
                for( j = 0; j <= roi.width - 32; j += 32 )
                {
                    __m128i v0, v1;
                    v0 = _mm_loadu_si128( (const __m128i*)(src + j) );
                    v1 = _mm_loadu_si128( (const __m128i*)(src + j + 16) );
                    v0 = _mm_cmpgt_epi8( _mm_xor_si128(v0, _x80), thresh_s );
                    v1 = _mm_cmpgt_epi8( _mm_xor_si128(v1, _x80), thresh_s );
                    v0 = _mm_and_si128( v0, maxval_ );
                    v1 = _mm_and_si128( v1, maxval_ );
                    _mm_storeu_si128( (__m128i*)(dst + j), v0 );
                    _mm_storeu_si128( (__m128i*)(dst + j + 16), v1 );
                }

                for( ; j <= roi.width - 8; j += 8 )
                {
                    __m128i v0 = _mm_loadl_epi64( (const __m128i*)(src + j) );
                    v0 = _mm_cmpgt_epi8( _mm_xor_si128(v0, _x80), thresh_s );
                    v0 = _mm_and_si128( v0, maxval_ );
                    _mm_storel_epi64( (__m128i*)(dst + j), v0 );
                }
                break;

            case THRESH_BINARY_INV:
                for( j = 0; j <= roi.width - 32; j += 32 )
                {
                    __m128i v0, v1;
                    v0 = _mm_loadu_si128( (const __m128i*)(src + j) );
                    v1 = _mm_loadu_si128( (const __m128i*)(src + j + 16) );
                    v0 = _mm_cmpgt_epi8( _mm_xor_si128(v0, _x80), thresh_s );
                    v1 = _mm_cmpgt_epi8( _mm_xor_si128(v1, _x80), thresh_s );
                    v0 = _mm_andnot_si128( v0, maxval_ );
                    v1 = _mm_andnot_si128( v1, maxval_ );
                    _mm_storeu_si128( (__m128i*)(dst + j), v0 );
                    _mm_storeu_si128( (__m128i*)(dst + j + 16), v1 );
                }

                for( ; j <= roi.width - 8; j += 8 )
                {
                    __m128i v0 = _mm_loadl_epi64( (const __m128i*)(src + j) );
                    v0 = _mm_cmpgt_epi8( _mm_xor_si128(v0, _x80), thresh_s );
                    v0 = _mm_andnot_si128( v0, maxval_ );
                    _mm_storel_epi64( (__m128i*)(dst + j), v0 );
                }
                break;

            case THRESH_TRUNC:
                for( j = 0; j <= roi.width - 32; j += 32 )
                {
                    __m128i v0, v1;
                    v0 = _mm_loadu_si128( (const __m128i*)(src + j) );
                    v1 = _mm_loadu_si128( (const __m128i*)(src + j + 16) );
                    v0 = _mm_subs_epu8( v0, _mm_subs_epu8( v0, thresh_u ));
                    v1 = _mm_subs_epu8( v1, _mm_subs_epu8( v1, thresh_u ));
                    _mm_storeu_si128( (__m128i*)(dst + j), v0 );
                    _mm_storeu_si128( (__m128i*)(dst + j + 16), v1 );
                }

                for( ; j <= roi.width - 8; j += 8 )
                {
                    __m128i v0 = _mm_loadl_epi64( (const __m128i*)(src + j) );
                    v0 = _mm_subs_epu8( v0, _mm_subs_epu8( v0, thresh_u ));
                    _mm_storel_epi64( (__m128i*)(dst + j), v0 );
                }
                break;

            case THRESH_TOZERO:
                for( j = 0; j <= roi.width - 32; j += 32 )
                {
                    __m128i v0, v1;
                    v0 = _mm_loadu_si128( (const __m128i*)(src + j) );
                    v1 = _mm_loadu_si128( (const __m128i*)(src + j + 16) );
                    v0 = _mm_and_si128( v0, _mm_cmpgt_epi8(_mm_xor_si128(v0, _x80), thresh_s ));
                    v1 = _mm_and_si128( v1, _mm_cmpgt_epi8(_mm_xor_si128(v1, _x80), thresh_s ));
                    _mm_storeu_si128( (__m128i*)(dst + j), v0 );
                    _mm_storeu_si128( (__m128i*)(dst + j + 16), v1 );
                }

                for( ; j <= roi.width - 8; j += 8 )
                {
                    __m128i v0 = _mm_loadl_epi64( (const __m128i*)(src + j) );
                    v0 = _mm_and_si128( v0, _mm_cmpgt_epi8(_mm_xor_si128(v0, _x80), thresh_s ));
                    _mm_storel_epi64( (__m128i*)(dst + j), v0 );
                }
                break;

            case THRESH_TOZERO_INV:
                for( j = 0; j <= roi.width - 32; j += 32 )
                {
                    __m128i v0, v1;
                    v0 = _mm_loadu_si128( (const __m128i*)(src + j) );
                    v1 = _mm_loadu_si128( (const __m128i*)(src + j + 16) );
                    v0 = _mm_andnot_si128( _mm_cmpgt_epi8(_mm_xor_si128(v0, _x80), thresh_s ), v0 );
                    v1 = _mm_andnot_si128( _mm_cmpgt_epi8(_mm_xor_si128(v1, _x80), thresh_s ), v1 );
                    _mm_storeu_si128( (__m128i*)(dst + j), v0 );
                    _mm_storeu_si128( (__m128i*)(dst + j + 16), v1 );
                }

                for( ; j <= roi.width - 8; j += 8 )
                {
                    __m128i v0 = _mm_loadl_epi64( (const __m128i*)(src + j) );
                    v0 = _mm_andnot_si128( _mm_cmpgt_epi8(_mm_xor_si128(v0, _x80), thresh_s ), v0 );
                    _mm_storel_epi64( (__m128i*)(dst + j), v0 );
                }
                break;
            }
        }
    }
#endif

    if( j_scalar < roi.width )
    {
        for( i = 0; i < roi.height; i++ )
        {
            const uchar* src = (const uchar*)(_src.data + _src.step*i);
            uchar* dst = (uchar*)(_dst.data + _dst.step*i);
            j = j_scalar;
#if CV_ENABLE_UNROLLED
            for( ; j <= roi.width - 4; j += 4 )
            {
                uchar t0 = tab[src[j]];
                uchar t1 = tab[src[j+1]];

                dst[j] = t0;
                dst[j+1] = t1;

                t0 = tab[src[j+2]];
                t1 = tab[src[j+3]];

                dst[j+2] = t0;
                dst[j+3] = t1;
            }
#endif
            for( ; j < roi.width; j++ )
                dst[j] = tab[src[j]];
        }
    }
}


static void
thresh_16s( const Mat& _src, Mat& _dst, short thresh, short maxval, int type )
{
    int i, j;
    Size roi = _src.size();
    roi.width *= _src.channels();
    const short* src = (const short*)_src.data;
    short* dst = (short*)_dst.data;
    size_t src_step = _src.step/sizeof(src[0]);
    size_t dst_step = _dst.step/sizeof(dst[0]);

#if CV_SSE2
    volatile bool useSIMD = checkHardwareSupport(CV_CPU_SSE);
#endif

    if( _src.isContinuous() && _dst.isContinuous() )
    {
        roi.width *= roi.height;
        roi.height = 1;
    }

#ifdef HAVE_TEGRA_OPTIMIZATION
    if (tegra::thresh_16s(_src, _dst, roi.width, roi.height, thresh, maxval, type))
        return;
#endif

    switch( type )
    {
    case THRESH_BINARY:
        for( i = 0; i < roi.height; i++, src += src_step, dst += dst_step )
        {
            j = 0;
            
        #if CV_NEON //~2.2x
            
            int16x8_t thresh8 = vdupq_n_s16(thresh);
            int16x8_t maxval8 = vdupq_n_s16(maxval);
            
            //load each 128-bit chunk of signed 16-bit ints
            for( ; j <= roi.width - 16; j += 16 )
            {
                int16x8_t v0, v1;
                
                v0 = vld1q_s16((const int16_t *)(src + j));
                v1 = vld1q_s16((const int16_t *)(src + j + 8));
                
                v0 = (int16x8_t)vcgtq_s16(v0, thresh8);
                v1 = (int16x8_t)vcgtq_s16(v1, thresh8);
                
                //computes the bitwise AND of the 128-bit value in a and b
                v0 = vandq_s16(v0, maxval8);
                v1 = vandq_s16(v1, maxval8);
                
                //store it
                vst1q_s16((int16_t *)(dst + j), v0);
                vst1q_s16((int16_t *)(dst + j + 8), v1);
                
            }
            
        #endif
            
        #if CV_SSE2
            if( useSIMD )
            {
                __m128i thresh8 = _mm_set1_epi16(thresh), maxval8 = _mm_set1_epi16(maxval);
                for( ; j <= roi.width - 16; j += 16 )
                {
                    __m128i v0, v1;
                    v0 = _mm_loadu_si128( (const __m128i*)(src + j) );
                    v1 = _mm_loadu_si128( (const __m128i*)(src + j + 8) );
                    v0 = _mm_cmpgt_epi16( v0, thresh8 );
                    v1 = _mm_cmpgt_epi16( v1, thresh8 );
                    v0 = _mm_and_si128( v0, maxval8 );
                    v1 = _mm_and_si128( v1, maxval8 );
                    _mm_storeu_si128((__m128i*)(dst + j), v0 );
                    _mm_storeu_si128((__m128i*)(dst + j + 8), v1 );
                }
            }
        #endif

            for( ; j < roi.width; j++ )
                dst[j] = src[j] > thresh ? maxval : 0;
        }
        break;

    case THRESH_BINARY_INV:
        for( i = 0; i < roi.height; i++, src += src_step, dst += dst_step )
        {
            j = 0;
            
        #if CV_NEON
            
            int16x8_t thresh8 = vdupq_n_s16(thresh);
            int16x8_t maxval8 = vdupq_n_s16(maxval);
            
            //load each 128-bit chunk of signed 16-bit ints
            for( ; j <= roi.width - 16; j += 16 )
            {
                int16x8_t v0, v1;
                
                v0 = vld1q_s16((const int16_t *)(src + j));
                v1 = vld1q_s16((const int16_t *)(src + j + 8));
                
                v0 = (int16x8_t)vcgtq_s16(v0, thresh8);
                v1 = (int16x8_t)vcgtq_s16(v1, thresh8);
                
                //computes the bitwise AND NOT of the 128-bit value in a and b
                v0 = vandq_s16(vmvnq_s16(v0), maxval8);
                v1 = vandq_s16(vmvnq_s16(v1), maxval8);
                
                //store it
                vst1q_s16((int16_t *)(dst + j), v0);
                vst1q_s16((int16_t *)(dst + j + 8), v1);
                
            }
            
        #endif
            
        #if CV_SSE2
            if( useSIMD )
            {
                __m128i thresh8 = _mm_set1_epi16(thresh), maxval8 = _mm_set1_epi16(maxval);
                for( ; j <= roi.width - 16; j += 16 )
                {
                    __m128i v0, v1;
                    v0 = _mm_loadu_si128( (const __m128i*)(src + j) );
                    v1 = _mm_loadu_si128( (const __m128i*)(src + j + 8) );
                    v0 = _mm_cmpgt_epi16( v0, thresh8 );
                    v1 = _mm_cmpgt_epi16( v1, thresh8 );
                    v0 = _mm_andnot_si128( v0, maxval8 );
                    v1 = _mm_andnot_si128( v1, maxval8 );
                    _mm_storeu_si128((__m128i*)(dst + j), v0 );
                    _mm_storeu_si128((__m128i*)(dst + j + 8), v1 );
                }
            }
        #endif

            for( ; j < roi.width; j++ )
                dst[j] = src[j] <= thresh ? maxval : 0;
        }
        break;

    case THRESH_TRUNC:
        for( i = 0; i < roi.height; i++, src += src_step, dst += dst_step )
        {
            j = 0;
            
        #if CV_NEON
            
            int16x8_t thresh8 = vdupq_n_s16(thresh);
            
            //load each 128-bit chunk of signed 16-bit ints
            for( ; j <= roi.width - 16; j += 16 )
            {
                int16x8_t v0, v1;
                
                v0 = vld1q_s16((const int16_t *)(src + j));
                v1 = vld1q_s16((const int16_t *)(src + j + 8));
                
                //pairwise minimum
                v0 = (int16x8_t)vminq_s16(v0, thresh8);
                v1 = (int16x8_t)vminq_s16(v1, thresh8);
                
                //store it
                vst1q_s16((int16_t *)(dst + j), v0);
                vst1q_s16((int16_t *)(dst + j + 8), v1);
                
            }
            
        #endif
            
        #if CV_SSE2
            if( useSIMD )
            {
                __m128i thresh8 = _mm_set1_epi16(thresh);
                for( ; j <= roi.width - 16; j += 16 )
                {
                    __m128i v0, v1;
                    v0 = _mm_loadu_si128( (const __m128i*)(src + j) );
                    v1 = _mm_loadu_si128( (const __m128i*)(src + j + 8) );
                    v0 = _mm_min_epi16( v0, thresh8 );
                    v1 = _mm_min_epi16( v1, thresh8 );
                    _mm_storeu_si128((__m128i*)(dst + j), v0 );
                    _mm_storeu_si128((__m128i*)(dst + j + 8), v1 );
                }
            }
        #endif

            for( ; j < roi.width; j++ )
                dst[j] = std::min(src[j], thresh);
        }
        break;

    case THRESH_TOZERO:
        for( i = 0; i < roi.height; i++, src += src_step, dst += dst_step )
        {
            j = 0;
            
        
        #if CV_NEON
            
            int16x8_t thresh8 = vdupq_n_s16(thresh);
            
            //load each 128-bit chunk of signed 16-bit ints
            for( ; j <= roi.width - 16; j += 16 )
            {
                int16x8_t v0, v1;
                
                v0 = vld1q_s16((const int16_t *)(src + j));
                v1 = vld1q_s16((const int16_t *)(src + j + 8));
                
                v0 = vandq_s16(v0, (int16x8_t)vcgtq_s16(v0, thresh8));
                v1 = vandq_s16(v1, (int16x8_t)vcgtq_s16(v1, thresh8));
                
                //store it
                vst1q_s16((int16_t *)(dst + j), v0);
                vst1q_s16((int16_t *)(dst + j + 8), v1);
                
            }

            
        #endif
            
        #if CV_SSE2
            if( useSIMD )
            {
                __m128i thresh8 = _mm_set1_epi16(thresh);
                for( ; j <= roi.width - 16; j += 16 )
                {
                    __m128i v0, v1;
                    v0 = _mm_loadu_si128( (const __m128i*)(src + j) );
                    v1 = _mm_loadu_si128( (const __m128i*)(src + j + 8) );
                    v0 = _mm_and_si128(v0, _mm_cmpgt_epi16(v0, thresh8));
                    v1 = _mm_and_si128(v1, _mm_cmpgt_epi16(v1, thresh8));
                    _mm_storeu_si128((__m128i*)(dst + j), v0 );
                    _mm_storeu_si128((__m128i*)(dst + j + 8), v1 );
                }
            }
        #endif

            for( ; j < roi.width; j++ )
            {
                short v = src[j];
                dst[j] = v > thresh ? v : 0;
            }
        }
        break;

    case THRESH_TOZERO_INV:
        for( i = 0; i < roi.height; i++, src += src_step, dst += dst_step )
        {
            j = 0;
                    
        #if CV_NEON
            
            int16x8_t thresh8 = vdupq_n_s16(thresh);
            
            //load each 128-bit chunk of signed 16-bit ints
            for( ; j <= roi.width - 16; j += 16 )
            {
                int16x8_t v0, v1;
                
                v0 = vld1q_s16((const int16_t *)(src + j));
                v1 = vld1q_s16((const int16_t *)(src + j + 8));
                
                v0 = vandq_s16(vmvnq_s16((int16x8_t)vcgtq_s16(v0, thresh8)), v0);
                v1 = vandq_s16(vmvnq_s16((int16x8_t)vcgtq_s16(v1, thresh8)), v1);
                
                //store it
                vst1q_s16((int16_t *)(dst + j), v0);
                vst1q_s16((int16_t *)(dst + j + 8), v1);
                
            }
            
        #endif
            
        #if CV_SSE2
            if( useSIMD )
            {
                __m128i thresh8 = _mm_set1_epi16(thresh);
                for( ; j <= roi.width - 16; j += 16 )
                {
                    __m128i v0, v1;
                    v0 = _mm_loadu_si128( (const __m128i*)(src + j) );
                    v1 = _mm_loadu_si128( (const __m128i*)(src + j + 8) );
                    v0 = _mm_andnot_si128(_mm_cmpgt_epi16(v0, thresh8), v0);
                    v1 = _mm_andnot_si128(_mm_cmpgt_epi16(v1, thresh8), v1);
                    _mm_storeu_si128((__m128i*)(dst + j), v0 );
                    _mm_storeu_si128((__m128i*)(dst + j + 8), v1 );
                }
            }
        #endif
            for( ; j < roi.width; j++ )
            {
                short v = src[j];
                dst[j] = v <= thresh ? v : 0;
            }
        }
        break;
    default:
        return CV_Error( CV_StsBadArg, "" );
    }
}


static void
thresh_32f( const Mat& _src, Mat& _dst, float thresh, float maxval, int type )
{
    int i, j;
    Size roi = _src.size();
    roi.width *= _src.channels();
    const float* src = (const float*)_src.data;
    float* dst = (float*)_dst.data;
    size_t src_step = _src.step/sizeof(src[0]);
    size_t dst_step = _dst.step/sizeof(dst[0]);

#if CV_SSE2
    volatile bool useSIMD = checkHardwareSupport(CV_CPU_SSE);
#endif

    if( _src.isContinuous() && _dst.isContinuous() )
    {
        roi.width *= roi.height;
        roi.height = 1;
    }

#ifdef HAVE_TEGRA_OPTIMIZATION
    if (tegra::thresh_32f(_src, _dst, roi.width, roi.height, thresh, maxval, type))
        return;
#endif

    switch( type )
    {
        case THRESH_BINARY:
            for( i = 0; i < roi.height; i++, src += src_step, dst += dst_step )
            {
                j = 0;
                
#pragma mark ANUVIS team debugging
#if CV_NEON
                /*
                //treat 256 bits (8 floats) at a time
                float32x4_t thresh4 = vdupq_n_f32(thresh);
                float32x4_t maxval4 = vdupq_n_f32(maxval);
                
                for( ; j <= roi.width - 8; j += 8 )
                {
                    float32x4_t v0, v1;
                    
                    v0 = vld1q_f32((const float32_t *)(src + j));
                    v1 = vld1q_f32((const float32_t *)(src + j + 4));
                    
                    v0 = (float32x4_t)vcgtq_f32(v0, thresh4);
                    v1 = (float32x4_t)vcgtq_f32(v1, thresh4);
                    
                    v0 = (float32x4_t)vandq_s32((int32x4_t)v0, (int32x4_t)thresh4);
                    v0 = (float32x4_t)vandq_s32((int32x4_t)v1, (int32x4_t)thresh4);
                    
                    vst1q_f32((float32_t*)dst + j, v0);
                    vst1q_f32((float32_t*)dst + j + 4, v1);
                }
                
                */
#endif
                
#if CV_SSE2
                if( useSIMD )
                {
                    __m128 thresh4 = _mm_set1_ps(thresh), maxval4 = _mm_set1_ps(maxval);
                    for( ; j <= roi.width - 8; j += 8 )
                    {
                        __m128 v0, v1;
                        v0 = _mm_loadu_ps( src + j );
                        v1 = _mm_loadu_ps( src + j + 4 );
                        v0 = _mm_cmpgt_ps( v0, thresh4 );
                        v1 = _mm_cmpgt_ps( v1, thresh4 );
                        v0 = _mm_and_ps( v0, maxval4 );
                        v1 = _mm_and_ps( v1, maxval4 );
                        _mm_storeu_ps( dst + j, v0 );
                        _mm_storeu_ps( dst + j + 4, v1 );
                    }
                }
#endif

                for( ; j < roi.width; j++ )
                    dst[j] = src[j] > thresh ? maxval : 0;
            }
            break;

        case THRESH_BINARY_INV:
            for( i = 0; i < roi.height; i++, src += src_step, dst += dst_step )
            {
                j = 0;
#if CV_SSE2
                if( useSIMD )
                {
                    __m128 thresh4 = _mm_set1_ps(thresh), maxval4 = _mm_set1_ps(maxval);
                    for( ; j <= roi.width - 8; j += 8 )
                    {
                        __m128 v0, v1;
                        v0 = _mm_loadu_ps( src + j );
                        v1 = _mm_loadu_ps( src + j + 4 );
                        v0 = _mm_cmple_ps( v0, thresh4 );
                        v1 = _mm_cmple_ps( v1, thresh4 );
                        v0 = _mm_and_ps( v0, maxval4 );
                        v1 = _mm_and_ps( v1, maxval4 );
                        _mm_storeu_ps( dst + j, v0 );
                        _mm_storeu_ps( dst + j + 4, v1 );
                    }
                }
#endif

                for( ; j < roi.width; j++ )
                    dst[j] = src[j] <= thresh ? maxval : 0;
            }
            break;

        case THRESH_TRUNC:
            for( i = 0; i < roi.height; i++, src += src_step, dst += dst_step )
            {
                j = 0;
#if CV_SSE2
                if( useSIMD )
                {
                    __m128 thresh4 = _mm_set1_ps(thresh);
                    for( ; j <= roi.width - 8; j += 8 )
                    {
                        __m128 v0, v1;
                        v0 = _mm_loadu_ps( src + j );
                        v1 = _mm_loadu_ps( src + j + 4 );
                        v0 = _mm_min_ps( v0, thresh4 );
                        v1 = _mm_min_ps( v1, thresh4 );
                        _mm_storeu_ps( dst + j, v0 );
                        _mm_storeu_ps( dst + j + 4, v1 );
                    }
                }
#endif

                for( ; j < roi.width; j++ )
                    dst[j] = std::min(src[j], thresh);
            }
            break;

        case THRESH_TOZERO:
            for( i = 0; i < roi.height; i++, src += src_step, dst += dst_step )
            {
                j = 0;
#if CV_SSE2
                if( useSIMD )
                {
                    __m128 thresh4 = _mm_set1_ps(thresh);
                    for( ; j <= roi.width - 8; j += 8 )
                    {
                        __m128 v0, v1;
                        v0 = _mm_loadu_ps( src + j );
                        v1 = _mm_loadu_ps( src + j + 4 );
                        v0 = _mm_and_ps(v0, _mm_cmpgt_ps(v0, thresh4));
                        v1 = _mm_and_ps(v1, _mm_cmpgt_ps(v1, thresh4));
                        _mm_storeu_ps( dst + j, v0 );
                        _mm_storeu_ps( dst + j + 4, v1 );
                    }
                }
#endif

                for( ; j < roi.width; j++ )
                {
                    float v = src[j];
                    dst[j] = v > thresh ? v : 0;
                }
            }
            break;

        case THRESH_TOZERO_INV:
            for( i = 0; i < roi.height; i++, src += src_step, dst += dst_step )
            {
                j = 0;
#if CV_SSE2
                if( useSIMD )
                {
                    __m128 thresh4 = _mm_set1_ps(thresh);
                    for( ; j <= roi.width - 8; j += 8 )
                    {
                        __m128 v0, v1;
                        v0 = _mm_loadu_ps( src + j );
                        v1 = _mm_loadu_ps( src + j + 4 );
                        v0 = _mm_and_ps(v0, _mm_cmple_ps(v0, thresh4));
                        v1 = _mm_and_ps(v1, _mm_cmple_ps(v1, thresh4));
                        _mm_storeu_ps( dst + j, v0 );
                        _mm_storeu_ps( dst + j + 4, v1 );
                    }
                }
#endif
                for( ; j < roi.width; j++ )
                {
                    float v = src[j];
                    dst[j] = v <= thresh ? v : 0;
                }
            }
            break;
        default:
            return CV_Error( CV_StsBadArg, "" );
    }
}


static double
getThreshVal_Otsu_8u( const Mat& _src )
{
    Size size = _src.size();
    if( _src.isContinuous() )
    {
        size.width *= size.height;
        size.height = 1;
    }
    const int N = 256;
    int i, j, h[N] = {0};
    for( i = 0; i < size.height; i++ )
    {
        const uchar* src = _src.data + _src.step*i;
        j = 0;
        #if CV_ENABLE_UNROLLED
        for( ; j <= size.width - 4; j += 4 )
        {
            int v0 = src[j], v1 = src[j+1];
            h[v0]++; h[v1]++;
            v0 = src[j+2]; v1 = src[j+3];
            h[v0]++; h[v1]++;
        }
        #endif
        for( ; j < size.width; j++ )
            h[src[j]]++;
    }

    double mu = 0, scale = 1./(size.width*size.height);
    for( i = 0; i < N; i++ )
        mu += i*(double)h[i];

    mu *= scale;
    double mu1 = 0, q1 = 0;
    double max_sigma = 0, max_val = 0;

    for( i = 0; i < N; i++ )
    {
        double p_i, q2, mu2, sigma;

        p_i = h[i]*scale;
        mu1 *= q1;
        q1 += p_i;
        q2 = 1. - q1;

        if( std::min(q1,q2) < FLT_EPSILON || std::max(q1,q2) > 1. - FLT_EPSILON )
            continue;

        mu1 = (mu1 + i*p_i)/q1;
        mu2 = (mu - q1*mu1)/q2;
        sigma = q1*q2*(mu1 - mu2)*(mu1 - mu2);
        if( sigma > max_sigma )
        {
            max_sigma = sigma;
            max_val = i;
        }
    }

    return max_val;
}

class ThresholdRunner
{
public:
    ThresholdRunner(Mat _src, Mat _dst, int _nStripes, double _thresh, double _maxval, int _thresholdType)
    {
        src = _src;
        dst = _dst;

        nStripes = _nStripes;

        thresh = _thresh;
        maxval = _maxval;
        thresholdType = _thresholdType;
    }

    void operator () ( const BlockedRange& range ) const
    {
        int row0 = std::min(cvRound(range.begin() * src.rows / nStripes), src.rows);
        int row1 = std::min(cvRound(range.end() * src.rows / nStripes), src.rows);

        /*if(0)
            printf("Size = (%d, %d), range[%d,%d), row0 = %d, row1 = %d\n",
                   src.rows, src.cols, range.begin(), range.end(), row0, row1);*/

        Mat srcStripe = src.rowRange(row0, row1);
        Mat dstStripe = dst.rowRange(row0, row1);

        if (srcStripe.depth() == CV_8U)
        {
            thresh_8u( srcStripe, dstStripe, (uchar)thresh, (uchar)maxval, thresholdType );
        }
        else if( srcStripe.depth() == CV_16S )
        {
            thresh_16s( srcStripe, dstStripe, (short)thresh, (short)maxval, thresholdType );
        }
        else if( srcStripe.depth() == CV_32F )
        {
            thresh_32f( srcStripe, dstStripe, (float)thresh, (float)maxval, thresholdType );
        }
    }

private:
    Mat src;
    Mat dst;
    int nStripes;

    double thresh;
    double maxval;
    int thresholdType;
};

}

double cv::threshold( InputArray _src, OutputArray _dst, double thresh, double maxval, int type )
{
    Mat src = _src.getMat();
    bool use_otsu = (type & THRESH_OTSU) != 0;
    type &= THRESH_MASK;

    if( use_otsu )
    {
        CV_Assert( src.type() == CV_8UC1 );
        thresh = getThreshVal_Otsu_8u(src);
    }

    _dst.create( src.size(), src.type() );
    Mat dst = _dst.getMat();

    int nStripes = 1;
#if defined HAVE_TBB && defined ANDROID
    nStripes = 4;
#endif

    if( src.depth() == CV_8U )
    {
        int ithresh = cvFloor(thresh);
        thresh = ithresh;
        int imaxval = cvRound(maxval);
        if( type == THRESH_TRUNC )
            imaxval = ithresh;
        imaxval = saturate_cast<uchar>(imaxval);

        if( ithresh < 0 || ithresh >= 255 )
        {
            if( type == THRESH_BINARY || type == THRESH_BINARY_INV ||
                ((type == THRESH_TRUNC || type == THRESH_TOZERO_INV) && ithresh < 0) ||
                (type == THRESH_TOZERO && ithresh >= 255) )
            {
                int v = type == THRESH_BINARY ? (ithresh >= 255 ? 0 : imaxval) :
                        type == THRESH_BINARY_INV ? (ithresh >= 255 ? imaxval : 0) :
                        /*type == THRESH_TRUNC ? imaxval :*/ 0;
                dst.setTo(v);
            }
            else
                src.copyTo(dst);
        }
        else
        {
            parallel_for(BlockedRange(0, nStripes),
                         ThresholdRunner(src, dst, nStripes, (uchar)ithresh, (uchar)imaxval, type));
        }
    }
    else if( src.depth() == CV_16S )
    {
        int ithresh = cvFloor(thresh);
        thresh = ithresh;
        int imaxval = cvRound(maxval);
        if( type == THRESH_TRUNC )
            imaxval = ithresh;
        imaxval = saturate_cast<short>(imaxval);

        if( ithresh < SHRT_MIN || ithresh >= SHRT_MAX )
        {
            if( type == THRESH_BINARY || type == THRESH_BINARY_INV ||
               ((type == THRESH_TRUNC || type == THRESH_TOZERO_INV) && ithresh < SHRT_MIN) ||
               (type == THRESH_TOZERO && ithresh >= SHRT_MAX) )
            {
                int v = type == THRESH_BINARY ? (ithresh >= SHRT_MAX ? 0 : imaxval) :
                type == THRESH_BINARY_INV ? (ithresh >= SHRT_MAX ? imaxval : 0) :
                /*type == THRESH_TRUNC ? imaxval :*/ 0;
                dst.setTo(v);
            }
            else
                src.copyTo(dst);
        }
        else
        {
            parallel_for(BlockedRange(0, nStripes),
                         ThresholdRunner(src, dst, nStripes, (short)ithresh, (short)imaxval, type));
        }
    }
    else if( src.depth() == CV_32F )
    {
        parallel_for(BlockedRange(0, nStripes),
                     ThresholdRunner(src, dst, nStripes, (float)thresh, (float)maxval, type));
    }
    else
        CV_Error( CV_StsUnsupportedFormat, "" );

    return thresh;
}


void cv::adaptiveThreshold( InputArray _src, OutputArray _dst, double maxValue,
                            int method, int type, int blockSize, double delta )
{
    Mat src = _src.getMat();
    CV_Assert( src.type() == CV_8UC1 );
    CV_Assert( blockSize % 2 == 1 && blockSize > 1 );
    Size size = src.size();

    _dst.create( size, src.type() );
    Mat dst = _dst.getMat();

    if( maxValue < 0 )
    {
        dst = Scalar(0);
        return;
    }

    Mat mean;

    if( src.data != dst.data )
        mean = dst;

    if( method == ADAPTIVE_THRESH_MEAN_C )
        boxFilter( src, mean, src.type(), Size(blockSize, blockSize),
                   Point(-1,-1), true, BORDER_REPLICATE );
    else if( method == ADAPTIVE_THRESH_GAUSSIAN_C )
        GaussianBlur( src, mean, Size(blockSize, blockSize), 0, 0, BORDER_REPLICATE );
    else
        CV_Error( CV_StsBadFlag, "Unknown/unsupported adaptive threshold method" );

    int i, j;
    uchar imaxval = saturate_cast<uchar>(maxValue);
    int idelta = type == THRESH_BINARY ? cvCeil(delta) : cvFloor(delta);
    uchar tab[768];

    if( type == CV_THRESH_BINARY )
        for( i = 0; i < 768; i++ )
            tab[i] = (uchar)(i - 255 > -idelta ? imaxval : 0);
    else if( type == CV_THRESH_BINARY_INV )
        for( i = 0; i < 768; i++ )
            tab[i] = (uchar)(i - 255 <= -idelta ? imaxval : 0);
    else
        CV_Error( CV_StsBadFlag, "Unknown/unsupported threshold type" );

    if( src.isContinuous() && mean.isContinuous() && dst.isContinuous() )
    {
        size.width *= size.height;
        size.height = 1;
    }

    for( i = 0; i < size.height; i++ )
    {
        const uchar* sdata = src.data + src.step*i;
        const uchar* mdata = mean.data + mean.step*i;
        uchar* ddata = dst.data + dst.step*i;

        for( j = 0; j < size.width; j++ )
            ddata[j] = tab[sdata[j] - mdata[j] + 255];
    }
}

CV_IMPL double
cvThreshold( const void* srcarr, void* dstarr, double thresh, double maxval, int type )
{
    cv::Mat src = cv::cvarrToMat(srcarr), dst = cv::cvarrToMat(dstarr), dst0 = dst;

    CV_Assert( src.size == dst.size && src.channels() == dst.channels() &&
        (src.depth() == dst.depth() || dst.depth() == CV_8U));

    thresh = cv::threshold( src, dst, thresh, maxval, type );
    if( dst0.data != dst.data )
        dst.convertTo( dst0, dst0.depth() );
    return thresh;
}


CV_IMPL void
cvAdaptiveThreshold( const void *srcIm, void *dstIm, double maxValue,
                     int method, int type, int blockSize, double delta )
{
    cv::Mat src = cv::cvarrToMat(srcIm), dst = cv::cvarrToMat(dstIm);
    CV_Assert( src.size == dst.size && src.type() == dst.type() );
    cv::adaptiveThreshold( src, dst, maxValue, method, type, blockSize, delta );
}

/* End of file. */
