#include "wingetopt.h"
#include <stdio.h>
#include <string.h>

#ifndef NULL
#define NULL	0
#endif
#define EOF	(-1)

int	opterr = 1;
int	optind = 1;
int	optopt;
char *optarg = NULL;

int getopt(int argc, char* argv[], char* opts)
{
    int sp = 1;
    int c;
    char* cp = NULL;

    if(sp == 1) {
        if (optind >= argc || argv[optind][0] != '-' || argv[optind][1] == '\0') {
            return(EOF);
        } else if(strcmp(argv[optind], "--") == 0) {
            optind++;
            return(EOF);
        }
        optopt = c = argv[optind][sp];
        if ((c == ':') || ((cp = strchr(opts, c)) == NULL)) {
            if(argv[optind][++sp] == '\0') {
                optind++;
                sp = 1;
            }
            return(':');
        }
        if(*++cp == ':') {
            if(argv[optind][sp+1] != '\0') {
                optarg = &argv[optind++][sp+1];
            } else if(++optind >= argc) {
                sp = 1;
                return(':');
            } else {
                optarg = argv[optind++];
            }
            sp = 1;
        } else {
            if(argv[optind][++sp] == '\0') {
                sp = 1;
                optind++;
            }
            optarg = NULL;
        }
    }
    return(c);
}