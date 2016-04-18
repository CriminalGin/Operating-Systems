#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]){
	int flags, opt;
	int nsecs, tfnd;
	nsecs = 0, tfnd = 0, flags = 0;
	while((opt = getopt(argc, argv, "nt:")) != -1){
		switch(opt){
			case 'n':
				flags = 1;
				break;
			case 't':
				nsecs = atoi(optarg);
				tfnd = 1;
				break;
			default:
				printf("Failure!\n");
				exit(EXIT_FAILURE);
		}
	}

	printf("flags=%d; tfnd=%d; optind=%d\n", flags, tfnd, optind);
	if(optind >= argc){
		fprintf(stderr, "Expected argument after opions\n");
		exit(EXIT_FAILURE);
	}

	printf("name argument = %s\n", argv[optind]);
	exit(EXIT_SUCCESS);
}
