
/*
#    Sfront, a SAOL to C translator    
#    This file: Sfront-specific network functions
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

/****************************************************************/
/*           receives RTP, RTCP, and SIP packets                */
/****************************************************************/

int nsys_newdata(void) 

{
  struct sockaddr_in ipaddr;
  unsigned long ssrc;
  struct nsys_source * sptr;
  int len;
  unsigned char packet[NSYS_UDPMAXSIZE+1];
  unsigned char digest[NSYS_MD5_LENGTH];
  unsigned char sysbuff[NSYS_NETIN_SYSBUFF];
  unsigned char * p, * pmax;
  unsigned char  ptype;
  unsigned short mlen;
  unsigned char cmd, oldcmd;
  unsigned char ndata = 0;
  unsigned char vdata = 0;
  unsigned int fromlen = sizeof(struct sockaddr);
  int retry = 0;
  int has_ts, dsize, rtpcode, jcode, fec, sysidx, skip;
  int nopacket = 1;
  unsigned short status;
  int fd = nsys_rtp_fd;

  nsys_bufflen = nsys_buffcnt = 0;

  nsys_netout_tstamp += ACYCLE;

  if (nsys_netout_jsend_guard_time && ((--nsys_netout_jsend_guard_time) == 0))
    nsys_netout_guard_tick();
 
  while (1)
    {
      if ((len = recvfrom(fd, packet, NSYS_UDPMAXSIZE, 0, 
			  (struct sockaddr *)&ipaddr, &fromlen)) <= 0)
	{

	  if ((errno == EAGAIN) && (fd == nsys_rtcp_fd))
	    {
	      if (nopacket && (time(NULL) > nsys_nexttime))
		{
		  nsys_netout_rtcptime();		  
		  if (nsys_powerup_mset)
		    nsys_netin_clear_mset(nsys_buff, &nsys_bufflen, 
					  NSYS_BUFFSIZE);
		}
	      return (nsys_bufflen ? NSYS_MIDIEVENTS : NSYS_DONE);
	    }

	  if ((errno == EAGAIN) && (fd == nsys_rtp_fd))
	    {
	      fd = nsys_rtcp_fd;
	      continue;
	    }
	  
	  if (errno == EINTR)
	    {
	      if (++retry > NSYS_MAXRETRY)
		NSYS_ERROR_TERMINATE("Too many I/O retries: nsys_netin_newdata");
	      continue;         
	    }
	  
	  NSYS_ERROR_TERMINATE("Error reading Internet socket");
	}
      else
	{
	  /*****************/
	  /* RTP fast path */
	  /*****************/

	  nopacket = 0;

	  if ((fd == nsys_rtp_fd) && 
	      (packet[NSYS_RTPLOC_BYTE1] == NSYS_RTPVAL_BYTE1))
	    {	      

	      /*****************************************/
	      /* parse top of header to extract fields */
	      /*****************************************/

	      if ((len < NSYS_RTPLEN_HDR) || (len <= NSYS_RTPSIZE_DIGEST))
		{
		  nsys_warning(NSYS_WARN_UNUSUAL, 
			       "Ignoring truncated RTP packet");
		  continue;
		}

	      if ((packet[NSYS_RTPLOC_PTYPE] & NSYS_RTPVAL_CHKMARK) == 0)
		{
		  nsys_warning(NSYS_WARN_UNUSUAL, 
			       "Marker bit not set, ignoring packet");
		  continue;
		}

	      ssrc = ntohl(*((unsigned long *)&(packet[NSYS_RTPLOC_SSRC])));
	      ptype = packet[NSYS_RTPLOC_PTYPE] & NSYS_RTPVAL_CLRMARK;
	      sptr = nsys_ssrc[ssrc & NSYS_HASHMASK];
	      
	      while (sptr && (sptr->ssrc != ssrc))
		sptr = sptr->xtra;
	      
	      if (!sptr)
		{
		  nsys_warning(NSYS_WARN_UNUSUAL, 
			       "RTP packet from unknown source");
		  continue;
		}

	      if (((ipaddr.sin_addr.s_addr != sptr->rtp_addr->sin_addr.s_addr) ||
		  (ipaddr.sin_port != sptr->rtp_addr->sin_port)) && 
		  (sptr->alt_rtp_addr == NULL))
		{
		  nsys_warning(NSYS_WARN_UNUSUAL, 
			       "RTP packet from unknown IP/port");
		  continue;
		}

	      /*********************/
	      /* do authentication */
	      /*********************/

	      len -= NSYS_RTPSIZE_DIGEST;

	      if (nsys_msession)
		memcpy(&(packet[NSYS_RTPLOC_SSRC]), &nsys_myssrc_net, 
		       sizeof(long));

	      nsys_hmac_md5(packet, len, nsys_keydigest, digest);

	      if (memcmp(&(packet[len]), digest, NSYS_RTPSIZE_DIGEST))
		{		  
		  nsys_warning(NSYS_WARN_UNUSUAL, 
			       "Discarding unauthorized RTP packet");
		  continue;
		}

	      /******************/
	      /* process packet */
	      /******************/

	      if (ptype != sptr->ptype)
		{
		  nsys_warning(NSYS_WARN_UNUSUAL, 
			       "RTP packet with incorrect ptype");
		  continue;
		}

	      if (sptr->alt_rtp_addr)
		{
		  if ((ipaddr.sin_addr.s_addr != nsys_sip_rtp_inet_addr) ||
		      (ipaddr.sin_port != nsys_sip_rtp_sin_port))
		    {
		      /* normal case */

		      memcpy(sptr->rtp_addr,&ipaddr,sizeof(struct sockaddr_in));
		    }
		  else
		    {
		      /* source-forge trick which may later be implemented */

		      memcpy(sptr->rtp_addr, &(sptr->alt_rtp_addr), 
			     sizeof(struct sockaddr_in));
		    }

		  free(sptr->alt_rtp_addr);
		  sptr->alt_rtp_addr = NULL;
		}

	      rtpcode = nsys_netin_rtpstats(sptr, packet);

	      if (rtpcode == NSYS_RTPCODE_SECURITY)
		{
		  nsys_warning(NSYS_WARN_UNUSUAL, "Possible RTP replay attack");
		  continue;
		}

	      if ((len -= NSYS_RTPLEN_HDR) == 0)
		{
		  nsys_warning(NSYS_WARN_UNUSUAL, "RTP payload empty");
		  continue;
		}

	      p = packet + NSYS_RTPLEN_HDR;
	      fec = (*p) & NSYS_SM_CHKJ;

	      if ((rtpcode == NSYS_RTPCODE_DISCARD) && fec)
		{
		  nsys_warning(NSYS_WARN_UNUSUAL, "Out of order RTP packet");
		  continue;        
		}

	      if ((rtpcode != NSYS_RTPCODE_NORMAL) && fec)
		{

		  jcode = nsys_netin_journal_recovery(sptr, rtpcode, p, len,
						      nsys_buff, &nsys_bufflen,
						      NSYS_BUFFSIZE);
		  if (jcode == NSYS_JOURNAL_CORRUPTED)
		    {
		      nsys_warning(NSYS_WARN_UNUSUAL, "RTP journal corrupt");
		      continue;
		    }
		  if (jcode == NSYS_JOURNAL_FILLEDBUFF)
		    {
		      nsys_warning(NSYS_WARN_UNUSUAL, "RTP journal too big");
		      return (nsys_bufflen ? NSYS_MIDIEVENTS : NSYS_DONE);
		    }

		  /* NSYS_JOURNAL_RECOVERED falls through */
		}

	      /****************/
	      /* RTP payload  */
	      /****************/

	      mlen = (*p) & NSYS_SM_MLENMASK;
	      has_ts = (*p) & NSYS_SM_CHKZ;

	      if ((*(p++)) & NSYS_SM_CHKB)
		{
		  if (!(--len))
		    {
		      nsys_warning(NSYS_WARN_UNUSUAL, "RTP MIDI header truncation");
		      continue;            
		    }
		  mlen = (*(p++)) + (mlen << 8);
		}

	      if (!mlen)
		continue;            

	      if (mlen > (--len))
		{
		  nsys_warning(NSYS_WARN_UNUSUAL, "RTP MIDI section truncation");
		  continue;            
		}

	      oldcmd = cmd = 0;
	      pmax = p + mlen;

	      while (p < pmax)
		{
		  if ((NSYS_BUFFSIZE - nsys_bufflen) < 4)
		    {
		      nsys_warning(NSYS_WARN_UNUSUAL, 
				   "RTP journal truncated MIDI commands");
		      return (nsys_bufflen ? NSYS_MIDIEVENTS : NSYS_DONE);	
		    }

		  /************************************************/
		  /* delta-time skip-over, null command skip-over */
		  /************************************************/

		  if (has_ts)
		    {
		      dsize = 0;

		      while ((*(p++)) & NSYS_SM_DTIME)
			{
			  dsize++;
			  if (p >= pmax)
			    break;
			}
			
		      if (dsize > 3)
			{
			  nsys_warning(NSYS_WARN_UNUSUAL, 
				       "RTP MIDI delta-time syntax error");
			  break;            
			}
		      
		      if (p >= pmax)
			break;        /* null-command skip-over */
		    }

		  has_ts = 1;

		  /*********************************************/
		  /* running status and MIDI System skip-overs */
		  /*********************************************/

		  if ((*p) >= CSYS_MIDI_NOTEOFF)
		    {
		      oldcmd = cmd;
		      if (((*p) != CSYS_MIDI_SYSTEM_SYSEX_START) &&
			  ((*p) != CSYS_MIDI_SYSTEM_SYSEX_END))
			cmd = (*(p++));
		      else
			{
			  /* only parse unsegmented Sysex commands (for now) */
			  
			  skip = ((*(p++)) != CSYS_MIDI_SYSTEM_SYSEX_START);

			  if (!skip)
			    {
			      sysidx = 0;
			      while ((p < pmax) &&
				     (sysidx < NSYS_NETIN_SYSBUFF) &&
				     ((*p) != CSYS_MIDI_SYSTEM_SYSEX_START) &&
				     ((*p) != CSYS_MIDI_SYSTEM_SYSEX_END) &&
				     ((*p) != CSYS_MIDI_SYSTEM_UNUSED1) &&
				     ((*p) != CSYS_MIDI_SYSTEM_UNUSED2))
				sysbuff[sysidx++] = *(p++);
			      
			      skip = (((*p) != CSYS_MIDI_SYSTEM_SYSEX_END) &&
				      ((*p) != CSYS_MIDI_SYSTEM_UNUSED1) &&
				      ((*p) != CSYS_MIDI_SYSTEM_UNUSED2));
			      
			      if (!skip)
				skip = nsys_sysex_parse(&cmd, &ndata, &vdata,
							sysbuff, sysidx);
			    }

			  if (skip)
			    {
			      nsys_warning(NSYS_WARN_UNUSUAL, 
					   "unknown arbitrary-length "
					   "System command in stream");
			      
			      cmd = 0;    /* cancel running status */

			      while ((p < pmax) && 
				     ((*p) != CSYS_MIDI_SYSTEM_SYSEX_START) &&
				     ((*p) != CSYS_MIDI_SYSTEM_SYSEX_END) &&
				     ((*p) != CSYS_MIDI_SYSTEM_UNUSED1) &&
				     ((*p) != CSYS_MIDI_SYSTEM_UNUSED2))
				p++;
			      
			      if (p < pmax)
				p++;
			      
			      continue;   /* process next command in list */
			    }

			  if (p < pmax)
			    p++;         /* skip sysex terminator */
			}
		    }

		  if (cmd == 0)
		    {
		      while ((p < pmax) && ((*p) < CSYS_MIDI_NOTEOFF))
			p++;

		      nsys_warning(NSYS_WARN_UNUSUAL, "RTP running status error");
		      continue;                     
		    }

		  /**************************************************/
		  /* skip late NoteOn commands w/ non-zero velocity */
		  /**************************************************/

		  if ((sptr->ontime == 0) &&
		      ((0xF0u & cmd) == CSYS_MIDI_NOTEON) && p[1])
		    {
		      p += 2;
		      continue;
		    }

		  /**********************************************/
		  /* put new command in buffer, track its state */
		  /**********************************************/

		  nsys_buff[nsys_bufflen++] = cmd;

		  if (((0xF0u & cmd) < CSYS_MIDI_SYSTEM) && (cmd >= CSYS_MIDI_NOTEOFF))
		    {
		      if (p < pmax)
			nsys_buff[nsys_bufflen++] = (ndata = *(p++));
		      if (((0xF0u & cmd) != CSYS_MIDI_PROGRAM) && 
			  ((0xF0u & cmd) != CSYS_MIDI_CTOUCH) && (p < pmax))
			nsys_buff[nsys_bufflen++] = (vdata = *(p++));
		    }
		  else
		    {
		      if (cmd >= CSYS_MIDI_SYSTEM)
			{
			  if ((cmd == CSYS_MIDI_SYSTEM_QFRAME) || 
			      (cmd == CSYS_MIDI_SYSTEM_SONG_SELECT) ||
			      (cmd == CSYS_MIDI_SYSTEM_SONG_PP))
			    {
			      if (p < pmax)
				nsys_buff[nsys_bufflen++] = (ndata = *(p++));
			      if ((cmd == CSYS_MIDI_SYSTEM_SONG_PP) && (p < pmax))
				nsys_buff[nsys_bufflen++] = (vdata = *(p++));
			    }

			  if ((cmd == CSYS_MIDI_SYSTEM_UNUSED1) ||
			      (cmd == CSYS_MIDI_SYSTEM_UNUSED2))
			    {
			      ndata = CSYS_MIDI_SYSTEM_SYSEX_END;
			      vdata = CSYS_MIDI_SYSTEM_SYSEX_END;

			      if ((p < pmax) && ((*p) < CSYS_MIDI_NOTEOFF))
				nsys_buff[nsys_bufflen++] = (ndata = *(p++));
			      if ((p < pmax) && ((*p) < CSYS_MIDI_NOTEOFF))
				nsys_buff[nsys_bufflen++] = (vdata = *(p++));

			      while ((p < pmax) && ((*p) < CSYS_MIDI_NOTEOFF))
				p++;
			      if ((p < pmax) && ((*p) == CSYS_MIDI_SYSTEM_SYSEX_END))
				p++;
			    }
			}
		      else
			switch(cmd) {
			case CSYS_MIDI_GMRESET:
			  nsys_buff[nsys_bufflen++] = ndata; 
			  break;
			case CSYS_MIDI_MVOLUME:
			case CSYS_MIDI_MANUEX:
			  nsys_buff[nsys_bufflen++] = ndata; 
			  nsys_buff[nsys_bufflen++] = vdata; 
			  break;
			}
		    }

		  if (fec)
		    nsys_netin_journal_trackstate(sptr, cmd, ndata, vdata);

		  if (cmd >= CSYS_MIDI_SYSTEM_CLOCK)
		    cmd = oldcmd;  /* real-time does not cancel running status */
		  else
		    if ((cmd >= CSYS_MIDI_SYSTEM) || (cmd < CSYS_MIDI_NOTEOFF))
		      cmd = 0;   /* system common cancels running status */
		  
		  nsys_buff[nsys_bufflen++] = sptr->mset;
		}

	      /* if no room left for another MIDI command, leave */
	      
	      if ((NSYS_BUFFSIZE - nsys_bufflen) < NSYS_SM_EXPANDMAX)
		return (nsys_bufflen ? NSYS_MIDIEVENTS : NSYS_DONE);

	      continue;
	    }

	  /***********************/
	  /* handle RTCP packets */
	  /***********************/

	  if ((fd == nsys_rtcp_fd) && 
	      (NSYS_RTCPVAL_BYTE1 == (packet[NSYS_RTCPLOC_BYTE1] 
				      & NSYS_RTCPVAL_COOKIEMASK)))
	    {
	      if ((len -= NSYS_RTPSIZE_DIGEST) <= 0)
		{
		  nsys_warning(NSYS_WARN_UNUSUAL, "RTCP packet truncated");
		  continue;
		}
	      
	      if (nsys_msession)
		{
		  memcpy(&ssrc, &(packet[NSYS_RTCPLOC_SSRC]), sizeof(long));
		  memcpy(&(packet[NSYS_RTCPLOC_SSRC]), &nsys_myssrc_net, 
			 sizeof(long));
		}

	      nsys_hmac_md5(packet, len, nsys_keydigest, digest);

	      if (memcmp(&(packet[len]), digest, NSYS_RTPSIZE_DIGEST))
		{		  
		  nsys_warning(NSYS_WARN_UNUSUAL, 
			       "Discarding unauthorized RTCP packet");
		  continue;
		}

	      if (nsys_msession)
		memcpy(&(packet[NSYS_RTCPLOC_SSRC]), &ssrc, sizeof(long));
	      
	      sptr = nsys_netin_rtcp(packet, len, &ipaddr);

	      if (nsys_feclevel)
		nsys_netin_journal_trimstate(sptr);
	
	      if (nsys_powerup_mset)
		{
		  nsys_netin_clear_mset(nsys_buff, &nsys_bufflen,
					NSYS_BUFFSIZE);
		  if ((NSYS_BUFFSIZE - nsys_bufflen) < NSYS_SM_EXPANDMAX)
		    return (nsys_bufflen ? NSYS_MIDIEVENTS : NSYS_DONE);
		}
	      continue;
	    }
	  
	  /***********************/
	  /* handle SIP packets  */
	  /***********************/

	  packet[len] = '\0';  /* null-terminate string */

	  if (sscanf((char *) packet, "SIP/2.0 %hu", &status) == 1)
	    nsys_netin_reply(fd, &ipaddr, packet, status);
	  else
	    if (packet[NSYS_RTPLOC_BYTE1] == 'I')
	      {
		nsys_netin_invite(fd,  &ipaddr, packet);
		if (nsys_powerup_mset)
		  {
		    nsys_netin_clear_mset(nsys_buff, &nsys_bufflen,
					  NSYS_BUFFSIZE);
		    if ((NSYS_BUFFSIZE - nsys_bufflen) < NSYS_SM_EXPANDMAX)
		      return (nsys_bufflen ? NSYS_MIDIEVENTS : NSYS_DONE);
		  }
	      }
	}
    } 
}


/****************************************************************/
/*          returns the next network MIDI command               */
/****************************************************************/

int nsys_midievent(unsigned char * cmd, unsigned char * ndata,
                    unsigned char * vdata, unsigned short * extchan)

{
  *cmd = nsys_buff[nsys_buffcnt];

  if (((*cmd) < CSYS_MIDI_SYSTEM) && ((*cmd) >= CSYS_MIDI_NOTEOFF))
    {
      *extchan = 0x0Fu & nsys_buff[nsys_buffcnt++];
      *ndata = nsys_buff[nsys_buffcnt++];

      if ((((*cmd) & 0xF0u) != CSYS_MIDI_PROGRAM) && 
	  (((*cmd) & 0xF0u) != CSYS_MIDI_CTOUCH))
	*vdata = nsys_buff[nsys_buffcnt++];
    }
  else
    {
      *extchan = 0;
      nsys_buffcnt++;  /* skip over command octet */

      if ((*cmd) >= CSYS_MIDI_NOTEOFF)
	{
	  if ((*cmd) < CSYS_MIDI_SYSTEM_SYSEX_END)
	    {
	      if (((*cmd) == CSYS_MIDI_SYSTEM_QFRAME) || 
		  ((*cmd) == CSYS_MIDI_SYSTEM_SONG_SELECT) ||
		  ((*cmd) == CSYS_MIDI_SYSTEM_SONG_PP))
		*ndata = nsys_buff[nsys_buffcnt++];
	      
	      if ((*cmd) == CSYS_MIDI_SYSTEM_SONG_PP)
		*vdata = nsys_buff[nsys_buffcnt++];

	      if ((*cmd == CSYS_MIDI_SYSTEM_UNUSED1) || 
		  (*cmd == CSYS_MIDI_SYSTEM_UNUSED2))
		{
		  /* logic: don't take mset octet */
		  /*   assumes 0 <= mset <= 127   */
 
		  if (((nsys_buffcnt + 1) < nsys_bufflen) && 
		      !(nsys_buff[nsys_buffcnt + 1] & 0x80u))
		    *ndata = nsys_buff[nsys_buffcnt++];
		  else
		    *ndata = CSYS_MIDI_SYSTEM_SYSEX_END;

		  if (((nsys_buffcnt + 1) < nsys_bufflen) && 
		      !(nsys_buff[nsys_buffcnt + 1] & 0x80u))
		    *vdata = nsys_buff[nsys_buffcnt++];
		  else
		    *vdata = CSYS_MIDI_SYSTEM_SYSEX_END;
		}
	    }
	}
      else
	{
	  *ndata = nsys_buff[nsys_buffcnt++];
	  if ((*cmd) != CSYS_MIDI_GMRESET)
	    *vdata = nsys_buff[nsys_buffcnt++];
	}
    }

  *extchan += (NSYS_NETSTART - CSYS_MIDI_NUMCHAN + 
	       (nsys_buff[nsys_buffcnt++] << 4));

  return (nsys_buffcnt == nsys_bufflen) ? NSYS_DONE : NSYS_MIDIEVENTS;
}

/****************************************************************/
/*              detects special System Exclusive commands      */
/****************************************************************/

int nsys_sysex_parse(unsigned char * cmd, unsigned char *ndata,
		     unsigned char * vdata, unsigned char * sysbuff, 
		     int sysidx)

{

  switch (sysidx) {
  case 4:   
    if ((sysbuff[0] == 0x7Eu) && (sysbuff[1] == 0x7Fu) && 
	(sysbuff[2] == 0x09u) && 
	((sysbuff[3] == 0x01u) || (sysbuff[3] == 0x02u)))
      {
	*cmd = CSYS_MIDI_GMRESET;
	*ndata = sysbuff[3];
	return 0;
      }
    break;
  case 6:   
    if ((sysbuff[0] == 0x7Fu) && (sysbuff[1] == 0x7Fu) &&
	(sysbuff[2] == 0x04u) && (sysbuff[3] == 0x01u) &&
	(sysbuff[4] < 0x80u)  && (sysbuff[5] < 0x80u))
      {
	*cmd = CSYS_MIDI_MVOLUME;
	*ndata = sysbuff[4];
	*vdata = sysbuff[5];
	return 0;
      }
    break;
  case 8:   
    if ((sysbuff[0] == 0x43u) && (sysbuff[1] == 0x73u) &&
	(sysbuff[2] == 0x7Fu) && (sysbuff[3] == 0x32u) &&
	(sysbuff[4] == 0x11u) && (sysbuff[5] < 0x80u) &&
	(sysbuff[6] < 0x80u) && (sysbuff[7] < 0x80u))
      {
	*cmd = CSYS_MIDI_MANUEX;
	*ndata = sysbuff[6];
	*vdata = sysbuff[7];
	return 0;
      }
    break; 
  }

  return 1;
}


/****************************************************************/
/*                    sends RTP packets                         */
/****************************************************************/

void nsys_midisend(unsigned char cmd, unsigned char ndata,
		  unsigned char vdata, unsigned short extchan)

{
  int retry = 0;
  int jsize = 0;
  unsigned char size, cbyte;
  unsigned long tstamp;
  unsigned short seqnum;
  struct nsys_source * sptr;
  struct sockaddr * addr;
  int alt = 0;
  int jstart, dstart;

  /**********************/
  /* MIDI preprocessing */
  /**********************/

  if ((cbyte = cmd & 0xF0u) < CSYS_MIDI_SYSTEM)
    {
      if (cbyte >= CSYS_MIDI_NOTEOFF)
	{
	  if ((cbyte != CSYS_MIDI_PROGRAM) && (cbyte != CSYS_MIDI_CTOUCH))
	    size = 3;
	  else
	    size = 2;
	  cmd = cbyte | (((unsigned char)extchan) & 0x0Fu);
	}
      else
	switch (cmd) {
	case CSYS_MIDI_MANUEX:
	  size = 10;
	  break;
	case CSYS_MIDI_MVOLUME:
	  size = 8;
	  break;
	case CSYS_MIDI_GMRESET:
	  size = 6;
	  break;
	default:
	  size = 0;
	  break;
	}
    }
  else
    {
      if (cmd != CSYS_MIDI_SYSTEM_SYSEX_END)
	{
	  if ((cmd != CSYS_MIDI_SYSTEM_QFRAME) && (cmd != CSYS_MIDI_SYSTEM_SONG_SELECT)
	      && (cmd != CSYS_MIDI_SYSTEM_SONG_PP))
	    {
	      size = 1;

	      if ((cmd == CSYS_MIDI_SYSTEM_UNUSED1) || (cmd == CSYS_MIDI_SYSTEM_UNUSED2))
		size = 1 + ((ndata != CSYS_MIDI_SYSTEM_SYSEX_END) + 
			    (vdata != CSYS_MIDI_SYSTEM_SYSEX_END)) + 1;
	    }
	  else
	    size = (cmd != CSYS_MIDI_SYSTEM_SONG_PP) ? 2 : 3;
	}
      else
	{ 
	  if (nsys_feclevel)
	    nsys_netout_journal_addhistory(cmd, ndata, vdata);
	  return;
	}
    }

  if (nsys_msession && 
      ((cbyte == CSYS_MIDI_NOTEON) || (cbyte == CSYS_MIDI_NOTEOFF)) && 
      ((ndata += NSYS_MSESSION_INTERVAL) > 0x7Fu))
    ndata = 0x7Fu;

  /*************************/
  /* leave if no receivers */
  /*************************/

  if ((sptr = nsys_srcroot) == NULL)
    { 
      if (nsys_feclevel)
	nsys_netout_journal_addhistory(cmd, ndata, vdata);
      return;
    }

  /**********************/
  /* fill in RTP header */
  /**********************/

  nsys_netout_rtp_packet[NSYS_RTPLOC_PTYPE] = sptr->ptype | nsys_netout_markbit;

  tstamp = htonl((unsigned long)(nsys_netout_tstamp));
  memcpy(&(nsys_netout_rtp_packet[NSYS_RTPLOC_TSTAMP]), &tstamp, sizeof(long));

  nsys_netout_seqnum = ((nsys_netout_seqnum != NSYS_RTPSEQ_HIGHEST) ?
			(nsys_netout_seqnum + 1) : 1); 
  seqnum = htons((unsigned short)(nsys_netout_seqnum & 0x0000FFFF));
  memcpy(&(nsys_netout_rtp_packet[NSYS_RTPLOC_SEQNUM]), &seqnum, 
	 sizeof(short));

  /**************************************/
  /* fill in command section of payload */
  /**************************************/

  nsys_netout_rtp_packet[NSYS_RTPLEN_HDR] = size | nsys_netout_sm_header;

  if (cbyte >= CSYS_MIDI_NOTEOFF)
    {
      nsys_netout_rtp_packet[NSYS_RTPLEN_HDR + 1] = cmd;
      nsys_netout_rtp_packet[NSYS_RTPLEN_HDR + 2] = ndata;
      nsys_netout_rtp_packet[NSYS_RTPLEN_HDR + 3] = vdata;

      if ((cmd == CSYS_MIDI_SYSTEM_UNUSED1) || (cmd == CSYS_MIDI_SYSTEM_UNUSED2))
	nsys_netout_rtp_packet[NSYS_RTPLEN_HDR + size] = CSYS_MIDI_SYSTEM_SYSEX_END;
    }
  else
    switch (cmd) {
    case CSYS_MIDI_MANUEX:
      memcpy(&(nsys_netout_rtp_packet[NSYS_RTPLEN_HDR + 1]),
	     nsys_netout_sysconst_manuex, size);
      nsys_netout_rtp_packet[NSYS_RTPLEN_HDR + 8] = ndata;
      nsys_netout_rtp_packet[NSYS_RTPLEN_HDR + 9] = vdata;
      break;
    case CSYS_MIDI_MVOLUME:
      memcpy(&(nsys_netout_rtp_packet[NSYS_RTPLEN_HDR + 1]),
	     nsys_netout_sysconst_mvolume, size);
      nsys_netout_rtp_packet[NSYS_RTPLEN_HDR + 6] = ndata;
      nsys_netout_rtp_packet[NSYS_RTPLEN_HDR + 7] = vdata;
      break;
    case CSYS_MIDI_GMRESET:
      memcpy(&(nsys_netout_rtp_packet[NSYS_RTPLEN_HDR + 1]),
	     nsys_netout_sysconst_gmreset, size);
      nsys_netout_rtp_packet[NSYS_RTPLEN_HDR + 5] = ndata;
      break;
    }

  /**************************************/
  /* fill in journal section of payload */
  /**************************************/

  if (nsys_feclevel)
    {
      jstart = NSYS_RTPLEN_HDR + 1 + size;
      jsize = nsys_netin_journal_create(&(nsys_netout_rtp_packet[jstart]),
					NSYS_UDPMAXSIZE - NSYS_MD5_LENGTH 
					- jstart);
      nsys_netout_journal_addstate(cmd, ndata, vdata);
    }

  /********************************/
  /* add authentication signature */
  /********************************/

  dstart = NSYS_RTPLEN_HDR + 1 + size + jsize;
  nsys_hmac_md5(nsys_netout_rtp_packet, dstart, nsys_keydigest, 
		&(nsys_netout_rtp_packet[dstart]));

  /********************************************/
  /* calculate packet size, update statistics */
  /********************************************/

  nsys_sent_this = 1;
  nsys_sent_packets++;
  nsys_sent_octets += (size + 1 + jsize + NSYS_RTPSIZE_DIGEST);
  size += (NSYS_RTPLEN_HDR + 1 + jsize + NSYS_RTPSIZE_DIGEST);     

  /*********************************************************/
  /* if loss model is active, don't always send the packet */
  /*********************************************************/

  if (NSYS_LOSSMODEL == NSYS_LOSSMODEL_ON)
    if (rand() > (int)((1.0F - NSYS_LOSSMODEL_LOSSRATE)*RAND_MAX))
      return;

  /***************************/
  /* send packet to everyone */
  /***************************/

  do 
    {
      addr = (struct sockaddr *) (alt ? sptr->alt_rtp_addr : sptr->rtp_addr);

      /* when multiple payload types supported, see if *p*[NSYS_RTPLOC_PTYPE] */
      /* differs from (sptr->ptype | nsys_netout_markbit) and reauthenticate  */

      if (sendto(nsys_rtp_fd, nsys_netout_rtp_packet, size, 0, addr, 
		 sizeof(struct sockaddr)) == -1)
	{
	  if (errno == EAGAIN)
	    continue;
	  
	  if ((errno == EINTR) || (errno == ENOBUFS))
	    {
	      if (++retry > NSYS_MAXRETRY)
		NSYS_ERROR_TERMINATE("Too many I/O retries -- nsys_netout_newdata");
	      continue;         
	    }
	  
	  NSYS_ERROR_TERMINATE("Error writing Internet socket");
	}

      if (!(alt = ((!alt) && sptr->alt_rtp_addr)))
	sptr = sptr->next;
    } 
  while (alt || (sptr != nsys_srcroot));

}

/****************************************************************/
/*              called at the end of control processing         */
/****************************************************************/

void nsys_endcycle(void)

{

 if (nsys_netout_jsend_guard_send) 
   nsys_midisend(CSYS_MIDI_NOOP, 0, 0, 0);
 
 if (nsys_graceful_exit && !graceful_exit)
   graceful_exit = 1;

}

/* end  Sfront-specific network functions */

