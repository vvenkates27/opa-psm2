/*

  This file is provided under a dual BSD/GPLv2 license.  When using or
  redistributing this file, you may do so under either license.

  GPL LICENSE SUMMARY

  Copyright(c) 2015 Intel Corporation.

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  Contact Information:
  Intel Corporation, www.intel.com

  BSD LICENSE

  Copyright(c) 2015 Intel Corporation.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef _PSMI_LOG_H
#define _PSMI_LOG_H

/*

  A note about PSM2_LOG and PSM2_LOG_FAST_IO:

  By default, the PSM2_LOG facility is safe, slow, and is complete.  That is, if the
  test case you are debugging has an abnormal termiation, no problem.  The logs are
  saved up to the point of the abnormal termination.  Abnormal termination can be
  a seg fault, the test case issues a fatal error, or exit()'s or abort()'s.

  However, debugging timing sensitive problems, make the usual SLOW PSM2_LOG
  facility inadequate as the timing overhead that it introduces dominates, and the
  symptoms of your problem may change or go away.

  For this case, you can use BOTH: PSM2_LOG and PSM2_LOG_FAST_IO.  To use
  PSM2_LOG_FAST_IO though, caution: for abnormal program termination, you will get
  no log file.

  To workaround this problem, and allow you to get a log file even after an abnormal
  program terminiation, we expose psmi_log_fini to the outside world (via the linker
  script file), and so in your client test code, you can call psmi_log_fini() on a
  fatal error (e.g. in a signal handler).

  --------------------------------------------------------------------------------

  This file (psm_log.h) defines macros for logging messages to assist investigations
  into the psm library.

  By default, these macros are not defined when building psm.  When not defined, the
  macros become no-ops in the PSM code.

  When enabled (by defining the PSM2_LOG symbol), the macros present information to
  the psmi_log_message() facility for processing.  See below for more information on the
  psmi_log_message() facility.

  To enable the macros, build PSM with the PSM2_LOG environment variable exported, ala:

  PSM2_LOG=1 make ...

  The macros are described in the following:

  PSM2_LOG_MSG(FORMAT,...)        Spills a printf-style message to the log.
  PSM2_LOG_DECLARE_BT_BUFFER()    Declares a local back trace buffer for use with the
  				 PSM2_LOG_BT() macro.
  PSM2_LOG_BT(NFRAMES,FORMAT,...) Spills the current backtrace, if it differs from the
                                 previous backtrace spilled to the log.

  The psmi_log_message() facility is the backend for these messages when PSM2_LOG is enabled.
  The psmi_log_message() facility spills messages to unique log files based on the process id
  and the thread id.  So every unique process id, and thread id will spill to unique log files.
  The psmi_log_message prefixes each message in the log files with a high resolution timer
  message so that messages from multiple threads and log files can be reconciled to one timeline.
  It is left as an exercise to the reader to reconcile log messages from different hosts to one
  timeline.

  The backtrace capability in the PSM2_LOG functionality needs some explanation: often a bug
  happens only when the code is tickled from a specific call-chain.  The PSM2_LOG_BT() macro
  supports identifying the unique call-chain when a problem occurs.  The model is as follows:

  A unique declaration is made for a backtrace to spill the backtrace information to.  This
  declaration should be made in the same basic block as the use of the PSM2_LOG_BT() macro.
  To make the declaration, use PSM2_LOG_DECLARE_BT_BUFFER().

  When the PSM2_LOG is enabled, at the statement for the macro: PSM2_LOG_BT(NFRAMES,FORMAT,...),
  the psmi_log_message() facility generates the current backtrace, and compares the first
  NFRAMES of the current backtrace against the previous backtrace stored in the backtrace
  buffer declared with the declaration.  If the two backtraces differ, the psmi_log_message()
  code saves the current backtrace into the declared buffer, and then spills the backtrace to the
  log file.

  At runtime, setting environment variables can squelch the log file from getting too big:

  PSM2_LOG_INC_FUNCTION_NAMES is a list of function name lists (abbreviated FNL) (see below),
  that will INClude the FNL's into the colleciton of functions to spill log data for.

  PSM2_LOG_EXC_FUNCTION_NAMES is a list of FNL's (see below), that will EXClude the FNL's from the
  collection of functions to spill log data for.

  An FNL is a 'Function Name List' that is defined by the following grammar:

  # A LINE1 is either a single line number of a range of line numbers:
  LINE1 :: lineNumber |
           lineNumber1 '-' lineNumber2

  # LINES is a list of LINE1's separated by commas:
  LINES :: LINE1 |
           LINE1 ',' LINES

  # An FN is either a function name, or a function name with a list of lines:
  FN :: functionName |
        functionName ';' LINES

  # A FNL is a list of FN's separated by colons:
  FNL ::  FN |
          FN ':' FNL

  # Examples:
  foo:bar    the two functions foo and bar
  foo;1-10   lines 1 to 10 of function foo.
  bar;1,3,5  lines 1, 3 and 5 of function bar

  PSM2_LOG_SRCH_FORMAT_STRING If set, overrides the PSM2_LOG_INC_FUNCTION_NAMES
  and PSM2_LOG_EXC_FUNCTION_NAMES settings.  Causes the psmi_log_message() facility
  to only emit the log messages that match (using fnmatch()) the message in FORMAT.

 */

#define PSM2_LOG_EPM_TX ((int)1)
#define PSM2_LOG_EPM_RX ((int)0)


#ifdef PSM2_LOG

extern void psmi_log_initialize(void);

/* defined in psm_utils.c */
extern void psmi_log_message(const char *fileName,
			     const char *functionName,
			     int lineNumber,
			     const char *format, ...);

#ifdef PSM2_LOG_FAST_IO
extern void psmi_log_fini(void);
#else
#define psmi_log_fini() /* nothing */
#endif

#define PSM2_LOG_MSG(FORMAT , ...) psmi_log_message(__FILE__,__FUNCTION__,__LINE__,FORMAT, ## __VA_ARGS__)

#define PSM2_LOG_BT_BUFFER_SIZE 100

#define PSM2_LOG_DECLARE_BT_BUFFER() static void * psm_log_bt_buffer[PSM2_LOG_BT_BUFFER_SIZE]

#define PSM2_LOG_BT_MAGIC ((const char *)-1)

#define PSM2_LOG_BT(NFRAMES,FORMAT , ...) psmi_log_message(__FILE__,__FUNCTION__,__LINE__,PSM2_LOG_BT_MAGIC,psm_log_bt_buffer,NFRAMES,FORMAT, ## __VA_ARGS__)

#define PSM2_LOG_EPM_MAGIC ((const char *)-2)

/* EPM is short for Emit Protocol Message to the log file.
OPCODE is an int, and corresponds to one of the OPCODES declared in ptl_ips/ips_proto_header.h
TXRX is an int, and should be one of the above two consts (PSM2_LOG_EPM_TX, or PSM2_LOG_EPM_RX).
FROMEPID and TOEPID are uint64_t's and the fromepid should be the epid (end point id) of the sender   of the message
                                   and the toepid   should be the epid (end point id) of the receiver of the message
    */
#define PSM2_LOG_EPM(OPCODE,TXRX,FROMEPID,TOEPID,FORMAT,...) psmi_log_message(__FILE__,__FUNCTION__,__LINE__,PSM2_LOG_EPM_MAGIC,OPCODE,TXRX,FROMEPID,TOEPID,FORMAT, ## __VA_ARGS__)

/* Just adds a condition to the PSM2_LOG_EPM() macro. */
#define PSM2_LOG_EPM_COND(COND,OPCODE,TXRX,FROMEPID,TOEPID,FORMAT,...) if (COND) PSM2_LOG_EPM(OPCODE,TXRX,FROMEPID,TOEPID,FORMAT, ## __VA_ARGS__)

#define PSM2_LOG_DUMP_MAGIC ((const char *)-3)

#define PSM2_LOG_MSG_DUMP(ADDR,SIZE,FORMAT , ...) psmi_log_message(__FILE__,__FUNCTION__,__LINE__,PSM2_LOG_DUMP_MAGIC,ADDR,SIZE,FORMAT, ## __VA_ARGS__)

#else

#define psmi_log_initialize()                               /* nothing */

#define PSM2_LOG_MSG(FORMAT , ...)                          /* nothing */

#define psmi_log_fini()                                     /* nothing */

#define PSM2_LOG_DECLARE_BT_BUFFER()                         /* nothing */

#define PSM2_LOG_BT(NFRAMES,FORMAT , ...)                    /* nothing */

#define PSM2_LOG_EPM(OPCODE,TXRX,FROMEPID,TOEPID,FORMAT,...) /* nothing */

#define PSM2_LOG_EPM_COND(COND,OPCODE,TXRX,FROMEPID,TOEPID,FORMAT,...) /* nothing */

#define PSM2_LOG_MSG_DUMP(ADDR,SIZE,FORMAT , ...)                      /* nothing */

#endif /* #ifdef PSM2_LOG */

#endif /* #ifndef _PSMI_LOG_H */
