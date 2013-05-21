
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
/*                 high-level RCTP functions                     */
/*______________________________________________________________*/
 

/****************************************************************/
/*        application-level function to trigger RTCP sends      */
/****************************************************************/

void nsys_netout_rtcptime(void)

{  
  if (nsys_behind_nat && (++nsys_sipinfo_count >= NSYS_SIPINFO_TRIGGER))
    {
      nsys_netout_keepalive();      /* resets nsys_sipinfo_count */
      return;
    }

  nsys_nexttime += NSYS_RTCPTIME_INCR; 

  if (nsys_rtcp_ex && nsys_netout_excheck())
    return;

  nsys_netout_rtcpreport();
}

/****************************************************************/
/*              process received RTCP packet                    */
/****************************************************************/

nsys_source * nsys_netin_rtcp(unsigned char * packet, int len, 
		     struct sockaddr_in * ipaddr)

{
  unsigned long ssrc, new_stamp, hiseq;
  struct nsys_source * sptr = NULL;
  int skip, overflow;
  unsigned short sect_len;
  int first = 0;
  struct timeval now;
  int offset;

  gettimeofday(&now, 0);

  if (NSYS_DISPLAY_RTCP)
    nsys_netin_rtcp_display(packet, len, &now); 

  while (len > 0)
    {
      if (len < NSYS_RTCPLEN_MINIMUM)
	{
	  nsys_warning(NSYS_WARN_UNUSUAL, "RTCP subpacket size subminimal");
	  return sptr;
	}

      /* assume all sub-packets share ssrc, just look at first */

      if (!first)
	{
	  first = 1;
	  ssrc = ntohl(*((unsigned long *)&(packet[NSYS_RTCPLOC_SSRC])));
	  sptr = nsys_ssrc[ssrc & NSYS_HASHMASK];
	  
	  while (sptr && (sptr->ssrc != ssrc))
	    sptr = sptr->xtra;
	  
	  if (!sptr)
	    {
	      nsys_warning(NSYS_WARN_UNUSUAL, "RTCP packet source unknown");
	      return sptr;
	    }
	  
	  if (!(sptr->rtcp_received++))
	    {
	      if (!(sptr->received))
		nsys_status(sptr, "Media (RTCP) flowing from");
	      if (NSYS_LATENOTES_DEBUG == NSYS_LATENOTES_DEBUG_ON)
		printf("%s is mset%i\n", sptr->cname, sptr->mset);
	    }

	  if (((ipaddr->sin_addr.s_addr != sptr->rtcp_addr->sin_addr.s_addr) ||
	      (ipaddr->sin_port != sptr->rtcp_addr->sin_port)) && 
	      (sptr->alt_rtcp_addr == NULL))
	    {
	      nsys_warning(NSYS_WARN_UNUSUAL, 
			   "RTCP packet from unknown IP/port");
	      return NULL;
	    }

	  if (sptr->alt_rtcp_addr)
	    {  
	      if ((ipaddr->sin_addr.s_addr != nsys_sip_rtcp_inet_addr) ||
		  (ipaddr->sin_port != nsys_sip_rtcp_sin_port))
		{
		  /* normal case */

		  memcpy(sptr->rtcp_addr, ipaddr, sizeof(struct sockaddr_in));
		}
	      else
		{
		  /* source-forge trick which may later be implemented */

		  memcpy(sptr->rtcp_addr, &(sptr->alt_rtcp_addr), 
			 sizeof(struct sockaddr_in));
		}

	      free(sptr->alt_rtcp_addr);
	      sptr->alt_rtcp_addr = NULL;
	    }

	  sptr->expire_rtcp = time(NULL) + NSYS_RTCPTIME_TIMEOUT;
	}

      /* process sub-packet */

      memcpy(&(sect_len), &(packet[NSYS_RTCPLOC_LENGTH]), sizeof(short));
      skip = 4 + 4*ntohs(sect_len);
      if ((len -= skip) < 0)
	{      
	  nsys_netin_rtcp_trunc(packet[NSYS_RTCPLOC_PTYPE]);
	  return sptr;
	}

      overflow = offset = 0;

      switch (packet[NSYS_RTCPLOC_PTYPE]) {
      case NSYS_RTCPVAL_SR:
	if (NSYS_RTCPVAL_COUNTMASK & packet[NSYS_RTCPLOC_BYTE1])
	  {
	    offset = NSYS_RTCPLEN_SRHDR + NSYS_RTCPLEN_SENDER;
	    overflow = (skip < NSYS_RTCPLEN_SR);
	  }
	else
	  overflow = (skip < NSYS_RTCPLEN_SREMPTY);
	break;
      case NSYS_RTCPVAL_RR:
	if (NSYS_RTCPVAL_COUNTMASK & packet[NSYS_RTCPLOC_BYTE1])
	  {
	    offset = NSYS_RTCPLEN_RRHDR; 
	    overflow = (skip < NSYS_RTCPLEN_RR);
	  }
	else
	  overflow = (skip < NSYS_RTCPLEN_RREMPTY);
	break;
      default:
	break;
      }
      
      if (overflow)
	{      
	  nsys_netin_rtcp_trunc(packet[NSYS_RTCPLOC_PTYPE]);
	  return sptr;
	}

      if (offset)
	{
	  memcpy(&hiseq, &(packet[offset+NSYS_RTCPLOC_RR_HISEQ]),sizeof(long));
	  hiseq = ntohl(hiseq);

	  if ((hiseq >= sptr->last_hiseq_rec) || 
	      (sptr->last_hiseq_rec > 
	       ((unsigned long) (NSYS_RTPSEQ_EXMASK | NSYS_RTPSEQ_HILIMIT))))
	    {
	      sptr->last_hiseq_ext = sptr->last_hiseq_rec = hiseq;
	    }
	  else
	    {
	      nsys_warning(NSYS_WARN_UNUSUAL, "Possible RTCP replay attack");
	      return NULL; 
	    }
	}

      switch (packet[NSYS_RTCPLOC_PTYPE]) {
      case NSYS_RTCPVAL_SR:

	/* create LSR value from NTP */

	sptr->lsr[0] =  packet[NSYS_RTCPLOC_SR_NTPMSB + 2];
	sptr->lsr[1] =  packet[NSYS_RTCPLOC_SR_NTPMSB + 3];
	sptr->lsr[2] =  packet[NSYS_RTCPLOC_SR_NTPLSB];
	sptr->lsr[3] =  packet[NSYS_RTCPLOC_SR_NTPLSB + 1];

	sptr->arrival.tv_sec = now.tv_sec;
	sptr->arrival.tv_usec = now.tv_usec;

	memcpy(&new_stamp, &(packet[NSYS_RTCPLOC_SR_TSTAMP]), sizeof(long));
	new_stamp = ntohl(new_stamp);

	if (sptr->tm_margin)
	  {
	    if (((int)(new_stamp - nsys_netout_tstamp - sptr->tm_convert)) > 0)
	      {
		sptr->tm_convert = (new_stamp - nsys_netout_tstamp
				    - sptr->tm_margin);
		if (sptr->tm_lateflag)
		  sptr->tm_lateflag = 0;
	      }
	    else
	      nsys_late_windowcheck(sptr, new_stamp);
	  }
	else
	  {
	    sptr->tm_margin = nsys_lateplay ? 0 : ARATE*nsys_latetime;
	    sptr->tm_convert = (new_stamp - nsys_netout_tstamp -
				sptr->tm_margin);
	    if (NSYS_LATENOTES_DEBUG == NSYS_LATENOTES_DEBUG_ON)
	      nsys_netin_latenotes_open(sptr);
	  }
	break;
      case NSYS_RTCPVAL_RR:
	break;
      case NSYS_RTCPVAL_SDES:
	break;
      case NSYS_RTCPVAL_BYE:
	nsys_netin_bye(packet, skip);
	return (sptr = NULL);
	break;
      case NSYS_RTCPVAL_APP:
	break;
      default:
	break;
      }

      packet += skip;     /* skip to next sub-packet */
    }

  return sptr;
}

/****************************************************************/
/*             top-level RTCP packet initializations            */
/****************************************************************/

void nsys_netout_rtcp_initpackets(void)

{
  int sdes_size;

  /************************************/
  /* calculate SDES CNAME packet size */
  /************************************/

  sdes_size = (NSYS_RTCPLEN_SDESHDR + NSYS_RTCPLEN_SDES_CHUNKHDR
	       + NSYS_RTCPLEN_SDES_ITEMHDR + nsys_cname_len);
  
  sdes_size += 4 - (sdes_size & 3);

  if (!nsys_netout_rtcp_packet_bye)
    {

      /*********************/
      /* create BYE packet */
      /*********************/

      nsys_netout_rtcp_len_bye = (NSYS_RTCPLEN_RREMPTY + sdes_size +
			     NSYS_RTCPLEN_BYE);

      nsys_netout_rtcp_packet_bye = malloc(nsys_netout_rtcp_len_bye + 
				      NSYS_MD5_LENGTH + 1);

      /*********************/
      /* create RR packets */
      /*********************/

      nsys_netout_rtcp_len_rrempty = NSYS_RTCPLEN_RREMPTY + sdes_size;
      nsys_netout_rtcp_packet_rrempty = malloc(nsys_netout_rtcp_len_rrempty + 
					  NSYS_MD5_LENGTH + 1);

      nsys_netout_rtcp_len_rr = NSYS_RTCPLEN_RR + sdes_size;
      nsys_netout_rtcp_packet_rr = malloc(nsys_netout_rtcp_len_rr + 
				     NSYS_MD5_LENGTH + 1);
	  
      /*********************/
      /* create SR packets */
      /*********************/

      nsys_netout_rtcp_len_srempty = NSYS_RTCPLEN_SREMPTY + sdes_size;
      nsys_netout_rtcp_packet_srempty = malloc(nsys_netout_rtcp_len_srempty + 
					  NSYS_MD5_LENGTH + 1);

      nsys_netout_rtcp_len_sr = NSYS_RTCPLEN_SR + sdes_size;
      nsys_netout_rtcp_packet_sr = malloc(nsys_netout_rtcp_len_sr + 
				     NSYS_MD5_LENGTH + 1);

    }

  /***************************************/
  /* initialize head of all RTCP packets */
  /***************************************/

  nsys_netout_rtcp_initrr(nsys_netout_rtcp_packet_rrempty, NSYS_RTCPLEN_RREMPTY);
  nsys_netout_rtcp_initrr(nsys_netout_rtcp_packet_rr, NSYS_RTCPLEN_RR);
  
  nsys_netout_rtcp_initsr(nsys_netout_rtcp_packet_srempty, NSYS_RTCPLEN_SREMPTY);
  nsys_netout_rtcp_initsr(nsys_netout_rtcp_packet_sr, NSYS_RTCPLEN_SR);
  
  nsys_netout_rtcp_initrr(nsys_netout_rtcp_packet_bye, NSYS_RTCPLEN_RREMPTY);
  
  /***************************************/
  /* initialize SDES of all RTCP packets */
  /***************************************/

  nsys_netout_rtcp_initsdes(&(nsys_netout_rtcp_packet_rrempty
			      [NSYS_RTCPLEN_RREMPTY]), sdes_size);
  nsys_netout_rtcp_initsdes(&(nsys_netout_rtcp_packet_rr
			      [NSYS_RTCPLEN_RR]), sdes_size);

  nsys_netout_rtcp_initsdes(&(nsys_netout_rtcp_packet_srempty
			      [NSYS_RTCPLEN_SREMPTY]), sdes_size);
  nsys_netout_rtcp_initsdes(&(nsys_netout_rtcp_packet_sr
			      [NSYS_RTCPLEN_SR]), sdes_size);

  nsys_netout_rtcp_initsdes(&(nsys_netout_rtcp_packet_bye
			      [NSYS_RTCPLEN_RREMPTY]), sdes_size);

  /*******************************/
  /* initialize BYE packet, sign */
  /*******************************/

  nsys_netout_rtcp_initbye(&(nsys_netout_rtcp_packet_bye
			     [NSYS_RTCPLEN_RREMPTY + sdes_size]));

  nsys_hmac_md5(nsys_netout_rtcp_packet_bye, nsys_netout_rtcp_len_bye, nsys_keydigest,
		&(nsys_netout_rtcp_packet_bye[nsys_netout_rtcp_len_bye]));

}

/****************************************************************/
/*               adds commands to reset an mset                 */
/****************************************************************/

int nsys_netin_clear_mset(unsigned char * buff, long * fill, long size)

{
  nsys_source s;

  if (!nsys_powerup_mset)
    return NSYS_JOURNAL_RECOVERED;

  s.mset = nsys_powerup_mset;
  nsys_powerup_mset = 0; 
 
  return nsys_netin_journal_addcmd_three(&s, buff, fill, size, 
					 CSYS_MIDI_POWERUP, 0, 0);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                  second-level RTCP functions                 */
/*______________________________________________________________*/


/****************************************************************/
/*             handle special rtcptime cases                    */
/****************************************************************/

int nsys_netout_excheck(void) 

{
  struct nsys_source * sptr;
  int print_exit, rfd;

  if (nsys_rtcp_ex & (NSYS_RTCPEX_RTPSIP | NSYS_RTCPEX_RTCPSIP))
    {
      print_exit = 0;

      if (nsys_rtcp_ex & NSYS_RTCPEX_RTPSIP)
	{
	  if (nsys_rtcp_ex & NSYS_RTCPEX_RTPNEXT)
	    nsys_rtcp_ex &= ~NSYS_RTCPEX_RTPNEXT;
	  else
	    {
	      if (nsys_rtp_sipretry++ < NSYS_SIP_RETRYMAX)
		nsys_sendsip(nsys_rtp_fd, NULL, nsys_rtp_invite);
	      else
		print_exit = nsys_graceful_exit = 1;
	    }
	}

      if (nsys_rtcp_ex & NSYS_RTCPEX_RTCPSIP)
	{
	  if (nsys_rtcp_ex & NSYS_RTCPEX_RTCPNEXT)
	    nsys_rtcp_ex &= ~NSYS_RTCPEX_RTCPNEXT;
	  else
	    {
	      if (nsys_rtcp_sipretry++ < NSYS_SIP_RETRYMAX)
		nsys_sendsip(nsys_rtcp_fd, NULL, nsys_rtcp_invite);
	      else
		print_exit = nsys_graceful_exit = 1;
	    }
	}

      if (print_exit)
	nsys_warning(NSYS_WARN_STANDARD, 
		     "SIP server probably off-line, exiting");
      else
	if (((nsys_rtcp_ex & NSYS_RTCPEX_RTPSIP) && 
	     ((nsys_rtcp_ex & NSYS_RTCPEX_RTPNEXT) == 0)) || 
	    ((nsys_rtcp_ex & NSYS_RTCPEX_RTCPSIP) && 
	     ((nsys_rtcp_ex & NSYS_RTCPEX_RTCPNEXT) == 0)))
	  nsys_warning(NSYS_WARN_STANDARD, 
			 "SIP server not responding, resending");
    }

  if ((nsys_rtcp_ex & NSYS_RTCPEX_SRCDUPL) == 0)
    return (nsys_rtcp_ex & NSYS_RTCPEX_NULLROOT);

  nsys_warning(NSYS_WARN_STANDARD, "CSRC clash: reinitializing network");

  nsys_rtcp_ex &= ~(NSYS_RTCPEX_RTPSIP | NSYS_RTCPEX_RTCPSIP | 
		    NSYS_RTCPEX_RTPNEXT | NSYS_RTCPEX_RTCPNEXT);

  /*************************/
  /* send RTCP BYE packet  */
  /*************************/

  if ((nsys_rtcp_ex & NSYS_RTCPEX_NULLROOT) == 0)
    nsys_netout_rtcpsend(nsys_netout_rtcp_packet_bye, 
			 nsys_netout_rtcp_len_bye + NSYS_RTPSIZE_DIGEST);

  /*****************/
  /* shut down SIP */
  /*****************/

  nsys_sip_shutdown();           

  /******************************************************************/
  /* pick new ssrc value, send SIP INVITEs, set up RTP/RTCP packets */
  /******************************************************************/

  rfd = open("/dev/urandom", O_RDONLY | O_NONBLOCK);

  if ((rfd < 0) || (read(rfd, &nsys_myssrc, sizeof(unsigned long)) < 0))
    nsys_myssrc = (unsigned long)rand();

  nsys_myssrc_net = htonl(nsys_myssrc);

  if (rfd)
    close(rfd);

  if (nsys_initsip() == NSYS_ERROR)
    NSYS_ERROR_TERMINATE("Error sending SIP INVITE during CSRC clash");

  memcpy(&(nsys_netout_rtp_packet[NSYS_RTPLOC_SSRC]), &nsys_myssrc_net,
	 sizeof(long));

  nsys_nexttime = time(NULL) + NSYS_RTCPTIME_INCR;

  nsys_netout_rtcp_initpackets();

  nsys_sent_this = nsys_sent_last = 0;
  nsys_sent_octets = nsys_sent_packets = 0;

  if ((sptr = nsys_srcroot))
    {
      do 
	{
	  sptr->received = sptr->rtcp_received = sptr->received_prior = 0;
	  sptr = sptr->next;
	} 
      while (sptr != nsys_srcroot);
    }

  /**********************/
  /* reset nsys_rtcp_ex */
  /**********************/

  nsys_rtcp_ex = (nsys_srcroot == NULL) ? NSYS_RTCPEX_NULLROOT : 0;
  nsys_rtcp_ex |= (NSYS_RTCPEX_RTCPSIP | NSYS_RTCPEX_RTPSIP); 

  return (0);

}

/****************************************************************/
/*        sends out RTCP SR/RR reports                          */
/****************************************************************/

void nsys_netout_rtcpreport(void)

{  
  int len_empty, len_full, offset;
  unsigned char * p_empty, * p_full;
  int len = 0;
  unsigned char * p = NULL;
  int retry = 0;
  int alt = 0;
  unsigned long dead_ssrc;
  unsigned long expected, expected_this, received_this;
  long num_lost;
  struct nsys_source * sptr;
  struct sockaddr * addr;
  struct timeval now;

  sptr = nsys_srcroot;

  if (nsys_sent_this || nsys_sent_last)
    {
      nsys_sent_last = nsys_sent_this;
      nsys_sent_this = 0;
      
      offset = NSYS_RTCPLEN_SRHDR + NSYS_RTCPLEN_SENDER;
      len_empty = nsys_netout_rtcp_len_srempty;
      p_empty = nsys_netout_rtcp_packet_srempty;
      len_full = nsys_netout_rtcp_len_sr;
      p_full = nsys_netout_rtcp_packet_sr;

      gettimeofday(&now, 0);

      *((unsigned long *) &(nsys_netout_rtcp_packet_sr[NSYS_RTCPLOC_SR_NTPMSB])) =
	htonl(0x83aa7e80 + now.tv_sec);

      *((unsigned long *) &(nsys_netout_rtcp_packet_sr[NSYS_RTCPLOC_SR_NTPLSB])) =
	htonl((unsigned long) 
	      (((((unsigned long)(now.tv_usec << 12))/15625) << 14))); 

      *((unsigned long *) &(nsys_netout_rtcp_packet_sr[NSYS_RTCPLOC_SR_TSTAMP])) =
	htonl(nsys_netout_tstamp);

      *((unsigned long *) &(nsys_netout_rtcp_packet_sr[NSYS_RTCPLOC_SR_PACKET])) =
	htonl(nsys_sent_packets);

      *((unsigned long *) &(nsys_netout_rtcp_packet_sr[NSYS_RTCPLOC_SR_OCTET])) =
	htonl(nsys_sent_octets);

      memcpy(&(nsys_netout_rtcp_packet_srempty[NSYS_RTCPLEN_SRHDR]), 
	     &(nsys_netout_rtcp_packet_sr[NSYS_RTCPLEN_SRHDR]), 
	     NSYS_RTCPLEN_SENDER);
    }
  else
    {
      offset = NSYS_RTCPLEN_RRHDR;

      len_empty = nsys_netout_rtcp_len_rrempty;
      p_empty = nsys_netout_rtcp_packet_rrempty;
      len_full = nsys_netout_rtcp_len_rr;
      p_full = nsys_netout_rtcp_packet_rr;
    }

  do 
    {
      if (!alt)
	{
	  addr = (struct sockaddr *)(sptr->rtcp_addr);
	  if (time(NULL) > sptr->expire_rtcp)
	    {
	      dead_ssrc = sptr->ssrc;
	      if ((sptr = sptr->next) == nsys_srcroot)
		sptr = NULL;
	      nsys_delete_ssrc(dead_ssrc, 
			       "RTCP silent too long, deleting");
	      if (sptr)
		continue;
	      else
		break;
	    }

	  if (sptr->received > sptr->received_prior)
	    {

	      p = p_full;
	      len = len_full;

	      *((unsigned long *) &(p[offset + NSYS_RTCPLOC_RR_SSRC])) =
		htonl(sptr->ssrc);

	      expected = sptr->hi_ext - sptr->base_seq + 1;

	      expected_this = expected - sptr->expected_prior;
	      sptr->expected_prior = expected;

	      received_this = sptr->received - sptr->received_prior;
	      sptr->received_prior = sptr->received;

	      num_lost = expected - sptr->received;
	      
	      if (num_lost < NSYS_RTPSEQ_MINLOSS)
		num_lost = NSYS_RTPSEQ_MINLOSS;

	      if (num_lost > NSYS_RTPSEQ_MAXLOSS)
		num_lost = NSYS_RTPSEQ_MAXLOSS;

	      num_lost &= NSYS_RTPSEQ_FMASK;

	      if (((expected_this - received_this) > 0) && expected_this)
		num_lost |= ((((expected_this - received_this) << 8)
			      /expected_this) << 24);

	      *((unsigned long *) &(p[offset + NSYS_RTCPLOC_RR_FRACTLOSS])) =
		htonl((unsigned long) num_lost);

	      *((unsigned long *) &(p[offset + NSYS_RTCPLOC_RR_HISEQ])) =
		htonl(sptr->hi_ext);

	      *((unsigned long *) &(p[offset + NSYS_RTCPLOC_RR_JITTER])) =
		htonl((unsigned long)(sptr->jitter >> 4));

	      memcpy(&(p[offset + NSYS_RTCPLOC_RR_LASTSR]), sptr->lsr, 4);

	      gettimeofday(&now, 0);

	      *((unsigned long *) &(p[offset + NSYS_RTCPLOC_RR_DELAY])) =
		(sptr->arrival.tv_sec == 0) ? 0 : 
		htonl((unsigned long)
		      (((now.tv_sec - sptr->arrival.tv_sec) << 16) +
		       (((now.tv_usec - sptr->arrival.tv_usec) << 10) 
			/15625)));

	      sptr->received_prior = sptr->received;
	    }
	  else
	    {
	      p = p_empty;
	      len = len_empty;
	    }

	  nsys_hmac_md5(p, len, nsys_keydigest, &(p[len]));
	  len += NSYS_RTPSIZE_DIGEST;
	}
      else
	addr = (struct sockaddr *)(sptr->alt_rtcp_addr);
      
      if (sendto(nsys_rtcp_fd, p, len, 0, addr, 
		 sizeof(struct sockaddr)) == -1)
	{
	  if (errno == EAGAIN)
	    continue;
	  
	  if ((errno == EINTR) || (errno == ENOBUFS))
	    {
	      if (++retry > NSYS_MAXRETRY)
		NSYS_ERROR_TERMINATE("Too many I/O retries -- nsys_netout_rtcptime");
	      continue;         
	    }
	  
	  NSYS_ERROR_TERMINATE("Error writing Internet socket");
	}
      
      if (!(alt = ((!alt) && sptr->alt_rtcp_addr)))
	sptr = sptr->next;
    } 
  while (alt || (sptr != nsys_srcroot));

  return;

}
/****************************************************************/
/*                  sends out RTCP packets                      */
/****************************************************************/

void nsys_netout_rtcpsend(unsigned char * p, int len)

{  
  int retry = 0;
  int alt = 0;
  struct nsys_source * sptr;
  struct sockaddr * addr;

  sptr = nsys_srcroot;

  do 
    {
      addr = (alt ? (struct sockaddr *)(sptr->alt_rtcp_addr) : 
	      (struct sockaddr *)(sptr->rtcp_addr));
      
      if (sendto(nsys_rtcp_fd, p, len, 0, addr, 
		 sizeof(struct sockaddr)) == -1)
	{
	  if (errno == EAGAIN)
	    continue;
	  
	  if ((errno == EINTR) || (errno == ENOBUFS))
	    {
	      if (++retry > NSYS_MAXRETRY)
		NSYS_ERROR_TERMINATE("Too many I/O retries -- nsys_netout_rtcptime");
	      continue;         
	    }
	  
	  NSYS_ERROR_TERMINATE("Error writing Internet socket");
	}
      
      if (!(alt = ((!alt) && sptr->alt_rtcp_addr)))
	sptr = sptr->next;
    } 
  while (alt || (sptr != nsys_srcroot));
  
  return;

}

/****************************************************************/
/*             sends out keepalive SIP INFO packets             */
/****************************************************************/

void nsys_netout_keepalive(void)

{        
  if (nsys_sipinfo_toggle && ((nsys_rtcp_ex & NSYS_RTCPEX_RTPSIP) == 0))
    nsys_sendsip(nsys_rtp_fd, NULL, nsys_rtp_info);
  else
    if ((nsys_rtcp_ex & NSYS_RTCPEX_RTCPSIP) == 0)
      nsys_sendsip(nsys_rtcp_fd, NULL, nsys_rtcp_info);
  
  nsys_sipinfo_count = 0;
  nsys_sipinfo_toggle = !nsys_sipinfo_toggle;
}


/****************************************************************/
/*              display received RTCP packet information        */
/****************************************************************/

void nsys_netin_rtcp_display(unsigned char * packet, int len, 
			     struct timeval * now) 

{

#if NSYS_DISPLAY_RTCP

  int error, complen;
  unsigned long csrc, current, sent, delay;
  long lost;
  unsigned char contents, slen, offset;
  unsigned char * p;

  if (NSYS_DISPLAY_RTCP_HDR)
    {
      printf("processing RTCP payload -- %i bytes\n", len);
    }

  error = 0;
  while ((len > 0) && !error)
    {
      contents = offset = 0;
      complen = ntohs(*((unsigned short *)&(packet[NSYS_RTCPLOC_LENGTH])));
      switch (packet[NSYS_RTCPLOC_PTYPE]) {
      case NSYS_RTCPVAL_SR:
	contents = packet[NSYS_RTCPLOC_BYTE1] & NSYS_RTCPVAL_COUNTMASK;
	if (contents)
	  offset = NSYS_RTCPLEN_SRHDR + NSYS_RTCPLEN_SENDER;
	if (NSYS_DISPLAY_RTCP_HDR)
	  {
	    csrc = ntohl(*((unsigned long *)&(packet[NSYS_RTCPLOC_SSRC])));
	    printf("type SR (%i bytes) from SSRC %lu with %hhu reception reports\n",
		   4*complen + 4, csrc, contents);
	  }
	if (NSYS_DISPLAY_RTCP_SRINFO)
	  {
	    printf("NTP time %f, RTP timestamp %lu\n", 
		   ntohl(*((unsigned long *)&(packet[NSYS_RTCPLOC_SR_NTPMSB])))
		   + (ntohl(*((unsigned long *)&(packet[NSYS_RTCPLOC_SR_NTPLSB])))
		      /(4294967296.0)), (unsigned long)
		   ntohl(*((unsigned long *)&(packet[NSYS_RTCPLOC_SR_TSTAMP]))));
	    printf("Total packets %lu, total octets %lu\n", (unsigned long)
		   ntohl(*((unsigned long *)&(packet[NSYS_RTCPLOC_SR_PACKET]))),
		   (unsigned long) 
		   ntohl(*((unsigned long *)&(packet[NSYS_RTCPLOC_SR_OCTET]))));
	  }
	break;
      case NSYS_RTCPVAL_RR:
	contents = packet[NSYS_RTCPLOC_BYTE1] & NSYS_RTCPVAL_COUNTMASK;
	if (contents)
	  offset = NSYS_RTCPLEN_RRHDR;
	if (NSYS_DISPLAY_RTCP_HDR)
	  {
	    csrc = ntohl(*((unsigned long *)&(packet[NSYS_RTCPLOC_SSRC])));
	    printf("type RR (%i bytes) -- from SSRC %lu with %hhu reception reports\n",
		   4*complen + 4, csrc, contents);
	  }
	break;
      case NSYS_RTCPVAL_SDES:
	contents = packet[NSYS_RTCPLOC_BYTE1] & NSYS_RTCPVAL_COUNTMASK;
	if (NSYS_DISPLAY_RTCP_HDR)
	  {
	    printf("type SDES (%i bytes) -- %hhu chunks\n", 4*complen + 4,
		   contents);
	  }
	if (NSYS_DISPLAY_RTCP_SDES)
	  {
	    p = packet + NSYS_RTCPLEN_SDES_CHUNKHDR;
	    while (contents--)
	      {
		/* add fail-safe for bad packets */
		
		csrc = ntohl(*((unsigned long *)p));
		printf("SDES for CSRC %lu\n", csrc);
		p += 4;
		while (p[NSYS_RTCPLOC_SDESITEM_TYPE] != '\0')
		  {
		    printf("%s: ", (p[NSYS_RTCPLOC_SDESITEM_TYPE] < 
				    NSYS_RTCPVAL_SDES_SIZE) ? 
			   sdes_typename[p[NSYS_RTCPLOC_SDESITEM_TYPE]] : 
			   "Illegal");
		    slen = p[NSYS_RTCPLOC_SDESITEM_LENGTH];
		    p += 2;
		    while (slen--)
		      putchar(*(p++));
		    printf("\n");
		  }
	      }
	  }
	break;
      case NSYS_RTCPVAL_BYE:
	if (NSYS_DISPLAY_RTCP_HDR)
	  {
	    printf("type BYE (%i bytes)\n", 4*complen + 4);
	  }
	break;
      case NSYS_RTCPVAL_APP:	
	if (NSYS_DISPLAY_RTCP_HDR)
	  {
	    printf("type APP\n");
	  }
	break;
      default:
	printf("error parsing PTYPE %i (%i bytes)\n",
	       packet[NSYS_RTCPLOC_PTYPE], complen);
	error = 1;
	break;
      }

      if (offset && NSYS_DISPLAY_RTCP_RRINFO)
	{
	  printf("reception quality from SSRC %lu\n",(unsigned long)
		 ntohl(*((unsigned long *)
			 &(packet[offset+NSYS_RTCPLOC_RR_SSRC]))));

	  printf("fraction lost: %f,", 
		 (packet[offset+NSYS_RTCPLOC_RR_FRACTLOSS]/256.0F));

	  memcpy(&lost, &(packet[offset+NSYS_RTCPLOC_RR_FRACTLOSS]),
		 sizeof(long));
	  lost = (signed long)ntohl((unsigned long)lost);

	  if (((lost &= NSYS_RTPSEQ_FMASK) & NSYS_RTPSEQ_TSIGN))
	    lost |= (~NSYS_RTPSEQ_FMASK);

	  printf(" total lost packets %li\n", lost);

	  printf("highest sequence number received %lu\n", (unsigned long)
		 ntohl(*((unsigned long *)
			 &(packet[offset+NSYS_RTCPLOC_RR_HISEQ]))));
	  printf("jitter: %f ms\n", (1000/ARATE)*(unsigned long)
		 ntohl(*((unsigned long *)
			 &(packet[offset + NSYS_RTCPLOC_RR_JITTER]))));

	  current = now->tv_sec;
	  current = (current + 32384) << 16;
	  current += (((unsigned long)now->tv_usec) << 10)/15625;
	  sent = ntohl(*((unsigned long *)
			 &(packet[offset + NSYS_RTCPLOC_RR_LASTSR])));
	  delay = ntohl(*((unsigned long *)
			  &(packet[offset + NSYS_RTCPLOC_RR_DELAY])));
	  if (sent || delay)
	    {
	      if (current > sent)
		printf("Estimated round-trip time: %f milliseconds\n",
		       1000.0F*(current - sent - delay)/65536.0F);
	      else
		printf("Estimated round-trip time: %f milliseconds (flip)\n",
		       1000.0F*(sent - current - delay)/65536.0F);
	      if (NSYS_DISPLAY_RTCP_RRTCOMP)
		{
		  printf("    Components\n");
		  printf("     Current time %fs\n", current/65536.0F);
		  printf("        Sent time %fs\n", sent/65536.0F);
		  printf("       Delay time %fs\n", delay/65536.0F);
		}
	    }

	}

      if (error)
	break;

      complen = 4*complen + 4;
      len -= complen;
      packet += complen;
    }

  if (error)
    printf("error processing RTCP payload\n");
  else
    if (NSYS_DISPLAY_RTCP_HDR)
      printf("RTCP payload succesfully processed \n\n");

  printf("\n");

  return;

#endif

}



/****************************************************************/
/*   parse new SDES cname subpacket item -- presently not used  */
/****************************************************************/

char * nsys_netin_newcname(unsigned char * packet, int len) 

{
  unsigned char slen;
  char * ret, * rptr;

  packet += 8;    /* skip header, assume one csrc */
  len -= 8; 

  while ((len > 0) && 
	 (packet[NSYS_RTCPLOC_SDESITEM_TYPE] != NSYS_RTCPVAL_SDES_CNAME))
    {
      slen = packet[NSYS_RTCPLOC_SDESITEM_LENGTH];
      len -= 2 + slen;
      packet +=  2 + slen;
    }

  if ((len <= 0) || 
      (packet[NSYS_RTCPLOC_SDESITEM_TYPE] != NSYS_RTCPVAL_SDES_CNAME))
    return NULL;

  slen = packet[NSYS_RTCPLOC_SDESITEM_LENGTH];
  rptr = ret = calloc(slen + 1, sizeof(char));
  packet += 2;

  while (slen)
    {
      *(rptr++) = *(packet++);
      slen--;
    }

  nsys_stderr_size += 
    fprintf(stderr, "Adding session member: %s\n", ret);
  fflush(stderr);

  return ret;

}


/****************************************************************/
/*                   parse new BYE subpacket                    */
/****************************************************************/

void nsys_netin_bye(unsigned char * packet, int len) 

{
  char sc;
  unsigned long ssrc;

  if ((len -= 4) <= 0)
    return;

  sc = packet[NSYS_RTCPLOC_BYTE1] & NSYS_RTCPVAL_COUNTMASK;

  while (sc && ((len -= 4) >= 0))
    {
      packet += 4;
      memcpy(&ssrc, packet, sizeof(long));
      ssrc = ntohl(ssrc);
      nsys_delete_ssrc(ssrc, "RTCP BYE received, deleting");
      sc--;
    }
}



/****************************************************************/
/*                     initialize RR packets                    */
/****************************************************************/

void nsys_netout_rtcp_initrr(unsigned char * p, int len)

{
  unsigned short nlen;

  memset(p, 0, len);

  /* Version 2, no padding, 0 or 1 reception reports */
  
  p[NSYS_RTCPLOC_BYTE1] = NSYS_RTCPVAL_BYTE1 + (len > NSYS_RTCPLEN_RREMPTY);

  /* PTYPE RR */

  p[NSYS_RTCPLOC_PTYPE] = NSYS_RTCPVAL_RR;

  /* length is number of 32-bit words - 1, in network byte order */

  nlen = htons((len >> 2) - 1);
  memcpy(&(p[NSYS_RTCPLOC_LENGTH]), &nlen, sizeof(unsigned short));

  /* SSRC of packet sender, in network byte order */

  memcpy(&(p[NSYS_RTCPLOC_SSRC]), &nsys_myssrc_net, sizeof(long));

}

/****************************************************************/
/*                     initialize SR packets                    */
/****************************************************************/

void nsys_netout_rtcp_initsr(unsigned char * p, int len)

{
  unsigned short nlen;

  memset(p, 0, len);

  /* Version 2, no padding, 0 or 1 reception reports */
  
  p[NSYS_RTCPLOC_BYTE1] = NSYS_RTCPVAL_BYTE1 + (len > NSYS_RTCPLEN_SREMPTY);

  /* PTYPE SR */

  p[NSYS_RTCPLOC_PTYPE] = NSYS_RTCPVAL_SR;

  /* length is number of 32-bit words - 1, in network byte order */

  nlen = htons((len >> 2) - 1);
  memcpy(&(p[NSYS_RTCPLOC_LENGTH]), &nlen, sizeof(unsigned short));

  /* SSRC of packet sender, in network byte order */

  memcpy(&(p[NSYS_RTCPLOC_SSRC]), &nsys_myssrc_net, sizeof(long));

}

/****************************************************************/
/*                   initialize SDES packets                    */
/****************************************************************/

void nsys_netout_rtcp_initsdes(unsigned char * p, int len)

{
  unsigned short nlen;

  memset(p, 0, len);  /* zero-pads end of chunklist */

  /* one SSRC chunk */

  p[NSYS_RTCPLOC_BYTE1] = 1 + NSYS_RTCPVAL_BYTE1;

  /* PTYPE SDES */

  p[NSYS_RTCPLOC_PTYPE] = NSYS_RTCPVAL_SDES;

  /* length is number of 32-bit words - 1, in network byte order */

  nlen = htons((len >> 2) - 1);
  memcpy(&(p[NSYS_RTCPLOC_LENGTH]), &nlen, sizeof(unsigned short));

  /* SSRC of packet sender, in network byte order */

  p += NSYS_RTCPLEN_SDESHDR;
  memcpy(p, &nsys_myssrc_net, sizeof(long));

  p += NSYS_RTCPLEN_SDES_CHUNKHDR;
  p[NSYS_RTCPLOC_SDESITEM_TYPE] = NSYS_RTCPVAL_SDES_CNAME;
  p[NSYS_RTCPLOC_SDESITEM_LENGTH] = nsys_cname_len;

  p += NSYS_RTCPLEN_SDES_ITEMHDR;

  strcpy((char *) p, nsys_cname);

}

/****************************************************************/
/*                   initialize BYE packets                     */
/****************************************************************/

void nsys_netout_rtcp_initbye(unsigned char * p)

{
  unsigned short nlen;

  /* Version 2, no padding, 1 BYE */
  
  p[NSYS_RTCPLOC_BYTE1] = NSYS_RTCPVAL_BYTE1 + 1;

  /* PTYPE SR */

  p[NSYS_RTCPLOC_PTYPE] = NSYS_RTCPVAL_BYE;

  /* BYE is 8 bytes long --> 2 - 1 words --> 1 word len field */

  nlen = htons(1);
  memcpy(&(p[NSYS_RTCPLOC_LENGTH]), &nlen, sizeof(unsigned short));

  /* SSRC of packet sender, in network byte order */

  memcpy(&(p[NSYS_RTCPLOC_SSRC]), &nsys_myssrc_net, sizeof(long));

}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                  low-level RTCP functions                    */
/*______________________________________________________________*/

/***************************************************************/
/*                   delete an ssrc                            */
/***************************************************************/

void nsys_delete_ssrc(unsigned long ssrc, char * reason)

{
  struct nsys_source * sptr;
  struct nsys_source * last;
  int mset, i;
  last = NULL;
  sptr = nsys_ssrc[ssrc & NSYS_HASHMASK];

  /**************/
  /* find match */
  /**************/

  while (sptr && (sptr->ssrc != ssrc))
    {
      last = sptr;
      sptr = sptr->xtra;
    }

  /*********************/
  /* leave if no match */
  /*********************/

  if (sptr == NULL)
    return;

  /***************/
  /* update xtra */
  /***************/

  if (last)
    last->xtra = sptr->xtra;
  else
    nsys_ssrc[ssrc & NSYS_HASHMASK] = sptr->xtra;

  /*********************/
  /* update prev, next */
  /*********************/

  if ((sptr == nsys_srcroot) && (sptr->prev == nsys_srcroot))
    {
      nsys_srcroot = NULL;
      nsys_rtcp_ex |= NSYS_RTCPEX_NULLROOT;
    }
  else
    {
      sptr->prev->next = sptr->next;
      sptr->next->prev = sptr->prev;
      if (nsys_srcroot == sptr)
	nsys_srcroot = sptr->next;
    }

  nsys_status(sptr, reason);

  free(sptr->cname);
  free(sptr->sdp_addr);
  free(sptr->rtp_addr);
  free(sptr->alt_rtp_addr);
  free(sptr->rtcp_addr);
  free(sptr->alt_rtcp_addr);

  nsys_powerup_mset = mset = sptr->mset;

  for (i = 0; i < CSYS_MIDI_NUMCHAN; i++)
    if (sptr->jrecv[i])
      {
	sptr->jrecv[i]->next = nsys_recvfree;
	nsys_recvfree = sptr->jrecv[i];
      }

  if (sptr->jrecvsys)
    {
      sptr->jrecvsys->next = nsys_recvsysfree;
      nsys_recvsysfree = sptr->jrecvsys;
    }
  
#if (NSYS_LATENOTES_DEBUG == NSYS_LATENOTES_DEBUG_ON)
  printf("mset%i packets expected: %i\n", sptr->mset,
	 sptr->hi_ext - sptr->base_seq + 1);
  printf("mset%i packets received: %i\n", sptr->mset,
	 sptr->received);
  fclose(sptr->tm_fd);

#endif

  memset(sptr, 0, sizeof(struct nsys_source));
  
  sptr->mset = mset;
  sptr->next = nsys_srcfree;
  nsys_srcfree = sptr;

}



/***************************************************************/
/*               harvest an unused ssrc slot                   */
/***************************************************************/

nsys_source * nsys_harvest_ssrc(int fd, struct sockaddr_in * ipaddr)

{
  nsys_source * sptr, * oldptr;
  unsigned long now, oldtime;
  int found = 0;

  /* sanity checks */

  if ((nsys_srcfree != NULL) || (nsys_srcroot == NULL)) 
    return NULL;

  oldtime = 0;
  now = time(NULL);

  oldptr = sptr = nsys_srcroot;

  do {

    if ((fd == nsys_rtp_fd) && 
	(sptr->alt_rtp_addr == NULL) && sptr->rtp_addr && 
	!memcmp(ipaddr, sptr->rtp_addr, sizeof(struct sockaddr_in)))
      {
	found = 1;
	break;
      }
    
    if ((fd == nsys_rtcp_fd) && 
	(sptr->alt_rtcp_addr == NULL) && sptr->rtcp_addr && 
	!memcmp(ipaddr, sptr->rtcp_addr, sizeof(struct sockaddr_in)))
      {
	found = 1;
	break;
      }

    if ((sptr->birthtime - now) >= oldtime)
      {
	oldptr = sptr;
	oldtime = sptr->birthtime - now;
      }

    sptr = sptr->next;
    
  } while (sptr != nsys_srcroot);

  if (found)
    nsys_delete_ssrc(sptr->ssrc, 
		     "Remote client restarted, deleting old entry for");
  else
    nsys_delete_ssrc(oldptr->ssrc, 
		     "Too many players, increase -bandsize value."
		     " Deleting oldest player");

  return nsys_srcfree;
}

/***************************************************************/
/*   state update when an RTP/RTCP packet arrive too late      */
/***************************************************************/

void nsys_late_windowcheck(nsys_source * sptr, unsigned long tstamp)

{
  unsigned long tdiff;

  if (sptr->tm_lateflag)
    {
      tdiff = nsys_netout_tstamp - sptr->tm_latetime;
      if (tdiff > (unsigned long)(ARATE*NSYS_SM_LATEWINDOW))
	{
	  sptr->tm_lateflag = 0;
	  sptr->tm_convert = tstamp - nsys_netout_tstamp - sptr->tm_margin;
	  nsys_warning(NSYS_WARN_UNUSUAL, "Resetting late packet detection");
	}
    }
  else
    {
      sptr->tm_latetime = nsys_netout_tstamp;
      sptr->tm_lateflag = 1;
    }

}

/***************************************************************/
/*      print warning message for truncated RTCP packet        */
/***************************************************************/

void nsys_netin_rtcp_trunc(int sub)

{
  switch (sub) {
  case NSYS_RTCPVAL_SR:
    nsys_warning(NSYS_WARN_UNUSUAL, "RTCP SR subpacket truncated");
    break;
  case NSYS_RTCPVAL_RR:
    nsys_warning(NSYS_WARN_UNUSUAL, "RTCP RR subpacket truncated");
    break;
  case NSYS_RTCPVAL_SDES:
    nsys_warning(NSYS_WARN_UNUSUAL, "RTCP SDES subpacket truncated");
    break;
  case NSYS_RTCPVAL_BYE:
    nsys_warning(NSYS_WARN_UNUSUAL, "RTCP SDES subpacket truncated");
    break;
  case NSYS_RTCPVAL_APP:
    nsys_warning(NSYS_WARN_UNUSUAL, "RTCP SDES subpacket truncated");
    break;
  default:
    nsys_warning(NSYS_WARN_UNUSUAL, "RTCP unknown subpacket truncated");
    break;
  }

}

/***************************************************************/
/*         initialize debug files for latenotes                */
/***************************************************************/

void nsys_netin_latenotes_open(nsys_source * sptr)

{

#if (NSYS_LATENOTES_DEBUG == NSYS_LATENOTES_DEBUG_ON)
				
  sptr->tm_first = nsys_netout_tstamp;
  
  switch (sptr->mset) {
  case 1:
    sptr->tm_fd = fopen("mset1.dat", "w");
    break;
  case 2:
    sptr->tm_fd = fopen("mset2.dat", "w");
    break;
  case 3:
    sptr->tm_fd = fopen("mset3.dat", "w");
    break;
  default:
    sptr->tm_fd = fopen("toomany.dat", "w");
    break;
  }
  
  fprintf(sptr->tm_fd,"1\n");
  fprintf(sptr->tm_fd,"#\n");
  fprintf(sptr->tm_fd,"pairs\n");
  fprintf(sptr->tm_fd,"remote%i\n", sptr->mset);
  fprintf(sptr->tm_fd,"*\n");
  fprintf(sptr->tm_fd,"*\n");

#endif

}

/* end Network library -- RTCP functions */
