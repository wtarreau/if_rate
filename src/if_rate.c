
/*
 version 2.0.1wt
 by mihvoi@rdsnet.ro
 first-level cleanups, timer fixes and few enhancements by willy tarreau.
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
    FILE *f;

    unsigned long int counteri[MAX_NR_INTERFETE][MAX_NR_COUNTERI];
    unsigned long int counteri_anterior[MAX_NR_INTERFETE][MAX_NR_COUNTERI];

    unsigned long int tmp_uint;
    char buff[MAX_LINE_FIS_PROC_SIZE + 1];	//  +1 pentru '\0'


    if (argv[1] == NULL) {
	interval_secs = 0;
    } else {
	interval_secs = atoi(argv[1]);
	if (interval_secs == 0) {
	    printf("Usage: if_rate [interval_in_seconds] (1..60, default 3)\n");
	    exit(1);
	}
    }

    if ((interval_secs <= 0) || (interval_secs > 60)) {
	interval_secs = DEFAULT_NR_SEC_MONITORIZARE;
	printf("Assuming intervar of %d seconds\n", interval_secs);
    }

    f = fopen(fis_dev, "r");
    if (f == NULL) {
	printf("Can not open file:\"%s\"", fis_dev);
	exit(1);
    }
    prima_oara = 1;

    time_start = 0;		//N-ar trebui sa conteze
    while (1) {
	time_stop = md_get_msec_time();
	printf("\e[H\e[J"); //system("clear");
	interval_msecs = time_stop - time_start;
	if (!prima_oara) {
	    printf("Averages for the last %d msec\n", interval_msecs);
	} else {
	    printf("Calculating rates for %d seconds...\n", interval_secs);
	}

	printf("+----------------------------------------------------------------------+\n");
	printf("| %-7s |%-29s |%-29s|\n", "IF", "Input", "Output");
	printf("+----------------------------------------------------------------------+\n");
	nr_crt = -1;

	while (nr_crt < MAX_NR_INTERFETE - 2) {
	    nr_crt++;
	    p = fgets(buff, MAX_LINE_FIS_PROC_SIZE, f);
	    if (p == NULL)
		break;

	    p = strchr(buff, ':');
	    if (p == NULL)
		continue;

	    *p = '\0';
	    p++;
	    i = -1;
	    printf("|%-8s ", buff);
	    p = strtok(p, SEPARATORI);

	    while (p != NULL) {
		i++;
		if (i > MAX_NR_COUNTERI) {
		    printf("Reached MAX_NR_COUNTERI=%d\n", MAX_NR_COUNTERI);
		    exit(0);
		}
		tmp_uint = strtoul(p, &p_supp, 10);
		if (p_supp == NULL) {
		    printf("Invalid format for number argument :\"%s\"\n", p);
		    exit(1);
		}
		counteri[nr_crt][i] = tmp_uint;

		if (!prima_oara) {
		    if (i == 0 || i == 8)	//bytes
			printf("|%7lu.%1d kbps",
				(counteri[nr_crt][i] - counteri_anterior[nr_crt][i]) * 8 / interval_msecs,	//mihvoi : de facut sa ia timpul in milisecunde
				(counteri[nr_crt][i] - counteri_anterior[nr_crt][i]) * 80 / interval_msecs % 10);
		    if (i == 1 || i == 9)	//pachet
			printf("|%7lu.%1d pk/s|",
				(counteri[nr_crt][i] - counteri_anterior[nr_crt][i]) * 1000 / interval_msecs,
				(counteri[nr_crt][i] - counteri_anterior[nr_crt][i]) * 1000 / interval_msecs % 10);
		}
		counteri_anterior[nr_crt][i] = counteri[nr_crt][i];
		p = strtok(NULL, SEPARATORI);
	    }
	    printf("\n");
	}
	if (fseek(f, 0, SEEK_SET) != 0) {
	    printf("Can not fseek to the start of the file %s\n", fis_dev);
	    exit(1);
	}

	if (prima_oara)
	    prima_oara = 0;

	printf("+----------------------------------------------------------------------+\n");
	
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
    struct timezone tz;
    tz.tz_dsttime = 0;
    tz.tz_minuteswest = 0;
    gettimeofday(&tv, &tz);
    //printf("Sec:%ld\n",tv.tv_sec);
    //printf("%lu\n",tv.tv_sec*1000000+tv.tv_usec);
    return (tv.tv_sec * 1000 + (tv.tv_usec) / 1000);
}

