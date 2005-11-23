
/*
 version 2.0.4wt
 by mihvoi@rdsnet.ro
 first-level cleanups, timer fixes and few enhancements by willy tarreau.
 2005/11/20: addition of interface selection and logging output by w.t.
*/


#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>


#define MAX_LINE_FIS_PROC_SIZE 1024
#define MAX_NR_INTERFETE 20
#define MAX_LEN_NUME_INTERFATA 20
#define MAX_NR_COUNTERI 20
#define SEPARATORI " \t\n\r"
#define DEFAULT_NR_SEC_MONITORIZARE 3

unsigned long long int md_get_msec_time(void);

/* string formats : [0]=!arg_log, [1]=arg_log */
const char *ifname_str[2]={"| %-8s", " "};
const char *kbps_str[2]={"|%7lu.%1d kbps", "%lu.%1d "};
const char *pkts_str[2]={"|%7lu.%1d pk/s|", "%lu.%1d "};

void usage()
{
    fprintf(stderr, "Usage: if_rate [-l] [ -i ifname ]* [interval_in_seconds] (1..60, default 3)\n");
    exit(1);
}

int main(int argc, char **argv)
{
    unsigned long long int time_start;
    unsigned long long int time_stop;
    int interval_msecs;
    int nr_crt, prima_oara, i;
    int interval_secs;
    int duration;
    char *fis_dev = "/proc/net/dev";
    char *p;
    char *p_supp;
    char *if_list = NULL;
    int arg_log = 0;
    FILE *f;

    unsigned long int counteri[MAX_NR_INTERFETE][MAX_NR_COUNTERI];
    unsigned long int counteri_anterior[MAX_NR_INTERFETE][MAX_NR_COUNTERI];

    unsigned long int tmp_uint;
    char buff[MAX_LINE_FIS_PROC_SIZE + 1];	//  +1 pentru '\0'

    interval_secs = 0;
    argv++; argc--;

    while (argc > 0) {
	if (argv[0][0] == '-' && argv[0][1] != '-') {
	    switch (argv[0][1]) {
	    case 'l':
		arg_log = 1;
		break;
	    case 'i': {
		int if_name_size;

		if (argc < 2)
		    usage();

		argv++; argc--;
		if_name_size = strlen(argv[0]);
		if (if_list) {
		    /* add 'ifname,' to the string */
		    if_list = realloc(if_list, strlen(if_list)+1+if_name_size+1);
		    sprintf(if_list + strlen(if_list), "%s,", argv[0]);
		}
		else {
		    /* start the string with ',ifname,' */
		    if_list = malloc(if_name_size + 3);
		    sprintf(if_list, ",%s,", argv[0]);
		}
		break;
	    } /* case 'i' */
	    default:
		usage();
	    } /* switch */
	} else if (argv[0][0] == '-') {
	    argc--;
	    argv++;
	    break;
	} else
	    break;
	argc--;
	argv++;
    }

    /* arg does not start with '-' */
    if (argc > 0) {
	interval_secs = atoi(argv[0]);
	if (interval_secs <= 0 || interval_secs > 60)
	    usage();
    } else {
	interval_secs = DEFAULT_NR_SEC_MONITORIZARE;
    }

    f = fopen(fis_dev, "r");
    if (f == NULL) {
	fprintf(stderr, "Can not open file:\"%s\"", fis_dev);
	exit(1);
    }
    prima_oara = 1;

    time_start = 0;		//N-ar trebui sa conteze
    while (1) {
	time_stop = md_get_msec_time();
	interval_msecs = time_stop - time_start;

	if (arg_log) {
	    if (prima_oara)
		printf("#   time   ");
	    else
		printf("%10lu", (unsigned long)(time_stop / 1000ULL));
	} else {
	    printf("\e[H\e[J"); //system("clear");
	    if (!prima_oara) {
		printf("Averages for the last %d msec\n", interval_msecs);
	    } else {
		printf("Calculating rates for %d seconds...\n", interval_secs);
	    }

	    printf("+----------------------------------------------------------------------+\n");
	    printf("| %-7s |%-29s |%-29s|\n", "IF", "Input", "Output");
	    printf("+----------------------------------------------------------------------+\n");
	}
	nr_crt = -1;

	while (nr_crt < MAX_NR_INTERFETE - 2) {
	    char search_name[32];
	    char *p1;

	    nr_crt++;
	    p1 = p = fgets(buff, MAX_LINE_FIS_PROC_SIZE, f);
	    if (p == NULL)
		break;

	    p = strchr(buff, ':');
	    if (p == NULL)
		continue;

	    *p++ = '\0';
	    while (p1 < p && (*p1 == ' ' || *p1 == '\t'))
		p1++;

	    /* the user has selected only some interfaces, let's check */
	    if (if_list && *if_list != 0) {
		snprintf(search_name, sizeof(search_name)-1, ",%s,", p1);
		search_name[sizeof(search_name)-1]=0;
		// printf("if_list=<%s>, search_name=<%s>\n",if_list, search_name);
		if (strstr(if_list, search_name) == NULL)
		    continue;
	    }

	    i = -1;

	    if (prima_oara && arg_log)
		printf("%s(ikb ipk okb opk) ", p1);
	    else
		printf(ifname_str[arg_log], p1);

	    p = strtok(p, SEPARATORI);

	    while (p != NULL) {
		i++;
		if (i > MAX_NR_COUNTERI)
		    break;

		tmp_uint = strtoul(p, &p_supp, 10);
		if (p_supp == NULL) {
		    /* fprintf(stderr, "Invalid format for number argument :\"%s\"\n", p); */
		    break;
		}

		counteri[nr_crt][i] = tmp_uint;

		if (!prima_oara) {
		    if (i == 0 || i == 8)	//bytes
			printf(kbps_str[arg_log],
				(counteri[nr_crt][i] - counteri_anterior[nr_crt][i]) * 8 / interval_msecs,	//mihvoi : de facut sa ia timpul in milisecunde
				(counteri[nr_crt][i] - counteri_anterior[nr_crt][i]) * 80 / interval_msecs % 10);
		    if (i == 1 || i == 9)	//pachet
			printf(pkts_str[arg_log],
				(counteri[nr_crt][i] - counteri_anterior[nr_crt][i]) * 1000 / interval_msecs,
				(counteri[nr_crt][i] - counteri_anterior[nr_crt][i]) * 1000 / interval_msecs % 10);
		}
		counteri_anterior[nr_crt][i] = counteri[nr_crt][i];
		p = strtok(NULL, SEPARATORI);
	    }
	    if (!arg_log)
		printf("\n");
	}
	if (fseek(f, 0, SEEK_SET) != 0) {
	    fprintf(stderr, "Can not fseek to the start of the file %s\n", fis_dev);
	    exit(1);
	}

	if (prima_oara)
	    prima_oara = 0;

	if (!arg_log)
	    printf("+----------------------------------------------------------------------+\n");
	else {
	    putchar('\n');
	    fflush(stdout);
	}
	
        time_start = md_get_msec_time();
	if (time_start >= time_stop + interval_secs * 1000)
	    duration = interval_secs * 1000 - (time_start - time_stop) % (interval_secs * 1000);
	else
	    duration = time_stop + interval_secs * 1000 - time_start;
	usleep(duration * 1000);
    }
}


unsigned long long int md_get_msec_time(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000ULL + (tv.tv_usec) / 1000ULL);
}

