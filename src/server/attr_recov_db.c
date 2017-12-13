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
 * @file    attr_recov_db.c
 *
 * @brief
 *
 * attr_recov_db.c - This file contains the functions to save attributes to the
 *		    database and recover them
 *
 * Included public functions are:
 *	save_attr_db		Save attributes to the database
 *	recov_attr_db		Read attributes from the database
 *	delete_attr_db		Delete a single attribute from the database
 *	make_attr			create a svrattrl structure from the attr_name, and values
 *  recov_attr_db_raw	Recover the list of attributes from the database without triggering
 *						the action routines
 */

#include <pbs_config.h>   /* the master config generated by configure */

#include "pbs_ifl.h"
#include <assert.h>
#include <errno.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "list_link.h"
#include "attribute.h"
#include "log.h"
#include "pbs_nodes.h"
#include "svrfunc.h"
#include "resource.h"
#include "pbs_db.h"


/* Global Variables */
extern int resc_access_perm;

extern struct attribute_def	svr_attr_def[];
extern struct attribute_def	que_attr_def[];

/**
 * @brief
 *	Create a svrattrl structure from the attr_name, and values
 *
 * @param[in]	attr_name - name of the attributes
 * @param[in]	attr_resc - name of the resouce, if any
 * @param[in]	attr_value - value of the attribute
 * @param[in]	attr_flags - Flags associated with the attribute
 *
 * @retval - Pointer to the newly created attribute
 * @retval - NULL - Failure
 * @retval - Not NULL - Success
 *
 */
static svrattrl *
make_attr(char *attr_name, char *attr_resc,
	char *attr_value, int attr_flags)
{
	int tsize;
	char *p;
	svrattrl *psvrat = NULL;

	tsize = sizeof(svrattrl);
	if (!attr_name)
		return NULL;

	tsize += strlen(attr_name) + 1;
	if (attr_resc) tsize += strlen(attr_resc) + 1;
	if (attr_value) tsize += strlen(attr_value) + 1;

	if ((psvrat = (svrattrl *) malloc(tsize)) == 0)
		return NULL;

	CLEAR_LINK(psvrat->al_link);
	psvrat->al_sister = (svrattrl *) 0;
	psvrat->al_atopl.next = 0;
	psvrat->al_tsize = tsize;
	psvrat->al_name = (char *) psvrat + sizeof(svrattrl);
	psvrat->al_resc = 0;
	psvrat->al_value = 0;
	psvrat->al_nameln = 0;
	psvrat->al_rescln = 0;
	psvrat->al_valln = 0;
	psvrat->al_flags = attr_flags;
	psvrat->al_refct = 1;

	strcpy(psvrat->al_name, attr_name);
	psvrat->al_nameln = strlen(attr_name);
	p = psvrat->al_name + psvrat->al_nameln + 1;

	if (attr_resc && strlen(attr_resc) > 0) {
		psvrat->al_resc = p;
		strcpy(psvrat->al_resc, attr_resc);
		psvrat->al_rescln = strlen(attr_resc);
		p = p + psvrat->al_rescln + 1;
	}

	psvrat->al_value = p;
	if (attr_value)
		strcpy(psvrat->al_value, attr_value);
	psvrat->al_valln = strlen(attr_value);

	psvrat->al_op = SET;

	return (psvrat);
}

/**
 * @brief
 *	Save the list of attributes to the database
 *
 * @param[in]	conn - Database connection handle
 * @param[in]	p_attr_info - Information about the database parent
 * @param[in]	padef - Address of parent's attribute definition array
 * @param[in]	pattr - Address of the parent objects attribute array
 * @param[in]	numattr - Number of attributes in the list
 * @param[in]	newparent - Parent is a new object or is being updated boolean
 *
 * @return      Error code
 * @retval	 0  - Success
 * @retval	-1  - Failure
 *
 */
int
save_attr_db(pbs_db_conn_t *conn, pbs_db_attr_info_t *p_attr_info,
	struct attribute_def *padef, struct attribute *pattr,
	int numattr, int newparent)
{
	pbs_list_head	lhead;
	int		i;
	svrattrl	*pal;
	int		rc = 0;
	int		dbrc = 0;
	int		firsttime=1;
	int		attr_count=0;
	pbs_db_obj_info_t obj;
	pbs_db_sql_buffer_t sql;
	pbs_db_sql_buffer_t temp;

	sql.buf_len = 0;
	sql.buff = NULL;

	temp.buf_len = 0;
	temp.buff = NULL;

	/* encode each attribute which has a value (not non-set) */
	CLEAR_HEAD(lhead);

	obj.pbs_db_obj_type = PBS_DB_ATTR;
	obj.pbs_db_un.pbs_db_attr = p_attr_info;

	if (newparent) {
		if (pbs_db_insert_multiattr_start(conn, &obj, &sql) != 0)
			return -1;
	}

	for (i = 0; i < numattr; i++) {

		if (!newparent && !((pattr+i)->at_flags &
			(ATR_VFLAG_MODIFY | ATR_VFLAG_MODCACHE)))
			continue;

		rc = (padef+i)->at_encode(pattr+i, &lhead,
			(padef+i)->at_name,
			(char *)0, ATR_ENCODE_DB, NULL);
		if (rc < 0)
			goto err;

		(pattr+i)->at_flags &= ~ATR_VFLAG_MODIFY;

		/* now that attribute has been encoded, update to db */
		while ((pal = (svrattrl *)GET_NEXT(lhead)) !=
			(svrattrl *)0) {

			strcpy(p_attr_info->attr_name, pal->al_atopl.name);
			if (pal->al_atopl.resource)
				p_attr_info->attr_resc = pal->al_atopl.resource;
			else
				p_attr_info->attr_resc = "";
			p_attr_info->attr_value = pal->al_atopl.value;
			p_attr_info->attr_flags = pal->al_flags;
			attr_count++;
#ifdef DEBUG
			printf("%s.%s=%s, flags=%d\n",
				p_attr_info->attr_name,
				p_attr_info->attr_resc,
				p_attr_info->attr_value,
				p_attr_info->attr_flags);
			fflush(stdout);
#endif

			if (newparent) {
				dbrc = pbs_db_insert_multiattr_add(conn, &obj,
					firsttime, &sql, &temp);
				if (dbrc != 0)
					goto err;
				firsttime = 0;
			} else {
				dbrc = pbs_db_update_obj(conn, &obj);
				if (dbrc == 1) /* no rows affected */
					dbrc = pbs_db_insert_obj(conn, &obj);
				if (dbrc != 0)
					goto err;
			}

			delete_link(&pal->al_link);
			(void)free(pal);
		}
	}

	if (newparent) {
		if (attr_count > 0)
			dbrc = pbs_db_insert_multiattr_execute(conn, &obj, &sql);
	}

err:
	if (sql.buff != NULL)
		free(sql.buff);
	if (temp.buff != NULL)
		free(temp.buff);

	return ((rc < 0 || dbrc !=0)? -1 : 0);
}

/**
 * @brief
 *	Recover the list of attributes from the database
 *
 * @param[in]	conn - Database connection handle
 * @param[in]	parent - Address of parent object
 * @param[in]	p_attr_info - Information about the database parent
 * @param[in]	padef - Address of parent's attribute definition array
 * @param[in]	pattr - Address of the parent objects attribute array
 * @param[in]	limit - Number of attributes in the list
 * @param[in]	unknown	- The index of the unknown attribute if any
 *
 * @return      Error code
 * @retval	 0  - Success
 * @retval	-1  - Failure
 */
int
recov_attr_db(pbs_db_conn_t *conn,
	void *parent,
	pbs_db_attr_info_t *p_attr_info,
	struct attribute_def *padef,
	struct attribute *pattr,
	int limit,
	int unknown)
{
//	static	  char	id[] = "recov_attr";
	int	  amt;
	int	  index;
	svrattrl *pal = (svrattrl *)0;
	svrattrl *tmp_pal = (svrattrl *)0;
	int	  ret;
	void	 *state = NULL;
	pbs_db_obj_info_t obj;
	void **palarray = NULL;

	if ((palarray = calloc(limit, sizeof(void *))) == NULL) {
		log_err(-1,__func__, "Out of memory");
		return -1;
	}

	/* set all privileges (read and write) for decoding resources	*/
	/* This is a special (kludge) flag for the recovery case, see	*/
	/* decode_resc() in lib/Libattr/attr_fn_resc.c			*/

	resc_access_perm = ATR_DFLAG_ACCESS;

	/* For each attribute, read in the attr_extern header */
	obj.pbs_db_obj_type = PBS_DB_ATTR;
	obj.pbs_db_un.pbs_db_attr = p_attr_info;
	state = pbs_db_cursor_init(conn, &obj, NULL);
	if (!state) {
		free(palarray);
		return -1;
	}

	while (1) {
		ret = pbs_db_cursor_next(conn, state, &obj);
		if (ret != 0)
			break;		/* end of attributes in DB or error */

		/* Below ensures that a server or queue resource is not set */
		/* if that resource is not known to the current server. */
		if ( (p_attr_info->attr_resc != NULL) && \
				(strlen(p_attr_info->attr_resc) > 0) && \
		     ((padef == svr_attr_def) || (padef == que_attr_def)) ) {
			resource_def	*prdef;

			prdef = find_resc_def(svr_resc_def,
				p_attr_info->attr_resc, svr_resc_size);
			if (prdef == (resource_def *)0) {
				snprintf(log_buffer, sizeof(log_buffer),
					"%s's unknown resource \"%s.%s\" ignored",
					((padef == svr_attr_def)?"server":"queue"),
					p_attr_info->attr_name,
					p_attr_info->attr_resc);
				log_err(-1,__func__, log_buffer);
				continue;
			}
		}

		pal = make_attr(p_attr_info->attr_name,
			p_attr_info->attr_resc,
			p_attr_info->attr_value,
			p_attr_info->attr_flags);

		/* Return when make_attr fails to create a svrattrl structure */
		if (pal == NULL) {
			log_err(-1,__func__, "Out of memory");
			free(palarray);
			return -1;
		}

		amt = pal->al_tsize - sizeof(svrattrl);
		if (amt < 1) {
			sprintf(log_buffer, "Invalid attr list size in DB");
			log_err(-1,__func__, log_buffer);
			(void)free(pal);
			ret = -1;
			break;
		}
		CLEAR_LINK(pal->al_link);

		pal->al_refct = 1;	/* ref count reset to 1 */

		/* find the attribute definition based on the name */
		index = find_attr(padef, pal->al_name, limit);
		if (index < 0) {

			/*
			 * There are two ways this could happen:
			 * 1. if the (job) attribute is in the "unknown" list -
			 *    keep it there;
			 * 2. if the server was rebuilt and an attribute was
			 *    deleted, -  the fact is logged and the attribute
			 *    is discarded (system,queue) or kept (job)
			 */
			if (unknown > 0) {
				index = unknown;
			} else {
				sprintf(log_buffer,
					"unknown attribute \"%s\" discarded",
					pal->al_name);
				log_err(-1,__func__, log_buffer);
				(void)free(pal);
				continue;
			}
		}
		if (palarray[index] == NULL)
			palarray[index] = pal;
		else {
			tmp_pal = palarray[index];
			while (tmp_pal->al_sister)
				tmp_pal = tmp_pal->al_sister;

			/* this is the end of the list of attributes */
			tmp_pal->al_sister = pal;
		}
	}
	pbs_db_cursor_close(conn, state);

	if (ret == -1) {
		/*
		 * some error happened above
		 * Error has already been logged
		 * so just free palarray indexes and return
		 */
		for (index = 0; index < limit; index++)
			if (palarray[index])
				free(palarray[index]);
		free(palarray);
		return -1;
	}

	/* now do the decoding */
	for (index = 0; index < limit; index++) {
		/*
		 * In the normal case we just decode the attribute directly
		 * into the real attribute since there will be one entry only
		 * for that attribute.
		 *
		 * However, "entity limits" are special and may have multiple,
		 * the first of which is "SET" and the following are "INCR".
		 * For the SET case, we do it directly as for the normal attrs.
		 * For the INCR,  we have to decode into a temp attr and then
		 * call set_entity to do the INCR.
		 */
		/*
		 * we don't store the op value into the database, so we need to
		 * determine (in case of an ENTITY) whether it is the first
		 * value, or was decoded before. We decide this based on whether
		 * the flag has ATR_VFLAG_SET
		 *
		 */
		pal = palarray[index];
		while (pal) {
			if (((padef + index)->at_type == ATR_TYPE_ENTITY) &&
				((pattr + index)->at_flags & ATR_VFLAG_SET)) {
				attribute tmpa;
				memset(&tmpa, 0, sizeof(attribute));
				/* for INCR case of entity limit, decode locally */
				if ((padef+index)->at_decode) {
					(void)(padef+index)->at_decode(&tmpa,
						pal->al_name,
						pal->al_resc,
						pal->al_value);
					(void)(padef+index)->at_set(pattr+index,
						&tmpa,
						INCR);
					(void)(padef+index)->at_free(&tmpa);
				}
			} else {
				if ((padef+index)->at_decode) {
					(void)(padef+index)->at_decode(pattr+index,
						pal->al_name,
						pal->al_resc,
						pal->al_value);
					if ((padef+index)->at_action)
						(void)(padef+index)->at_action(
							pattr+index, parent,
							ATR_ACTION_RECOV);
				}
			}
			(pattr+index)->at_flags = pal->al_flags & ~ATR_VFLAG_MODIFY;

			tmp_pal = pal->al_sister;
			(void)free(pal);
			pal = tmp_pal;
		}
	}
	(void)free(palarray);

	return (0);
}

/**
 * @brief
 *	Recover the list of attributes from the database without triggering
 *	the action routines. This is required for loading node attributes.
 *
 * @param[in]	conn - Database connection handle
 * @param[in]	p_attr_info - Information about the database parent
 * @param[in]	phead - The pbs_list_head to which to append attributes to
 *
 * @return      Error code
 * @retval	 0  - Success
 * @retval	-1  - Failure
 */
int
recov_attr_db_raw(pbs_db_conn_t *conn,
	pbs_db_attr_info_t *p_attr_info,
	pbs_list_head *phead)
{
//	static	  char	id[] = "recov_attr";
	int	  amt;
	int	  ret;
	svrattrl *pal = NULL;
	void	 *state = NULL;
	pbs_db_obj_info_t obj;

	/* set all privileges (read and write) for decoding resources	*/
	/* This is a special (kludge) flag for the recovery case, see	*/
	/* decode_resc() in lib/Libattr/attr_fn_resc.c			*/

	resc_access_perm = ATR_DFLAG_ACCESS;

	/* For each attribute, read in the attr_extern header */
	obj.pbs_db_obj_type = PBS_DB_ATTR;
	obj.pbs_db_un.pbs_db_attr = p_attr_info;
	state = pbs_db_cursor_init(conn, &obj, NULL);
	if (!state)
		return -1;

	while (1) {
		ret = pbs_db_cursor_next(conn, state, &obj);
		if (ret == -1) {
			if (pal)
				free(pal);
			pbs_db_cursor_close(conn, state);
			return -1;
		}
		if (ret == 1)
			break;		/* end of attributes in DB */

		pal = make_attr(p_attr_info->attr_name,
			p_attr_info->attr_resc,
			p_attr_info->attr_value,
			p_attr_info->attr_flags);

		/* Return when make_attr fails to create a svrattrl structure */
		if (pal == NULL) {
			log_err(-1,__func__, "Out of memory");
			pbs_db_cursor_close(conn, state);
			return -1;
		}

		amt = pal->al_tsize - sizeof(svrattrl);
		if (amt < 1) {
			sprintf(log_buffer, "Invalid attr list size in DB");
			log_err(-1,__func__, log_buffer);
			free(pal);
			return (-1);
		}
		CLEAR_LINK(pal->al_link);
		pal->al_refct = 1;	/* ref count reset to 1 */
		append_link(phead, &pal->al_link, pal);
	}
	pbs_db_cursor_close(conn, state);
	return (0);
}


/**
 * @brief
 *	Delete an attribute from the database
 *
 * @param[in]	conn - Database connection handle
 * @param[in]	p_attr_info - Information about the database parent
 * @param[in]	pal - The attribute to delete
 *
 * @return      Error code
 * @retval	 0  - Success
 * @retval	-1  - Failure
 * @retval	 1 -  Success but no attibute deleted
 */
int
delete_attr_db(pbs_db_conn_t *conn,
	pbs_db_attr_info_t *p_attr_info,
	struct svrattrl *pal)
{
	pbs_db_obj_info_t obj;

	obj.pbs_db_obj_type = PBS_DB_ATTR;
	obj.pbs_db_un.pbs_db_attr = p_attr_info;

	strcpy(p_attr_info->attr_name, pal->al_atopl.name);
	if (pal->al_atopl.resource)
		p_attr_info->attr_resc = pal->al_atopl.resource;
	else
		p_attr_info->attr_resc = "";


	return (pbs_db_delete_obj(conn, &obj));
}
