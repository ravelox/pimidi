
/*
#    Sfront, a SAOL to C translator    
#    This file: Network library -- crypto functions
#
# Copyright (c) 2000, Regents of the University of California
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#  Redistributions of source code must retain the above copyright
#  notice, this list of conditions and the following disclaimer.
#
#  Redistributions in binary form must reproduce the above copyright
#  notice, this list of conditions and the following disclaimer in the
#  documentation and/or other materials provided with the distribution.
#
#  Neither the name of the University of California, Berkeley nor the
#  names of its contributors may be used to endorse or promote products
#  derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#    Maintainer: John Lazzaro, lazzaro@cs.berkeley.edu
*/

#include <ctype.h>
#include <string.h>

/*******************************************************************/
/* beginning of MD5 code, extensively altered from Aladdin version */
/*                 BSD applies to our modified form only           */
/*******************************************************************/

/*
  Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  L. Peter Deutsch
  ghost@aladdin.com

 */

/*******************************************************************/
/*               macros, typedefs, and global variables            */
/*******************************************************************/

#define NSYS_ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

#define NSYS_FUNCT_F(x, y, z) (((x) & (y)) | (~(x) & (z)))
#define NSYS_FUNCT_G(x, y, z) (((x) & (z)) | ((y) & ~(z)))
#define NSYS_FUNCT_H(x, y, z) ((x) ^ (y) ^ (z))
#define NSYS_FUNCT_I(x, y, z) ((y) ^ ((x) | ~(z)))

#define NSYS_SETF(a, b, c, d, k, s, Ti) t = a + NSYS_FUNCT_F(b,c,d) +X[k]+ Ti;\
                                        a = NSYS_ROTATE_LEFT(t, s) + b
#define NSYS_SETG(a, b, c, d, k, s, Ti) t = a + NSYS_FUNCT_G(b,c,d) +X[k]+ Ti;\
                                        a = NSYS_ROTATE_LEFT(t, s) + b
#define NSYS_SETH(a, b, c, d, k, s, Ti) t = a + NSYS_FUNCT_H(b,c,d) +X[k]+ Ti;\
                                        a = NSYS_ROTATE_LEFT(t, s) + b
#define NSYS_SETI(a, b, c, d, k, s, Ti) t = a + NSYS_FUNCT_I(b,c,d) +X[k]+ Ti;\
                                        a = NSYS_ROTATE_LEFT(t, s) + b

/* holds state for an MD5 computation */

typedef struct nsys_md5_state_s {
  unsigned int count[2];           /* message length in bits, lsw first */
  unsigned int abcd[4];	           /* digest buffer */
  unsigned char buf[64];           /* accumulate block */
} nsys_md5_state_t;

/* MD5 padding array */

static unsigned char nsys_md5_pad[64] = {
  0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/*******************************************************************/
/*               initializes MD5 computation                       */
/*******************************************************************/

void nsys_md5_init(nsys_md5_state_t * pms)

{
  pms->count[0] = pms->count[1] = 0;
  pms->abcd[0] = 0x67452301;
  pms->abcd[1] = 0xefcdab89;
  pms->abcd[2] = 0x98badcfe;
  pms->abcd[3] = 0x10325476;
}

/***********************************************************************/
/*  process a block -- used in nsys_md5_append() and nsys_md5_finish() */
/***********************************************************************/

void nsys_md5_process(nsys_md5_state_t * pms, unsigned char *data)

{
  unsigned int a, b, c, d, t, X[16], i;
  unsigned char *xp = data;

  a = pms->abcd[0]; 
  b = pms->abcd[1];
  c = pms->abcd[2]; 
  d = pms->abcd[3];

  for (i = 0; i < 16; ++i, xp += 4)
    X[i] = xp[0] + (xp[1] << 8) + (xp[2] << 16) + (xp[3] << 24);

  NSYS_SETF(a, b, c, d,  0,  7,  0xd76aa478);
  NSYS_SETF(d, a, b, c,  1, 12,  0xe8c7b756);
  NSYS_SETF(c, d, a, b,  2, 17,  0x242070db);
  NSYS_SETF(b, c, d, a,  3, 22,  0xc1bdceee);
  NSYS_SETF(a, b, c, d,  4,  7,  0xf57c0faf);
  NSYS_SETF(d, a, b, c,  5, 12,  0x4787c62a);
  NSYS_SETF(c, d, a, b,  6, 17,  0xa8304613);
  NSYS_SETF(b, c, d, a,  7, 22,  0xfd469501);
  NSYS_SETF(a, b, c, d,  8,  7,  0x698098d8);
  NSYS_SETF(d, a, b, c,  9, 12,  0x8b44f7af);
  NSYS_SETF(c, d, a, b, 10, 17,  0xffff5bb1);
  NSYS_SETF(b, c, d, a, 11, 22,  0x895cd7be);
  NSYS_SETF(a, b, c, d, 12,  7,  0x6b901122);
  NSYS_SETF(d, a, b, c, 13, 12,  0xfd987193);
  NSYS_SETF(c, d, a, b, 14, 17,  0xa679438e);
  NSYS_SETF(b, c, d, a, 15, 22,  0x49b40821);

  NSYS_SETG(a, b, c, d,  1,  5, 0xf61e2562);
  NSYS_SETG(d, a, b, c,  6,  9, 0xc040b340);
  NSYS_SETG(c, d, a, b, 11, 14, 0x265e5a51);
  NSYS_SETG(b, c, d, a,  0, 20, 0xe9b6c7aa);
  NSYS_SETG(a, b, c, d,  5,  5, 0xd62f105d);
  NSYS_SETG(d, a, b, c, 10,  9, 0x02441453);
  NSYS_SETG(c, d, a, b, 15, 14, 0xd8a1e681);
  NSYS_SETG(b, c, d, a,  4, 20, 0xe7d3fbc8);
  NSYS_SETG(a, b, c, d,  9,  5, 0x21e1cde6);
  NSYS_SETG(d, a, b, c, 14,  9, 0xc33707d6);
  NSYS_SETG(c, d, a, b,  3, 14, 0xf4d50d87);
  NSYS_SETG(b, c, d, a,  8, 20, 0x455a14ed);
  NSYS_SETG(a, b, c, d, 13,  5, 0xa9e3e905);
  NSYS_SETG(d, a, b, c,  2,  9, 0xfcefa3f8);
  NSYS_SETG(c, d, a, b,  7, 14, 0x676f02d9);
  NSYS_SETG(b, c, d, a, 12, 20, 0x8d2a4c8a);

  NSYS_SETH(a, b, c, d,  5,  4, 0xfffa3942);
  NSYS_SETH(d, a, b, c,  8, 11, 0x8771f681);
  NSYS_SETH(c, d, a, b, 11, 16, 0x6d9d6122);
  NSYS_SETH(b, c, d, a, 14, 23, 0xfde5380c);
  NSYS_SETH(a, b, c, d,  1,  4, 0xa4beea44);
  NSYS_SETH(d, a, b, c,  4, 11, 0x4bdecfa9);
  NSYS_SETH(c, d, a, b,  7, 16, 0xf6bb4b60);
  NSYS_SETH(b, c, d, a, 10, 23, 0xbebfbc70);
  NSYS_SETH(a, b, c, d, 13,  4, 0x289b7ec6);
  NSYS_SETH(d, a, b, c,  0, 11, 0xeaa127fa);
  NSYS_SETH(c, d, a, b,  3, 16, 0xd4ef3085);
  NSYS_SETH(b, c, d, a,  6, 23, 0x04881d05);
  NSYS_SETH(a, b, c, d,  9,  4, 0xd9d4d039);
  NSYS_SETH(d, a, b, c, 12, 11, 0xe6db99e5);
  NSYS_SETH(c, d, a, b, 15, 16, 0x1fa27cf8);
  NSYS_SETH(b, c, d, a,  2, 23, 0xc4ac5665);

  NSYS_SETI(a, b, c, d,  0,  6, 0xf4292244);
  NSYS_SETI(d, a, b, c,  7, 10, 0x432aff97);
  NSYS_SETI(c, d, a, b, 14, 15, 0xab9423a7);
  NSYS_SETI(b, c, d, a,  5, 21, 0xfc93a039);
  NSYS_SETI(a, b, c, d, 12,  6, 0x655b59c3);
  NSYS_SETI(d, a, b, c,  3, 10, 0x8f0ccc92);
  NSYS_SETI(c, d, a, b, 10, 15, 0xffeff47d);
  NSYS_SETI(b, c, d, a,  1, 21, 0x85845dd1);
  NSYS_SETI(a, b, c, d,  8,  6, 0x6fa87e4f);
  NSYS_SETI(d, a, b, c, 15, 10, 0xfe2ce6e0);
  NSYS_SETI(c, d, a, b,  6, 15, 0xa3014314);
  NSYS_SETI(b, c, d, a, 13, 21, 0x4e0811a1);
  NSYS_SETI(a, b, c, d,  4,  6, 0xf7537e82);
  NSYS_SETI(d, a, b, c, 11, 10, 0xbd3af235);
  NSYS_SETI(c, d, a, b,  2, 15, 0x2ad7d2bb);
  NSYS_SETI(b, c, d, a,  9, 21, 0xeb86d391);

  pms->abcd[0] += a;
  pms->abcd[1] += b;
  pms->abcd[2] += c;
  pms->abcd[3] += d;
}

#undef NSYS_ROTATE_LEFT
#undef NSYS_FUNCT_F
#undef NSYS_FUNCT_G
#undef NSYS_FUNCT_H
#undef NSYS_FUNCT_I
#undef NSYS_SETF
#undef NSYS_SETG
#undef NSYS_SETH
#undef NSYS_SETI

/*******************************************************************/
/*               append phase of MD5 computation                   */
/*******************************************************************/

void nsys_md5_append(nsys_md5_state_t * pms, unsigned char * data, int nbytes)

{
  unsigned char *p;
  int left, offset, copy;
  unsigned int nbits;

  if (nbytes <= 0)
    return;

  p = data;
  left = nbytes;
  offset = (pms->count[0] >> 3) & 63;
  nbits  = (unsigned int)(nbytes << 3);

  /* Update the message length. */

  pms->count[1] += nbytes >> 29;
  pms->count[0] += nbits;

  if (pms->count[0] < nbits)
    pms->count[1]++;

  /* Process an initial partial block. */
  
  if (offset) 
    {
      copy = (offset + nbytes > 64 ? 64 - offset : nbytes);
      
      memcpy(pms->buf + offset, p, copy);
      if (offset + copy < 64)
	return;

      p += copy;
      left -= copy;
      nsys_md5_process(pms, pms->buf);
    }

  /* Process full blocks. */
  
  for (; left >= 64; p += 64, left -= 64)
    nsys_md5_process(pms, p);
  
  /* Process a final partial block. */
  
  if (left)
    memcpy(pms->buf, p, left);
}

/*******************************************************************/
/*               final phase of MD5 computation                   */
/*******************************************************************/

void nsys_md5_finish(nsys_md5_state_t * pms, unsigned char digest[16])

{
  unsigned char data[8];
  int i;

  /* Save the length before padding. */

  for (i = 0; i < 8; ++i)
    data[i] = (unsigned char)(pms->count[i >> 2] >> ((i & 3) << 3));

  /* Pad to 56 bytes mod 64. */

  nsys_md5_append(pms, nsys_md5_pad, ((55 - (pms->count[0] >> 3)) & 63) + 1);

  /* Append the length. */

  nsys_md5_append(pms, data, 8);

  for (i = 0; i < 16; ++i)
    digest[i] = (unsigned char)(pms->abcd[i >> 2] >> ((i & 3) << 3));
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*           end of code covered by Alladin Copyright              */
/*_________________________________________________________________*/


/*******************************************************************/
/*               function call to compute MD5                      */
/*******************************************************************/

unsigned char * nsys_md5(unsigned char * digest, unsigned char * text, int len)

{
  nsys_md5_state_t state;

  nsys_md5_init(&state);	
  nsys_md5_append(&state, text, len);
  nsys_md5_finish(&state, digest);
  return digest;
}

/*********************************************************/
/*   computes hmac-md5, key is always a 16-byte digest   */
/*                modified code from RFC2104             */
/*********************************************************/

char * nsys_hmac_md5(unsigned char* text, int text_len, 
		     unsigned char * keydigest, unsigned char * digest)

{
  nsys_md5_state_t context;
  unsigned char k_ipad[64], k_opad[64];
  int i;

  memcpy(k_ipad, keydigest, 16);
  memset(&(k_ipad[16]), 0, 48);
  memcpy(k_opad, keydigest, 16);
  memset(&(k_opad[16]), 0, 48);

  for (i=0; i<64; i++) 
    {
      k_ipad[i] ^= 0x36;
      k_opad[i] ^= 0x5c;
    }

  nsys_md5_init(&context);
  nsys_md5_append(&context, k_ipad, 64); 
  nsys_md5_append(&context, text, text_len);
  nsys_md5_finish(&context, digest);  

  nsys_md5_init(&context);
  nsys_md5_append(&context, k_opad, 64);
  nsys_md5_append(&context, digest, 16);
  nsys_md5_finish(&context, digest);
  return (char *) digest;
}


/******************************************************************/
/*          constant array for base64 translation                 */
/******************************************************************/

unsigned char nsys_b64map[64] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 
  'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 
  'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
}; 


/***********************************************************/
/* converts 16-byte hash function to 25-byte output string */
/***********************************************************/

unsigned char * nsys_digest_base64(unsigned char * output, 
				   unsigned char * input)

{
  int i, j;

  j = 0;
  for (i = 0; i <= 12; i += 3)
    {
      output[j++] = nsys_b64map[input[i] >> 2];
      output[j++] = nsys_b64map[((input[i] & 3) << 4) | (input[i+1] >> 4)];
      output[j++] = nsys_b64map[((input[i+1] & 15) << 2) | (input[i+2] >> 6)];
      output[j++] = nsys_b64map[input[i+2] & 63];
    }
  output[j++] = nsys_b64map[input[i] >> 2];
  output[j++] = nsys_b64map[(input[i] & 3) << 4];
  output[j++] = '=';
  output[j++] = '=';
  output[j++] = '\0';
  return output;
}

/****************************************************************/
/*           checks string for Base64 MD5 Digest format         */
/****************************************************************/

int nsys_digest_syntaxcheck(char * s)

{
  int ret = 1;
  int i;

  if ((strlen(s) == 24) && (s[23] == '=') && (s[22] == '=') && 
	  ((s[21] == 'A') || (s[21] == 'Q') || (s[21] == 'g') || 
	   (s[21] == 'w')))
    {
      for (i=0; i < 21; i++)
	ret &= (isalnum((int)s[i]) || (s[i] == '+') || (s[i] == '/')); 
    }
  else
    ret = 0;

  return ret;
}

/*******************************************************************/
/*                end of crypto network library                    */
/*******************************************************************/
