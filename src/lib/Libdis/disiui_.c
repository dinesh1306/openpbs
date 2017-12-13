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

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include "dis.h"
#include "dis_.h"

char *dis_umax = NULL;
unsigned dis_umaxd = 0;
/**
 * @file	disiui_.c
 */
/**
 * @brief
 *	Allocate and fill a counted string containing the constant, UINT_MAX,
 *	expressed as character codes of decimal digits.
 *
 * @par SEE:
 *      dis_umaxd = number of digits in UINT_MAX\n
 *	dis_umax[0] through dis_umax[dis_umaxd - 1] = the digits, in order.
 */

void
disiui_()
{
	char		*cp;

	assert(dis_umax == NULL);
	assert(dis_umaxd == 0);

	cp = discui_(dis_buffer + DIS_BUFSIZ, UINT_MAX, &dis_umaxd);
	assert(dis_umaxd > 0);
	dis_umax = (char *)malloc(dis_umaxd);
	assert(dis_umax != NULL);
	memcpy(dis_umax, cp, dis_umaxd);
}
