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
 * @file	diswcs.c
 *
 * @par Synopsis:
 *	int diswcs(int stream, char *value, size_t nchars)
 *
 *	Converts a counted string in *<value> into a Data-is-Strings character
 *	string and sends it to <stream>.  The character string in <stream>
 *	consists of the unsigned integer representation of <nchars>, followed by
 *	<nchars> characters from *<value>.
 *
 *	Returns DIS_SUCCESS if everything works well.  Returns an error code
 *	otherwise.  In case of an error, no characters are sent to <stream>.
 */

#include <pbs_config.h>   /* the master config generated by configure */

#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "dis.h"
#include "dis_.h"

/**
 * @brief
 *	Converts a counted string in *<value> into a Data-is-Strings character
 *      string and sends it to <stream>.
 *
 * @param[in] stream - pointer to data stream
 * @param[in] value - value to be converted
 * @param[in] nchars - size of chars
 *
 * @return	int
 * @return	DIS_SUCCESS	success
 * @retval	error code	error
 *
 */
int
diswcs(int stream, const char *value, size_t nchars)
{
	int		retval;

	assert(disw_commit != NULL);
	assert(dis_puts != NULL);
	assert(nchars <= UINT_MAX);

	retval = diswui_(stream, (unsigned)nchars);
	if (retval == DIS_SUCCESS && nchars > 0 &&
		(*dis_puts)(stream, value, nchars) != nchars)
		retval = DIS_PROTO;
	return (((*disw_commit)(stream, retval == DIS_SUCCESS) < 0) ?
		DIS_NOCOMMIT : retval);
}
