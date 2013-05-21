
/*
#    Sfront, a SAOL to C translator    
#    This file: Network library -- constants and externs
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
#
*/


/*************************/
/* network include files */
/*************************/

#ifndef NSYS_NET_INCLUDE

#define NSYS_NET_INCLUDE

#ifndef NSYS_NET

#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <ctype.h>
#include "net_local.h"

#endif

#include <sys/ioctl.h>
#include <unistd.h> 
#include <fcntl.h>
#include <time.h>  
#include <sys/time.h>  
#include <sys/types.h> 
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h> 
#include <net/if.h>

#if (defined(sun) || defined(__sun__))
#include <sys/sockio.h>
#endif

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                           Constants                                    */
/*________________________________________________________________________*/

/***********************/
/* debug/warning flags */
/***********************/

#define NSYS_WARN_NONE     0    /* never warn about anything */
#define NSYS_WARN_STANDARD 1    /* print connect messages    */
#define NSYS_WARN_UNUSUAL  2    /* print unusual events      */

#define NSYS_WARN   NSYS_WARN_UNUSUAL

#define NSYS_JOURNAL_DEBUG_OFF 0
#define NSYS_JOURNAL_DEBUG_ON 1

#define NSYS_JOURNAL_DEBUG NSYS_JOURNAL_DEBUG_OFF

#define NSYS_LATENOTES_DEBUG_OFF 0
#define NSYS_LATENOTES_DEBUG_ON 1

#define NSYS_LATENOTES_DEBUG NSYS_LATENOTES_DEBUG_OFF

#define NSYS_LOSSMODEL_OFF       0
#define NSYS_LOSSMODEL_ON        1

#define NSYS_LOSSMODEL_LOSSRATE   0.2F      /* between 0.0F and 1.0F */
#define NSYS_LOSSMODEL NSYS_LOSSMODEL_OFF

#define NSYS_NETOUT_CHAPTERE_OFF   0        /* don't send Chapter E */
#define NSYS_NETOUT_CHAPTERE_ON    1        /* send Chapter E       */

#define NSYS_NETOUT_CHAPTERE_STATUS   NSYS_NETOUT_CHAPTERE_OFF

/********************/
/* network constants */
/********************/

#define NSYS_MAXRETRY 256
#define NSYS_DONE  0
#define NSYS_ERROR 1

#define NSYS_NONBLOCK 0
#define NSYS_BLOCK    1

#define NSYS_BUFFSIZE 5120

/*****************************/
/* default system parameters */
/*****************************/

#if defined(__APPLE__)          /* variable-length ifconf */
#define NSYS_IFCONF_VARLEN 1
#else
#define NSYS_IFCONF_VARLEN 0
#endif

#ifndef SO_BSDCOMPAT
#define SO_BSDCOMPAT 0
#endif

#ifndef NSYS_MSETS
#define NSYS_MSETS 5
#endif

#ifndef ACYCLE
#define ACYCLE 42L
#endif

#ifndef ARATE
#define ARATE 44100.0F
#endif

#ifndef APPNAME
#define APPNAME "unknown"
#endif

#ifndef APPVERSION
#define APPVERSION "0.0"
#endif

#ifndef CSYS_MIDI_NUMCHAN
#define CSYS_MIDI_NUMCHAN  16
#endif

#ifndef CSYS_MIDI_NOOP
#define CSYS_MIDI_NOOP     0x70u
#endif

#ifndef CSYS_MIDI_POWERUP
#define CSYS_MIDI_POWERUP  0x73u
#endif

#ifndef CSYS_MIDI_TSTART  
#define CSYS_MIDI_TSTART   0x7Du
#endif

#ifndef CSYS_MIDI_MANUEX  
#define CSYS_MIDI_MANUEX   0x7Du
#endif

#ifndef CSYS_MIDI_MVOLUME  
#define CSYS_MIDI_MVOLUME  0x7Eu
#endif

#ifndef CSYS_MIDI_GMRESET  
#define CSYS_MIDI_GMRESET  0x7Fu
#endif

#ifndef CSYS_MIDI_NOTEOFF
#define CSYS_MIDI_NOTEOFF  0x80u
#endif

#ifndef CSYS_MIDI_NOTEON 
#define CSYS_MIDI_NOTEON   0x90u
#endif

#ifndef CSYS_MIDI_PTOUCH  
#define CSYS_MIDI_PTOUCH   0xA0u
#endif

#ifndef CSYS_MIDI_CC  
#define CSYS_MIDI_CC       0xB0u
#endif

#ifndef CSYS_MIDI_PROGRAM 
#define CSYS_MIDI_PROGRAM  0xC0u
#endif

#ifndef CSYS_MIDI_CTOUCH 
#define CSYS_MIDI_CTOUCH   0xD0u
#endif

#ifndef CSYS_MIDI_WHEEL 
#define CSYS_MIDI_WHEEL    0xE0u
#endif

#ifndef CSYS_MIDI_SYSTEM
#define CSYS_MIDI_SYSTEM   0xF0u
#endif

#ifndef CSYS_MIDI_SYSTEM_SYSEX_START
#define CSYS_MIDI_SYSTEM_SYSEX_START  0xF0u
#endif

#ifndef CSYS_MIDI_SYSTEM_QFRAME
#define CSYS_MIDI_SYSTEM_QFRAME       0xF1u
#endif

#ifndef CSYS_MIDI_SYSTEM_SONG_PP
#define CSYS_MIDI_SYSTEM_SONG_PP      0xF2u
#endif

#ifndef CSYS_MIDI_SYSTEM_SONG_SELECT
#define CSYS_MIDI_SYSTEM_SONG_SELECT  0xF3u
#endif

#ifndef CSYS_MIDI_SYSTEM_UNUSED1
#define CSYS_MIDI_SYSTEM_UNUSED1      0xF4u
#endif

#ifndef CSYS_MIDI_SYSTEM_UNUSED2
#define CSYS_MIDI_SYSTEM_UNUSED2      0xF5u
#endif

#ifndef CSYS_MIDI_SYSTEM_TUNE_REQUEST
#define CSYS_MIDI_SYSTEM_TUNE_REQUEST 0xF6u
#endif

#ifndef CSYS_MIDI_SYSTEM_SYSEX_END
#define CSYS_MIDI_SYSTEM_SYSEX_END    0xF7u
#endif

#ifndef CSYS_MIDI_SYSTEM_CLOCK
#define CSYS_MIDI_SYSTEM_CLOCK        0xF8u
#endif

#ifndef CSYS_MIDI_SYSTEM_TICK
#define CSYS_MIDI_SYSTEM_TICK         0xF9u
#endif

#ifndef CSYS_MIDI_SYSTEM_START
#define CSYS_MIDI_SYSTEM_START        0xFAu
#endif

#ifndef CSYS_MIDI_SYSTEM_CONTINUE
#define CSYS_MIDI_SYSTEM_CONTINUE     0xFBu
#endif

#ifndef CSYS_MIDI_SYSTEM_STOP
#define CSYS_MIDI_SYSTEM_STOP         0xFCu
#endif

#ifndef CSYS_MIDI_SYSTEM_UNUSED3
#define CSYS_MIDI_SYSTEM_UNUSED3      0xFDu
#endif

#ifndef CSYS_MIDI_SYSTEM_SENSE
#define CSYS_MIDI_SYSTEM_SENSE        0xFEu
#endif

#ifndef CSYS_MIDI_SYSTEM_RESET
#define CSYS_MIDI_SYSTEM_RESET        0xFFu
#endif

#ifndef CSYS_MIDI_CC_BANKSELECT_MSB 
#define CSYS_MIDI_CC_BANKSELECT_MSB  0x00u
#endif

#ifndef CSYS_MIDI_CC_MODWHEEL_MSB
#define CSYS_MIDI_CC_MODWHEEL_MSB    0x01u
#endif

#ifndef CSYS_MIDI_CC_DATAENTRY_MSB
#define CSYS_MIDI_CC_DATAENTRY_MSB   0x06u
#endif

#ifndef CSYS_MIDI_CC_CHANVOLUME_MSB
#define CSYS_MIDI_CC_CHANVOLUME_MSB  0x07u
#endif

#ifndef CSYS_MIDI_CC_BANKSELECT_LSB 
#define CSYS_MIDI_CC_BANKSELECT_LSB  0x20u
#endif

#ifndef CSYS_MIDI_CC_DATAENTRY_LSB
#define CSYS_MIDI_CC_DATAENTRY_LSB  0x26u
#endif

#ifndef CSYS_MIDI_CC_SUSTAIN
#define CSYS_MIDI_CC_SUSTAIN         0x40u
#endif

#ifndef CSYS_MIDI_CC_PORTAMENTOSRC 
#define CSYS_MIDI_CC_PORTAMENTOSRC   0x54u
#endif
    
#ifndef CSYS_MIDI_CC_DATAENTRYPLUS
#define CSYS_MIDI_CC_DATAENTRYPLUS   0x60u
#endif

#ifndef CSYS_MIDI_CC_DATAENTRYMINUS
#define CSYS_MIDI_CC_DATAENTRYMINUS  0x61u
#endif

#ifndef CSYS_MIDI_CC_NRPN_LSB
#define CSYS_MIDI_CC_NRPN_LSB        0x62u
#endif

#ifndef CSYS_MIDI_CC_NRPN_MSB
#define CSYS_MIDI_CC_NRPN_MSB        0x63u
#endif

#ifndef CSYS_MIDI_CC_RPN_LSB
#define CSYS_MIDI_CC_RPN_LSB         0x64u
#endif

#ifndef CSYS_MIDI_CC_RPN_MSB
#define CSYS_MIDI_CC_RPN_MSB         0x65u
#endif

#ifndef CSYS_MIDI_CC_ALLSOUNDOFF
#define CSYS_MIDI_CC_ALLSOUNDOFF     0x78u
#endif

#ifndef CSYS_MIDI_CC_RESETALLCONTROL
#define CSYS_MIDI_CC_RESETALLCONTROL 0x79u
#endif

#ifndef CSYS_MIDI_CC_LOCALCONTROL
#define CSYS_MIDI_CC_LOCALCONTROL    0x7Au
#endif

#ifndef CSYS_MIDI_CC_ALLNOTESOFF
#define CSYS_MIDI_CC_ALLNOTESOFF     0x7Bu
#endif

#ifndef CSYS_MIDI_CC_OMNI_OFF
#define CSYS_MIDI_CC_OMNI_OFF        0x7Cu
#endif

#ifndef CSYS_MIDI_CC_OMNI_ON
#define CSYS_MIDI_CC_OMNI_ON         0x7Du
#endif

#ifndef CSYS_MIDI_CC_MONOMODE
#define CSYS_MIDI_CC_MONOMODE        0x7Eu
#endif

#ifndef CSYS_MIDI_CC_POLYMODE
#define CSYS_MIDI_CC_POLYMODE        0x7Fu
#endif

#define CSYS_MIDI_RPN_NULL_MSB       0x7Fu
#define CSYS_MIDI_RPN_NULL_LSB       0x7Fu
#define CSYS_MIDI_NRPN_NULL_MSB      0x7Fu
#define CSYS_MIDI_NRPN_NULL_LSB      0x7Fu

#define CYS_MIDI_RPN_0_DEFAULT_MSB   0x02u 
#define CYS_MIDI_RPN_0_DEFAULT_LSB   0x00u

#define CYS_MIDI_RPN_1_DEFAULT_MSB  0x04u
#define CYS_MIDI_RPN_1_DEFAULT_LSB  0x00u

#define CYS_MIDI_RPN_2_DEFAULT_MSB  0x04u
#define CYS_MIDI_RPN_2_DEFAULT_LSB  0x00u

#define CSYS_MIDI_MANUEX_SIZE  10    /* size of Manufacturers sample Sysex */
#define CSYS_MIDI_MVOLUME_SIZE  8    /* size of Master Volume Sysex        */
#define CSYS_MIDI_GMRESET_SIZE  6    /* size of General MIDI Reset Sysex   */

#define NSYS_NETIN_SYSBUFF  (CSYS_MIDI_MANUEX_SIZE - 2)

#ifndef NSYS_DISPLAY_RTCP
#define NSYS_DISPLAY_RTCP         0
#endif

#ifndef NSYS_DISPLAY_RTCP_HDR
#define NSYS_DISPLAY_RTCP_HDR     0
#endif

#ifndef NSYS_DISPLAY_RTCP_SRINFO
#define NSYS_DISPLAY_RTCP_SRINFO  0
#endif

#ifndef NSYS_DISPLAY_RTCP_SDES 
#define NSYS_DISPLAY_RTCP_SDES    0
#endif

#ifndef NSYS_DISPLAY_RTCP_RRINFO
#define NSYS_DISPLAY_RTCP_RRINFO  0
#endif

#ifndef NSYS_DISPLAY_RTCP_RRTCOMP
#define NSYS_DISPLAY_RTCP_RRTCOMP 0
#endif

/***************/
/* rtp defines */
/***************/

/* SSRC hash table constants */

#define NSYS_HASHSIZE 32         /* table size */
#define NSYS_HASHMASK 31         /* table mask */

/* preferred value for bidirectional RTP port */

#define NSYS_RTP_PORT     5004
#define NSYS_RTP_MAXPORT 65535

/* Based on Ethernet default MTU of 1500 bytes */

#define NSYS_UDPMAXSIZE 1472  
#define NSYS_RTPMAXSIZE 1460

/* masks for combing usec and sec for srand() seed */

#define NSYS_USECMASK   (0x000FFFFF)   
#define NSYS_SECMASK    (0xFFF00000)
#define NSYS_SECSHIFT   20

/* positions of interesting parts of RTP header */

#define NSYS_RTPLOC_BYTE1   0
#define NSYS_RTPLOC_PTYPE   1
#define NSYS_RTPLOC_SEQNUM  2
#define NSYS_RTPLOC_TSTAMP  4
#define NSYS_RTPLOC_SSRC    8

/* extended sequence number constants */

#define NSYS_RTPSEQ_MAXDIFF  4000        /* max number of lost packets */
#define NSYS_RTPSEQ_HIGHEST  0xFFFFFFFF  /* highest sequence number    */
#define NSYS_RTPSEQ_LOWLIMIT 4095        /* lo edge of 16-bit sequence */
#define NSYS_RTPSEQ_HILIMIT  61440       /* hi edge of 16-bit sequence */
#define NSYS_RTPSEQ_EXMASK   0xFFFF0000  /* masks out extension        */
#define NSYS_RTPSEQ_LOMASK   0x0000FFFF  /* masks out 16-bit seqnum    */
#define NSYS_RTPSEQ_EXINCR   0x00010000  /* increments extension       */

#define NSYS_RTPSEQ_MAXLOSS   8388607    /* max number of lost packets */
#define NSYS_RTPSEQ_MINLOSS  -8388608    /* min number of lost packets */
#define NSYS_RTPSEQ_FMASK    0x00FFFFFF  /* bit mask to add fraction   */
#define NSYS_RTPSEQ_TSIGN    0x00800000  /* bit mask for sign-extend   */

/* other RTP constants */

#define NSYS_RTPVAL_BYTE1   0x80  /* V=2, P=X=CC=0   */
#define NSYS_RTPVAL_SETMARK 0x80  /* | to set marker */
#define NSYS_RTPVAL_CLRMARK 0x7F  /* & to clr marker */
#define NSYS_RTPVAL_CHKMARK 0x80  /* & to chk marker */

#define NSYS_RTPLEN_HDR  12

/* condition codes for a new RTP packet */

#define NSYS_RTPCODE_NORMAL   0   /* seqnum directly follows last packet */
#define NSYS_RTPCODE_LOSTONE  1   /* one packet has been lost            */
#define NSYS_RTPCODE_LOSTMANY 2   /* many packets have been lost         */
#define NSYS_RTPCODE_DISCARD  3   /* too late to use, or duplicate       */
#define NSYS_RTPCODE_SECURITY 4   /* possible replay attack, discard     */

/* number of bytes of digest to append */

#define NSYS_RTPSIZE_DIGEST   4

/**************/
/* rtcp sizes */
/**************/

/* sizes of complete RTCP packets */

#define NSYS_RTCPLEN_BYE      8    /* BYE packet */
#define NSYS_RTCPLEN_RREMPTY  8    /* RR with no  report    */
#define NSYS_RTCPLEN_RR      32    /* RR with one report    */
#define NSYS_RTCPLEN_SREMPTY 28    /* SR with no  reciept report */
#define NSYS_RTCPLEN_SR      52    /* SR with one receipt report */
#define NSYS_RTCPLEN_MINIMUM  8    /* the minimum packet size */

/* sizes of RTCP headers */

#define NSYS_RTCPLEN_SRHDR    8
#define NSYS_RTCPLEN_RRHDR    8
#define NSYS_RTCPLEN_SDESHDR  4
#define NSYS_RTCPLEN_BYEHDR   4

/* sizes of RTCP segments */

#define NSYS_RTCPLEN_SENDER  20
#define NSYS_RTCPLEN_REPORT  24

/* sizes of SDES headers */

#define NSYS_RTCPLEN_SDES_CHUNKHDR  4
#define NSYS_RTCPLEN_SDES_ITEMHDR   2

/******************/
/* rtcp locations */
/******************/

/* interesting shared locations in RR/SR headers */

#define NSYS_RTCPLOC_BYTE1  0
#define NSYS_RTCPLOC_PTYPE  1
#define NSYS_RTCPLOC_LENGTH 2
#define NSYS_RTCPLOC_SSRC   4

/* locations of SR sender info */

#define NSYS_RTCPLOC_SR_NTPMSB  8
#define NSYS_RTCPLOC_SR_NTPLSB 12
#define NSYS_RTCPLOC_SR_TSTAMP 16
#define NSYS_RTCPLOC_SR_PACKET 20
#define NSYS_RTCPLOC_SR_OCTET  24

/* interesting offsets into RR sender info */

#define NSYS_RTCPLOC_RR_SSRC       0
#define NSYS_RTCPLOC_RR_FRACTLOSS  4
#define NSYS_RTCPLOC_RR_NUMLOST    5
#define NSYS_RTCPLOC_RR_HISEQ      8
#define NSYS_RTCPLOC_RR_JITTER    12
#define NSYS_RTCPLOC_RR_LASTSR    16
#define NSYS_RTCPLOC_RR_DELAY     20

/* interesting locations in an SDES packet ITEM */

#define NSYS_RTCPLOC_SDESITEM_TYPE   0
#define NSYS_RTCPLOC_SDESITEM_LENGTH 1

/* interesting locations in RTCP BYE packet */

#define NSYS_RTCPLEN_BYE_SSRC       4

/***************/
/* rtcp values */
/***************/

#define NSYS_RTCPVAL_BYTE1   0x80        /* V=2, P=XC=0   */

#define NSYS_RTCPVAL_COUNTMASK   0x1F    /* mask for RC value  */
#define NSYS_RTCPVAL_COOKIEMASK  0xE0    /* mask for VP bits  */

#define NSYS_RTCPVAL_SR   0xC8    /* RTCP sender */
#define NSYS_RTCPVAL_RR   0xC9    /* RTCP receiver */
#define NSYS_RTCPVAL_SDES 0xCA    /* RTCP source desc */
#define NSYS_RTCPVAL_BYE  0xCB    /* RTCP bye */
#define NSYS_RTCPVAL_APP  0xCC    /* RTCP APP packet */

#define NSYS_RTCPVAL_SDES_CNAME   0x01  /* CNAME for SDES */
#define NSYS_RTCPVAL_SDES_NAME    0x02  /* NAME  for SDES */
#define NSYS_RTCPVAL_SDES_EMAIL   0x03  /* EMAIL for SDES */
#define NSYS_RTCPVAL_SDES_PHONE   0x04  /* PHONE for SDES */
#define NSYS_RTCPVAL_SDES_LOC     0x05  /* LOC for SDES */
#define NSYS_RTCPVAL_SDES_TOOL    0x06  /* TOOL for SDES */
#define NSYS_RTCPVAL_SDES_NOTE    0x07  /* NOTE for SDES */
#define NSYS_RTCPVAL_SDES_PRIV    0x08  /* PRIV for SDES */

#define NSYS_RTCPVAL_SDES_SIZE   9      /* number of SDES types */

/****************/
/* rtcp control */
/****************/

/* RTCP and SIP INFO timer constants */

#define NSYS_RTCPTIME_INCR       5      /* 5 seconds between RTCP sends  */
#define NSYS_RTCPTIME_SKIP       2      /* threshold for retransmit skip */
#define NSYS_RTCPTIME_TIMEOUT   25      /* 25 second SSRC timeout        */
#define NSYS_RTCPTIME_MLENGTH  300      /* 5 minute limit for msession   */
#define NSYS_SIPINFO_TRIGGER     6      /* every 5 RTCP send a SIP INFO  */

/* RTCP-monitored exceptions */

#define NSYS_RTCPEX_RTPSIP       1     /* no response on SIP RTP channel */
#define NSYS_RTCPEX_RTPNEXT      2     /* skip RTP retransmission cycle  */
#define NSYS_RTCPEX_RTCPSIP      4     /* no response on SIP RTP channel */
#define NSYS_RTCPEX_RTCPNEXT     8     /* skip RTCP retransmission cycle */
#define NSYS_RTCPEX_NULLROOT     16    /* if nsys_srcroot == NULL        */
#define NSYS_RTCPEX_SRCDUPL      32    /* SSRC clash                     */

/********************/
/* rtp-midi payload */
/********************/

/*+++++++++++++++++++++++*/
/*  midi command payload */
/*+++++++++++++++++++++++*/

#define NSYS_SM_MLENMASK     15  /* minimal LEN field mask      */
#define NSYS_SM_EXPANDMAX  4095  /* worst case buffer expansion */

#define NSYS_SM_DTIME 128 /* & for continuing delta-time octet  */

#define NSYS_SM_SETB 128  /* | to set B header bit   */
#define NSYS_SM_CLRB 127  /* & to clear B header bit */
#define NSYS_SM_CHKB 128  /* & to check B header bit */

#define NSYS_SM_SETJ  64  /* | to set J header bit   */
#define NSYS_SM_CLRJ 191  /* & to clear J header bit */
#define NSYS_SM_CHKJ  64  /* & to check J header bit */

#define NSYS_SM_SETZ  32  /* | to set Z header bit   */
#define NSYS_SM_CLRZ 223  /* & to clear Z header bit */
#define NSYS_SM_CHKZ  32  /* & to check Z header bit */

#define NSYS_SM_SETP  16  /* | to set P header bit   */
#define NSYS_SM_CLRP 239  /* & to clear P header bit */
#define NSYS_SM_CHKP  16  /* & to check P header bit */

/*++++++++++++++++++++++++++++*/
/* recovery journal constants */
/*++++++++++++++++++++++++++++*/

#define NSYS_SM_SETS 128  /* | to set S header bit   */
#define NSYS_SM_CLRS 127  /* & to clear S header bit */
#define NSYS_SM_CHKS 128  /* & to check S header bit */

#define NSYS_SM_SETH 128  /* | to set H history bit   */
#define NSYS_SM_CLRH 127  /* & to clear H history bit */
#define NSYS_SM_CHKH 128  /* & to check H history bit */

#define NSYS_SM_SLISTLEN 128  /* maximum S list size */

#define NSYS_SM_SETHI 128    /* for MIDI data bytes */
#define NSYS_SM_CLRHI 127  
#define NSYS_SM_CHKHI 128  

/*+++++++++++++++++++++++++*/
/* recovery journal header */
/*+++++++++++++++++++++++++*/

#define NSYS_SM_JH_SIZE 3       /* journal header size */
#define NSYS_SM_JH_LOC_FLAGS 0  /* byte 0: flags       */
#define NSYS_SM_JH_LOC_CHECK 1  /* bytes 1-2: checkpoint packet */

#define NSYS_SM_JH_CHANMASK 0x0F  /* to mask channel number  */

#define NSYS_SM_JH_SETA  32  /* | to set A header bit   */
#define NSYS_SM_JH_CLRA 223  /* & to clear A header bit */
#define NSYS_SM_JH_CHKA  32  /* & to check A header bit   */

#define NSYS_SM_JH_SETY  64  /* | to set Y header bit   */
#define NSYS_SM_JH_CLRY 191  /* & to clear Y header bit */
#define NSYS_SM_JH_CHKY  64  /* & to check Y header bit   */

/*+++++++++++++++++++++++*/
/* system journal header */
/*+++++++++++++++++++++++*/

#define NSYS_SM_SH_SIZE       2  /* system journal header size */
#define NSYS_SM_SH_LOC_FLAGS  0  /* octet 0: flags             */
#define NSYS_SM_SH_LOC_LENLSB 1  /* octet 1: length lsb        */

#define NSYS_SM_SH_MSBMASK 0x03   /* to mask length msb         */
#define NSYS_SM_SH_LSBMASK 0x00FF /* to mask length lsb         */

#define NSYS_SM_SH_SETD  64      /* | to set D header bit      */
#define NSYS_SM_SH_CLRD 191      /* & to clear D header bit    */
#define NSYS_SM_SH_CHKD  64      /* & to check D header bit    */

#define NSYS_SM_SH_SETV  32      /* | to set V header bit      */
#define NSYS_SM_SH_CLRV 223      /* & to clear V header bit    */
#define NSYS_SM_SH_CHKV  32      /* & to check V header bit    */

#define NSYS_SM_SH_SETQ  16      /* | to set Q header bit      */
#define NSYS_SM_SH_CLRQ 239      /* & to clear Q header bit    */
#define NSYS_SM_SH_CHKQ  16      /* & to check Q header bit    */

#define NSYS_SM_SH_SETF   8      /* | to set F header bit      */
#define NSYS_SM_SH_CLRF 247      /* & to clear F header bit    */ 
#define NSYS_SM_SH_CHKF   8      /* & to check F header bit    */ 

#define NSYS_SM_SH_SETX   4      /* | to set X header bit      */
#define NSYS_SM_SH_CLRX 251      /* & to clear X header bit    */ 
#define NSYS_SM_SH_CHKX   4      /* & to check X header bit    */ 

/*++++++++++++++++*/
/* channel header */
/*++++++++++++++++*/

#define NSYS_SM_CH_SIZE 3       /* channel header size */

#define NSYS_SM_CH_LOC_FLAGS 0   /* location of S bit & channel */
#define NSYS_SM_CH_LOC_LEN   0   /* location of 10-bit length */
#define NSYS_SM_CH_LOC_TOC   2   /* location of table of contents */

#define NSYS_SM_CH_CHANMASK 0x78   /* to extract channel number  */
#define NSYS_SM_CH_LENMASK  0x03FF /* to extract channel length  */

#define NSYS_SM_CH_CHANSHIFT 3     /* to align channel number */

#define NSYS_SM_CH_TOC_SETP 128    /* Program Change (0xC) */
#define NSYS_SM_CH_TOC_CLRP 127    
#define NSYS_SM_CH_TOC_CHKP 128    

#define NSYS_SM_CH_TOC_SETC  64    /* Control Change (0xB) */
#define NSYS_SM_CH_TOC_CLRC 191    
#define NSYS_SM_CH_TOC_CHKC  64    

#define NSYS_SM_CH_TOC_SETM  32    /* Parameter System (part of 0xB) */
#define NSYS_SM_CH_TOC_CLRM 223    
#define NSYS_SM_CH_TOC_CHKM  32    

#define NSYS_SM_CH_TOC_SETW  16    /* Pitch Wheel (0xE) */
#define NSYS_SM_CH_TOC_CLRW 239    
#define NSYS_SM_CH_TOC_CHKW  16    

#define NSYS_SM_CH_TOC_SETN   8    /* NoteOff (0x8), NoteOn (0x9) */
#define NSYS_SM_CH_TOC_CLRN 247    
#define NSYS_SM_CH_TOC_CHKN   8    

#define NSYS_SM_CH_TOC_SETE   4    /* Note Command Extras (0x8, 0x9) */
#define NSYS_SM_CH_TOC_CLRE 251    
#define NSYS_SM_CH_TOC_CHKE   4    

#define NSYS_SM_CH_TOC_SETT   2    /* Channel Aftertouch (0xD) */
#define NSYS_SM_CH_TOC_CLRT 253    
#define NSYS_SM_CH_TOC_CHKT   2    

#define NSYS_SM_CH_TOC_SETA   1    /* Poly Aftertouch (0xA) */
#define NSYS_SM_CH_TOC_CLRA 254    
#define NSYS_SM_CH_TOC_CHKA   1

/*++++++++++++++++++++++++++++*/
/* chapter P (program change) */
/*++++++++++++++++++++++++++++*/

#define NSYS_SM_CP_SIZE 3               /* chapter size */

#define NSYS_SM_CP_LOC_PROGRAM 0       /* PROGRAM octet  */
#define NSYS_SM_CP_LOC_BANKMSB 1       /* BANK-MSB octet */
#define NSYS_SM_CP_LOC_BANKLSB 2       /* BANK-LSB octet */

#define NSYS_SM_CP_SETB 128             /* | to set B header bit   */
#define NSYS_SM_CP_CLRB 127             /* & to clear B header bit */
#define NSYS_SM_CP_CHKB 128             /* & to check B header bit */

#define NSYS_SM_CP_SETX 128             /* | to set X header bit   */
#define NSYS_SM_CP_CLRX 127             /* & to clear X header bit */
#define NSYS_SM_CP_CHKX 128             /* & to check X header bit */

/*++++++++++++++++++++++++++++*/
/* chapter C (control change) */
/*++++++++++++++++++++++++++++*/

#define NSYS_SM_CC_SIZE  257         /* maximum chapter size */

#define NSYS_SM_CC_LOC_LENGTH   0    /* LENGTH byte */
#define NSYS_SM_CC_LOC_LOGSTART 1    /* start of controller logs */ 

#define NSYS_SM_CC_HDRSIZE 1         /* size of chapter c header */  
#define NSYS_SM_CC_LOGSIZE 2         /* size of each controller log */  

#define NSYS_SM_CC_LOC_LNUM 0        /* CONTROLLER byte in each log  */
#define NSYS_SM_CC_LOC_LVAL 1        /* VALUE/COUNT byte in each log */

#define NSYS_SM_CC_ARRAYSIZE  128    /* size of controller state arrays */

#define NSYS_SM_CC_SETA 128          /* | to set A log bit   */
#define NSYS_SM_CC_CLRA 127          /* & to clear A log bit */
#define NSYS_SM_CC_CHKA 128          /* & to check A log bit */

#define NSYS_SM_CC_SETT  64          /* | to set T log bit   */
#define NSYS_SM_CC_CLRT 191          /* & to clear T log bit */
#define NSYS_SM_CC_CHKT  64          /* & to check T log bit */

#define NSYS_SM_CC_ALTMOD  0x3F       /* for updating ALT state */

/*++++++++++++++++++++++++++++++*/
/* chapter M (parameter change) */
/*++++++++++++++++++++++++++++++*/

/* chapter M: header */

#define NSYS_SM_CM_LOC_HDR     0    /* header location        */
#define NSYS_SM_CM_LOC_PENDING 2    /* pending field          */

#define NSYS_SM_CM_HDRSIZE     2   /* Chapter M header size    */  
#define NSYS_SM_CM_PENDINGSIZE 1   /* PENDING field size    */  

#define NSYS_SM_CM_LENMASK 0x03FF  /* LENGTH header field mask */  

#define NSYS_SM_CM_HDR_SETP  64    /* Pending header bit        */
#define NSYS_SM_CM_HDR_CLRP 191    
#define NSYS_SM_CM_HDR_CHKP  64    

#define NSYS_SM_CM_HDR_SETE  32    /* progress header bit      */
#define NSYS_SM_CM_HDR_CLRE 223    
#define NSYS_SM_CM_HDR_CHKE  32    

#define NSYS_SM_CM_HDR_SETU     16    /* ALL-RPN bit  */
#define NSYS_SM_CM_HDR_CLRU    239    
#define NSYS_SM_CM_HDR_CHKU     16    

#define NSYS_SM_CM_HDR_SETW      8    /* ALL-NRPN bit */
#define NSYS_SM_CM_HDR_CLRW    247    
#define NSYS_SM_CM_HDR_CHKW      8    

#define NSYS_SM_CM_HDR_SETZ      4    /* PNUM-MSB == 0x00 bit*/
#define NSYS_SM_CM_HDR_CLRZ    251    
#define NSYS_SM_CM_HDR_CHKZ      4    

#define NSYS_SM_CM_PENDING_SETQ 128  /* RPN/NRPN flag for PENDING */  
#define NSYS_SM_CM_PENDING_CLRQ 127    
#define NSYS_SM_CM_PENDING_CHKQ 128    

#define NSYS_SM_CM_LOC_LIST 0      /* list location in chapterl[] */

/* Chapter M: parameter log */

#define NSYS_SM_CM_LOGMAXSIZE   10  /* maximum size of a log    */

#define NSYS_SM_CM_LOC_PNUMLSB   0  
#define NSYS_SM_CM_LOC_PNUMMSB   1
#define NSYS_SM_CM_LOC_TOC       2
#define NSYS_SM_CM_LOC_INFOBITS  2

#define NSYS_SM_CM_LOGHDRSIZE    3 /* parameter log header size */

#define NSYS_SM_CM_SETQ        128     /* | to set Q log bit   */
#define NSYS_SM_CM_CLRQ        127     /* & to clear Q log bit */
#define NSYS_SM_CM_CHKQ        128     /* & to check Q log bit */

#define NSYS_SM_CM_SETX        128     /* | to set X log bit   */
#define NSYS_SM_CM_CLRX        127     /* & to clear X log bit */
#define NSYS_SM_CM_CHKX        128     /* & to check X log bit */

#define NSYS_SM_CM_BUTTON_SETG  128    /* | to set G log bit   */
#define NSYS_SM_CM_BUTTON_CLRG  127    /* & to clear G log bit */
#define NSYS_SM_CM_BUTTON_CHKG  128    /* & to check G log bit */

#define NSYS_SM_CM_BUTTON_SETX  64     /* | to set X log bit   */
#define NSYS_SM_CM_BUTTON_CLRX  191    /* & to clear X log bit */
#define NSYS_SM_CM_BUTTON_CHKX  64     /* & to check X log bit */

#define NSYS_SM_CM_BUTTON_SETR  64     /* | to set R log bit   */
#define NSYS_SM_CM_BUTTON_CLRR  191    /* & to clear R log bit */
#define NSYS_SM_CM_BUTTON_CHKR  64     /* & to check R log bit */

#define NSYS_SM_CM_COUNT_SIZE    1

#define NSYS_SM_CM_ENTRYMSB_SIZE 1
#define NSYS_SM_CM_ENTRYLSB_SIZE 1

#define NSYS_SM_CM_ABUTTON_SIZE  2
#define NSYS_SM_CM_CBUTTON_SIZE  2

#define NSYS_SM_CM_BUTTON_LIMIT  0x3FFF   /* 16383 decimal */

#define NSYS_SM_CM_TOC_SETJ    128    /* ENTRY-MSB field */
#define NSYS_SM_CM_TOC_CLRJ    127    
#define NSYS_SM_CM_TOC_CHKJ    128    

#define NSYS_SM_CM_TOC_SETK     64    /* ENTRY-LSB field */
#define NSYS_SM_CM_TOC_CLRK    191    
#define NSYS_SM_CM_TOC_CHKK     64    

#define NSYS_SM_CM_TOC_SETL     32    /* A-BUTTON field */
#define NSYS_SM_CM_TOC_CLRL    223    
#define NSYS_SM_CM_TOC_CHKL     32    

#define NSYS_SM_CM_TOC_SETM     16    /* C-BUTTON field */
#define NSYS_SM_CM_TOC_CLRM    239    
#define NSYS_SM_CM_TOC_CHKM     16    

#define NSYS_SM_CM_TOC_SETN      8    /* COUNT field */
#define NSYS_SM_CM_TOC_CLRN    247    
#define NSYS_SM_CM_TOC_CHKN      8    

#define NSYS_SM_CM_INFO_SETT      4    /* Count-mode*/
#define NSYS_SM_CM_INFO_CLRT    251    
#define NSYS_SM_CM_INFO_CHKT      4    

#define NSYS_SM_CM_INFO_SETV      2    /* Value-mode */
#define NSYS_SM_CM_INFO_CLRV    253    
#define NSYS_SM_CM_INFO_CHKV      2    

#define NSYS_SM_CM_INFO_SETA      1    /* active-mode */
#define NSYS_SM_CM_INFO_CLRA    254    
#define NSYS_SM_CM_INFO_CHKA      1

/* Chapter M: implementation-specific */

/* state machine */

#define NSYS_SM_CM_STATE_OFF           0 
#define NSYS_SM_CM_STATE_PENDING_RPN   1
#define NSYS_SM_CM_STATE_PENDING_NRPN  2
#define NSYS_SM_CM_STATE_RPN           3
#define NSYS_SM_CM_STATE_NRPN          4

/* transaction constants */

#define NSYS_SM_CM_TRANS_NONE          0
#define NSYS_SM_CM_TRANS_NRPN          1
#define NSYS_SM_CM_TRANS_RPN           2  /* transaction type         */
#define NSYS_SM_CM_TRANS_NO_OPEN       4  /* do not open transaction  */
#define NSYS_SM_CM_TRANS_NO_SET        8  /* do not set values        */
#define NSYS_SM_CM_TRANS_NO_CLOSE     16  /* do not close transaction */

/* size restrictions */

#define NSYS_SM_CM_HDRMAXSIZE          3  /* 2 octets w/o pending        */
#define NSYS_SM_CM_LISTMAXSIZE        30  /* 3 10-octet logs             */

#define NSYS_SM_CM_ARRAYSIZE  3 /* 3 RPNs:                     */
                                /* MSB=00 LSB=00 Pitch Bend    */
                                /* MSB=00 LSB=01 Fine Tuning   */
                                /* MSB=00 LSB=02 Coarse Tuning */

/*+++++++++++++++++++++++++*/
/* chapter W (pitch wheel) */
/*+++++++++++++++++++++++++*/

#define NSYS_SM_CW_SIZE 2           /* chapter size */

#define NSYS_SM_CW_LOC_FIRST 0      /* FIRST  byte */
#define NSYS_SM_CW_LOC_SECOND 1     /* SECOND byte */

#define NSYS_SM_CW_SETD 128         /* | to set D header bit   */
#define NSYS_SM_CW_CLRD 127         /* & to clear D header bit */
#define NSYS_SM_CW_CHKD 128         /* & to check D header bit */

/*+++++++++++++++++++*/
/* chapter N (notes) */
/*+++++++++++++++++++*/

#define NSYS_SM_CN_SIZE  258         /* maximum chapter size (front) */
#define NSYS_SM_CB_SIZE   16         /* maximum bitfield size  */

#define NSYS_SM_CN_LOC_LENGTH   0    /* LENGTH  byte */
#define NSYS_SM_CN_LOC_LOWHIGH  1    /* LOWHIGH byte */
#define NSYS_SM_CN_LOC_LOGSTART 2    /* start of note logs */ 

#define NSYS_SM_CN_LOWMASK   0xF0    /* mask for LOW  */
#define NSYS_SM_CN_HIGHMASK  0x0F    /* mask for HIGH */
#define NSYS_SM_CN_LOWSHIFT     4    /* shift align for LOW */

#define NSYS_SM_CN_HDRSIZE 2         /* size of chapter n header */  
#define NSYS_SM_CN_LOGSIZE 2         /* size of each note log    */  

#define NSYS_SM_CN_LOC_NUM 0         /* note number byte in each log  */
#define NSYS_SM_CN_LOC_VEL 1         /* velocity byte in each log */

#define NSYS_SM_CN_SETB 128          /* | to set B header bit   */
#define NSYS_SM_CN_CLRB 127          /* & to clear B header bit */
#define NSYS_SM_CN_CHKB 128          /* & to check B header bit */

#define NSYS_SM_CN_SETY 128          /* | to set Y header bit   */
#define NSYS_SM_CN_CLRY 127          /* & to clear Y header bit */
#define NSYS_SM_CN_CHKY 128          /* & to check Y header bit */

#define NSYS_SM_CN_ARRAYSIZE  128    /* size of note state arrays */

#define NSYS_SM_CN_BFMIN        0    /* minimum bitfield value    */
#define NSYS_SM_CN_BFMAX       15    /* maximum bitfield value    */
#define NSYS_SM_CN_BFSHIFT      3    /* note to bitfield byte shift */
#define NSYS_SM_CN_BFMASK    0x07    /* mask for bit position      */

#define NSYS_SM_CN_MAXDELAY  0.020F   /* lifetime of Y bit (seconds)  */
#define NSYS_SM_CN_RECDELAY  0.10F    /* receiver test for Y lifetime */

/*+++++++++++++++++++++++++*/
/* chapter E (note extras) */
/*+++++++++++++++++++++++++*/

#define NSYS_SM_CE_SIZE    257        /* maximum size: header + ref logs */

#define NSYS_SM_CE_LOC_LENGTH   0    /* LENGTH byte */
#define NSYS_SM_CE_LOC_LOGSTART 1    /* start of note logs */ 

#define NSYS_SM_CE_HDRSIZE 1         /* size of chapter E header */  
#define NSYS_SM_CE_LOGSIZE 2         /* size of each note log */  

#define NSYS_SM_CE_LOC_NUM 0         /* NOTENUM byte in each log  */
#define NSYS_SM_CE_LOC_COUNTVEL 1    /* COUNT/VEL byte in each log */

#define NSYS_SM_CE_SETV 128          /* | to set V header bit   */
#define NSYS_SM_CE_CLRV 127          /* & to clear V header bit */
#define NSYS_SM_CE_CHKV 128          /* & to check V header bit */

#define NSYS_SM_CE_ARRAYSIZE  128    /* size of note extra state arrays */
#define NSYS_SM_CE_DEFREL      64    /* default release velocity value */
#define NSYS_SM_CE_MAXCOUNT   127    /* maximum reference count */

/*++++++++++++++++++++++++++++++++*/
/* chapter T (channel aftertouch) */
/*++++++++++++++++++++++++++++++++*/

#define NSYS_SM_CT_SIZE 1            /* chapter size */

#define NSYS_SM_CT_LOC_PRESSURE 0    /* PRESSURE byte */

/*++++++++++++++++++++++++*/
/* chapter A (poly touch) */
/*++++++++++++++++++++++++*/

#define NSYS_SM_CA_SIZE  257         /* maximum chapter size */

#define NSYS_SM_CA_LOC_LENGTH   0    /* LENGTH byte */
#define NSYS_SM_CA_LOC_LOGSTART 1    /* start of note logs */ 

#define NSYS_SM_CA_HDRSIZE 1         /* size of chapter A header */  
#define NSYS_SM_CA_LOGSIZE 2         /* size of each note log */  

#define NSYS_SM_CA_LOC_NUM 0         /* NOTENUM byte in each log  */
#define NSYS_SM_CA_LOC_PRESSURE 1    /* PRESSURE byte in each log */

#define NSYS_SM_CA_SETX 128          /* | to set X header bit   */
#define NSYS_SM_CA_CLRX 127          /* & to clear X header bit */
#define NSYS_SM_CA_CHKX 128          /* & to check X header bit */

#define NSYS_SM_CA_ARRAYSIZE  128    /* size of ptouch state arrays */

/*+++++++++++++++++++++++++*/
/* system journal chapters */
/*+++++++++++++++++++++++++*/

/*+++++++++++++++++++++++++++*/
/* chapter D (miscellaneous) */
/*+++++++++++++++++++++++++++*/

#define NSYS_SM_CD_LOC_TOC      0    /* Table of Contents     */
#define NSYS_SM_CD_LOC_LOGS     1    /* start of command logs */ 

#define NSYS_SM_CD_SIZE_TOC      1    /* TOC header size  */  
#define NSYS_SM_CD_SIZE_RESET    1    /* Reset field size */
#define NSYS_SM_CD_SIZE_TUNE     1    /* Tune Request field size */  
#define NSYS_SM_CD_SIZE_SONG     1    /* Song Select field size */  

#define NSYS_SM_CD_FRONT_MAXSIZE  4   /* maximum size (J = K = Y = Z = 0) */

#define NSYS_SM_CD_TOC_SETB     64    /* Reset field */
#define NSYS_SM_CD_TOC_CLRB    191    
#define NSYS_SM_CD_TOC_CHKB     64    

#define NSYS_SM_CD_TOC_SETG     32    /* Tune Request field */
#define NSYS_SM_CD_TOC_CLRG    223    
#define NSYS_SM_CD_TOC_CHKG     32    

#define NSYS_SM_CD_TOC_SETH     16    /* Song Select field */
#define NSYS_SM_CD_TOC_CLRH    239    
#define NSYS_SM_CD_TOC_CHKH     16    

#define NSYS_SM_CD_TOC_SETJ      8    /* Common 0xF4 field */
#define NSYS_SM_CD_TOC_CLRJ    247    
#define NSYS_SM_CD_TOC_CHKJ      8    

#define NSYS_SM_CD_TOC_SETK      4    /* Common 0xF5 field */
#define NSYS_SM_CD_TOC_CLRK    251    
#define NSYS_SM_CD_TOC_CHKK      4    

#define NSYS_SM_CD_TOC_SETY      2    /* Real-time 0xF9 field */
#define NSYS_SM_CD_TOC_CLRY    253    
#define NSYS_SM_CD_TOC_CHKY      2    

#define NSYS_SM_CD_TOC_SETZ      1    /* Real-time 0xFD field */
#define NSYS_SM_CD_TOC_CLRZ    254    
#define NSYS_SM_CD_TOC_CHKZ      1

/*  undefined System Common log */

#define NSYS_SM_CD_COMMON_LOC_TOC     0   /* Table of Contents  */ 
#define NSYS_SM_CD_COMMON_LOC_LENGTH  1   /* Log length         */  
#define NSYS_SM_CD_COMMON_LOC_FIELDS  2   /* Start of fields    */  

#define NSYS_SM_CD_COMMON_TOC_SIZE    1    /* TOC size         */  
#define NSYS_SM_CD_COMMON_LENGTH_SIZE 1    /* Log Length Size  */
#define NSYS_SM_CD_COMMON_COUNT_SIZE  1    /* Count Field Size */

#define NSYS_SM_CD_COMMON_TOC_SETC     64    /* Count field */
#define NSYS_SM_CD_COMMON_TOC_CLRC    191    
#define NSYS_SM_CD_COMMON_TOC_CHKC     64    

#define NSYS_SM_CD_COMMON_TOC_SETV     32    /* Value field */
#define NSYS_SM_CD_COMMON_TOC_CLRV    223    
#define NSYS_SM_CD_COMMON_TOC_CHKV     32    

#define NSYS_SM_CD_COMMON_TOC_SETL     16    /* Legal field */
#define NSYS_SM_CD_COMMON_TOC_CLRL    239    
#define NSYS_SM_CD_COMMON_TOC_CHKL     16    

#define NSYS_SM_CD_COMMON_DSZ_MASK   0x03    /* bit-mask for DSZ field */

#define NSYS_SM_CD_COMMON_SIZE          5      /* implementation-specific */

/*  undefined System Real-Time log */

#define NSYS_SM_CD_REALTIME_LOC_TOC     0   /* Table of Contents  */ 
#define NSYS_SM_CD_REALTIME_LOC_FIELDS  1   /* Start of fields    */  

#define NSYS_SM_CD_REALTIME_TOC_SIZE    1   /* TOC size          */  
#define NSYS_SM_CD_REALTIME_COUNT_SIZE  1   /* Count field       */

#define NSYS_SM_CD_REALTIME_TOC_SETC     64    /* Count field */
#define NSYS_SM_CD_REALTIME_TOC_CLRC    191    
#define NSYS_SM_CD_REALTIME_TOC_CHKC     64    

#define NSYS_SM_CD_REALTIME_TOC_SETL     32    /* Legal field */
#define NSYS_SM_CD_REALTIME_TOC_CLRL    223    
#define NSYS_SM_CD_REALTIME_TOC_CHKL     32    

#define NSYS_SM_CD_REALTIME_LENGTH_MASK  0x1F  /* LENGTH bit-mask */
#define NSYS_SM_CD_REALTIME_SIZE         2     /* implementation-specific  */ 

/*+++++++++++++++++++++++++++*/
/* chapter V (active sense)  */
/*+++++++++++++++++++++++++++*/

#define NSYS_SM_CV_SIZE       1   /* chapter size         */
#define NSYS_SM_CV_LOC_COUNT  0   /* COUNT field location */  

/*+++++++++++++++++++++++++++*/
/* chapter Q (sequencer)     */
/*+++++++++++++++++++++++++++*/

#define NSYS_SM_CQ_LOC_HDR      0    /* chapter header            */
#define NSYS_SM_CQ_LOC_FIELDS   1    /* start of sequencer fields */ 

#define NSYS_SM_CQ_SIZE_HDR        1    /* chapter header size  */  
#define NSYS_SM_CQ_SIZE_CLOCK      2    /* CLOCK field size     */
#define NSYS_SM_CQ_SIZE_TIMETOOLS  3    /* TIMETOOLS field size */  

#define NSYS_SM_CQ_MAXSIZE         3    /* maximum chapter size (if T = 0) */

#define NSYS_SM_CQ_HDR_SETN     64     /* Start/Stop bit */
#define NSYS_SM_CQ_HDR_CLRN    191    
#define NSYS_SM_CQ_HDR_CHKN     64    

#define NSYS_SM_CQ_HDR_SETD     32     /* Download field */
#define NSYS_SM_CQ_HDR_CLRD    223    
#define NSYS_SM_CQ_HDR_CHKD     32    

#define NSYS_SM_CQ_HDR_SETC     16     /* CLOCK TOC bit */
#define NSYS_SM_CQ_HDR_CLRC    239    
#define NSYS_SM_CQ_HDR_CHKC     16    

#define NSYS_SM_CQ_HDR_SETT      8     /* TIMETOOLS TOC bit */
#define NSYS_SM_CQ_HDR_CLRT    247    
#define NSYS_SM_CQ_HDR_CHKT      8    

#define NSYS_SM_CQ_TOP_MASK      0x07        /* TOP field bit-mask       */
#define NSYS_SM_CQ_BOTTOM_MASK   0x0000FFFF  /* bottom 16-bits for CLOCK */

#define NSYS_SM_CQ_TIMETOOLS_SETB 128    /* TIMETOOLS B bit */
#define NSYS_SM_CQ_TIMETOOLS_CLRB 127  
#define NSYS_SM_CQ_TIMETOOLS_CHKB 128  

/*++++++++++++++++++++++++++++*/
/* chapter F (MIDI Time Code) */
/*++++++++++++++++++++++++++++*/

#define NSYS_SM_CF_LOC_HDR         0    /* chapter header           */
#define NSYS_SM_CF_LOC_FIELDS      1    /* start of timecode fields */ 

#define NSYS_SM_CF_SIZE_HDR        1    /* chapter header size      */  
#define NSYS_SM_CF_SIZE_COMPLETE   4    /* COMPLETE timestamp       */
#define NSYS_SM_CF_SIZE_PARTIAL    4    /* PARTIAL timestamp        */  
#define NSYS_SM_CF_MAXSIZE         9    /* maximum chapter size     */  

#define NSYS_SM_CF_HDR_SETC     64     /* COMPLETE field TOC bit */
#define NSYS_SM_CF_HDR_CLRC    191    
#define NSYS_SM_CF_HDR_CHKC     64    

#define NSYS_SM_CF_HDR_SETP     32     /* PARTIAL field TOC bit */    
#define NSYS_SM_CF_HDR_CLRP    223    
#define NSYS_SM_CF_HDR_CHKP     32    

#define NSYS_SM_CF_HDR_SETQ     16    /* Quarter-Frame status bit */ 
#define NSYS_SM_CF_HDR_CLRQ    239    
#define NSYS_SM_CF_HDR_CHKQ     16    

#define NSYS_SM_CF_HDR_SETD      8     /* tape direction status bit */
#define NSYS_SM_CF_HDR_CLRD    247    
#define NSYS_SM_CF_HDR_CHKD      8    

#define NSYS_SM_CF_POINT_MASK   0x07    /* mask POINT field bit-mask  */
#define NSYS_SM_CF_POINT_CLR    0xF8    /* clear POINT field bit-mask */

#define NSYS_SM_CF_SIZE_MT         1    /* message-type size     */  
#define NSYS_SM_CF_SIZE_HR         1    /* hours FF size         */  
#define NSYS_SM_CF_SIZE_MN         1    /* minutes FF size       */  
#define NSYS_SM_CF_SIZE_SC         1    /* seconds FF size       */  
#define NSYS_SM_CF_SIZE_FR         1    /* frames FF size        */  

#define NSYS_SM_CF_FF_LOC_HR       0    /* hours FF location     */
#define NSYS_SM_CF_FF_LOC_MN       1    /* minutes FF location   */
#define NSYS_SM_CF_FF_LOC_SC       2    /* seconds FF location   */
#define NSYS_SM_CF_FF_LOC_FR       3    /* frames FF location    */

#define NSYS_SM_CF_QF_LOC_FR_LSN   0   /* Frame LSN location     */
#define NSYS_SM_CF_QF_LOC_FR_MSN   0   /* Frame MSN location     */
#define NSYS_SM_CF_QF_LOC_SC_LSN   1   /* Seconds LSN location   */
#define NSYS_SM_CF_QF_LOC_SC_MSN   1   /* Seconds MSN location   */
#define NSYS_SM_CF_QF_LOC_MN_LSN   2   /* Minutes LSN location   */
#define NSYS_SM_CF_QF_LOC_MN_MSN   2   /* Minutes MSN location   */
#define NSYS_SM_CF_QF_LOC_HR_LSN   3   /* Hours LSN location     */
#define NSYS_SM_CF_QF_LOC_HR_MSN   3   /* Hours MSN location     */

#define NSYS_SM_CF_IDNUM_MASK    0x70   /* MT ID # bit-mask      */
#define NSYS_SM_CF_PAYLOAD_MASK  0x0F   /* MT PAYLOAD # bit-mask */

#define NSYS_SM_CF_EVEN_MASK  0xF0      /* mask even MTs */
#define NSYS_SM_CF_EVEN_CLR   0x0F      /* clear even MTs */

#define NSYS_SM_CF_ODD_MASK   0x0F      /* mask odd MTs  */
#define NSYS_SM_CF_ODD_CLR    0xF0      /* clear odd MTs */

#define NSYS_SM_CF_IDNUM_FR_LSN  0    /* Frame LSN ID Number     */
#define NSYS_SM_CF_IDNUM_FR_MSN  1    /* Frame MSN ID Number     */
#define NSYS_SM_CF_IDNUM_SC_LSN  2    /* Seconds LSN ID Number   */
#define NSYS_SM_CF_IDNUM_SC_MSN  3    /* Seconds MSN ID Number   */
#define NSYS_SM_CF_IDNUM_MN_LSN  4    /* Minutes LSN ID Number   */
#define NSYS_SM_CF_IDNUM_MN_MSN  5    /* Minutes MSN ID Number   */
#define NSYS_SM_CF_IDNUM_HR_LSN  6    /* Hours LSN ID Number     */
#define NSYS_SM_CF_IDNUM_HR_MSN  7    /* Hours MSN ID Number     */

/*++++++++++++++++++++++++++++++*/
/* chapter X (System Exclusive) */
/*++++++++++++++++++++++++++++++*/

#define NSYS_SM_CX_LOC_HDR       0    /* chapter header      */
#define NSYS_SM_CX_LOC_FIELDS    1    /* TCOUNT, COUNT, FIRST, DATA */ 

#define NSYS_SM_CX_SIZE_HDR      1    /* chapter header  */
#define NSYS_SM_CX_SIZE_TCOUNT   1    /* TCOUNT field    */
#define NSYS_SM_CX_SIZE_COUNT    1    /* COUNT field     */

#define NSYS_SM_CX_HDR_SETT     64    /* TCOUNT field TOC bit */
#define NSYS_SM_CX_HDR_CLRT    191    
#define NSYS_SM_CX_HDR_CHKT     64    

#define NSYS_SM_CX_HDR_SETC     32    /* COUNT field TOC bit */
#define NSYS_SM_CX_HDR_CLRC    223    
#define NSYS_SM_CX_HDR_CHKC     32    

#define NSYS_SM_CX_HDR_SETF     16    /* FIRST field TOC bit */
#define NSYS_SM_CX_HDR_CLRF    239    
#define NSYS_SM_CX_HDR_CHKF     16    

#define NSYS_SM_CX_HDR_SETD      8    /* DATA field TOC bit */
#define NSYS_SM_CX_HDR_CLRD    247    
#define NSYS_SM_CX_HDR_CHKD      8    

#define NSYS_SM_CX_HDR_SETL      4    /* list/recency bit */
#define NSYS_SM_CX_HDR_CLRL    251    
#define NSYS_SM_CX_HDR_CHKL      4    

#define NSYS_SM_CX_STA_MASK   0x03    /* STA field bit-mask  */

#define NSYS_SM_CX_STA_UNFINISHED  0x00    /* unfinished SysEx      */
#define NSYS_SM_CX_STA_CANCELLED   0x01    /* cancelled SysEx       */
#define NSYS_SM_CX_STA_DROPPEDF7   0x02    /* dropped-F7 SysEx      */
#define NSYS_SM_CX_STA_NORMAL      0x03    /* normal finished SysEx */

#define NSYS_SM_CX_DATA_SETEND  0x80
#define NSYS_SM_CX_DATA_CHKEND  0x80
#define NSYS_SM_CX_DATA_CLREND  0x7F
#define NSYS_SM_CX_DATA_MASK    0x7F

#define NSYS_SM_CX_GMRESET_ONVAL   0x01    /* GM Reset on ndata value */
#define NSYS_SM_CX_GMRESET_OFFVAL  0x02    /* GM Reset on ndata value */

/* implementation specific */

#define NSYS_SM_CX_SIZE_GMRESET    6    /* log for GM Resets */
#define NSYS_SM_CX_SIZE_MVOLUME    8    /* log for Master Volume */
#define NSYS_SM_CX_SIZE_MAXLOG     8    /* maximum log size */

#define NSYS_SM_CX_MAXSLOTS        3    /* at most, 3 logs in Chapter X */
    
/*++++++++++++++++++++*/
/* rtp-midi: receiver */
/*++++++++++++++++++++*/

/* number of MIDI channels to pre-allocate per receiver */

#define NSYS_RECVCHAN  2

/* constants to handle flag bit */

#define NSYS_SM_RV_SETF 128        /* | to set flag bit   */
#define NSYS_SM_RV_CLRF 127        /* & to clear flag bit */
#define NSYS_SM_RV_CHKF 128        /* & to check flag bit */

/* return values for nsys_netin_journal_recovery() */

#define NSYS_JOURNAL_RECOVERED  0   /* must be zero */
#define NSYS_JOURNAL_FILLEDBUFF 1
#define NSYS_JOURNAL_CORRUPTED  2

/*++++++++++++++++++*/
/* rtp-midi: sender */
/*++++++++++++++++++*/

#define NSYS_SM_GUARD_ONTIME     NSYS_SM_CN_MAXDELAY
#define NSYS_SM_GUARD_MINTIME    0.25F
#define NSYS_SM_GUARD_STDTIME    0.10F
#define NSYS_SM_GUARD_MAXTIME    1.0F

#ifndef NSYS_NET
#define NSYS_SM_FEC_NONE     0
#define NSYS_SM_FEC_NOGUARD  1
#define NSYS_SM_FEC_MINIMAL  2  
#define NSYS_SM_FEC_STANDARD 3  
#define NSYS_SM_FEC_EXTRA    4  
#endif

#define NSYS_SM_LATETIME        0.040F     /* maximum lateness */
#define NSYS_SM_LATEWINDOW        3.5F     /* time window for reset */

/*******/
/* SIP */
/*******/

#ifndef NSYS_SIP_IP
#define NSYS_SIP_IP          "169.229.59.210"
#endif

#ifndef NSYS_SIP_RTP_PORT
#define NSYS_SIP_RTP_PORT    5060
#endif

#ifndef NSYS_SIP_RTCP_PORT
#define NSYS_SIP_RTCP_PORT   5061
#endif

#define NSYS_HOSTNAMESIZE 256
#define NSYS_SDPNATSIZE   128
#define NSYS_CNAMESIZE     32

#define NSYS_NETIN_SIPSTATE 0   /* SIP INVITE state machine states */
#define NSYS_NETIN_SDPSTATE 1
#define NSYS_NETIN_EOFSTATE 2
#define NSYS_NETIN_ERRSTATE 3

#define NSYS_SIP_RETRYMAX       5         /* max retry for initial connect */
#define NSYS_SIP_AUTHRETRYMAX   4         /* max retry for authentication  */

#define NSYS_SIP_UNIXTONTP      2208988800UL /* add to UNIX time for NTP (s) */
 
/********/
/* SDP  */
/********/

/* number of supported payload types */

#define NSYS_RTP_PAYSIZE 1

/* pindex values for payload types */

#define NSYS_MPEG4_PINDEX  0
#define NSYS_NATIVE_PINDEX 1

/*******************/
/* Crypto/Security */
/*******************/

#define NSYS_MD5_LENGTH     16   /* array size for an MD5 digest in binary */
#define NSYS_MD5_ROUNDS  10000   /* number of MD5 passes for keydigest  */

#define NSYS_BASE64_LENGTH  25   /* string size of a BASE64 MD5 digest     */
#define NSYS_MKEY_LENGTH    32   /* length of random key for mirror        */

#define NSYS_MAXLATETIME     5400   /* clocks no more than 90 minutes fast */
#define NSYS_MAXSSESIONTIME 21600   /* session maxtime is 6 hours          */

#define NSYS_MSESSION_NAME "mirror"  /* change if sfront hashes names  */

#ifndef NSYS_MSESSION_INTERVAL
#define NSYS_MSESSION_INTERVAL  12   /* pitch shift for mirror         */
#endif

#define NSYS_MSESSION_MAXRTP  9000   /* maximum RTP packets in session  */
#define NSYS_MSESSION_MAXRTCP  200   /* maximum RTCP packets in session */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                           Variable externs                             */
/*________________________________________________________________________*/


/*************************/
/* rtp and rtcp: network */
/*************************/

extern int nsys_rtp_fd;               /* fd for rtp           */
extern int nsys_rtcp_fd;              /* fd for rtcp          */
extern int nsys_max_fd;               /* fd for select        */

extern unsigned short nsys_rtp_port;  /* actual rtp port      */ 
extern unsigned short nsys_rtcp_port; /* actual rtcp port     */ 

extern unsigned long nsys_rtp_cseq;   /* rtp cseq number      */
extern unsigned long nsys_rtcp_cseq;  /* rtcp cseq number     */

/*************************/
/* rtp and rtcp: packets */
/*************************/

extern unsigned char nsys_netout_rtp_packet[];  /* rtp packet out */

/* rtcp packets and lengths */

extern unsigned char * nsys_netout_rtcp_packet_bye;
extern unsigned char * nsys_netout_rtcp_packet_rrempty;
extern unsigned char * nsys_netout_rtcp_packet_rr;
extern unsigned char * nsys_netout_rtcp_packet_srempty;
extern unsigned char * nsys_netout_rtcp_packet_sr;

extern int nsys_netout_rtcp_len_bye;
extern int nsys_netout_rtcp_len_rrempty;
extern int nsys_netout_rtcp_len_rr;
extern int nsys_netout_rtcp_len_srempty;
extern int nsys_netout_rtcp_len_sr;

extern char * nsys_sdes_typename[];            /* rtcp debug array */

extern unsigned long nsys_netout_seqnum;  /* rtp output sequence number */
extern unsigned long nsys_netout_tstamp;  /* rtp output timestamp */
extern unsigned char nsys_netout_markbit; /* for setting markerbit */

/************************/
/* rtp payload support  */
/************************/

typedef struct nsys_payinfo {

  int pindex;               /* index into nsys_payload_types  */
  unsigned char ptype;      /* RTP payload number */
  char name[32];            /* name of payload    */
  int srate;                /* audio sample rate  */

} nsys_payinfo;

extern struct nsys_payinfo nsys_payload_types[];

/**************/
/* rtcp state */
/**************/

extern int nsys_sent_last;    /* a packet was sent last RTCP period     */
extern int nsys_sent_this;    /* a packet was sent this RTCP period     */

extern unsigned long nsys_sent_packets;  /* number of packets sent */
extern unsigned long nsys_sent_octets;   /* number of octets sent */

extern time_t nsys_nexttime;  /* time for next RTCP check               */
extern int nsys_rtcp_ex;      /* status flags to check at RTCP time     */

/******************/
/* identification */
/******************/

extern unsigned long nsys_myssrc;     /*  SSRC -- hostorder  */
extern unsigned long nsys_myssrc_net; /*  SSRC -- netorder   */

extern char * nsys_sessionname;
extern char * nsys_sessionkey;

extern unsigned char nsys_keydigest[];        /* digested key */
extern unsigned char nsys_session_base64[];   /* signed session name */
extern int nsys_msession;                     /* mirror session flag */
extern int nsys_msessionmirror;               /* mirror in mirror session */

extern int nsys_feclevel;
extern int nsys_lateplay;
extern float nsys_latetime;

extern char nsys_clientname[];
extern char nsys_clientip[];
extern char * nsys_username;

extern char nsys_cname[];
extern unsigned char nsys_cname_len;

extern int nsys_powerup_mset; 

/***********/
/* logging */
/***********/

extern int nsys_stderr_size;

/*********************/
/* MIDI input buffer */
/*********************/

extern unsigned char nsys_buff[];
extern long nsys_bufflen;
extern long nsys_buffcnt;

/*~~~~~~~~~~~~~~~~~~~~~~~~*/
/* rtp-midi: sender state */
/*________________________*/

/*************************/
/* command section flags */
/*************************/

extern unsigned char nsys_netout_sm_header;

/*************************/
/* checkpoint management */
/*************************/

extern unsigned long nsys_netout_jsend_checkpoint_seqnum;  /* current checkpoint */
extern unsigned long nsys_netout_jsend_checkpoint_changed; /* 1 if changed */

/*********************/
/* S-list management */
/*********************/

extern unsigned char * nsys_netout_jsend_slist[];
extern int nsys_netout_jsend_slist_size;

/*******************************/
/* guard journal state machine */
/*******************************/

extern int nsys_netout_jsend_guard_send;    /* flag variable: send guard packet */
extern int nsys_netout_jsend_guard_time;    /* guard packet timer state */
extern int nsys_netout_jsend_guard_next;    /* reload value for timer */
extern int nsys_netout_jsend_guard_ontime;  /* minimum delay time for noteon */ 
extern int nsys_netout_jsend_guard_mintime; /* minimum delay time for (!noteon) */
extern int nsys_netout_jsend_guard_maxtime; /* maximum delay time */ 

/***************************/
/* recovery journal header */
/***************************/

extern unsigned char nsys_netout_jsend_header[];    /* journal header */

/**************************/
/* channel journal record */
/**************************/

extern unsigned char nsys_netout_jsend_channel[];
extern unsigned char nsys_netout_jsend_channel_size;

/******************************************/
/* structure holding sender channel state */
/******************************************/

typedef struct nsys_netout_jsend_state {

  unsigned char  chan;  /* MIDI channel */
  unsigned short clen;  /* first two bytes of cheader */

  /* most recent MIDI commands, used for new source init */

  unsigned char history_active;        /* flag for channel activity */
  unsigned char history_cc_bank_msb;   /* current cc values */
  unsigned char history_cc_bank_lsb;
  unsigned char history_cc_modwheel;
  unsigned char history_cc_volume;

  unsigned char history_program;       /* last program change */
  unsigned char history_program_bank_msb;
  unsigned char history_program_bank_lsb;

  /* sequence numbers -- 0 or highest seqnum */

  unsigned long cheader_seqnum; 
  unsigned long chapterp_seqnum;
  unsigned long chapterc_seqnum;
  unsigned long chapterc_seqarray[NSYS_SM_CC_ARRAYSIZE];
  unsigned long chapterm_seqnum;
  unsigned long chapterm_seqarray[NSYS_SM_CM_ARRAYSIZE];
  unsigned long chapterm_dummy_seqnum;
  unsigned long chapterw_seqnum;    
  unsigned long chaptern_seqnum;    
  unsigned long chaptern_seqarray[NSYS_SM_CN_ARRAYSIZE];
  unsigned long chaptere_seqnum;
  unsigned long chapterer_seqarray[NSYS_SM_CE_ARRAYSIZE];
  unsigned long chapterev_seqarray[NSYS_SM_CE_ARRAYSIZE];
  unsigned long chaptert_seqnum;    
  unsigned long chaptera_seqnum;
  unsigned long chaptera_seqarray[NSYS_SM_CA_ARRAYSIZE];

  /* sender state for chapter c */
  
  unsigned long chapterc_logptr[NSYS_SM_CC_ARRAYSIZE];  /* log positions */
  int chapterc_sset;                                    /* S bit indicators */

  unsigned char chapterc_sustain;                       /* ALT state holders */
  unsigned char chapterc_allsound;                      
  unsigned char chapterc_rac;
  unsigned char chapterc_allnotes;                    
  unsigned char chapterc_omni_off;                    
  unsigned char chapterc_omni_on;                    
  unsigned char chapterc_monomode;                    
  unsigned char chapterc_polymode;                    

  /* sender state for chapter m */
  
  unsigned long chapterm_logptr[NSYS_SM_CM_ARRAYSIZE];  /* log positions    */
  unsigned long chapterm_dummy_logptr;
  int chapterm_sset;                                    /* S bit indicators */

  unsigned char chapterm_state;                         /* state machine    */
  unsigned char chapterm_rpn_msb;
  unsigned char chapterm_rpn_lsb;
  unsigned char chapterm_nrpn_msb;
  unsigned char chapterm_nrpn_lsb;

  short chapterm_cbutton[NSYS_SM_CM_ARRAYSIZE];    /* C-active button count */

  /* sender state for chapter n */
  
  unsigned char chapterb_low;      
  unsigned char chapterb_high;   
  unsigned long chaptern_logptr[NSYS_SM_CN_ARRAYSIZE]; /* note log position */
  unsigned long chaptern_timer[NSYS_SM_CN_ARRAYSIZE];  /* Y timer values */
  unsigned long chaptern_timernum;                     /* number of values */
  int chaptern_sset;                                   /* S bit indicators */

  /* sender state for chapter e */
  
  unsigned char chaptere_ref[NSYS_SM_CE_ARRAYSIZE];     /* reference count   */
  unsigned long chapterer_logptr[NSYS_SM_CE_ARRAYSIZE];  /* ref log positions */
  unsigned long chapterev_logptr[NSYS_SM_CE_ARRAYSIZE]; /* off vel positions */
  int chaptere_sset;                                    /* S bit indicator */
  
  /* sender state for chapter a */
  
  unsigned long chaptera_logptr[NSYS_SM_CA_ARRAYSIZE];  /* log positions */
  int chaptera_sset;                                    /* S bit indicators */
  
  /* sizes for dynamic chapters */
  
  int chapterc_size;  /* chapter c */
  int chapterm_size;  /* chapter m -- header  */
  int chapterl_size;  /* chapter m -- loglist */
  int chaptern_size;  /* chapter n */
  int chapterb_size;  /* chapter n - bitfields */
  int chaptere_size;  /* chapter e - header + ref log array */
  int chaptera_size;  /* chapter a */
  
  /* holds current packet bytes */
  
  unsigned char cheader[NSYS_SM_CH_SIZE];         /* chapter header */
  unsigned char chapterp[NSYS_SM_CP_SIZE];        /* chapter p  */
  unsigned char chapterc[NSYS_SM_CC_SIZE];        /* chapter c  */
  unsigned char chapterm[NSYS_SM_CM_HDRMAXSIZE];  /* chapter m  */
  unsigned char chapterl[NSYS_SM_CM_LISTMAXSIZE]; /* chapter m - loglist */
  unsigned char chapterw[NSYS_SM_CW_SIZE];        /* chapter w  */
  unsigned char chaptern[NSYS_SM_CN_SIZE];        /* chapter n  */
  unsigned char chapterb[NSYS_SM_CB_SIZE];        /* chapter n - bfields */
  unsigned char chaptere[NSYS_SM_CE_SIZE];        /* chapter e */
  unsigned char chaptert[NSYS_SM_CT_SIZE];        /* chapter t  */
  unsigned char chaptera[NSYS_SM_CA_SIZE];        /* chapter a  */
  
} nsys_netout_jsend_state;

extern nsys_netout_jsend_state nsys_netout_jsend[];

/***********************************************/
/* structure holding a chapter x stack element */
/***********************************************/

typedef struct nsys_netout_jsend_xstack_element {

  int size;
  int index;                                          /* stack position    */
  unsigned long seqnum;
  struct nsys_netout_jsend_xstack_element ** cmdptr;  /* command pointer   */

  struct nsys_netout_jsend_xstack_element * next;

  unsigned char log[NSYS_SM_CX_SIZE_MAXLOG];

} nsys_netout_jsend_xstack_element;

extern nsys_netout_jsend_xstack_element nsys_netout_jsend_xpile[]; /* element array */
extern nsys_netout_jsend_xstack_element * nsys_netout_jsend_xstackfree; /* freelist */

/*****************************************/
/* structure holding sender system state */
/*****************************************/

typedef struct nsys_netout_jsend_system_state {
  
  unsigned short slen;    /* value of LENGTH header field */

  /* sequence numbers -- 0 or highest seqnum */

  unsigned long sheader_seqnum; 
  unsigned long chapterd_seqnum;
  unsigned long chapterd_reset_seqnum;
  unsigned long chapterd_tune_seqnum;
  unsigned long chapterd_song_seqnum;
  unsigned long chapterd_scj_seqnum;
  unsigned long chapterd_sck_seqnum;
  unsigned long chapterd_rty_seqnum;
  unsigned long chapterd_rtz_seqnum;
  unsigned long chapterv_seqnum;
  unsigned long chapterq_seqnum;
  unsigned long chapterf_seqnum;
  unsigned long chapterfc_seqnum;
  unsigned long chapterx_seqnum;

  /* sizes for dynamic chapters */

  int chapterd_front_size;
  int chapterd_scj_size;
  int chapterd_sck_size;
  int chapterq_size; 
  int chapterf_size; 

  /* state for Chapter D (Simple System Commands) */

  unsigned char chapterd_reset;
  unsigned char chapterd_tune;
  unsigned char chapterd_song;

  /* state for Chapter F (MIDI Time Code) */

  unsigned char chapterf_point;

  /* state for Chapter X (SysEx) */

  int chapterx_stacklen;          /* number of logs in stack            */

  nsys_netout_jsend_xstack_element * chapterx_gmreset_off;   /* points into stack */
  nsys_netout_jsend_xstack_element * chapterx_gmreset_on; 
  nsys_netout_jsend_xstack_element * chapterx_mvolume;  

  unsigned char chapterx_mvolume_count;      /* session history counts */
  unsigned char chapterx_gmreset_off_count;  
  unsigned char chapterx_gmreset_on_count;   

  /* holds current packet bytes */
  
  unsigned char sheader[NSYS_SM_SH_SIZE];                  /* chapter header */
  unsigned char chapterd_front[NSYS_SM_CD_FRONT_MAXSIZE];  /* chapter d (front) */
  unsigned char chapterd_scj[NSYS_SM_CD_COMMON_SIZE];      /* chapter d (common J) */
  unsigned char chapterd_sck[NSYS_SM_CD_COMMON_SIZE];      /* chapter d (common K) */
  unsigned char chapterd_rty[NSYS_SM_CD_REALTIME_SIZE];    /* chapter d (realtime Y) */
  unsigned char chapterd_rtz[NSYS_SM_CD_REALTIME_SIZE];    /* chapter d (realtime Z) */
  unsigned char chapterv[NSYS_SM_CV_SIZE];                 /*   chapter v    */
  unsigned char chapterq[NSYS_SM_CQ_MAXSIZE];              /*   chapter q    */
  unsigned char chapterf[NSYS_SM_CF_MAXSIZE];              /*   chapter f    */
  nsys_netout_jsend_xstack_element * chapterx_stack[NSYS_SM_CX_MAXSLOTS]; /* ch x */

} nsys_netout_jsend_system_state;

extern nsys_netout_jsend_system_state nsys_netout_jsend_system;

/********************************************/
/* structure holding receiver channel state */
/********************************************/

typedef struct nsys_netout_jrecv_state {

  /* chapter p */

  unsigned char chapterp_program;
  unsigned char chapterp_bank_msb;
  unsigned char chapterp_bank_lsb;

  /* chapter c */

  unsigned char chapterc_value[NSYS_SM_CC_ARRAYSIZE];

  /* chapter m */

  unsigned char chapterm_value_lsb[NSYS_SM_CM_ARRAYSIZE];
  unsigned char chapterm_value_msb[NSYS_SM_CM_ARRAYSIZE];
  short chapterm_cbutton[NSYS_SM_CM_ARRAYSIZE];

  unsigned char chapterm_state;             /* state machine */
  unsigned char chapterm_rpn_msb;
  unsigned char chapterm_rpn_lsb;
  unsigned char chapterm_nrpn_msb;
  unsigned char chapterm_nrpn_lsb;

  /* chapter w */

  unsigned char chapterw_first;
  unsigned char chapterw_second;

  /* chapter n */

  unsigned char chaptern_ref[NSYS_SM_CN_ARRAYSIZE];
  unsigned char chaptern_vel[NSYS_SM_CN_ARRAYSIZE];
  unsigned long chaptern_tstamp[NSYS_SM_CN_ARRAYSIZE];
  unsigned long chaptern_extseq[NSYS_SM_CN_ARRAYSIZE];

  /* chapter t */

  unsigned char chaptert_pressure;

  /* chapter a */

  unsigned char chaptera_pressure[NSYS_SM_CA_ARRAYSIZE];

  /* navigation */

  unsigned char chan;
  struct nsys_netout_jrecv_state * next;

} nsys_netout_jrecv_state;

extern nsys_netout_jrecv_state * nsys_recvfree;  /* channel free list */

/*******************************************/
/* structure holding receiver system state */
/*******************************************/

typedef struct nsys_netout_jrecv_system_state {

  /* chapter d */

  unsigned char chapterd_reset;
  unsigned char chapterd_tune;
  unsigned char chapterd_song;
  unsigned char chapterd_scj_count;
  unsigned char chapterd_scj_data1;
  unsigned char chapterd_scj_data2;
  unsigned char chapterd_sck_count;
  unsigned char chapterd_sck_data1;
  unsigned char chapterd_sck_data2;
  unsigned char chapterd_rty;
  unsigned char chapterd_rtz;

  /* chapter v */

  unsigned char chapterv_count;

  /* chapter q */

  unsigned char chapterq_shadow[NSYS_SM_CQ_MAXSIZE];

  /* chapter f */

  unsigned char chapterf_has_complete;
  unsigned char chapterf_quarter;
  unsigned char chapterf_has_partial;
  unsigned char chapterf_direction;
  unsigned char chapterf_point;

  unsigned char chapterf_complete[NSYS_SM_CF_SIZE_COMPLETE];
  unsigned char chapterf_partial[NSYS_SM_CF_SIZE_PARTIAL];

  /* chapter x */

  unsigned char chapterx_gmreset;
  unsigned char chapterx_gmreset_off_count;  
  unsigned char chapterx_gmreset_on_count;   

  unsigned char chapterx_mvolume_lsb;
  unsigned char chapterx_mvolume_msb;

  /* navigation */

  struct nsys_netout_jrecv_system_state * next;

} nsys_netout_jrecv_system_state;

extern nsys_netout_jrecv_system_state * nsys_recvsysfree;  /* system free list */

/****************************/
/* supported SysEx commands */
/****************************/

extern unsigned char nsys_netout_sysconst_manuex[];
extern unsigned char nsys_netout_sysconst_mvolume[];
extern unsigned char nsys_netout_sysconst_gmreset[];

/*******/
/* SIP */
/*******/

extern unsigned char nsys_rtp_invite[];
extern unsigned char nsys_rtcp_invite[];

extern int nsys_rtp_sipretry;                  /* sip server retry counter */
extern int nsys_rtcp_sipretry;                 

extern int nsys_rtp_authretry;                 /* reauthorization counter  */
extern int nsys_rtcp_authretry;

extern struct sockaddr_in nsys_sip_rtp_addr;   /* current SIP RTP channel */
extern char nsys_sip_rtp_ip[];
extern unsigned long nsys_sip_rtp_inet_addr;
extern unsigned short nsys_sip_rtp_port;
extern unsigned short nsys_sip_rtp_sin_port;

extern struct sockaddr_in nsys_sip_rtcp_addr;  /* current SIP RTCP channel */
extern char nsys_sip_rtcp_ip[];
extern unsigned long nsys_sip_rtcp_inet_addr;
extern unsigned short nsys_sip_rtcp_port;
extern unsigned short nsys_sip_rtcp_sin_port;

extern int nsys_graceful_exit;                 /* requests termination    */

extern unsigned char nsys_rtp_info[];          /* SIP INFO packets        */
extern unsigned char nsys_rtcp_info[];

extern int nsys_behind_nat;                    /* 1 if behind a nat        */
extern int nsys_sipinfo_count;                 /* INFO sending timer       */
extern int nsys_sipinfo_toggle;                /* RTP/RTCP toggle          */

/***************/
/* SSRC stack  */
/***************/

typedef struct nsys_source {

  /* information */

  int mset;                           /* MIDI extchan set  */
  unsigned long ssrc;                 /* RTP SSRC number   */
  unsigned long birthtime;            /* Time SSRC was born (UNIX)    */

  unsigned long siptime;              /* SIP INVITE sessiontime (UNIX)    */
  struct sockaddr_in * sdp_addr;      /* IP/port in SDP packet, unchanged */

  struct sockaddr_in * rtp_addr;      /* RTP IP/port of source */
  struct sockaddr_in * alt_rtp_addr;  /* RTP IP/port of NATing */

  struct sockaddr_in * rtcp_addr;     /* RTCP IP/port of source */
  struct sockaddr_in * alt_rtcp_addr; /* RTCP IP/port of NATing */
 
  unsigned char ptype;                /* payload type to send/recv */
  int pindex;                         /* index into nsys_payload_types[]  */
  int srate;                          /* SDP-assigned srate        */

  /* navigation */

  struct nsys_source * xtra;               /* if hash clashes */
  struct nsys_source * prev;               /* doubly-linked list */
  struct nsys_source * next;

  /* reception statistics */

  unsigned long received;             /* total num packets received   */
  unsigned long received_prior;       /* total at last SR/RR          */
  unsigned long expected_prior;       /* total expected at last SR/RR */

  unsigned long base_seq;             /* initial extended seq num   */
  unsigned long hi_lobits;            /* highest seq num low 16b    */
  unsigned long hi_ext;               /* highest extended seq num   */

  /* rtcp items */

  int j_delta;                        /* state variable for jitter  */
  unsigned long jitter;               /* current jitter value       */
  unsigned char lsr[4];               /* last SR timestamp received */
  struct timeval arrival;             /* arrival time of last SR    */
  time_t expire_rtcp;                 /* time to expire ssrc entry  */
  char * cname;                       /* canonical name             */
  unsigned long rtcp_received;        /* for DoS detection          */

  /* time model */

  int ontime;                         /* flags on-time RTP packets   */    
  unsigned long tm_convert;           /* local/remote time offset    */
  unsigned long tm_margin;            /* maximum lateness allowed    */
  unsigned long tm_lateflag;          /* congestion detection flag   */
  unsigned long tm_latetime;          /* tstamp of first late packet */

#if (NSYS_LATENOTES_DEBUG == NSYS_LATENOTES_DEBUG_ON)
  unsigned long tm_first;             /* temporary for printfs       */
  FILE * tm_fd;                       /* file pointer for dat files  */
#endif

  /* journal items */

  unsigned long last_hiseq_rec;       /* RTCP extended sequence number   */
  unsigned long last_hiseq_ext;       /* local-adjusted RTCP ext seq num */

  struct nsys_netout_jrecv_state
  * jrecv[CSYS_MIDI_NUMCHAN];         /* reciever channel journal state */

  nsys_netout_jrecv_system_state * jrecvsys;   /* receiver system state */
  
} nsys_source;

extern struct nsys_source * nsys_srcfree;       /* mset ssrc tokens */
extern struct nsys_source * nsys_ssrc[];        /* SSRC hash table  */
extern struct nsys_source * nsys_srcroot;       /* points into nsys_ssrc */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                              Macros                                    */
/*________________________________________________________________________*/


/******************/
/* error-checking */
/******************/

#define  NSYS_ERROR_RETURN(x) do {\
      nsys_stderr_size+=fprintf(stderr, "  Error: %s.\n", x);\
      nsys_stderr_size+=fprintf(stderr, "  Errno Message: %s\n\n", strerror(errno));\
      return NSYS_ERROR; } while (0)

#define  NSYS_ERROR_TERMINATE(x) do {\
      nsys_stderr_size+=fprintf(stderr, "  Runtime Errno Message: %s\n", strerror(errno));\
      epr(0,NULL,NULL, "Network error -- " x );}  while (0)


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                        Function Externs                                */
/*________________________________________________________________________*/

/******************/
/* error checking */
/******************/

extern void epr(int linenum, char * filename, char * token, char * message);

/****************/
/* net_siplib.c */
/****************/

extern int nsys_initsip(void);
extern void nsys_netin_reply(int fd, struct sockaddr_in * addr, 
			     unsigned char * packet, unsigned short status);

extern void nsys_netin_invite(int fd, struct sockaddr_in * addr, 
			      unsigned char * packet);

extern void nsys_createsip(int fd, char * method, unsigned char * sip,
			   char * natline, char * nonce);
extern void nsys_sendsip(int fd, struct sockaddr_in * addr, 
			 unsigned char * sip); 

extern unsigned char * nsys_netin_readmethod(int fd, struct sockaddr_in * addr, 
					     unsigned char * packet); 
extern void nsys_netin_ack(int fd, unsigned char * reply, 
			   unsigned long callid, unsigned long cseq);
extern int nsys_netin_replyparse(int fd, unsigned char * packet, 
				 char *nonce, char * natline, 
				 unsigned long * callid, unsigned long * cseq);
extern void nsys_netin_redirect(int fd, unsigned char * packet,
				unsigned short status);

extern int nsys_netin_sipaddr(int fd, char * ip, unsigned short port);
extern int nsys_netin_siporigin(int fd, struct sockaddr_in * addr);
extern int nsys_netin_make_marray(struct nsys_payinfo ** marray, char * media);
extern int nsys_netin_set_marray(char * line, struct nsys_payinfo marray[],
				 int mlen);

extern int nsys_netin_payvalid(struct nsys_payinfo marray[], int mlen, int fd);
extern void nsys_netin_payset(struct nsys_source * sptr, struct nsys_payinfo marray[],
			      int mlen);
extern int nsys_netin_noreplay(char * ip, unsigned short port, 
			       unsigned long sdp_time);
extern void nsys_sip_shutdown(void);

/****************/
/* net_rtplib.c */
/****************/

extern int nsys_setup(int block);
extern void nsys_shutdown(void);

extern nsys_source * nsys_netin_addsrc(int fd, long ssrc,
				  char * ip, unsigned short port);
extern int nsys_netin_rtpstats(nsys_source * sptr, unsigned char * packet);
extern void  nsys_warning(int level, char * message);
extern void nsys_terminate_error(char * message); 
extern void nsys_status(nsys_source * sptr, char * message);
extern int nsys_netin_ptypevalid(unsigned char ptype);
extern void nsys_netin_ptypeset(struct nsys_source * sptr, unsigned char ptype);
extern nsys_netout_jrecv_state * nsys_netin_newrecv(unsigned char chan);
extern nsys_netout_jrecv_system_state * nsys_netin_newrecvsys(void);
extern char * nsys_find_clientip(char * ip);

/****************/
/* net_rtcplib.c */
/****************/

extern nsys_source * nsys_netin_rtcp(unsigned char * packet, int len, 
				struct sockaddr_in * ipaddr);
extern int nsys_make_rtcp(void);
extern int nsys_make_rtcpbye(void);
extern void nsys_netout_rtcptime(void);
extern void nsys_delete_ssrc(unsigned long ssrc, char * reason);
extern nsys_source * nsys_harvest_ssrc(int fd, struct sockaddr_in * ipaddr);
extern void nsys_late_windowcheck(nsys_source * sptr, unsigned long tstamp);
extern char * nsys_netin_newcname(unsigned char * packet, int len);
extern void nsys_netin_bye(unsigned char * packet, int len);
extern int nsys_netout_excheck(void);
extern void nsys_netout_keepalive(void);
extern void nsys_netin_rtcp_display(unsigned char * packet, int len,
				    struct timeval * now);
extern void nsys_netout_rtcpreport(void);
extern void nsys_netout_rtcpsend(unsigned char * p, int len);
extern int nsys_netin_clear_mset(unsigned char * buff, long * fill,
				 long size);

extern void nsys_netout_rtcp_initpackets(void);
extern void nsys_netout_rtcp_initrr(unsigned char * p, int len);
extern void nsys_netout_rtcp_initsr(unsigned char * p, int len);
extern void nsys_netout_rtcp_initsdes(unsigned char * p, int len);
extern void nsys_netout_rtcp_initbye(unsigned char * p);
extern void nsys_netin_rtcp_trunc(int sub);
extern void nsys_netin_latenotes_open(nsys_source * sptr);

/****************/
/* net_sfront.c */
/****************/

extern int nsys_sysex_parse(unsigned char * cmd, unsigned char *ndata,
			    unsigned char * vdata, unsigned char * sysbuf, 
			    int sysidx);

/***************/
/* net_jsend.c */
/***************/

extern int nsys_netin_journal_create(unsigned char * packet, int len);

extern void nsys_netout_journal_addstate(unsigned char cmd,
					unsigned char ndata,
					unsigned char vdata);

extern void nsys_netout_journal_addhistory(unsigned char cmd,
					   unsigned char ndata,
					   unsigned char vdata);

extern void nsys_netin_journal_trimstate(nsys_source * lptr);

extern void nsys_netout_midistate_init(void);

extern void nsys_netout_guard_tick(void);

extern void nsys_netout_journal_changecheck(void);

extern void nsys_netout_journal_addprogram(nsys_netout_jsend_state * jsend, 
					   unsigned char ndata);

extern void nsys_netout_journal_addcontrol(nsys_netout_jsend_state * jsend, 
					   unsigned char ndata,
					   unsigned char vdata);

extern void nsys_netout_journal_addparameter(nsys_netout_jsend_state * jsend, 
					     unsigned char ndata,
					     unsigned char vdata);

extern void nsys_netout_journal_addpwheel(nsys_netout_jsend_state * jsend, 
					  unsigned char ndata, 
					  unsigned char vdata);

extern void nsys_netout_journal_addnoteoff(nsys_netout_jsend_state * jsend, 
					   unsigned char ndata, 
					   unsigned char vdata);

extern void nsys_netout_journal_addnoteon(nsys_netout_jsend_state * jsend, 
					  unsigned char ndata, 
					  unsigned char vdata);

extern void nsys_netout_journal_addnoteon_extras(nsys_netout_jsend_state * jsend, 
						 unsigned char ndata);

extern void nsys_netout_journal_addnoteoff_extras(nsys_netout_jsend_state * jsend, 
						  unsigned char ndata,
						  unsigned char vdata);

extern void nsys_netout_journal_addctouch(nsys_netout_jsend_state * jsend, 
					  unsigned char ndata);

extern void nsys_netout_journal_addptouch(nsys_netout_jsend_state * jsend, 
					   unsigned char ndata,
					   unsigned char vdata);

extern void nsys_netout_journal_addsong(unsigned char ndata);
extern void nsys_netout_journal_addtune(void);
extern void nsys_netout_journal_addreset(void);
extern void nsys_netout_journal_addsc(unsigned char cmd, unsigned char ndata,
				      unsigned char vdata);
extern void nsys_netout_journal_addrt(unsigned char cmd);

extern void nsys_netout_journal_addsense(void);

extern void nsys_netout_journal_addsequence(unsigned char cmd, unsigned char ndata,
					    unsigned char vdata);

extern void nsys_netout_journal_addtimecode(unsigned char ndata);

extern void nsys_netout_journal_addsysex(unsigned char cmd, unsigned char ndata,
					 unsigned char vdata);

extern void nsys_netout_journal_trimchapter(nsys_netout_jsend_state * jsend,
					    int channel);

extern void nsys_netout_journal_trimprogram(nsys_netout_jsend_state * jsend);

extern void nsys_netout_journal_trimallcontrol(nsys_netout_jsend_state * jsend);

extern void nsys_netout_journal_trimpartcontrol(nsys_netout_jsend_state * jsend,
						unsigned long minseq);

extern void nsys_netout_journal_trimallparams(nsys_netout_jsend_state * jsend);

extern void nsys_netout_journal_trimpartparams(nsys_netout_jsend_state * jsend,
					       unsigned long minseq);

extern void nsys_netout_journal_trimpwheel(nsys_netout_jsend_state * jsend);

extern void nsys_netout_journal_trimallnote(nsys_netout_jsend_state * jsend);

extern void nsys_netout_journal_trimpartnote(nsys_netout_jsend_state * jsend,
					     unsigned long minseq);

extern void nsys_netout_journal_trimallextras(nsys_netout_jsend_state * jsend);

extern void nsys_netout_journal_trimpartextras(nsys_netout_jsend_state * jsend,
					       unsigned long minseq);

extern void nsys_netout_journal_trimctouch(nsys_netout_jsend_state * jsend);

extern void nsys_netout_journal_trimallptouch(nsys_netout_jsend_state * jsend);

extern void nsys_netout_journal_trimpartptouch(nsys_netout_jsend_state * jsend,
						unsigned long minseq);

extern void nsys_netout_journal_trimsystem(void);
extern void nsys_netout_journal_trimsimple(void);
extern void nsys_netout_journal_trimreset(void);
extern void nsys_netout_journal_trimtune(void);
extern void nsys_netout_journal_trimsong(void);
extern void nsys_netout_journal_trimscj(void);
extern void nsys_netout_journal_trimsck(void);
extern void nsys_netout_journal_trimrty(void);
extern void nsys_netout_journal_trimrtz(void);
extern void nsys_netout_journal_trimsense(void);
extern void nsys_netout_journal_trimsequence(void);
extern void nsys_netout_journal_trimtimecode(void);
extern void nsys_netout_journal_trimparttimecode(void);
extern void nsys_netout_journal_trimsysex(void);
extern void nsys_netout_journal_trimpartsysex(unsigned long minseq);

extern void nsys_netout_journal_clear_nactive(nsys_netout_jsend_state * jsend);

extern void nsys_netout_journal_clear_cactive(nsys_netout_jsend_state * jsend);

extern void nsys_netout_journal_clear_active(unsigned char cmd);

extern void nsys_netout_journal_clear_clog(nsys_netout_jsend_state * jsend,
					   unsigned char number);

/**************/
/* net_recv.c */
/**************/

extern int nsys_netin_journal_recovery(nsys_source * sptr, int rtpcode, 
				       unsigned char * packet, int numbytes,
				       unsigned char * buff, long * fill,
				       long size);

extern void nsys_netin_journal_trackstate(nsys_source * sptr, unsigned char cmd,
					  unsigned char ndata, 
					  unsigned char vdata);

extern int nsys_netin_journal_addcmd_three(nsys_source * sptr, unsigned char * buff, 
					   long * fill, long size,
					   unsigned char cmd, unsigned char ndata,
					   unsigned char vdata);

extern int nsys_netin_journal_addcmd_two(nsys_source * sptr, unsigned char * buff, 
					 long * fill, long size,
					 unsigned char cmd, unsigned char ndata);

extern int nsys_netin_journal_addcmd_one(nsys_source * sptr, unsigned char * buff, 
					 long * fill, long size, unsigned char cmd);

extern int nsys_netin_journal_trans(nsys_source * sptr, unsigned char * buff, 
				long * fill, long size, unsigned char chan,
				int flags, unsigned char msb_num, 
				unsigned char lsb_num, unsigned char msb_val, 
				unsigned char lsb_val);

extern int nsys_netin_journal_button_trans(nsys_source * sptr, unsigned char * buff, 
					   long * fill, long size, 
					   unsigned char chan, int flags,
					   unsigned char msb_num, 
					   unsigned char lsb_num, short count);

extern void nsys_netin_journal_clear_active(unsigned char cmd);
     
extern int nsys_netin_jrec_program(nsys_source * sptr, unsigned char * p,
				   nsys_netout_jrecv_state * jrecv,
				   unsigned char * buff,  
				   long * fill, long size);

extern int nsys_netin_jrec_control(nsys_source * sptr, unsigned char * p,
				   nsys_netout_jrecv_state * jrecv, short loglen,
				   unsigned char many, unsigned char * buff, 
				   long * fill, long size);

extern int nsys_netin_jrec_param(nsys_source * sptr, unsigned char * p,
				 nsys_netout_jrecv_state * jrecv, 
				 short paramlen, unsigned char many, 
				 unsigned char * buff, long * fill,
				 long size);

extern int nsys_netin_jrec_wheel(nsys_source * sptr, unsigned char * p,
				 nsys_netout_jrecv_state * jrecv,
				 unsigned char * buff,  
				 long * fill, long size);

extern int nsys_netin_jrec_notelog(nsys_source * sptr, unsigned char * p,
				   nsys_netout_jrecv_state * jrecv, 
				   unsigned char many,
				   short loglen, unsigned char * checkptr,
				   unsigned char * buff, 
				   long * fill, long size);

extern int nsys_netin_jrec_bitfield(nsys_source * sptr, unsigned char * p,
				    nsys_netout_jrecv_state * jrecv, 
				    unsigned char low, unsigned char high,
				    unsigned char * buff, long * fill,
				    long size);

extern int nsys_netin_jrec_ctouch(nsys_source * sptr, unsigned char * p,
				  nsys_netout_jrecv_state * jrecv,
				  unsigned char * buff,  
				  long * fill, long size);

extern int nsys_netin_jrec_ptouch(nsys_source * sptr, unsigned char * p,
				  nsys_netout_jrecv_state * jrecv, short loglen,
				  unsigned char many, unsigned char * buff, 
				  long * fill, long size);

extern int nsys_netin_jrec_reset(nsys_source * sptr, unsigned char * ps,
				 nsys_netout_jrecv_system_state * jrecvsys,
				 unsigned char * buff, long * fill, long size);

extern int nsys_netin_jrec_tune(nsys_source * sptr, unsigned char * ps,
				nsys_netout_jrecv_system_state * jrecvsys,
				unsigned char * buff, long * fill, long size);

extern int nsys_netin_jrec_song(nsys_source * sptr, unsigned char * ps,
				nsys_netout_jrecv_system_state * jrecvsys,
				unsigned char * buff, long * fill, long size);

extern int nsys_netin_jrec_scj(nsys_source * sptr, unsigned char * ps,
			       nsys_netout_jrecv_system_state * jrecvsys,
			       unsigned char * buff, long * fill, long size);

extern int nsys_netin_jrec_sck(nsys_source * sptr, unsigned char * ps,
			       nsys_netout_jrecv_system_state * jrecvsys,
			       unsigned char * buff, long * fill, long size);

extern int nsys_netin_jrec_rty(nsys_source * sptr, unsigned char * ps,
			       nsys_netout_jrecv_system_state * jrecvsys,
			       unsigned char * buff, long * fill, long size);

extern int nsys_netin_jrec_rtz(nsys_source * sptr, unsigned char * ps,
			       nsys_netout_jrecv_system_state * jrecvsys,
			       unsigned char * buff, long * fill, long size);

extern int nsys_netin_jrec_sense(nsys_source * sptr, unsigned char * ps,
				 nsys_netout_jrecv_system_state * jrecvsys,
				 unsigned char * buff, long * fill, long size);

extern int nsys_netin_jrec_sequence(nsys_source * sptr, unsigned char * ps,
				    nsys_netout_jrecv_system_state * jrecvsys,
				    unsigned char * buff, long * fill, long size);

extern int nsys_netin_jrec_timecode(nsys_source * sptr, unsigned char * ps,
				    nsys_netout_jrecv_system_state * jrecvsys,
				    unsigned char * buff, long * fill, long size);

extern int nsys_netin_jrec_sysex(nsys_source * sptr, unsigned char * ps, int syslen,
				 nsys_netout_jrecv_system_state * jrecvsys,
				 unsigned char * buff, long * fill, long size);

extern void nsys_netin_track_timecode(nsys_netout_jrecv_system_state * jrecvsys,
				      unsigned char ndata);

/****************/
/* net_crypto.c */
/****************/

extern unsigned char * nsys_md5(unsigned char * digest, unsigned char * text,
				int len);

extern char * nsys_hmac_md5(unsigned char* text, int text_len, 
			    unsigned char * keydigest, unsigned char * digest);

extern unsigned char * nsys_digest_base64(unsigned char * output, 
					  unsigned char * input);

extern int nsys_digest_syntaxcheck(char * s);

#endif /* NSYS_NET_INCLUDE */

/* end Network library -- constants and externs */

