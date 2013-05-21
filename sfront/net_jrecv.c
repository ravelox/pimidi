
/*
#    Sfront, a SAOL to C translator    
#    This file: Network library -- receiver journal functions
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
/*     high-level functions: receiving recovery journals        */
/*______________________________________________________________*/


/****************************************************************/
/*          main routine for parsing recovery journal           */
/****************************************************************/

int nsys_netin_journal_recovery(nsys_source * sptr, int rtpcode, 
				unsigned char * packet,
				int numbytes, unsigned char * buff,  
				long * fill, long size)

{
  nsys_netout_jrecv_state * jrecv;
  nsys_netout_jrecv_system_state * jrecvsys;
  int numchan, bflag, i, j;
  short chanlen, syslen, loglen, cmdlen, paramlen;
  unsigned char chan, low, high, many, sysj, chanj;
  unsigned char chapters, schapters, dchapters;
  unsigned char * checkptr, * p, * ps;

  if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
    {
      printf("\ndoing recovery for mset%i (fill: %i)\n", sptr->mset, 
	     (j = (*fill))); 

#if (NSYS_LATENOTES_DEBUG == NSYS_LATENOTES_DEBUG_ON)
      if (sptr->tm_margin)
	printf("Time: %f\n", (nsys_netout_tstamp - sptr->tm_first)/ARATE);
#endif

      fflush(stdout);
    }

  if ((numbytes -= 1) < NSYS_SM_JH_SIZE)
    return NSYS_JOURNAL_CORRUPTED;

  cmdlen = (*packet) & NSYS_SM_MLENMASK;

  if ((*packet) & NSYS_SM_CHKB)
    {
      if ((numbytes -= 1) < NSYS_SM_JH_SIZE)
	return NSYS_JOURNAL_CORRUPTED;

      cmdlen = packet[1] + (cmdlen << 8);
      packet += 1;
    }

  if ((numbytes -= cmdlen) < NSYS_SM_JH_SIZE)
    return NSYS_JOURNAL_CORRUPTED;

  packet += 1 + cmdlen;
  many = (rtpcode == NSYS_RTPCODE_LOSTMANY);

  sysj = (packet[NSYS_SM_JH_LOC_FLAGS] & NSYS_SM_JH_CHKY);
  chanj = (packet[NSYS_SM_JH_LOC_FLAGS] & NSYS_SM_JH_CHKA);

  if ((!sysj && !chanj) || 
      ((packet[NSYS_SM_JH_LOC_FLAGS] & NSYS_SM_CHKS) && (!many)))
    return NSYS_JOURNAL_RECOVERED;

  numchan = chanj ? (packet[NSYS_SM_JH_LOC_FLAGS] & NSYS_SM_JH_CHANMASK) + 1 : 0;
  checkptr = &(packet[NSYS_SM_JH_LOC_CHECK]);

  numbytes -= NSYS_SM_JH_SIZE;
  packet += NSYS_SM_JH_SIZE;

  if (sysj)
    {
      if (numbytes < NSYS_SM_SH_SIZE)
	return NSYS_JOURNAL_CORRUPTED;

      if ((jrecvsys = sptr->jrecvsys) == NULL)
	jrecvsys = sptr->jrecvsys = nsys_netin_newrecvsys();

      schapters = packet[NSYS_SM_SH_LOC_FLAGS];
      syslen = packet[NSYS_SM_SH_LOC_LENLSB] + ((schapters & NSYS_SM_SH_MSBMASK) << 8);

      if ((numbytes -= syslen) < 0)
	return NSYS_JOURNAL_CORRUPTED;

      ps = packet + NSYS_SM_SH_SIZE;
      packet += syslen;
      syslen -= NSYS_SM_SH_SIZE;

      /*************************************/
      /* chapter D: Simple System Commands */
      /*************************************/

      if (schapters & NSYS_SM_SH_CHKD)
	{
	  if ((syslen -= NSYS_SM_CD_SIZE_TOC) < 0)
	    return NSYS_JOURNAL_CORRUPTED;

	  dchapters = ps[NSYS_SM_CD_LOC_TOC];
	  ps += NSYS_SM_CD_SIZE_TOC;

	  if (dchapters & NSYS_SM_CD_TOC_CHKB)
	    {
	      if ((syslen -= NSYS_SM_CD_SIZE_RESET) < 0)
		return NSYS_JOURNAL_CORRUPTED;

	      if (((!(ps[0] & NSYS_SM_CHKS)) || many) &&
		  nsys_netin_jrec_reset(sptr, ps, jrecvsys, buff, fill, size))
		return NSYS_JOURNAL_FILLEDBUFF;

	      ps += NSYS_SM_CD_SIZE_RESET;
	    }

	  if (dchapters & NSYS_SM_CD_TOC_CHKG)
	    {
	      if ((syslen -= NSYS_SM_CD_SIZE_TUNE) < 0)
		return NSYS_JOURNAL_CORRUPTED;

	      if (((!(ps[0] & NSYS_SM_CHKS)) || many) &&
		  nsys_netin_jrec_tune(sptr, ps, jrecvsys, buff, fill, size))
		return NSYS_JOURNAL_FILLEDBUFF;

	      ps += NSYS_SM_CD_SIZE_TUNE;
	    }

	  if (dchapters & NSYS_SM_CD_TOC_CHKH)
	    {
	      if ((syslen -= NSYS_SM_CD_SIZE_SONG) < 0)
		return NSYS_JOURNAL_CORRUPTED;

	      if (((!(ps[0] & NSYS_SM_CHKS)) || many) &&
		  nsys_netin_jrec_song(sptr, ps, jrecvsys, buff, fill, size))
		return NSYS_JOURNAL_FILLEDBUFF;

	      ps += NSYS_SM_CD_SIZE_SONG;
	    }

	  if (dchapters & NSYS_SM_CD_TOC_CHKJ)
	    {
	      if ((syslen -= (NSYS_SM_CD_COMMON_TOC_SIZE +
			      NSYS_SM_CD_COMMON_LENGTH_SIZE)) < 0)
		return NSYS_JOURNAL_CORRUPTED;

	      loglen = ps[1];

	      if (((!(ps[0] & NSYS_SM_CHKS)) || many) &&
		  nsys_netin_jrec_scj(sptr, ps, jrecvsys, buff, fill, size))
		return NSYS_JOURNAL_FILLEDBUFF;

	      syslen -= (loglen - (NSYS_SM_CD_COMMON_TOC_SIZE +
				   NSYS_SM_CD_COMMON_LENGTH_SIZE));
	      ps += loglen;
	    }

	  if (dchapters & NSYS_SM_CD_TOC_CHKK)
	    {
	      if ((syslen -= (NSYS_SM_CD_COMMON_TOC_SIZE +
			      NSYS_SM_CD_COMMON_LENGTH_SIZE)) < 0)
		return NSYS_JOURNAL_CORRUPTED;

	      loglen = ps[1];

	      if (((!(ps[0] & NSYS_SM_CHKS)) || many) &&
		  nsys_netin_jrec_sck(sptr, ps, jrecvsys, buff, fill, size))
		return NSYS_JOURNAL_FILLEDBUFF;

	      syslen -= (loglen - (NSYS_SM_CD_COMMON_TOC_SIZE +
				   NSYS_SM_CD_COMMON_LENGTH_SIZE));
	      ps += loglen;
	    }

	  if (dchapters & NSYS_SM_CD_TOC_CHKY)
	    {
	      if ((syslen -= NSYS_SM_CD_REALTIME_TOC_SIZE) < 0)
		return NSYS_JOURNAL_CORRUPTED;

	      loglen = ps[0] & NSYS_SM_CD_REALTIME_LENGTH_MASK;

	      if (((!(ps[0] & NSYS_SM_CHKS)) || many) &&
		  nsys_netin_jrec_rty(sptr, ps, jrecvsys, buff, fill, size))
		return NSYS_JOURNAL_FILLEDBUFF;

	      syslen -= (loglen - NSYS_SM_CD_REALTIME_TOC_SIZE);
	      ps += loglen;
	    }

	  if (dchapters & NSYS_SM_CD_TOC_CHKZ)
	    {
	      if ((syslen -= NSYS_SM_CD_REALTIME_TOC_SIZE) < 0)
		return NSYS_JOURNAL_CORRUPTED;

	      loglen = ps[0] & NSYS_SM_CD_REALTIME_LENGTH_MASK;

	      if (((!(ps[0] & NSYS_SM_CHKS)) || many) &&
		  nsys_netin_jrec_rtz(sptr, ps, jrecvsys, buff, fill, size))
		return NSYS_JOURNAL_FILLEDBUFF;

	      syslen -= (loglen - NSYS_SM_CD_REALTIME_TOC_SIZE);
	      ps += loglen;
	    }
	}

      /***************************/
      /* chapter V: Active Sense */
      /***************************/

      if (schapters & NSYS_SM_SH_CHKV)
	{
	  if ((syslen -= NSYS_SM_CV_SIZE) < 0)
	    return NSYS_JOURNAL_CORRUPTED;

	  if (((!(ps[NSYS_SM_CV_LOC_COUNT] & NSYS_SM_CHKS)) || many) &&
	      nsys_netin_jrec_sense(sptr, ps, jrecvsys, buff, fill, size))
	    return NSYS_JOURNAL_FILLEDBUFF;

	  ps += NSYS_SM_CV_SIZE;
	}

      /************************/
      /* chapter Q: Sequencer */
      /************************/

      if (schapters & NSYS_SM_SH_CHKQ)
	{
	  if ((syslen -= NSYS_SM_CQ_SIZE_HDR) < 0)
	    return NSYS_JOURNAL_CORRUPTED;

	  loglen = 0;

	  if (ps[NSYS_SM_CQ_LOC_HDR] & NSYS_SM_CQ_HDR_CHKC)
	    loglen += NSYS_SM_CQ_SIZE_CLOCK;

	  if (ps[NSYS_SM_CQ_LOC_HDR] & NSYS_SM_CQ_HDR_CHKT)
	    loglen += NSYS_SM_CQ_SIZE_TIMETOOLS;

	  if ((syslen -= loglen) < 0)
	    return NSYS_JOURNAL_CORRUPTED;

	  loglen += NSYS_SM_CQ_SIZE_HDR;

	  if (((!(ps[NSYS_SM_CQ_LOC_HDR] & NSYS_SM_CHKS)) || many) &&
	      nsys_netin_jrec_sequence(sptr, ps, jrecvsys, buff, fill, size))
	    return NSYS_JOURNAL_FILLEDBUFF;

	  ps += loglen;
	}

      /*****************************/
      /* chapter F: MIDI Time Code */
      /*****************************/

      if (schapters & NSYS_SM_SH_CHKF)
	{
	  if ((syslen -= NSYS_SM_CF_SIZE_HDR) < 0)
	    return NSYS_JOURNAL_CORRUPTED;

	  loglen = 0;

	  if (ps[NSYS_SM_CF_LOC_HDR] & NSYS_SM_CF_HDR_CHKC)
	    loglen += NSYS_SM_CF_SIZE_COMPLETE;

	  if (ps[NSYS_SM_CF_LOC_HDR] & NSYS_SM_CF_HDR_CHKP)
	    loglen += NSYS_SM_CF_SIZE_PARTIAL;

	  if ((syslen -= loglen) < 0)
	    return NSYS_JOURNAL_CORRUPTED;

	  loglen += NSYS_SM_CF_SIZE_HDR;

	  if (((!(ps[NSYS_SM_CF_LOC_HDR] & NSYS_SM_CHKS)) || many) &&
	      nsys_netin_jrec_timecode(sptr, ps, jrecvsys, buff, fill, size))
	    return NSYS_JOURNAL_FILLEDBUFF;

	  ps += loglen;
	}

      /*******************************/
      /* chapter X: System Exclusive */
      /*******************************/

      if ((schapters & NSYS_SM_SH_CHKX) && syslen && 
	  (!(ps[NSYS_SM_CX_LOC_HDR] & NSYS_SM_CHKS) || many))
	{
	  if (nsys_netin_jrec_sysex(sptr, ps, syslen, jrecvsys, buff, fill, size))
	    return NSYS_JOURNAL_FILLEDBUFF;
	}
    }

  while (numchan--)
    {
      if (numbytes < NSYS_SM_CH_SIZE)
	return NSYS_JOURNAL_CORRUPTED;

      chan = ((packet[NSYS_SM_CH_LOC_FLAGS] & NSYS_SM_CH_CHANMASK)
	      >> NSYS_SM_CH_CHANSHIFT);

      if ((jrecv = sptr->jrecv[chan]) == NULL)
	jrecv = sptr->jrecv[chan] = nsys_netin_newrecv(chan);

      chapters = packet[NSYS_SM_CH_LOC_TOC];
      chanlen = *((short *)&(packet[NSYS_SM_CH_LOC_FLAGS]));
      chanlen = ntohs((unsigned short)chanlen) & NSYS_SM_CH_LENMASK;
      
      if ((numbytes -= chanlen) < 0)
	return NSYS_JOURNAL_CORRUPTED;

      p = packet + NSYS_SM_CH_SIZE;
      packet += chanlen;

      /*****************************/
      /* chapter P: Program Change */
      /*****************************/

      if (chapters & NSYS_SM_CH_TOC_SETP)
	{
	  if ((chanlen -= NSYS_SM_CP_SIZE) < 0)
	    return NSYS_JOURNAL_CORRUPTED;

	  if (((!(p[NSYS_SM_CP_LOC_PROGRAM] & NSYS_SM_CHKS)) || many) && 
	      nsys_netin_jrec_program(sptr, p, jrecv, buff, fill, size))
	    return NSYS_JOURNAL_FILLEDBUFF;

	  p += NSYS_SM_CP_SIZE;
	}

      /**************************/
      /* chapter C: Controllers */
      /**************************/

      if (chapters & NSYS_SM_CH_TOC_SETC)
	{
	  if ((chanlen -= NSYS_SM_CC_HDRSIZE) < 0)
	    return NSYS_JOURNAL_CORRUPTED;

	  loglen = (p[NSYS_SM_CC_LOC_LENGTH] & NSYS_SM_CLRS) + 1;

	  if ((chanlen -= loglen*NSYS_SM_CC_LOGSIZE) < 0)
	    return NSYS_JOURNAL_CORRUPTED;
	  
	  if (((!(p[NSYS_SM_CC_LOC_LENGTH] & NSYS_SM_CHKS)) || many) &&
	      nsys_netin_jrec_control(sptr, p, jrecv, loglen, many,
				      buff, fill, size))
	    return NSYS_JOURNAL_FILLEDBUFF;

	  p += (NSYS_SM_CC_HDRSIZE + loglen*NSYS_SM_CC_LOGSIZE);
	}

      /*******************************/
      /* chapter M: Parameter System */
      /*******************************/
      
      if (chapters & NSYS_SM_CH_TOC_SETM)
	{
	  if (chanlen < NSYS_SM_CM_HDRSIZE)
	    return NSYS_JOURNAL_CORRUPTED;
	  
	  memcpy(&(paramlen), &(p[NSYS_SM_CM_LOC_HDR]), sizeof(short));
	  paramlen = ntohs(paramlen) & NSYS_SM_CM_LENMASK;
	  
	  if ((chanlen -= paramlen) < 0)
	    return NSYS_JOURNAL_CORRUPTED;

	  if (((!(p[NSYS_SM_CM_LOC_HDR] & NSYS_SM_CHKS)) || many) &&
	      nsys_netin_jrec_param(sptr, p, jrecv, paramlen, many,
				    buff, fill, size))
	    return NSYS_JOURNAL_FILLEDBUFF;

	  p += paramlen;
	}

      /**************************/
      /* chapter W: Pitch Wheel */
      /**************************/

      if (chapters & NSYS_SM_CH_TOC_SETW)
	{
	  if ((chanlen -= NSYS_SM_CW_SIZE) < 0)
	    return NSYS_JOURNAL_CORRUPTED;

	  if (((!(p[NSYS_SM_CW_LOC_FIRST] & NSYS_SM_CHKS)) || many) &&
	      nsys_netin_jrec_wheel(sptr, p, jrecv, buff, fill, size))
	    return NSYS_JOURNAL_FILLEDBUFF;

	  p += NSYS_SM_CW_SIZE;
	}

      /**************************/
      /* chapter N: Note On/Off */
      /**************************/

      if (chapters & NSYS_SM_CH_TOC_SETN)
	{
	  if ((chanlen -= NSYS_SM_CN_HDRSIZE) < 0)
	    return NSYS_JOURNAL_CORRUPTED;

	  loglen = p[NSYS_SM_CN_LOC_LENGTH] & NSYS_SM_CN_CLRB;
	  bflag = !(p[NSYS_SM_CN_LOC_LENGTH] & NSYS_SM_CN_CHKB);
	  low = ((p[NSYS_SM_CN_LOC_LOWHIGH] & NSYS_SM_CN_LOWMASK) >>
		 NSYS_SM_CN_LOWSHIFT);
	  high = (p[NSYS_SM_CN_LOC_LOWHIGH] & NSYS_SM_CN_HIGHMASK);

	  if ((loglen == 127) && (low == 15) && (high == 0))
	    loglen = 128;

	  if ((chanlen -= (loglen*NSYS_SM_CN_LOGSIZE + 
			   ((low <= high) ? (high - low + 1) : 0))) < 0)
	    return NSYS_JOURNAL_CORRUPTED;

	  p += NSYS_SM_CN_HDRSIZE;

	  if (loglen)
	    {
	      if (nsys_netin_jrec_notelog(sptr, p, jrecv, many, loglen,
					  checkptr, buff, fill, size))
		return NSYS_JOURNAL_FILLEDBUFF;
	      p += loglen*NSYS_SM_CN_LOGSIZE;
	    }
	  if ((bflag || many) && (low <= high))
	    {	      
	      if (nsys_netin_jrec_bitfield(sptr, p, jrecv, low, high,
					   buff, fill, size))
		return NSYS_JOURNAL_FILLEDBUFF;
	    }

	  p += (low <= high) ? (high - low + 1) : 0;
	}

      /**************************/
      /* chapter E: Note Extras */
      /**************************/
      
      if (chapters & NSYS_SM_CH_TOC_SETE)
	{
	  if ((chanlen -= NSYS_SM_CE_HDRSIZE) < 0)
	    return NSYS_JOURNAL_CORRUPTED;
	  
	  loglen = (p[NSYS_SM_CE_LOC_LENGTH] & NSYS_SM_CLRS) + 1;
	  
	  if ((chanlen -= loglen*NSYS_SM_CE_LOGSIZE) < 0)
	    return NSYS_JOURNAL_CORRUPTED;
	  
	  p += (NSYS_SM_CE_HDRSIZE + loglen*NSYS_SM_CE_LOGSIZE);
	}

      /****************************/
      /* chapter T: Channel Touch */
      /****************************/

      if (chapters & NSYS_SM_CH_TOC_SETT)
	{
	  if ((chanlen -= NSYS_SM_CT_SIZE) < 0)
	    return NSYS_JOURNAL_CORRUPTED;

	  if (((!(p[NSYS_SM_CT_LOC_PRESSURE] & NSYS_SM_CHKS)) || many) &&
	      nsys_netin_jrec_ctouch(sptr, p, jrecv, buff, fill, size))
	    return NSYS_JOURNAL_FILLEDBUFF;

	  p += NSYS_SM_CT_SIZE;
	}

      /**************************/
      /* chapter A: Poly Touch */
      /**************************/

      if (chapters & NSYS_SM_CH_TOC_SETA)
	{
	  if ((chanlen -= NSYS_SM_CA_HDRSIZE) < 0)
	    return NSYS_JOURNAL_CORRUPTED;
	  
	  loglen = (p[NSYS_SM_CA_LOC_LENGTH] & NSYS_SM_CLRS) + 1;

	  if ((chanlen -= loglen*NSYS_SM_CA_LOGSIZE) < 0)
	    return NSYS_JOURNAL_CORRUPTED;

	  if (((!(p[NSYS_SM_CA_LOC_LENGTH] & NSYS_SM_CHKS)) || many) &&
	      nsys_netin_jrec_ptouch(sptr, p, jrecv, loglen, many,
				     buff, fill, size))
	    return NSYS_JOURNAL_FILLEDBUFF;

	  p += (NSYS_SM_CA_HDRSIZE + loglen*NSYS_SM_CA_LOGSIZE);
	}
    }

  if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
    {
      printf("recovery (fill: %li):", (*fill));
      for (i = 0; j + i < (*fill); i++)
	printf(" %hhu", buff[j + i]);
      printf("\n");
      fflush(stdout);
    }

  return NSYS_JOURNAL_RECOVERED;
}

/****************************************************************/
/*            receiver state: add a new MIDI event              */
/****************************************************************/

void nsys_netin_journal_trackstate(nsys_source * sptr, unsigned char cmd,
				   unsigned char ndata, unsigned char vdata)

{
  nsys_netout_jrecv_state * jrecv;
  nsys_netout_jrecv_system_state * jrecvsys;
  long song_pp;

  if (cmd < CSYS_MIDI_NOTEOFF)      /* SysEx */
    {
      if ((jrecvsys = sptr->jrecvsys) == NULL)
	jrecvsys = sptr->jrecvsys = nsys_netin_newrecvsys();

      switch (cmd) {
      case CSYS_MIDI_GMRESET:
	nsys_netin_journal_clear_active(cmd);
	jrecvsys->chapterx_gmreset = ndata | NSYS_SM_RV_SETF;
	if (ndata == NSYS_SM_CX_GMRESET_ONVAL)
	  jrecvsys->chapterx_gmreset_on_count++;
	else
	  jrecvsys->chapterx_gmreset_off_count++; 
	break;
      case CSYS_MIDI_MVOLUME:
	jrecvsys->chapterx_mvolume_lsb = ndata | NSYS_SM_RV_SETF;
	jrecvsys->chapterx_mvolume_msb = vdata;
	break;
      }
      return;                      
    }

  if (cmd < ((unsigned char) CSYS_MIDI_SYSTEM))
    {
      if ((jrecv = sptr->jrecv[cmd & 0x0F]) == NULL)
	jrecv = sptr->jrecv[cmd & 0x0F] = nsys_netin_newrecv(cmd & 0x0F);

      switch (cmd & 0xF0) {

      case CSYS_MIDI_PROGRAM:
	jrecv->chapterp_program = vdata | NSYS_SM_RV_SETF;
	jrecv->chapterp_bank_msb = NSYS_SM_RV_CLRF & 
	  jrecv->chapterc_value[CSYS_MIDI_CC_BANKSELECT_MSB];
	jrecv->chapterp_bank_lsb = NSYS_SM_RV_CLRF & 
	  jrecv->chapterc_value[CSYS_MIDI_CC_BANKSELECT_LSB];
	break;

      case CSYS_MIDI_CC:
	switch (ndata) {
	case CSYS_MIDI_CC_ALLSOUNDOFF:
	case CSYS_MIDI_CC_ALLNOTESOFF:
	case CSYS_MIDI_CC_RESETALLCONTROL:
	case CSYS_MIDI_CC_OMNI_OFF:
	case CSYS_MIDI_CC_OMNI_ON:
	case CSYS_MIDI_CC_MONOMODE:
	case CSYS_MIDI_CC_POLYMODE:
	  jrecv->chapterc_value[ndata] = NSYS_SM_RV_SETF | 
	    (((jrecv->chapterc_value[ndata] & NSYS_SM_CC_ALTMOD) + 1)
	     & NSYS_SM_CC_ALTMOD);
	  switch (ndata) {
	  case CSYS_MIDI_CC_ALLSOUNDOFF:
	  case CSYS_MIDI_CC_ALLNOTESOFF:
	  case CSYS_MIDI_CC_OMNI_OFF:
	  case CSYS_MIDI_CC_OMNI_ON:
	  case CSYS_MIDI_CC_MONOMODE:
	  case CSYS_MIDI_CC_POLYMODE:
	    memset(jrecv->chaptern_ref, 0, NSYS_SM_CN_ARRAYSIZE);
	    jrecv->chaptert_pressure = 0;
	    break;
	  case CSYS_MIDI_CC_RESETALLCONTROL:

	    /* Chapter C -- update pedal count */

	    jrecv->chapterc_value[CSYS_MIDI_CC_SUSTAIN] = NSYS_SM_RV_SETF |
	      (((jrecv->chapterc_value[CSYS_MIDI_CC_SUSTAIN] & NSYS_SM_CC_ALTMOD) + 1)
	       & NSYS_SM_CC_ALTMOD);

	    /* Clear parameter transaction system */

	    jrecv->chapterm_nrpn_msb = jrecv->chapterm_nrpn_lsb = 0;
	    jrecv->chapterm_rpn_msb = jrecv->chapterm_rpn_lsb = 0;
	    jrecv->chapterm_state = NSYS_SM_CM_STATE_OFF;

	    /* Clear parameter system C-BUTTON counts */

	    memset(jrecv->chapterm_cbutton, 0, sizeof(short)*NSYS_SM_CM_ARRAYSIZE);

	    /* C-active Chapters:  W, T, and A */

	    jrecv->chapterw_first = 0;
	    jrecv->chapterw_second = 0;
	    memset(jrecv->chaptera_pressure, 0, NSYS_SM_CA_ARRAYSIZE);
	    jrecv->chaptert_pressure = 0;
	    break;
	  }
	  break;
	case CSYS_MIDI_CC_SUSTAIN:
	  if (((vdata >= 64) && !(jrecv->chapterc_value[ndata] & 0x01))
	      || ((vdata < 64) && (jrecv->chapterc_value[ndata] & 0x01)))
	    {
	      jrecv->chapterc_value[ndata] = NSYS_SM_RV_SETF |
		(((jrecv->chapterc_value[ndata] & NSYS_SM_CC_ALTMOD) + 1)
		 & NSYS_SM_CC_ALTMOD);
	    }
	  break;
	case CSYS_MIDI_CC_RPN_MSB:
	  jrecv->chapterm_state = NSYS_SM_CM_STATE_PENDING_RPN;
	  jrecv->chapterm_rpn_msb = vdata;
	  jrecv->chapterm_rpn_lsb = 0;
	  break;
	case CSYS_MIDI_CC_NRPN_MSB:
	  jrecv->chapterm_state = NSYS_SM_CM_STATE_PENDING_NRPN;
	  jrecv->chapterm_nrpn_msb = vdata;
	  jrecv->chapterm_nrpn_lsb = 0;
	  break;
	case CSYS_MIDI_CC_RPN_LSB:
	  if ((jrecv->chapterm_rpn_msb == CSYS_MIDI_RPN_NULL_MSB) &&
	      (vdata == CSYS_MIDI_RPN_NULL_LSB))
	    jrecv->chapterm_state = NSYS_SM_CM_STATE_OFF;
	  else
	    jrecv->chapterm_state = NSYS_SM_CM_STATE_RPN;
	  jrecv->chapterm_rpn_lsb = vdata;
	  break;
	case CSYS_MIDI_CC_NRPN_LSB:
	  if ((jrecv->chapterm_nrpn_msb == CSYS_MIDI_NRPN_NULL_MSB) &&
	      (vdata == CSYS_MIDI_NRPN_NULL_LSB))
	    jrecv->chapterm_state = NSYS_SM_CM_STATE_OFF;
	  else
	    jrecv->chapterm_state = NSYS_SM_CM_STATE_NRPN;
	  jrecv->chapterm_nrpn_lsb = vdata;
	  break;
	case CSYS_MIDI_CC_DATAENTRY_MSB:
	case CSYS_MIDI_CC_DATAENTRY_LSB:
	  switch(jrecv->chapterm_state) {
	  case NSYS_SM_CM_STATE_OFF:   
	    jrecv->chapterc_value[ndata] = vdata | NSYS_SM_RV_SETF;
	    break;    
	  case NSYS_SM_CM_STATE_PENDING_NRPN: 
	    jrecv->chapterm_state = NSYS_SM_CM_STATE_NRPN;
	    break;
	  case NSYS_SM_CM_STATE_NRPN:
	    break;
	  case NSYS_SM_CM_STATE_PENDING_RPN:
	    jrecv->chapterm_state = NSYS_SM_CM_STATE_RPN; /* fall through */
	  case NSYS_SM_CM_STATE_RPN:
	    if ((jrecv->chapterm_rpn_msb == 0) && 
		(jrecv->chapterm_rpn_lsb < NSYS_SM_CM_ARRAYSIZE))
	      {
		if (ndata == CSYS_MIDI_CC_DATAENTRY_LSB)
		  jrecv->chapterm_value_lsb[jrecv->chapterm_rpn_lsb] = vdata;
		else
		  {
		    jrecv->chapterm_value_msb[jrecv->chapterm_rpn_lsb] = vdata;
		    jrecv->chapterm_value_lsb[jrecv->chapterm_rpn_lsb] = 0;
		  }
		jrecv->chapterm_cbutton[jrecv->chapterm_rpn_lsb] = 0;
	      }
	    break;
	  }
	  break;
	case CSYS_MIDI_CC_DATAENTRYPLUS:
	case CSYS_MIDI_CC_DATAENTRYMINUS:
	  switch(jrecv->chapterm_state) {
	  case NSYS_SM_CM_STATE_OFF:   
	    jrecv->chapterc_value[ndata] = vdata | NSYS_SM_RV_SETF;
	    break;    
	  case NSYS_SM_CM_STATE_PENDING_NRPN: 
	    jrecv->chapterm_state = NSYS_SM_CM_STATE_NRPN;
	    break;
	  case NSYS_SM_CM_STATE_NRPN:
	    break;
	  case NSYS_SM_CM_STATE_PENDING_RPN: 
	    jrecv->chapterm_state = NSYS_SM_CM_STATE_RPN;  /* fall through */
	    break;
	  case NSYS_SM_CM_STATE_RPN:
	    if ((jrecv->chapterm_rpn_msb == 0) && 
		(jrecv->chapterm_rpn_lsb < NSYS_SM_CM_ARRAYSIZE))
	      {
		if (ndata == CSYS_MIDI_CC_DATAENTRYPLUS)
		  jrecv->chapterm_cbutton[jrecv->chapterm_rpn_lsb]++;
		else
		  jrecv->chapterm_cbutton[jrecv->chapterm_rpn_lsb]--;
		
		if (jrecv->chapterm_cbutton[jrecv->chapterm_rpn_lsb] > 
		    NSYS_SM_CM_BUTTON_LIMIT)
		  jrecv->chapterm_cbutton[jrecv->chapterm_rpn_lsb] =
		    NSYS_SM_CM_BUTTON_LIMIT;
		else
		  {
		    if (jrecv->chapterm_cbutton[jrecv->chapterm_rpn_lsb] < 
			- NSYS_SM_CM_BUTTON_LIMIT)
		      jrecv->chapterm_cbutton[jrecv->chapterm_rpn_lsb] =
			- NSYS_SM_CM_BUTTON_LIMIT;
		  }
	      }
	    break;
	  }
	  break;
	default:
	  jrecv->chapterc_value[ndata] = vdata | NSYS_SM_RV_SETF;
	  break;
	}
	break;

      case CSYS_MIDI_WHEEL:
	jrecv->chapterw_first = ndata | NSYS_SM_RV_SETF;
	jrecv->chapterw_second = vdata;
	break;

      case CSYS_MIDI_NOTEOFF:
	if (jrecv->chaptern_ref[ndata])
	  {
	    jrecv->chaptern_ref[ndata]--;
	    jrecv->chaptern_vel[ndata] = 0;
	  }
	break;

      case CSYS_MIDI_NOTEON:
	if (vdata)
	  {
	    if ((++(jrecv->chaptern_ref[ndata])) == 1)
	      {
		jrecv->chaptern_vel[ndata] = vdata;
		jrecv->chaptern_tstamp[ndata] = nsys_netout_tstamp;
		jrecv->chaptern_extseq[ndata] = sptr->hi_ext;
	      }
	    else
	      {
		jrecv->chaptern_vel[ndata] = 0;
		
		if ((jrecv->chaptern_ref[ndata]) == 0)
		  jrecv->chaptern_ref[ndata] = 255;
	      }
	  }
	else
	  if (jrecv->chaptern_ref[ndata])
	    {
	      jrecv->chaptern_ref[ndata]--;
	      jrecv->chaptern_vel[ndata] = 0;
	    }
	break;

      case CSYS_MIDI_CTOUCH:
	jrecv->chaptert_pressure = ndata | NSYS_SM_RV_SETF;
	break;
	
      case CSYS_MIDI_PTOUCH:
	jrecv->chaptera_pressure[ndata] = vdata | NSYS_SM_RV_SETF;
	break;
      }
    }
  else
    {
      if ((jrecvsys = sptr->jrecvsys) == NULL)
	jrecvsys = sptr->jrecvsys = nsys_netin_newrecvsys();

      switch (cmd) {
      case CSYS_MIDI_SYSTEM_RESET:
	nsys_netin_journal_clear_active(cmd);
	jrecvsys->chapterd_reset = NSYS_SM_RV_CLRF & (jrecvsys->chapterd_reset + 1);
	break;
      case CSYS_MIDI_SYSTEM_TUNE_REQUEST:
	jrecvsys->chapterd_tune = NSYS_SM_RV_CLRF & (jrecvsys->chapterd_tune + 1);
	break;
      case CSYS_MIDI_SYSTEM_SONG_SELECT:
	jrecvsys->chapterd_song = ndata;
	break;
      case CSYS_MIDI_SYSTEM_TICK:
	jrecvsys->chapterd_rty++;
	break;
      case CSYS_MIDI_SYSTEM_UNUSED3:
	jrecvsys->chapterd_rtz++;
	break;
      case CSYS_MIDI_SYSTEM_QFRAME:
	nsys_netin_track_timecode(jrecvsys, ndata);
	break;
      case CSYS_MIDI_SYSTEM_UNUSED1:
	jrecvsys->chapterd_scj_count++;
	jrecvsys->chapterd_scj_data1 = ndata;
	jrecvsys->chapterd_scj_data2 = vdata;
	break;
      case CSYS_MIDI_SYSTEM_UNUSED2:
	jrecvsys->chapterd_sck_count++;
	jrecvsys->chapterd_sck_data1 = ndata;
	jrecvsys->chapterd_sck_data2 = vdata;
	break;
      case CSYS_MIDI_SYSTEM_CLOCK:  
	if (!(jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_HDR] & NSYS_SM_CQ_HDR_CHKD))
	  jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_HDR] |= NSYS_SM_CQ_HDR_SETD;
	else
	  {
	    if (!(jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_HDR] & NSYS_SM_CQ_HDR_CHKC))
	      {
		jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_HDR] |= NSYS_SM_CQ_HDR_SETC;
		jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_FIELDS + 1] = 0;
		jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_FIELDS] = 0;
		jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_HDR] &= ~(NSYS_SM_CQ_TOP_MASK);
	      }
	    if (!(++(jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_FIELDS + 1])))
	      if (!(++(jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_FIELDS])))
		{
		  if ((jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_HDR] 
		       & NSYS_SM_CQ_TOP_MASK) != NSYS_SM_CQ_TOP_MASK)
		    jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_HDR]++;
		  else
		    jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_HDR] &=
		      ~(NSYS_SM_CQ_TOP_MASK);
		}
	  }
	break;
      case CSYS_MIDI_SYSTEM_START:
	jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_HDR] = NSYS_SM_CQ_HDR_SETN;  /* C=D=0 */
	break;
      case CSYS_MIDI_SYSTEM_CONTINUE:
	jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_HDR] |= NSYS_SM_CQ_HDR_SETN;
	if (jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_HDR] & NSYS_SM_CQ_HDR_CHKD)
	  {
	    if (!(jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_HDR] & NSYS_SM_CQ_HDR_CHKC))
	      {
		jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_HDR] |= NSYS_SM_CQ_HDR_SETC;
		jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_FIELDS + 1] = 0;
		jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_FIELDS] = 0;
		jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_HDR] &= ~(NSYS_SM_CQ_TOP_MASK);
	      }
	    if (!(++(jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_FIELDS + 1])))
	      if (!(++(jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_FIELDS])))
		{
		  if ((jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_HDR] 
		       & NSYS_SM_CQ_TOP_MASK) != NSYS_SM_CQ_TOP_MASK)
		    jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_HDR]++;
		  else
		    jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_HDR] &=
		      ~(NSYS_SM_CQ_TOP_MASK);
		}
	    jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_HDR] &= NSYS_SM_CQ_HDR_CLRD;
	  }
	break;
      case CSYS_MIDI_SYSTEM_STOP:
	jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_HDR] &= NSYS_SM_CQ_HDR_CLRN;
	break;
      case CSYS_MIDI_SYSTEM_SONG_PP:
	if ((song_pp = 6*((vdata << 7) + ndata)))
	  {
	    if (!(jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_HDR] & NSYS_SM_CQ_HDR_CHKC))
	      jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_HDR] |= NSYS_SM_CQ_HDR_SETC;
	    jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_FIELDS + 1] = 
	      (unsigned char) (song_pp & 0x000000FF);
	    jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_FIELDS] = 
	      (unsigned char) ((song_pp >> 8) & 0x000000FF);
	    jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_HDR] &= ~(NSYS_SM_CQ_TOP_MASK);
	    if (song_pp > NSYS_SM_CQ_BOTTOM_MASK)
	      jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_HDR] |= 0x01;  /* range limit */
	  }
	else
	  if ((jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_HDR] & NSYS_SM_CQ_HDR_CHKC))
	    jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_HDR] &= NSYS_SM_CQ_HDR_CLRC;
	jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_HDR] &= NSYS_SM_CQ_HDR_CLRD;
	break;
      case CSYS_MIDI_SYSTEM_SENSE:
	jrecvsys->chapterv_count = NSYS_SM_RV_CLRF & (jrecvsys->chapterv_count + 1);
	break;
      }
    }
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                 second-level receiver functions              */
/*______________________________________________________________*/

/****************************************************************/
/*               process chapter P (program change)             */
/****************************************************************/


int nsys_netin_jrec_program(nsys_source * sptr, unsigned char * p,
			  nsys_netout_jrecv_state * jrecv,
			  unsigned char * buff,  
			  long * fill, long size)

{
  unsigned char newx, newy, vel, yflag;

  vel = (p[NSYS_SM_CP_LOC_PROGRAM] & NSYS_SM_CLRS);
  newx = (p[NSYS_SM_CP_LOC_BANKMSB] & NSYS_SM_CP_CLRB);
  newy = (p[NSYS_SM_CP_LOC_BANKLSB] & NSYS_SM_CP_CLRX);
  
  if ((jrecv->chapterp_program == 0) || 
      (newx != jrecv->chapterp_bank_msb) || 
      (newy != jrecv->chapterp_bank_lsb) ||
      (vel != (jrecv->chapterp_program & NSYS_SM_RV_CLRF)))
    {
      yflag = ((newx != (NSYS_SM_CLRS & jrecv->chapterc_value
			 [CSYS_MIDI_CC_BANKSELECT_MSB])) ||
	       (newy != (NSYS_SM_CLRS & jrecv->chapterc_value
			 [CSYS_MIDI_CC_BANKSELECT_LSB])) ||
	       (jrecv->chapterc_value
		[CSYS_MIDI_CC_BANKSELECT_MSB] == 0) ||
	       (jrecv->chapterc_value
		[CSYS_MIDI_CC_BANKSELECT_LSB] == 0));
      
      /* if needed, change bank-switch values */
      
      if (yflag)
	{			  
	  if (nsys_netin_journal_addcmd_three(sptr, buff, fill, size,
					jrecv->chan | CSYS_MIDI_CC,
					CSYS_MIDI_CC_BANKSELECT_MSB
					, newx)) 
	    return NSYS_JOURNAL_FILLEDBUFF;
	  if (nsys_netin_journal_addcmd_three(sptr, buff, fill, size,
					jrecv->chan | CSYS_MIDI_CC,
					CSYS_MIDI_CC_BANKSELECT_LSB
					, newy))
	    return NSYS_JOURNAL_FILLEDBUFF;
	}
      
      /* do program change */
      
      if (nsys_netin_journal_addcmd_two(sptr, buff, fill, size,
					jrecv->chan | CSYS_MIDI_PROGRAM, vel))
	return NSYS_JOURNAL_FILLEDBUFF;
      
      /* if changed, reset bank-switch values */
      
      if (yflag)
	{
	  if (nsys_netin_journal_addcmd_three(sptr, buff, fill, size,
					jrecv->chan | CSYS_MIDI_CC,
					CSYS_MIDI_CC_BANKSELECT_MSB
					, jrecv->chapterc_value
					[CSYS_MIDI_CC_BANKSELECT_MSB])) 
	    return NSYS_JOURNAL_FILLEDBUFF;
	  if (nsys_netin_journal_addcmd_three(sptr, buff, fill, size,
					jrecv->chan | CSYS_MIDI_CC,
					CSYS_MIDI_CC_BANKSELECT_LSB
					, jrecv->chapterc_value
					[CSYS_MIDI_CC_BANKSELECT_LSB]))
	    return NSYS_JOURNAL_FILLEDBUFF;
	}
      
      if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	printf("PChange %hhu [%hhu,%hhu] --> %hhu [%hhu, %hhu]\n",
	       jrecv->chapterp_program & NSYS_SM_RV_CLRF,
	       jrecv->chapterp_bank_msb, jrecv->chapterp_bank_lsb,
	       vel, newx, newy);
      
      jrecv->chapterp_program = vel | NSYS_SM_RV_SETF;
      jrecv->chapterp_bank_msb = newx ;
      jrecv->chapterp_bank_lsb = newy ;
    }
  
  return NSYS_JOURNAL_RECOVERED;
}

/****************************************************************/
/*               process chapter C (controllers)                */
/****************************************************************/

int nsys_netin_jrec_control(nsys_source * sptr, unsigned char * p,
			    nsys_netout_jrecv_state * jrecv, short loglen,
			    unsigned char many, unsigned char * buff, 
			    long * fill, long size)

{
  unsigned char newx, newy, cancel;
  int unexpected = 0;
  int update = 0;

  p += NSYS_SM_CC_HDRSIZE;
  while (loglen)
    {
      if ((!(p[NSYS_SM_CC_LOC_LNUM] & NSYS_SM_CHKS)) || many)
	{
	  newx = p[NSYS_SM_CC_LOC_LNUM] & NSYS_SM_CLRS;
	  newy = p[NSYS_SM_CC_LOC_LVAL];

	  switch (newx) {   
	  case CSYS_MIDI_CC_ALLSOUNDOFF:
	  case CSYS_MIDI_CC_ALLNOTESOFF:
	  case CSYS_MIDI_CC_RESETALLCONTROL:
	  case CSYS_MIDI_CC_OMNI_OFF:
	  case CSYS_MIDI_CC_OMNI_ON:
	  case CSYS_MIDI_CC_MONOMODE:
	  case CSYS_MIDI_CC_POLYMODE:
	    if ((newy & NSYS_SM_CC_CHKA) && !(newy & NSYS_SM_CC_CHKT))
	      {
		update = ((jrecv->chapterc_value[newx] == 0) || 
			  ((jrecv->chapterc_value[newx] & NSYS_SM_RV_CLRF) !=
			   (newy & NSYS_SM_CC_ALTMOD)));

		if (update)
		  {
		    jrecv->chapterc_value[newx] = ((newy & NSYS_SM_CC_ALTMOD)
						   | NSYS_SM_RV_SETF);
		    newy = 0;

		    switch (newx) {
		    case CSYS_MIDI_CC_ALLSOUNDOFF:
		    case CSYS_MIDI_CC_ALLNOTESOFF:
		    case CSYS_MIDI_CC_OMNI_OFF:
		    case CSYS_MIDI_CC_OMNI_ON:
		    case CSYS_MIDI_CC_MONOMODE:
		    case CSYS_MIDI_CC_POLYMODE:
		      memset(jrecv->chaptern_ref, 0, NSYS_SM_CN_ARRAYSIZE);
		      jrecv->chaptert_pressure = 0;
		      break;
		    case CSYS_MIDI_CC_RESETALLCONTROL:

		      /* Chapter C -- update pedal count */
		      
		      jrecv->chapterc_value[CSYS_MIDI_CC_SUSTAIN] = NSYS_SM_RV_SETF |
			(((jrecv->chapterc_value[CSYS_MIDI_CC_SUSTAIN] 
			   & NSYS_SM_CC_ALTMOD) + 1) & NSYS_SM_CC_ALTMOD);

		      /* Clear parameter transaction system */
		      
		      jrecv->chapterm_nrpn_msb = jrecv->chapterm_nrpn_lsb = 0;
		      jrecv->chapterm_rpn_msb = jrecv->chapterm_rpn_lsb = 0;
		      jrecv->chapterm_state = NSYS_SM_CM_STATE_OFF;

		      /* Clear parameter system C-BUTTON counts */

		      memset(jrecv->chapterm_cbutton, 0, 
			     sizeof(short)*NSYS_SM_CM_ARRAYSIZE);

		      /* C-active Chapters:  W, T, and A */
		      
		      jrecv->chapterw_first = 0;
		      jrecv->chapterw_second = 0;
		      memset(jrecv->chaptera_pressure, 0, NSYS_SM_CA_ARRAYSIZE);
		      jrecv->chaptert_pressure = 0;
		      break;
		    }
		  }
	      }
	    else
	      unexpected = 1;
	    break;
	  case CSYS_MIDI_CC_SUSTAIN:
	    if ((newy & NSYS_SM_CC_CHKA) && (newy & NSYS_SM_CC_CHKT))
	      {
		update = ((jrecv->chapterc_value[newx] == 0) || 
			  ((jrecv->chapterc_value[newx] & NSYS_SM_RV_CLRF) !=
			   (newy & NSYS_SM_CC_ALTMOD)));

		if (update)
		  {
		    cancel = (newy & 0x01) & (jrecv->chapterc_value[newx] & 1);

		    jrecv->chapterc_value[newx] = ((newy & NSYS_SM_CC_ALTMOD)
						   | NSYS_SM_RV_SETF);
		    newy = (newy & 0x01) ? 64 : 0;

		    if (cancel)
		      {
			if (nsys_netin_journal_addcmd_three(sptr, buff, fill, size,
						      jrecv->chan | CSYS_MIDI_CC,
						      newx, 0))
			  return NSYS_JOURNAL_FILLEDBUFF;
	      
			if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
			  printf("CntrlLog %hhu: cancel sustain pedal\n", newx);
		      }
		  }
	      }
	    else
	      unexpected = 1;
	    break;
	  default:
	    if (!(newy & NSYS_SM_CC_CHKA))
	      {
		update = (jrecv->chapterc_value[newx] == 0) || 
		  ((jrecv->chapterc_value[newx] & NSYS_SM_RV_CLRF) != newy);

		if (update)
		  jrecv->chapterc_value[newx] = newy | NSYS_SM_RV_SETF;
	      }
	    else
	      unexpected = 1;
	    break;
	  }

	  if (update && unexpected)
	    unexpected = update = 0;   /* unexpected log format -- do later */

	  if (update)
	    {
	      update = 0;

	      if (nsys_netin_journal_addcmd_three(sptr, buff, fill, size,
					    jrecv->chan | CSYS_MIDI_CC,
					    newx, newy))
		  return NSYS_JOURNAL_FILLEDBUFF;
	      
	      if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
		printf("CntrlLog %hhu: --> %hhu\n", newx, newy);
	    }
	}
      loglen--;
      p += NSYS_SM_CC_LOGSIZE;
    }

  return NSYS_JOURNAL_RECOVERED;
}

/****************************************************************/
/*               process chapter M (parameters)                 */
/****************************************************************/

int nsys_netin_jrec_param(nsys_source * sptr, unsigned char * p,
			  nsys_netout_jrecv_state * jrecv, 
			  short paramlen, unsigned char many, 
			  unsigned char * buff, long * fill,
			  long size)

{
  unsigned char msb_num, lsb_num, msb_val, lsb_val, toc;
  int has_j, has_k, has_l, has_m, has_n;
  short cbutton;
  int i, rpnlog, rpnlast, final_state, skip, adjust, val_eq, button_eq;
  int repair = 0;

  /*~~~~~~~~~~~~~~~~~~~*/
  /* repair RTP values */
  /*___________________*/

  rpnlast = NSYS_SM_CM_TRANS_NONE;
  msb_num = lsb_num = 0;
  adjust = 0;

  if (p[NSYS_SM_CM_LOC_HDR] & NSYS_SM_CM_HDR_CHKP)
    i = NSYS_SM_CM_HDRSIZE + NSYS_SM_CM_PENDINGSIZE;
  else
    i = NSYS_SM_CM_HDRSIZE;

  if ((p[NSYS_SM_CM_LOC_HDR] & NSYS_SM_CM_HDR_CHKZ) &&
      ((p[NSYS_SM_CM_LOC_HDR] & NSYS_SM_CM_HDR_CHKU) ||
       (p[NSYS_SM_CM_LOC_HDR] & NSYS_SM_CM_HDR_CHKW)))
    adjust = 1;

  rpnlast = (p[NSYS_SM_CM_LOC_HDR] & NSYS_SM_CM_HDR_CHKU) ?
    NSYS_SM_CM_TRANS_RPN : NSYS_SM_CM_TRANS_NRPN;

  while (i + NSYS_SM_CM_LOGHDRSIZE - adjust <= paramlen)
    {
      skip = (p[i + NSYS_SM_CM_LOC_PNUMLSB] & NSYS_SM_CHKS) && !many;
      lsb_num = p[i + NSYS_SM_CM_LOC_PNUMLSB] & NSYS_SM_CLRS;

      if (!adjust)
	{
	  msb_num = p[i + NSYS_SM_CM_LOC_PNUMMSB] & NSYS_SM_CM_CLRQ;
	  rpnlast = (p[i + NSYS_SM_CM_LOC_PNUMMSB] & NSYS_SM_CM_CHKQ) ?
	    NSYS_SM_CM_TRANS_NRPN : NSYS_SM_CM_TRANS_RPN;
	}

      toc = p[i + NSYS_SM_CM_LOC_TOC - adjust];

      has_j = toc & NSYS_SM_CM_TOC_CHKJ;
      has_k = toc & NSYS_SM_CM_TOC_CHKK;
      has_l = toc & NSYS_SM_CM_TOC_CHKL;
      has_m = toc & NSYS_SM_CM_TOC_CHKM;
      has_n = toc & NSYS_SM_CM_TOC_CHKN;

      i += (NSYS_SM_CM_LOGHDRSIZE - adjust);

      if (skip || (rpnlast != NSYS_SM_CM_TRANS_RPN) || msb_num 
	  || (lsb_num >= NSYS_SM_CM_ARRAYSIZE) || 
	  ! (has_j || has_k || has_m))
	{
	  i += ((has_j ? NSYS_SM_CM_ENTRYMSB_SIZE : 0) + 
		(has_k ? NSYS_SM_CM_ENTRYLSB_SIZE : 0) +
		(has_l ? NSYS_SM_CM_ABUTTON_SIZE  : 0) +
		(has_m ? NSYS_SM_CM_CBUTTON_SIZE  : 0) +
		(has_n ? NSYS_SM_CM_COUNT_SIZE    : 0));
	  continue;
	}

      msb_val = jrecv->chapterm_value_msb[lsb_num];
      lsb_val = jrecv->chapterm_value_lsb[lsb_num];
      cbutton = jrecv->chapterm_cbutton[lsb_num];

      if (has_j)
	{
	  if (i < paramlen)
	    {
	      msb_val = p[i] & NSYS_SM_CM_CLRX;
	      if (!has_k)
		lsb_val = 0;
	      if (!has_m)
		cbutton = 0;
	      i += NSYS_SM_CM_ENTRYMSB_SIZE;
	    }
	  else
	    return NSYS_JOURNAL_CORRUPTED; /* add state repairs */
	}

      if (has_k)
	{
	  if (i < paramlen)
	    {
	      lsb_val = p[i] & NSYS_SM_CM_CLRX;
	      if (!has_m)
		cbutton = 0;
	      i += NSYS_SM_CM_ENTRYLSB_SIZE;
	    }
	  else
	    return NSYS_JOURNAL_CORRUPTED; /* add state repairs */
	}

      if (has_l)
	i += NSYS_SM_CM_ABUTTON_SIZE;

      if (has_m)
	{
	  if ((i + 1) < paramlen)
	    {
	      cbutton = p[i] & (NSYS_SM_CM_BUTTON_CLRX & NSYS_SM_CM_BUTTON_CLRG);
	      cbutton = (cbutton << 8) + p[i+1];
	      if (p[i] & NSYS_SM_CM_BUTTON_CHKG)
		cbutton = - cbutton;
	      i += NSYS_SM_CM_CBUTTON_SIZE;
	    }
	  else
	    return NSYS_JOURNAL_CORRUPTED; /* add state repairs */
	}

      if (has_n)
	i += NSYS_SM_CM_COUNT_SIZE;

      if (i > paramlen)
	return NSYS_JOURNAL_CORRUPTED; /* add state repairs */

      val_eq = ((msb_val == jrecv->chapterm_value_msb[lsb_num]) && 
		(lsb_val == jrecv->chapterm_value_lsb[lsb_num]));

      button_eq = (cbutton == jrecv->chapterm_cbutton[lsb_num]);

      if (val_eq && button_eq)
	continue;

      repair = 1;

      if (!val_eq)
	{
	  if (nsys_netin_journal_trans(sptr, buff, fill, size, jrecv->chan,
				       rpnlast, msb_num, lsb_num, msb_val, lsb_val))
	    return NSYS_JOURNAL_FILLEDBUFF;  /* add state repairs */

	  if (has_m && cbutton)
	    {
	      if (nsys_netin_journal_button_trans(sptr, buff, fill, size, jrecv->chan,
						  rpnlast, msb_num, lsb_num, cbutton))
		return NSYS_JOURNAL_FILLEDBUFF;  /* add state repairs */

	    if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	      printf("RPN-Log %hhu: --> (%hhu, %hhu) delta %hi\n", lsb_num, 
		     msb_val, lsb_val, cbutton);
	    }
	  else
	    if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	      printf("RPN-Log %hhu: --> (%hhu, %hhu)\n", lsb_num, msb_val, lsb_val);
	}
      else
	{
	  if (nsys_netin_journal_button_trans(sptr, buff, fill, size, jrecv->chan,
					      rpnlast, msb_num, lsb_num, cbutton -
					      jrecv->chapterm_cbutton[lsb_num]))
	    return NSYS_JOURNAL_FILLEDBUFF;  /* add state repairs */

	  if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	    printf("RPN-Log %hhu: --> delta %hi\n", lsb_num, 
		   cbutton - jrecv->chapterm_cbutton[lsb_num]);
	}
    }

  /*~~~~~~~~~~~~~~~~~~~~~~~*/
  /* determine final state */
  /*_______________________*/

  if (p[NSYS_SM_CM_LOC_HDR] & NSYS_SM_CM_HDR_CHKP)
    {
      if (p[NSYS_SM_CM_LOC_PENDING] & NSYS_SM_CM_PENDING_CHKQ)
	final_state = NSYS_SM_CM_STATE_PENDING_NRPN;
      else
	final_state = NSYS_SM_CM_STATE_PENDING_RPN;
    }
  else
    {
      if (p[NSYS_SM_CM_LOC_HDR] & NSYS_SM_CM_HDR_CHKE)
	{
	  switch (rpnlast) {
	    case NSYS_SM_CM_TRANS_RPN:
	      final_state = NSYS_SM_CM_STATE_RPN;
	      break;
	    case NSYS_SM_CM_TRANS_NRPN:
	      final_state = NSYS_SM_CM_STATE_NRPN;
	      break;
	  default:
	    switch (jrecv->chapterm_state) {    /* should never run */
	    case NSYS_SM_CM_STATE_PENDING_NRPN:
	    case NSYS_SM_CM_STATE_NRPN:
	      final_state = NSYS_SM_CM_STATE_NRPN;
	      lsb_num = jrecv->chapterm_nrpn_lsb;
	      msb_num = jrecv->chapterm_nrpn_msb;
	      break;
	    default:
	      final_state = NSYS_SM_CM_STATE_RPN;
	      lsb_num = jrecv->chapterm_rpn_lsb;
	      msb_num = jrecv->chapterm_rpn_msb;
	      break;
	    }
	  }
	}
      else
	final_state = NSYS_SM_CM_STATE_OFF;
    }

  /*~~~~~~~~~~~~~~~~~~~~*/
  /* exit if no changes */
  /*____________________*/

  if ((repair == 0) && (final_state == jrecv->chapterm_state))
    {
      switch (final_state) {
      case NSYS_SM_CM_STATE_OFF:
	return NSYS_JOURNAL_RECOVERED;
      case NSYS_SM_CM_STATE_RPN:
	if ((lsb_num == jrecv->chapterm_rpn_lsb)
	    && (msb_num == jrecv->chapterm_rpn_msb))
	  return NSYS_JOURNAL_RECOVERED;
	break;
      case NSYS_SM_CM_STATE_PENDING_RPN:
	if ((p[NSYS_SM_CM_LOC_PENDING] & NSYS_SM_CM_PENDING_CLRQ) 
	    == jrecv->chapterm_rpn_msb)
	  return NSYS_JOURNAL_RECOVERED;
	break;
      case NSYS_SM_CM_STATE_NRPN:
	if ((lsb_num == jrecv->chapterm_nrpn_lsb)
	    && (msb_num == jrecv->chapterm_nrpn_msb))
	  return NSYS_JOURNAL_RECOVERED;
	break;
      case NSYS_SM_CM_STATE_PENDING_NRPN:
	if ((p[NSYS_SM_CM_LOC_PENDING] & NSYS_SM_CM_PENDING_CLRQ) 
	    == jrecv->chapterm_rpn_msb)
	  return NSYS_JOURNAL_RECOVERED;
	break;
      }
    }

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
  /* do transaction repairs for each state */
  /*_______________________________________*/

  switch (final_state) {
  case NSYS_SM_CM_STATE_OFF:
    jrecv->chapterm_state = NSYS_SM_CM_STATE_OFF;
    if (!repair)
      {
	rpnlog = (NSYS_SM_CM_TRANS_RPN | NSYS_SM_CM_TRANS_NO_OPEN
		  | NSYS_SM_CM_TRANS_NO_SET);
	if (nsys_netin_journal_trans(sptr, buff, fill, size, jrecv->chan,
				     rpnlog, 0, 0, 0, 0))
	  return NSYS_JOURNAL_FILLEDBUFF;   /* add state repairs */
      if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	printf("ParamLog  --> off-state\n");
      }
    break;
  case NSYS_SM_CM_STATE_RPN:
    jrecv->chapterm_state = NSYS_SM_CM_STATE_RPN;
    jrecv->chapterm_rpn_lsb = lsb_num;
    jrecv->chapterm_rpn_msb = msb_num;
    rpnlog = (NSYS_SM_CM_TRANS_RPN | NSYS_SM_CM_TRANS_NO_SET
	      | NSYS_SM_CM_TRANS_NO_CLOSE);
    if (nsys_netin_journal_trans(sptr, buff, fill, size, jrecv->chan,
				 rpnlog, msb_num, lsb_num, 0, 0))
      return NSYS_JOURNAL_FILLEDBUFF;  /* add state repairs */
    if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
      printf("ParamLog: --> rpn-state (%hhu, %hhu)\n", msb_num, lsb_num);
    break;
  case NSYS_SM_CM_STATE_NRPN:
    jrecv->chapterm_state = NSYS_SM_CM_STATE_NRPN;
    jrecv->chapterm_nrpn_lsb = lsb_num;
    jrecv->chapterm_nrpn_msb = msb_num;
    rpnlog = (NSYS_SM_CM_TRANS_NRPN | NSYS_SM_CM_TRANS_NO_SET
	      | NSYS_SM_CM_TRANS_NO_CLOSE);
    if (nsys_netin_journal_trans(sptr, buff, fill, size, jrecv->chan,
				 rpnlog, msb_num, lsb_num, 0, 0))
      return NSYS_JOURNAL_FILLEDBUFF;  /* add state repairs */
    if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
      printf("ParamLog: --> nrpn-state (%hhu, %hhu)\n", msb_num, lsb_num);
    break;
  case NSYS_SM_CM_STATE_PENDING_RPN:
    if (!repair && (jrecv->chapterm_state != NSYS_SM_CM_STATE_OFF))
      {
	if ((jrecv->chapterm_state == NSYS_SM_CM_STATE_PENDING_RPN) || 
	    (jrecv->chapterm_state == NSYS_SM_CM_STATE_RPN))
	  rpnlog = (NSYS_SM_CM_TRANS_RPN | NSYS_SM_CM_TRANS_NO_OPEN
		    | NSYS_SM_CM_TRANS_NO_SET);
	else
	  rpnlog = (NSYS_SM_CM_TRANS_NRPN | NSYS_SM_CM_TRANS_NO_OPEN
		    | NSYS_SM_CM_TRANS_NO_SET);
	
	if (nsys_netin_journal_trans(sptr, buff, fill, size, jrecv->chan,
				     rpnlog, 0, 0, 0, 0))
	  return NSYS_JOURNAL_FILLEDBUFF;   /* add state repairs */
      }
      jrecv->chapterm_state = NSYS_SM_CM_STATE_PENDING_RPN;
      jrecv->chapterm_rpn_msb = p[NSYS_SM_CM_LOC_PENDING] & NSYS_SM_CM_PENDING_CLRQ;
      jrecv->chapterm_rpn_lsb = 0;
      if (nsys_netin_journal_addcmd_three(sptr, buff, fill, size,
				    jrecv->chan | CSYS_MIDI_CC,
				    CSYS_MIDI_CC_RPN_MSB
				    , jrecv->chapterm_rpn_msb)) 
	    return NSYS_JOURNAL_FILLEDBUFF; /* add state repairs */
      if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	printf("ParamLog: --> rpn-pending (%hhu, 0)\n", jrecv->chapterm_rpn_msb);
      break;
  case NSYS_SM_CM_STATE_PENDING_NRPN:
    if (!repair && (jrecv->chapterm_state != NSYS_SM_CM_STATE_OFF))
      {
	if ((jrecv->chapterm_state == NSYS_SM_CM_STATE_PENDING_RPN) || 
	    (jrecv->chapterm_state == NSYS_SM_CM_STATE_RPN))
	  rpnlog = (NSYS_SM_CM_TRANS_RPN | NSYS_SM_CM_TRANS_NO_OPEN
		    | NSYS_SM_CM_TRANS_NO_SET);
	else
	  rpnlog = (NSYS_SM_CM_TRANS_NRPN | NSYS_SM_CM_TRANS_NO_OPEN
		    | NSYS_SM_CM_TRANS_NO_SET);
	
	if (nsys_netin_journal_trans(sptr, buff, fill, size, jrecv->chan,
				     rpnlog, 0, 0, 0, 0))
	  return NSYS_JOURNAL_FILLEDBUFF;   /* add state repairs */
      }
      jrecv->chapterm_state = NSYS_SM_CM_STATE_PENDING_NRPN;
      jrecv->chapterm_nrpn_msb = p[NSYS_SM_CM_LOC_PENDING] & NSYS_SM_CM_PENDING_CLRQ;
      jrecv->chapterm_nrpn_lsb = 0;
      if (nsys_netin_journal_addcmd_three(sptr, buff, fill, size,
				    jrecv->chan | CSYS_MIDI_CC,
				    CSYS_MIDI_CC_NRPN_MSB,
				    jrecv->chapterm_nrpn_msb)) 
	return NSYS_JOURNAL_FILLEDBUFF; /* add state repairs */
      if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	printf("ParamLog: --> nrpn-pending (%hhu, 0)\n", jrecv->chapterm_nrpn_msb);
      break;
  }

  return NSYS_JOURNAL_RECOVERED;
}

/****************************************************************/
/*               process chapter W (pitch wheel)                */
/****************************************************************/


int nsys_netin_jrec_wheel(nsys_source * sptr, unsigned char * p,
			  nsys_netout_jrecv_state * jrecv,
			  unsigned char * buff,  
			  long * fill, long size)

{
  unsigned char newx, newy;

  newx = p[NSYS_SM_CW_LOC_FIRST] & NSYS_SM_CLRS;
  newy = p[NSYS_SM_CW_LOC_SECOND] & NSYS_SM_CLRS;
  
  if ((newx != (jrecv->chapterw_first & NSYS_SM_RV_CLRF)) || 
      (newy != jrecv->chapterw_second) || 
      (jrecv->chapterw_first == 0))
    {
      if (nsys_netin_journal_addcmd_three(sptr, buff, fill, size,
				    jrecv->chan | CSYS_MIDI_WHEEL,
				    newx, newy))
	return NSYS_JOURNAL_FILLEDBUFF;
      
      if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	printf("PWheel (%hhu, %hhu) --> (%hhu, %hhu)\n", 
	       jrecv->chapterw_first & NSYS_SM_CLRS,
	       jrecv->chapterw_second, newx, newy);
      
      jrecv->chapterw_first = newx | NSYS_SM_RV_SETF;
      jrecv->chapterw_second = newy;
    }

  return NSYS_JOURNAL_RECOVERED;
}




/****************************************************************/
/*               process Note Logs of chapter N                 */
/****************************************************************/

int nsys_netin_jrec_notelog(nsys_source * sptr, unsigned char * p,
			    nsys_netout_jrecv_state * jrecv, unsigned char many, 
			    short loglen, unsigned char * checkptr,
			    unsigned char * buff, 
			    long * fill, long size)

{
  unsigned char newx, newy, vel, i;
  int yflag, noteon, noteoff, dated, predate;
  unsigned long check_ext;

  /*********************************************/
  /* prepare extended checkpoint packet number */
  /*********************************************/
  
  check_ext = ntohs(*((unsigned short *)checkptr)); 
  
  if (check_ext < sptr->hi_lobits)
    check_ext |= (sptr->hi_ext & NSYS_RTPSEQ_EXMASK);
  else
    check_ext |= ((sptr->hi_ext & NSYS_RTPSEQ_EXMASK)
		  - NSYS_RTPSEQ_EXINCR);

  /**************************/
  /* loop through note logs */
  /**************************/

  while (loglen)
    {
      if ((!(p[NSYS_SM_CN_LOC_NUM] & NSYS_SM_CHKS)) || many)
	{		      
	  newx = p[NSYS_SM_CN_LOC_NUM] & NSYS_SM_CLRS;
	  newy = p[NSYS_SM_CN_LOC_VEL] & NSYS_SM_CN_CLRY;
	  yflag = (p[NSYS_SM_CN_LOC_VEL] & NSYS_SM_CN_CHKY);
	  vel = jrecv->chaptern_vel[newx];

	  /**************************************/
	  /* by default, turn off all notes ... */
	  /**************************************/

	  noteoff = jrecv->chaptern_ref[newx];

	  /******************************************************/
	  /* ... and play the new NoteOn if it won't sound late */
	  /******************************************************/

	  noteon = (newy != 0) && yflag && sptr->ontime;

	  /*************************************/
	  /* exceptions for the ambigious case */
	  /*************************************/

	  if ((noteoff == 1)  &&               /* one note is playing */
	      (vel && newy && (vel == newy)))  /* unchanged velocity  */
	    {
	      /* has old note been playing a long time? */

	      dated = NSYS_SM_CN_RECDELAY < (nsys_netout_tstamp - 
					     jrecv->chaptern_tstamp[newx]);

	      /* does playing note predate checkpoint? (if so, different) */

	      predate = (jrecv->chaptern_extseq[newx] < check_ext);

	      if (noteon)
		{
		  /* noteon = 1 because new note is recent. if the evidence */
		  /* indicates old note is recent too, then let note ring   */

		  if ((!predate) && (!dated))
		      noteon = noteoff = 0;

		  if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
		    printf(" -EX1- ");
		}
	      else
		{
		  /* noteon = 0 because we think new note is old. if the   */
		  /* old note appears old too, then let the note ring      */

		  if (predate || dated)
		    noteoff = 0;

		  if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
		    printf(" -EX2- ");
		}
	    }
	  
	  if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	    {
	      printf("NoteLog %hhu: %s(%hhu, %hhu); Actions: ", 
		     newx, (p[NSYS_SM_CN_LOC_VEL] & NSYS_SM_CN_CHKY) 
		     ? "#" : "-", vel, newy);

	      if (noteoff)
		printf(" NoteOff ");
 
	      if (noteoff > 1)
		printf( "(%i) ", noteoff);

	      if (noteon)
		printf(" NoteOn");

	      printf("\n");
	    }
	  
	  /* do NoteOff, then NoteOn, then update state */

	  for (i = 0; i < noteoff; i++)
	    if ((nsys_netin_journal_addcmd_three
		 (sptr, buff, fill, size,
		  jrecv->chan | CSYS_MIDI_NOTEOFF, newx, 64)))
	      return NSYS_JOURNAL_FILLEDBUFF;
	  
	  if (noteon && (nsys_netin_journal_addcmd_three
			 (sptr, buff, fill, size,
			  jrecv->chan | CSYS_MIDI_NOTEON, newx, newy)))
	    return NSYS_JOURNAL_FILLEDBUFF;
	  
	  if (noteoff || noteon)
	    {
	      jrecv->chaptern_ref[newx] -= (noteoff - noteon);

	      if (noteon)
		{
		  jrecv->chaptern_vel[newx] = newy;
		  jrecv->chaptern_tstamp[newx] = nsys_netout_tstamp;
		  jrecv->chaptern_extseq[newx] = sptr->hi_ext;
		}
	      else
		if (jrecv->chaptern_ref[newx] == 0)
		  jrecv->chaptern_vel[newx] = 0;
	    }
	}
      loglen--;
      p += NSYS_SM_CN_LOGSIZE;
    }

  return NSYS_JOURNAL_RECOVERED;
}



/****************************************************************/
/*               process bitfields of chapter N                */
/****************************************************************/

int nsys_netin_jrec_bitfield(nsys_source * sptr, unsigned char * p,
			     nsys_netout_jrecv_state * jrecv, unsigned char low, 
			     unsigned char high, unsigned char * buff, 
			     long * fill, long size)
{
  int i, first;
  unsigned char newx, bitfield;

  while (low <= high)
    {	      
      i = 0;
      bitfield = 128;
      while (bitfield)
	{
	  if ((*p) & bitfield)
	    {
	      newx = (low << NSYS_SM_CN_BFSHIFT) + i;
	      
	      if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
		printf("Bitfield %hhu: (%hhu); ", newx,  
		       jrecv->chaptern_vel[newx]);

	      first = 1;

	      while (jrecv->chaptern_ref[newx])
		{
		  if (nsys_netin_journal_addcmd_three 
		      (sptr, buff, fill, size, 
		       jrecv->chan | CSYS_MIDI_NOTEOFF, newx, 64))
		    return NSYS_JOURNAL_FILLEDBUFF;

		  if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
		    if (first)
		      {
			first = 0;
			printf(" Actions: Noteoff (%hhu) ",
			       jrecv->chaptern_ref[newx]);
		      }

		  jrecv->chaptern_ref[newx]--;
		}

	      if (jrecv->chaptern_vel[newx])
		jrecv->chaptern_vel[newx] = 0;

	      if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
		printf("\n");
	    }
	  bitfield >>= 1;
	  i++;
	}
      low++;
      p++;
    }
  
  return NSYS_JOURNAL_RECOVERED;
}

/****************************************************************/
/*               process chapter T (channel touch)              */
/****************************************************************/

int nsys_netin_jrec_ctouch(nsys_source * sptr, unsigned char * p,
			   nsys_netout_jrecv_state * jrecv,
			   unsigned char * buff,  
			   long * fill, long size)

{
  unsigned char newx;

  newx = p[NSYS_SM_CT_LOC_PRESSURE] & NSYS_SM_CLRS;
  if ((newx != (jrecv->chaptert_pressure & NSYS_SM_RV_CLRF)) ||
      (jrecv->chaptert_pressure == 0))
    {
      if (nsys_netin_journal_addcmd_two(sptr, buff, fill, size,
				    jrecv->chan | CSYS_MIDI_CTOUCH, newx))
	  return NSYS_JOURNAL_FILLEDBUFF;
      
      if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	printf("CTouch (%hhu) --> (%hhu)\n", 
	       jrecv->chaptert_pressure & NSYS_SM_RV_CLRF, newx);
      
      jrecv->chaptert_pressure = newx | NSYS_SM_RV_SETF;
    }

  return NSYS_JOURNAL_RECOVERED;
}

/****************************************************************/
/*               process chapter P (poly touch)                 */
/****************************************************************/

int nsys_netin_jrec_ptouch(nsys_source * sptr, unsigned char * p,
			   nsys_netout_jrecv_state * jrecv, short loglen,
			   unsigned char many, unsigned char * buff, 
			   long * fill, long size)

{
  unsigned char newx, newy;

  p += NSYS_SM_CA_HDRSIZE;
  while (loglen)
    {
      if ((!(p[NSYS_SM_CA_LOC_NUM] & NSYS_SM_CHKS)) || many)
	{
	  newx = p[NSYS_SM_CA_LOC_NUM] & NSYS_SM_CLRS;
	  newy = p[NSYS_SM_CA_LOC_PRESSURE] & NSYS_SM_CA_CLRX;
	  if (((jrecv->chaptera_pressure[newx] & NSYS_SM_RV_CLRF)
	       != newy) || (jrecv->chaptera_pressure[newx] == 0))
	    {
	      if (nsys_netin_journal_addcmd_three(sptr, buff, fill, size,
					    jrecv->chan | CSYS_MIDI_PTOUCH,
					    newx, newy))
		  return NSYS_JOURNAL_FILLEDBUFF;
	      
	      if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
		printf("PTouchLog %hhu: %hhu --> %hhu\n", newx, 
		       jrecv->chaptera_pressure[newx] &
		       NSYS_SM_RV_CLRF, newy);
	      
	      jrecv->chaptera_pressure[newx] = (newy |
						NSYS_SM_RV_SETF);
	    }
	}
      loglen--;
      p += NSYS_SM_CA_LOGSIZE;
    }

  return NSYS_JOURNAL_RECOVERED;
}

/****************************************************************/
/*              process System Chapter D/B (Reset)              */
/****************************************************************/


int nsys_netin_jrec_reset(nsys_source * sptr, unsigned char * ps,
			  nsys_netout_jrecv_system_state * jrecvsys,
			  unsigned char * buff,  
			  long * fill, long size)
     
{
  unsigned char reset;

  reset = ps[0] & NSYS_SM_CLRS;

  if (reset != jrecvsys->chapterd_reset)
    {
      nsys_netin_journal_clear_active(CSYS_MIDI_SYSTEM_RESET);

      if (nsys_netin_journal_addcmd_one(sptr, buff, fill, size, CSYS_MIDI_SYSTEM_RESET))
	  return NSYS_JOURNAL_FILLEDBUFF;

      if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	printf("System Reset (%hhu) --> (%hhu)\n", jrecvsys->chapterd_reset, reset);
      
      jrecvsys->chapterd_reset = reset;
    }

  return NSYS_JOURNAL_RECOVERED;
}


/****************************************************************/
/*           process System Chapter D/G (Tune Request)          */
/****************************************************************/


int nsys_netin_jrec_tune(nsys_source * sptr, unsigned char * ps,
			 nsys_netout_jrecv_system_state * jrecvsys,
			 unsigned char * buff,  
			 long * fill, long size)
     
{
  unsigned char tune;

  tune = ps[0] & NSYS_SM_CLRS;

  if (tune != jrecvsys->chapterd_tune)
    {
      if (nsys_netin_journal_addcmd_one(sptr, buff, fill, size, 
					CSYS_MIDI_SYSTEM_TUNE_REQUEST))
	  return NSYS_JOURNAL_FILLEDBUFF;

      if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	printf("Tune Request (%hhu) --> (%hhu)\n", jrecvsys->chapterd_tune, tune);
      
      jrecvsys->chapterd_tune = tune;
    }

  return NSYS_JOURNAL_RECOVERED;
}


/****************************************************************/
/*           process System Chapter D/H (Song Select)           */
/****************************************************************/


int nsys_netin_jrec_song(nsys_source * sptr, unsigned char * ps,
			 nsys_netout_jrecv_system_state * jrecvsys,
			 unsigned char * buff,  
			 long * fill, long size)
     
{
  unsigned char song;

  song = ps[0] & NSYS_SM_CLRS;

  if (song != jrecvsys->chapterd_song)
    {
      if (nsys_netin_journal_addcmd_two(sptr, buff, fill, size, 
					CSYS_MIDI_SYSTEM_SONG_SELECT, song))
	  return NSYS_JOURNAL_FILLEDBUFF;

      if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	printf("Song Select (%hhu) --> (%hhu)\n", jrecvsys->chapterd_song, song);
      
      jrecvsys->chapterd_song = song;
    }

  return NSYS_JOURNAL_RECOVERED;
}

/****************************************************************/
/*           process System Chapter D/J (unused Common)        */
/****************************************************************/


int nsys_netin_jrec_scj(nsys_source * sptr, unsigned char * ps,
			nsys_netout_jrecv_system_state * jrecvsys,
			unsigned char * buff,  
			long * fill, long size)
     
{
  unsigned char ndata = CSYS_MIDI_SYSTEM_SYSEX_END;
  unsigned char vdata = CSYS_MIDI_SYSTEM_SYSEX_END;
  unsigned char count = 0;
  int has_count = 0;
  int dstart, dsize, update;

  dstart = NSYS_SM_CD_COMMON_LOC_FIELDS;

  if ((has_count = (ps[NSYS_SM_CD_COMMON_LOC_TOC] & NSYS_SM_CD_COMMON_TOC_CHKC)))
    {
      count = ps[NSYS_SM_CD_COMMON_LOC_FIELDS];
      dstart++;
    }

  if (ps[NSYS_SM_CD_COMMON_LOC_TOC] & NSYS_SM_CD_COMMON_TOC_CHKV)
    {
      ndata = ps[dstart] & NSYS_SM_RV_CLRF;
      if (ps[dstart] & NSYS_SM_RV_CHKF)
	vdata = ps[dstart + 1] & NSYS_SM_RV_CLRF;
    }

  dsize = ((ndata != CSYS_MIDI_SYSTEM_SYSEX_END) + 
	   (vdata != CSYS_MIDI_SYSTEM_SYSEX_END));

  if (has_count)
    {
      if ((update = (count != jrecvsys->chapterd_scj_count)))
	jrecvsys->chapterd_scj_count = count;
    }
  else
    update = ((ndata != jrecvsys->chapterd_scj_data1) ||
	      (vdata != jrecvsys->chapterd_scj_data2));

  if (update)
    switch (dsize) {
    case 0:
      if (nsys_netin_journal_addcmd_one(sptr, buff, fill, size, 
					CSYS_MIDI_SYSTEM_UNUSED1))
	return NSYS_JOURNAL_FILLEDBUFF;
      
      if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	printf("Common %hhu\n", CSYS_MIDI_SYSTEM_UNUSED1);
      break;
    case 1:
      if (nsys_netin_journal_addcmd_two(sptr, buff, fill, size, 
					CSYS_MIDI_SYSTEM_UNUSED1, 
					ndata))
	return NSYS_JOURNAL_FILLEDBUFF;
      
      if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	printf("Common %hhu (%hhu) --> (%hhu)\n", 
	       CSYS_MIDI_SYSTEM_UNUSED1, 
	       jrecvsys->chapterd_scj_data1, ndata);
      break;
    case 2:
      if (nsys_netin_journal_addcmd_three(sptr, buff, fill, size, 
					  CSYS_MIDI_SYSTEM_UNUSED1, 
					  ndata, vdata))
	return NSYS_JOURNAL_FILLEDBUFF;
      
      if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	printf("Common %hhu (%hhu %hhu) --> (%hhu %hhu)\n", 
	       CSYS_MIDI_SYSTEM_UNUSED1, 
	       jrecvsys->chapterd_scj_data1, 
	       jrecvsys->chapterd_scj_data2, 
	       ndata, vdata);
      break;
    }

  if (ndata != jrecvsys->chapterd_scj_data1)
    jrecvsys->chapterd_scj_data1 = ndata;

  if (ndata != jrecvsys->chapterd_scj_data2)
    jrecvsys->chapterd_scj_data2 = vdata;

  return NSYS_JOURNAL_RECOVERED;
}

/****************************************************************/
/*           process System Chapter D/K (unused Common)        */
/****************************************************************/


int nsys_netin_jrec_sck(nsys_source * sptr, unsigned char * ps,
			nsys_netout_jrecv_system_state * jrecvsys,
			unsigned char * buff,  
			long * fill, long size)
{
  unsigned char ndata = CSYS_MIDI_SYSTEM_SYSEX_END;
  unsigned char vdata = CSYS_MIDI_SYSTEM_SYSEX_END;
  unsigned char count = 0;
  int has_count = 0;
  int dstart, dsize, update;

  dstart = NSYS_SM_CD_COMMON_LOC_FIELDS;

  if ((has_count = (ps[NSYS_SM_CD_COMMON_LOC_TOC] & NSYS_SM_CD_COMMON_TOC_CHKC)))
    {
      count = ps[NSYS_SM_CD_COMMON_LOC_FIELDS];
      dstart++;
    }

  if (ps[NSYS_SM_CD_COMMON_LOC_TOC] & NSYS_SM_CD_COMMON_TOC_CHKV)
    {
      ndata = ps[dstart] & NSYS_SM_RV_CLRF;
      if (ps[dstart] & NSYS_SM_RV_CHKF)
	vdata = ps[dstart + 1] & NSYS_SM_RV_CLRF;
    }

  dsize = ((ndata != CSYS_MIDI_SYSTEM_SYSEX_END) + 
	   (vdata != CSYS_MIDI_SYSTEM_SYSEX_END));

  if (has_count)
    {
      if ((update = (count != jrecvsys->chapterd_sck_count)))
	jrecvsys->chapterd_sck_count = count;
    }
  else
    update = ((ndata != jrecvsys->chapterd_sck_data1) ||
	      (vdata != jrecvsys->chapterd_sck_data2));

  if (update)
    switch (dsize) {
    case 0:
      if (nsys_netin_journal_addcmd_one(sptr, buff, fill, size, 
					CSYS_MIDI_SYSTEM_UNUSED2))
	return NSYS_JOURNAL_FILLEDBUFF;
      
      if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	printf("Common %hhu\n", CSYS_MIDI_SYSTEM_UNUSED2);
      break;
    case 1:
      if (nsys_netin_journal_addcmd_two(sptr, buff, fill, size, 
					CSYS_MIDI_SYSTEM_UNUSED2, 
					ndata))
	return NSYS_JOURNAL_FILLEDBUFF;
      
      if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	printf("Common %hhu (%hhu) --> (%hhu)\n", 
	       CSYS_MIDI_SYSTEM_UNUSED2, 
	       jrecvsys->chapterd_sck_data1, ndata);
      break;
    case 2:
      if (nsys_netin_journal_addcmd_three(sptr, buff, fill, size, 
					  CSYS_MIDI_SYSTEM_UNUSED2, 
					  ndata, vdata))
	return NSYS_JOURNAL_FILLEDBUFF;
      
      if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	printf("Common %hhu (%hhu %hhu) --> (%hhu %hhu)\n", 
	       CSYS_MIDI_SYSTEM_UNUSED2, 
	       jrecvsys->chapterd_sck_data1, 
	       jrecvsys->chapterd_sck_data2, 
	       ndata, vdata);
      break;
    }

  if (ndata != jrecvsys->chapterd_sck_data1)
    jrecvsys->chapterd_sck_data1 = ndata;

  if (ndata != jrecvsys->chapterd_sck_data2)
    jrecvsys->chapterd_sck_data2 = vdata;

  return NSYS_JOURNAL_RECOVERED;
}

/****************************************************************/
/*           process System Chapter D/Y (unused RealTime)       */
/****************************************************************/


int nsys_netin_jrec_rty(nsys_source * sptr, unsigned char * ps,
			nsys_netout_jrecv_system_state * jrecvsys,
			unsigned char * buff,  
			long * fill, long size)
     
{
  unsigned char rty;

  if (ps[NSYS_SM_CD_REALTIME_LOC_TOC] & NSYS_SM_CD_REALTIME_TOC_CHKC)
    {
      rty = ps[NSYS_SM_CD_REALTIME_LOC_FIELDS];

      if (rty != jrecvsys->chapterd_rty)
	{
	  if (nsys_netin_journal_addcmd_one(sptr, buff, fill, size, 
					    CSYS_MIDI_SYSTEM_TICK))
	    return NSYS_JOURNAL_FILLEDBUFF;
	  
	  if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	    printf("RealTime 0xF9 (%hhu) --> (%hhu)\n", jrecvsys->chapterd_rty, rty);
	  
	  jrecvsys->chapterd_rty = rty;
	}
    }

  return NSYS_JOURNAL_RECOVERED;
}

/****************************************************************/
/*           process System Chapter D/Z (unused RealTime)       */
/****************************************************************/


int nsys_netin_jrec_rtz(nsys_source * sptr, unsigned char * ps,
			nsys_netout_jrecv_system_state * jrecvsys,
			unsigned char * buff,  
			long * fill, long size)
     
{
  unsigned char rtz;

  if (ps[NSYS_SM_CD_REALTIME_LOC_TOC] & NSYS_SM_CD_REALTIME_TOC_CHKC)
    {
      rtz = ps[NSYS_SM_CD_REALTIME_LOC_FIELDS];

      if (rtz != jrecvsys->chapterd_rtz)
	{
	  if (nsys_netin_journal_addcmd_one(sptr, buff, fill, size, 
					    CSYS_MIDI_SYSTEM_UNUSED3))
	    return NSYS_JOURNAL_FILLEDBUFF;
	  
	  if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	    printf("RealTime 0xFD (%hhu) --> (%hhu)\n", jrecvsys->chapterd_rtz, rtz);
	  
	  jrecvsys->chapterd_rtz = rtz;
	}
    }

  return NSYS_JOURNAL_RECOVERED;
}

/****************************************************************/
/*           process System Chapter V (active sense)            */
/****************************************************************/


int nsys_netin_jrec_sense(nsys_source * sptr, unsigned char * ps,
			  nsys_netout_jrecv_system_state * jrecvsys,
			  unsigned char * buff,  
			  long * fill, long size)

{
  unsigned char count;

  count = (ps[NSYS_SM_CV_LOC_COUNT] & NSYS_SM_CLRS);

  if (count != jrecvsys->chapterv_count)
    {
      /* a simple-minded repair approach -- later, consider sensing */
      /* the disappearance of active sense commands in the stream,  */
      /* and synthesizing them on the fly.                          */

      if (nsys_netin_journal_addcmd_one(sptr, buff, fill, size, CSYS_MIDI_SYSTEM_SENSE))
	  return NSYS_JOURNAL_FILLEDBUFF;

      if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	printf("Active Sense (%hhu) --> (%hhu)\n", jrecvsys->chapterv_count, count);
      
      jrecvsys->chapterv_count = count;
    }

  return NSYS_JOURNAL_RECOVERED;
}

/****************************************************************/
/*           process System Chapter Q (sequencer)               */
/****************************************************************/


int nsys_netin_jrec_sequence(nsys_source * sptr, unsigned char * ps,
			     nsys_netout_jrecv_system_state * jrecvsys,
			     unsigned char * buff,  
			     long * fill, long size)

{
  long chapter_clocks, nearest_pp;
  int chapter_running, shadow_running;
  int chapter_downbeat, chapter_size;

  /*****************************************************/
  /* leave quickly if chapter and shadow are identical */
  /*****************************************************/

  chapter_size = (ps[NSYS_SM_CQ_LOC_HDR] & NSYS_SM_CQ_HDR_CHKC) ? 3 : 1;

  if ((ps[NSYS_SM_CQ_LOC_HDR] & NSYS_SM_CLRS) == 
      jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_HDR])
    {
      if (chapter_size == 3)
	{
	  if ((ps[NSYS_SM_CQ_LOC_FIELDS] == 
	       jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_FIELDS]) &&
	      (ps[NSYS_SM_CQ_LOC_FIELDS + 1] == 
	       jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_FIELDS + 1]))
	    return NSYS_JOURNAL_RECOVERED;	
	}
      else
	return NSYS_JOURNAL_RECOVERED;	
    }

  /********************************************************/
  /* extract chapter and shadow state, then update shadow */
  /********************************************************/

  shadow_running = jrecvsys->chapterq_shadow[NSYS_SM_CQ_LOC_HDR] & NSYS_SM_CQ_HDR_CHKN;
  memcpy(jrecvsys->chapterq_shadow, ps, chapter_size);

  chapter_running = ps[NSYS_SM_CQ_LOC_HDR] & NSYS_SM_CQ_HDR_CHKN;
  chapter_downbeat = ps[NSYS_SM_CQ_LOC_HDR] & NSYS_SM_CQ_HDR_CHKD;

  if (ps[NSYS_SM_CQ_LOC_HDR] & NSYS_SM_CQ_HDR_CHKC)
    chapter_clocks = (ps[NSYS_SM_CQ_LOC_FIELDS + 1] +
		      (ps[NSYS_SM_CQ_LOC_FIELDS] << 8) +
		      ((ps[NSYS_SM_CQ_LOC_HDR] & NSYS_SM_CQ_TOP_MASK) << 16));
  else
    chapter_clocks = 0;

  /******************************************************************/
  /* Do repairs -- artifact-laden repairs done here for simplicity. */
  /*   A production implementation would do incremental updating.   */
  /******************************************************************/

  if (shadow_running)
    {
      if (nsys_netin_journal_addcmd_one(sptr, buff, fill, size, 
					CSYS_MIDI_SYSTEM_STOP))
	return NSYS_JOURNAL_FILLEDBUFF;
      
      if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	printf("Sequencer STOP issued.\n");
    }

  nearest_pp = chapter_clocks / 6;

  if (nsys_netin_journal_addcmd_three(sptr, buff, fill, size, 
				      CSYS_MIDI_SYSTEM_SONG_PP, (unsigned char)
				      (nearest_pp & 0x007F), (unsigned char)
				      ((nearest_pp >> 7) & 0x007F)))
    return NSYS_JOURNAL_FILLEDBUFF;
  
  if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
    printf("Song Position moved to %li clocks.\n", nearest_pp*6);

  if (chapter_downbeat)
    {
      if (nsys_netin_journal_addcmd_one(sptr, buff, fill, size, 
					CSYS_MIDI_SYSTEM_CONTINUE))
	return NSYS_JOURNAL_FILLEDBUFF;

      if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	printf("Sequencer CONTINUE issued.\n");

      chapter_clocks -=  nearest_pp*6 - 1;

      while (chapter_clocks--)
	if (nsys_netin_journal_addcmd_one(sptr, buff, fill, size, 
					  CSYS_MIDI_SYSTEM_CLOCK))
	  return NSYS_JOURNAL_FILLEDBUFF;
	else
	  if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	    printf("Sequencer CLOCK issued.\n");

      if (!chapter_running)
	{
	  if (nsys_netin_journal_addcmd_one(sptr, buff, fill, size, 
					    CSYS_MIDI_SYSTEM_STOP))
	    return NSYS_JOURNAL_FILLEDBUFF;
	  
	  if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	    printf("Sequencer STOP issued.\n");
	}
    }
  else
    if (chapter_running)
      {
	if (nsys_netin_journal_addcmd_one(sptr, buff, fill, size, 
					  CSYS_MIDI_SYSTEM_CONTINUE))
	  return NSYS_JOURNAL_FILLEDBUFF;
	
	if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	  printf("Sequencer CONTINUE issued.\n");
      }

  return NSYS_JOURNAL_RECOVERED;
}

/****************************************************************/
/*           process System Chapter F (MIDI Time Code)          */
/****************************************************************/


int nsys_netin_jrec_timecode(nsys_source * sptr, unsigned char * ps,
			     nsys_netout_jrecv_system_state * jrecvsys,
			     unsigned char * buff,  
			     long * fill, long size)

{
  int complete_equal, partial_equal;

  /* fast path checks: breaks denote cases that may need repair */

  complete_equal = partial_equal = 0;

  do {

    if (ps[NSYS_SM_CF_LOC_HDR] & NSYS_SM_CF_HDR_CHKC)
      {
	if (!jrecvsys->chapterf_has_complete)
	  break;

	if ((ps[NSYS_SM_CF_LOC_HDR] & NSYS_SM_CF_HDR_CHKQ) ? 
	    !jrecvsys->chapterf_quarter : jrecvsys->chapterf_quarter)
	  break;
	
	if (memcmp(&(ps[NSYS_SM_CF_LOC_FIELDS]), jrecvsys->chapterf_complete, 
		   NSYS_SM_CF_SIZE_COMPLETE))
	  break;

	complete_equal = 1;
      }

    if (ps[NSYS_SM_CF_LOC_HDR] & NSYS_SM_CF_HDR_CHKP)
      {
	if (!jrecvsys->chapterf_has_partial)
	  break;

	if ((ps[NSYS_SM_CF_LOC_HDR] & NSYS_SM_CF_HDR_CHKD) ? 
	    !jrecvsys->chapterf_direction : jrecvsys->chapterf_direction)
	  break;

	if ((ps[NSYS_SM_CF_LOC_HDR] & NSYS_SM_CF_POINT_MASK) != jrecvsys->chapterf_point)
	  break;

	if (ps[NSYS_SM_CF_LOC_HDR] & NSYS_SM_CF_HDR_CHKC)
	  {
	    if (memcmp(&(ps[NSYS_SM_CF_LOC_FIELDS + NSYS_SM_CF_SIZE_COMPLETE]),
		       jrecvsys->chapterf_partial, NSYS_SM_CF_SIZE_PARTIAL))
	      break;
	  }
	else
	  {
	    if (memcmp(&(ps[NSYS_SM_CF_LOC_FIELDS]), jrecvsys->chapterf_partial, 
		       NSYS_SM_CF_SIZE_PARTIAL))
	      break;
	  }

	partial_equal = 1;
      }
    else
      if (jrecvsys->chapterf_has_partial)
	break;

    return NSYS_JOURNAL_RECOVERED;   /* no repairs or state updates are needed */

  } while (0);

  /* repairs start here */

  if ((ps[NSYS_SM_CF_LOC_HDR] & NSYS_SM_CF_HDR_CHKC) && !complete_equal)
    {
      jrecvsys->chapterf_has_complete = 1;
      memcpy(jrecvsys->chapterf_complete, &(ps[NSYS_SM_CF_LOC_FIELDS]),
	     NSYS_SM_CF_SIZE_COMPLETE);
      jrecvsys->chapterf_quarter = ((ps[NSYS_SM_CF_LOC_HDR] & NSYS_SM_CF_HDR_CHKQ)
				    ? 1 : 0);

      if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	  printf("Time Code COMPLETE repair.\n");

      /*
       * Later, add commands to gracefully issue Full Frame or QF command(s)
       */
    }

  if ((ps[NSYS_SM_CF_LOC_HDR] & NSYS_SM_CF_HDR_CHKP) && !partial_equal)
    {
      jrecvsys->chapterf_has_partial = 1;
      jrecvsys->chapterf_point = ps[NSYS_SM_CF_LOC_HDR] & NSYS_SM_CF_POINT_MASK;
      jrecvsys->chapterf_direction = 
	(ps[NSYS_SM_CF_LOC_HDR] & NSYS_SM_CF_HDR_CHKD) ? 1 : 0;

      if (ps[NSYS_SM_CF_LOC_HDR] & NSYS_SM_CF_HDR_CHKC)
	memcpy(jrecvsys->chapterf_partial, 
	       &(ps[NSYS_SM_CF_LOC_FIELDS + NSYS_SM_CF_SIZE_COMPLETE]), 
	       NSYS_SM_CF_SIZE_PARTIAL);
      else
	memcpy(jrecvsys->chapterf_partial, &(ps[NSYS_SM_CF_LOC_FIELDS]), 
	       NSYS_SM_CF_SIZE_PARTIAL);

      if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	  printf("Time Code PARTIAL repair.\n");

      /*
       * Later, add commands to gracefully issue QF command(s)
       */
    }

  if (!(ps[NSYS_SM_CF_LOC_HDR] & NSYS_SM_CF_HDR_CHKP))
    {
      jrecvsys->chapterf_has_partial = 0;

      if (ps[NSYS_SM_CF_LOC_HDR] & NSYS_SM_CF_HDR_CHKC)
	jrecvsys->chapterf_point = ps[NSYS_SM_CF_LOC_HDR] & NSYS_SM_CF_POINT_MASK;

      jrecvsys->chapterf_direction = 
	(ps[NSYS_SM_CF_LOC_HDR] & NSYS_SM_CF_HDR_CHKD) ? 1 : 0;

      if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	  printf("Time Code PARTIAL zeroing.\n");
    }

  return NSYS_JOURNAL_RECOVERED;
}


/****************************************************************/
/*      track commands for System Chapter F (MIDI Time Code)    */
/****************************************************************/

void nsys_netin_track_timecode(nsys_netout_jrecv_system_state *
			       jrecvsys, unsigned char ndata)

{
  unsigned char frames, seconds, minutes, hours, type, typeframe;
  unsigned char point, direction, idnum;
  unsigned char * p;

  point = jrecvsys->chapterf_point;
  direction = jrecvsys->chapterf_direction;
  jrecvsys->chapterf_point = (idnum = (ndata & NSYS_SM_CF_IDNUM_MASK) >> 4);

  /* handle QF commands with expected idnums */

  if ((!direction && (((point + 1) & NSYS_SM_CF_POINT_MASK) == idnum)) ||
      (direction && (((point - 1) & NSYS_SM_CF_POINT_MASK) == idnum)))
    {
      switch (idnum) {               	     
      case NSYS_SM_CF_IDNUM_FR_LSN:
      case NSYS_SM_CF_IDNUM_HR_MSN:
	if ((direction && (idnum == NSYS_SM_CF_IDNUM_FR_LSN)) ||
	    (!direction && (idnum == NSYS_SM_CF_IDNUM_HR_MSN)))
	  {
	    /* handle QF commands that end the frame */
	    
	    if (jrecvsys->chapterf_has_partial)
	      {
		jrecvsys->chapterf_has_partial = 0;
		jrecvsys->chapterf_has_complete = 1;
		jrecvsys->chapterf_quarter = 1;

		memcpy(jrecvsys->chapterf_complete, jrecvsys->chapterf_partial,
		       NSYS_SM_CF_SIZE_PARTIAL);

		if (!direction)
		  {
		    p = &(jrecvsys->chapterf_complete[NSYS_SM_CF_QF_LOC_HR_MSN]);
		    (*p) &= NSYS_SM_CF_ODD_CLR;
		    (*p) |= (ndata & NSYS_SM_CF_PAYLOAD_MASK);
		  }
		else
		  {
		    p = &(jrecvsys->chapterf_complete[NSYS_SM_CF_QF_LOC_FR_LSN]);
		    (*p) &= NSYS_SM_CF_EVEN_CLR;
		    (*p) |= (ndata << 4);
		  }
		
		/* for forward tape motion, adjust complete by two frames */
		
		if (!direction)
		  {
		    p = jrecvsys->chapterf_complete;
		    
		    frames = ((p[0] >> 4) & 0x0F) + ((p[0] & 0x01) << 4);
		    if ((type = p[3] & 0x06))
		      typeframe = (type == 2) ? 25 : 30;
		    else
		      typeframe = 24;
		    
		    if ((frames += 2) >= typeframe)
		      {
			frames -= typeframe;
			seconds = ((p[1] >> 4) & 0x0F) + ((p[1] & 0x03) << 4);
			if ((++seconds) >= 60)
			  {
			    seconds -= 60;
			    minutes = ((p[2] >> 4) & 0x0F) + ((p[2] & 0x03) << 4);
			    if ((++minutes) >= 60)
			      {
				minutes -= 60;
				hours = ((p[3] >> 4) & 0x0F) + ((p[3] & 0x01) << 4);
				if ((++hours) >= 24)
				  hours -= 24;
				p[3] = ((0x0F & hours) << 4)  | (hours >> 4) | type;
			      }
			    p[2] = ((0x0F & minutes) << 4) | (minutes >> 4);
			  }
			p[1] = ((0x0F & seconds) << 4) | (seconds >> 4);
		      }
		    p[0] = ((0x0F & frames) << 4)  | (frames >> 4);
		  }

	      }
	    return;
	  }

	/* handle QF commands that start a frame */

	if (!(jrecvsys->chapterf_has_partial))
	  {
	    jrecvsys->chapterf_has_partial = 1;
	    memset(jrecvsys->chapterf_partial, 0, NSYS_SM_CF_SIZE_PARTIAL);
	  }
      default:           /* handle all expected non-frame-ending MT values */
	if (jrecvsys->chapterf_has_partial)
	  {
	    p = &(jrecvsys->chapterf_partial[idnum >> 1]);
	    if (idnum & 0x01)
	      {
		(*p) &= NSYS_SM_CF_ODD_CLR;
		(*p) |= (ndata & NSYS_SM_CF_PAYLOAD_MASK);
	      }
	    else
	      {
		(*p) &= NSYS_SM_CF_EVEN_CLR;
		(*p) |= (ndata << 4);
	      }
	  }
	return;
      }
    }

  /* handle tape direction change that happens in a legal way */
  
  if ((idnum == NSYS_SM_CF_IDNUM_FR_LSN) || (idnum == NSYS_SM_CF_IDNUM_HR_MSN))
    {
      jrecvsys->chapterf_has_partial = 1;

      if (idnum == NSYS_SM_CF_IDNUM_FR_LSN)
	jrecvsys->chapterf_direction = 0;
      else
	jrecvsys->chapterf_direction = 1;
      
      memset((p = jrecvsys->chapterf_partial), 0, NSYS_SM_CF_SIZE_PARTIAL);
      
      if (idnum == NSYS_SM_CF_IDNUM_FR_LSN)
	{
	  (*p) &= NSYS_SM_CF_EVEN_CLR;
	  (*p) |= (ndata << 4);
	}
      else
	{
	  p += (NSYS_SM_CF_SIZE_PARTIAL - 1);
	  (*p) &= NSYS_SM_CF_ODD_CLR;
	  (*p) |= (ndata & NSYS_SM_CF_PAYLOAD_MASK);
	}	
      return;
    }

  /* handle "wrong direction guess" for an earlier unexpected QF command */

  if (!direction && (((point - 1) & NSYS_SM_CF_POINT_MASK) == idnum))
    {
      jrecvsys->chapterf_direction = 1;
      return;
    }

  /* handle unexpected QF commands */

  jrecvsys->chapterf_has_partial = jrecvsys->chapterf_direction = 0;
}


/****************************************************************/
/*          process System Chapter X (System Exclusive)         */
/*  (does not process unfinished command logs and other items)  */
/****************************************************************/

int nsys_netin_jrec_sysex(nsys_source * sptr, unsigned char * ps,
			  int syslen,
			  nsys_netout_jrecv_system_state * jrecvsys,
			  unsigned char * buff,  
			  long * fill, long size)

{
  int sysex_len;
  unsigned char sysex_hdr;
  unsigned char gmreset_state, mvolume_lsb_state, mvolume_msb_state;
  unsigned char gmreset_has_tcount, gmreset_tcount;
  unsigned char sysex_has_tcount, sysex_tcount;
  unsigned char sysex_has_first, sysex_first;
  unsigned char sysexbuff[NSYS_SM_CX_SIZE_MAXLOG - 1];

  gmreset_state = gmreset_tcount = gmreset_has_tcount = 0;
  mvolume_lsb_state = mvolume_msb_state = 0;
  
  /* parse each log in list */

  do
    {
      sysex_has_tcount = sysex_has_first = sysex_first = 0;
      
      /* parse log header */

      if ((syslen -= NSYS_SM_CX_SIZE_HDR) < 0)
	return NSYS_JOURNAL_CORRUPTED;
      
      if ((sysex_hdr = *(ps++)) & NSYS_SM_CX_HDR_CHKT)
	{
	  if ((syslen -= NSYS_SM_CX_SIZE_TCOUNT) < 0)
	    return NSYS_JOURNAL_CORRUPTED;
	  sysex_has_tcount = 1;
	  sysex_tcount = ps[0];
	  ps += NSYS_SM_CX_SIZE_TCOUNT;
	}
      
      if (sysex_hdr & NSYS_SM_CX_HDR_CHKC)
	{
	  if ((syslen -= NSYS_SM_CX_SIZE_COUNT) < 0)
	    return NSYS_JOURNAL_CORRUPTED;
	  ps += NSYS_SM_CX_SIZE_COUNT;
	}

      if (sysex_hdr & NSYS_SM_CX_HDR_CHKF)
	{
	  if ((--syslen) >= 0)
	    sysex_first = NSYS_SM_CX_DATA_MASK & ps[0];
	  else
	    return NSYS_JOURNAL_CORRUPTED;

	  while (NSYS_SM_CX_DATA_CHKEND & ps[0])
	    if ((--syslen) >= 0)
	      sysex_first = ((sysex_first << 7) + 
			     (NSYS_SM_CX_DATA_MASK & (++ps)[0]));
	    else
	      return NSYS_JOURNAL_CORRUPTED;

	  sysex_has_first = 1;
	  ps++;
	}

      if ((!syslen) && (sysex_hdr & NSYS_SM_CX_HDR_CHKD))
	return NSYS_JOURNAL_CORRUPTED;

      sysex_len = 0;

      if (sysex_hdr & NSYS_SM_CX_HDR_CHKD)
	do
	  {
	    if (sysex_len < (NSYS_SM_CX_SIZE_MAXLOG - 1))
	      sysexbuff[sysex_len++] = (*ps) & NSYS_SM_CX_DATA_CLREND;
	    
	    syslen--;
	    
	    if ((*(ps++)) & NSYS_SM_CX_DATA_CHKEND)
	      break;
	    
	    if (!syslen)
	      return NSYS_JOURNAL_CORRUPTED;
	    
	  } while (1);

      /* skip processing for unimplemented features, cancelled commands */

      if ((sysex_has_first && sysex_first) /* windowed data */ ||
	  ((sysex_hdr & NSYS_SM_CX_STA_MASK) == NSYS_SM_CX_STA_UNFINISHED) ||
	  ((sysex_hdr & NSYS_SM_CX_STA_MASK) == NSYS_SM_CX_STA_CANCELLED))
	{
	  if (syslen)
	    continue;
	  break;
	}
      
      /* update GMRESET, MVOLUME state for log */
      
      if ((sysex_len == (NSYS_SM_CX_SIZE_GMRESET - 1)) && 
	  (!memcmp(&(nsys_netout_sysconst_gmreset[1]), sysexbuff, 3)) &&
	  ((sysexbuff[3] == NSYS_SM_CX_GMRESET_ONVAL) ||
	   (sysexbuff[3] == NSYS_SM_CX_GMRESET_OFFVAL)))
	{
	  gmreset_state = sysexbuff[3] | NSYS_SM_RV_SETF;
	  if (sysex_has_tcount)
	    {
	      gmreset_tcount = sysex_tcount;
	      gmreset_has_tcount = 1;
	    }
	  else
	    gmreset_has_tcount = 0;
	}
      
      if ((sysex_len == NSYS_SM_CX_SIZE_MVOLUME - 1) && 
	  !memcmp(&(nsys_netout_sysconst_mvolume[1]), sysexbuff, 4))
	{
	  mvolume_lsb_state = sysexbuff[4] | NSYS_SM_RV_SETF;
	  mvolume_msb_state = sysexbuff[5];
	}
      
    } while (syslen);
  
  /* do MVOLUME repairs */
  
  if (mvolume_lsb_state && 
      ((mvolume_lsb_state != jrecvsys->chapterx_mvolume_lsb) ||
       (mvolume_msb_state != jrecvsys->chapterx_mvolume_msb)))
    {
      if (nsys_netin_journal_addcmd_three
	  (sptr, buff, fill, size, CSYS_MIDI_MVOLUME,
	   mvolume_lsb_state & NSYS_SM_RV_CLRF, mvolume_msb_state))
	return NSYS_JOURNAL_FILLEDBUFF;
      
      if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
	printf("MVOLUME (%hhu, %hhu) --> (%hhu, %hhu)\n", 
	       jrecvsys->chapterx_mvolume_lsb & NSYS_SM_RV_CLRF, 
	       jrecvsys->chapterx_mvolume_msb,
	       mvolume_lsb_state & NSYS_SM_RV_CLRF, mvolume_msb_state);
      
      jrecvsys->chapterx_mvolume_lsb = mvolume_lsb_state;
      jrecvsys->chapterx_mvolume_msb = mvolume_msb_state;
    }
  
  /* do GMRESET repairs */
  
  do {
    
    if (jrecvsys->chapterx_gmreset == gmreset_state)
      {
	if (!gmreset_has_tcount)
	  break;
	
	if (gmreset_state == (NSYS_SM_RV_SETF | NSYS_SM_CX_GMRESET_ONVAL))
	  {
	    if (jrecvsys->chapterx_gmreset_on_count == gmreset_state)
	      break;
	  }
	else
	  if (jrecvsys->chapterx_gmreset_off_count == gmreset_state)
	    break;
      }
    else
      jrecvsys->chapterx_gmreset = gmreset_state;
    
    if (gmreset_has_tcount)
      {
	if (gmreset_state == (NSYS_SM_RV_SETF | NSYS_SM_CX_GMRESET_ONVAL))
	  {
	    if (jrecvsys->chapterx_gmreset_on_count != gmreset_state)
	      jrecvsys->chapterx_gmreset_on_count = gmreset_state;
	  }
	else
	  if (jrecvsys->chapterx_gmreset_off_count != gmreset_state)
	    jrecvsys->chapterx_gmreset_off_count = gmreset_state;
      }
    
    if (nsys_netin_journal_addcmd_two(sptr, buff, fill, size,
				      CSYS_MIDI_GMRESET,
				      gmreset_state & NSYS_SM_RV_CLRF))
      return NSYS_JOURNAL_FILLEDBUFF;
    
    if (NSYS_JOURNAL_DEBUG == NSYS_JOURNAL_DEBUG_ON)
      printf("GMRESET %s \n", 
	     (gmreset_state == (NSYS_SM_RV_SETF | NSYS_SM_CX_GMRESET_ONVAL))
	     ? "On" : "Off");

    nsys_netin_journal_clear_active(CSYS_MIDI_GMRESET);

  } 
  while (0);

  return NSYS_JOURNAL_RECOVERED;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                  low-level journal functions                 */
/*______________________________________________________________*/

/****************************************************************/
/*           adds a 3-octet MIDI command to the buffer          */
/****************************************************************/

int nsys_netin_journal_addcmd_three(nsys_source * sptr, unsigned char * buff, 
				    long * fill, long size,
				    unsigned char cmd, unsigned char ndata,
				    unsigned char vdata)
			      
{
  long idx;

  if ((size - (*fill)) < 4)           /* 3 octets + mset */
    return NSYS_JOURNAL_FILLEDBUFF;

  idx = *fill;

  buff[idx++] = cmd;
  buff[idx++] = ndata;
  buff[idx++] = vdata;
  buff[idx++] = (unsigned char)(sptr->mset);

  *fill = idx;

  return NSYS_JOURNAL_RECOVERED;
}

/****************************************************************/
/*           adds a 2-octet MIDI command to the buffer          */
/****************************************************************/

int nsys_netin_journal_addcmd_two(nsys_source * sptr, unsigned char * buff, 
				    long * fill, long size,
				    unsigned char cmd, unsigned char ndata)
			      
{
  long idx;

  if ((size - (*fill)) < 3)           /* 2 octets + mset */
    return NSYS_JOURNAL_FILLEDBUFF;

  idx = *fill;

  buff[idx++] = cmd;
  buff[idx++] = ndata;
  buff[idx++] = (unsigned char)(sptr->mset);

  *fill = idx;

  return NSYS_JOURNAL_RECOVERED;
}

/****************************************************************/
/*           adds a 1-octet MIDI command to the buffer          */
/****************************************************************/

int nsys_netin_journal_addcmd_one(nsys_source * sptr, unsigned char * buff, 
				  long * fill, long size, unsigned char cmd)
			      
{
  long idx;

  if ((size - (*fill)) < 2)           /* 1 octet + mset */
    return NSYS_JOURNAL_FILLEDBUFF;

  idx = *fill;

  buff[idx++] = cmd;
  buff[idx++] = (unsigned char)(sptr->mset);

  *fill = idx;

  return NSYS_JOURNAL_RECOVERED;
}

/****************************************************************/
/*           adds a parameter transaction to the buffer         */
/****************************************************************/

int nsys_netin_journal_trans(nsys_source * sptr, unsigned char * buff, 
			     long * fill, long size, unsigned char chan,
			     int flags, unsigned char msb_num, 
			     unsigned char lsb_num, unsigned char msb_val, 
			     unsigned char lsb_val)

{  
  unsigned char msb_cmd, lsb_cmd;

  msb_cmd = ((flags & NSYS_SM_CM_TRANS_RPN) ? CSYS_MIDI_CC_RPN_MSB :
	     CSYS_MIDI_CC_NRPN_MSB);
  lsb_cmd = ((flags & NSYS_SM_CM_TRANS_RPN) ? CSYS_MIDI_CC_RPN_LSB :
	     CSYS_MIDI_CC_NRPN_LSB);

  if (!(flags & NSYS_SM_CM_TRANS_NO_OPEN))
    {
      if (nsys_netin_journal_addcmd_three(sptr, buff, fill, size,
				    chan | CSYS_MIDI_CC, msb_cmd, msb_num))
	return NSYS_JOURNAL_FILLEDBUFF;

      if (nsys_netin_journal_addcmd_three(sptr, buff, fill, size,
				    chan | CSYS_MIDI_CC, lsb_cmd, lsb_num))
	return NSYS_JOURNAL_FILLEDBUFF;
    }

  if (!(flags & NSYS_SM_CM_TRANS_NO_SET))
    {
      if (nsys_netin_journal_addcmd_three(sptr, buff, fill, size, chan | CSYS_MIDI_CC,
				    CSYS_MIDI_CC_DATAENTRY_MSB, msb_val))
	return NSYS_JOURNAL_FILLEDBUFF;

      if (nsys_netin_journal_addcmd_three(sptr, buff, fill, size, chan | CSYS_MIDI_CC,
				    CSYS_MIDI_CC_DATAENTRY_LSB, lsb_val))
	return NSYS_JOURNAL_FILLEDBUFF;
    }

  if (!(flags & NSYS_SM_CM_TRANS_NO_CLOSE))
    {
      if (nsys_netin_journal_addcmd_three(sptr, buff, fill, size, chan | CSYS_MIDI_CC,
				    msb_cmd, CSYS_MIDI_RPN_NULL_MSB))
	return NSYS_JOURNAL_FILLEDBUFF;

      if (nsys_netin_journal_addcmd_three(sptr, buff, fill, size, chan | CSYS_MIDI_CC,
				    lsb_cmd, CSYS_MIDI_RPN_NULL_LSB))
	return NSYS_JOURNAL_FILLEDBUFF;
    }

  return NSYS_JOURNAL_RECOVERED;
}
/*************************************************************/
/*           adds a button transaction to the buffer         */
/*************************************************************/

int nsys_netin_journal_button_trans(nsys_source * sptr, unsigned char * buff, 
				    long * fill, long size, 
				    unsigned char chan, int flags,
				    unsigned char msb_num, 
				    unsigned char lsb_num, short count)

{  
  unsigned char msb_cmd, lsb_cmd, minus;

  if ((minus = (count < 0)))
    count = - count;

  msb_cmd = ((flags & NSYS_SM_CM_TRANS_RPN) ? CSYS_MIDI_CC_RPN_MSB :
	     CSYS_MIDI_CC_NRPN_MSB);
  lsb_cmd = ((flags & NSYS_SM_CM_TRANS_RPN) ? CSYS_MIDI_CC_RPN_LSB :
	     CSYS_MIDI_CC_NRPN_LSB);

  if (!(flags & NSYS_SM_CM_TRANS_NO_OPEN))
    {
      if (nsys_netin_journal_addcmd_three(sptr, buff, fill, size,
					  chan | CSYS_MIDI_CC, msb_cmd, msb_num))
	return NSYS_JOURNAL_FILLEDBUFF;

      if (nsys_netin_journal_addcmd_three(sptr, buff, fill, size,
					  chan | CSYS_MIDI_CC, lsb_cmd, lsb_num))
	return NSYS_JOURNAL_FILLEDBUFF;
    }

  if (!(flags & NSYS_SM_CM_TRANS_NO_SET))
    {
      while (count--)
	if (nsys_netin_journal_addcmd_three(sptr, buff, fill, size, chan | CSYS_MIDI_CC,
					    minus ? CSYS_MIDI_CC_DATAENTRYMINUS :
					    CSYS_MIDI_CC_DATAENTRYPLUS, 0))
	  return NSYS_JOURNAL_FILLEDBUFF;
    }

  if (!(flags & NSYS_SM_CM_TRANS_NO_CLOSE))
    {
      if (nsys_netin_journal_addcmd_three(sptr, buff, fill, size, chan | CSYS_MIDI_CC,
					  msb_cmd, CSYS_MIDI_RPN_NULL_MSB))
	return NSYS_JOURNAL_FILLEDBUFF;

      if (nsys_netin_journal_addcmd_three(sptr, buff, fill, size, chan | CSYS_MIDI_CC,
					  lsb_cmd, CSYS_MIDI_RPN_NULL_LSB))
	return NSYS_JOURNAL_FILLEDBUFF;
    }

  return NSYS_JOURNAL_RECOVERED;
}

/****************************************************************/
/*            sender state:  clear all active state             */
/****************************************************************/

void nsys_netin_journal_clear_active(unsigned char cmd)

{
  /* to do 
   *
   * cmd codes CSYS_MIDI_SYSTEM_RESET or CSYS_MIDI_GMRESET
   *
   * clear journal state defined as "active"
   *
   */
}

/* end Network library -- receiver journal functions */

