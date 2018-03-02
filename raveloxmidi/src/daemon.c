/*
   This file is part of raveloxmidi.

   Copyright (C) 2014 Dave Kelly

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA 
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "utils.h"
#include "raveloxmidi_config.h"
#include "logging.h"

extern int errno;

void daemon_start(void)
{
        pid_t pid;
	char *pid_file;
	FILE *reopen_file = NULL;

	pid_file = config_get("daemon.pid_file");

	if( is_yes( config_get("security.check") ) )
	{
		/* Check that the pid file is valid as it's going to be unlinked at the end */
		if( ! check_file_security( pid_file ) )
		{
			logging_printf( LOGGING_ERROR, "%s fails security check\n", pid_file);
			_exit(1);
		}
	}

        if ( ( pid = fork() )< 0 ) {
                logging_printf( LOGGING_ERROR,"Can't fork:%s\n", strerror( errno ));
                return;
        } else if ( pid != 0 ) {
                /* Parent exiting */
                _exit(0);
        }       

        pid = setsid();
        if ( pid < 0 ) { 
                logging_printf( LOGGING_ERROR,"Couldn't create session leader\n");
                _exit(1);
        }       

        if ( ( pid = fork() )< 0 ) {
                logging_printf( LOGGING_ERROR,"Can't fork:%s\n", strerror( errno));
                _exit(1);
        } else if ( pid != 0 ) {
                FILE *pidfile = NULL;

		// Disable writing a pid file if readonly option is set
		if( is_no( config_get("readonly") ) )
		{
			pidfile = fopen(pid_file, "w");

			if( pidfile != NULL ) {
				fprintf(pidfile, "%d\n", pid);
				fclose(pidfile);
			} else {
				logging_printf( LOGGING_ERROR, "daemon_start: Unable to write pid=%d to %s:%s\n", pid, pid_file, strerror( errno ));
			}       
		}
                /* Parent exiting */
                _exit(0);
        }       

        reopen_file = freopen("/dev/null","r",stdin);
	if( ! reopen_file )
	{
		logging_printf( LOGGING_ERROR, "daemon_start: Unable to reopen stdin as /dev/null: %s\n", strerror( errno ) );
	}
        reopen_file = freopen("/dev/null","w",stdout);
	if( ! reopen_file )
	{
		logging_printf( LOGGING_ERROR, "daemon_start: Unable to reopen stdout as /dev/null: %s\n", strerror( errno ) );
	}
}

void daemon_stop(void)
{
	
	struct stat filestat;
	int ret = 0;
	char *pid_file_name = NULL;

	pid_file_name = config_get("daemon.pid_file");

	if( pid_file_name )
	{
		ret = stat( pid_file_name, &filestat );

		if( ret == 0 )
		{
			ret = unlink( pid_file_name );
			if( ret != 0 )
			{
				logging_printf( LOGGING_ERROR, "Cannot remove %s:%s\n", pid_file_name, strerror( errno ) );
			}
		}
	}
}
