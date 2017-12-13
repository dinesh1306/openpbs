/*
 * Copyright (C) 1994-2018 Altair Engineering, Inc.
 * For more information, contact Altair at www.altair.com.
 *
 * This file is part of the PBS Professional("PBS Pro") software.
 *
 * Open Source License Information:
 *
 * PBS Pro is free software. You can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * PBS Pro is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Commercial License Information:
 *
 * For a copy of the commercial license terms and conditions,
 * go to: (http://www.pbspro.com/UserArea/agreement.html)
 * or contact the Altair Legal Department.
 *
 * Altair’s dual-license business model allows companies, individuals, and
 * organizations to create proprietary derivative works of PBS Pro and
 * distribute them - whether embedded or bundled with other software -
 * under a commercial license agreement.
 *
 * Use of Altair’s trademarks, including but not limited to "PBS™",
 * "PBS Professional®", and "PBS Pro™" and Altair’s logos is subject to Altair's
 * trademark licensing policies.
 *
 */
/**
 *  @file    pbsTkInit.c
 *
 *  @brief
 *		pbsTkInit - All the pbs specific code needed to make pbswish
 *		work is here.  The routine pbsTk_Init() is to be called
 *		in place of Tk_Init().
 *
 * Functions included are:
 * 	pbsTcl_Init()
 * 	main()
 */

#include	"pbs_version.h"
#include	"pbs_config.h"

#include	"tcl.h"
#include	"tk.h"
#include	<string.h>
#include	<stdlib.h>
#include	"rm.h"
#include	"pbs_ifl.h"
#include	"pbs_internal.h"
#include	"log.h"


char	log_buffer[LOG_BUF_SIZE];

extern	void	add_cmds(Tcl_Interp *interp);

/**
 * @brief
 * 		pbsTcl_Init	- Function to initialize Tcl interpreter based on the environment.
 *
 * @param[in,out]	interp	-	Interpreter for application.
 *
 * @return	int
 * @retval	TCL_OK	: everything looks good.
 * @retval	TCL_ERROR	: something got wrong!
 */
int
pbsTcl_Init(Tcl_Interp *interp)
{
	if (Tcl_Init(interp) == TCL_ERROR)
		return TCL_ERROR;
	if (Tk_Init(interp) == TCL_ERROR)
		return TCL_ERROR;
	Tcl_StaticPackage(interp, "Tk", Tk_Init, Tk_SafeInit);
#if	TCLX
	if (Tclx_Init(interp) == TCL_ERROR)
		return TCL_ERROR;
	if (Tkx_Init(interp) == TCL_ERROR)
		return TCL_ERROR;
#endif

	fullresp(0);
	add_cmds(interp);

	Tcl_SetVar(interp, "tcl_rcFileName", "~/.wishrc", TCL_GLOBAL_ONLY);
	return TCL_OK;
}
/**
 * @brief
 * 		main - the entry point in pbsTkInit.c
 *
 * @param[in]	argc	-	argument count.
 * @param[in]	argv	-	argument variables.
 *
 * @return	int
 * @retval	0	: success
 */
int
main(int argc, char *argv[])
{

	char    tcl_libpath[MAXPATHLEN+13];     /* 13 for "TCL_LIBRARY=" + \0 */
	char    tk_libpath[MAXPATHLEN+12];     /* 12 for "TK_LIBRARY=" + \0 */

	/*the real deal or just pbs_version and exit?*/

	execution_mode(argc, argv);
	set_logfile(stderr);

#ifdef WIN32
	winsock_init();
	Tcl_FindExecutable(argv[0]);
#endif

	pbs_loadconf(0);

	if (!getenv("TCL_LIBRARY")) {
		if (pbs_conf.pbs_exec_path) {
			sprintf((char *)tcl_libpath,
				"TCL_LIBRARY=%s/tcltk/lib/tcl%s",
				pbs_conf.pbs_exec_path, TCL_VERSION);
			putenv(tcl_libpath);
		}
	}


	if (!getenv("TK_LIBRARY")) {
		if (pbs_conf.pbs_exec_path) {
			sprintf((char *)tk_libpath,
#ifdef WIN32
				"TK_LIBRARY=%s/lib/tk%s",
#else
				"TK_LIBRARY=%s/tcltk/lib/tk%s",
#endif
				pbs_conf.pbs_exec_path, TK_VERSION);
			putenv(tk_libpath);
		}
	}

	Tk_Main(argc, argv, pbsTcl_Init);
	return 0;
}
