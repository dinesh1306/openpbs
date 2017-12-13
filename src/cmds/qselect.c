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
 * @file	qselect.c
 * @brief
 * 	qselect - (PBS) select batch job
 *
 * @author     	Terry Heidelberg
 *				Livermore Computing
 *
 * @author     	Bruce Kelly
 * 				National Energy Research Supercomputer Center
 *
 * @author     	Lawrence Livermore National Laboratory
 * 				University of California
 */

#include "cmds.h"
#include <pbs_config.h>   /* the master config generated by configure */
#include <pbs_version.h>


/**
 * @brief
 *	sets the attribute details
 *
 * @param[out] list - attribute list
 * @param[in] a_name - attribute name
 * @param[in] r_name - resource name
 * @param[in] v_name - value for the attribute
 *
 * @return Void
 *
 */ 
void
set_attrop(struct attropl **list, char *a_name, char *r_name, char *v_name, enum batch_op op)
{
	struct attropl *attr;

	attr = (struct attropl *) malloc(sizeof(struct attropl));
	if (attr == NULL) {
		fprintf(stderr, "qselect: out of memory\n");
		exit(2);
	}
	if (a_name == NULL)
		attr->name = NULL;
	else {
		attr->name = (char *) malloc(strlen(a_name)+1);
		if (attr->name == NULL) {
			fprintf(stderr, "qselect: out of memory\n");
			exit(2);
		}
		strcpy(attr->name, a_name);
	}
	if (r_name == NULL)
		attr->resource = NULL;
	else {
		attr->resource = (char *) malloc(strlen(r_name)+1);
		if (attr->resource == NULL) {
			fprintf(stderr, "qselect: out of memory\n");
			exit(2);
		}
		strcpy(attr->resource, r_name);
	}
	if (v_name == NULL)
		attr->value = NULL;
	else {
		attr->value = (char *) malloc(strlen(v_name)+1);
		if (attr->value == NULL) {
			fprintf(stderr, "qselect: out of memory\n");
			exit(2);
		}
		strcpy(attr->value, v_name);
	}
	attr->op = op;
	attr->next = *list;
	*list = attr;
	return;
}



#define OPSTRING_LEN 4
#define OP_LEN 2
#define OP_ENUM_LEN 6
static char *opstring_vals[] = { "eq", "ne", "ge", "gt", "le", "lt" };
static enum batch_op opstring_enums[] = { EQ, NE, GE, GT, LE, LT };

/**
 * @brief
 *	processes the argument string and checks the operation 
 *	to be performed 
 *
 * @param[in] optarg -  argument string
 * @param[out] op    -  enum value
 * @param[out] optargout  -  char pointer to hold output
 *
 */
void
check_op(char *optarg, enum batch_op *op, char *optargout)
{
	char opstring[OP_LEN+1];
	int i;
	int cp_pos;

	*op = EQ;   /* default */
	cp_pos = 0;

	if (optarg[0] == '.') {
		strncpy(opstring, &optarg[1], OP_LEN);
		opstring[OP_LEN] = '\0';
		cp_pos = OPSTRING_LEN;
		for (i=0; i<OP_ENUM_LEN; i++) {
			if (strncmp(opstring, opstring_vals[i], OP_LEN) == 0) {
				*op = opstring_enums[i];
				break;
			}
		}
	}
	strcpy(optargout, &optarg[cp_pos]);
	return;
}

/**
 * @brief
 *	Function name: get_tsubopt()
 * 	To parse the optarg and get the sub option to -t option
 * 	and populate the appropriate time atrr in attr_t.
 *
 * @param[in]  opt the first character of the "optarg" string.
 * @param[out] attr_t string pointer to be populated with appropriate
 * 		 job level time attribute.
 * @param[out] resc_t string pointer to be populated with appropriate
 *		 resource associated with attr_t if necessary (NULL if not).
 *
 * @retval 0 for SUCCESS
 * @retval -1 for FAILURE
 *
 * @par Scope of Linkage: local
 *
 */

static int
get_tsubopt(char opt, char **attr_t, char **resc_t)
{
	*resc_t = NULL;
	switch (opt) {
		case 'a':
			*attr_t = ATTR_a;
			break;
		case 'c':
			*attr_t = ATTR_ctime;
			break;
		case 'e':
			*attr_t = ATTR_etime;
			break;
		case 'g':
			*attr_t = ATTR_eligible_time;
			break;
		case 'm':
			*attr_t = ATTR_mtime;
			break;
		case 'q':
			*attr_t = ATTR_qtime;
			break;
		case 's':
			*attr_t = ATTR_stime;
			break;
		case 't':
			*attr_t = ATTR_estimated;
			*resc_t = "start_time";
			break;
		default:
			return -1; /* failure */
	}
	return 0; /* success */
}
/**
 * @brief
 *      To parse the optarg and check the option to -l option
 *      and populate the appropriate resource value in resource_list.
 *
 * @param[in]  optarg -  string containing resource info
 * @param[out] resource_name - string to hold resource name
 * @param[out] op - enum value to hold option 
 * @param[out] resource_value - string to hold resource value
 * @param[out] res_pos - string holding the resource info
 *
 * @retval 0 for SUCCESS
 * @retval 1 for FAILURE
 *
 */
int
check_res_op(char *optarg, char *resource_name, enum batch_op *op, char *resource_value, char **res_pos)
{
	char opstring[OPSTRING_LEN];
	int i;
	int hit;
	char *p;

	p = strchr(optarg, '.');
	if (p == NULL || *p == '\0') {
		fprintf(stderr, "qselect: illegal -l value\n");
		fprintf(stderr, "resource_list: %s\n", optarg);
		return (1);
	}
	else {
		strncpy(resource_name, optarg, p-optarg);
		resource_name[p-optarg] = '\0';
		*res_pos = p + OPSTRING_LEN;
	}
	if (p[0] == '.') {
		strncpy(opstring, &p[1] , OP_LEN);
		opstring[OP_LEN] = '\0';
		hit = 0;
		for (i=0; i<OP_ENUM_LEN; i++) {
			if (strncmp(opstring, opstring_vals[i], OP_LEN) == 0) {
				*op = opstring_enums[i];
				hit = 1;
				break;
			}
		}
		if (! hit) {
			fprintf(stderr, "qselect: illegal -l value\n");
			fprintf(stderr, "resource_list: %s\n", optarg);
			return (1);
		}
	}
	p = strchr(*res_pos, ',');
	if (p == NULL) {
		p = strchr(*res_pos, '\0');
	}
	strncpy(resource_value, *res_pos, p-(*res_pos));
	resource_value[p-(*res_pos)] = '\0';
	if (strlen(resource_value) == 0) {
		fprintf(stderr, "qselect: illegal -l value\n");
		fprintf(stderr, "resource_list: %s\n", optarg);
		return (1);
	}
	*res_pos =  (*p == '\0') ? p : (p += 1) ;
	if (**res_pos == '\0' && *(p-1) == ',') {
		fprintf(stderr, "qselect: illegal -l value\n");
		fprintf(stderr, "resource_list: %s\n", optarg);
		return (1);
	}

	return (0);  /* ok */
}

/**
 * @brief
 * 	prints usage format for qselect command
 *
 * @return - Void
 *
 */
static void
print_usage()
{
	static char usag2[]="       qselect --version\n";
	static char usage[]=
		"usage: qselect [-a [op]date_time] [-A account_string] [-c [op]interval]\n"
	"\t[-h hold_list] [-H] [-J] [-l resource_list] [-N name] [-p [op]priority]\n"
	"\t[-q destination] [-r y|n] [-s states] [-t subopt[op]date_time] [-T] [-P project_name]\n"
	"\t[-x] [-u user_name]\n";
	fprintf(stderr, "%s", usage);
	fprintf(stderr, "%s", usag2);
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
	struct attropl *attribute;
	char * opt;
	int i;

	for (i = 0; i < err_list->ecl_numerrors; i++) {
		attribute = err_list->ecl_attrerr[i].ecl_attribute;
		if (strcmp(attribute->name, ATTR_a) == 0)
			opt = "a";
		else if (strcmp(attribute->name, ATTR_project) == 0)
			opt = "P";
		else if (strcmp(attribute->name, ATTR_A) == 0)
			opt = "A";
		else if (strcmp(attribute->name, ATTR_c) == 0)
			opt = "c";
		else if (strcmp(attribute->name, ATTR_h) == 0)
			opt = "h";
		else if (strcmp(attribute->name, ATTR_array) == 0)
			opt = "J";
		else if (strcmp(attribute->name, ATTR_l) == 0) {
			opt = "l";
			fprintf(stderr, "qselect: %s\n",
				err_list->ecl_attrerr[i].ecl_errmsg);
			exit(err_list->ecl_attrerr[i].ecl_errcode);
		} else if (strcmp(attribute->name, ATTR_N) == 0)
			opt = "N";
		else if (strcmp(attribute->name, ATTR_p) == 0) {
			opt = "p";
			fprintf(stderr, "qselect: %s\n",
				err_list->ecl_attrerr[i].ecl_errmsg);
			exit(err_list->ecl_attrerr[i].ecl_errcode);
		} else if (strcmp(attribute->name, ATTR_q) == 0)
			opt = "q";
		else if (strcmp(attribute->name, ATTR_r) == 0)
			opt = "r";
		else if (strcmp(attribute->name, ATTR_state) == 0)
			opt = "s";
		else if (strcmp(attribute->name, ATTR_ctime) == 0)
			opt = "t";
		else if (strcmp(attribute->name, ATTR_etime) == 0)
			opt = "t";
		else if (strcmp(attribute->name, ATTR_eligible_time) == 0)
			opt = "t";
		else if (strcmp(attribute->name, ATTR_mtime) == 0)
			opt = "t";
		else if (strcmp(attribute->name, ATTR_qtime) == 0)
			opt = "t";
		else if (strcmp(attribute->name, ATTR_stime) == 0)
			opt = "t";
		else if (strcmp(attribute->name, ATTR_u) == 0)
			opt = "u";
		else
			return;

		fprintf(stderr, "qselect: illegal -%s value\n", opt);
		print_usage();
		/*cleanup security library initializations before exiting*/
		CS_close_app();
		exit(2);
	}
}


int
main(int argc, char **argv, char **envp) /* qselect */
{
	int c;
	int errflg=0;
	char *errmsg;

#define MAX_OPTARG_LEN 256
#define MAX_RESOURCE_NAME_LEN 256
	char optargout[MAX_OPTARG_LEN+1];
	char resource_name[MAX_RESOURCE_NAME_LEN+1];

	enum batch_op op;
	enum batch_op *pop = &op;

	struct attropl *select_list = 0;

	static char destination[PBS_MAXQUEUENAME+1] = "";
	char server_out[MAXSERVERNAME] = "";

	char *queue_name_out;
	char *server_name_out;

	int connect;
	char **selectjob_list;
	char *res_pos;
	char *pc;
	time_t after;
	char a_value[80];
	char extendopts[4] = "";
	char *attr_time = NULL;
	struct ecl_attribute_errors *err_list;
	char *resc_time = NULL;

#define GETOPT_ARGS "a:A:c:h:HJl:N:p:q:r:s:t:Tu:xP:"

	/*test for real deal or just version and exit*/

	execution_mode(argc, argv);

#ifdef WIN32
	winsock_init();
#endif

	while ((c = getopt(argc, argv, GETOPT_ARGS)) != EOF)
		switch (c) {
			case 'a':
				check_op(optarg, pop, optargout);
				if ((after = cvtdate(optargout)) < 0) {
					fprintf(stderr, "qselect: illegal -a value\n");
					errflg++;
					break;
				}
				sprintf(a_value, "%ld", (long)after);
				set_attrop(&select_list, ATTR_a, NULL, a_value, op);
				break;
			case 'c':
				check_op(optarg, pop, optargout);
				pc = optargout;
				while (isspace((int)*pc)) pc++;
				if (strlen(pc) == 0) {
					fprintf(stderr, "qselect: illegal -c value\n");
					errflg++;
					break;
				}
				set_attrop(&select_list, ATTR_c, NULL, optargout, op);
				break;
			case 'h':
				check_op(optarg, pop, optargout);
				pc = optargout;
				while (isspace((int)*pc)) pc++;
				set_attrop(&select_list, ATTR_h, NULL, optargout, op);
				break;
			case 'J':
				op = EQ;
				set_attrop(&select_list, ATTR_array, NULL, "True", op);
				break;
			case 'l':
				res_pos = optarg;
				while (*res_pos != '\0') {
					if (check_res_op(res_pos, resource_name, pop, optargout, &res_pos) != 0) {
						errflg++;
						break;
					}
					set_attrop(&select_list, ATTR_l, resource_name, optargout, op);
				}
				break;
			case 'p':
				check_op(optarg, pop, optargout);
				set_attrop(&select_list, ATTR_p, NULL, optargout, op);
				break;
			case 'q':
				strcpy(destination, optarg);
				check_op(optarg, pop, optargout);
				set_attrop(&select_list, ATTR_q, NULL, optargout, op);
				break;
			case 'r':
				op = EQ;
				pc = optarg;
				while (isspace((int)(*pc))) pc++;
				if (*pc != 'y' && *pc != 'n') { /* qselect specific check - stays */
					fprintf(stderr, "qselect: illegal -r value\n");
					errflg++;
					break;
				}
				set_attrop(&select_list, ATTR_r, NULL, pc, op);
				break;
			case 's':
				check_op(optarg, pop, optargout);
				pc = optargout;
				while (isspace((int)(*pc))) pc++;
				set_attrop(&select_list, ATTR_state, NULL, optargout, op);
				break;
			case 't':
				if (get_tsubopt(*optarg, &attr_time, &resc_time)) {
					fprintf(stderr, "qselect: illegal -t value\n");
					errflg++;
					break;
				}
				/* 1st character possess the subopt, so send optarg++ */
				optarg ++;
				check_op(optarg, pop, optargout);
				if ((after = cvtdate(optargout)) < 0) {
					fprintf(stderr, "qselect: illegal -t value\n");
					errflg++;
					break;
				}
				sprintf(a_value, "%ld", (long)after);
				set_attrop(&select_list, attr_time, resc_time, a_value, op);
				break;
			case 'T':
				if (strchr(extendopts, (int)'T') == NULL)
					(void)strcat(extendopts, "T");
				break;
			case 'x':
				if (strchr(extendopts, (int)'x') == NULL)
					(void)strcat(extendopts, "x");
				break;
			case 'H':
				op = EQ;
				if (strchr(extendopts, (int)'x') == NULL)
					(void)strcat(extendopts, "x");
				set_attrop(&select_list, ATTR_state, NULL, "FM", op);
				break;
			case 'u':
				op = EQ;
				set_attrop(&select_list, ATTR_u, NULL, optarg, op);
				break;
			case 'A':
				op = EQ;
				set_attrop(&select_list, ATTR_A, NULL, optarg, op);
				break;
			case 'P':
				op = EQ;
				set_attrop(&select_list, ATTR_project, NULL, optarg, op);
				break;
			case 'N':
				op = EQ;
				set_attrop(&select_list, ATTR_N, NULL, optarg, op);
				break;
			default :
				errflg++;
		}

	if (errflg || (optind < argc)) {
		print_usage();
		exit(2);
	}

	if (notNULL(destination)) {
		if (parse_destination_id(destination, &queue_name_out, &server_name_out)) {
			fprintf(stderr, "qselect: illegally formed destination: %s\n", destination);
			exit(2);
		} else {
			if (notNULL(server_name_out)) {
				strcpy(server_out, server_name_out);
			}
		}
	}

	/*perform needed security library initializations (including none)*/

	if (CS_client_init() != CS_SUCCESS) {
		fprintf(stderr, "qselect: unable to initialize security library.\n");
		exit(2);
	}

	connect = cnt2server(server_out);
	if (connect <= 0) {
		fprintf(stderr, "qselect: cannot connect to server %s (errno=%d)\n",
			pbs_server, pbs_errno);

		/*cleanup security library initializations before exiting*/
		CS_close_app();

		exit(pbs_errno);
	}

	if (extendopts[0] == '\0')
		selectjob_list = pbs_selectjob(connect, select_list, NULL);
	else
		selectjob_list = pbs_selectjob(connect, select_list, extendopts);
	if (selectjob_list == NULL) {
		if ((err_list = pbs_get_attributes_in_error(connect)))
			handle_attribute_errors(err_list);

		if (pbs_errno != PBSE_NONE) {
			errmsg = pbs_geterrmsg(connect);
			if (errmsg != NULL) {
				fprintf(stderr, "qselect: %s\n", errmsg);
			} else {
				fprintf(stderr, "qselect: Error (%d) selecting jobs\n", pbs_errno);
			}

			/*
			 * If the server is not configured for history jobs i.e.
			 * job_history_enable svr attr is unset/set to FALSE, qselect
			 * command with -x/-H option is being used, then pbs_selectjob()
			 * will return PBSE_JOBHISTNOTSET error code. But command will
			 * exit with exit code '0'after printing the corresponding error
			 * message. i.e. "job_history_enable is set to false"
			 */
			if (pbs_errno == PBSE_JOBHISTNOTSET)
				pbs_errno = 0;

			/*cleanup security library initializations before exiting*/
			CS_close_app();
			exit(pbs_errno);
		}
	} else {   /* got some jobs ids */
		int i = 0;
		while (selectjob_list[i] != NULL) {
			printf("%s\n", selectjob_list[i++]);
		}
		free(selectjob_list);
	}
	pbs_disconnect(connect);

	/*cleanup security library initializations before exiting*/
	CS_close_app();

	exit(0);
}
