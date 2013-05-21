
/*
#    Sfront, a SAOL to C translator    
#    This file: Network library -- sender journal functions
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
/*     high-level functions: sending recovery journals          */
/*______________________________________________________________*/

/****************************************************************/
/*      generate recovery journal for an RTP packet             */
/****************************************************************/

int nsys_netin_journal_create(unsigned char * packet, int len)

{
  unsigned char * p = packet;
  int i, j, channel;
  unsigned char toc;
  nsys_netout_jsend_system_state * jsys;
  nsys_netout_jsend_state * jsend;

  /********************************/
  /* add journal header to packet */
  /********************************/

  memcpy(p, nsys_netout_jsend_header, NSYS_SM_JH_SIZE);
  p += NSYS_SM_JH_SIZE;
  len -= NSYS_SM_JH_SIZE;

  /*************************/
  /* add Systems if needed */
  /*************************/

  if (nsys_netout_jsend_system.sheader_seqnum)
    {
      jsys = &(nsys_netout_jsend_system);

      if ((len -= jsys->slen) >= 0)
	{
	  memcpy(p, jsys->sheader, NSYS_SM_SH_SIZE);
	  p += NSYS_SM_SH_SIZE;

	  /*************/
	  /* Chapter D */
	  /*************/

	  if (jsys->chapterd_seqnum)
	    {
	      memcpy(p, jsys->chapterd_front, jsys->chapterd_front_size);
	      p += jsys->chapterd_front_size;

	      if (jsys->chapterd_scj_seqnum)
		{
		  memcpy(p, jsys->chapterd_scj, jsys->chapterd_scj_size);
		  p += jsys->chapterd_scj_size;
		}

	      if (jsys->chapterd_sck_seqnum)
		{
		  memcpy(p, jsys->chapterd_sck, jsys->chapterd_sck_size);
		  p += jsys->chapterd_sck_size;
		}

	      if (jsys->chapterd_rty_seqnum)
		{
		  memcpy(p, jsys->chapterd_rty, NSYS_SM_CD_REALTIME_SIZE);
		  p += NSYS_SM_CD_REALTIME_SIZE;
		}

	      if (jsys->chapterd_rtz_seqnum)
		{
		  memcpy(p, jsys->chapterd_rtz, NSYS_SM_CD_REALTIME_SIZE);
		  p += NSYS_SM_CD_REALTIME_SIZE;
		}
	    }

	  /*************/
	  /* Chapter V */
	  /*************/

	  if (jsys->chapterv_seqnum)
	    {
	      memcpy(p, jsys->chapterv, NSYS_SM_CV_SIZE);
	      p += NSYS_SM_CV_SIZE;
	    }

	  /*************/
	  /* Chapter Q */
	  /*************/
	  
	  if (jsys->chapterq_seqnum)
	    {
	      memcpy(p, jsys->chapterq, jsys->chapterq_size);
	      p += jsys->chapterq_size;
	    }

	  /*************/
	  /* Chapter F */
	  /*************/
	  
	  if (jsys->chapterf_seqnum)
	    {
	      memcpy(p, jsys->chapterf, jsys->chapterf_size);
	      p += jsys->chapterf_size;
	    }

	  /*************/
	  /* Chapter X */
	  /*************/
	  
	  if (jsys->chapterx_seqnum)
	    for (i = 0; i < jsys->chapterx_stacklen; i++)
	      {
		memcpy(p, jsys->chapterx_stack[i]->log,
		       jsys->chapterx_stack[i]->size);

		if (!(jsys->chapterx_stack[i]->log[0] & NSYS_SM_CHKS))
		  jsys->chapterx_stack[i]->log[0] |= NSYS_SM_SETS;

		p += jsys->chapterx_stack[i]->size;
	      }

	}
      else        /* recover from overly-large system journal */
	{
	  len += jsys->slen;
	  packet[NSYS_SM_JH_LOC_FLAGS] &= NSYS_SM_JH_CLRY;
	  nsys_warning(NSYS_WARN_UNUSUAL, "Sending an incomplete journal (sys)");
	}
    }

  /****************************/
  /* add chapter(s) if needed */
  /****************************/

  for (channel = 0; channel < nsys_netout_jsend_channel_size; channel++)
    {
      jsend = &(nsys_netout_jsend[nsys_netout_jsend_channel[channel]]);

      if (jsend->cheader_seqnum)
	{	  

	  if ((len -= (NSYS_SM_CH_LENMASK & jsend->clen)) < 0)
	    {
	      len += (NSYS_SM_CH_LENMASK & jsend->clen);

	      if (packet[NSYS_SM_JH_LOC_FLAGS] & NSYS_SM_JH_CHANMASK)
		packet[NSYS_SM_JH_LOC_FLAGS]--;
	      else
		packet[NSYS_SM_JH_LOC_FLAGS] &= NSYS_SM_JH_CLRA;

	      nsys_warning(NSYS_WARN_UNUSUAL, "Sending an incomplete journal (ch)");
	      continue;
	    }

	  memcpy(p, jsend->cheader, NSYS_SM_CH_SIZE);
	  p += NSYS_SM_CH_SIZE;

	  /*************/
	  /* Chapter P */
	  /*************/

	  if (jsend->chapterp_seqnum)
	    {
	      memcpy(p, jsend->chapterp, NSYS_SM_CP_SIZE);
	      p += NSYS_SM_CP_SIZE;
	    }

	  /*************/
	  /* Chapter C */
	  /*************/
	  
	  if (jsend->chapterc_seqnum)
	    {
	      memcpy(p, jsend->chapterc, jsend->chapterc_size);
	      p += jsend->chapterc_size;
	      if (jsend->chapterc_sset)
		{
		  for (i = NSYS_SM_CC_HDRSIZE; i < jsend->chapterc_size;
		       i += NSYS_SM_CC_LOGSIZE)
		    jsend->chapterc[i] |= NSYS_SM_SETS;
		  jsend->chapterc_sset = 0;
		}
	    }

	  /*************/
	  /* Chapter M */
	  /*************/
	  
	  if (jsend->chapterm_seqnum)
	    {
	      memcpy(p, jsend->chapterm, jsend->chapterm_size);
	      p += jsend->chapterm_size;

	      if (jsend->chapterl_size)
		{
		  memcpy(p, jsend->chapterl, jsend->chapterl_size);
		  p += jsend->chapterl_size;

		  if (jsend->chapterm_sset)
		    {
		      for (i = 0; i < jsend->chapterl_size;)
			{
			  jsend->chapterl[i] |= NSYS_SM_SETS;
			  
			  toc = jsend->chapterl[i + NSYS_SM_CM_LOC_TOC];
			  i += NSYS_SM_CM_LOGHDRSIZE;
			  
			  if (toc & NSYS_SM_CM_TOC_CHKJ)
			    i += NSYS_SM_CM_ENTRYMSB_SIZE;
			  
			  if (toc & NSYS_SM_CM_TOC_CHKK)
			    i += NSYS_SM_CM_ENTRYLSB_SIZE;
			  
			  if (toc & NSYS_SM_CM_TOC_CHKL)
			    i += NSYS_SM_CM_ABUTTON_SIZE;
			  
			  if (toc & NSYS_SM_CM_TOC_CHKM)
			    i += NSYS_SM_CM_CBUTTON_SIZE;

			  if (toc & NSYS_SM_CM_TOC_CHKN)
			    i += NSYS_SM_CM_COUNT_SIZE;
			}
		      jsend->chapterm_sset = 0;
		    }
		}
	    }

	  /*************/
	  /* Chapter W */
	  /*************/

	  if (jsend->chapterw_seqnum)
	    {
	      memcpy(p, jsend->chapterw, NSYS_SM_CW_SIZE);
	      p += NSYS_SM_CW_SIZE;
	    }

	  /*************/
	  /* Chapter N */
	  /*************/
	  
	  if (jsend->chaptern_seqnum)
	    {
	      if (jsend->chaptern_timernum)
		{
		  j = jsend->chaptern_timernum;
		  for (i = jsend->chaptern_size - NSYS_SM_CN_HDRSIZE; 
		       i >= NSYS_SM_CN_HDRSIZE; i -= NSYS_SM_CN_LOGSIZE)
		    if (jsend->chaptern[i+1] & NSYS_SM_CN_CHKY)
		      {
			if (jsend->chaptern_timer[NSYS_SM_CLRS & 
						 jsend->chaptern[i]]
			    < nsys_netout_tstamp)
			  {
			    jsend->chaptern[i+1] &= NSYS_SM_CN_CLRY;
			    jsend->chaptern_timernum--;
			  }
			if ((--j) == 0)
			  break;
		      }
		}
	      memcpy(p, jsend->chaptern, jsend->chaptern_size);
	      p += jsend->chaptern_size;
	      if (jsend->chaptern_sset)
		{
		  for (i = NSYS_SM_CN_HDRSIZE; i < jsend->chaptern_size;
		       i += NSYS_SM_CN_LOGSIZE)
		    jsend->chaptern[i] |= NSYS_SM_SETS;
		  jsend->chaptern_sset = 0;
		}
	      if (jsend->chapterb_size)
		{
		  memcpy(p, &(jsend->chapterb[jsend->chapterb_low]),
			 jsend->chapterb_size);
		  p += jsend->chapterb_size;
		}
	    }

	  /*************/
	  /* Chapter E */
	  /*************/
	  
	  if (jsend->chaptere_seqnum)
	    {
	      memcpy(p, jsend->chaptere, jsend->chaptere_size);
	      p += jsend->chaptere_size;
	      if (jsend->chaptere_sset)
		{
		  for (i = NSYS_SM_CE_HDRSIZE; i < jsend->chaptere_size;
		       i += NSYS_SM_CE_LOGSIZE)
		    jsend->chaptere[i] |= NSYS_SM_SETS;
		  jsend->chaptere_sset = 0;
		}
	    }

	  /*************/
	  /* Chapter T */
	  /*************/
	  
	  if (jsend->chaptert_seqnum)
	    {
	      memcpy(p, jsend->chaptert, NSYS_SM_CT_SIZE);
	      p += NSYS_SM_CT_SIZE;
	    }
	  	  
	  /*************/
	  /* Chapter A */
	  /*************/
	  
	  if (jsend->chaptera_seqnum)
	    {
	      memcpy(p, jsend->chaptera, jsend->chaptera_size);
	      p += jsend->chaptera_size;
	      if (jsend->chaptera_sset)
		{
		  for (i = NSYS_SM_CA_HDRSIZE; i < jsend->chaptera_size;
		       i += NSYS_SM_CA_LOGSIZE)
		    jsend->chaptera[i] |= NSYS_SM_SETS;
		  jsend->chaptera_sset = 0;
		}
	    }

	}
    }

  /****************/
  /* set S flags  */
  /****************/

  while (nsys_netout_jsend_slist_size > 0)
    *(nsys_netout_jsend_slist[--nsys_netout_jsend_slist_size]) |= NSYS_SM_SETS; 

  return (p - packet);
}

/****************************************************************/
/*            sender state: add a new MIDI event                */
/****************************************************************/

void nsys_netout_journal_addstate(unsigned char cmd, unsigned char ndata,
				  unsigned char vdata)

{
  nsys_netout_jsend_system_state * jsys;
  nsys_netout_jsend_state * jsend;
  unsigned char chan;

  nsys_netout_jsend_guard_send = 0;

  if (nsys_netout_jsend_checkpoint_changed)
    nsys_netout_jsend_checkpoint_changed = 0;

  if (cmd == CSYS_MIDI_NOOP)
    {
      nsys_netout_jsend_header[NSYS_SM_JH_LOC_FLAGS] |= NSYS_SM_SETS;
      return;
    }

  if ((cmd & 0xF0) < CSYS_MIDI_NOTEOFF)    /* System Chapter X */
    {
      nsys_netout_journal_addsysex(cmd, ndata, vdata);
      return;
    }
  
  if ((cmd & 0xF0) < CSYS_MIDI_SYSTEM)
    {
      chan = 0x0F & cmd;
      cmd &= 0xF0;

      jsend = &(nsys_netout_jsend[chan]);

      if (jsend->cheader_seqnum == 0)
	{  
	  nsys_netout_jsend_header[NSYS_SM_JH_LOC_FLAGS] = NSYS_SM_JH_SETA |
	    (nsys_netout_jsend_system.sheader_seqnum ? NSYS_SM_JH_SETY : 0) |
	    ((unsigned char) nsys_netout_jsend_channel_size);
	  
	  nsys_netout_jsend_channel[nsys_netout_jsend_channel_size++] = chan;
	  
	  jsend->clen = ((((unsigned short)chan) << (NSYS_SM_CH_CHANSHIFT + 8)) |
			 NSYS_SM_CH_SIZE);
	}

      jsend->cheader_seqnum = nsys_netout_seqnum;
      nsys_netout_jsend_slist[nsys_netout_jsend_slist_size++] = 
	&(jsend->cheader[NSYS_SM_CH_LOC_FLAGS]);

      nsys_netout_jsend_header[NSYS_SM_JH_LOC_FLAGS] &= NSYS_SM_CLRS;

      switch (cmd) {
      case CSYS_MIDI_PROGRAM:
	nsys_netout_journal_addprogram(jsend, ndata);
	return;
      case CSYS_MIDI_CC:
	nsys_netout_journal_addcontrol(jsend, ndata, vdata);
	return;
      case CSYS_MIDI_WHEEL:
	nsys_netout_journal_addpwheel(jsend, ndata, vdata);
	return;
      case CSYS_MIDI_NOTEON:
	if (vdata)
	  {
	    nsys_netout_journal_addnoteon(jsend, ndata, vdata);
	    if (NSYS_NETOUT_CHAPTERE_STATUS == NSYS_NETOUT_CHAPTERE_ON)
	      nsys_netout_journal_addnoteon_extras(jsend, ndata);
	    return;
	  }
	else
	  vdata = NSYS_SM_CE_DEFREL;  /* fall through */
      case CSYS_MIDI_NOTEOFF:
	nsys_netout_journal_addnoteoff(jsend, ndata, vdata);
	if (NSYS_NETOUT_CHAPTERE_STATUS == NSYS_NETOUT_CHAPTERE_ON)
	  nsys_netout_journal_addnoteoff_extras(jsend, ndata, vdata);
	return;
      case CSYS_MIDI_CTOUCH:
	nsys_netout_journal_addctouch(jsend, ndata);
	return;
      case CSYS_MIDI_PTOUCH:
	nsys_netout_journal_addptouch(jsend, ndata, vdata);
	return;
      }
    }
  else   /* System command, excepting Chapter X */
    {
      jsys = &(nsys_netout_jsend_system);

      /* leave early for commands that do not update the journal */

      switch (cmd) {
      case CSYS_MIDI_SYSTEM_STOP:
      case CSYS_MIDI_SYSTEM_CLOCK:
	if (!(jsys->chapterq[NSYS_SM_CQ_LOC_HDR] & NSYS_SM_CQ_HDR_CHKN))
	  return;
	break;
      case CSYS_MIDI_SYSTEM_CONTINUE:
	if ((jsys->chapterq[NSYS_SM_CQ_LOC_HDR] & NSYS_SM_CQ_HDR_CHKN))
	  return;
	break;
      }

      nsys_netout_jsend_header[NSYS_SM_JH_LOC_FLAGS] &= NSYS_SM_CLRS;

      if (jsys->sheader_seqnum == 0)
	{
	  if (nsys_netout_jsend_channel_size)
	    nsys_netout_jsend_header[NSYS_SM_JH_LOC_FLAGS] |= NSYS_SM_JH_SETY;
	  else
	    nsys_netout_jsend_header[NSYS_SM_JH_LOC_FLAGS] = NSYS_SM_JH_SETY;

	  jsys->slen = NSYS_SM_SH_SIZE;
	}

      jsys->sheader_seqnum = nsys_netout_seqnum;
      nsys_netout_jsend_slist[nsys_netout_jsend_slist_size++] = 
	&(jsys->sheader[NSYS_SM_SH_LOC_FLAGS]);

      switch (cmd) {
      case CSYS_MIDI_SYSTEM_RESET:
	nsys_netout_journal_addreset();
	return;
      case CSYS_MIDI_SYSTEM_TUNE_REQUEST:
	nsys_netout_journal_addtune();
	return;
      case CSYS_MIDI_SYSTEM_SONG_SELECT:
	nsys_netout_journal_addsong(ndata);
	return;
      case CSYS_MIDI_SYSTEM_UNUSED1:
      case CSYS_MIDI_SYSTEM_UNUSED2:
	nsys_netout_journal_addsc(cmd, ndata, vdata);
	return;
      case CSYS_MIDI_SYSTEM_TICK:
	nsys_netout_journal_addrt(cmd);
	return;
      case CSYS_MIDI_SYSTEM_UNUSED3:
	nsys_netout_journal_addrt(cmd);
	return;
      case CSYS_MIDI_SYSTEM_QFRAME:
	nsys_netout_journal_addtimecode(ndata);
	return;
      case CSYS_MIDI_SYSTEM_CLOCK:
      case CSYS_MIDI_SYSTEM_START:
      case CSYS_MIDI_SYSTEM_CONTINUE: 
      case CSYS_MIDI_SYSTEM_STOP:
      case CSYS_MIDI_SYSTEM_SONG_PP:
	nsys_netout_journal_addsequence(cmd, ndata, vdata);
	return;
      case CSYS_MIDI_SYSTEM_SENSE:
	nsys_netout_journal_addsense();
	return;
      }
    }
}

/****************************************************************/
/*            sender state: adds history information            */
/****************************************************************/

void nsys_netout_journal_addhistory(unsigned char cmd, unsigned char ndata,
				    unsigned char vdata)

{
  unsigned char chan = 0x0F & cmd;
  nsys_netout_jsend_state * jsend;

  cmd &= 0xF0;

  if ((cmd != CSYS_MIDI_PROGRAM) && (cmd != CSYS_MIDI_CC))
    return;
  
  jsend = &(nsys_netout_jsend[chan]);

  if (jsend->history_active == 0)
    jsend->history_active = 1;

  switch (cmd & 0xF0) {
  case CSYS_MIDI_PROGRAM:
    jsend->history_program = ndata | NSYS_SM_SETH;
    jsend->history_program_bank_msb = jsend->history_cc_bank_msb;
    jsend->history_program_bank_lsb = jsend->history_cc_bank_lsb;
    break;
  case CSYS_MIDI_CC:  
    switch (ndata) {
    case CSYS_MIDI_CC_BANKSELECT_MSB:
      jsend->history_cc_bank_msb = vdata | NSYS_SM_CP_SETB;
      jsend->history_cc_bank_lsb = 0;
      break;
    case CSYS_MIDI_CC_BANKSELECT_LSB:
      if (jsend->history_cc_bank_msb & NSYS_SM_CP_CHKB)
	jsend->history_cc_bank_lsb = vdata;
      break;
    case CSYS_MIDI_CC_MODWHEEL_MSB:
      jsend->history_cc_modwheel = vdata | NSYS_SM_SETH;
      break;
    case CSYS_MIDI_CC_CHANVOLUME_MSB:
      jsend->history_cc_volume = vdata | NSYS_SM_SETH;
      break;
    case CSYS_MIDI_CC_RESETALLCONTROL:
      jsend->history_cc_bank_lsb |= NSYS_SM_CP_SETX;
      break;
    }
    break;
  }
}

/****************************************************************/
/*    sender state:  trim journal, update checkpoint packet     */
/****************************************************************/

void nsys_netin_journal_trimstate(nsys_source * lptr)

{ 
  unsigned long minseq;
  nsys_source * minsource;
  nsys_source * sptr;
  int channel;
  nsys_netout_jsend_state * jsend;
  nsys_netout_jsend_system_state * jsys;

  /********************************************/
  /* localize extension to new last_hiseq_ext */
  /********************************************/

  if (lptr)
    {
      lptr->last_hiseq_ext &= NSYS_RTPSEQ_LOMASK;
      lptr->last_hiseq_ext |= (nsys_netout_seqnum & NSYS_RTPSEQ_EXMASK);
      if (lptr->last_hiseq_ext > nsys_netout_seqnum)
	lptr->last_hiseq_ext -= NSYS_RTPSEQ_EXINCR;
    }

  /********************************************/
  /* find source with smallest last_hiseq_ext */
  /********************************************/

  if (nsys_srcroot == NULL)
    return;

  minsource = NULL;
  minseq = NSYS_RTPSEQ_HIGHEST;
  sptr = nsys_srcroot;

  /* later handle 32-bit wraparound correctly */

  do {
    if (minseq > sptr->last_hiseq_ext)
      {
	minsource = sptr;
	minseq = sptr->last_hiseq_ext;
      }
  } while ((sptr = sptr->next) != nsys_srcroot);

  /***************************************/
  /* return if checkpoint is still valid */
  /***************************************/

  if ((minseq == NSYS_RTPSEQ_HIGHEST) || 
      (minseq == nsys_netout_jsend_checkpoint_seqnum))
    return;

  /**********************/
  /* update checkpoint  */
  /**********************/

  nsys_netout_jsend_checkpoint_seqnum = minseq;
  nsys_netout_journal_changecheck();

  if ((nsys_netout_jsend_channel_size == 0) &&
      (nsys_netout_jsend_system.sheader_seqnum == 0))
    return;

  /***********************/
  /* trim System journal */
  /***********************/

  if (nsys_netout_jsend_system.sheader_seqnum)
    {
      jsys = &(nsys_netout_jsend_system);

      if (jsys->sheader_seqnum <= minseq)
	{
	  nsys_netout_journal_trimsystem();
	  nsys_netout_jsend_header[NSYS_SM_JH_LOC_FLAGS] &= NSYS_SM_JH_CLRY;
	}
      else
	{
	  if (jsys->chapterd_seqnum && (jsys->chapterd_seqnum <= minseq))
	    nsys_netout_journal_trimsimple();

	  if (jsys->chapterd_seqnum && jsys->chapterd_reset_seqnum && 
	      (jsys->chapterd_reset_seqnum <= minseq))
	    nsys_netout_journal_trimreset();

	  if (jsys->chapterd_seqnum && jsys->chapterd_tune_seqnum && 
	      (jsys->chapterd_tune_seqnum <= minseq))
	    nsys_netout_journal_trimtune();

	  if (jsys->chapterd_seqnum && jsys->chapterd_song_seqnum && 
	      (jsys->chapterd_song_seqnum <= minseq))
	    nsys_netout_journal_trimsong();

	  if (jsys->chapterd_seqnum && jsys->chapterd_scj_seqnum && 
	      (jsys->chapterd_scj_seqnum <= minseq))
	    nsys_netout_journal_trimscj();

	  if (jsys->chapterd_seqnum && jsys->chapterd_sck_seqnum && 
	      (jsys->chapterd_sck_seqnum <= minseq))
	    nsys_netout_journal_trimsck();

	  if (jsys->chapterd_seqnum && jsys->chapterd_rty_seqnum && 
	      (jsys->chapterd_rty_seqnum <= minseq))
	    nsys_netout_journal_trimrty();

	  if (jsys->chapterd_seqnum && jsys->chapterd_rtz_seqnum && 
	      (jsys->chapterd_rtz_seqnum <= minseq))
	    nsys_netout_journal_trimrtz();

	  if (jsys->chapterv_seqnum && (jsys->chapterv_seqnum <= minseq))
	    nsys_netout_journal_trimsense();

	  if (jsys->chapterq_seqnum && (jsys->chapterq_seqnum <= minseq))
	    nsys_netout_journal_trimsequence();

	  if (jsys->chapterf_seqnum && (jsys->chapterf_seqnum <= minseq))
	    nsys_netout_journal_trimtimecode();

	  if (jsys->chapterf_seqnum && jsys->chapterfc_seqnum && 
	      (jsys->chapterfc_seqnum <= minseq))
	    nsys_netout_journal_trimparttimecode();

	  if (jsys->chapterx_seqnum && (jsys->chapterx_seqnum <= minseq))
	    nsys_netout_journal_trimsysex();

	  if (jsys->chapterx_seqnum)
	    nsys_netout_journal_trimpartsysex(minseq);

	  /* other chapters go here */

	  jsys->sheader[NSYS_SM_SH_LOC_FLAGS] &= ~(NSYS_SM_SH_MSBMASK);

	  if (jsys->slen <= NSYS_SM_SH_LSBMASK)
	    jsys->sheader[NSYS_SM_SH_LOC_LENLSB] = (unsigned char)(jsys->slen);
	  else
	    {
	      jsys->sheader[NSYS_SM_SH_LOC_FLAGS] |= ((unsigned char)(jsys->slen >> 8));
	      jsys->sheader[NSYS_SM_SH_LOC_LENLSB] = 
		(unsigned char)(NSYS_SM_SH_LSBMASK & jsys->slen);  
	    }
	}
    }

  /*************************/
  /* trim channel journals */
  /*************************/

  if (nsys_netout_jsend_channel_size)
    {
      for (channel = nsys_netout_jsend_channel_size - 1; channel >= 0; channel--)
	{
	  jsend = &(nsys_netout_jsend[nsys_netout_jsend_channel[channel]]);
	  
	  /*******************************/
	  /* drop entire channel, or ... */
	  /*******************************/
	  
	  if (jsend->cheader_seqnum <= minseq)
	    {
	      nsys_netout_journal_trimchapter(jsend, channel);
	      
	      if (nsys_netout_jsend_channel_size)
		continue;
	      else
		break;
	    }
	  
	  /*****************************************/
	  /* ... drop chapters from channel header */
	  /*****************************************/
	  
	  if (jsend->chapterp_seqnum && (jsend->chapterp_seqnum <= minseq))
	    nsys_netout_journal_trimprogram(jsend);
	  
	  if (jsend->chapterc_seqnum && (jsend->chapterc_seqnum <= minseq))
	    nsys_netout_journal_trimallcontrol(jsend); 
	  
	  if (jsend->chapterc_seqnum)
	    nsys_netout_journal_trimpartcontrol(jsend, minseq);
	  
	  if (jsend->chapterm_seqnum && (jsend->chapterm_seqnum <= minseq))
	    nsys_netout_journal_trimallparams(jsend); 
	
	  if (jsend->chapterm_seqnum)
	    nsys_netout_journal_trimpartparams(jsend, minseq);
	
	  if (jsend->chapterw_seqnum && (jsend->chapterw_seqnum <= minseq))
	    nsys_netout_journal_trimpwheel(jsend);
	
	  if (jsend->chaptern_seqnum && (jsend->chaptern_seqnum <= minseq))
	    nsys_netout_journal_trimallnote(jsend); 
	
	  if (jsend->chaptern_seqnum)
	    nsys_netout_journal_trimpartnote(jsend, minseq); 

	  if (jsend->chaptere_seqnum && (jsend->chaptere_seqnum <= minseq))
	    nsys_netout_journal_trimallextras(jsend); 
	
	  if (jsend->chaptere_seqnum)
	    nsys_netout_journal_trimpartextras(jsend, minseq);
	
	  if (jsend->chaptert_seqnum && (jsend->chaptert_seqnum <= minseq))
	    nsys_netout_journal_trimctouch(jsend);
	
	  if (jsend->chaptera_seqnum && (jsend->chaptera_seqnum <= minseq))
	    nsys_netout_journal_trimallptouch(jsend); 
	
	  if (jsend->chaptera_seqnum)
	    nsys_netout_journal_trimpartptouch(jsend, minseq);
	
	  *((unsigned short *) &(jsend->cheader[NSYS_SM_CH_LOC_LEN])) = 
	    htons(jsend->clen);
	}

      if (nsys_netout_jsend_channel_size == 0)
	nsys_netout_jsend_header[NSYS_SM_JH_LOC_FLAGS] &= NSYS_SM_JH_CLRA;
      else
	{
	  nsys_netout_jsend_header[NSYS_SM_JH_LOC_FLAGS] &= (~NSYS_SM_JH_CHANMASK);
	  nsys_netout_jsend_header[NSYS_SM_JH_LOC_FLAGS] |= 
	    NSYS_SM_JH_SETA | ((unsigned char)(nsys_netout_jsend_channel_size - 1));
	}
    }

  /****************/
  /* housekeeping */
  /****************/

  if (!nsys_netout_jsend_system.sheader_seqnum && !nsys_netout_jsend_channel_size)
    {
      nsys_netout_jsend_slist_size =  0;

      /* uncomment to disable keep-alive empty-journal RTP packets:        */ 
      /* nsys_netout_jsend_guard_time = nsys_netout_jsend_guard_send = 0;  */
    }

  if (!nsys_netout_jsend_channel_size && !nsys_netout_jsend_system.sheader_seqnum)
    nsys_netout_jsend_header[NSYS_SM_JH_LOC_FLAGS] |= NSYS_SM_SETS;
}


/****************************************************************/
/*               ticks down the guard packet timer              */
/****************************************************************/

void nsys_netout_guard_tick(void)

{
  /* enable conditional to disable keep-alive empty-journal RTP packets */

  if (1 || nsys_netout_jsend_channel_size)
    {
      nsys_netout_jsend_guard_send = 1;
      nsys_netout_jsend_guard_time = nsys_netout_jsend_guard_next;
      if ((nsys_netout_jsend_guard_next <<= 1) > nsys_netout_jsend_guard_maxtime)
	nsys_netout_jsend_guard_next = nsys_netout_jsend_guard_maxtime;
    }
}


/****************************************************************/
/*         add initialization state to recovery journal         */
/****************************************************************/

void nsys_netout_midistate_init(void)

{
  int slist_size = nsys_netout_jsend_slist_size;
  unsigned long seqsafe;
  nsys_netout_jsend_state * jsend;
  unsigned char i, cmd, ndata, vdata, bank_msb, bank_lsb;


  /* seqnum updated so an rtcp packet doesn't delete inits */

  seqsafe = nsys_netout_seqnum;      
  nsys_netout_seqnum = ((nsys_netout_seqnum != NSYS_RTPSEQ_HIGHEST) ?
			(nsys_netout_seqnum + 1) : 1); 


  /* load recovery journal with initial values */

  for (i = 0; i < CSYS_MIDI_NUMCHAN; i++)
    {
      jsend = &(nsys_netout_jsend[i]);

      if (jsend->history_active == 0)
	continue;

      /***********************/
      /* MIDI Program Change */ 
      /***********************/
      
      bank_msb = jsend->history_cc_bank_msb;   /* save before overwrite happens */
      bank_lsb = jsend->history_cc_bank_lsb;

      if (jsend->history_program)
	{
	  if (jsend->history_program_bank_msb)
	    {
	      cmd = CSYS_MIDI_CC | i;
	      ndata = CSYS_MIDI_CC_BANKSELECT_MSB;
	      vdata = NSYS_SM_CP_CLRB & jsend->history_program_bank_msb;
	      nsys_netout_journal_addstate(cmd, ndata, vdata);

	      ndata = CSYS_MIDI_CC_BANKSELECT_LSB;
	      vdata = NSYS_SM_CP_CLRX & jsend->history_program_bank_lsb;
	      nsys_netout_journal_addstate(cmd, ndata, vdata);
	    }

	  cmd = CSYS_MIDI_PROGRAM | i;
	  ndata = NSYS_SM_CLRH & jsend->history_program;
	  vdata = 0;
	  nsys_netout_journal_addstate(cmd, ndata, vdata);
	}

      /************************/
      /* MIDI Control Changes */ 
      /************************/

      cmd = CSYS_MIDI_CC | i;

      if (bank_msb)
	{
	  ndata = CSYS_MIDI_CC_BANKSELECT_MSB;
	  vdata = NSYS_SM_CP_CLRB & bank_msb;
	  nsys_netout_journal_addstate(cmd, ndata, vdata);
	}

      if (bank_lsb)
	{
	  ndata = CSYS_MIDI_CC_BANKSELECT_LSB;
	  vdata = NSYS_SM_CP_CLRX & bank_lsb;
	  nsys_netout_journal_addstate(cmd, ndata, vdata);
	}

      if (jsend->history_cc_modwheel)
	{
	  ndata = CSYS_MIDI_CC_MODWHEEL_MSB;
	  vdata = NSYS_SM_CLRH & jsend->history_cc_modwheel;
	  nsys_netout_journal_addstate(cmd, ndata, vdata);
	}

      if (jsend->history_cc_volume)
	{
	  ndata = CSYS_MIDI_CC_CHANVOLUME_MSB;
	  vdata = NSYS_SM_CLRH & jsend->history_cc_volume;
	  nsys_netout_journal_addstate(cmd, ndata, vdata);
	}

      while (nsys_netout_jsend_slist_size > slist_size)
	*(nsys_netout_jsend_slist[--nsys_netout_jsend_slist_size]) |= NSYS_SM_SETS;

    }

  if (slist_size == 0)
    nsys_netout_jsend_header[NSYS_SM_JH_LOC_FLAGS] |= NSYS_SM_SETS;
  
  while (slist_size > 0)
    *(nsys_netout_jsend_slist[--slist_size]) &= NSYS_SM_CLRS;

  nsys_netout_seqnum = seqsafe;   /* restore nsys_netout_seqnum */

  nsys_netout_jsend_guard_send = 1;    
  nsys_netout_jsend_guard_time = nsys_netout_jsend_guard_mintime;
  nsys_netout_jsend_guard_next = (nsys_netout_jsend_guard_mintime << 
			     (nsys_feclevel == NSYS_SM_FEC_MINIMAL));
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*      second-level journal functions: addstate chapters       */
/*______________________________________________________________*/

/****************************************************************/
/*               add MIDI Program Change chapter                */
/****************************************************************/

void nsys_netout_journal_addprogram(nsys_netout_jsend_state * jsend, 
				    unsigned char ndata)

{
  jsend->chapterp[NSYS_SM_CP_LOC_PROGRAM] = ndata;
  jsend->chapterp[NSYS_SM_CP_LOC_BANKMSB] = jsend->history_cc_bank_msb;
  jsend->chapterp[NSYS_SM_CP_LOC_BANKLSB] = jsend->history_cc_bank_lsb;
    
  nsys_netout_journal_addhistory(CSYS_MIDI_PROGRAM | jsend->chan, ndata, 0);

  nsys_netout_jsend_slist[nsys_netout_jsend_slist_size++] = 
    &(jsend->chapterp[NSYS_SM_CP_LOC_PROGRAM]);
  
  if (jsend->chapterp_seqnum)
    jsend->cheader[NSYS_SM_CH_LOC_FLAGS] &= NSYS_SM_CLRS;
  else
    {
      jsend->cheader[NSYS_SM_CH_LOC_TOC] |= NSYS_SM_CH_TOC_SETP;
      
      *((unsigned short *) &(jsend->cheader[NSYS_SM_CH_LOC_LEN])) = 
	htons((jsend->clen += NSYS_SM_CP_SIZE));
    }
  jsend->chapterp_seqnum = nsys_netout_seqnum;

  nsys_netout_jsend_guard_time = nsys_netout_jsend_guard_mintime;
  nsys_netout_jsend_guard_next = (nsys_netout_jsend_guard_mintime << 
			     (nsys_feclevel == NSYS_SM_FEC_MINIMAL));

}


/****************************************************************/
/*                  add MIDI Controller chapter                */
/****************************************************************/

void nsys_netout_journal_addcontrol(nsys_netout_jsend_state * jsend, 
				    unsigned char ndata,
				    unsigned char vdata)

{
  int i;

  /***************************/
  /* Chapter M state machine */
  /***************************/

  switch(ndata) {
  case CSYS_MIDI_CC_RPN_MSB:
    jsend->chapterm_state = NSYS_SM_CM_STATE_PENDING_RPN;
    jsend->chapterm_rpn_msb = vdata;
    jsend->chapterm_rpn_lsb = 0;
    nsys_netout_journal_addparameter(jsend, ndata, vdata);
    return;
  case CSYS_MIDI_CC_NRPN_MSB:
    jsend->chapterm_state = NSYS_SM_CM_STATE_PENDING_NRPN;
    jsend->chapterm_nrpn_msb = vdata;
    jsend->chapterm_nrpn_lsb = 0;
    nsys_netout_journal_addparameter(jsend, ndata, vdata);
    return;
  case CSYS_MIDI_CC_RPN_LSB:
    jsend->chapterm_state = NSYS_SM_CM_STATE_RPN;
    jsend->chapterm_rpn_lsb = vdata;
    nsys_netout_journal_addparameter(jsend, ndata, vdata);
    return;
  case CSYS_MIDI_CC_NRPN_LSB:
    jsend->chapterm_state = NSYS_SM_CM_STATE_NRPN;
    jsend->chapterm_nrpn_lsb = vdata;
    nsys_netout_journal_addparameter(jsend, ndata, vdata);
    return;
  case CSYS_MIDI_CC_DATAENTRY_MSB:
  case CSYS_MIDI_CC_DATAENTRY_LSB:
  case CSYS_MIDI_CC_DATAENTRYPLUS:
  case CSYS_MIDI_CC_DATAENTRYMINUS:
    switch(jsend->chapterm_state) {
    case NSYS_SM_CM_STATE_OFF:   
      break;                            /* code in Chapter C */
    case NSYS_SM_CM_STATE_PENDING_RPN:
      jsend->chapterm_state = NSYS_SM_CM_STATE_RPN;
      nsys_netout_journal_addparameter(jsend, ndata, vdata);
      return;
    case NSYS_SM_CM_STATE_PENDING_NRPN: 
      jsend->chapterm_state = NSYS_SM_CM_STATE_NRPN;
      nsys_netout_journal_addparameter(jsend, ndata, vdata);
      return;
    case NSYS_SM_CM_STATE_RPN:
    case NSYS_SM_CM_STATE_NRPN:
      nsys_netout_journal_addparameter(jsend, ndata, vdata);
      return;
    }
    break;
  default:
    break;
  }

  /*****************************/
  /* initialize chapter header */
  /*****************************/
  
  if (jsend->chapterc_seqnum)
    jsend->chapterc[NSYS_SM_CC_LOC_LENGTH] &= NSYS_SM_CLRS;
  else
    {
      jsend->clen += NSYS_SM_CC_HDRSIZE;
      jsend->chapterc_size = NSYS_SM_CC_HDRSIZE;
      jsend->chapterc[NSYS_SM_CC_LOC_LENGTH] = 0;
      jsend->cheader[NSYS_SM_CH_LOC_TOC] |= NSYS_SM_CH_TOC_SETC;
    }
    
  /**************************/
  /* update ancillary state */
  /**************************/

  nsys_netout_journal_addhistory(CSYS_MIDI_CC | jsend->chan, ndata, vdata);

  switch (ndata) {
  case CSYS_MIDI_CC_SUSTAIN:
    if (((vdata >= 64) && !(jsend->chapterc_sustain & 0x01)) || 
	((vdata < 64) && (jsend->chapterc_sustain & 0x01)))
      jsend->chapterc_sustain = ((jsend->chapterc_sustain + 1)  
				  & NSYS_SM_CC_ALTMOD);

    vdata = NSYS_SM_CC_SETA | NSYS_SM_CC_SETT | jsend->chapterc_sustain;
    break;
  case CSYS_MIDI_CC_ALLSOUNDOFF:
    jsend->chapterc_allsound = ((jsend->chapterc_allsound + 1)  
				& NSYS_SM_CC_ALTMOD);
    vdata = NSYS_SM_CC_SETA | jsend->chapterc_allsound;
    nsys_netout_journal_clear_nactive(jsend);
    break;
  case CSYS_MIDI_CC_RESETALLCONTROL:
    jsend->chapterc_rac = ((jsend->chapterc_rac + 1) & NSYS_SM_CC_ALTMOD);
    vdata = NSYS_SM_CC_SETA | jsend->chapterc_rac;
    nsys_netout_journal_clear_cactive(jsend);
    break;
  case CSYS_MIDI_CC_ALLNOTESOFF:
    jsend->chapterc_allnotes = ((jsend->chapterc_allnotes + 1)  
				& NSYS_SM_CC_ALTMOD);
    vdata = NSYS_SM_CC_SETA | jsend->chapterc_allnotes;
    nsys_netout_journal_clear_nactive(jsend);
    break;
  case CSYS_MIDI_CC_OMNI_OFF:
    jsend->chapterc_omni_off = ((jsend->chapterc_omni_off + 1)  
				& NSYS_SM_CC_ALTMOD);
    vdata = NSYS_SM_CC_SETA | jsend->chapterc_omni_off;
    nsys_netout_journal_clear_nactive(jsend);
    break;
  case CSYS_MIDI_CC_OMNI_ON:
    jsend->chapterc_omni_on = ((jsend->chapterc_omni_on + 1)  
			       & NSYS_SM_CC_ALTMOD);
    vdata = NSYS_SM_CC_SETA | jsend->chapterc_omni_on;
    nsys_netout_journal_clear_nactive(jsend);
    break;
  case CSYS_MIDI_CC_MONOMODE:
    jsend->chapterc_monomode = ((jsend->chapterc_monomode + 1)  
				& NSYS_SM_CC_ALTMOD);
    vdata = NSYS_SM_CC_SETA | jsend->chapterc_monomode;
    nsys_netout_journal_clear_nactive(jsend);
    break;
  case CSYS_MIDI_CC_POLYMODE:
    jsend->chapterc_polymode = ((jsend->chapterc_polymode + 1)  
				& NSYS_SM_CC_ALTMOD);
    vdata = NSYS_SM_CC_SETA | jsend->chapterc_polymode;
    nsys_netout_journal_clear_nactive(jsend);
    break;
  }
  
  /******************************************/
  /* prepare for two types of log additions */
  /******************************************/

  if (jsend->chapterc_seqarray[ndata])
    {
      /*****************************/
      /* "update log" preparations */
      /*****************************/

      i = jsend->chapterc_logptr[ndata];

      if (i < jsend->chapterc_size - NSYS_SM_CC_LOGSIZE)
	{
	  memmove(&(jsend->chapterc[i]),
		  &(jsend->chapterc[i + NSYS_SM_CC_LOGSIZE]),
		  jsend->chapterc_size - i - NSYS_SM_CC_LOGSIZE);

	  while (i < (jsend->chapterc_size - NSYS_SM_CC_LOGSIZE))
	    {
	      jsend->chapterc_logptr[jsend->chapterc[i + NSYS_SM_CC_LOC_LNUM]
				    & NSYS_SM_CLRS] = i;
	      i += NSYS_SM_CC_LOGSIZE;
	    }
	}
    }
  else
    {
      /**************************/
      /* "new log" preparations */
      /**************************/

      jsend->chapterc_size += NSYS_SM_CC_LOGSIZE;

      if (jsend->chapterc_seqnum)
	jsend->chapterc[NSYS_SM_CC_LOC_LENGTH]++;

      jsend->clen += NSYS_SM_CC_LOGSIZE;
    }

  /*******************/
  /* do log addition */
  /*******************/

  i = (jsend->chapterc_size - NSYS_SM_CC_LOGSIZE);
  jsend->chapterc[i + NSYS_SM_CC_LOC_LNUM] = ndata;
  jsend->chapterc[i + NSYS_SM_CC_LOC_LVAL] = vdata;
  jsend->chapterc_logptr[ndata] = i;

  /***********************************************/
  /* update slists, seqnums, cheader, guard time */
  /***********************************************/
  
  nsys_netout_jsend_slist[nsys_netout_jsend_slist_size++] = 
    &(jsend->chapterc[NSYS_SM_CC_LOC_LENGTH]);

  jsend->chapterc_sset = 1;
  
  jsend->chapterc_seqarray[ndata] = nsys_netout_seqnum;
  jsend->chapterc_seqnum = nsys_netout_seqnum;

  /* writing clen also clears S bit, which is always needed */

  *((unsigned short *) &(jsend->cheader[NSYS_SM_CH_LOC_LEN])) = 
    htons(jsend->clen);  

  nsys_netout_jsend_guard_time = nsys_netout_jsend_guard_mintime;
  nsys_netout_jsend_guard_next = (nsys_netout_jsend_guard_mintime << 
			     (nsys_feclevel == NSYS_SM_FEC_MINIMAL));
}


/****************************************************************/
/*                   add Parameter chapter                      */
/****************************************************************/

void nsys_netout_journal_addparameter(nsys_netout_jsend_state * jsend, 
				      unsigned char ndata,
				      unsigned char vdata)

{
  unsigned char * logplace;
  unsigned char toc;
  unsigned short bholder;
  int loglen, logincr, i, j, nullparam, dummy;
 
  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
  /* at present, all non-null NRPNs, and all non-null RPNs
     except for MSB=00, LSB==00-02, are coded as dummies.

     dummy coding preserves transactional semantics of the
     RPN/NRPN system, but state information to recovery the
     value of dummy variables is not coded in Chapter M.
  */
  /*________________________________________________________*/


  /*****************************/
  /* initialize chapter header */
  /*****************************/
  
  if (jsend->chapterm_seqnum == 0)
    {
      jsend->clen += NSYS_SM_CM_HDRSIZE;
      jsend->chapterm_size = NSYS_SM_CM_HDRSIZE;
      jsend->chapterl_size = 0;
      jsend->cheader[NSYS_SM_CH_LOC_TOC] |= NSYS_SM_CH_TOC_SETM;
    }

  /************************************/
  /* check for null, dummy parameters */
  /************************************/

  nullparam = (((ndata == CSYS_MIDI_CC_RPN_LSB) && 
		(jsend->chapterm_rpn_msb == CSYS_MIDI_RPN_NULL_MSB) && 
		(jsend->chapterm_rpn_lsb == CSYS_MIDI_RPN_NULL_LSB)) ||
	       ((ndata == CSYS_MIDI_CC_NRPN_LSB) && 
		(jsend->chapterm_nrpn_msb == CSYS_MIDI_NRPN_NULL_MSB) && 
		(jsend->chapterm_nrpn_lsb == CSYS_MIDI_NRPN_NULL_LSB)));

  if (nullparam)
    dummy = 0;
  else
    dummy = ((jsend->chapterm_state == NSYS_SM_CM_STATE_NRPN) ||
	     (jsend->chapterm_rpn_lsb >= NSYS_SM_CM_ARRAYSIZE) ||  
	     jsend->chapterm_rpn_msb);

  /************************************/
  /* remove current dummy if unneeded */
  /************************************/

  if (jsend->chapterm_dummy_seqnum && !dummy)
    {
      jsend->chapterm_dummy_seqnum = 0;
      jsend->chapterl_size -= NSYS_SM_CM_LOGHDRSIZE;
      jsend->clen -= NSYS_SM_CM_LOGHDRSIZE;
    }

  /***********************/
  /* update button state */
  /***********************/

  switch (ndata) {
  case CSYS_MIDI_CC_DATAENTRY_MSB:
  case CSYS_MIDI_CC_DATAENTRY_LSB:
    jsend->chapterm_cbutton[jsend->chapterm_rpn_lsb] = 0;
    break;
  case CSYS_MIDI_CC_DATAENTRYPLUS:
    jsend->chapterm_cbutton[jsend->chapterm_rpn_lsb] += 2;  /* fallthru */
  case CSYS_MIDI_CC_DATAENTRYMINUS:
    jsend->chapterm_cbutton[jsend->chapterm_rpn_lsb] -= 1;

    if (jsend->chapterm_cbutton[jsend->chapterm_rpn_lsb] > NSYS_SM_CM_BUTTON_LIMIT)
      jsend->chapterm_cbutton[jsend->chapterm_rpn_lsb] = NSYS_SM_CM_BUTTON_LIMIT;
    else
      if (jsend->chapterm_cbutton[jsend->chapterm_rpn_lsb] < - NSYS_SM_CM_BUTTON_LIMIT)
	jsend->chapterm_cbutton[jsend->chapterm_rpn_lsb] = - NSYS_SM_CM_BUTTON_LIMIT;
  }

  /*****************/
  /* do log update */
  /*****************/

  if ((jsend->chapterm_state == NSYS_SM_CM_STATE_PENDING_RPN) || 
      (jsend->chapterm_state == NSYS_SM_CM_STATE_PENDING_NRPN))
    {
      /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
      /* add PENDING field to header */
      /*_____________________________*/

      if (jsend->chapterm_size == NSYS_SM_CM_HDRSIZE)
	{
	  jsend->chapterm_size += NSYS_SM_CM_PENDINGSIZE;
	  jsend->clen += NSYS_SM_CM_PENDINGSIZE;
	}
      if (jsend->chapterm_state == NSYS_SM_CM_STATE_PENDING_RPN)
	jsend->chapterm[NSYS_SM_CM_LOC_PENDING] = vdata;
      else
	jsend->chapterm[NSYS_SM_CM_LOC_PENDING] = vdata | NSYS_SM_CM_PENDING_SETQ;
    }
  else
    {
      /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
      /* remove PENDING field from header */
      /*__________________________________*/

      if (jsend->chapterm_seqnum && 
	  (jsend->chapterm[NSYS_SM_CM_LOC_HDR] & NSYS_SM_CM_HDR_CHKP))
	{
	  jsend->chapterm_size -= NSYS_SM_CM_PENDINGSIZE;
	  jsend->clen -= NSYS_SM_CM_PENDINGSIZE;
	}

      if (!nullparam && !dummy && 
	  (jsend->chapterm_seqarray[jsend->chapterm_rpn_lsb]))
	{
	  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	  /* move, resize, and update an existing log */
	  /*__________________________________________*/
	  
	  /******************************/
	  /* compute current log length */
	  /******************************/
	  
	  i = jsend->chapterm_logptr[jsend->chapterm_rpn_lsb];
	  toc = jsend->chapterl[i + NSYS_SM_CM_LOC_TOC];
	  
	  loglen = NSYS_SM_CM_LOGHDRSIZE;
	  
	  if (toc & NSYS_SM_CM_TOC_CHKJ)
	    loglen += NSYS_SM_CM_ENTRYMSB_SIZE;
	  
	  if (toc & NSYS_SM_CM_TOC_CHKK)
	    loglen += NSYS_SM_CM_ENTRYLSB_SIZE;
	  
	  if (toc & NSYS_SM_CM_TOC_CHKL)
	    loglen += NSYS_SM_CM_ABUTTON_SIZE;
	  
	  if (toc & NSYS_SM_CM_TOC_CHKM)
	    loglen += NSYS_SM_CM_CBUTTON_SIZE;

	  if (toc & NSYS_SM_CM_TOC_CHKN)
	    loglen += NSYS_SM_CM_COUNT_SIZE;
	  
	  /*************************/
	  /* move log if necessary */
	  /*************************/
	  
	  if (i + loglen != jsend->chapterl_size)
	    {
	      memcpy(&(jsend->chapterl[jsend->chapterl_size]),
		     &(jsend->chapterl[i]), loglen);
	      memmove(&(jsend->chapterl[i]), &(jsend->chapterl[i + loglen]),
		     jsend->chapterl_size - i);

	      for (j = 0; j < NSYS_SM_CM_ARRAYSIZE; j++)
		if (jsend->chapterm_seqarray[j] && (jsend->chapterm_logptr[j] > i)
		    && (j != jsend->chapterm_rpn_lsb))
		  jsend->chapterm_logptr[j] -= loglen;

	      i = jsend->chapterm_logptr[jsend->chapterm_rpn_lsb] = 
		jsend->chapterl_size - loglen;
	    }

	  /*****************************************************/
	  /* update log contents, compute log resize increment */
	  /*****************************************************/
	  
	  logplace = &(jsend->chapterl[i]);
	  logplace[NSYS_SM_CM_LOC_PNUMLSB] &= NSYS_SM_CLRS;
	  logincr = 0;
	  
	  switch (ndata) {
	  case CSYS_MIDI_CC_DATAENTRY_MSB:
	    if (!(logplace[NSYS_SM_CM_LOC_TOC] & NSYS_SM_CM_TOC_CHKJ))
	      logincr += NSYS_SM_CM_ENTRYMSB_SIZE;
	    if (logplace[NSYS_SM_CM_LOC_TOC] & NSYS_SM_CM_TOC_CHKK)
	      logincr -= NSYS_SM_CM_ENTRYLSB_SIZE;
	    if (logplace[NSYS_SM_CM_LOC_TOC] & NSYS_SM_CM_TOC_CHKM)
	      logincr -= NSYS_SM_CM_CBUTTON_SIZE;

	    logplace[NSYS_SM_CM_LOC_TOC] = (NSYS_SM_CM_TOC_SETJ |
					     NSYS_SM_CM_INFO_SETV);
	    logplace[NSYS_SM_CM_LOGHDRSIZE] = vdata;
	    break;
	  case CSYS_MIDI_CC_DATAENTRY_LSB:
	    if (!(logplace[NSYS_SM_CM_LOC_TOC] & NSYS_SM_CM_TOC_CHKK))
	      logincr += NSYS_SM_CM_ENTRYLSB_SIZE;
	    if (logplace[NSYS_SM_CM_LOC_TOC] & NSYS_SM_CM_TOC_CHKM)
	      logincr -= NSYS_SM_CM_CBUTTON_SIZE;
	    
	    if (logplace[NSYS_SM_CM_LOC_TOC] & NSYS_SM_CM_TOC_CHKJ)
	      {
		logplace[NSYS_SM_CM_LOC_TOC] = (NSYS_SM_CM_TOC_SETJ |
						NSYS_SM_CM_TOC_SETK |
						NSYS_SM_CM_INFO_SETV);
		logplace[NSYS_SM_CM_LOGHDRSIZE + NSYS_SM_CM_ENTRYMSB_SIZE] = vdata;
	      }
	    else
	      {
		logplace[NSYS_SM_CM_LOC_TOC] = (NSYS_SM_CM_TOC_SETK |
						 NSYS_SM_CM_INFO_SETV);
		logplace[NSYS_SM_CM_LOGHDRSIZE] = vdata;
	      }
	    break;
	  case CSYS_MIDI_CC_DATAENTRYPLUS:
	  case CSYS_MIDI_CC_DATAENTRYMINUS:
	    if (!(logplace[NSYS_SM_CM_LOC_TOC] & NSYS_SM_CM_TOC_CHKM))
	      {
		logincr += NSYS_SM_CM_CBUTTON_SIZE;
		logplace[NSYS_SM_CM_LOC_TOC] |= NSYS_SM_CM_TOC_SETM;
	      }

	    j = NSYS_SM_CM_LOGHDRSIZE;
	    if (logplace[NSYS_SM_CM_LOC_TOC] & NSYS_SM_CM_TOC_CHKJ)
	      j += NSYS_SM_CM_ENTRYMSB_SIZE;
	    if (logplace[NSYS_SM_CM_LOC_TOC] & NSYS_SM_CM_TOC_CHKK)
	      j += NSYS_SM_CM_ENTRYLSB_SIZE;

	    bholder = abs(jsend->chapterm_cbutton[jsend->chapterm_rpn_lsb]);
	    bholder = htons((unsigned short) bholder);

	    memmove(&(logplace[j]), &bholder, sizeof(unsigned short));

	    if (jsend->chapterm_cbutton[jsend->chapterm_rpn_lsb] < 0)
	      logplace[j] |= NSYS_SM_CM_BUTTON_SETG;
	    break;
	  }

	  /****************/
	  /* update state */
	  /****************/
	  
	  jsend->chapterm_seqarray[jsend->chapterm_rpn_lsb] = nsys_netout_seqnum;
	  jsend->clen += logincr;
	  jsend->chapterl_size += logincr;
	}
      else
	{
	  /******************************/
	  /* make new log for parameter */
	  /******************************/
	  
	  /* determine log length */
	  
	  if (nullparam)
	    loglen = 0;   
	  else
	    {
	      if (dummy)
		loglen = NSYS_SM_CM_LOGHDRSIZE;
	      else
		switch (ndata) {
		case CSYS_MIDI_CC_DATAENTRY_MSB:
		  loglen = NSYS_SM_CM_LOGHDRSIZE + NSYS_SM_CM_ENTRYMSB_SIZE;
		  break;
		case CSYS_MIDI_CC_DATAENTRY_LSB:
		  loglen = NSYS_SM_CM_LOGHDRSIZE + NSYS_SM_CM_ENTRYLSB_SIZE;
		  break;
		case CSYS_MIDI_CC_DATAENTRYPLUS:
		case CSYS_MIDI_CC_DATAENTRYMINUS:
		  loglen = NSYS_SM_CM_LOGHDRSIZE + NSYS_SM_CM_CBUTTON_SIZE;
		  break;
		default:
		  loglen = NSYS_SM_CM_LOGHDRSIZE;
		  break;
		}
	    }

	  if (dummy && jsend->chapterm_dummy_seqnum)
	    i = jsend->chapterm_dummy_logptr;
	  else
	    i = jsend->chapterl_size;

	  /* fill in new log */
	  
	  if (loglen)
	    {
	      if (jsend->chapterm_state == NSYS_SM_CM_STATE_RPN)
		{
		  jsend->chapterl[i + NSYS_SM_CM_LOC_PNUMMSB] = 
		    jsend->chapterm_rpn_msb;
		  
		  jsend->chapterl[i + NSYS_SM_CM_LOC_PNUMLSB] = 
		    jsend->chapterm_rpn_lsb;
		}
	      else
		{
		  jsend->chapterl[i + NSYS_SM_CM_LOC_PNUMLSB] = 
		    jsend->chapterm_nrpn_lsb;
		  
		  jsend->chapterl[i + NSYS_SM_CM_LOC_PNUMMSB] = 
		    jsend->chapterm_nrpn_msb | NSYS_SM_CM_SETQ;
		}
	      
	      if (!dummy)
		{
		  switch (ndata) {
		  case CSYS_MIDI_CC_DATAENTRY_MSB:
		    jsend->chapterl[i + NSYS_SM_CM_LOC_TOC]
		      = NSYS_SM_CM_TOC_SETJ | NSYS_SM_CM_INFO_SETV;
		    jsend->chapterl[i + NSYS_SM_CM_LOGHDRSIZE] = vdata;
		    break;
		  case CSYS_MIDI_CC_DATAENTRY_LSB:
		    jsend->chapterl[i + NSYS_SM_CM_LOC_TOC]
		      = NSYS_SM_CM_TOC_SETK | NSYS_SM_CM_INFO_SETV;
		    jsend->chapterl[i + NSYS_SM_CM_LOGHDRSIZE] = vdata;
		    break;
		  case CSYS_MIDI_CC_DATAENTRYPLUS:
		  case CSYS_MIDI_CC_DATAENTRYMINUS:
		    jsend->chapterl[i + NSYS_SM_CM_LOC_TOC]
		      = NSYS_SM_CM_TOC_SETM | NSYS_SM_CM_INFO_SETV;

		    bholder = abs(jsend->chapterm_cbutton[jsend->chapterm_rpn_lsb]);
		    bholder = htons((unsigned short) bholder);

		    memmove(&(jsend->chapterl[i + NSYS_SM_CM_LOGHDRSIZE]), 
			    &bholder, sizeof(unsigned short));

		    if (jsend->chapterm_cbutton[jsend->chapterm_rpn_lsb]< 0)
		       jsend->chapterl[i + NSYS_SM_CM_LOGHDRSIZE] |= 
			 NSYS_SM_CM_BUTTON_SETG;
		  break;
		  default:
		    jsend->chapterl[i + NSYS_SM_CM_LOC_TOC] = NSYS_SM_CM_INFO_SETV;
		    break;  
		  }
		  jsend->chapterm_logptr[jsend->chapterm_rpn_lsb] = i;
		  jsend->chapterm_seqarray[jsend->chapterm_rpn_lsb]
		    = nsys_netout_seqnum;
		}
	      else
		{
		  jsend->chapterl[i + NSYS_SM_CM_LOC_TOC] = 0;
		  if (jsend->chapterm_dummy_seqnum)
		    loglen = 0;
		  jsend->chapterm_dummy_seqnum = nsys_netout_seqnum;
		  jsend->chapterm_dummy_logptr = i;
		}
	    }
	
	  jsend->clen += loglen;
	  jsend->chapterl_size += loglen;
	}
    }

  /**************************************************/
  /* update slists, seqnums, state, guard time, etc */
  /**************************************************/

  nsys_netout_jsend_slist[nsys_netout_jsend_slist_size++] = 
    &(jsend->chapterm[NSYS_SM_CM_LOC_HDR]);

  jsend->chapterm_sset = 1;
  jsend->chapterm_seqnum = nsys_netout_seqnum;

  *((unsigned short *) &(jsend->chapterm[NSYS_SM_CM_LOC_HDR])) = 
    htons((unsigned short)(jsend->chapterm_size + jsend->chapterl_size));  

  if (nullparam)
    jsend->chapterm_state = NSYS_SM_CM_STATE_OFF;

  if ((jsend->chapterm_state == NSYS_SM_CM_STATE_PENDING_RPN) || 
      (jsend->chapterm_state == NSYS_SM_CM_STATE_PENDING_NRPN))
    jsend->chapterm[NSYS_SM_CM_LOC_HDR] |= NSYS_SM_CM_HDR_SETP;
  else
    if (jsend->chapterm_state != NSYS_SM_CM_STATE_OFF)
      jsend->chapterm[NSYS_SM_CM_LOC_HDR] |= NSYS_SM_CM_HDR_SETE;

  /* writing clen also clears S bit, which is always needed */

  *((unsigned short *) &(jsend->cheader[NSYS_SM_CH_LOC_LEN])) = 
    htons(jsend->clen);

  nsys_netout_jsend_guard_time = nsys_netout_jsend_guard_mintime;
  nsys_netout_jsend_guard_next = (nsys_netout_jsend_guard_mintime << 
			     (nsys_feclevel == NSYS_SM_FEC_MINIMAL));
}


/****************************************************************/
/*               add MIDI Pitch Wheel chapter                   */
/****************************************************************/

void nsys_netout_journal_addpwheel(nsys_netout_jsend_state * jsend, 
				unsigned char ndata, unsigned char vdata)

{
  jsend->chapterw[NSYS_SM_CW_LOC_FIRST] = ndata;
  jsend->chapterw[NSYS_SM_CW_LOC_SECOND] = vdata;

  nsys_netout_jsend_slist[nsys_netout_jsend_slist_size++] = 
    &(jsend->chapterw[NSYS_SM_CW_LOC_FIRST]);
  
  if (jsend->chapterw_seqnum)
    jsend->cheader[NSYS_SM_CH_LOC_FLAGS] &= NSYS_SM_CLRS;
  else
    {
      jsend->cheader[NSYS_SM_CH_LOC_TOC] |= NSYS_SM_CH_TOC_SETW;
      
      *((unsigned short *) &(jsend->cheader[NSYS_SM_CH_LOC_LEN])) = 
	htons((jsend->clen += NSYS_SM_CW_SIZE));
    }
  jsend->chapterw_seqnum = nsys_netout_seqnum;

  nsys_netout_jsend_guard_time = nsys_netout_jsend_guard_mintime;
  nsys_netout_jsend_guard_next = (nsys_netout_jsend_guard_mintime << 
			     (nsys_feclevel == NSYS_SM_FEC_MINIMAL));
}

/****************************************************************/
/*                  add MIDI NoteOff chapter                    */
/****************************************************************/

void nsys_netout_journal_addnoteoff(nsys_netout_jsend_state * jsend, 
				    unsigned char ndata, unsigned char vdata)

{
  unsigned char bitfield;
  int i;

  bitfield = (ndata >> NSYS_SM_CN_BFSHIFT);
  
  /******************************/
  /* initialize chapter and TOC */
  /******************************/
  
  if (jsend->chaptern_seqnum == 0)
    {
      jsend->chaptern_size = NSYS_SM_CN_HDRSIZE;
      jsend->chapterb_size = 1;
      jsend->chapterb_low = bitfield;
      jsend->chapterb_high = bitfield;
      
      jsend->chaptern[NSYS_SM_CN_LOC_LENGTH] = 0;
      jsend->chaptern[NSYS_SM_CN_LOC_LOWHIGH] = 
	((jsend->chapterb_low << NSYS_SM_CN_LOWSHIFT) |
	 jsend->chapterb_high);
      jsend->chapterb[bitfield] = 0;
      
      jsend->clen += 1 + NSYS_SM_CN_HDRSIZE;
      jsend->cheader[NSYS_SM_CH_LOC_TOC] |= NSYS_SM_CH_TOC_SETN;
    }
  
  /***************************/
  /* delete expired note log */
  /***************************/
  
  if (jsend->chaptern_seqarray[ndata] && 
      ((i = jsend->chaptern_logptr[ndata]) < NSYS_SM_CN_SIZE))
    {
      if (jsend->chaptern[i+NSYS_SM_CN_LOC_VEL] & NSYS_SM_CN_CHKY)
	jsend->chaptern_timernum--;
      memmove(&(jsend->chaptern[i]), 
	      &(jsend->chaptern[i + NSYS_SM_CN_LOGSIZE]),
	      jsend->chaptern_size - i - NSYS_SM_CN_LOGSIZE);

      if (jsend->chaptern_size < NSYS_SM_CN_SIZE)
	jsend->chaptern[NSYS_SM_CN_LOC_LENGTH]--;
      jsend->chaptern_size -= NSYS_SM_CN_LOGSIZE;
      jsend->clen -= NSYS_SM_CN_LOGSIZE;

      while (i < jsend->chaptern_size)
	{
	  jsend->chaptern_logptr[jsend->chaptern[i] & NSYS_SM_CLRS] 
	    -= NSYS_SM_CN_LOGSIZE;
	  i += NSYS_SM_CN_LOGSIZE;
	}
    }
  
  /********************************/
  /* add bitfield bytes if needed */
  /********************************/
  
  if (jsend->chapterb_high < jsend->chapterb_low)
    {
      jsend->chapterb_size = 1;
      jsend->chapterb_low = bitfield;
      jsend->chapterb_high = bitfield;
      jsend->chaptern[NSYS_SM_CN_LOC_LOWHIGH] = 
	((jsend->chapterb_low << NSYS_SM_CN_LOWSHIFT) |
	 jsend->chapterb_high);
      jsend->chapterb[bitfield] = 0;
      jsend->clen++;
    }
  
  if (bitfield > jsend->chapterb_high)
    {
      memset(&(jsend->chapterb[jsend->chapterb_high + 1]), 0,
	     (bitfield - jsend->chapterb_high));
      jsend->chapterb_size += (bitfield - jsend->chapterb_high);
      jsend->clen += (bitfield - jsend->chapterb_high);
      jsend->chapterb_high = bitfield;	  
      jsend->chaptern[NSYS_SM_CN_LOC_LOWHIGH] = 
	((jsend->chapterb_low << NSYS_SM_CN_LOWSHIFT) |
	 jsend->chapterb_high);
    }
  
  if (bitfield < jsend->chapterb_low)
    {
      memset(&(jsend->chapterb[bitfield]), 0,
	     (jsend->chapterb_low - bitfield));
      jsend->chapterb_size += (jsend->chapterb_low - bitfield);
      jsend->clen += (jsend->chapterb_low - bitfield);
      jsend->chapterb_low = bitfield;	  
      jsend->chaptern[NSYS_SM_CN_LOC_LOWHIGH] = 
	((jsend->chapterb_low << NSYS_SM_CN_LOWSHIFT) |
	 jsend->chapterb_high);
    }
  
  /*****************/
  /* update slists */
  /*****************/
  
  jsend->chaptern[NSYS_SM_CN_LOC_LENGTH] &= NSYS_SM_CLRS; 
  nsys_netout_jsend_slist[nsys_netout_jsend_slist_size++] = 
    &(jsend->chaptern[NSYS_SM_CN_LOC_LENGTH]);
  
  /******************************************************************/
  /* set correct bit in bitfield byte, update channel/chapter state */
  /******************************************************************/
  
  jsend->chapterb[bitfield] |= (1 << ((~ndata) & NSYS_SM_CN_BFMASK));
  jsend->chaptern_logptr[ndata] = (NSYS_SM_CN_SIZE + bitfield);
  
  *((unsigned short *) &(jsend->cheader[NSYS_SM_CH_LOC_LEN])) = 
    htons(jsend->clen);
  
  jsend->chaptern_seqarray[ndata] = nsys_netout_seqnum;
  jsend->chaptern_seqnum = nsys_netout_seqnum;

  nsys_netout_jsend_guard_time = nsys_netout_jsend_guard_mintime;
  nsys_netout_jsend_guard_next = (nsys_netout_jsend_guard_mintime << 
			     (nsys_feclevel == NSYS_SM_FEC_MINIMAL));

}

/****************************************************************/
/*               add MIDI NoteOn chapter                   */
/****************************************************************/

void nsys_netout_journal_addnoteon(nsys_netout_jsend_state * jsend, 
				   unsigned char ndata, unsigned char vdata)

{
  int i;

  /******************************/
  /* initialize chapter and TOC */
  /******************************/
  
  if (jsend->chaptern_seqnum == 0)
    {
      jsend->chaptern_size = NSYS_SM_CN_HDRSIZE;
      jsend->chapterb_size = 0;
      jsend->chapterb_low = NSYS_SM_CN_BFMAX;
      jsend->chapterb_high = 1;
      
      jsend->chaptern[NSYS_SM_CN_LOC_LENGTH] = NSYS_SM_CN_SETB;
      jsend->chaptern[NSYS_SM_CN_LOC_LOWHIGH] = 
	((jsend->chapterb_low << NSYS_SM_CN_LOWSHIFT) |
	 jsend->chapterb_high);
      
      jsend->clen += NSYS_SM_CN_HDRSIZE;
      jsend->cheader[NSYS_SM_CH_LOC_TOC] |= NSYS_SM_CH_TOC_SETN;
    }
  
  
  if (jsend->chaptern_seqarray[ndata]) 
    {
      if ((i = (jsend->chaptern_logptr[ndata] - NSYS_SM_CN_SIZE)) >= 0)
	{

	  /*********************************/
	  /* zero expired NoteOff bitfield */
	  /*********************************/
	  
	  jsend->chapterb[i] &= ~(1 << ((~ndata) & NSYS_SM_CN_BFMASK));
	  if ((jsend->chapterb[i] == 0) && 
	      ((i == jsend->chapterb_low) || 
	       (i == jsend->chapterb_high)))
	    {
	      while (jsend->chapterb_size && 
		     (jsend->chapterb[jsend->chapterb_low] == 0))
		{
		  jsend->chapterb_low++;
		  jsend->chapterb_size--;
		  jsend->clen--;
		}
	      while (jsend->chapterb_size && 
		     (jsend->chapterb[jsend->chapterb_high] == 0))
		{
		  jsend->chapterb_high--;
		  jsend->chapterb_size--;
		  jsend->clen--;
		}
	      if (jsend->chapterb_size == 0)
		{	  
		  jsend->chapterb_low = NSYS_SM_CN_BFMAX;
		  jsend->chapterb_high = 1; 
		}
	      jsend->chaptern[NSYS_SM_CN_LOC_LOWHIGH] = 
		((jsend->chapterb_low << NSYS_SM_CN_LOWSHIFT) |
		 jsend->chapterb_high);
	    }
	}
      else
	{
	  /***************************/
	  /* delete expired note log */
	  /***************************/

	  i = jsend->chaptern_logptr[ndata];

	  if (jsend->chaptern[i+NSYS_SM_CN_LOC_VEL] & NSYS_SM_CN_CHKY)
	    jsend->chaptern_timernum--;
	  memmove(&(jsend->chaptern[i]), 
		  &(jsend->chaptern[i + NSYS_SM_CN_LOGSIZE]),
		  jsend->chaptern_size - i - NSYS_SM_CN_LOGSIZE);
	  
	  if (jsend->chaptern_size < NSYS_SM_CN_SIZE)
	    jsend->chaptern[NSYS_SM_CN_LOC_LENGTH]--;
	  else
	    jsend->chapterb_high = 1; 

	  jsend->chaptern_size -= NSYS_SM_CN_LOGSIZE;
	  jsend->clen -= NSYS_SM_CN_LOGSIZE;
	  
	  while (i < jsend->chaptern_size)
	    {
	      jsend->chaptern_logptr[jsend->chaptern[i] & NSYS_SM_CLRS] 
		-= NSYS_SM_CN_LOGSIZE;
	      i += NSYS_SM_CN_LOGSIZE;
	    }
	}
    }
  
  /********************/
  /* add new note log */
  /********************/

  jsend->chaptern[jsend->chaptern_size +
		  NSYS_SM_CN_LOC_NUM] = ndata;
  jsend->chaptern[jsend->chaptern_size +
		  NSYS_SM_CN_LOC_VEL] = (vdata | NSYS_SM_CN_SETY);
  jsend->chaptern_logptr[ndata] = jsend->chaptern_size;
  jsend->chaptern_timernum++;
  jsend->chaptern_size += NSYS_SM_CN_LOGSIZE;
  jsend->clen += NSYS_SM_CN_LOGSIZE;
  
  if (jsend->chaptern_size != NSYS_SM_CN_SIZE)
    jsend->chaptern[NSYS_SM_CN_LOC_LENGTH]++;
  else
    jsend->chapterb_high = NSYS_SM_CN_BFMIN;

  jsend->chaptern_timer[ndata] = (nsys_netout_tstamp + (int)
				  (ARATE * NSYS_SM_CN_MAXDELAY));
  
  /********************************/
  /* update channel/chapter state */
  /********************************/
  
  *((unsigned short *) &(jsend->cheader[NSYS_SM_CH_LOC_LEN])) = 
    htons(jsend->clen);
  jsend->chaptern_sset = 1;
  
  jsend->chaptern_seqarray[ndata] = nsys_netout_seqnum;
  jsend->chaptern_seqnum = nsys_netout_seqnum;

  if (nsys_netout_jsend_guard_ontime)
    {
      nsys_netout_jsend_guard_time = nsys_netout_jsend_guard_ontime;
      nsys_netout_jsend_guard_next = nsys_netout_jsend_guard_mintime;
    }
}

/****************************************************************/
/*            NoteOn update for Note Extras chapter             */
/****************************************************************/

void nsys_netout_journal_addnoteon_extras(nsys_netout_jsend_state * jsend, 
					 unsigned char ndata)
{
  int i, j, update, delete;

  delete = update = 0;

  /**********************************************/
  /* delete old reference count for note number */
  /**********************************************/

  if (jsend->chapterer_seqarray[ndata])
    {
      update = 1;

      i = jsend->chapterer_logptr[ndata];

      memmove(&(jsend->chaptere[i]), 
	      &(jsend->chaptere[i + NSYS_SM_CE_LOGSIZE]),
	      jsend->chaptere_size - i - NSYS_SM_CE_LOGSIZE);

      jsend->chapterer_seqarray[ndata] = 0;
      jsend->clen -= NSYS_SM_CE_LOGSIZE;
      jsend->chaptere_size -= NSYS_SM_CE_LOGSIZE;

      while (i < jsend->chaptere_size)
	{
	  if (jsend->chaptere[i + 1] & NSYS_SM_CE_CHKV) 
	    jsend->chapterev_logptr[jsend->chaptere[i] & NSYS_SM_CLRS] 
	      -= NSYS_SM_CE_LOGSIZE;
	  else
	    jsend->chapterer_logptr[jsend->chaptere[i] & NSYS_SM_CLRS] 
	      -= NSYS_SM_CE_LOGSIZE;
	  i += NSYS_SM_CE_LOGSIZE;
	}

      if (jsend->chaptere[NSYS_SM_CE_LOC_LENGTH] & NSYS_SM_CLRS)
	jsend->chaptere[NSYS_SM_CE_LOC_LENGTH]--;
      else
	delete = 1;
    }

  /*******************************************/
  /* delete release velocity for note number */
  /*******************************************/

  if (jsend->chapterev_seqarray[ndata])
    {
      update = 1;

      i = jsend->chapterev_logptr[ndata];

      memmove(&(jsend->chaptere[i]), 
	      &(jsend->chaptere[i + NSYS_SM_CE_LOGSIZE]),
	      jsend->chaptere_size - i - NSYS_SM_CE_LOGSIZE);

      jsend->chapterev_seqarray[ndata] = 0;
      jsend->clen -= NSYS_SM_CE_LOGSIZE;
      jsend->chaptere_size -= NSYS_SM_CE_LOGSIZE;

      while (i < jsend->chaptere_size)
	{
	  if (jsend->chaptere[i + 1] & NSYS_SM_CE_CHKV) 
	    jsend->chapterev_logptr[jsend->chaptere[i] & NSYS_SM_CLRS] 
	      -= NSYS_SM_CE_LOGSIZE;
	  else
	    jsend->chapterer_logptr[jsend->chaptere[i] & NSYS_SM_CLRS] 
	      -= NSYS_SM_CE_LOGSIZE;
	  i += NSYS_SM_CE_LOGSIZE;
	}

      if (jsend->chaptere[NSYS_SM_CE_LOC_LENGTH] & NSYS_SM_CLRS)
	jsend->chaptere[NSYS_SM_CE_LOC_LENGTH]--;
      else
	delete = 1;
    }

  /**************************************************/
  /* increase reference count, add log if necessary */
  /**************************************************/

  jsend->chaptere_ref[ndata]++;

  if (jsend->chaptere_ref[ndata] > 1)
    {
      /***************************************/
      /* make space for new log if necessary */
      /***************************************/
      
      if ((jsend->chaptere[NSYS_SM_CE_LOC_LENGTH] & NSYS_SM_CLRS) == 127)
	{
	  j = NSYS_SM_CE_HDRSIZE;
	  while (j < jsend->chaptere_size)
	    {
	      i = NSYS_SM_CLRS & jsend->chaptere[j];
	      
	      if (((jsend->chaptere[NSYS_SM_CE_LOC_LENGTH] & NSYS_SM_CLRS) == 127) &&
		  (jsend->chaptere[j + 1] & NSYS_SM_CE_CHKV))
		{
		  jsend->chapterev_seqarray[i] = 0;
		  memmove(&(jsend->chaptere[j]), 
			  &(jsend->chaptere[j + NSYS_SM_CE_LOGSIZE]),
			  jsend->chaptere_size - j - NSYS_SM_CE_LOGSIZE);
		  jsend->clen -= NSYS_SM_CE_LOGSIZE;
		  jsend->chaptere_size -= NSYS_SM_CE_LOGSIZE;
		  jsend->chaptere[NSYS_SM_CE_LOC_LENGTH]--;
		}
	      else
		{
		  if (jsend->chaptere[j + 1] & NSYS_SM_CE_CHKV)
		    jsend->chapterev_logptr[i] = j;
		  else
		    jsend->chapterer_logptr[i] = j;
		  j += NSYS_SM_CE_LOGSIZE;
		}
	    }
	}

      /*****************************/
      /* initialize chapter header */
      /*****************************/
      
      if (jsend->chaptere_seqnum)
	{
	  jsend->chaptere[NSYS_SM_CE_LOC_LENGTH] &= NSYS_SM_CLRS;
	}
      else
	{
	  jsend->chaptere_size = NSYS_SM_CE_HDRSIZE;
	  jsend->chaptere[NSYS_SM_CA_LOC_LENGTH] = 0;
	  jsend->cheader[NSYS_SM_CH_LOC_TOC] |= NSYS_SM_CH_TOC_SETE;
	}

      /**************************************************/
      /* code below is broken for counts > 127.         */
      /* sender needs to keep track of counts > 127.    */
      /**************************************************/

      if (jsend->chaptere_ref[ndata] > NSYS_SM_CE_MAXCOUNT)
	jsend->chaptere_ref[ndata] = NSYS_SM_CE_MAXCOUNT;

      /**************************/
      /* add new controller log */
      /**************************/

      if (jsend->chaptere_seqnum && !delete)
	jsend->chaptere[NSYS_SM_CE_LOC_LENGTH]++;
     
      jsend->chaptere[jsend->chaptere_size + NSYS_SM_CE_LOC_NUM] = ndata;
      jsend->chaptere[jsend->chaptere_size + NSYS_SM_CE_LOC_COUNTVEL] = 
	jsend->chaptere_ref[ndata];  /* also sets V bit to 0 */

      jsend->chapterer_logptr[ndata] = jsend->chaptere_size;
      jsend->chaptere_size += NSYS_SM_CE_LOGSIZE;
      jsend->clen += (NSYS_SM_CE_LOGSIZE + (jsend->chaptere_seqnum ? 0 :
					    NSYS_SM_CE_HDRSIZE));

      jsend->chapterer_seqarray[ndata] = nsys_netout_seqnum;

      jsend->chaptere_sset = 1;

      nsys_netout_jsend_slist[nsys_netout_jsend_slist_size++] = 
	&(jsend->chaptere[NSYS_SM_CA_LOC_LENGTH]);

      nsys_netout_jsend_guard_time = nsys_netout_jsend_guard_mintime;
      nsys_netout_jsend_guard_next = (nsys_netout_jsend_guard_mintime << 
				      (nsys_feclevel == NSYS_SM_FEC_MINIMAL));

      update = 1;
      delete = 0;
    }

  /**************************/
  /* Chapter E housekeeping */
  /**************************/

  if (update)
    {
      if (delete)
	{
	  jsend->chaptere_sset = 0;
	  jsend->chaptere_seqnum = 0;
	  jsend->cheader[NSYS_SM_CH_LOC_TOC] &= NSYS_SM_CH_TOC_CLRE;
	  jsend->clen -= NSYS_SM_CE_HDRSIZE;
	}
      else
	{
  	  jsend->chaptere_seqnum = nsys_netout_seqnum;
	}

      *((unsigned short *) &(jsend->cheader[NSYS_SM_CH_LOC_LEN])) = 
	htons(jsend->clen);
    }

  return;
}

/****************************************************************/
/*            NoteOff update for Note Extras chapter             */
/****************************************************************/

void nsys_netout_journal_addnoteoff_extras(nsys_netout_jsend_state * jsend, 
					   unsigned char ndata,
					   unsigned char vdata)
{
  int i, j, update, delete;

  delete = update = 0;

  /**********************************************/
  /* delete old reference count for note number */
  /**********************************************/

  if (jsend->chapterer_seqarray[ndata])
    {
      update = 1;

      i = jsend->chapterer_logptr[ndata];

      memmove(&(jsend->chaptere[i]), 
	      &(jsend->chaptere[i + NSYS_SM_CE_LOGSIZE]),
	      jsend->chaptere_size - i - NSYS_SM_CE_LOGSIZE);

      jsend->chapterer_seqarray[ndata] = 0;
      jsend->clen -= NSYS_SM_CE_LOGSIZE;
      jsend->chaptere_size -= NSYS_SM_CE_LOGSIZE;

      while (i < jsend->chaptere_size)
	{
	  if (jsend->chaptere[i + 1] & NSYS_SM_CE_CHKV) 
	    jsend->chapterev_logptr[jsend->chaptere[i] & NSYS_SM_CLRS] 
	      -= NSYS_SM_CE_LOGSIZE;
	  else
	    jsend->chapterer_logptr[jsend->chaptere[i] & NSYS_SM_CLRS] 
	      -= NSYS_SM_CE_LOGSIZE;
	  i += NSYS_SM_CE_LOGSIZE;
	}

      if (jsend->chaptere[NSYS_SM_CE_LOC_LENGTH] & NSYS_SM_CLRS)
	jsend->chaptere[NSYS_SM_CE_LOC_LENGTH]--;
      else
	delete = 1;
    }

  /*******************************************/
  /* delete release velocity for note number */
  /*******************************************/

  if (jsend->chapterev_seqarray[ndata])
    {
      update = 1;

      i = jsend->chapterev_logptr[ndata];

      memmove(&(jsend->chaptere[i]), 
	      &(jsend->chaptere[i + NSYS_SM_CE_LOGSIZE]),
	      jsend->chaptere_size - i - NSYS_SM_CE_LOGSIZE);

      jsend->chapterev_seqarray[ndata] = 0;
      jsend->clen -= NSYS_SM_CE_LOGSIZE;
      jsend->chaptere_size -= NSYS_SM_CE_LOGSIZE;

      while (i < jsend->chaptere_size)
	{
	  if (jsend->chaptere[i + 1] & NSYS_SM_CE_CHKV) 
	    jsend->chapterev_logptr[jsend->chaptere[i] & NSYS_SM_CLRS] 
	      -= NSYS_SM_CE_LOGSIZE;
	  else
	    jsend->chapterer_logptr[jsend->chaptere[i] & NSYS_SM_CLRS] 
	      -= NSYS_SM_CE_LOGSIZE;
	  i += NSYS_SM_CE_LOGSIZE;
	}

      if (jsend->chaptere[NSYS_SM_CE_LOC_LENGTH] & NSYS_SM_CLRS)
	jsend->chaptere[NSYS_SM_CE_LOC_LENGTH]--;
      else
	delete = 1;
    }

  /**************************************************/
  /* decrease reference count, add log if necessary */
  /**************************************************/

  if (jsend->chaptere_ref[ndata])
    jsend->chaptere_ref[ndata]--;

  if (jsend->chaptere_ref[ndata] > 1)
    {
      /***************************************/
      /* make space for new log if necessary */
      /***************************************/
      
      if ((jsend->chaptere[NSYS_SM_CE_LOC_LENGTH] & NSYS_SM_CLRS) == 127)
	{
	  j = NSYS_SM_CE_HDRSIZE;
	  while (j < jsend->chaptere_size)
	    {
	      i = NSYS_SM_CLRS & jsend->chaptere[j];
	      
	      if (((jsend->chaptere[NSYS_SM_CE_LOC_LENGTH] & NSYS_SM_CLRS) == 127) &&
		  (jsend->chaptere[j + 1] & NSYS_SM_CE_CHKV))
		{
		  jsend->chapterev_seqarray[i] = 0;
		  memmove(&(jsend->chaptere[j]), 
			  &(jsend->chaptere[j + NSYS_SM_CE_LOGSIZE]),
			  jsend->chaptere_size - j - NSYS_SM_CE_LOGSIZE);
		  jsend->clen -= NSYS_SM_CE_LOGSIZE;
		  jsend->chaptere_size -= NSYS_SM_CE_LOGSIZE;
		  jsend->chaptere[NSYS_SM_CE_LOC_LENGTH]--;
		}
	      else
		{
		  if (jsend->chaptere[j + 1] & NSYS_SM_CE_CHKV)
		    jsend->chapterev_logptr[i] = j;
		  else
		    jsend->chapterer_logptr[i] = j;
		  j += NSYS_SM_CE_LOGSIZE;
		}
	    }
	}

      /*****************************/
      /* initialize chapter header */
      /*****************************/
      
      if (jsend->chaptere_seqnum)
	{
	  jsend->chaptere[NSYS_SM_CE_LOC_LENGTH] &= NSYS_SM_CLRS;
	}
      else
	{
	  jsend->chaptere_size = NSYS_SM_CE_HDRSIZE;
	  jsend->chaptere[NSYS_SM_CA_LOC_LENGTH] = 0;
	  jsend->cheader[NSYS_SM_CH_LOC_TOC] |= NSYS_SM_CH_TOC_SETE;
	}

      /**************************************************/
      /* code below is broken for counts > 127.         */
      /* sender needs to keep track of counts > 127.    */
      /**************************************************/

      if (jsend->chaptere_ref[ndata] > NSYS_SM_CE_MAXCOUNT)
	jsend->chaptere_ref[ndata] = NSYS_SM_CE_MAXCOUNT;

      /**************************/
      /* add new controller log */
      /**************************/

      if (jsend->chaptere_seqnum && !delete)
	jsend->chaptere[NSYS_SM_CE_LOC_LENGTH]++;
     
      jsend->chaptere[jsend->chaptere_size + NSYS_SM_CE_LOC_NUM] = ndata;
      jsend->chaptere[jsend->chaptere_size + NSYS_SM_CE_LOC_COUNTVEL] = 
	jsend->chaptere_ref[ndata];  /* also sets V bit to 0 */

      jsend->chapterer_logptr[ndata] = jsend->chaptere_size;
      jsend->chaptere_size += NSYS_SM_CE_LOGSIZE;
      jsend->clen += (NSYS_SM_CE_LOGSIZE + (jsend->chaptere_seqnum ? 0 :
					    NSYS_SM_CE_HDRSIZE));

      jsend->chapterer_seqarray[ndata] = nsys_netout_seqnum;

      jsend->chaptere_sset = 1;

      nsys_netout_jsend_slist[nsys_netout_jsend_slist_size++] = 
	&(jsend->chaptere[NSYS_SM_CA_LOC_LENGTH]);
	
      nsys_netout_jsend_guard_time = nsys_netout_jsend_guard_mintime;
      nsys_netout_jsend_guard_next = (nsys_netout_jsend_guard_mintime << 
				      (nsys_feclevel == NSYS_SM_FEC_MINIMAL));

      update = 1;
      delete = 0;
    }

  /***************************************************************/
  /* add release velocity log if necessary, and if there is room */
  /***************************************************************/

  if ((vdata != NSYS_SM_CE_DEFREL) && 
      ((jsend->chaptere[NSYS_SM_CE_LOC_LENGTH] & NSYS_SM_CLRS) < 127))
    {
      /*****************************/
      /* initialize chapter header */
      /*****************************/
      
      if (jsend->chaptere_seqnum)
	{
	  jsend->chaptere[NSYS_SM_CE_LOC_LENGTH] &= NSYS_SM_CLRS;
	}
      else
	{
	  jsend->chaptere_size = NSYS_SM_CE_HDRSIZE;
	  jsend->chaptere[NSYS_SM_CA_LOC_LENGTH] = 0;
	  jsend->cheader[NSYS_SM_CH_LOC_TOC] |= NSYS_SM_CH_TOC_SETE;
	}

      /**************************/
      /* add new controller log */
      /**************************/

      if (jsend->chaptere_seqnum && !delete)
	jsend->chaptere[NSYS_SM_CE_LOC_LENGTH]++;
     
      jsend->chaptere[jsend->chaptere_size + NSYS_SM_CE_LOC_NUM] = ndata;
      jsend->chaptere[jsend->chaptere_size + NSYS_SM_CE_LOC_COUNTVEL] = 
	vdata | NSYS_SM_CE_SETV;

      jsend->chapterev_logptr[ndata] = jsend->chaptere_size;
      jsend->chaptere_size += NSYS_SM_CE_LOGSIZE;
      jsend->clen += (NSYS_SM_CE_LOGSIZE + (jsend->chaptere_seqnum ? 0 :
					    NSYS_SM_CE_HDRSIZE));

      jsend->chapterev_seqarray[ndata] = nsys_netout_seqnum;

      jsend->chaptere_sset = 1;

      nsys_netout_jsend_slist[nsys_netout_jsend_slist_size++] = 
	&(jsend->chaptere[NSYS_SM_CA_LOC_LENGTH]);
      
      nsys_netout_jsend_guard_time = nsys_netout_jsend_guard_mintime;
      nsys_netout_jsend_guard_next = (nsys_netout_jsend_guard_mintime << 
				      (nsys_feclevel == NSYS_SM_FEC_MINIMAL));

      update = 1;
      delete = 0;
    }

  /**************************/
  /* Chapter E housekeeping */
  /**************************/

  if (update)
    {
      if (delete)
	{
	  jsend->chaptere_sset = 0;
	  jsend->chaptere_seqnum = 0;
	  jsend->cheader[NSYS_SM_CH_LOC_TOC] &= NSYS_SM_CH_TOC_CLRE;
	  jsend->clen -= NSYS_SM_CE_HDRSIZE;
	}
      else
	{
  	  jsend->chaptere_seqnum = nsys_netout_seqnum;
	}

      *((unsigned short *) &(jsend->cheader[NSYS_SM_CH_LOC_LEN])) = 
	htons(jsend->clen);
    }

  return;
}

/****************************************************************/
/*                  add MIDI Channel Touch chapter              */
/****************************************************************/

void nsys_netout_journal_addctouch(nsys_netout_jsend_state * jsend, 
				 unsigned char ndata)

{
  jsend->chaptert[NSYS_SM_CT_LOC_PRESSURE] = ndata;

  nsys_netout_jsend_slist[nsys_netout_jsend_slist_size++] = 
    &(jsend->chaptert[NSYS_SM_CT_LOC_PRESSURE]);
  
  if (jsend->chaptert_seqnum)
    jsend->cheader[NSYS_SM_CH_LOC_FLAGS] &= NSYS_SM_CLRS;
  else
    {
      jsend->cheader[NSYS_SM_CH_LOC_TOC] |= NSYS_SM_CH_TOC_SETT;
      
      *((unsigned short *) &(jsend->cheader[NSYS_SM_CH_LOC_LEN])) = 
	htons((jsend->clen += NSYS_SM_CT_SIZE));
    }
  
  jsend->chaptert_seqnum = nsys_netout_seqnum;

  nsys_netout_jsend_guard_time = nsys_netout_jsend_guard_mintime;
  nsys_netout_jsend_guard_next = (nsys_netout_jsend_guard_mintime << 
			     (nsys_feclevel == NSYS_SM_FEC_MINIMAL));
}

/****************************************************************/
/*                  add MIDI Poly Touch chapter                 */
/****************************************************************/

void nsys_netout_journal_addptouch(nsys_netout_jsend_state * jsend, 
				    unsigned char ndata,
				    unsigned char vdata)

{
  int i;

  /*****************************/
  /* initialize chapter header */
  /*****************************/
  
  if (jsend->chaptera_seqnum)
    jsend->chaptera[NSYS_SM_CA_LOC_LENGTH] &= NSYS_SM_CLRS;
  else
    {
      jsend->chaptera_size = NSYS_SM_CA_HDRSIZE;
      jsend->chaptera[NSYS_SM_CA_LOC_LENGTH] = 0;
      jsend->cheader[NSYS_SM_CH_LOC_TOC] |= NSYS_SM_CH_TOC_SETA;
    }

  if (jsend->chaptera_seqarray[ndata])
    {
      /**********************/
      /* remove expired log */
      /**********************/

      i = jsend->chaptera_logptr[ndata];

      memmove(&(jsend->chaptera[i]), 
	      &(jsend->chaptera[i + NSYS_SM_CA_LOGSIZE]),
	      jsend->chaptera_size - i - NSYS_SM_CA_LOGSIZE);

      jsend->chaptera[NSYS_SM_CA_LOC_LENGTH]--;
      jsend->chaptera_size -= NSYS_SM_CA_LOGSIZE;
      jsend->clen -= NSYS_SM_CA_LOGSIZE;

      while (i < jsend->chaptera_size)
	{
	  jsend->chaptera_logptr[jsend->chaptera[i] & NSYS_SM_CLRS] 
	    -= NSYS_SM_CA_LOGSIZE;
	  i += NSYS_SM_CA_LOGSIZE;
	}
    }
  
  /************************/
  /* add new pressure log */
  /************************/
  
  /* put data in chapter a and update state variables */
      
  if (jsend->chaptera_seqnum)
    jsend->chaptera[NSYS_SM_CA_LOC_LENGTH]++;
  
  jsend->chaptera[jsend->chaptera_size + 
		  NSYS_SM_CA_LOC_NUM] = ndata;
  jsend->chaptera[jsend->chaptera_size + 
		  NSYS_SM_CA_LOC_PRESSURE] = vdata;
  
  jsend->chaptera_logptr[ndata] = jsend->chaptera_size;
  jsend->chaptera_size += NSYS_SM_CA_LOGSIZE;
  
  jsend->clen += (NSYS_SM_CA_LOGSIZE + (jsend->chaptera_seqnum ? 0 :
					NSYS_SM_CA_HDRSIZE));
  
  *((unsigned short *) &(jsend->cheader[NSYS_SM_CH_LOC_LEN])) = 
    htons(jsend->clen);
  
  /*****************/
  /* update slists */
  /*****************/
  
  nsys_netout_jsend_slist[nsys_netout_jsend_slist_size++] = 
    &(jsend->chaptera[NSYS_SM_CA_LOC_LENGTH]);
  jsend->chaptera_sset = 1;
  
  jsend->chaptera_seqarray[ndata] = nsys_netout_seqnum;
  jsend->chaptera_seqnum = nsys_netout_seqnum;

  nsys_netout_jsend_guard_time = nsys_netout_jsend_guard_mintime;
  nsys_netout_jsend_guard_next = (nsys_netout_jsend_guard_mintime << 
			     (nsys_feclevel == NSYS_SM_FEC_MINIMAL));
}

/****************************************************************/
/*              add Reset to System Simple chapter              */
/****************************************************************/

void nsys_netout_journal_addreset()

{
  nsys_netout_jsend_system_state * jsys;
  unsigned char * p;

  nsys_netout_journal_clear_active(CSYS_MIDI_SYSTEM_RESET);

  jsys = &(nsys_netout_jsend_system);

  jsys->chapterd_reset++;
  jsys->chapterd_reset &= NSYS_SM_CLRS;
  jsys->chapterd_front[NSYS_SM_CD_LOC_LOGS] = jsys->chapterd_reset;

  if (!(jsys->chapterd_reset_seqnum))
    {
      p = &(jsys->chapterd_front[NSYS_SM_CD_LOC_LOGS]);
      if (jsys->chapterd_tune_seqnum)
	(*(++p)) = jsys->chapterd_tune;
      if (jsys->chapterd_song_seqnum)
	(*(++p)) = jsys->chapterd_song;
    }

  nsys_netout_jsend_slist[nsys_netout_jsend_slist_size++] = 
    &(jsys->chapterd_front[NSYS_SM_CD_LOC_TOC]);
  nsys_netout_jsend_slist[nsys_netout_jsend_slist_size++] = 
    &(jsys->chapterd_front[NSYS_SM_CD_LOC_LOGS]);

  if (jsys->chapterd_seqnum && jsys->chapterd_reset_seqnum)
    {
      jsys->chapterd_front[NSYS_SM_CD_LOC_TOC] &= NSYS_SM_CLRS;
      jsys->sheader[NSYS_SM_SH_LOC_FLAGS] &= NSYS_SM_CLRS;
    }
  else
    {
      jsys->sheader[NSYS_SM_SH_LOC_FLAGS] &= ~(NSYS_SM_SH_MSBMASK | NSYS_SM_SETS);

      if (!(jsys->chapterd_seqnum))
	{
	  jsys->sheader[NSYS_SM_SH_LOC_FLAGS] |= NSYS_SM_SH_SETD;
	  jsys->chapterd_front[NSYS_SM_CD_LOC_TOC] = 0;
	  jsys->slen += NSYS_SM_CD_SIZE_TOC;
	  jsys->chapterd_front_size = NSYS_SM_CD_SIZE_TOC;
	}

      jsys->chapterd_front[NSYS_SM_CD_LOC_TOC] |= NSYS_SM_CD_TOC_SETB;
      jsys->chapterd_front_size += NSYS_SM_CD_SIZE_RESET;

      if ((jsys->slen += NSYS_SM_CD_SIZE_RESET) <= NSYS_SM_SH_LSBMASK)
	jsys->sheader[NSYS_SM_SH_LOC_LENLSB] = (unsigned char)(jsys->slen);
      else
	{
	  jsys->sheader[NSYS_SM_SH_LOC_FLAGS] |= (unsigned char)(jsys->slen >> 8);
	  jsys->sheader[NSYS_SM_SH_LOC_LENLSB] = 
	    (unsigned char)(NSYS_SM_SH_LSBMASK & jsys->slen);  
	}
    }

  jsys->chapterd_seqnum = nsys_netout_seqnum;
  jsys->chapterd_reset_seqnum = nsys_netout_seqnum;

  nsys_netout_jsend_guard_time = nsys_netout_jsend_guard_mintime;
  nsys_netout_jsend_guard_next = (nsys_netout_jsend_guard_mintime << 
			     (nsys_feclevel == NSYS_SM_FEC_MINIMAL));
}

/****************************************************************/
/*           add Tune Request to System Simple chapter          */
/****************************************************************/

void nsys_netout_journal_addtune()

{
  nsys_netout_jsend_system_state * jsys;
  unsigned char * p;

  jsys = &(nsys_netout_jsend_system);

  jsys->chapterd_tune++;
  jsys->chapterd_tune &= NSYS_SM_CLRS;

  p = &(jsys->chapterd_front[NSYS_SM_CD_LOC_LOGS]);

  if (jsys->chapterd_reset_seqnum)
    p++;

  *p = jsys->chapterd_tune;

  if (!(jsys->chapterd_tune_seqnum) && (jsys->chapterd_song_seqnum))
    *(p + 1) = jsys->chapterd_song;

  nsys_netout_jsend_slist[nsys_netout_jsend_slist_size++] = 
    &(jsys->chapterd_front[NSYS_SM_CD_LOC_TOC]);
  nsys_netout_jsend_slist[nsys_netout_jsend_slist_size++] = p;

  if (jsys->chapterd_seqnum && jsys->chapterd_tune_seqnum)
    {
      jsys->chapterd_front[NSYS_SM_CD_LOC_TOC] &= NSYS_SM_CLRS;
      jsys->sheader[NSYS_SM_SH_LOC_FLAGS] &= NSYS_SM_CLRS;
    }
  else
    {
      jsys->sheader[NSYS_SM_SH_LOC_FLAGS] &= ~(NSYS_SM_SH_MSBMASK | NSYS_SM_SETS);

      if (!(jsys->chapterd_seqnum))
	{
	  jsys->sheader[NSYS_SM_SH_LOC_FLAGS] |= NSYS_SM_SH_SETD;
	  jsys->chapterd_front[NSYS_SM_CD_LOC_TOC] = 0;
	  jsys->slen += NSYS_SM_CD_SIZE_TOC;
	  jsys->chapterd_front_size = NSYS_SM_CD_SIZE_TOC;
	}

      jsys->chapterd_front[NSYS_SM_CD_LOC_TOC] |= NSYS_SM_CD_TOC_SETG;
      jsys->chapterd_front_size += NSYS_SM_CD_SIZE_TUNE;

      if ((jsys->slen += NSYS_SM_CD_SIZE_TUNE) <= NSYS_SM_SH_LSBMASK)
	jsys->sheader[NSYS_SM_SH_LOC_LENLSB] = (unsigned char)(jsys->slen);
      else
	{
	  jsys->sheader[NSYS_SM_SH_LOC_FLAGS] |= (unsigned char)(jsys->slen >> 8);
	  jsys->sheader[NSYS_SM_SH_LOC_LENLSB] = 
	    (unsigned char)(NSYS_SM_SH_LSBMASK & jsys->slen);  
	}
    }

  jsys->chapterd_seqnum = nsys_netout_seqnum;
  jsys->chapterd_tune_seqnum = nsys_netout_seqnum;

  nsys_netout_jsend_guard_time = nsys_netout_jsend_guard_mintime;
  nsys_netout_jsend_guard_next = (nsys_netout_jsend_guard_mintime << 
			     (nsys_feclevel == NSYS_SM_FEC_MINIMAL));
}

/****************************************************************/
/*           add Song Select to System Simple chapter           */
/****************************************************************/

void nsys_netout_journal_addsong(unsigned char ndata)

{
  nsys_netout_jsend_system_state * jsys;
  unsigned char * p;

  jsys = &(nsys_netout_jsend_system);

  jsys->chapterd_song = (ndata & NSYS_SM_CLRS);

  p = &(jsys->chapterd_front[NSYS_SM_CD_LOC_LOGS]);

  if (jsys->chapterd_reset_seqnum)
    p++;

  if (jsys->chapterd_tune_seqnum)
    p++;

  (*p) = jsys->chapterd_song;

  nsys_netout_jsend_slist[nsys_netout_jsend_slist_size++] = 
    &(jsys->chapterd_front[NSYS_SM_CD_LOC_TOC]);
  nsys_netout_jsend_slist[nsys_netout_jsend_slist_size++] = p;

  if (jsys->chapterd_seqnum && jsys->chapterd_song_seqnum)
    {
      jsys->chapterd_front[NSYS_SM_CD_LOC_TOC] &= NSYS_SM_CLRS;
      jsys->sheader[NSYS_SM_SH_LOC_FLAGS] &= NSYS_SM_CLRS;
    }
  else
    {
      jsys->sheader[NSYS_SM_SH_LOC_FLAGS] &= ~(NSYS_SM_SH_MSBMASK | NSYS_SM_SETS);

      if (!(jsys->chapterd_seqnum))
	{
	  jsys->sheader[NSYS_SM_SH_LOC_FLAGS] |= NSYS_SM_SH_SETD;
	  jsys->chapterd_front[NSYS_SM_CD_LOC_TOC] = 0;
	  jsys->slen += NSYS_SM_CD_SIZE_TOC;
	  jsys->chapterd_front_size = NSYS_SM_CD_SIZE_TOC;
	}

      jsys->chapterd_front[NSYS_SM_CD_LOC_TOC] |= NSYS_SM_CD_TOC_SETH;
      jsys->chapterd_front_size += NSYS_SM_CD_SIZE_SONG;

      if ((jsys->slen += NSYS_SM_CD_SIZE_SONG) <= NSYS_SM_SH_LSBMASK)
	jsys->sheader[NSYS_SM_SH_LOC_LENLSB] = (unsigned char)(jsys->slen);
      else
	{
	  jsys->sheader[NSYS_SM_SH_LOC_FLAGS] |= (unsigned char)(jsys->slen >> 8);
	  jsys->sheader[NSYS_SM_SH_LOC_LENLSB] = 
	    (unsigned char)(NSYS_SM_SH_LSBMASK & jsys->slen);  
	}
    }

  jsys->chapterd_seqnum = nsys_netout_seqnum;
  jsys->chapterd_song_seqnum = nsys_netout_seqnum;

  nsys_netout_jsend_guard_time = nsys_netout_jsend_guard_mintime;
  nsys_netout_jsend_guard_next = (nsys_netout_jsend_guard_mintime << 
			     (nsys_feclevel == NSYS_SM_FEC_MINIMAL));
}

/****************************************************************/
/*      add undefined System Common to System Simple chapter    */
/****************************************************************/

void nsys_netout_journal_addsc(unsigned char cmd, 
			       unsigned char ndata,
			       unsigned char vdata)

{
  nsys_netout_jsend_system_state * jsys;
  unsigned long sc_seqnum;
  unsigned char * p;
  unsigned char dsize;
  int delta;

  jsys = &(nsys_netout_jsend_system);
  
  dsize = ((ndata != CSYS_MIDI_SYSTEM_SYSEX_END) +
	   (vdata != CSYS_MIDI_SYSTEM_SYSEX_END));

  p = (cmd == CSYS_MIDI_SYSTEM_UNUSED1) ? jsys->chapterd_scj : jsys->chapterd_sck;
  
  p[NSYS_SM_CD_COMMON_LOC_TOC] = (dsize | NSYS_SM_CD_COMMON_TOC_SETC |
				  (dsize ? NSYS_SM_CD_COMMON_TOC_SETV : 0));
  
  delta = (NSYS_SM_CD_COMMON_TOC_SIZE + NSYS_SM_CD_COMMON_LENGTH_SIZE +
	   NSYS_SM_CD_COMMON_COUNT_SIZE + dsize);

  p[NSYS_SM_CD_COMMON_LOC_LENGTH] = delta;
  
  p[NSYS_SM_CD_COMMON_LOC_FIELDS]++;      /* increment COUNT */

  if (ndata != CSYS_MIDI_SYSTEM_SYSEX_END)
    {
      if (vdata != CSYS_MIDI_SYSTEM_SYSEX_END)
	p[NSYS_SM_CD_COMMON_LOC_FIELDS + 1] = ndata;
      else
      	p[NSYS_SM_CD_COMMON_LOC_FIELDS + 1] = ndata | NSYS_SM_CX_DATA_SETEND;
    }

  if (vdata != CSYS_MIDI_SYSTEM_SYSEX_END)
    p[NSYS_SM_CD_COMMON_LOC_FIELDS + 2] = vdata | NSYS_SM_CX_DATA_SETEND;;

  nsys_netout_jsend_slist[nsys_netout_jsend_slist_size++] = 
    &(jsys->chapterd_front[NSYS_SM_CD_LOC_TOC]);
  nsys_netout_jsend_slist[nsys_netout_jsend_slist_size++] = 
    &(p[NSYS_SM_CD_COMMON_LOC_TOC]);

  sc_seqnum = ((cmd == CSYS_MIDI_SYSTEM_UNUSED1) ? 
	       jsys->chapterd_scj_seqnum : jsys->chapterd_sck_seqnum);

  if (jsys->chapterd_seqnum && sc_seqnum)
    {
      delta -= ((cmd == CSYS_MIDI_SYSTEM_UNUSED1) ? 
		jsys->chapterd_scj_size : jsys->chapterd_sck_size);
    }
  else
    {
      if (!(jsys->chapterd_seqnum))
	{
	  jsys->sheader[NSYS_SM_SH_LOC_FLAGS] |= NSYS_SM_SH_SETD;
	  jsys->chapterd_front[NSYS_SM_CD_LOC_TOC] = 0;
	  jsys->slen += NSYS_SM_CD_SIZE_TOC;
	  jsys->chapterd_front_size = NSYS_SM_CD_SIZE_TOC;
	}

      jsys->chapterd_front[NSYS_SM_CD_LOC_TOC] |= 
	(cmd == CSYS_MIDI_SYSTEM_UNUSED1) ? NSYS_SM_CD_TOC_SETJ : NSYS_SM_CD_TOC_SETK;
    }

  if (delta)
    {
      if (cmd == CSYS_MIDI_SYSTEM_UNUSED1)
	jsys->chapterd_scj_size = p[NSYS_SM_CD_COMMON_LOC_LENGTH];
      else
	jsys->chapterd_sck_size = p[NSYS_SM_CD_COMMON_LOC_LENGTH];

      if ((jsys->slen += delta) <= NSYS_SM_SH_LSBMASK)
	jsys->sheader[NSYS_SM_SH_LOC_LENLSB] = (unsigned char)(jsys->slen);
      else
	{
	  jsys->sheader[NSYS_SM_SH_LOC_FLAGS] &= ~(NSYS_SM_SH_MSBMASK);
	  jsys->sheader[NSYS_SM_SH_LOC_FLAGS] |= (unsigned char)(jsys->slen >> 8);
	  jsys->sheader[NSYS_SM_SH_LOC_LENLSB] = 
	    (unsigned char)(NSYS_SM_SH_LSBMASK & jsys->slen);  
	}
    }

  jsys->chapterd_front[NSYS_SM_CD_LOC_TOC] &= NSYS_SM_CLRS;
  jsys->sheader[NSYS_SM_SH_LOC_FLAGS] &= NSYS_SM_CLRS;

  jsys->chapterd_seqnum = nsys_netout_seqnum;

  if (cmd == CSYS_MIDI_SYSTEM_UNUSED1)
    jsys->chapterd_scj_seqnum = nsys_netout_seqnum;
  else
    jsys->chapterd_sck_seqnum = nsys_netout_seqnum;

  nsys_netout_jsend_guard_time = nsys_netout_jsend_guard_mintime;
  nsys_netout_jsend_guard_next = (nsys_netout_jsend_guard_mintime << 
			     (nsys_feclevel == NSYS_SM_FEC_MINIMAL));
}


/****************************************************************/
/*         add undefined RealTime to System Simple chapter      */
/****************************************************************/

void nsys_netout_journal_addrt(unsigned char cmd)

{
  nsys_netout_jsend_system_state * jsys;
  unsigned long rt_seqnum;
  unsigned char * p;

  jsys = &(nsys_netout_jsend_system);

  p = (cmd == CSYS_MIDI_SYSTEM_TICK) ? jsys->chapterd_rty : jsys->chapterd_rtz;
  rt_seqnum = ((cmd == CSYS_MIDI_SYSTEM_TICK) ? 
	       jsys->chapterd_rty_seqnum : jsys->chapterd_rtz_seqnum);

  if (!rt_seqnum)
    p[NSYS_SM_CD_REALTIME_LOC_TOC] = 
      (NSYS_SM_CD_REALTIME_TOC_SETC | NSYS_SM_CD_REALTIME_SIZE);

  p[NSYS_SM_CD_REALTIME_LOC_TOC] &= NSYS_SM_CLRS; 
  p[NSYS_SM_CD_REALTIME_LOC_FIELDS]++;

  nsys_netout_jsend_slist[nsys_netout_jsend_slist_size++] = 
    &(jsys->chapterd_front[NSYS_SM_CD_LOC_TOC]);
  nsys_netout_jsend_slist[nsys_netout_jsend_slist_size++] = 
    &(p[NSYS_SM_CD_REALTIME_LOC_TOC]);

  if (jsys->chapterd_seqnum && rt_seqnum)
    {
      jsys->chapterd_front[NSYS_SM_CD_LOC_TOC] &= NSYS_SM_CLRS;
      jsys->sheader[NSYS_SM_SH_LOC_FLAGS] &= NSYS_SM_CLRS;
    }
  else
    {
      jsys->sheader[NSYS_SM_SH_LOC_FLAGS] &= ~(NSYS_SM_SH_MSBMASK | NSYS_SM_SETS);

      if (!(jsys->chapterd_seqnum))
	{
	  jsys->sheader[NSYS_SM_SH_LOC_FLAGS] |= NSYS_SM_SH_SETD;
	  jsys->chapterd_front[NSYS_SM_CD_LOC_TOC] = 0;
	  jsys->slen += NSYS_SM_CD_SIZE_TOC;
	  jsys->chapterd_front_size = NSYS_SM_CD_SIZE_TOC;
	}

      jsys->chapterd_front[NSYS_SM_CD_LOC_TOC] |= 
	(cmd == CSYS_MIDI_SYSTEM_TICK) ? NSYS_SM_CD_TOC_SETY : NSYS_SM_CD_TOC_SETZ;

      if ((jsys->slen += NSYS_SM_CD_REALTIME_SIZE) <= NSYS_SM_SH_LSBMASK)
	jsys->sheader[NSYS_SM_SH_LOC_LENLSB] = (unsigned char)(jsys->slen);
      else
	{
	  jsys->sheader[NSYS_SM_SH_LOC_FLAGS] |= (unsigned char)(jsys->slen >> 8);
	  jsys->sheader[NSYS_SM_SH_LOC_LENLSB] = 
	    (unsigned char)(NSYS_SM_SH_LSBMASK & jsys->slen);  
	}
    }

  jsys->chapterd_seqnum = nsys_netout_seqnum;

  if (cmd == CSYS_MIDI_SYSTEM_TICK)
    jsys->chapterd_rty_seqnum = nsys_netout_seqnum;
  else
    jsys->chapterd_rtz_seqnum = nsys_netout_seqnum;

  nsys_netout_jsend_guard_time = nsys_netout_jsend_guard_mintime;
  nsys_netout_jsend_guard_next = (nsys_netout_jsend_guard_mintime << 
			     (nsys_feclevel == NSYS_SM_FEC_MINIMAL));
}

/****************************************************************/
/*                  add System Active Sense chapter             */
/****************************************************************/

void nsys_netout_journal_addsense()

{
  nsys_netout_jsend_system_state * jsys;

  jsys = &(nsys_netout_jsend_system);

  jsys->chapterv[NSYS_SM_CV_LOC_COUNT]++;
  jsys->chapterv[NSYS_SM_CV_LOC_COUNT] &= NSYS_SM_CLRS;

  nsys_netout_jsend_slist[nsys_netout_jsend_slist_size++] = 
    &(jsys->chapterv[NSYS_SM_CV_LOC_COUNT]);

  if (jsys->chapterv_seqnum)
    jsys->sheader[NSYS_SM_SH_LOC_FLAGS] &= NSYS_SM_CLRS;
  else
    {
      jsys->sheader[NSYS_SM_SH_LOC_FLAGS] &= ~(NSYS_SM_SH_MSBMASK |
					       NSYS_SM_SETS);

      if ((jsys->slen += NSYS_SM_CV_SIZE) <= NSYS_SM_SH_LSBMASK)
	{
	  jsys->sheader[NSYS_SM_SH_LOC_FLAGS] |= NSYS_SM_SH_SETV;
	  jsys->sheader[NSYS_SM_SH_LOC_LENLSB] = (unsigned char)(jsys->slen);
	}
      else
	{
	  jsys->sheader[NSYS_SM_SH_LOC_FLAGS] |= 
	    (NSYS_SM_SH_SETV | ((unsigned char)(jsys->slen >> 8)));
	  jsys->sheader[NSYS_SM_SH_LOC_LENLSB] = 
	    (unsigned char)(NSYS_SM_SH_LSBMASK & jsys->slen);  
	}
    }

  jsys->chapterv_seqnum = nsys_netout_seqnum;

  nsys_netout_jsend_guard_time = nsys_netout_jsend_guard_mintime;
  nsys_netout_jsend_guard_next = (nsys_netout_jsend_guard_mintime << 
			     (nsys_feclevel == NSYS_SM_FEC_MINIMAL));
}


/****************************************************************/
/*            add command to System sequencer chapter           */
/****************************************************************/

void nsys_netout_journal_addsequence(unsigned char cmd, unsigned char ndata,
				     unsigned char vdata)

{
  nsys_netout_jsend_system_state * jsys;
  long song_pp, slen_update;

  jsys = &(nsys_netout_jsend_system);
  slen_update = 0;

  nsys_netout_jsend_slist[nsys_netout_jsend_slist_size++] = 
    &(jsys->chapterq[NSYS_SM_CQ_LOC_HDR]);

  if (jsys->chapterq_seqnum == 0)
    {
      if (jsys->chapterq_size == 0)
	jsys->chapterq_size = NSYS_SM_CQ_SIZE_HDR;
      jsys->slen += jsys->chapterq_size;
      slen_update = 1;
    }

  jsys->chapterq[NSYS_SM_CQ_LOC_HDR] &= NSYS_SM_CLRS;

  /* caller pre-filters stopped CLOCKs, redundant CONTINUE and STOPs */

  switch (cmd) {
  case CSYS_MIDI_SYSTEM_CLOCK:  
    if (!(jsys->chapterq[NSYS_SM_CQ_LOC_HDR] & NSYS_SM_CQ_HDR_CHKD))
      jsys->chapterq[NSYS_SM_CQ_LOC_HDR] |= NSYS_SM_CQ_HDR_SETD;
    else
      {
	if (!(jsys->chapterq[NSYS_SM_CQ_LOC_HDR] & NSYS_SM_CQ_HDR_CHKC))
	  {
	    jsys->chapterq[NSYS_SM_CQ_LOC_HDR] |= NSYS_SM_CQ_HDR_SETC;
	    jsys->chapterq[NSYS_SM_CQ_LOC_FIELDS + 1] = 0;
	    jsys->chapterq[NSYS_SM_CQ_LOC_FIELDS] = 0;
	    jsys->chapterq[NSYS_SM_CQ_LOC_HDR] &= ~(NSYS_SM_CQ_TOP_MASK);
	    jsys->chapterq_size += NSYS_SM_CQ_SIZE_CLOCK;
	    jsys->slen += NSYS_SM_CQ_SIZE_CLOCK;
	    slen_update = 1;
	  }
	if (!(++(jsys->chapterq[NSYS_SM_CQ_LOC_FIELDS + 1])))
	  if (!(++(jsys->chapterq[NSYS_SM_CQ_LOC_FIELDS])))
	    {
	      if ((jsys->chapterq[NSYS_SM_CQ_LOC_HDR] & NSYS_SM_CQ_TOP_MASK) !=
		  NSYS_SM_CQ_TOP_MASK)
		jsys->chapterq[NSYS_SM_CQ_LOC_HDR]++;
	      else
		jsys->chapterq[NSYS_SM_CQ_LOC_HDR] &= ~(NSYS_SM_CQ_TOP_MASK);
	    }
      }
    break;
  case CSYS_MIDI_SYSTEM_START:
    if ((jsys->chapterq[NSYS_SM_CQ_LOC_HDR] & NSYS_SM_CQ_HDR_CHKC))
      {
	jsys->chapterq_size -= NSYS_SM_CQ_SIZE_CLOCK;
	jsys->slen -= NSYS_SM_CQ_SIZE_CLOCK;
	slen_update = 1;
      }
    jsys->chapterq[NSYS_SM_CQ_LOC_HDR] = NSYS_SM_CQ_HDR_SETN;  /* C = D = 0 */
    break;
  case CSYS_MIDI_SYSTEM_CONTINUE:
    jsys->chapterq[NSYS_SM_CQ_LOC_HDR] |= NSYS_SM_CQ_HDR_SETN;
    if (!(jsys->chapterq[NSYS_SM_CQ_LOC_HDR] & NSYS_SM_CQ_HDR_CHKC))
      {
	jsys->chapterq[NSYS_SM_CQ_LOC_HDR] |= NSYS_SM_CQ_HDR_SETC;
	jsys->chapterq[NSYS_SM_CQ_LOC_FIELDS + 1] = 0;
	jsys->chapterq[NSYS_SM_CQ_LOC_FIELDS] = 0;
	jsys->chapterq[NSYS_SM_CQ_LOC_HDR] &= ~(NSYS_SM_CQ_TOP_MASK);
	jsys->chapterq_size += NSYS_SM_CQ_SIZE_CLOCK;
	jsys->slen += NSYS_SM_CQ_SIZE_CLOCK;
	slen_update = 1;
      }
    if (jsys->chapterq[NSYS_SM_CQ_LOC_HDR] & NSYS_SM_CQ_HDR_CHKD)
      {
	if (!(++(jsys->chapterq[NSYS_SM_CQ_LOC_FIELDS + 1])))
	  if (!(++(jsys->chapterq[NSYS_SM_CQ_LOC_FIELDS])))
	    {
	      if ((jsys->chapterq[NSYS_SM_CQ_LOC_HDR] & NSYS_SM_CQ_TOP_MASK) !=
		  NSYS_SM_CQ_TOP_MASK)
		jsys->chapterq[NSYS_SM_CQ_LOC_HDR]++;
	      else
		jsys->chapterq[NSYS_SM_CQ_LOC_HDR] &= ~(NSYS_SM_CQ_TOP_MASK);
	    }
	jsys->chapterq[NSYS_SM_CQ_LOC_HDR] &= NSYS_SM_CQ_HDR_CLRD;
      }
    break;
  case CSYS_MIDI_SYSTEM_STOP:
    jsys->chapterq[NSYS_SM_CQ_LOC_HDR] &= NSYS_SM_CQ_HDR_CLRN;
    break;
  case CSYS_MIDI_SYSTEM_SONG_PP:
    if ((song_pp = 6*((vdata << 7) + ndata)))
      {
	if (!(jsys->chapterq[NSYS_SM_CQ_LOC_HDR] & NSYS_SM_CQ_HDR_CHKC))
	  {
	    jsys->chapterq[NSYS_SM_CQ_LOC_HDR] |= NSYS_SM_CQ_HDR_SETC;
	    jsys->chapterq_size += NSYS_SM_CQ_SIZE_CLOCK;
	    jsys->slen += NSYS_SM_CQ_SIZE_CLOCK;
	    slen_update = 1;
	  }
	jsys->chapterq[NSYS_SM_CQ_LOC_FIELDS + 1] = 
	  (unsigned char) (song_pp & 0x000000FF);
	jsys->chapterq[NSYS_SM_CQ_LOC_FIELDS] = 
	  (unsigned char) ((song_pp >> 8) & 0x000000FF);
	jsys->chapterq[NSYS_SM_CQ_LOC_HDR] &= ~(NSYS_SM_CQ_TOP_MASK);
	if (song_pp > NSYS_SM_CQ_BOTTOM_MASK)
	  jsys->chapterq[NSYS_SM_CQ_LOC_HDR] |= 0x01;  /* song_pp range limit */
      }
    else
      {
	if ((jsys->chapterq[NSYS_SM_CQ_LOC_HDR] & NSYS_SM_CQ_HDR_CHKC))
	  {
	    jsys->chapterq[NSYS_SM_CQ_LOC_HDR] &= NSYS_SM_CQ_HDR_CLRC;
	    jsys->chapterq_size -= NSYS_SM_CQ_SIZE_CLOCK;
	    jsys->slen -= NSYS_SM_CQ_SIZE_CLOCK;
	    slen_update = 1;
	  }
      }
    jsys->chapterq[NSYS_SM_CQ_LOC_HDR] &= NSYS_SM_CQ_HDR_CLRD;
    break;
  }

  if (slen_update)
    {
      jsys->sheader[NSYS_SM_SH_LOC_FLAGS] &= ~(NSYS_SM_SH_MSBMASK);
      if (jsys->slen <= NSYS_SM_SH_LSBMASK)
	jsys->sheader[NSYS_SM_SH_LOC_LENLSB] = (unsigned char)(jsys->slen);
      else
	{
	  jsys->sheader[NSYS_SM_SH_LOC_LENLSB] =
	    (unsigned char)(jsys->slen & NSYS_SM_SH_LSBMASK);
	  jsys->sheader[NSYS_SM_SH_LOC_FLAGS] |= 
	    (unsigned char)((jsys->slen >> 8) & NSYS_SM_SH_MSBMASK);
	}
    }

  if (jsys->chapterq_seqnum == 0)
    jsys->sheader[NSYS_SM_SH_LOC_FLAGS] |= NSYS_SM_SH_SETQ;

  jsys->sheader[NSYS_SM_SH_LOC_FLAGS] &= NSYS_SM_CLRS;
  jsys->chapterq_seqnum = nsys_netout_seqnum;

  nsys_netout_jsend_guard_time = nsys_netout_jsend_guard_mintime;
  nsys_netout_jsend_guard_next = (nsys_netout_jsend_guard_mintime << 
			     (nsys_feclevel == NSYS_SM_FEC_MINIMAL));
}

/****************************************************************/
/*         add command to System MIDI Time Code chapter         */
/****************************************************************/

void nsys_netout_journal_addtimecode(unsigned char ndata)

{
  nsys_netout_jsend_system_state * jsys;
  unsigned char point, idnum, direction, partial, complete;
  unsigned char frames, seconds, minutes, hours, type, typeframe;
  unsigned char * p;
  int slen_update;

  jsys = &(nsys_netout_jsend_system);
  slen_update = 0;

  nsys_netout_jsend_slist[nsys_netout_jsend_slist_size++] = 
    &(jsys->chapterf[NSYS_SM_CF_LOC_HDR]);

  if (jsys->chapterf_seqnum == 0)
    {
      if (jsys->chapterf_size == 0)
	jsys->chapterf_size = NSYS_SM_CF_SIZE_HDR;
      jsys->slen += jsys->chapterf_size;
      slen_update = 1;
    }

  jsys->chapterf[NSYS_SM_CF_LOC_HDR] &= NSYS_SM_CLRS;

  point = jsys->chapterf_point;
  jsys->chapterf_point = idnum = (ndata & NSYS_SM_CF_IDNUM_MASK) >> 4;

  complete = jsys->chapterf[NSYS_SM_CF_LOC_HDR] & NSYS_SM_CF_HDR_CHKC;
  partial = jsys->chapterf[NSYS_SM_CF_LOC_HDR] & NSYS_SM_CF_HDR_CHKP;
  direction = jsys->chapterf[NSYS_SM_CF_LOC_HDR] & NSYS_SM_CF_HDR_CHKD;

  do {

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

	      if (partial)
		{
		  jsys->chapterf[NSYS_SM_CF_LOC_HDR] &= NSYS_SM_CF_POINT_CLR;
		  jsys->chapterf[NSYS_SM_CF_LOC_HDR] |= idnum;
		  jsys->chapterfc_seqnum = nsys_netout_seqnum;

		  jsys->chapterf[NSYS_SM_CF_LOC_HDR] &= NSYS_SM_CF_HDR_CLRP;
		  jsys->chapterf[NSYS_SM_CF_LOC_HDR] |= 
		    (NSYS_SM_CF_HDR_SETC | NSYS_SM_CF_HDR_SETQ);

		  /* the old partial becomes the new complete */

		  if (complete)
		    {
		      memcpy(&(jsys->chapterf[NSYS_SM_CF_LOC_FIELDS]),
			     &(jsys->chapterf[NSYS_SM_CF_LOC_FIELDS + 
					      NSYS_SM_CF_SIZE_COMPLETE]),
			     NSYS_SM_CF_SIZE_PARTIAL);
		      
		      jsys->chapterf_size -= NSYS_SM_CF_SIZE_PARTIAL;
		      jsys->slen -= NSYS_SM_CF_SIZE_PARTIAL;
		      slen_update = 1;
		    }

		  /* write the new complete's MT0 or MT7 */

		  if (!direction)
		    {
		      p = &(jsys->chapterf[NSYS_SM_CF_LOC_FIELDS + 
					   NSYS_SM_CF_QF_LOC_HR_MSN]);
		      (*p) &= NSYS_SM_CF_ODD_CLR;
		      (*p) |= (ndata & NSYS_SM_CF_PAYLOAD_MASK);
		    }
		  else
		    {
		      p = &(jsys->chapterf[NSYS_SM_CF_LOC_FIELDS + 
					   NSYS_SM_CF_QF_LOC_FR_LSN]);
		      (*p) &= NSYS_SM_CF_EVEN_CLR;
		      (*p) |= (ndata << 4);
		    }

		  /* for forward tape, adjust complete by two frames */
	      
		  if (!direction)
		    {
		      p = &(jsys->chapterf[NSYS_SM_CF_LOC_FIELDS]);
		      
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
	      break;      /* from switch, for logic for ending a frame */
	    }

	  /* handle QF commands that start a frame: make partial space, fall through */

	  if (!partial)
	    {
	      if (complete)
		p = &(jsys->chapterf[NSYS_SM_CF_LOC_FIELDS + NSYS_SM_CF_SIZE_COMPLETE]);
	      else
		p = &(jsys->chapterf[NSYS_SM_CF_LOC_FIELDS]);
	      memset(p, 0, NSYS_SM_CF_SIZE_PARTIAL);

	      jsys->chapterf[NSYS_SM_CF_LOC_HDR] |= NSYS_SM_CF_HDR_SETP;
	      jsys->chapterf_size += NSYS_SM_CF_SIZE_PARTIAL;
	      jsys->slen += NSYS_SM_CF_SIZE_PARTIAL;
	      partial = slen_update = 1;
	    }
	default:    /* handle all expected non-frame-ending MT values */
	  if (partial)
	    {
	      jsys->chapterf[NSYS_SM_CF_LOC_HDR] &= NSYS_SM_CF_POINT_CLR;
	      jsys->chapterf[NSYS_SM_CF_LOC_HDR] |= idnum;

	      if (complete)
		p = &(jsys->chapterf[NSYS_SM_CF_LOC_FIELDS + 
				     NSYS_SM_CF_SIZE_COMPLETE + (idnum >> 1)]);
	      else
		p = &(jsys->chapterf[NSYS_SM_CF_LOC_FIELDS + (idnum >> 1)]);

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
	}

	break;   /* from do/while, for QF commands with expected values */
      }

    /* next, handle tape direction change that happens in a legal way */

    if ((idnum == NSYS_SM_CF_IDNUM_FR_LSN) || (idnum == NSYS_SM_CF_IDNUM_HR_MSN))
      {
	jsys->chapterf[NSYS_SM_CF_LOC_HDR] &= NSYS_SM_CF_POINT_CLR;
	jsys->chapterf[NSYS_SM_CF_LOC_HDR] |= idnum;

	if (idnum == NSYS_SM_CF_IDNUM_FR_LSN)
	  jsys->chapterf[NSYS_SM_CF_LOC_HDR] &= NSYS_SM_CF_HDR_CLRD;
	else
	  jsys->chapterf[NSYS_SM_CF_LOC_HDR] |= NSYS_SM_CF_HDR_SETD;

	if (!partial)
	  {
	    jsys->chapterf[NSYS_SM_CF_LOC_HDR] |= NSYS_SM_CF_HDR_SETP;
	    jsys->chapterf_size += NSYS_SM_CF_SIZE_PARTIAL;
	    jsys->slen += NSYS_SM_CF_SIZE_PARTIAL;
	    slen_update = 1;
	  }

	if (complete)
	  p = &(jsys->chapterf[NSYS_SM_CF_LOC_FIELDS + NSYS_SM_CF_SIZE_COMPLETE]);
	else
	  p = &(jsys->chapterf[NSYS_SM_CF_LOC_FIELDS]);

	memset(p, 0, NSYS_SM_CF_SIZE_PARTIAL);

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

	break;  /* from do/while, for tape movement reset */
      }

    /* handle "wrong direction guess" for an earlier unexpected QF command */

    if (!direction && (((point - 1) & NSYS_SM_CF_POINT_MASK) == idnum))
      {
	jsys->chapterf[NSYS_SM_CF_LOC_HDR] &= NSYS_SM_CF_POINT_CLR;
	jsys->chapterf[NSYS_SM_CF_LOC_HDR] |= NSYS_SM_CF_HDR_SETD;
	break;  /* from do/while */
      }

    /* handle unexpected QF commands */

    jsys->chapterf[NSYS_SM_CF_LOC_HDR] &= NSYS_SM_CF_HDR_CLRD;

    if (partial)
      {
	jsys->chapterf[NSYS_SM_CF_LOC_HDR] &= NSYS_SM_CF_HDR_CLRP;
	jsys->chapterf[NSYS_SM_CF_LOC_HDR] |= NSYS_SM_CF_POINT_MASK;
	jsys->chapterf_size -= NSYS_SM_CF_SIZE_PARTIAL;
	jsys->slen -= NSYS_SM_CF_SIZE_PARTIAL;
	slen_update = 1;
      }

  } while (0);

  /* housekeeping before exit */
  
  if (slen_update)
    {
      jsys->sheader[NSYS_SM_SH_LOC_FLAGS] &= ~(NSYS_SM_SH_MSBMASK);
      if (jsys->slen <= NSYS_SM_SH_LSBMASK)
	jsys->sheader[NSYS_SM_SH_LOC_LENLSB] = (unsigned char)(jsys->slen);
      else
	{
	  jsys->sheader[NSYS_SM_SH_LOC_LENLSB] =
	    (unsigned char)(jsys->slen & NSYS_SM_SH_LSBMASK);
	  jsys->sheader[NSYS_SM_SH_LOC_FLAGS] |= 
	    (unsigned char)((jsys->slen >> 8) & NSYS_SM_SH_MSBMASK);
	}
    }

  if (jsys->chapterf_seqnum == 0)
    jsys->sheader[NSYS_SM_SH_LOC_FLAGS] |= NSYS_SM_SH_SETF;

  jsys->sheader[NSYS_SM_SH_LOC_FLAGS] &= NSYS_SM_CLRS;
  jsys->chapterf_seqnum = nsys_netout_seqnum;

  nsys_netout_jsend_guard_time = nsys_netout_jsend_guard_mintime;
  nsys_netout_jsend_guard_next = (nsys_netout_jsend_guard_mintime << 
			     (nsys_feclevel == NSYS_SM_FEC_MINIMAL));
}

/****************************************************************/
/*           add command to System Exclusive chapter            */
/****************************************************************/

void nsys_netout_journal_addsysex(unsigned char cmd, 
				  unsigned char ndata, 
				  unsigned char vdata)

{
  nsys_netout_jsend_system_state * jsys;
  nsys_netout_jsend_xstack_element * eptr;
  int sincr, i;

  jsys = &(nsys_netout_jsend_system);

  if ((cmd != CSYS_MIDI_GMRESET) && (cmd != CSYS_MIDI_MVOLUME))
    {
      nsys_netout_jsend_header[NSYS_SM_JH_LOC_FLAGS] |= NSYS_SM_SETS;
      return;
    }

  if (cmd == CSYS_MIDI_GMRESET) 
    nsys_netout_journal_clear_active(cmd);

  nsys_netout_jsend_header[NSYS_SM_JH_LOC_FLAGS] &= NSYS_SM_CLRS;
  sincr = 0;

  if (jsys->sheader_seqnum == 0)
    {
      if (nsys_netout_jsend_channel_size)
	nsys_netout_jsend_header[NSYS_SM_JH_LOC_FLAGS] |= NSYS_SM_JH_SETY;
      else
	nsys_netout_jsend_header[NSYS_SM_JH_LOC_FLAGS] = NSYS_SM_JH_SETY;
      
      jsys->slen = NSYS_SM_SH_SIZE;
    }
  
  jsys->sheader_seqnum = nsys_netout_seqnum;
  nsys_netout_jsend_slist[nsys_netout_jsend_slist_size++] = 
    &(jsys->sheader[NSYS_SM_SH_LOC_FLAGS]);

  switch (cmd) {
  case CSYS_MIDI_GMRESET:
    if (ndata == NSYS_SM_CX_GMRESET_ONVAL)
      eptr = jsys->chapterx_gmreset_on;
    else
      eptr = jsys->chapterx_gmreset_off;
    break;
  case CSYS_MIDI_MVOLUME:
    eptr = jsys->chapterx_mvolume;
    break;
  default:
    eptr = NULL;  /* should never execute -- gcc warning supression */
  }

  if (eptr)       /* move existing recency log to the top of the stack */
    {
      if (eptr->index != jsys->chapterx_stacklen - 1)
	{
	  for (i = eptr->index + 1; i < jsys->chapterx_stacklen; i++)
	    {
	      jsys->chapterx_stack[i]->index = i - 1;
	      jsys->chapterx_stack[i - 1] = jsys->chapterx_stack[i];
	    }
	  jsys->chapterx_stack[i - 1] = eptr;
	  eptr->index = i - 1;
	}
      eptr->log[0] &= NSYS_SM_CLRS;
    }
  else         /* add a new recency log to the top of the stack */
    {
      eptr = nsys_netout_jsend_xstackfree;
      nsys_netout_jsend_xstackfree = nsys_netout_jsend_xstackfree->next;
      eptr->index = jsys->chapterx_stacklen;
      eptr->next = NULL;

      switch (cmd) {
      case CSYS_MIDI_GMRESET:
	if (ndata == NSYS_SM_CX_GMRESET_ONVAL)
	  {
	    jsys->chapterx_gmreset_on = eptr;
	    eptr->cmdptr = &(jsys->chapterx_gmreset_on);
	  }
	else
	  {
	    jsys->chapterx_gmreset_off = eptr;
	    eptr->cmdptr = &(jsys->chapterx_gmreset_off);
	  }
	sincr = eptr->size = NSYS_SM_CX_SIZE_GMRESET;
	eptr->log[0] = (NSYS_SM_CX_HDR_SETT | NSYS_SM_CX_HDR_SETD |
			NSYS_SM_CX_STA_NORMAL);
	eptr->log[2] = 0x7E;
	eptr->log[3] = 0x7F;
	eptr->log[4] = 0x09;
	break;
      case CSYS_MIDI_MVOLUME:
	jsys->chapterx_mvolume = eptr;
	eptr->cmdptr = &(jsys->chapterx_mvolume);
	sincr = eptr->size = NSYS_SM_CX_SIZE_MVOLUME;
	eptr->log[0] = (NSYS_SM_CX_HDR_SETT | NSYS_SM_CX_HDR_SETD |
			NSYS_SM_CX_STA_NORMAL);
	eptr->log[2] = 0x7F;
	eptr->log[3] = 0x7F;
	eptr->log[4] = 0x04;
	eptr->log[5] = 0x01;
	break;
      }
      jsys->chapterx_stack[(jsys->chapterx_stacklen)++] = eptr;
    }

  /* work in common for new and moved stack tops */

  eptr->seqnum = nsys_netout_seqnum;

  switch (cmd) {
  case CSYS_MIDI_GMRESET:
    if (ndata == NSYS_SM_CX_GMRESET_ONVAL)
      eptr->log[1] = ++(jsys->chapterx_gmreset_on_count);
    else
      eptr->log[1] = ++(jsys->chapterx_gmreset_off_count);
    eptr->log[5] = ndata | NSYS_SM_CX_DATA_SETEND;
    break;
  case CSYS_MIDI_MVOLUME:
    eptr->log[1] = ++(jsys->chapterx_mvolume_count);
    eptr->log[6] = ndata;
    eptr->log[7] = vdata | NSYS_SM_CX_DATA_SETEND;
    break;
  }

  jsys->chapterx_stack[0]->log[0] &= NSYS_SM_CLRS;

  if (sincr == 0)
    jsys->sheader[NSYS_SM_SH_LOC_FLAGS] &= NSYS_SM_CLRS;
  else
    {
      jsys->sheader[NSYS_SM_SH_LOC_FLAGS] &= ~(NSYS_SM_SH_MSBMASK |
					       NSYS_SM_SETS);

      if ((jsys->slen += sincr) <= NSYS_SM_SH_LSBMASK)
	{
	  jsys->sheader[NSYS_SM_SH_LOC_FLAGS] |= NSYS_SM_SH_SETX;
	  jsys->sheader[NSYS_SM_SH_LOC_LENLSB] = (unsigned char)(jsys->slen);
	}
      else
	{
	  jsys->sheader[NSYS_SM_SH_LOC_FLAGS] |= 
	    (NSYS_SM_SH_SETX | ((unsigned char)(jsys->slen >> 8)));
	  jsys->sheader[NSYS_SM_SH_LOC_LENLSB] = 
	    (unsigned char)(NSYS_SM_SH_LSBMASK & jsys->slen);  
	}
    }

  jsys->chapterx_seqnum = nsys_netout_seqnum;

  nsys_netout_jsend_guard_time = nsys_netout_jsend_guard_mintime;
  nsys_netout_jsend_guard_next = (nsys_netout_jsend_guard_mintime << 
			     (nsys_feclevel == NSYS_SM_FEC_MINIMAL));
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*      second-level journal functions: trimstate chapters      */
/*______________________________________________________________*/

/****************************************************************/
/*                  trim entire chapter                         */
/****************************************************************/

void nsys_netout_journal_trimchapter(nsys_netout_jsend_state * jsend, 
				     int channel)

{
  int i, j, k;

  memset(jsend->cheader, 0, NSYS_SM_CH_SIZE);

  jsend->cheader_seqnum  = 0;
  jsend->chapterp_seqnum = 0;
  jsend->chapterc_seqnum = 0;
  jsend->chapterm_seqnum = 0;
  jsend->chapterw_seqnum = 0;
  jsend->chaptern_seqnum = 0;
  jsend->chaptere_seqnum = 0;
  jsend->chaptert_seqnum = 0;
  jsend->chaptera_seqnum = 0;

  jsend->chapterc_sset = 0;
  jsend->chapterm_sset = 0;
  jsend->chaptern_sset = 0;
  jsend->chaptere_sset = 0;
  jsend->chaptera_sset = 0;

  jsend->chaptern_timernum = 0;

  /*******************************************/
  /* clear seqnums of journal chapter arrays */
  /*******************************************/

  while (jsend->chapterc_size > NSYS_SM_CC_HDRSIZE)
    {
      jsend->chapterc_size -= NSYS_SM_CC_LOGSIZE;
      i = jsend->chapterc[jsend->chapterc_size];
      jsend->chapterc_seqarray[NSYS_SM_CLRS & i] = 0;
    }

  jsend->chapterm_dummy_seqnum = 0;
  for (j = 0; j < NSYS_SM_CM_ARRAYSIZE; j++)
    jsend->chapterm_seqarray[j] = 0;

  for (j = NSYS_SM_CN_HDRSIZE; j < jsend->chaptern_size;
       j += NSYS_SM_CN_LOGSIZE)
    {
      i = NSYS_SM_CLRS & jsend->chaptern[j];
      jsend->chaptern_seqarray[i] = 0;
    }

  i = (jsend->chapterb_low << NSYS_SM_CN_BFSHIFT);
  for (j = jsend->chapterb_low; j <= jsend->chapterb_high; j++)
    {
      if (jsend->chapterb[j])
	for (k = 128; k >= 1; k = (k >> 1))
	  {
	    if (jsend->chapterb[j] & k)
	      jsend->chaptern_seqarray[i] = 0;
	    i++;
	  }
      else
	i += 8;
    }

  while (jsend->chaptere_size > NSYS_SM_CE_HDRSIZE)
    {
      jsend->chaptere_size -= NSYS_SM_CE_LOGSIZE;
      i = NSYS_SM_CLRS & jsend->chaptere[jsend->chaptere_size];
      if (jsend->chaptere[jsend->chaptere_size + 1] & NSYS_SM_CE_CHKV)
	jsend->chapterev_seqarray[i] = 0;
      else
	jsend->chapterer_seqarray[i] = 0;
    }

  while (jsend->chaptera_size > NSYS_SM_CA_HDRSIZE)
    {
      jsend->chaptera_size -= NSYS_SM_CA_LOGSIZE;
      i = jsend->chaptera[jsend->chaptera_size];
      jsend->chaptera_seqarray[NSYS_SM_CLRS & i] = 0;
    }

  /************************/
  /* update channel array */
  /************************/

  if (channel < (--nsys_netout_jsend_channel_size))
    memmove(&(nsys_netout_jsend_channel[channel]), 
	    &(nsys_netout_jsend_channel[channel+1]),
	    sizeof(unsigned char)*(nsys_netout_jsend_channel_size -
				   channel));
}

/****************************************************************/
/*                 trim program change chapter                  */
/****************************************************************/

void nsys_netout_journal_trimprogram(nsys_netout_jsend_state * jsend) 
 
{
  jsend->chapterp_seqnum = 0;
  jsend->cheader[NSYS_SM_CH_LOC_TOC] &= NSYS_SM_CH_TOC_CLRP;
  jsend->clen -= NSYS_SM_CP_SIZE;
}


/****************************************************************/
/*                 trim entire controller chapter               */
/****************************************************************/

void nsys_netout_journal_trimallcontrol(nsys_netout_jsend_state * jsend) 
 
{
  int i, j;

  jsend->chapterc_seqnum = 0;
  jsend->cheader[NSYS_SM_CH_LOC_TOC] &= NSYS_SM_CH_TOC_CLRC;
  jsend->clen -= jsend->chapterc_size;
  j = NSYS_SM_CC_HDRSIZE;
  while (j < jsend->chapterc_size)
    {
      i = NSYS_SM_CLRS & jsend->chapterc[j];
      jsend->chapterc_seqarray[i] = 0;
      j += NSYS_SM_CC_LOGSIZE;
    }
  jsend->chapterc_sset = 0;
}

/****************************************************************/
/*                 partially trim controller chapter            */
/****************************************************************/

void nsys_netout_journal_trimpartcontrol(nsys_netout_jsend_state * jsend, 
					 unsigned long minseq)

{
  int i, j;

  j = NSYS_SM_CC_HDRSIZE;

  while (j < jsend->chapterc_size)
    {
      i = NSYS_SM_CLRS & jsend->chapterc[j];

      if (jsend->chapterc_seqarray[i] <= minseq)
	{
	  jsend->chapterc_seqarray[i] = 0;
	  jsend->clen -= NSYS_SM_CC_LOGSIZE;
	  jsend->chapterc_size -= NSYS_SM_CC_LOGSIZE;
	  jsend->chapterc[NSYS_SM_CC_LOC_LENGTH]--;
	  j += NSYS_SM_CC_LOGSIZE;
	}
      else
	break;
    }

  if (j == NSYS_SM_CC_HDRSIZE)
    return;

  memmove(&(jsend->chapterc[NSYS_SM_CC_HDRSIZE]), &(jsend->chapterc[j]),
	  jsend->chapterc_size - NSYS_SM_CC_HDRSIZE);

  j = NSYS_SM_CC_HDRSIZE;

  while (j < jsend->chapterc_size)
    {
      i = NSYS_SM_CLRS & jsend->chapterc[j];
      jsend->chapterc_logptr[i] = j;
      j += NSYS_SM_CC_LOGSIZE;
    }
}

/****************************************************************/
/*                 trim entire parameter chapter                */
/****************************************************************/

void nsys_netout_journal_trimallparams(nsys_netout_jsend_state * jsend) 
 
{
  int i;

  jsend->chapterm_seqnum = 0;
  jsend->cheader[NSYS_SM_CH_LOC_TOC] &= NSYS_SM_CH_TOC_CLRM;
  jsend->clen -= (jsend->chapterm_size + jsend->chapterl_size);
  jsend->chapterm_dummy_seqnum = 0;
  for (i = 0; i < NSYS_SM_CM_ARRAYSIZE; i++)
    jsend->chapterm_seqarray[i] = 0;
  jsend->chapterm_sset = 0;
}

/****************************************************************/
/*                 partially trim parameter chapter             */
/****************************************************************/

void nsys_netout_journal_trimpartparams(nsys_netout_jsend_state * jsend, 
					unsigned long minseq)

{
  unsigned char lsb, msb, nrpn, hbits, toc;
  int i, j, loglen, found;
  
  found = i = 0;

  while (i < jsend->chapterl_size)
    {
      msb = jsend->chapterl[i + NSYS_SM_CM_LOC_PNUMMSB] & NSYS_SM_CM_CLRQ;
      lsb = jsend->chapterl[i + NSYS_SM_CM_LOC_PNUMLSB] & NSYS_SM_CLRS;
      nrpn =  jsend->chapterl[i + NSYS_SM_CM_LOC_PNUMMSB] & NSYS_SM_CM_CHKQ;

      if (nrpn || (msb || (lsb >= NSYS_SM_CM_ARRAYSIZE)))
	{
	  if (jsend->chapterm_dummy_seqnum <= minseq)
	    jsend->chapterm_dummy_seqnum = 0;
	  else
	    {
	      found = 1;
	      break;
	    }
	}
      else
	{
	  if (jsend->chapterm_seqarray[lsb] <= minseq)
	    jsend->chapterm_seqarray[lsb] = 0;
	  else
	    {
	      found = 1;
	      break;
	    }
	}
      
      loglen = NSYS_SM_CM_LOGHDRSIZE;
      toc = jsend->chapterl[i + NSYS_SM_CM_LOC_TOC];

      if (toc & NSYS_SM_CM_TOC_CHKJ)
	loglen += NSYS_SM_CM_ENTRYMSB_SIZE;
      
      if (toc & NSYS_SM_CM_TOC_CHKK)
	loglen += NSYS_SM_CM_ENTRYLSB_SIZE;
      
      if (toc & NSYS_SM_CM_TOC_CHKL)
	loglen += NSYS_SM_CM_ABUTTON_SIZE;
      
      if (toc & NSYS_SM_CM_TOC_CHKM)
	loglen += NSYS_SM_CM_CBUTTON_SIZE;

      if (toc & NSYS_SM_CM_TOC_CHKN)
	loglen += NSYS_SM_CM_COUNT_SIZE;
      
      i += loglen;
    }

  if (!i)
    return;

  if (found)
    {
      memmove(jsend->chapterl, &(jsend->chapterl[i]), jsend->chapterl_size - i);

      if (jsend->chapterm_dummy_seqnum)
	jsend->chapterm_dummy_logptr -= i;

      for (j = 0; j < NSYS_SM_CM_ARRAYSIZE; j++)
	if (jsend->chapterm_seqarray[j])
	  jsend->chapterm_logptr[j] -= i;
    }

  jsend->clen -= i;
  jsend->chapterl_size -= i;

  hbits = ((NSYS_SM_CHKS | NSYS_SM_CM_HDR_CHKP | NSYS_SM_CM_HDR_CHKE)
	   & jsend->chapterm[NSYS_SM_CM_LOC_HDR]);

  *((unsigned short *) &(jsend->chapterm[NSYS_SM_CM_LOC_HDR])) = 
    htons((unsigned short)(jsend->chapterm_size + jsend->chapterl_size));

  jsend->chapterm[NSYS_SM_CM_LOC_HDR] |= hbits;

}

/****************************************************************/
/*                 trim pitch wheel chapter                  */
/****************************************************************/

void nsys_netout_journal_trimpwheel(nsys_netout_jsend_state * jsend) 
 
{
  jsend->chapterw_seqnum = 0;
  jsend->cheader[NSYS_SM_CH_LOC_TOC] &= NSYS_SM_CH_TOC_CLRW;
  jsend->clen -= NSYS_SM_CW_SIZE;
}

/****************************************************************/
/*                 trim entire note chapter                     */
/****************************************************************/

void nsys_netout_journal_trimallnote(nsys_netout_jsend_state * jsend) 
 
{
  int i, j, k;

  jsend->chaptern_seqnum = 0;
  jsend->cheader[NSYS_SM_CH_LOC_TOC] &= NSYS_SM_CH_TOC_CLRN;
  jsend->clen -= (jsend->chaptern_size + jsend->chapterb_size);
  
  for (j = NSYS_SM_CN_HDRSIZE; j < jsend->chaptern_size;
       j += NSYS_SM_CN_LOGSIZE)
    {
      i = NSYS_SM_CLRS & jsend->chaptern[j];
      jsend->chaptern_seqarray[i] = 0;
    }
  i = (jsend->chapterb_low << NSYS_SM_CN_BFSHIFT);
  for (j = jsend->chapterb_low; j <= jsend->chapterb_high; j++)
    {
      if (jsend->chapterb[j])
	for (k = 128; k >= 1; k = (k >> 1))
	  {
	    if (jsend->chapterb[j] & k) 
	      jsend->chaptern_seqarray[i] = 0;
	    i++;
	  }
      else
	i += 8;
    }
  jsend->chaptern_timernum = 0;
  jsend->chaptern_sset = 0;
}

/****************************************************************/
/*                 trim partial note chapter                    */
/****************************************************************/

void nsys_netout_journal_trimpartnote(nsys_netout_jsend_state * jsend, 
				      unsigned long minseq)

{
  int i, j, k;

  /* first prune note logs */
  
  j = NSYS_SM_CN_HDRSIZE;
  while (j < jsend->chaptern_size)
    {
      i = NSYS_SM_CLRS & jsend->chaptern[j];
      if (jsend->chaptern_seqarray[i] <= minseq)
	{	  
	  if (jsend->chaptern[i+NSYS_SM_CN_LOC_VEL] & NSYS_SM_CN_CHKY)
	    jsend->chaptern_timernum--;
	  jsend->chaptern_seqarray[i] = 0;
	  memmove(&(jsend->chaptern[j]), 
		  &(jsend->chaptern[j + NSYS_SM_CN_LOGSIZE]),
		  jsend->chaptern_size - j - NSYS_SM_CN_LOGSIZE);

	  if (jsend->chaptern_size < NSYS_SM_CN_SIZE)
	    jsend->chaptern[NSYS_SM_CN_LOC_LENGTH]--;
	  jsend->chaptern_size -= NSYS_SM_CN_LOGSIZE;
	  jsend->clen -= NSYS_SM_CN_LOGSIZE;
	}
      else
	{
	  jsend->chaptern_logptr[i] = j;
	  j += NSYS_SM_CN_LOGSIZE;
	}
    }
  
  /* then prune bitfields */
  
  i = (jsend->chapterb_low << NSYS_SM_CN_BFSHIFT);
  for (j = jsend->chapterb_low; j <= jsend->chapterb_high; j++)
    {
      if (jsend->chapterb[j])
	for (k = 128; k >= 1; k = (k >> 1))
	  {
	    if ((jsend->chapterb[j] & k) && 
		(jsend->chaptern_seqarray[i] <= minseq))
	      {
		jsend->chaptern_seqarray[i] = 0;
		jsend->chapterb[j] &= ~k;
	      }
	    i++;
	  }
      else
	i += 8;
      if ((j == jsend->chapterb_low) && 
	  (jsend->chapterb[j] == 0))
	{
	  jsend->chapterb_low++;
	  jsend->chapterb_size--;
	  jsend->clen--;
	}
    }
  while (jsend->chapterb_size && 
	 (jsend->chapterb[jsend->chapterb_high] == 0))
    {
      jsend->chapterb_high--;
      jsend->chapterb_size--;
      jsend->clen--;
    }
  if (jsend->chapterb_size == 0)
    {	  
      jsend->chapterb_low = NSYS_SM_CN_BFMAX;
      if (jsend->chaptern_size != NSYS_SM_CN_SIZE)
	jsend->chapterb_high = 1;
      else
	jsend->chapterb_high = NSYS_SM_CN_BFMIN;
    }
  jsend->chaptern[NSYS_SM_CN_LOC_LOWHIGH] = 
    ((jsend->chapterb_low << NSYS_SM_CN_LOWSHIFT) |
     jsend->chapterb_high);

}

/****************************************************************/
/*                trim entire note extras chapter               */
/****************************************************************/

void nsys_netout_journal_trimallextras(nsys_netout_jsend_state * jsend) 
 
{
  int i, j;

  jsend->chaptere_seqnum = 0;
  jsend->cheader[NSYS_SM_CH_LOC_TOC] &= NSYS_SM_CH_TOC_CLRE;
  jsend->clen -= jsend->chaptere_size;

  j = NSYS_SM_CE_HDRSIZE;
  while (j < jsend->chaptere_size)
    {
      i = NSYS_SM_CLRS & jsend->chaptere[j];
      if (jsend->chaptere[j + 1] & NSYS_SM_CE_CHKV)
	jsend->chapterev_seqarray[i] = 0;
      else
	jsend->chapterer_seqarray[i] = 0;
      j += NSYS_SM_CE_LOGSIZE;
    }

  jsend->chaptere_sset = 0;
}

/****************************************************************/
/*              partially trim note extras chapter              */
/****************************************************************/

void nsys_netout_journal_trimpartextras(nsys_netout_jsend_state * jsend, 
					unsigned long minseq)

{
  int i, j;

  j = NSYS_SM_CE_HDRSIZE;
  while (j < jsend->chaptere_size)
    {
      i = NSYS_SM_CLRS & jsend->chaptere[j];
      if (jsend->chaptere[j + 1] & NSYS_SM_CE_CHKV)
	{
	  if (jsend->chapterev_seqarray[i] <= minseq)
	    {
	      jsend->chapterev_seqarray[i] = 0;
	      memmove(&(jsend->chaptere[j]), 
		      &(jsend->chaptere[j + NSYS_SM_CE_LOGSIZE]),
		      jsend->chaptere_size - j - NSYS_SM_CE_LOGSIZE);
	      jsend->clen -= NSYS_SM_CE_LOGSIZE;
	      jsend->chaptere_size -= NSYS_SM_CE_LOGSIZE;
	      jsend->chaptere[NSYS_SM_CE_LOC_LENGTH]--;
	    }
	  else
	    {
	      jsend->chapterev_logptr[i] = j;
	      j += NSYS_SM_CE_LOGSIZE;
	    }
	}
      else
	{
	  if (jsend->chapterer_seqarray[i] <= minseq)
	    {
	      jsend->chapterer_seqarray[i] = 0;
	      memmove(&(jsend->chaptere[j]), 
		      &(jsend->chaptere[j + NSYS_SM_CE_LOGSIZE]),
		      jsend->chaptere_size - j - NSYS_SM_CE_LOGSIZE);
	      jsend->clen -= NSYS_SM_CE_LOGSIZE;
	      jsend->chaptere_size -= NSYS_SM_CE_LOGSIZE;
	      jsend->chaptere[NSYS_SM_CE_LOC_LENGTH]--;
	    }
	  else
	    {
	      jsend->chapterer_logptr[i] = j;
	      j += NSYS_SM_CE_LOGSIZE;
	    }
	}
    }
}

/****************************************************************/
/*                 trim channel touch chapter                   */
/****************************************************************/

void nsys_netout_journal_trimctouch(nsys_netout_jsend_state * jsend) 
 
{
  jsend->chaptert_seqnum = 0;
  jsend->cheader[NSYS_SM_CH_LOC_TOC] &= NSYS_SM_CH_TOC_CLRT;
  jsend->clen -= NSYS_SM_CT_SIZE;
}

/****************************************************************/
/*                 trim entire poly touch chapter               */
/****************************************************************/

void nsys_netout_journal_trimallptouch(nsys_netout_jsend_state * jsend) 
 
{
  int i, j;

  jsend->chaptera_seqnum = 0;
  jsend->cheader[NSYS_SM_CH_LOC_TOC] &= NSYS_SM_CH_TOC_CLRA;
  jsend->clen -= jsend->chaptera_size;
  j = NSYS_SM_CA_HDRSIZE;
  while (j < jsend->chaptera_size)
    {
      i = NSYS_SM_CLRS & jsend->chaptera[j];
      jsend->chaptera_seqarray[i] = 0;
      j += NSYS_SM_CA_LOGSIZE;
    }
  jsend->chaptera_sset = 0;
}

/****************************************************************/
/*                 partially trim poly touch chapter            */
/****************************************************************/

void nsys_netout_journal_trimpartptouch(nsys_netout_jsend_state * jsend, 
					unsigned long minseq)

{
  int i, j;

  j = NSYS_SM_CA_HDRSIZE;
  while (j < jsend->chaptera_size)
    {
      i = NSYS_SM_CLRS & jsend->chaptera[j];
      if (jsend->chaptera_seqarray[i] <= minseq)
	{
	  jsend->chaptera_seqarray[i] = 0;
	  memmove(&(jsend->chaptera[j]), 
		  &(jsend->chaptera[j + NSYS_SM_CA_LOGSIZE]),
		  jsend->chaptera_size - j - NSYS_SM_CA_LOGSIZE);
	  jsend->clen -= NSYS_SM_CA_LOGSIZE;
	  jsend->chaptera_size -= NSYS_SM_CA_LOGSIZE;
	  jsend->chaptera[NSYS_SM_CA_LOC_LENGTH]--;
	}
      else
	{
	  jsend->chaptera_logptr[i] = j;
	  j += NSYS_SM_CA_LOGSIZE;
	}
    }
}

/****************************************************************/
/*                  trim entire Systems journal                 */
/****************************************************************/

void nsys_netout_journal_trimsystem(void)

{
  nsys_netout_jsend_system_state * jsys;
  int i;

  jsys = &(nsys_netout_jsend_system);
  memset(jsys->sheader, 0, NSYS_SM_SH_SIZE);

  jsys->sheader_seqnum  = 0;
  jsys->chapterv_seqnum = 0;
  jsys->chapterq_seqnum = 0;

  jsys->chapterf_seqnum = 0;
  jsys->chapterfc_seqnum = 0;

  jsys->chapterd_seqnum = 0;
  jsys->chapterd_reset_seqnum = 0;
  jsys->chapterd_tune_seqnum = 0;
  jsys->chapterd_song_seqnum = 0;
  jsys->chapterd_scj_seqnum = 0;
  jsys->chapterd_sck_seqnum = 0;
  jsys->chapterd_rty_seqnum = 0;
  jsys->chapterd_rtz_seqnum = 0;

  jsys->chapterx_seqnum = 0;

  if (jsys->chapterf[NSYS_SM_CF_LOC_HDR] & NSYS_SM_CF_HDR_CHKC)
    {
      jsys->chapterf[NSYS_SM_CF_LOC_HDR] &= NSYS_SM_CF_HDR_CLRC;
      jsys->chapterf_size -= NSYS_SM_CF_SIZE_COMPLETE;

      if (jsys->chapterf[NSYS_SM_CF_LOC_HDR] & NSYS_SM_CF_HDR_CHKP)
	memcpy(&(jsys->chapterf[NSYS_SM_CF_LOC_FIELDS]),
	       &(jsys->chapterf[NSYS_SM_CF_LOC_FIELDS + NSYS_SM_CF_SIZE_COMPLETE]),
	       NSYS_SM_CF_SIZE_PARTIAL);
    }

  jsys->chapterx_gmreset_off = NULL;
  jsys->chapterx_gmreset_on = NULL;
  jsys->chapterx_mvolume = NULL;

  for (i = 0; i < jsys->chapterx_stacklen; i++)
    {
      jsys->chapterx_stack[i]->next = nsys_netout_jsend_xstackfree;
      nsys_netout_jsend_xstackfree = jsys->chapterx_stack[i];
      jsys->chapterx_stack[i] = NULL;
    }

  jsys->chapterx_stacklen = 0;

}

/****************************************************************/
/*                 Trim (entire) simple chapter                 */
/****************************************************************/

void nsys_netout_journal_trimsimple(void)

{
  nsys_netout_jsend_system_state * jsys;

  jsys = &(nsys_netout_jsend_system);

  jsys->chapterd_seqnum = 0;
  jsys->chapterd_reset_seqnum = 0;
  jsys->chapterd_tune_seqnum = 0;
  jsys->chapterd_song_seqnum = 0;

  jsys->sheader[NSYS_SM_SH_LOC_FLAGS] &= NSYS_SM_SH_CLRD;
  jsys->slen -= jsys->chapterd_front_size;

  if (jsys->chapterd_scj_seqnum)
    {
      jsys->slen -= jsys->chapterd_scj_size;
      jsys->chapterd_scj_seqnum = 0;
    }

  if (jsys->chapterd_sck_seqnum)
    {
      jsys->slen -= jsys->chapterd_sck_size;
      jsys->chapterd_sck_seqnum = 0;
    }

  if (jsys->chapterd_rty_seqnum)
    {
      jsys->slen -= NSYS_SM_CD_REALTIME_SIZE;
      jsys->chapterd_rty_seqnum = 0;
    }

  if (jsys->chapterd_rtz_seqnum)
    {
      jsys->slen -= NSYS_SM_CD_REALTIME_SIZE;
      jsys->chapterd_rtz_seqnum = 0;
    }

}

/****************************************************************/
/*                Trim Reset from simple chapter                */
/****************************************************************/

void nsys_netout_journal_trimreset(void)

{
  nsys_netout_jsend_system_state * jsys;
  unsigned char * p;

  jsys = &(nsys_netout_jsend_system);

  jsys->chapterd_reset_seqnum = 0;

  p = &(jsys->chapterd_front[NSYS_SM_CD_LOC_LOGS]);

  if (jsys->chapterd_tune_seqnum)
    (*(++p)) = jsys->chapterd_tune;
  if (jsys->chapterd_song_seqnum)
    (*(++p)) = jsys->chapterd_song;

  jsys->chapterd_front[NSYS_SM_CD_LOC_TOC] &= NSYS_SM_CD_TOC_CLRB;
  jsys->chapterd_front_size -= NSYS_SM_CD_SIZE_RESET;
  jsys->slen -= NSYS_SM_CD_SIZE_RESET;
}

/****************************************************************/
/*             Trim Tune Request from simple chapter            */
/****************************************************************/

void nsys_netout_journal_trimtune(void)

{
  nsys_netout_jsend_system_state * jsys;
  unsigned char * p;

  jsys = &(nsys_netout_jsend_system);

  jsys->chapterd_tune_seqnum = 0;

  p = &(jsys->chapterd_front[NSYS_SM_CD_LOC_LOGS]);

  if (jsys->chapterd_reset_seqnum)
    p++;

  if (jsys->chapterd_song_seqnum)
    *p = jsys->chapterd_song;

  jsys->chapterd_front[NSYS_SM_CD_LOC_TOC] &= NSYS_SM_CD_TOC_CLRG;
  jsys->chapterd_front_size -= NSYS_SM_CD_SIZE_TUNE;
  jsys->slen -= NSYS_SM_CD_SIZE_TUNE;
}

/****************************************************************/
/*            Trim Song Select from simple chapter              */
/****************************************************************/

void nsys_netout_journal_trimsong(void)

{
  nsys_netout_jsend_system_state * jsys;

  jsys = &(nsys_netout_jsend_system);

  jsys->chapterd_song_seqnum = 0;

  jsys->chapterd_front[NSYS_SM_CD_LOC_TOC] &= NSYS_SM_CD_TOC_CLRH;
  jsys->chapterd_front_size -= NSYS_SM_CD_SIZE_SONG;
  jsys->slen -= NSYS_SM_CD_SIZE_SONG;
}

/****************************************************************/
/*        Trim undefined Common (j) from simple chapter         */
/****************************************************************/

void nsys_netout_journal_trimscj(void)

{
  nsys_netout_jsend_system_state * jsys;

  jsys = &(nsys_netout_jsend_system);

  jsys->chapterd_scj_seqnum = 0;

  jsys->chapterd_front[NSYS_SM_CD_LOC_TOC] &= NSYS_SM_CD_TOC_CLRJ;
  jsys->slen -= jsys->chapterd_scj_size;
}

/****************************************************************/
/*        Trim undefined Common (k) from simple chapter         */
/****************************************************************/

void nsys_netout_journal_trimsck(void)

{
  nsys_netout_jsend_system_state * jsys;

  jsys = &(nsys_netout_jsend_system);

  jsys->chapterd_sck_seqnum = 0;

  jsys->chapterd_front[NSYS_SM_CD_LOC_TOC] &= NSYS_SM_CD_TOC_CLRK;
  jsys->slen -= jsys->chapterd_sck_size;
}

/****************************************************************/
/*        Trim undefined Realtime (y) from simple chapter       */
/****************************************************************/

void nsys_netout_journal_trimrty(void)

{
  nsys_netout_jsend_system_state * jsys;

  jsys = &(nsys_netout_jsend_system);

  jsys->chapterd_rty_seqnum = 0;

  jsys->chapterd_front[NSYS_SM_CD_LOC_TOC] &= NSYS_SM_CD_TOC_CLRY;
  jsys->slen -= NSYS_SM_CD_REALTIME_SIZE;
}

/****************************************************************/
/*        Trim undefined Realtime (z) from simple chapter       */
/****************************************************************/

void nsys_netout_journal_trimrtz(void)

{
  nsys_netout_jsend_system_state * jsys;

  jsys = &(nsys_netout_jsend_system);

  jsys->chapterd_rtz_seqnum = 0;

  jsys->chapterd_front[NSYS_SM_CD_LOC_TOC] &= NSYS_SM_CD_TOC_CLRZ;
  jsys->slen -= NSYS_SM_CD_REALTIME_SIZE;
}

/****************************************************************/
/*                  Trim active sense chapter                   */
/****************************************************************/

void nsys_netout_journal_trimsense(void)

{
  nsys_netout_jsend_system_state * jsys;

  jsys = &(nsys_netout_jsend_system);
  jsys->chapterv_seqnum = 0;
  jsys->sheader[NSYS_SM_SH_LOC_FLAGS] &= NSYS_SM_SH_CLRV;
  jsys->slen -= NSYS_SM_CV_SIZE;
}

/****************************************************************/
/*                  Trim sequencer chapter                      */
/****************************************************************/

void nsys_netout_journal_trimsequence(void)

{
  nsys_netout_jsend_system_state * jsys;

  jsys = &(nsys_netout_jsend_system);
  jsys->chapterq_seqnum = 0;
  jsys->sheader[NSYS_SM_SH_LOC_FLAGS] &= NSYS_SM_SH_CLRQ;
  if (jsys->chapterq[NSYS_SM_CQ_LOC_HDR] & NSYS_SM_CQ_HDR_CHKC)
    jsys->slen -= NSYS_SM_CQ_SIZE_CLOCK + NSYS_SM_CQ_SIZE_HDR;
  else
    jsys->slen -= NSYS_SM_CQ_SIZE_HDR;
}

/****************************************************************/
/*                  Trim MIDI Time code chapter                 */
/****************************************************************/

void nsys_netout_journal_trimtimecode(void)

{
  nsys_netout_jsend_system_state * jsys;

  jsys = &(nsys_netout_jsend_system);

  jsys->sheader[NSYS_SM_SH_LOC_FLAGS] &= NSYS_SM_SH_CLRF;
  jsys->slen -= NSYS_SM_CF_SIZE_HDR;
  jsys->chapterf_seqnum = 0;
  jsys->chapterfc_seqnum = 0;

  if (jsys->chapterf[NSYS_SM_CF_LOC_HDR] & NSYS_SM_CF_HDR_CHKC)
    {
      jsys->chapterf[NSYS_SM_CF_LOC_HDR] &= NSYS_SM_CF_HDR_CLRC;
      jsys->chapterf_size -= NSYS_SM_CF_SIZE_COMPLETE;
      jsys->slen -= NSYS_SM_CF_SIZE_COMPLETE;

      if (jsys->chapterf[NSYS_SM_CF_LOC_HDR] & NSYS_SM_CF_HDR_CHKP)
	memcpy(&(jsys->chapterf[NSYS_SM_CF_LOC_FIELDS]),
	       &(jsys->chapterf[NSYS_SM_CF_LOC_FIELDS + NSYS_SM_CF_SIZE_COMPLETE]),
	       NSYS_SM_CF_SIZE_PARTIAL);
    }

  if (jsys->chapterf[NSYS_SM_CF_LOC_HDR] & NSYS_SM_CF_HDR_CHKP)
    jsys->slen -= NSYS_SM_CF_SIZE_PARTIAL;
}

/****************************************************************/
/*        Trim COMPLETE field from MIDI Time code chapter       */
/****************************************************************/

void nsys_netout_journal_trimparttimecode(void)

{
  nsys_netout_jsend_system_state * jsys;

  jsys = &(nsys_netout_jsend_system);
  jsys->chapterfc_seqnum = 0;

  jsys->chapterf[NSYS_SM_CF_LOC_HDR] &= NSYS_SM_CF_HDR_CLRC;
  jsys->chapterf_size -= NSYS_SM_CF_SIZE_COMPLETE;
  jsys->slen -= NSYS_SM_CF_SIZE_COMPLETE;

  if (jsys->chapterf[NSYS_SM_CF_LOC_HDR] & NSYS_SM_CF_HDR_CHKP)
    memcpy(&(jsys->chapterf[NSYS_SM_CF_LOC_FIELDS]),
	   &(jsys->chapterf[NSYS_SM_CF_LOC_FIELDS + NSYS_SM_CF_SIZE_COMPLETE]),
	   NSYS_SM_CF_SIZE_PARTIAL);
}


/****************************************************************/
/*                    Trim SysEx chapter                        */
/****************************************************************/

void nsys_netout_journal_trimsysex(void)

{
  nsys_netout_jsend_system_state * jsys;
  int i;

  jsys = &(nsys_netout_jsend_system);
  jsys->chapterx_seqnum = 0;

  jsys->chapterx_gmreset_off = NULL;
  jsys->chapterx_gmreset_on = NULL;
  jsys->chapterx_mvolume = NULL;

  for (i = 0; i < jsys->chapterx_stacklen; i++)
    {
      jsys->slen -= jsys->chapterx_stack[i]->size;
      jsys->chapterx_stack[i]->next = nsys_netout_jsend_xstackfree;
      nsys_netout_jsend_xstackfree = jsys->chapterx_stack[i];
      jsys->chapterx_stack[i] = NULL;
    }

  jsys->chapterx_stacklen = 0;

  jsys->sheader[NSYS_SM_SH_LOC_FLAGS] &= NSYS_SM_SH_CLRX;
}

/****************************************************************/
/*            Trim part of the SysEx chapter                    */
/****************************************************************/

void nsys_netout_journal_trimpartsysex(unsigned long minseq)

{
  nsys_netout_jsend_system_state * jsys;
  int i, j;

  jsys = &(nsys_netout_jsend_system);

  for (i = j = 0; i < jsys->chapterx_stacklen; i++)
    if (jsys->chapterx_stack[i]->seqnum <= minseq)
      {
	jsys->slen -= jsys->chapterx_stack[i]->size;
	*(jsys->chapterx_stack[i]->cmdptr) = NULL;
	jsys->chapterx_stack[i]->next = nsys_netout_jsend_xstackfree;
	nsys_netout_jsend_xstackfree = jsys->chapterx_stack[i];
	jsys->chapterx_stack[i] = NULL;
      }
    else
      {
	j = i;
	break;
      }

  jsys->chapterx_stacklen -= j;
  
  for (i = 0; i < jsys->chapterx_stacklen; i++)
    {
      jsys->chapterx_stack[i] = jsys->chapterx_stack[i + j];
      jsys->chapterx_stack[i]->index = i;
    }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*      second-level journal functions: miscellaneous           */
/*______________________________________________________________*/

/****************************************************************/
/*           updates state after checkpoint change              */
/****************************************************************/

void nsys_netout_journal_changecheck(void)

{
  unsigned short check_seqnum_net;

  nsys_netout_jsend_checkpoint_changed = 1;
  check_seqnum_net = htons((unsigned short) (NSYS_RTPSEQ_LOMASK &
					     nsys_netout_jsend_checkpoint_seqnum));
  memcpy(&(nsys_netout_jsend_header[NSYS_SM_JH_LOC_CHECK]),
	 &check_seqnum_net, sizeof(unsigned short));

}

/****************************************************************/
/*    sender state:  clear N-active state, Chapters N, T, A     */
/****************************************************************/

void nsys_netout_journal_clear_nactive(nsys_netout_jsend_state * jsend)

{ 
  if (jsend->chaptern_seqnum)
    nsys_netout_journal_trimallnote(jsend); 

  if (jsend->chaptere_seqnum)
    nsys_netout_journal_trimallextras(jsend); 

  if (jsend->chaptert_seqnum)
    nsys_netout_journal_trimctouch(jsend);

  *((unsigned short *) &(jsend->cheader[NSYS_SM_CH_LOC_LEN])) = 
    htons(jsend->clen);
}

/****************************************************************/
/*    sender state:  clear C-active state, Chapters C, M        */
/****************************************************************/

void nsys_netout_journal_clear_cactive(nsys_netout_jsend_state * jsend)

{ 
  int i;

  /*************/
  /* Chapter C */
  /*************/

  if (jsend->chapterc_sustain & 0x01)
    jsend->chapterc_sustain = ((jsend->chapterc_sustain + 1)  
			       & NSYS_SM_CC_ALTMOD);

  /*************/
  /* Chapter M */
  /*************/

  jsend->chapterm_state = NSYS_SM_CM_STATE_OFF;
  for (i = 0; i < NSYS_SM_CM_ARRAYSIZE; i++)
    jsend->chapterm_cbutton[i] = 0;

  /*************/
  /* Chapter W */
  /*************/

  if (jsend->chapterw_seqnum)
    nsys_netout_journal_trimpwheel(jsend);

  /*************/
  /* Chapter T */
  /*************/

  if (jsend->chaptert_seqnum)
    nsys_netout_journal_trimctouch(jsend);

  /*************/
  /* Chapter A */
  /*************/

  if (jsend->chaptera_seqnum)
    nsys_netout_journal_trimallptouch(jsend);

  /***************/
  /* update clen */
  /***************/

  *((unsigned short *) &(jsend->cheader[NSYS_SM_CH_LOC_LEN])) = 
    htons(jsend->clen);
}

/****************************************************************/
/*            sender state:  clear all active state             */
/****************************************************************/

void nsys_netout_journal_clear_active(unsigned char cmd)

{
  /* to do 
   *
   * cmd codes CSYS_MIDI_SYSTEM_RESET or CSYS_MIDI_GMRESET
   *
   * clear journal state defined as "active"
   *
   */
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                  low-level journal functions                 */
/*______________________________________________________________*/


/* end Network library -- sender journal functions */
