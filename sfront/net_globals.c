
/*
#    Sfront, a SAOL to C translator    
#    This file: Network library -- global variables
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

#ifndef NSYS_NET
#include "net_include.h"
#endif


/**********************************/
/* to get nsys_sfront to compile  */
/**********************************/

#ifndef NSYS_NET

sig_atomic_t graceful_exit;

/* handles termination in case of error */

void epr(int linenum, char * filename, char * token, char * message)

{

  fprintf(stderr, "\nRuntime Error.\n");
  if (filename != NULL)
    fprintf(stderr, "Location: File %s near line %i.\n",filename, linenum);
  if (token != NULL)
    fprintf(stderr, "While executing: %s.\n",token);
  if (message != NULL)
    fprintf(stderr, "Potential problem: %s.\n",message);
  fprintf(stderr, "Exiting program.\n\n");
  exit(-1);

}

#endif


/*************************/
/* rtp and rtcp: network */
/*************************/

int nsys_rtp_fd;                  /* fd for rtp           */
int nsys_rtcp_fd;                 /* fd for rtcp          */
int nsys_max_fd;                  /* fd for select        */

unsigned short nsys_rtp_port;     /* actual rtp port     */ 
unsigned short nsys_rtcp_port;    /* actual rtcp port    */ 

unsigned long nsys_rtp_cseq;      /* rtp cseq number      */
unsigned long nsys_rtcp_cseq;     /* rtcp cseq number     */

/*************************/
/* rtp and rtcp: packets */
/*************************/

unsigned char nsys_netout_rtp_packet[NSYS_UDPMAXSIZE];  /* rtp packet out */

unsigned char * nsys_netout_rtcp_packet_rrempty;
unsigned char * nsys_netout_rtcp_packet_rr;
unsigned char * nsys_netout_rtcp_packet_srempty;
unsigned char * nsys_netout_rtcp_packet_sr;
unsigned char * nsys_netout_rtcp_packet_bye;

int nsys_netout_rtcp_len_rrempty;
int nsys_netout_rtcp_len_rr;
int nsys_netout_rtcp_len_srempty;
int nsys_netout_rtcp_len_sr;
int nsys_netout_rtcp_len_bye;


unsigned char nsys_netout_rtcp_packet[NSYS_UDPMAXSIZE]; /* rtcp packet out */
int nsys_netout_rtcp_size;                              /* rtcp packet size*/

char * nsys_sdes_typename[NSYS_RTCPVAL_SDES_SIZE] = {  /* rtcp debug array */
  "ILLEGAL", "Cname", "Name", "Email", "Phone", 
  "Loc", "Tool", "Note", "Priv" };

unsigned long nsys_netout_seqnum;  /* rtp output sequence number */
unsigned long nsys_netout_tstamp;  /* rtp output timestamp */
unsigned char nsys_netout_markbit; /* for setting markerbit */

/************************/
/* rtp payload support  */
/************************/

#if 0
/* when adding new codecs, follow this sample  */
/* remember to increment NSYS_RTP_PAYSIZE, and */
/* get ordering correct.                       */

struct nsys_payinfo nsys_payload_types[NSYS_RTP_PAYSIZE] = 
{ 
  {NSYS_MPEG4_PINDEX,  96, "mpeg4-generic", (int)(ARATE + 0.5F)},
  {NSYS_NATIVE_PINDEX, 97, "rtp-midi", (int)(ARATE + 0.5F)}
};
#endif

struct nsys_payinfo nsys_payload_types[NSYS_RTP_PAYSIZE] = 
{ 
  {NSYS_MPEG4_PINDEX, 96, "pre1-mpeg4-generic", (int)(ARATE + 0.5F)}
};

/**************/
/* rtcp timer */
/**************/

int nsys_sent_last;       /* a packet was sent last RTCP period */
int nsys_sent_this;       /* a packet was sent this RTCP period */

unsigned long nsys_sent_packets;  /* number of packets sent */
unsigned long nsys_sent_octets;   /* number of octets sent */

time_t nsys_nexttime;     /* time for next RTCP check           */
int nsys_rtcp_ex;         /* status flags to check at RTCP time */

/******************/
/* identification */
/******************/

unsigned long nsys_myssrc;        /*  SSRC -- hostorder  */
unsigned long nsys_myssrc_net;    /*  SSRC -- netorder   */

#ifndef NSYS_NET
char * nsys_sessionname;   
char * nsys_sessionkey;   
int nsys_feclevel = NSYS_SM_FEC_STANDARD;
int nsys_lateplay;
float nsys_latetime = NSYS_SM_LATETIME;
#endif

unsigned char nsys_keydigest[NSYS_MD5_LENGTH];
unsigned char nsys_session_base64[NSYS_BASE64_LENGTH];
int nsys_msession;
int nsys_msessionmirror;

char nsys_clientname[NSYS_HOSTNAMESIZE];
char nsys_clientip[16];
char * nsys_username;

char nsys_cname[NSYS_CNAMESIZE];
unsigned char nsys_cname_len;
int nsys_powerup_mset; 

/***********/
/* logging */
/***********/

int nsys_stderr_size;

/*********************/
/* MIDI input buffer */
/*********************/

unsigned char nsys_buff[NSYS_BUFFSIZE];
long nsys_bufflen;
long nsys_buffcnt;

/*~~~~~~~~~~~*/
/* rtp-midi  */
/*___________*/

/*************************/
/* command section flags */
/*************************/

unsigned char nsys_netout_sm_header;

/*************************/
/* checkpoint management */
/*************************/

unsigned long nsys_netout_jsend_checkpoint_seqnum; /* current checkpoint */
unsigned long nsys_netout_jsend_checkpoint_changed; /* 1 if changed */

/*********************/
/* S-list management */
/*********************/

unsigned char * nsys_netout_jsend_slist[NSYS_SM_SLISTLEN];
int nsys_netout_jsend_slist_size;

/*******************************/
/* guard journal state machine */
/*******************************/

int nsys_netout_jsend_guard_send;    /* flag variable: send a guard packet */
int nsys_netout_jsend_guard_time;    /* guard packet timer state */
int nsys_netout_jsend_guard_next;    /* reload value for timer */
int nsys_netout_jsend_guard_ontime;  /* minimum delay time for noteon */ 
int nsys_netout_jsend_guard_mintime; /* minimum delay time for (!noteon) */
int nsys_netout_jsend_guard_maxtime; /* maximum delay time */ 

/***************************/
/* recovery journal header */
/***************************/

unsigned char nsys_netout_jsend_header[NSYS_SM_JH_SIZE];   /* journal header */

/**************************/
/* channel journal record */
/**************************/

unsigned char nsys_netout_jsend_channel[CSYS_MIDI_NUMCHAN];
unsigned char nsys_netout_jsend_channel_size;

/************************/
/* sender channel state */
/************************/

nsys_netout_jsend_state nsys_netout_jsend[CSYS_MIDI_NUMCHAN];

/***********************/
/* sender system state */
/***********************/

nsys_netout_jsend_system_state nsys_netout_jsend_system;
nsys_netout_jsend_xstack_element nsys_netout_jsend_xpile[NSYS_SM_CX_MAXSLOTS]; 
nsys_netout_jsend_xstack_element * nsys_netout_jsend_xstackfree; 

/**************************/
/* receiver channel state */
/**************************/

nsys_netout_jrecv_state * nsys_recvfree;  
nsys_netout_jrecv_system_state * nsys_recvsysfree;

/****************************/
/* supported SysEx commands */
/****************************/

unsigned char nsys_netout_sysconst_manuex[CSYS_MIDI_MANUEX_SIZE] = {
  0xF0, 0x43, 0x73, 0x7F, 0x32, 0x11, 0x00, 0x00, 0x00, 0xF7 };

unsigned char nsys_netout_sysconst_mvolume[CSYS_MIDI_MVOLUME_SIZE] = {
  0xF0, 0x7F, 0x7F, 0x04, 0x01, 0x00, 0x00, 0xF7};

unsigned char nsys_netout_sysconst_gmreset[CSYS_MIDI_GMRESET_SIZE] = {
  0xF0, 0x7E, 0x7F, 0x09, 0x00, 0xF7};


/*******/
/* SIP */
/*******/

unsigned char nsys_rtp_invite[NSYS_UDPMAXSIZE+1];
unsigned char nsys_rtcp_invite[NSYS_UDPMAXSIZE+1];

int nsys_rtp_sipretry;      /* sip server retry counter */
int nsys_rtcp_sipretry;                 

int nsys_rtp_authretry;     /* reauthorization counter  */
int nsys_rtcp_authretry;

struct sockaddr_in nsys_sip_rtp_addr;   /* current SIP RTP channel */
char nsys_sip_rtp_ip[16];
unsigned long nsys_sip_rtp_inet_addr;
unsigned short nsys_sip_rtp_port;
unsigned short nsys_sip_rtp_sin_port;

struct sockaddr_in nsys_sip_rtcp_addr;  /* current SIP RTCP channel */
char nsys_sip_rtcp_ip[16];
unsigned long nsys_sip_rtcp_inet_addr;
unsigned short nsys_sip_rtcp_port;
unsigned short nsys_sip_rtcp_sin_port;

int nsys_graceful_exit;                 /* requests termination      */

unsigned char nsys_rtp_info[NSYS_UDPMAXSIZE+1];  /* SIP INFO packets */
unsigned char nsys_rtcp_info[NSYS_UDPMAXSIZE+1];

int nsys_behind_nat;                      /* 1 if behind a nat        */
int nsys_sipinfo_count;                   /* INFO sending timer       */
int nsys_sipinfo_toggle;                  /* RTP/RTCP toggle          */

/***************/
/* SSRC stack  */
/***************/

struct nsys_source * nsys_srcfree = NULL;       /* mset ssrc tokens */
struct nsys_source * nsys_ssrc[NSYS_HASHSIZE];  /* SSRC hash table  */
struct nsys_source * nsys_srcroot = NULL;       /* points into nsys_ssrc */

/* end Network library -- global variables */
