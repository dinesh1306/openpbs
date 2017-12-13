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
#include <pbs_config.h>   /* the master config generated by configure */

#include <stdio.h>	/* DEBUG */
/**
 * @file	site_mom_ckp.c
 * @brief
 * site_mom_pchk.c = a site modifiable file
 *
 *	Contains Pre and Post checkpoint stubs for MOM.
 */

/*
 * This is used only in mom and needs PBS_MOM defined in order to
 * have things from other .h files (such as struct task) be defined
 */
#define PBS_MOM

#include <sys/types.h>
#include <pwd.h>
#include "portability.h"
#include "list_link.h"
#include "server_limits.h"
#include "attribute.h"
#include "job.h"
#include "mom_mach.h"
#include "mom_func.h"


/**
 * @brief
 * 	site_mom_postchk() - Post-checkpoint stub for MOM.
 *	Called if checkpoint (on qhold,qterm) suceeded.
 *
 * @param[in] pjob - job pointer
 * @param[in] hold_type - hold type indicating what caused held state of job
 *
 * @return	int
 * @retval	0		If ok
 * @retval	non-zero	If not ok.
 */

int
site_mom_postchk(job *pjob, int hold_type)
{
	return 0;
}

/**
 * @brief
 * 	site_mom_prerst() - Pre-restart stub for MOM.
 *	Called just before restart is performed.
 *
 * @param[in] pjob - job pointer
 *
 * @return	int
 * @retval	0		if ok
 * @retval	JOB_EXEC_FATAL1 for permanent error, abort job
 * @retval	JOB_EXEC_RETRY	for temporary problem, requeue job.
 */

int
site_mom_prerst(job *pjob)
{
	return 0;
}
