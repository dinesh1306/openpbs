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

#include <sys/types.h>
#include "portability.h"
#include "list_link.h"
#include "attribute.h"
#include "server_limits.h"
#include "pbs_error.h"
#include "job.h"

/**
 * @file	site_allow_u.c
 */

/**
 * @brief
 * 	site_allow_u - site allow user access
 *
 *	This routine determines if a user is privileged to access the batch
 *	ssytem on this host.
 *
 *	It's implementation is "left as an exersize for the reader."
 *
 * @param[in]	user - the user's name making the request
 * @param[in]	host - host from which the user is making the request
 *
 * @return	int
 * @retval	zero - if user is acceptable
 * @retval	non-zero error number (PBSE_PERM) if not
 */

int
site_allow_u(char *user, char *host)
{
	return 0;
}
