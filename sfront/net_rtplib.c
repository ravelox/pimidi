
/*
#    Sfront, a SAOL to C translator    
#    This file: Network library -- RTP functions
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

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                 high-level RTP functions                     */
/*______________________________________________________________*/
 
/****************************************************************/
/*                  sets up networking                          */
/****************************************************************/

int nsys_setup(int block) 

{
  struct hostent * myinfo;
  struct nsys_source * newsrc;
  struct nsys_netout_jrecv_state * newrecv;
  struct nsys_netout_jrecv_system_state * newrecvsys;
  struct sockaddr_in iclient_addr; 
  struct timeval tv;
  unsigned char session_digest[NSYS_MD5_LENGTH];
  int one = 1;
  int i, rfd, bad_netconfig;

  /*******************************/
  /* create RTP and RTCP sockets */
  /*******************************/

  if ((nsys_rtp_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
    NSYS_ERROR_RETURN("Couldn't create Internet RTP socket");

  if ((nsys_rtcp_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
    NSYS_ERROR_RETURN("Couldn't create Internet RTCP socket");

  if (nsys_rtp_fd > nsys_rtcp_fd)
    nsys_max_fd = nsys_rtp_fd + 1;
  else
    nsys_max_fd = nsys_rtcp_fd + 1;

  /***************************/
  /* find open ports for RTP */
  /***************************/

  memset(&(iclient_addr.sin_zero), 0, 8);    
  iclient_addr.sin_family = AF_INET;   
  iclient_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  nsys_rtp_port = NSYS_RTP_PORT - 2;

  while ((nsys_rtp_port += 2) < NSYS_RTP_MAXPORT)
    {
      iclient_addr.sin_port = htons(nsys_rtp_port);
      if (bind(nsys_rtp_fd, (struct sockaddr *)&iclient_addr,
	       sizeof(struct sockaddr)) < 0)
	{
	  if ((errno == EADDRINUSE) || (errno == EADDRNOTAVAIL))
	    continue;
	  else
	    NSYS_ERROR_RETURN("Couldn't bind Internet RTP socket");
	}
      else
	{
	  nsys_rtcp_port = nsys_rtp_port + 1;
	  iclient_addr.sin_port = htons(nsys_rtcp_port);
	  if (bind(nsys_rtcp_fd, (struct sockaddr *)&iclient_addr,
		   sizeof(struct sockaddr)) < 0)
	    {
	      if ((errno == EADDRINUSE) || (errno == EADDRNOTAVAIL))
		continue;
	      else
		NSYS_ERROR_RETURN("Couldn't bind Internet RTCP socket");
	    }
	  else
	    break;
	}
    }

  if (nsys_rtp_port >= NSYS_RTP_MAXPORT)
    NSYS_ERROR_RETURN("Couldn't find open ports for RTP/RTCP sockets");


  /************************************************************/
  /* set non-blocking status & shield ICMP ECONNREFUSED errno */
  /************************************************************/

  if (block == NSYS_NONBLOCK)
    {
      fcntl(nsys_rtp_fd, F_SETFL, O_NONBLOCK);
      fcntl(nsys_rtcp_fd, F_SETFL, O_NONBLOCK);
    }

  setsockopt(nsys_rtp_fd, SOL_SOCKET, SO_BSDCOMPAT, &one, sizeof(one));
  setsockopt(nsys_rtcp_fd, SOL_SOCKET, SO_BSDCOMPAT, &one, sizeof(one));

  /************************/
  /* pick SSRC, set CNAME */
  /************************/

  gettimeofday(&tv, 0);

  srand((unsigned int)(((tv.tv_sec << NSYS_SECSHIFT) & NSYS_SECMASK)
		       |(NSYS_USECMASK & tv.tv_usec)));

  rfd = open("/dev/urandom", O_RDONLY | O_NONBLOCK);

  if ((rfd < 0) || (read(rfd, &nsys_myssrc, sizeof(unsigned long)) < 0))
    nsys_myssrc = (unsigned long)rand();

  nsys_myssrc_net = htonl(nsys_myssrc);

  bad_netconfig = 0;

  if (gethostname(nsys_clientname, NSYS_HOSTNAMESIZE) < 0)
    {
      bad_netconfig = 1;
      strcpy(nsys_clientname, "not_known");
    }

  if (!bad_netconfig && (myinfo = gethostbyname(nsys_clientname)))
    {
      strcpy(nsys_clientip, inet_ntoa(*((struct in_addr *)myinfo->h_addr)));
      bad_netconfig |= !strcmp(nsys_clientip, "127.0.0.1");
      bad_netconfig |= !strcmp(nsys_clientip, "0.0.0.0");
      bad_netconfig |= !strcmp(nsys_clientip, "255.255.255.255");
    }
  else
    bad_netconfig = 1;

  if (bad_netconfig)
    {
      if (nsys_find_clientip(nsys_clientip) == NULL)
	NSYS_ERROR_RETURN("Machine IP number unknown");

      if (!strcmp(nsys_clientip, "127.0.0.1"))
	NSYS_ERROR_RETURN("Machine IP misconfiguration (loopback address)");

      if (!strcmp(nsys_clientip, "0.0.0.0"))
	NSYS_ERROR_RETURN("Machine IP misconfiguration (0.0.0.0)");

      if (!strcmp(nsys_clientip, "255.255.255.255"))
	NSYS_ERROR_RETURN("Machine IP misconfiguration (broadcast address)");
    }

  if (getlogin())
    strcpy((nsys_username = malloc(strlen(getlogin())+1)), getlogin());
  else
    nsys_username = "unknown";

  snprintf(nsys_cname, NSYS_CNAMESIZE, "%s@%s:%hu",
	   nsys_username, nsys_clientip, nsys_rtcp_port);
  nsys_cname[NSYS_CNAMESIZE-1] = '\0';
  nsys_cname_len = (unsigned char) strlen(nsys_cname);

  /************************************************************/
  /* create digest sessionname, initialize authentication key */
  /************************************************************/

  if (nsys_sessionname == NULL)
    nsys_sessionname = "test_session";

  nsys_msession = !strcmp(NSYS_MSESSION_NAME, nsys_sessionname);
  nsys_msessionmirror = nsys_msession && !strcmp("mirror", APPNAME);

  if (nsys_sessionkey == NULL)
    {
      nsys_sessionkey = calloc(NSYS_MKEY_LENGTH + 1, 1);
      i = 0;
      while (i < NSYS_MKEY_LENGTH)
	{
	  if ((rfd < 0)|| (read(rfd, &(nsys_sessionkey[i]), sizeof(char)) < 0))
	    nsys_sessionkey[i] = (char)((255.0F*rand()
					 /(RAND_MAX+1.0F))-128.0F);
	  i = (isprint((int)nsys_sessionkey[i])) ? i + 1 : i; 
	}
    }

  nsys_md5(nsys_keydigest, (unsigned char *) nsys_sessionkey, 
	   strlen(nsys_sessionkey));

  for (i = 0; i < NSYS_MD5_ROUNDS; i++)
    nsys_md5(nsys_keydigest, nsys_keydigest, NSYS_MD5_LENGTH);

  if (nsys_msession)
    nsys_md5(session_digest, (unsigned char *) nsys_sessionname, 
	     strlen(nsys_sessionname));
  else
    nsys_hmac_md5((unsigned char *) nsys_sessionname, strlen(nsys_sessionname), 
		   nsys_keydigest, session_digest);

  nsys_digest_base64(nsys_session_base64, session_digest);

  /*************************************/
  /* create and send SIP invite packet */
  /*************************************/

  if (nsys_initsip() == NSYS_ERROR)
    return NSYS_ERROR;

  /* create mset slots */

  for (i = NSYS_MSETS; i > 0; i--)
    {
      newsrc = calloc(1, sizeof(struct nsys_source));
      newsrc->mset = i;
      newsrc->next = nsys_srcfree;
      nsys_srcfree = newsrc;
    }

  for (i = 0; i < NSYS_RECVCHAN*NSYS_MSETS; i++)
    {
      newrecv = calloc(1, sizeof(struct nsys_netout_jrecv_state));
      newrecv->next = nsys_recvfree;
      nsys_recvfree = newrecv;
    }

  for (i = 0; i < NSYS_MSETS; i++)
    {
      newrecvsys = calloc(1, sizeof(struct nsys_netout_jrecv_system_state));
      newrecvsys->next = nsys_recvsysfree;
      nsys_recvsysfree = newrecvsys;
    }

  for (i = 0; i < NSYS_SM_CX_MAXSLOTS; i++)
    {
      nsys_netout_jsend_xpile[i].next = nsys_netout_jsend_xstackfree; 
      nsys_netout_jsend_xstackfree = &(nsys_netout_jsend_xpile[i]); 
    }

  /************************/
  /* setup network output */
  /************************/

  if ((rfd < 0)||(read(rfd, &(nsys_netout_tstamp), sizeof(unsigned long)) < 0))
    nsys_netout_tstamp = (unsigned long) rand();

  if ((rfd < 0)||(read(rfd, &(nsys_netout_seqnum), sizeof(unsigned long)) < 0))
    nsys_netout_seqnum = ((unsigned long) rand());

  nsys_netout_seqnum &= 0x0000FFFF;
  nsys_netout_seqnum = (!nsys_netout_seqnum) ? 1 : nsys_netout_seqnum;

  nsys_netout_rtp_packet[NSYS_RTPLOC_BYTE1] = NSYS_RTPVAL_BYTE1;

  nsys_netout_markbit = NSYS_RTPVAL_SETMARK;   /* required by mpeg4-generic */

  memcpy(&(nsys_netout_rtp_packet[NSYS_RTPLOC_SSRC]), &nsys_myssrc_net,
	 sizeof(long));

  if (nsys_feclevel != NSYS_SM_FEC_NONE)
    nsys_netout_sm_header = NSYS_SM_SETJ;

  /*********************/
  /* setup RTCP system */
  /*********************/

  nsys_netout_rtcp_initpackets();
  nsys_nexttime = time(NULL) + NSYS_RTCPTIME_INCR;

  nsys_rtcp_ex  = NSYS_RTCPEX_NULLROOT;
  nsys_rtcp_ex |= (NSYS_RTCPEX_RTCPSIP | NSYS_RTCPEX_RTPSIP); 

  /********************************/
  /* setup rtp-midi packetization */
  /********************************/

  nsys_netout_jsend_checkpoint_seqnum = nsys_netout_seqnum;
  nsys_netout_jsend_header[NSYS_SM_JH_LOC_FLAGS] = 0;

  for (i = 0; i < CSYS_MIDI_NUMCHAN; i++)
    nsys_netout_jsend[i].chan = i;

  nsys_netout_journal_changecheck();

  switch (nsys_feclevel) {
  case NSYS_SM_FEC_NONE:
  case NSYS_SM_FEC_NOGUARD:
    nsys_netout_jsend_guard_ontime  = 0;
    nsys_netout_jsend_guard_mintime = 0;
    nsys_netout_jsend_guard_maxtime = 0;
    break;
  case NSYS_SM_FEC_MINIMAL:
    nsys_netout_jsend_guard_ontime  = 0;
    nsys_netout_jsend_guard_mintime = (ARATE*NSYS_SM_GUARD_MINTIME)/ACYCLE;
    nsys_netout_jsend_guard_maxtime = (ARATE*NSYS_SM_GUARD_MAXTIME)/ACYCLE;
    break;
  case NSYS_SM_FEC_STANDARD:
    nsys_netout_jsend_guard_ontime  = 0;
    nsys_netout_jsend_guard_mintime = (ARATE*NSYS_SM_GUARD_STDTIME)/ACYCLE;
    nsys_netout_jsend_guard_maxtime = (ARATE*NSYS_SM_GUARD_MAXTIME)/ACYCLE;
    break;
  case NSYS_SM_FEC_EXTRA:
    nsys_netout_jsend_guard_ontime  = (ARATE*NSYS_SM_GUARD_ONTIME)/ACYCLE;
    nsys_netout_jsend_guard_mintime = (ARATE*NSYS_SM_GUARD_STDTIME)/ACYCLE;
    nsys_netout_jsend_guard_maxtime = (ARATE*NSYS_SM_GUARD_MAXTIME)/ACYCLE;
    break;
  }

  if (rfd >= 0)
    close(rfd);

  return NSYS_DONE;
}


/****************************************************************/
/*                  tears down networking                       */
/****************************************************************/

void nsys_shutdown(void) 

{  

#if (NSYS_LATENOTES_DEBUG == NSYS_LATENOTES_DEBUG_ON)
  struct nsys_source * sptr = nsys_srcroot;
#endif

  nsys_warning(NSYS_WARN_STANDARD, "Shutdown in progress");

  /* send RTCP BYE command to all listeners */

  if ((nsys_rtcp_ex & NSYS_RTCPEX_NULLROOT) == 0)
    nsys_netout_rtcpsend(nsys_netout_rtcp_packet_bye, 
			 nsys_netout_rtcp_len_bye + NSYS_RTPSIZE_DIGEST);

  /* send SIP BYE command to the SIP server */

  if ((nsys_rtcp_ex & (NSYS_RTCPEX_RTPSIP | NSYS_RTCPEX_RTCPSIP)) == 0)
    nsys_sip_shutdown();

  /* close network FD's */

  close(nsys_rtp_fd);
  close(nsys_rtcp_fd);

  nsys_warning(NSYS_WARN_STANDARD, "Shutdown completed");

#if (NSYS_LATENOTES_DEBUG == NSYS_LATENOTES_DEBUG_ON)

  if (sptr)
    do {
      printf("mset%i packets expected: %i\n", sptr->mset,
	     sptr->hi_ext - sptr->base_seq + 1);
      printf("mset%i packets received: %i\n", sptr->mset,
	     sptr->received);
      if (sptr->tm_fd)
	fclose(sptr->tm_fd);
    } while ((sptr = sptr->next) != nsys_srcroot);

#endif

}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                  low-level RTP functions                     */
/*______________________________________________________________*/

/****************************************************************/
/*              add new SSRC to send/receive list               */
/****************************************************************/

nsys_source * nsys_netin_addsrc(int fd, long ssrc, char * ip, unsigned short port)

{
  struct nsys_source * sptr;
  struct sockaddr_in ipaddr;

  if (nsys_srcfree == NULL)
    {
      memset(&ipaddr, 0, sizeof(struct sockaddr_in));
      ipaddr.sin_port = htons(port);
      ipaddr.sin_family = AF_INET;  
      ipaddr.sin_addr.s_addr = inet_addr(ip);
      if (nsys_harvest_ssrc(nsys_rtp_fd, &ipaddr) == NULL)
	return NULL;
    }

  nsys_rtcp_ex |= (nsys_myssrc == ssrc) ? NSYS_RTCPEX_SRCDUPL : 0;

  sptr = nsys_srcfree;
  nsys_srcfree = nsys_srcfree->next;
  
  /* set ssrc and IP/port */

  sptr->ssrc = ssrc;

  /* unchanged, used to detect replay attacks */

  sptr->sdp_addr = calloc(1, sizeof(struct sockaddr_in));
  sptr->sdp_addr->sin_family = AF_INET;   
  sptr->sdp_addr->sin_port = htons(port);
  sptr->sdp_addr->sin_addr.s_addr = inet_addr(ip);

  /* may be changed if NAT present */

  sptr->rtp_addr = calloc(1, sizeof(struct sockaddr_in));
  sptr->rtp_addr->sin_family = AF_INET;   
  sptr->rtp_addr->sin_port = htons(port);
  sptr->rtp_addr->sin_addr.s_addr = inet_addr(ip);

  sptr->rtcp_addr = calloc(1, sizeof(struct sockaddr_in));
  sptr->rtcp_addr->sin_family = AF_INET;   
  sptr->rtcp_addr->sin_port = htons(port + 1);
  sptr->rtcp_addr->sin_addr.s_addr = sptr->rtp_addr->sin_addr.s_addr;

  sptr->alt_rtp_addr = sptr->alt_rtcp_addr = NULL;
  sptr->cname = NULL;

  if (nsys_srcroot)
    {
      nsys_srcroot->next->prev = sptr;
      sptr->next = nsys_srcroot->next;
      sptr->prev = nsys_srcroot;
      nsys_srcroot->next = sptr;
    }
  else
    {
      nsys_srcroot = sptr->prev = sptr->next = sptr;
      nsys_rtcp_ex &= ~NSYS_RTCPEX_NULLROOT;
    }

  sptr->xtra = nsys_ssrc[NSYS_HASHMASK & ssrc];
  nsys_ssrc[NSYS_HASHMASK & ssrc] = sptr;

  sptr->birthtime = time(NULL);
  sptr->expire_rtcp = sptr->birthtime + NSYS_RTCPTIME_TIMEOUT;

  nsys_netout_midistate_init();

  return sptr;
}

/****************************************************************/
/*         updates RTCP info for a new RTP packet               */
/****************************************************************/

int nsys_netin_rtpstats(nsys_source * sptr, unsigned char * packet)

{
  unsigned long seq;
  int seqinc, delta;
  int rtpcode;

#if (NSYS_LATENOTES_DEBUG == NSYS_LATENOTES_DEBUG_ON)
  int mlen, first;
#endif

  seq = ntohs(*((unsigned short *)&(packet[NSYS_RTPLOC_SEQNUM])));

  delta = nsys_netout_tstamp - 
    ntohl(*((unsigned long *)&(packet[NSYS_RTPLOC_TSTAMP])));
   
  if (sptr->received++)
    {
      if (abs((seqinc = (seq - sptr->hi_lobits))) > NSYS_RTPSEQ_MAXDIFF)
	{
	  if ((seq < NSYS_RTPSEQ_LOWLIMIT) && 
	      (sptr->hi_lobits > NSYS_RTPSEQ_HILIMIT))
	    {
	      /* wraparound */
	      
	      seqinc = seq + (NSYS_RTPSEQ_LOMASK - sptr->hi_lobits) + 1;
	      sptr->hi_ext += NSYS_RTPSEQ_EXINCR;
	    }
	  else
	    {
	      if ((sptr->hi_lobits < NSYS_RTPSEQ_LOWLIMIT) && 
		  (seq < NSYS_RTPSEQ_HILIMIT))
		{
		  seqinc = 0;  /* out-of-order packets in wraparound */
		}
	      else
		{
		  /* possible replay attack -- discard packet */
		  
		  sptr->received--;
		  return NSYS_RTPCODE_SECURITY;
		  
		  /* older behavior: something bad happened -- reset */
		  
		  seqinc = 3;        
		  sptr->hi_ext = 0;
		  sptr->base_seq = seq;
		}
	    }
	}
      
      if (seqinc <= 0)
	rtpcode = NSYS_RTPCODE_DISCARD;   /* duplicate or out-of-order */
      else
	{
	  sptr->hi_lobits = seq;
	  sptr->hi_ext &= NSYS_RTPSEQ_EXMASK;
	  sptr->hi_ext += seq;
	  
	  switch(seqinc) {
	  case 1:
	    rtpcode = NSYS_RTPCODE_NORMAL; 
	    break;
	  case 2:
	    rtpcode = NSYS_RTPCODE_LOSTONE; 
	    break;
	  default:
	    rtpcode = NSYS_RTPCODE_LOSTMANY; 
	    break;
	  }
	}

      sptr->jitter += (abs(delta - sptr->j_delta) -
		       ((sptr->jitter + 8) >> 4));
    }
  else
    {
      sptr->hi_ext = sptr->hi_lobits = sptr->base_seq = seq;
      if (!(sptr->rtcp_received))
	nsys_status(sptr, "Media (RTP) flowing from");
      rtpcode = NSYS_RTPCODE_LOSTMANY;
    }
      
  sptr->j_delta = delta;

  if (sptr->tm_margin)
    {
      sptr->ontime = (((signed long)(delta + sptr->tm_convert)) < 0);
      if (sptr->ontime)
	{
	  if (sptr->tm_lateflag)
	    sptr->tm_lateflag = 0;
	}
      else
	{
	  nsys_late_windowcheck(sptr, nsys_netout_tstamp - delta);
	  sptr->ontime = !(sptr->tm_lateflag);
	}
    }
  else
    sptr->ontime = 1;

#if (NSYS_LATENOTES_DEBUG == NSYS_LATENOTES_DEBUG_ON)

  if (sptr->tm_margin)
    {
      fprintf(sptr->tm_fd, "%f %f #",
	      (nsys_netout_tstamp - sptr->tm_first)/ARATE,
	      ((int)(delta + sptr->tm_convert))/ARATE);

      /* not checking length, but not dangerous */

      mlen = NSYS_SM_MLENMASK & packet[NSYS_RTPLEN_HDR];
      first = NSYS_RTPLEN_HDR + 1;

      if (packet[NSYS_RTPLEN_HDR] & NSYS_SM_CHKB)
	{
	  mlen = packet[NSYS_RTPLEN_HDR+1] + (mlen << 8);
	  first++;
	}

      if (mlen == 0)
	fprintf(sptr->tm_fd, "Empty\n");
      else
	if (packet[NSYS_RTPLEN_HDR] & NSYS_SM_CHKZ)
	  fprintf(sptr->tm_fd, "Delta-Time\n");
	else
	  switch(packet[first] & 0xF0) {
	  case CSYS_MIDI_NOTEOFF:
	    fprintf(sptr->tm_fd, "NoteOff\n");
	    break;
	  case CSYS_MIDI_NOTEON:
	    if (packet[first + 2])
	      fprintf(sptr->tm_fd, "NoteOn\n");
	    else
	      fprintf(sptr->tm_fd, "NoteOff\n");
	    break;
	  case CSYS_MIDI_PTOUCH:
	    fprintf(sptr->tm_fd, "PTouch\n");
	    break;
	  case CSYS_MIDI_CC:
	    fprintf(sptr->tm_fd, "CChange\n");
	    break;
	  case CSYS_MIDI_PROGRAM:
	    fprintf(sptr->tm_fd, "PChange\n");
	    break;
	  case CSYS_MIDI_CTOUCH:
	    fprintf(sptr->tm_fd, "CTouch\n");
	    break;
	  case CSYS_MIDI_WHEEL:
	    fprintf(sptr->tm_fd, "PWheel\n");
	    break;
	  case CSYS_MIDI_SYSTEM:
	    fprintf(sptr->tm_fd, "System\n");
	    break;
	  default:
	    fprintf(sptr->tm_fd, "Data Octet\n");
	    break;
	  }
    }

#endif

  return rtpcode;

}

/****************************************************************/
/*             prints out warning messages                      */
/****************************************************************/

void nsys_warning(int level, char * message) 
		
{

#if (NSYS_WARN > NSYS_WARN_NONE)

  time_t now = time(NULL);

  switch (level) {
  case NSYS_WARN_STANDARD:
    nsys_stderr_size += 
      fprintf(stderr, "Network status: %s %s", message,
	      nsys_msessionmirror ? ctime(&now) : "\n");
    fflush(stderr);
    break;
  case NSYS_WARN_UNUSUAL:
    if (NSYS_WARN >= NSYS_WARN_UNUSUAL)
      {
	nsys_stderr_size += 
	  fprintf(stderr, "Network advisory: %s %s", message,
		  nsys_msessionmirror ? ctime(&now) : "\n");
	fflush(stderr);
      }
    break;
  }
  
#endif

}
	
/****************************************************************/
/*             prints out status message for a source           */
/****************************************************************/

void nsys_status(nsys_source * sptr, char * message) 
		
{
  time_t now = time(NULL);

  nsys_stderr_size += 
    fprintf(stderr, "Network status: %s %s %s", message, sptr->cname, 
	    nsys_msessionmirror ? ctime(&now) : "\n");

  fflush(stderr);
}
	  
/****************************************************************/
/*             prints out error-termination messages            */
/****************************************************************/

void nsys_terminate_error(char * message) 
		
{

  nsys_stderr_size += 
    fprintf(stderr, "Network status: Probable client/server bug found\n");
  nsys_stderr_size += 
    fprintf(stderr, "              : %s\n", message);
  nsys_stderr_size += 
    fprintf(stderr, "              : Please send bug details to"
	    " lazzaro@cs.berkeley.edu -- thanks! --jl\n");
  nsys_graceful_exit = 1;
  
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                   utility RTP functions                      */
/*______________________________________________________________*/


/****************************************************************/
/*             checks validity of payload ptype                 */
/****************************************************************/

int nsys_netin_ptypevalid(unsigned char ptype) 
			 

{
  int j;

  for (j = 0; j < NSYS_RTP_PAYSIZE; j++)
    if (nsys_payload_types[j].ptype == ptype)
     return 1;
      
  return 0;
}

/****************************************************************/
/*              fix payload for new RTP stream                  */
/****************************************************************/

void nsys_netin_ptypeset(struct nsys_source * sptr, unsigned char ptype)

{  
  int j;

  for (j = 0; j < NSYS_RTP_PAYSIZE; j++)
    if (nsys_payload_types[j].ptype == ptype)
      {	  
	sptr->ptype  = ptype;
	sptr->pindex = nsys_payload_types[j].pindex;
	sptr->srate  = nsys_payload_types[j].srate;
	return;
      }
}

/****************************************************************/
/*              return a new jrecv channel pointer              */
/****************************************************************/

nsys_netout_jrecv_state * nsys_netin_newrecv(unsigned char chan)

{
  nsys_netout_jrecv_state * ret;

  if (nsys_recvfree)
    {
      ret = nsys_recvfree;
      nsys_recvfree = nsys_recvfree->next;
      memset(ret, 0, sizeof(struct nsys_netout_jrecv_state));
    }
  else
    ret = calloc(1, sizeof(struct nsys_netout_jrecv_state));

  ret->chan = chan;
  return ret;
}
/***************************************************************/
/*              return a new jrecv system pointer              */
/***************************************************************/

nsys_netout_jrecv_system_state * nsys_netin_newrecvsys(void)

{
  nsys_netout_jrecv_system_state * ret;

  if (nsys_recvfree)
    {
      ret = nsys_recvsysfree;
      nsys_recvsysfree = nsys_recvsysfree->next;
      memset(ret, 0, sizeof(struct nsys_netout_jrecv_system_state));
    }
  else
    ret = calloc(1, sizeof(struct nsys_netout_jrecv_system_state));

  return ret;
}

/****************************************************************/
/*       determines client IP number, returns NULL on error     */
/****************************************************************/

char * nsys_find_clientip(char * ip)
     
{
  char buf[64*sizeof(struct ifreq)];
  struct ifconf ifc;
  struct ifreq * ifr;
  int s, n, incr;

  /* open a socket, get list of addresses */
  
  if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    return NULL;
  
  ifc.ifc_len = sizeof(buf);
  ifc.ifc_buf = (caddr_t) buf;
  
  if (ioctl(s, SIOCGIFCONF, &ifc) < 0)
    {
      close(s);
      return NULL;
    }

  /* look through addresses, return a useful one    */
  /* thanks to Jens-Uwe Mager for BSD 4.4 updates   */
  /* found his code snippet on comp.unix.programmer */

  ifr = ifc.ifc_req;

  for (n = 0; n < ifc.ifc_len; ifr = (struct ifreq *)(incr + (char *)ifr))
    {
      incr = sizeof(struct ifreq);

#if NSYS_IFCONF_VARLEN

      incr -= sizeof(struct sockaddr);
      incr += ((sizeof(struct sockaddr) > ifr->ifr_addr.sa_len) ?
	       sizeof(struct sockaddr) : ifr->ifr_addr.sa_len);

#endif

      n += incr;

      if (ifr->ifr_addr.sa_family != AF_INET)
	continue;
      
      if (ioctl(s, SIOCGIFFLAGS, (char *) ifr) < 0)
	continue;
      
      if ((ifr->ifr_flags & IFF_UP) == 0)
	continue;

      if (ifr->ifr_flags & IFF_LOOPBACK)
	continue;

      if ((ifr->ifr_flags & (IFF_BROADCAST | IFF_POINTOPOINT)) == 0)
	continue;

      if (ioctl(s, SIOCGIFADDR, (char *) ifr) < 0)
	continue;

      close(s);
      return strcpy(ip, 
	     inet_ntoa(((struct sockaddr_in *)(&(ifr->ifr_addr)))->sin_addr));

    }

  close(s);
  return NULL;
}


/* end Network library -- RTP functions */
