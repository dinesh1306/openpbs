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
 * @file	uLTostr.c
 *
 * @brief
 * 	uLTostr -  returns a pointer to the character string representation of the
 *	u_Long, value, represented in base, base.
 */
#include <pbs_config.h>   /* the master config generated by configure */

#include <string.h>
#include <errno.h>
#include "Long.h"
#include "Long_.h"

#define LBUFSIZ (CHAR_BIT * sizeof(u_Long) + 2)

static char buffer[LBUFSIZ];
static const char Long_dig[] = LONG_DIG_VALUE;

/**
 * @brief
 * 	uLTostr -  returns a pointer to the character string representation of the
 *	u_Long, value, represented in base, base.
 *
 *	If base is outside its domain of 2 through the number of characters in
 *	the Long_dig array, uLTostr returns a zero-length string and sets errno
 *	to EDOM.
 *
 *	The string is stored in a static array and will be clobbered the next
 *	time either LTostr or uLTostr is called.  The price of eliminating the
 *	possibility of memory leaks is the necessity to copy the string
 *	immediately to a safe place if it must last.
 *
 * @param[in] value - u_Long value
 * @param[in] base - base representation of val
 *
 * @return      string
 * @retval      char reprsn of u_Long     Success
 * @retval      NULL                      error
 *
 */

const char *
uLTostr(u_Long value, int base)
{
	char		*bp = &buffer[LBUFSIZ];

	*--bp = '\0';
	if (base < 2 || base > strlen(Long_dig)) {
		errno = EDOM;
		return (bp);
	}
	do {
		*--bp = Long_dig[value % base];
		value /= base;
	} while (value);
	switch (base) {
		case 16:
			*--bp = 'x';
		case 8:
			*--bp = '0';
	}
	return (bp);
}
