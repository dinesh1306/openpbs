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
#include <pbs_version.h>

#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <pbs_ifl.h>
#include "cmds.h"
#include "net_connect.h"

static struct attrl *attrib = NULL;
static time_t dtstart;
static time_t dtend;

/*
 * @brief process options input to the command.
 *
 * @param[in]  argc  - the number of arguments to be parsed.
 * @param[in]  argv  - the argument array.
 * @param[out] attrp - attribute list sent to the caller.
 * @param[out] dest  - destination server.
 *
 * @retval 0 on no error.
 * @retval No. of options having errors.
 */
int
process_opts(int argc, char **argv, struct attrl **attrp, char *dest)
{
	int	i = 0;
	int	c = 0;
	char	*erp = NULL;
	int	errflg = 0;
	time_t	t;

	char	time_buf[80] = {0};
	char	dur_buf[800] = {0};
	int	opt_re_flg = FALSE;
	char	*endptr = NULL;
	long	temp = 0;

	while ((c = getopt(argc, argv, "E:I:m:M:N:R:q:")) != EOF) {
		switch (c) {
			case 'E':
				opt_re_flg = TRUE;
				t = cvtdate(optarg);
				if (t >= 0) {
					(void)sprintf(time_buf, "%ld", (long)t);
					set_attr(&attrib, ATTR_resv_end, time_buf);
					dtend = t;
				} else {
					fprintf(stderr, "pbs_ralter: illegal -E time value\n");
					errflg++;
				}
				break;

			case 'I':
				temp = strtol(optarg, &endptr, 0);
				if (*endptr == '\0' && temp > 0) {
					set_attr(&attrib, ATTR_inter, optarg);
				} else {
					fprintf(stderr, "pbs_ralter: illegal -I time value\n");
					errflg++;
				}
				break;

			case 'm':
				set_attr(&attrib, ATTR_m, optarg);
				break;

			case 'M':
				set_attr(&attrib, ATTR_M, optarg);
				break;

			case 'N':
				set_attr(&attrib, ATTR_resv_name, optarg);
				break;

			case 'R':
				opt_re_flg = TRUE;
				t = cvtdate(optarg);
				if (t >= 0) {
					(void)sprintf(time_buf, "%ld", (long)t);
					set_attr(&attrib, ATTR_resv_start, time_buf);
					dtstart = t;
				} else {
					fprintf(stderr, "pbs_ralter: illegal -R time value\n");
					errflg++;
				}
				break;

			case 'q':
				/* destination can only be another server */
				if (optarg[0] != '@') {
					fprintf(stderr, "pbs_ralter: illegal -q value: format \"@server\"\n");
					errflg++;
					break;
				}
				strcpy(dest, &optarg[1]);
				break;

			default:
				/* pbs_ralter option not recognized */
				errflg++;

		} /* End of lengthy 'switch on option' constuction */
	}   /* End of lengthy while loop on 'options' */

	*attrp = attrib;
	return (errflg);
}

/*
 * @brief - prints correct usage of the command to the console.
 */

static void
print_usage()
{
	static char usag2[]="       pbs_ralter --version\n";
	static char usage[]=
		"usage: pbs_ralter [-I seconds] [-m mail_points] [-M mail_list]\n"
	"                [-N reservation_name] [-R start_time] [-E end_time]\n"
	"                resv_id\n";
	fprintf(stderr, usage);
	fprintf(stderr, usag2);
}


/**
 * @brief
 * 	handles attribute errors and prints appropriate errmsg
 *
 * @param[in] err_list - list of possible attribute errors
 *
 * @return - Void
 *
 */
static void
handle_attribute_errors(struct ecl_attribute_errors *err_list)
{
	struct attropl	*attribute = NULL;
	char 		*opt = NULL;
	int		i = 0;

	for (i = 0; i < err_list->ecl_numerrors; i++) {
		attribute = err_list->ecl_attrerr[i].ecl_attribute;
		if (strcmp(attribute->name, ATTR_resv_end) == 0)
			opt = "E";
		else if (strcmp(attribute->name, ATTR_inter) == 0)
			opt = "I";
		else if (strcmp(attribute->name, ATTR_m) == 0)
			opt = "m";
		else if (strcmp(attribute->name, ATTR_M) == 0)
			opt = "M";
		else if (strcmp(attribute->name, ATTR_resv_name) == 0)
			opt = "N";
		else if (strcmp(attribute->name, ATTR_resv_start) == 0)
			opt = "R";
		else
			return ;

		CS_close_app();
		fprintf(stderr, "pbs_ralter: illegal -%s value\n", opt);
		print_usage();
		exit(2);
	}
}


int
main(int argc, char *argv[], char *envp[])		/* pbs_ralter */
{
	int		errflg = 0;			/* command line option error */
	int		connect = -1;			/* return from pbs_connect */
	char		*errmsg = NULL;			/* return from pbs_geterrmsg */
	char		destbuf[256] = {0};		/* buffer for option server */
	struct attrl 	*attrib = NULL;			/* the attrib list */
	char		*new_resvname = NULL;		/* the name returned from pbs_modify_resv */
	struct		ecl_attribute_errors *err_list = NULL;
	char		resv_id[PBS_MAXCLTJOBID] = {0};
	char		resv_id_out[PBS_MAXCLTJOBID] = {0};
	char		server_out[MAXSERVERNAME] = {0};
	char		*stat = NULL;

	/*test for real deal or just version and exit*/

	execution_mode(argc, argv);

#ifdef WIN32
	winsock_init();
#endif

	destbuf[0] = '\0';
	errflg = process_opts(argc, argv, &attrib, destbuf); /* get cmdline options */

	if (errflg || ((optind+1) != argc) || argc == 1) {
		print_usage();
		exit(2);
	}

	/*perform needed security library initializations (including none)*/

	if (CS_client_init() != CS_SUCCESS) {
		fprintf(stderr, "pbs_ralter: unable to initialize security library.\n");
		exit(1);
	}

	/* Connect to the server */
	connect = cnt2server(destbuf);
	if (connect <= 0) {
		fprintf(stderr, "pbs_ralter: cannot connect to server %s (errno=%d)\n",
			pbs_server, pbs_errno);
		CS_close_app();
		exit(pbs_errno);
	}

	strcpy(resv_id, argv[optind]);
	if (get_server(resv_id, resv_id_out, server_out)) {
		fprintf(stderr, "pbs_ralter: illegally formed reservation identifier: %s\n", resv_id);
		exit(2);
	}
	stat = pbs_modify_resv(connect, resv_id_out, (struct attropl *)attrib, NULL);

	if (stat == NULL) {
		if ((err_list = pbs_get_attributes_in_error(connect)))
			handle_attribute_errors(err_list);

		errmsg = pbs_geterrmsg(connect);
		if (errmsg != NULL) {
			fprintf(stderr, "pbs_ralter: %s\n", errmsg);
		} else
			fprintf(stderr, "pbs_ralter: Error (%d) modifying reservation\n", pbs_errno);
		CS_close_app();
		exit(pbs_errno);
	} else {
		printf("pbs_ralter: %s\n", stat);
		free(stat);
	}
	/* Disconnet from the server. */
	pbs_disconnect(connect);

	CS_close_app();
	exit(0);
}

