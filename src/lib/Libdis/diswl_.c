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
 * @file	diswl_.c
 *
 * @par Synopsis:
 * 	int diswl_(int stream, dis_long_double_t value, unsigned int ndigs)
 *
 *	Converts <value> into a Data-is-Strings floating point number and sends
 *	it to <stream>.  The converted number consists of two consecutive signed
 *	integers.  The first is the coefficient, at most <ndigs> long, with its
 *	implied decimal point at the low-order end.  The second is the exponent
 *	as a power of 10.
 *
 *	This function is only invoked through the macros, diswf, diswd, and
 *	diswl, which are defined in the header file, dis.h.
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
 *      Converts <value> into a Data-is-Strings floating point number and sends
 *      it to <stream>.
 *
 * @param[in] stream    socket fd
 * @param[in] ndigs	length of number
 * @param[in] value     value to be converted
 *
 * @return      int
 * @retval      DIS_SUCCESS     success
 * @retval      error code      error
 *
 */
int
diswl_(int stream, dis_long_double_t value, unsigned ndigs)
{
	int		c;
	int		expon;
	int		negate;
	int		retval;
	unsigned	pow2;
	char		*cp;
	char		*ocp;
	dis_long_double_t	ldval;

	assert(ndigs > 0 && ndigs <= LDBL_DIG);
	assert(stream >= 0);
	assert(dis_puts != NULL);
	assert(disw_commit != NULL);

	/* Make zero a special case.  If we don't it will blow exponent		*/
	/* calculation.								*/
	if (value == 0.0L) {
		retval = (*dis_puts)(stream, "+0+0", 4) < 0 ?
			DIS_PROTO : DIS_SUCCESS;
		return (((*disw_commit)(stream, retval == DIS_SUCCESS) < 0) ?
			DIS_NOCOMMIT : retval);
	}
	/* Extract the sign from the coefficient.				*/
	ldval = (negate = value < 0.0L) ? -value : value;
	/* Detect and complain about the infinite form.				*/
	if (ldval > LDBL_MAX)
		return (DIS_HUGEVAL);
	/* Compute the integer part of the log to the base 10 of ldval.  As a	*/
	/* byproduct, reduce the range of ldval to the half-open interval,      */
	/* [1, 10).								*/

	/* dis_lmx10 would be initialized by prior call to dis_init_tables */
	expon = 0;
	pow2 = dis_lmx10 + 1;
	if (ldval < 1.0L) {
		do {
			if (ldval < dis_ln10[--pow2]) {
				ldval *= dis_lp10[pow2];
				expon += 1 << pow2;
			}
		} while (pow2);
		ldval *= 10.0;
		expon = -expon - 1;
	} else {
		do {
			if (ldval >= dis_lp10[--pow2]) {
				ldval *= dis_ln10[pow2];
				expon += 1 << pow2;
			}
		} while (pow2);
	}
	/* Round the value to the last digit					*/
	ldval += 5.0L * disp10l_(-ndigs);
	if (ldval >= 10.0L) {
		expon++;
		ldval *= 0.1L;
	}
	/* Starting in the middle of the buffer, convert coefficient digits,	*/
	/* most significant first.						*/
	ocp = cp = &dis_buffer[DIS_BUFSIZ - ndigs];
	do {
		c = ldval;
		ldval = (ldval - c) * 10.0L;
		*ocp++ = c + '0';
	} while (--ndigs);
	/* Eliminate trailing zeros.						*/
	while (*--ocp == '0');
	/* The decimal point is at the low order end of the coefficient		*/
	/* integer, so adjust the exponent for the number of digits in the	*/
	/* coefficient.								*/
	ndigs = ++ocp - cp;
	expon -= ndigs - 1;
	/* Put the coefficient sign into the buffer, left of the coefficient.	*/
	*--cp = negate ? '-' : '+';
	/* Insert the necessary number of counts on the left.			*/
	while (ndigs > 1)
		cp = discui_(cp, ndigs, &ndigs);
	/* The complete coefficient integer is done.  Put it out.		*/
	retval = (*dis_puts)(stream, cp, (size_t)(ocp - cp)) < 0 ?
		DIS_PROTO : DIS_SUCCESS;
	/* If that worked, follow with the exponent, commit, and return.	*/
	if (retval == DIS_SUCCESS)
		return (diswsi(stream, expon));
	/* If coefficient didn't work, negative commit and return the error.	*/
	return (((*disw_commit)(stream, FALSE) < 0)  ? DIS_NOCOMMIT : retval);
}
