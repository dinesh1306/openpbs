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
 * @file	munge_supp.c
 * @brief
 *  Utility functions to support munge authentication
 */

#include <pbs_config.h>   /* the master config generated by configure */

#include <libutil.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <pbs_ifl.h>
#include <pbs_internal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#ifndef WIN32
#include <dlfcn.h>
#include <grp.h>
#endif
#include "pbs_error.h"



#ifndef WIN32
const char libmunge[] = "libmunge.so";  /* MUNGE library */
void *munge_dlhandle; /* MUNGE dynamic loader handle */
int (*munge_encode_ptr)(char **, void *, const void *, int); /* MUNGE munge_encode() function pointer */
int (*munge_decode_ptr)(const char *cred, void *, void **, int *, uid_t *, gid_t *); /* MUNGE munge_decode() function pointer */
char * (*munge_strerror_ptr) (int); /* MUNGE munge_stderror() function pointer */

/**
 * @brief
 *      Check if libmunge.so shared library is present in the system
 *      and assign specific function pointers to be used at the time
 *      of decode or encode.
 *
 * @param[in/out] ebuf	Error message is updated here
 * @param[in] ebufsz	size of the error message buffer
 *
 * @return int
 * @retval  0 on success
 * @retval -1 on failure
 */
int
init_munge(char *ebuf, int ebufsz)
{
        munge_dlhandle = dlopen(libmunge, RTLD_LAZY);
        if (munge_dlhandle == NULL) {
            snprintf(ebuf, ebufsz, "%s not found", libmunge);
            goto err;
        }

        munge_encode_ptr = dlsym(munge_dlhandle, "munge_encode");
        if (munge_encode_ptr == NULL) {
        	snprintf(ebuf, ebufsz, "symbol munge_encode not found in %s", libmunge);
            goto err;
        }

        munge_decode_ptr = dlsym(munge_dlhandle, "munge_decode");
        if (munge_decode_ptr == NULL) {
        	snprintf(ebuf, ebufsz, "symbol munge_decode not found in %s", libmunge);
            goto err;
        }

        munge_strerror_ptr = dlsym(munge_dlhandle, "munge_strerror");
        if (munge_strerror_ptr == NULL) {
        	snprintf(ebuf, ebufsz, "symbol munge_strerror not found in %s", libmunge);
            goto err;
        }
        /*
         * Don't close the munge handler as it will be used
         * further for encode or decode
         */
        return (0);

err:
	if (munge_dlhandle)
		dlclose(munge_dlhandle);

	munge_dlhandle = NULL;
	munge_encode_ptr = NULL;
	munge_decode_ptr = NULL;
	munge_strerror_ptr = NULL;
	return (-1);
}

/**
 * @brief
 *      Call Munge encode API's to get the authentication
 *      data for the current user
 *
 * @param[in] fromsvr	connection initiated from server?
 * @param[in/out] ebuf	Error message is updated here
 * @param[in] ebufsz	size of the error message buffer
 *
 * @return  The encoded munge authentication data
 * @retval   !NULL on success
 * @retval  NULL on failure
 */
char *
pbs_get_munge_auth_data(int fromsvr, char *ebuf, int ebufsz)
{
	char *cred = NULL;
	uid_t myrealuid;
	struct passwd *pwent;
	struct group *grp;
	char payload[2 + PBS_MAXUSER + PBS_MAXGRPN + 1] = { '\0' };
	int munge_err = 0;

	if (munge_dlhandle == NULL) {
		if (init_munge(ebuf, ebufsz) != 0) {
			pbs_errno = PBSE_SYSTEM;
			goto err;
		}
	}

	myrealuid = getuid();
	pwent = getpwuid(myrealuid);
	if (pwent == (struct passwd *) 0) {
		snprintf(ebuf, ebufsz, "Failed to obtain user-info for uid = %d", myrealuid);
		pbs_errno = PBSE_SYSTEM;
		goto err;
	}

	grp = getgrgid(pwent->pw_gid);
	if (grp == (struct group *) 0) {
		snprintf(ebuf, ebufsz, "Failed to obtain group-info for gid=%d", pwent->pw_gid);
		pbs_errno = PBSE_SYSTEM;
		goto err;
	}

	/* if the connection is being initiated from server, encode a 1 before the user:grp */
	snprintf(payload, PBS_MAXUSER + PBS_MAXGRPN + 2, "%c:%s:%s", (fromsvr ? '1' : '0'), pwent->pw_name, grp->gr_name);

	munge_err = munge_encode_ptr(&cred, NULL, payload, strlen(payload));
	if (munge_err != 0) {
		pbs_errno = PBSE_BADCRED;
		snprintf(ebuf, ebufsz, "MUNGE user-authentication on encode failed with `%s`", munge_strerror_ptr(munge_err));
		goto err;
	}
	return cred;

err:
	free(cred);
	return (NULL);
}


/**
 * @brief
 *      Validate the munge authentication data
 *
 * @param[in]  auth_data	opaque auth data that is to be verified
 * @param[out] from_svr		1 - sender is server, 0 - sender not a server
 * @param[in/out] ebuf		Error message is updated here
 * @param[in] ebufsz		size of the error message buffer
 *
 * @return error code
 * @retval  0 - Success
 * @retval -1 - Failure
 *
 */
int
pbs_munge_validate(void *auth_data, int *fromsvr, char *ebuf, int ebufsz)
{
	uid_t uid;
	gid_t gid;
	int recv_len = 0;
	struct passwd *pwent = NULL;
	struct group *grp = NULL;
	void *recv_payload = NULL;
	char user_credential[PBS_MAXUSER + PBS_MAXGRPN + 1] = { "\0" };
	int munge_err = 0;
	char *p;
	int rc = -1;

	*fromsvr = 0;

	if (munge_dlhandle == NULL) {
		if (init_munge(ebuf, ebufsz) != 0) {
			pbs_errno = PBSE_SYSTEM;
			goto err;
		}
	}

	munge_err = munge_decode_ptr(auth_data, NULL, &recv_payload, &recv_len, &uid, &gid);
	if (munge_err != 0) {
		snprintf(ebuf, ebufsz, "MUNGE user-authentication on decode failed with `%s`",
					munge_strerror_ptr(munge_err));
		goto err;
	}

	if ((pwent = getpwuid(uid)) == NULL) {
		snprintf(ebuf, ebufsz, "Failed to obtain user-info for uid = %d", uid);
		goto err;
	}

	if ((grp = getgrgid(pwent->pw_gid)) == NULL) {
		snprintf(ebuf, ebufsz, "Failed to obtain group-info for gid=%d", gid);
		goto err;
	}
	snprintf(user_credential, PBS_MAXUSER + PBS_MAXGRPN, "%s:%s", pwent->pw_name, grp->gr_name);

	/* parse the recv_payload past the first two characters */
	p = (char *) recv_payload;
	if (*p == '1')
		*fromsvr = 1; /* connection was from a server */

	p = recv_payload + 2;

	if (strcmp(user_credential, p) == 0)
		rc = 0;
	else
		snprintf(ebuf, ebufsz, "User credentials do not match");

err:
	/* Don't close the munge handler */
	free(recv_payload);
	return rc;
}
#endif
