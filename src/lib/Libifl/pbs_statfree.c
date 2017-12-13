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
 * @file	pbs_statfree.c
 * @brief
 * The function that deallocates a "batch_status" structure
 */

#include <pbs_config.h>   /* the master config generated by configure */

#include <stdlib.h>
#include <stdio.h>
#include "libpbs.h"


/**
 * @brief
 *	-The function that deallocates a "batch_status" structure
 * 
 * @param[in] bsp - pointer to batch request.
 *
 * @return	Void
 *
 */
void
__pbs_statfree(struct batch_status *bsp)
{
	struct attrl        *atnxt;
	struct batch_status *bsnxt;

	while (bsp != (struct batch_status *)NULL) {
		if (bsp->name != (char *)NULL)(void)free(bsp->name);
		if (bsp->text != (char *)NULL)(void)free(bsp->text);
		while (bsp->attribs != (struct attrl *)NULL) {
			if (bsp->attribs->name != (char *)NULL)
				(void)free(bsp->attribs->name);
			if (bsp->attribs->resource != (char *)NULL)
				(void) free(bsp->attribs->resource);
			if (bsp->attribs->value != (char *)NULL)
				(void)free(bsp->attribs->value);
			atnxt = bsp->attribs->next;
			(void)free(bsp->attribs);
			bsp->attribs = atnxt;
		}
		bsnxt = bsp->next;
		(void)free(bsp);
		bsp = bsnxt;
	}
}
