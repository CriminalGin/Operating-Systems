#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

int print_usage(char *argv[]){
	printf("Usage: %s -d [device filename] [other arguments]\n", argv[0]);
        printf("-i                   Print file system information\n");
        printf("-l                   List the root directory\n");
        printf("-r target -o dest    Recover the target deleted file\n");
        printf("-x target            Cleanse the target deleted file\n");
	return 0;
}

int main(int argc, char *argv[]){
	int ret = 0, d_num = 0, r_num = 0;
	while((ret = getopt(argc, argv, "d:ilr:o:x:")) != -1){
		printf("ret is %d\n", ret);
		if(d_num == 0){
			if(ret == 'd'){ puts(optarg); ++d_num;}
			else{ break; }
		}
		else if(d_num == 1){
			switch(ret){
				case 'd': puts(optarg); ++d_num; break;
				case 'i': break;
				case 'l': break;
				case 'r':
				case 'o':	
					break;
				case 'x': puts(optarg); break;
				default: print_usage(argv); break;
				}
		}
	}
	if(d_num == 0){ print_usage(argv); }
	return 0;
}
