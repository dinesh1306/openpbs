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

/*	pbsD_modify_resv.c
 *
 *	The Modify Reservation request.
 */

#include <pbs_config.h>   /* the master config generated by configure */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "libpbs.h"
#include "pbs_ecl.h"

/**
 * @brief
 *	Passes modify reservation request to PBSD_modify_resv( )
 *
 * @param[in]   c - socket on which connected
 * @param[in]   attrib - the list of attributes for batch request
 * @param[in]   extend - extension of batch request
 *
 * @return char*
 * @retval SUCCESS returns the response from the server.
 * @retval ERROR NULL
 */
char *
pbs_modify_resv(int c, char *resv_id, struct attropl *attrib, char *extend)
{
	struct attropl *pal = NULL;
	int		rc = 0;
	char		*ret = NULL;

	for (pal = attrib; pal; pal = pal->next)
		pal->op = SET;

	/* initialize the thread context data, if not already initialized */
	if (pbs_client_thread_init_thread_context() != 0)
		return (char *)NULL;

	/* first verify the attributes, if verification is enabled */
	rc = pbs_verify_attributes(c, PBS_BATCH_ModifyResv,
		MGR_OBJ_RESV, MGR_CMD_NONE, attrib);
	if (rc)
		return (char *)NULL;

	/* lock pthread mutex here for this connection
	 * blocking call, waits for mutex release */
	if (pbs_client_thread_lock_connection(c) != 0)
		return (char *)NULL;

	/* initiate the modification of the reservation  */
	ret = PBSD_modify_resv(c, resv_id, attrib, extend);

	/* unlock the thread lock and update the thread context data */
	if (pbs_client_thread_unlock_connection(c) != 0)
		return (char *) NULL;

	return ret;
}
