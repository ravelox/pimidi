
/*
#    Sfront, a SAOL to C translator    
#    This file: Network library -- SIP functions
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
/*                 high-level SIP functions                     */
/*______________________________________________________________*/
 
/****************************************************************/
/*        initialize SIP address and INVITE packet              */
/****************************************************************/

int nsys_initsip(void)

{

  /************************/
  /* create SIP addresses */
  /************************/

  if ((nsys_netin_sipaddr(nsys_rtp_fd, NSYS_SIP_IP,
			  NSYS_SIP_RTP_PORT) != NSYS_DONE) ||
      (nsys_netin_sipaddr(nsys_rtcp_fd, NSYS_SIP_IP,
			  NSYS_SIP_RTCP_PORT) != NSYS_DONE))
    return NSYS_ERROR;

  nsys_rtp_cseq = nsys_rtcp_cseq = 1;

  /***********************************/
  /* create SIP packets and send off */
  /***********************************/
  
  nsys_createsip(nsys_rtp_fd, "INVITE", nsys_rtp_invite, NULL, NULL); 
  nsys_sendsip(nsys_rtp_fd, NULL, nsys_rtp_invite);

  nsys_createsip(nsys_rtcp_fd, "INVITE", nsys_rtcp_invite, NULL, NULL);
  nsys_sendsip(nsys_rtcp_fd, NULL, nsys_rtcp_invite);

  nsys_rtp_sipretry = nsys_rtcp_sipretry = 1;

  nsys_behind_nat = 0;

  nsys_createsip(nsys_rtp_fd,  "INFO", nsys_rtp_info,  NULL, NULL); 
  nsys_createsip(nsys_rtcp_fd, "INFO", nsys_rtcp_info, NULL, NULL); 

  return NSYS_DONE;
}

/****************************************************************/
/*                 handle a SIP reply packet                    */
/****************************************************************/

void nsys_netin_reply(int fd, struct sockaddr_in * addr, 
		      unsigned char * packet, unsigned short status)

{
  unsigned char ack[NSYS_UDPMAXSIZE+1];
  char natline[NSYS_SDPNATSIZE];
  char nonce[NSYS_BASE64_LENGTH];
  unsigned long callid, cseq;
  char * invite;

  /***********************/
  /* SIP validity checks */
  /***********************/

  if (nsys_netin_siporigin(fd, addr) == NSYS_ERROR)
    return;

  if (nsys_netin_replyparse(fd, packet, nonce, natline, &callid, &cseq)
      == NSYS_ERROR)
    return;

  /*************************************/
  /* ignore informational status codes */
  /*************************************/

  if ((status >= 100) && (status < 200))
    return;

  /***************************/
  /* all others receive acks */
  /***************************/

  nsys_netin_ack(fd, ack, callid, cseq);
  nsys_sendsip(fd, addr, ack);

  /********************************/
  /* take no action if unexpected */
  /********************************/

  if ((fd == nsys_rtp_fd) && ((nsys_rtcp_ex & NSYS_RTCPEX_RTPSIP) == 0))
    return;
      
  if ((fd == nsys_rtcp_fd) && ((nsys_rtcp_ex & NSYS_RTCPEX_RTCPSIP) == 0))
    return;

  /*************************/
  /* 200 OK --> clear flag */
  /*************************/

  if ((status >= 200) && (status < 300))
    {  
      if (fd == nsys_rtp_fd)
	{
	  nsys_rtcp_ex &= ~(NSYS_RTCPEX_RTPSIP | NSYS_RTCPEX_RTPNEXT);
	  nsys_rtp_authretry = 0;
	  
	  if (nsys_rtcp_sipretry)
	    nsys_warning(NSYS_WARN_STANDARD, 
			 "SIP server contact successful, awaiting client SIP");
	  nsys_rtp_sipretry = 0;
	}
      else
	{
	  nsys_rtcp_ex &= ~(NSYS_RTCPEX_RTCPSIP | NSYS_RTCPEX_RTCPNEXT);
	  nsys_rtcp_authretry = 0;

	  if (nsys_rtp_sipretry)
	    nsys_warning(NSYS_WARN_STANDARD, 
			 "SIP server contact successful, awaiting client SIP");

	  nsys_rtcp_sipretry = 0;
	}
      return;
    }

  /************************/
  /* 401 -- resend INVITE */
  /************************/

  if ((status == 401) && (nonce[0]))
    {
      if (((fd == nsys_rtp_fd) && 
	   (nsys_rtp_authretry++ > NSYS_SIP_AUTHRETRYMAX)) || 
	  ((fd == nsys_rtcp_fd) && 
	   (nsys_rtcp_authretry++ > NSYS_SIP_AUTHRETRYMAX)))
	{
	  nsys_terminate_error("Initial SIP server authentication failure");
	  return;
	}

      if (fd == nsys_rtp_fd)
	{
	  nsys_rtp_cseq++;
	  invite = (char *) nsys_rtp_invite;

	  nsys_rtcp_ex |=  NSYS_RTCPEX_RTPSIP;
	  
	  if (nsys_nexttime < (time(NULL) + NSYS_RTCPTIME_SKIP))
	    nsys_rtcp_ex |= NSYS_RTCPEX_RTPNEXT;
	  else
	    nsys_rtcp_ex &= ~NSYS_RTCPEX_RTPNEXT;

	  nsys_createsip(nsys_rtp_fd, "INFO", nsys_rtp_info,  NULL, NULL); 
	}
      else
	{
	  nsys_rtcp_cseq++;
	  invite = (char *) nsys_rtcp_invite;

	  nsys_rtcp_ex |=  NSYS_RTCPEX_RTCPSIP;
	  
	  if (nsys_nexttime < (time(NULL) + NSYS_RTCPTIME_SKIP))
	    nsys_rtcp_ex |= NSYS_RTCPEX_RTCPNEXT;
	  else
	    nsys_rtcp_ex &= ~NSYS_RTCPEX_RTCPNEXT;

	  nsys_createsip(nsys_rtcp_fd, "INFO", nsys_rtcp_info, NULL, NULL); 
	}

      nsys_createsip(fd, "INVITE", (unsigned char *) invite, 
		     natline[0] ? natline : NULL, nonce[0] ? nonce : NULL);
      nsys_sendsip(fd, NULL, (unsigned char *) invite);

      return;
    }

  /******************************/
  /* 300 series --> do redirect */
  /******************************/

  if ((status >= 300) && (status < 400))
    {
      nsys_netin_redirect(fd, packet, status);
      return;
    }

  /*******************************/
  /* 400 series --> buggy client */
  /*******************************/

  if ((status >= 400) && (status < 500))
    {
      nsys_warning(NSYS_WARN_STANDARD, APPNAME "/" APPVERSION 
		   " is too out-of-date to function with SIP server");

      nsys_warning(NSYS_WARN_STANDARD, 
		   "Download new version at:" 
		   " http://www.cs.berkeley.edu/~lazzaro/sa/index.html");

      nsys_rtcp_ex &= ~(NSYS_RTCPEX_RTPSIP | NSYS_RTCPEX_RTCPSIP | 
			NSYS_RTCPEX_RTPNEXT | NSYS_RTCPEX_RTCPNEXT);

      nsys_graceful_exit = 1;
      return;
    }

  /**********************************/
  /* 500/600 series --> server down */
  /**********************************/

  if ((status >= 500) && (status < 700))
    {
      nsys_warning(NSYS_WARN_STANDARD, 
		   "SIP server is overloaded, refusing new sessions");
      nsys_warning(NSYS_WARN_STANDARD, 
		   "Please try again later. Ending session");

      nsys_rtcp_ex &= ~(NSYS_RTCPEX_RTPSIP | NSYS_RTCPEX_RTCPSIP | 
			NSYS_RTCPEX_RTPNEXT | NSYS_RTCPEX_RTCPNEXT);

      nsys_graceful_exit = 1;
      return;
    }

}


/****************************************************************/
/*                 handle a SIP INVITE packet                   */
/****************************************************************/

void nsys_netin_invite(int fd, struct sockaddr_in * addr, 
		       unsigned char * packet)

{  
  struct sockaddr_in ** aptr;
  struct nsys_source * sptr;
  char line[NSYS_UDPMAXSIZE+1];
  char media[NSYS_UDPMAXSIZE+1];
  char sname[NSYS_BASE64_LENGTH];
  char tool[32] = "unknown";
  struct nsys_payinfo * marray;
  int mlen = 0;
  char cname[32];
  char dnsname[64];
  char ip[16], nat_ip[16];
  unsigned long ssrc, sdp_time, now;
  unsigned short port, nat_port, ptype;
  int state, len;
  int has_nat, has_session, has_time, has_ssrc; 
  int has_dnsname, has_port, has_ip, has_map;

  /*****************************************************************/
  /* do weak and strong security checks, leave function if failure */
  /*****************************************************************/

  if (nsys_netin_siporigin(fd, addr) == NSYS_ERROR)
    return;

  if ((packet = nsys_netin_readmethod(fd, addr, packet)) == NULL)
    return;

  /*************/
  /* parse SDP */
  /*************/

  has_nat = has_session = has_time = has_ssrc = 0;
  has_dnsname = has_port = has_ip = has_map = 0;
  state = NSYS_NETIN_SDPSTATE;

  while (state == NSYS_NETIN_SDPSTATE)
    {
      len = sscanf((char *) packet, "%1023[^\n]\n", line);
      if (len && (len != EOF))
	{
	  switch (line[0]) {
	  case 'v':
	    break;
	  case 's':
	    if ((sscanf(line, "s = %24s", sname) != 1) || 
		(strcmp((char *) nsys_session_base64, sname)))
	      state = NSYS_NETIN_ERRSTATE;
	    else
	      has_session = 1;
	    break;
	  case 't':
	    if (sscanf(line, "t = %lu", &sdp_time) == 1)
	      {
		now = time(NULL);
		sdp_time -= 2208988800UL;
		has_time = 1;

		if (!nsys_msession || nsys_msessionmirror)
		  {
		    if ((sdp_time > now) && 
			((sdp_time - now) > NSYS_MAXLATETIME))
		      state = NSYS_NETIN_ERRSTATE;
		    if ((sdp_time < now) && 
			((now-sdp_time) > NSYS_MAXSSESIONTIME))
		      state = NSYS_NETIN_ERRSTATE;
		  }
	      }
	    break;
	  case 'o':
	    if (sscanf(line, "o = %31s %lu %*u IN IP4 %63s", 
		       cname, &ssrc, dnsname) != 3)
	      state = NSYS_NETIN_ERRSTATE;
	    else
	      has_dnsname = has_ssrc = 1;
	    break;
	  case 'm':
	    if (sscanf(line, "m = audio %hu RTP/AVP %[0-9 ]", &port, media) != 2)
	      state = NSYS_NETIN_ERRSTATE;
	    else
	      {
		has_port = 1;
		if ((mlen = nsys_netin_make_marray(&marray, media)) < 1)
		  state = NSYS_NETIN_ERRSTATE;
	      }
	    break;
	  case 'c':
	    if ((sscanf(line, "c = IN IP4 %15s", ip) != 1) || 
		!(strcmp(ip, "127.0.0.1") && 
		  strcmp(ip, "0.0.0.0") &&
		  strcmp(ip, "255.255.255.255")))
	      state = NSYS_NETIN_ERRSTATE;
	    else
	      has_ip = 1;
	    break;
	  case 'a':
	    if (sscanf(line, "a = nat : %15s %hu", nat_ip, &nat_port) == 2)
	      {
		has_nat = strcmp(nat_ip, "127.0.0.1") && 
		  strcmp(nat_ip, "0.0.0.0") &&
		  strcmp(nat_ip, "255.255.255.255");
		break;
	      }
	    if (sscanf(line, "a = tool : %31[^\n]", tool) == 1)
	      break;
	    if (sscanf(line, "a = rtpmap : %hu", &ptype) == 1)
	      {
		if ((!mlen) || (!nsys_netin_set_marray(line, marray, mlen)))
		  state = NSYS_NETIN_ERRSTATE;
		else
		  has_map = 1;
		break;
	      }
	    break;
	  }
	  packet += strlen(line) + 1;
	}
      else
	state = NSYS_NETIN_EOFSTATE;
    }

  /****************************************/
  /* miscellaneous security/syntax checks */
  /****************************************/

  if (!(has_session && has_time && has_ssrc && has_dnsname
	&& has_port && has_ip && has_map))
    state = NSYS_NETIN_ERRSTATE;

  if ((strcmp(ip, nsys_clientip) == 0) && (strcmp(cname, nsys_username) == 0)
      && (port == nsys_rtp_port) && (nsys_myssrc == ssrc))
    state = NSYS_NETIN_ERRSTATE;

  if (nsys_msession && !nsys_msessionmirror && 
      ((strcmp(nsys_sip_rtp_ip, ip) && (fd == nsys_rtp_fd)) || 
       (strcmp(nsys_sip_rtcp_ip, ip) && (fd == nsys_rtcp_fd)) || has_nat ))
    state = NSYS_NETIN_ERRSTATE;

  if (state == NSYS_NETIN_EOFSTATE)
    {  
      sptr = nsys_ssrc[ssrc & NSYS_HASHMASK];
      
      while (sptr && (sptr->ssrc != ssrc))
	sptr = sptr->xtra;
      
      if ((sptr == NULL) && nsys_netin_noreplay(ip, port, sdp_time) &&
	  nsys_netin_payvalid(marray, mlen, fd) &&
	  (sptr = nsys_netin_addsrc(fd, ssrc, ip, port)))
	{
	  
	  sptr->siptime = sdp_time;
	  nsys_netin_payset(sptr, marray, mlen);
	  sptr->last_hiseq_ext = nsys_netout_jsend_checkpoint_seqnum;

	  if (nsys_msessionmirror)
	    nsys_rtcp_ex &= ~NSYS_RTCPEX_SRCDUPL;
	  
	  sptr->cname = calloc(strlen(cname) + strlen(dnsname) + 8,
			       sizeof(char));
	  sprintf(sptr->cname, "%s@%s:%hu", cname, dnsname, port);
	  nsys_status(sptr, "SIP INVITE accepted from");
	}

      if (sptr && has_nat)
	{
	  if (fd == nsys_rtp_fd)
	    aptr = &(sptr->alt_rtp_addr);
	  else
	    aptr = &(sptr->alt_rtcp_addr);
	  
	  if (*aptr == NULL)
	    {  
	      *aptr = calloc(1, sizeof(struct sockaddr_in));
	      (*aptr)->sin_family = AF_INET;   
	    }
	  (*aptr)->sin_port = htons(nat_port);
	  (*aptr)->sin_addr.s_addr = inet_addr(nat_ip);
	}
    }

  if (state == NSYS_NETIN_ERRSTATE)
    {		
      sprintf(line, "Discarding a SIP INVITE: probably a client/server bug"); 
      nsys_warning(NSYS_WARN_UNUSUAL, line);
    }

}

/****************************************************************/
/*               close down SIP server connection               */
/****************************************************************/

void nsys_sip_shutdown(void)

{
  unsigned char packet[NSYS_UDPMAXSIZE+1];
  fd_set set;
  int rtp_flags, rtcp_flags;
  struct timeval timeout, deadline;
  int rtp_send = 1;
  int rtcp_send = 1;
  int waitcycles = 0;

  /**********************/
  /* create BYE packets */
  /**********************/

  nsys_rtp_cseq++;
  nsys_rtcp_cseq++;

  nsys_createsip(nsys_rtp_fd, "BYE", nsys_rtp_invite, NULL, NULL);
  nsys_createsip(nsys_rtcp_fd, "BYE", nsys_rtcp_invite, NULL, NULL);

  /*************************/
  /* make sockets blocking */
  /*************************/

  rtp_flags = fcntl(nsys_rtp_fd, F_GETFL);
  rtcp_flags = fcntl(nsys_rtcp_fd, F_GETFL);

  fcntl(nsys_rtp_fd,  F_SETFL, (~O_NONBLOCK) & rtp_flags );
  fcntl(nsys_rtcp_fd, F_SETFL, (~O_NONBLOCK) & rtcp_flags);

  /***********************************/
  /* do three sends before giving up */
  /***********************************/

  while ((waitcycles++ < 3) && (rtp_send || rtcp_send))
    {

      if (rtp_send)
	nsys_sendsip(nsys_rtp_fd, NULL, nsys_rtp_invite);
      if (rtcp_send)
	nsys_sendsip(nsys_rtcp_fd, NULL, nsys_rtcp_invite);

      /* one second between resends */

      gettimeofday(&deadline, NULL);
      deadline.tv_sec++;
      
      do {

	FD_ZERO(&set);
	FD_SET(nsys_rtp_fd, &set);
	FD_SET(nsys_rtcp_fd, &set);

	timeout.tv_sec = 0;
	timeout.tv_usec = 200000;
 
	select(nsys_max_fd, &set, NULL, NULL, &timeout);

	if (FD_ISSET(nsys_rtp_fd, &set) &&
	    (recv(nsys_rtp_fd, packet, NSYS_UDPMAXSIZE, 0) > 0) &&
	    (packet[NSYS_RTPLOC_BYTE1] == 'S'))
	  rtp_send = 0;

	if (FD_ISSET(nsys_rtcp_fd, &set) &&
	    (recv(nsys_rtcp_fd, packet, NSYS_UDPMAXSIZE, 0) > 0) &&
	    (packet[NSYS_RTPLOC_BYTE1] == 'S'))
	  rtcp_send = 0;

	gettimeofday(&timeout, NULL);

      } while ((rtcp_send | rtp_send) && 
	       ((timeout.tv_sec < deadline.tv_sec) ||
		((timeout.tv_sec == deadline.tv_sec) &&
		 (timeout.tv_usec < deadline.tv_usec))));

    }

  /*************************/
  /* restore socket status */
  /*************************/

  fcntl(nsys_rtp_fd,  F_SETFL, rtp_flags);
  fcntl(nsys_rtcp_fd, F_SETFL, rtcp_flags);

}  


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                  low-level SIP functions                     */
/*______________________________________________________________*/


/****************************************************************/
/*                 create a SIP method packet                   */
/****************************************************************/

void nsys_createsip(int fd, char * method, unsigned char * sip,
		    char * natline, char * nonce) 

{
  unsigned char base64_digest[NSYS_BASE64_LENGTH];
  unsigned char binary_digest[NSYS_MD5_LENGTH];
  char media[4*NSYS_RTP_PAYSIZE + 1];
  char nline[16+NSYS_BASE64_LENGTH];
  int i, j, len, body, dpoint;


  /**********************/
  /* create SIP headers */
  /**********************/

  body = snprintf((char *) sip, NSYS_UDPMAXSIZE+1,
		  "%s sip:%s@%s:%hu SIP/2.0\n"     /* INVITE or BYE method */
		  "i: %lu\n"                       /* Call-ID              */
		  "v: SIP/2.0/UDP %s:%hu\n"        /* Via                  */
		  "%s"                             /* Content-Type         */
		  "f: sip:%s@%s\n"                 /* From                 */
		  "t: sip:%s@%s:%hu\n"             /* To                   */
		  "CSeq: %lu\n"                    /* CSeq                 */
                  "User-Agent: %s/%s\n",           /* User-Agent           */
		  
		  /* SIP URL */
		  
		  method, nsys_session_base64, 
		  (fd == nsys_rtp_fd) ? nsys_sip_rtp_ip : nsys_sip_rtcp_ip,
		  (fd == nsys_rtp_fd) ? nsys_sip_rtp_port : nsys_sip_rtcp_port,
		  
		  /* Call-ID */
		  
		  nsys_myssrc + (fd == nsys_rtcp_fd),
		  
		  /* Via */

		  nsys_clientip,
		  (fd == nsys_rtp_fd) ? nsys_rtp_port : nsys_rtcp_port,
		  
		  /* Content-Type */

		  (method[2] == 'V') ? "c: application/sdp\n" : "",

		  /* From */

		  nsys_username, nsys_clientname,
		  
		  /* To */
		  
		  nsys_session_base64, 
		  (fd == nsys_rtp_fd) ? nsys_sip_rtp_ip : nsys_sip_rtcp_ip,
		  (fd == nsys_rtp_fd) ? nsys_sip_rtp_port : nsys_sip_rtcp_port,
		  
		  /* CSeq */

		  (fd == nsys_rtp_fd) ? nsys_rtp_cseq : nsys_rtcp_cseq,

		  /* User-Agent */

		  APPNAME, APPVERSION

		  );

  if (body > NSYS_UDPMAXSIZE)
    NSYS_ERROR_TERMINATE("SIP INVITE packet creation error [1]");

  if (method[2] != 'V')               /* BYE/INFO currently have no body */
    return; 

  /*********************/
  /* add Authorization */
  /*********************/

  if (nonce)
    {
      body += snprintf((char *) &(sip[body]), NSYS_UDPMAXSIZE+1-body,
		       "Authorization: SignSDP nonce=\"%s\", "
		       "digest=\"012345678901234567890A==\"\n",
		       nonce);
      sprintf(nline, "a=nonce:\"%s\"\n", nonce);
      dpoint = body - 26;
    }
  else
    dpoint = 0;

  if (body > NSYS_UDPMAXSIZE)
    NSYS_ERROR_TERMINATE("SIP INVITE packet creation error [2]");

  /***********/
  /* add SDP */
  /***********/

  for (j = i = 0; i < NSYS_RTP_PAYSIZE; i++)
    j += sprintf(&(media[j]), "%hhu ", nsys_payload_types[i].ptype);

  len = body + snprintf((char *) &(sip[body]), NSYS_UDPMAXSIZE+1-body,
		 "\n"
		 "v=0\n"
		 "o=%s %lu 0 IN IP4 %s\n"          /* CNAME, SSRC, IP    */
		 "s=%s\n"                          /* sessionname        */
		 "a=tool:%s %s\n"                  /* SDP creator        */
		 "t=%lu 0\n"                       /* current time       */
		 "m=audio %i RTP/AVP %s\n"         /* media              */
		 "c=IN IP4 %s\n"                   /* ipnumber           */
		 "%s"                              /* nonce, if needed   */
 		 "%s",                             /* nat, if needed     */

		 /* SDP originator (o=) */

		 nsys_username, nsys_myssrc, nsys_clientname,

		 /* SDP session (s=) */

		 nsys_session_base64,

		 /* SDP tool (a=tool) */

		 APPNAME, APPVERSION,

 		 /* NTP time */

		 ((unsigned long) time(NULL)) + NSYS_SIP_UNIXTONTP,

		 /* SDP media (m=) */

		 nsys_rtp_port, media,

		 /* SDP connection (c=) */

		 nsys_clientip,
			
		 /* nonce line */

		 nonce ? nline : "",

		 /* nat line */

		 natline ? natline : ""

		 );

  if (len > NSYS_UDPMAXSIZE)
    NSYS_ERROR_TERMINATE("SIP INVITE packet creation error [3]");

  for (i = 0; i < NSYS_RTP_PAYSIZE; i++)
    {
      /**********************/
      /* append rtpmap line */
      /**********************/

      len += snprintf((char *) &(sip[len]), NSYS_UDPMAXSIZE+1 - len, 
		      "a=rtpmap:%hhu %s/%i\n", 
		      nsys_payload_types[i].ptype, 
		      nsys_payload_types[i].name, 
		      nsys_payload_types[i].srate);

      if ((len > NSYS_UDPMAXSIZE))
	NSYS_ERROR_TERMINATE("SIP INVITE packet creation error [4]");

      /*********************/
      /* append fmtp lines */
      /*********************/

      switch (nsys_payload_types[i].pindex) {
      case NSYS_MPEG4_PINDEX:
	len += snprintf((char *) &(sip[len]), NSYS_UDPMAXSIZE+1 - len,

	"a=fmtp:%hhu streamtype=5; mode=rtp-midi; profile-level-id=12;"
	" config=\"\"; render=synthetic; rinit=audio/asc;"
	" tsmode=buffer; octpos=last; mperiod=%lu;"
	" rtp_ptime=0; rtp_maxptime=0; cm_unused=DEFMQVX;%s"
	" guardtime=%lu\n",

	nsys_payload_types[i].ptype,

	/* tsmode line */

	(unsigned long) ACYCLE,

	/* rtp_ptime line */

	(nsys_feclevel != NSYS_SM_FEC_NONE) 
	? "" : " j_sec=none;",

	/* guardtime line */

	(unsigned long)(NSYS_SM_GUARD_MAXTIME*ARATE + 0.5F)

	);

	if ((len > NSYS_UDPMAXSIZE))
	  NSYS_ERROR_TERMINATE("SIP INVITE packet creation error [5]");
	break;
      default:
	break;
      }

    }

  /***************************************/
  /* compute digest, place in SIP header */
  /***************************************/

  if (nonce)
    {    
      nsys_hmac_md5(&(sip[body + 1]), len - body - 1, 
		    nsys_keydigest, binary_digest);
      nsys_digest_base64(base64_digest, binary_digest);
      for (i = 0; i < NSYS_BASE64_LENGTH - 1; i++)
	sip[dpoint + i] = base64_digest[i];
    }
}

/****************************************************************/
/*             reads an INVITE, generates an OK                 */
/****************************************************************/

unsigned char * nsys_netin_readmethod(int fd, struct sockaddr_in * addr, 
				      unsigned char * packet) 

{  
  unsigned char local_base64[NSYS_BASE64_LENGTH];
  unsigned char reply[NSYS_UDPMAXSIZE+1];
  unsigned char line[NSYS_UDPMAXSIZE+1];
  unsigned char nonce[NSYS_BASE64_LENGTH];
  unsigned char digest[NSYS_BASE64_LENGTH];
  unsigned char local_binary[NSYS_MD5_LENGTH];
  unsigned char text[14];
  unsigned char * ret;
  int found_invite, found_callid, found_reply, found_auth;
  int num, len, first, overflow;
  unsigned long callid, inreplyto;

  first = 1;
  overflow = len = found_invite = found_callid = found_reply = found_auth = 0;
  nonce[0] = digest[0] = '\0';

  /***********************************************************/
  /* extract Call-ID, In-Reply-To, Nonce and Digest from SIP */
  /***********************************************************/

  while ((num = sscanf((char *) packet, "%[^\n]\n", line)) && (num != EOF))
    {
      if (first)
	{
	  first = 0;
	  found_invite = (line == ((unsigned char *)strstr((char *)line,"INVITE")));

	  if ((len += strlen("SIP/2.0 200\n")) < NSYS_UDPMAXSIZE+1)
	    strcpy((char *) reply, "SIP/2.0 200\n");
	  else
	    overflow = 1;
	}
      else
	{
	  if ((len += strlen((char *) line) + 1) < NSYS_UDPMAXSIZE+1)
	    {
	      strcat((char *) reply, (char *) line);
	      strcat((char *) reply, "\n");
	    }
	  else
	    overflow = 1;

	  if ((line[0] == 'i') || (line[0] == 'C'))
	    found_callid |= ((sscanf((char *) line, "i : %lu", &callid) == 1) || 
			     (sscanf((char *) line, "Call-ID : %lu", &callid) == 1));
	  if (line[0] == 'I')
	    found_reply |= (sscanf((char *) line, "In-Reply-To : %lu",&inreplyto) == 1);

	  if (line[0] == 'A')
	    found_auth |= ((sscanf((char *) line, "Authorization : SignSDP " 
				   "nonce = \"%24[^\"]\" , "
				   "digest = \"%24[^\"]\" , ",
				   nonce, digest) == 2) || 
			   (sscanf((char *) line, "Authorization : SignSDP " 
				   "digest = \"%24[^\"]\" , "
				   "nonce = \"%24[^\"]\" , ",
				   digest, nonce) == 2));
	}
      packet += strlen((char *) line) + 1;
    }

  ret = (num != EOF) ? packet + 1 : NULL;

  if (found_invite && found_callid && found_reply && 
      (inreplyto == (nsys_myssrc + (fd == nsys_rtcp_fd))))
    {
      memcpy(&(text[0]), &callid, 4);
      memcpy(&(text[4]), &inreplyto, 4);
      memcpy(&(text[8]), &(addr->sin_addr), 4);
      memcpy(&(text[12]), &(addr->sin_port), 2);

      nsys_hmac_md5(text, 14, nsys_keydigest, local_binary);
      nsys_digest_base64(local_base64, local_binary);

      if ((!found_auth) || strcmp((char *) local_base64, (char *) nonce))
	{
	  sprintf((char *) line, "WWW-Authenticate: SignSDP nonce =\"%s\"\n",
		  local_base64);

	  if ((len += strlen((char *)line)) < NSYS_UDPMAXSIZE+1)
	    strcat((char *) reply, (char *) line);
	  else
	    overflow = 1;

	  reply[8] = '4'; reply[10] = '1';  /* 200 -> 401 */
	  ret = NULL;
	}
    }
  else
    ret = NULL;

  if (ret && (nsys_msession == 0))
    {
      nsys_hmac_md5(ret, strlen((char *) ret), nsys_keydigest, local_binary);
      nsys_digest_base64(local_base64, local_binary);
      if (strcmp((char *) digest, (char *) local_base64))
	ret = NULL;
    }

  nsys_sendsip(fd, NULL, reply);

  if (!ret && found_auth)
    nsys_warning(NSYS_WARN_UNUSUAL, 
		 "Discarding a SIP INVITE: probably a client/server bug");

  if (overflow)
    nsys_warning(NSYS_WARN_UNUSUAL, 
		 "Send a truncated 200 or 401: probably a client/server bug");

  return ret;
}


/****************************************************************/
/*           reads a final response, generate an ACK            */
/****************************************************************/

void nsys_netin_ack(int fd, unsigned char * reply, 
		    unsigned long callid, unsigned long cseq)

{
  int len;

  /****************************/
  /* create top of ACK header */
  /****************************/

  len = snprintf((char *) reply, NSYS_UDPMAXSIZE+1, 
	  
		 "ACK sip:%s@%s:%hu SIP/2.0\n"
		 "v: SIP/2.0/UDP %s:%hu\n"        /* Via                  */
		 "f: sip:%s@%s\n"                 /* From                 */
		 "t: sip:%s@%s:%hu\n"             /* To                   */
		 "i: %lu\n"                       /* Call-ID              */	
		 "CSeq: %lu\n" 	                  /* CSeq                 */
		 "User-Agent: %s/%s\n",           /* User-Agent           */

		 /* method line */
		 
		 nsys_session_base64, 
		 (fd == nsys_rtp_fd) ? nsys_sip_rtp_ip : nsys_sip_rtcp_ip,
		 (fd == nsys_rtp_fd) ? nsys_sip_rtp_port : nsys_sip_rtcp_port,
		 
		 /* Via */
		 
		 nsys_clientip,
		 (fd == nsys_rtp_fd) ? nsys_rtp_port : nsys_rtcp_port,
		 
		 /* From */
		 
		 nsys_username, nsys_clientname,
		 
		 /* To */
		 
		 nsys_session_base64, 
		 (fd == nsys_rtp_fd) ? nsys_sip_rtp_ip : nsys_sip_rtcp_ip,
		 (fd == nsys_rtp_fd) ? nsys_sip_rtp_port : nsys_sip_rtcp_port,
		 
		 /* Call-ID, CSeq */
		 
		 callid, cseq,

		 /* User-Agent */
		 
		 APPNAME, APPVERSION

		 );

  if (len > NSYS_UDPMAXSIZE)
    NSYS_ERROR_TERMINATE("SIP REPLY packet creation error [1]");

}

/****************************************************************/
/*      read a SIP/2.0 reply packet, parse useful fields        */
/****************************************************************/

int nsys_netin_replyparse(int fd, unsigned char * packet, 
			  char * nonce, char * natline, 
			  unsigned long * callid, unsigned long * cseq)

{
  char line[NSYS_UDPMAXSIZE+1];
  unsigned short nat_port;
  unsigned char nat_ip[16];
  int found_callid = 0;
  int found_cseq = 0;
  int found_auth = 0;

  natline[0] = '\0';
  nonce[0] = '\0';

  while (sscanf((char *) packet,"%[^\n]\n",line) == 1)
    {
      packet += strlen(line) + 1;

      if ((sscanf(line, "i : %lu", callid) == 1) || 
	  (sscanf(line, "Call-ID : %lu", callid) == 1))
	{
	  if (found_callid || (*callid != (nsys_myssrc + (fd == nsys_rtcp_fd))))
	    return NSYS_ERROR;
	  found_callid = 1;
	}

      if ((sscanf(line, "CSeq : %lu", cseq) == 1))
	{
	  if (found_cseq)
	    return NSYS_ERROR;
	  found_cseq = 1;
	}

     if (sscanf(line, 
		"WWW-Authenticate : SignSDP nonce = \"%24[^\"]\"",
		nonce) == 1)
       {
	 if (found_auth || !nsys_digest_syntaxcheck(nonce))
	   nonce[0] = '\0';
	 else
	   found_auth = 1;
       }
     
     if ((sscanf(line, "v : %*[^;]; received = %15[0-9.] ; rport = %hu ",
		nat_ip, &nat_port) == 2) || 
	 (sscanf(line, "Via : %*[^;]; received = %15[0-9.] ; rport = %hu ",
		 nat_ip, &nat_port) == 2))
       {
	 sprintf(natline, "a=nat:%s %hu\n", nat_ip, nat_port);
	 nsys_behind_nat = 1;
       }
    }

  return ((found_callid && found_cseq) ? NSYS_DONE : NSYS_ERROR);
}

/****************************************************************/
/*      read a SIP/2.0 300-series packet, do redirection        */
/****************************************************************/

void nsys_netin_redirect(int fd, unsigned char * packet, 
			 unsigned short status) 

{
  char line[NSYS_UDPMAXSIZE+1];
  int found = 0;
  char ip[16];
  unsigned short port;
  char * invite;

  /* find first contact address, extract IP and port */

  while (sscanf((char *) packet,"%[^\n]\n",line) == 1)
    {
      packet += strlen(line) + 1;
      if ((found = (sscanf(line,"m:%*[^@]@%15[^:]:%hu", ip, &port) == 2)))
	break;
      if ((found = (sscanf(line,
			   "Contact:%*[^@]@%15[^:]:%hu", ip, &port) == 2)))
	break;
    }

  /* handle incomplete redirect packets */

  if ((!found) || (nsys_netin_sipaddr(fd, ip, port) != NSYS_DONE))
    {      
      nsys_terminate_error("SIP redirection to an unavailable/unknown server");

      nsys_rtcp_ex &= ~(NSYS_RTCPEX_RTPSIP | NSYS_RTCPEX_RTCPSIP | 
			NSYS_RTCPEX_RTPNEXT | NSYS_RTCPEX_RTCPNEXT);
      return;
    }

  /* send INVITE to new server */

  nsys_stderr_size += 
    fprintf(stderr, "Redirecting %s INVITE to %s:%hu, please stand by ...\n",
	    fd == nsys_rtp_fd ? "RTP" : "RTCP", ip, port);
  fflush(stderr);

  if (fd == nsys_rtp_fd)
    {
      invite = (char *) nsys_rtp_invite;

      nsys_rtcp_ex &= ~NSYS_RTCPEX_RTPNEXT;
      nsys_rtcp_ex |=  NSYS_RTCPEX_RTPSIP;
      nsys_rtp_authretry = 0;
      nsys_createsip(nsys_rtp_fd,  "INFO", nsys_rtp_info,  NULL, NULL); 
    }
  else
    {
      invite = (char *) nsys_rtcp_invite;

      nsys_rtcp_ex &= ~NSYS_RTCPEX_RTCPNEXT;
      nsys_rtcp_ex |=  NSYS_RTCPEX_RTCPSIP;
      nsys_rtcp_authretry = 0;
      nsys_createsip(nsys_rtcp_fd, "INFO", nsys_rtcp_info, NULL, NULL); 
    }

  nsys_createsip(fd, "INVITE", (unsigned char *) invite, NULL, NULL);
  nsys_sendsip(fd, NULL, (unsigned char *) invite);

}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                   utility SIP functions                      */
/*______________________________________________________________*/

/****************************************************************/
/*             send a formatted SIP packet                      */
/****************************************************************/

void nsys_sendsip(int fd, struct sockaddr_in * addr, unsigned char * sip) 

{
  int retry = 0;

  if (addr == NULL)
    addr = (fd == nsys_rtp_fd) ? &nsys_sip_rtp_addr : &nsys_sip_rtcp_addr;

  while (sendto(fd, sip, strlen((char *) sip) + 1, 0, (struct sockaddr *) addr,
		sizeof(struct sockaddr)) == -1)
    {
      if (errno == EAGAIN)
	continue;
      
      if ((errno == EINTR) || (errno == ENOBUFS))
	{
	  if (++retry > NSYS_MAXRETRY)
	    NSYS_ERROR_TERMINATE("Too many I/O retries -- nsys_sendsip");
	  continue;         
	}
      
      NSYS_ERROR_TERMINATE("Error writing Internet socket");
    }
}


/****************************************************************/
/*                   sets a SIP address                         */
/****************************************************************/

int nsys_netin_sipaddr(int fd, char * ip, unsigned short port)

{
  struct sockaddr_in * addr;

  if (fd == nsys_rtp_fd)
    {
      addr = &nsys_sip_rtp_addr;
      strcpy(nsys_sip_rtp_ip, ip);
      nsys_sip_rtp_inet_addr = inet_addr(ip);
      nsys_sip_rtp_port = port;
      nsys_sip_rtp_sin_port = htons(port);
    }
  else
    {
      addr = &nsys_sip_rtcp_addr;
      strcpy(nsys_sip_rtcp_ip, ip);
      nsys_sip_rtcp_inet_addr = inet_addr(ip);
      nsys_sip_rtcp_port = port;
      nsys_sip_rtcp_sin_port = htons(port);
    }

  memset(&(addr->sin_zero), 0, 8);    
  addr->sin_family = AF_INET;   
  addr->sin_port = htons(port);
  if ((addr->sin_addr.s_addr = inet_addr(ip)) == -1)
    NSYS_ERROR_RETURN("Bad format for SIP address");
  
  return NSYS_DONE;
}

/****************************************************************/
/*              checks origin of incoming SIP                   */
/****************************************************************/

int nsys_netin_siporigin(int fd, struct sockaddr_in * addr)

{
  int ret = NSYS_ERROR;
  
  if (fd == nsys_rtp_fd)
    {
      if ((addr->sin_addr.s_addr == nsys_sip_rtp_inet_addr) && 
	  (addr->sin_port == nsys_sip_rtp_sin_port))
	ret = NSYS_DONE;
    }
  else
    {
      if ((addr->sin_addr.s_addr == nsys_sip_rtcp_inet_addr) && 
	  (addr->sin_port == nsys_sip_rtcp_sin_port))
	return ret = NSYS_DONE;
    }

  if (ret == NSYS_ERROR)
    {
      nsys_stderr_size += 
	fprintf(stderr, "Network advisory: SIP from unknown source %s:%hu"
		" rejected\n", inet_ntoa(addr->sin_addr), 
		ntohs(addr->sin_port));
      fflush(stderr);
    }

  return ret;
}

/****************************************************************/
/*              makes an array of media payloads                */
/****************************************************************/

int nsys_netin_make_marray(struct nsys_payinfo ** marray, char * media)

{
  int size, i, len;
  char * eptr, * cptr;

  i = size = 0;
  len = strlen(media);

  while (i < len)
    {
      while ((i < len) && (media[i] == ' '))
	i++;
      size += (i < len);
      while ((i < len) && (media[i] != ' '))
	i++;
    }

  if (!size)
    return size;

 *marray = calloc(size, sizeof(struct nsys_payinfo));
 cptr = media;

 for (i = 0; i < size; i++)
   {
     (*marray)[i].pindex = i;
     (*marray)[i].ptype = (unsigned char)strtoul(cptr, &eptr, 10);
     cptr = eptr;
   }

 return size;

}


/****************************************************************/
/*                 sets media payloads                          */
/****************************************************************/

int nsys_netin_set_marray(char * line, struct nsys_payinfo marray[], int mlen)

{
  unsigned short ptype;  /* sscanf needs a short, not a char */
  char name[32];
  int srate; 
  int i, num;

  num = sscanf(line, "a=rtpmap:%hu %31[^/]/%i", &ptype, name, &srate);

  if ((num != 2) && (num != 3))
    return 0;

  i = 0;
  while (i < mlen)
    {
      if (marray[i].ptype == (unsigned char)ptype)
	break;
      i++;
    }

  if (i == mlen)
    return 0;

  strcpy(marray[i].name, name);

  if (num == 3)
    marray[i].srate = srate;
  else
    marray[i].srate = -1;

  return 1;
}


/****************************************************************/
/*             checks validity of payload ptype                 */
/****************************************************************/

int nsys_netin_payvalid(struct nsys_payinfo marray[], int mlen, 
			int fd) 
			 

{
  int i, j;
  char line[NSYS_UDPMAXSIZE+1];

  for (i = 0; i < mlen; i++)
    for (j = 0; j < NSYS_RTP_PAYSIZE; j++)
      if (!strcasecmp(marray[i].name, nsys_payload_types[j].name))
	{
	  if (nsys_msession || (marray[i].srate == -1) || 
	      (marray[i].srate == nsys_payload_types[j].srate))
	    return 1;

	  if (fd == nsys_rtp_fd)
	    {
	      nsys_warning(NSYS_WARN_STANDARD, 
			   "Rejecting an INVITE due to SAOL srate mismatch");

	      sprintf(line, "(local srate %iHz, remote srate %iHz)", 
		      nsys_payload_types[j].srate, marray[i].srate);

	      nsys_warning(NSYS_WARN_STANDARD, line);
	    }

	  return 0;
	}

  if (fd == nsys_rtp_fd)
    {
      nsys_warning(NSYS_WARN_STANDARD, 
		   "Rejecting an INVITE, no common RTP payloads");
      nsys_warning(NSYS_WARN_STANDARD, 
		   "(probably caused by local or remote out-of-date software)");
      nsys_warning(NSYS_WARN_STANDARD, 
		   "Download new version at:" 
		   " http://www.cs.berkeley.edu/~lazzaro/sa/index.html");
    }
  return 0;
}

/****************************************************************/
/*             sets payload information in source               */
/****************************************************************/

void nsys_netin_payset(struct nsys_source * sptr, struct nsys_payinfo marray[],
		       int mlen) 
			 

{
  int i, j;

  for (i = 0; i < mlen; i++)
    for (j = 0; j < NSYS_RTP_PAYSIZE; j++)
      if (!strcasecmp(marray[i].name, nsys_payload_types[j].name))
	{
	  sptr->ptype = marray[i].ptype;
	  sptr->pindex = nsys_payload_types[j].pindex;
	  if ((sptr->srate = marray[i].srate) < 0)
	    sptr->srate = nsys_payload_types[j].srate;
	  return;
	}
}

/****************************************************************/
/*             checks for a SIP replay attack                  */
/****************************************************************/

int nsys_netin_noreplay(char * ip, unsigned short port, unsigned long sdp_time)

{
  struct nsys_source * sptr = nsys_srcroot;
  struct sockaddr_in ipaddr;
  int ret = 1;

  if (sptr)
    {
      memset(&ipaddr, 0, sizeof(struct sockaddr_in));
      ipaddr.sin_port = htons(port);
      ipaddr.sin_family = AF_INET;  
      ipaddr.sin_addr.s_addr = inet_addr(ip);

      do {
	if ((!memcmp(&ipaddr, sptr->sdp_addr, sizeof(struct sockaddr_in))) &&
	    (sptr->siptime > sdp_time))
	  ret = 0;
      } while ((sptr = sptr->next) != nsys_srcroot);
    }

  return ret;
}

/* end Network library -- SIP functions */


