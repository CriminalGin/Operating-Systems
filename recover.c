#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <stdint.h>
#include <string.h>
#include <endian.h>

#define ARGMAXNUM 6
#define CLUSTERSIZE 512 // should be replaced by data from fread

int print_usage(char *argv[]){
	printf("Usage: %s -d [device filename] [other arguments]\n", argv[0]);
        printf("-i                   Print file system information\n");
        printf("-l                   List the root directory\n");
        printf("-r target -o dest    Recover the target deleted file\n");
        printf("-x target            Cleanse the target deleted file\n");
	return 0;
}

struct global_args_t{
	char *d_options;
	int d_num;
	int i_num;
	int l_num;
	char *r_options;
	int r_num;
	char *o_options;
	int o_num;
	char *x_options;
	int x_num;
	int last_index;
	char *args;
	int args_num;
}global_args;

static const char *opt_strings = "d:ilr:o:x:";

int global_args_t_init(struct global_args_t global_args){
	global_args.d_options = NULL;
	global_args.d_num = 0;
	global_args.i_num = 0;
	global_args.l_num = 0;
	global_args.r_options = NULL;
	global_args.r_num = 0;
	global_args.o_options = NULL;
	global_args.o_num = 0;
	global_args.x_options = NULL;
	global_args.x_num = 0;
	global_args.last_index = 0;
	global_args.args = NULL;
	global_args.args_num = 0;
	return 0;
}

#pragma pack(push, 1)
struct BootEntry{
	uint8_t BS_jmpBoot[3]; /* Assembly instruction to jump to boot code */
	uint8_t BS_OEMName[8]; /* OEM Name in ASCII */
	uint16_t BPB_BytsPerSec; /* Bytes per sector. Allowed values include 512, 1024, 2048, and 4096 */
	uint8_t BPB_SecPerClus; /* Sectors per cluster (data unit). Allowed values are powers of 2, but the cluster size must be 32KB or smaller */
	uint16_t BPB_RsvdSecCnt; /* Size in sectors of the reserved area */
	uint8_t BPB_NumFATs; /* Number of FATs */
	uint16_t BPB_RootEntCnt; /* Maximum number of files in the root directory for FAT12 and FAT16. This is 0 for FAT32 */
	uint16_t BPB_TotSec16; /* 16-bit value of number of sectors in file system */
 	uint8_t BPB_Media; /* Media type */
	uint16_t BPB_FATSz16; /* 16-bit size in sectors of each FAT for FAT12 and FAT16. For FAT32, this field is 0 */
	uint16_t BPB_SecPerTrk; /* Sectors per track of storage device */
	uint16_t BPB_NumHeads; /* Number of heads in storage device */
	uint32_t BPB_HiddSec; /* Number of sectors before the start of partition */
	uint32_t BPB_TotSec32; /* 32-bit value of number of sectors in file system. Either this value or the 16-bit value above must be 0 */
	uint32_t BPB_FATSz32; /* 32-bit size in sectors of one FAT */
	uint16_t BPB_ExtFlags; /* A flag for FAT */
	uint16_t BPB_FSVer; /* The major and minor version number */
	uint32_t BPB_RootClus; /* Cluster where the root directory can be found */
	uint16_t BPB_FSInfo; /* Sector where FSINFO structure can be
found */
	uint16_t BPB_BkBootSec; /* Sector where backup copy of boot sector is located */
	uint8_t BPB_Reserved[12]; /* Reserved */
	uint8_t BS_DrvNum; /* BIOS INT13h drive number */
	uint8_t BS_Reserved1; /* Not used */
	uint8_t BS_BootSig; /* Extended boot signature to identify if the next three values are valid */
	uint32_t BS_VolID; /* Volume serial number */
	uint8_t BS_VolLab[11]; /* Volume label in ASCII. User defines when creating the file system */
	uint8_t BS_FilSysType[8]; /* File system type label in ASCII */
};
#pragma pack(pop)

#pragma pack(push, 1)
struct DirEntry{
	uint8_t DIR_Name[11]; /* File name */
	uint8_t DIR_Attr; /* File attributes */
	uint8_t DIR_NTRes; /* Reserved */
	uint8_t DIR_CrtTimeTenth; /* Created time (tenths of second) */
	uint16_t DIR_CrtTime; /* Created time (hours, minutes, seconds) */
	uint16_t DIR_CrtDate; /* Created day */
	uint16_t DIR_LstAccDate; /* Accessed day */
	uint16_t DIR_FstClusHI; /* High 2 bytes of the first cluster address */
	uint16_t DIR_WrtTime; /* Written time (hours, minutes, seconds */
	uint16_t DIR_WrtDate; /* Written day */
	uint16_t DIR_FstClusLO; /* Low 2 bytes of the first cluster address */
	uint32_t DIR_FileSize; /* File size in bytes. (0 for directories) */
};
#pragma pack(pop)

int read_options(int *num, char *options, struct global_args_t *global_args, char *optarg, int ret){
	++*num; 
	if(*num > 1){free(global_args->args); return -1;}
	options = (char *)malloc(strlen(optarg) * sizeof(char));
	strcpy(options, optarg);
	global_args->args[global_args->args_num++] = ret; 
	return 0;
}

int read_args(int argc, char *argv[]){
	int ret = 0;
	global_args.args = (char *)malloc(ARGMAXNUM * sizeof(char));
	while((ret = getopt(argc, argv, opt_strings)) != -1){
		if(global_args.args_num >= ARGMAXNUM){ free(global_args.args); return -1; }
		if(global_args.last_index == optind) {free(global_args.args); return -1;}
		switch(ret){
			case 'd': if(read_options(&global_args.d_num, global_args.d_options, &global_args, optarg, ret) == -1) { return -1;} break;
			case 'i': ++global_args.i_num; if(global_args.i_num > 1){free(global_args.args); return -1;} global_args.args[global_args.args_num++] = ret; break;
			case 'l': ++global_args.l_num;  if(global_args.l_num > 1){free(global_args.args); return -1;}global_args.args[global_args.args_num++] = ret; break;
			case 'r': if(read_options(&global_args.r_num, global_args.r_options, &global_args, optarg, ret) == -1) {return -1;} break;
			case 'o': if(read_options(&global_args.o_num, global_args.o_options, &global_args, optarg, ret) == -1) {return -1;} break;
			case 'x': if(read_options(&global_args.x_num, global_args.x_options, &global_args, optarg, ret) == -1) {return -1;} break;
			default: free(global_args.args); return -1;
		}
		global_args.last_index = optind;
	}
	if(global_args.args_num <= 1 || global_args.args_num > 3){ free(global_args.args); return -1; }
	if(global_args.d_num == 0){free(global_args.args); return -1;}
	if((global_args.args_num == 3)){
		if(!(global_args.r_num & global_args.o_num)){free(global_args.args); printf("before return -1\n"); return -1;}
	}
	if(global_args.d_num > 1 || global_args.i_num > 1 || global_args.l_num > 1 || global_args.r_num > 1 || global_args.o_num > 1 || global_args.x_num > 1) {free(global_args.args); return -1;}
	if(global_args.r_num ^ global_args.o_num){free(global_args.args); return -1;}
	return 0;
}

int print_info(int argc, char *argv[]){
	FILE *fp;
	struct BootEntry boot_entry;
	if((fp = fopen(argv[2], "rb")) == NULL){ perror(argv[2]); exit(1); }
	fread(&boot_entry, sizeof(struct BootEntry), 1, fp);
	printf("Number of FATs = %d\n", (boot_entry).BPB_NumFATs);		
	printf("Number of bytes per sector = %d\n", ((boot_entry).BPB_BytsPerSec));
	printf("Number of sectors per cluster = %d\n", (boot_entry).BPB_SecPerClus);
	printf("Number of reserved sectors = %d\n", ((boot_entry).BPB_RsvdSecCnt));
	int fat1_start = boot_entry.BPB_BytsPerSec * boot_entry.BPB_RsvdSecCnt;
	printf("First FAT starts at byte = %d\n", fat1_start);
	long unsigned int data_start = (boot_entry.BPB_RsvdSecCnt + boot_entry.BPB_NumFATs * boot_entry.BPB_FATSz32) * boot_entry.BPB_BytsPerSec;
	printf("Data area starts at byte = %lu\n", data_start);
	fclose(fp);
	return 0;
}

int list_directory(int argc, char *argv[]){
	FILE *fp;
	struct DirEntry dir_entry; struct BootEntry boot_entry;
	if((fp = fopen(argv[2], "rb")) == NULL){ perror(argv[2]); exit(1); }
	fread(&boot_entry, sizeof(struct BootEntry), 1, fp);
	long unsigned int data_start = (boot_entry.BPB_RsvdSecCnt + boot_entry.BPB_NumFATs * boot_entry.BPB_FATSz32) * boot_entry.BPB_BytsPerSec;
	int order = 1; char name[12]; long unsigned int cluster = 0;
	while(1){
		int dir_flag = 0;	
		fseek(fp, data_start + sizeof(struct DirEntry) * (order - 1), SEEK_SET);
		fread(&dir_entry, sizeof(struct DirEntry), 1, fp);
		if(dir_entry.DIR_Name[0] == 0 && dir_entry.DIR_Name[1] == 0 && dir_entry.DIR_Name[2] == 0 && dir_entry.DIR_Name[3] == 0 && dir_entry.DIR_Name[4] == 0 && dir_entry.DIR_Name[5] == 0 && dir_entry.DIR_Name[6] == 0 && dir_entry.DIR_Name[7] == 0 && dir_entry.DIR_Name[8] == 0 && dir_entry.DIR_Name[9] == 0 && dir_entry.DIR_Name[10] == 0){ break; }
		if((dir_entry.DIR_Attr & 0x00F) == 0x00F){printf("%d, LFN entry\n", order);}
		else{
			
			if((dir_entry.DIR_Attr & 0x010) == 0x010){dir_flag = 1;}
			NAME_to_name(dir_entry.DIR_Name, name, dir_flag);
			cluster = dir_entry.DIR_FstClusLO + dir_entry.DIR_FstClusHI * 0x10000;
			printf("%d, %s, %lu, %ld\n", order, name, dir_entry.DIR_FileSize, cluster);
		}
		++order;	
	}
	fclose(fp);
	return 0;
}

int NAME_to_name(uint8_t *DIR_Name, char *name, int dir_flag){
	int i = 0, j = 8;
	while((char)DIR_Name[i] != ' ' && i < 8){
		if(i == 0 && DIR_Name[i] == 0xE5){name[i] = '?';}
		else{name[i] = (char)DIR_Name[i];}
		 ++i;
	}
	if((char)DIR_Name[j] == ' '){ }
	else{
		name[i++] = '.';
		while((char)DIR_Name[j] != ' '){name[i] = (char)DIR_Name[j]; ++i; ++j; } 
	}
	if(dir_flag == 1){name[i++] = '/';}
	name[i++] = '\0';
	return 0;
}

/* no directory*/
int name_to_NAME(int *NAME, char *name){
	int name_idx = 0, NAME_IDX = 0;
	int i = 0;	
	for(i = 0; i < 11; ++i){NAME[i] = 0x20;}
	for(name_idx = 0; name_idx < strlen(name); ++name_idx){
		if(name[name_idx] == '.'){NAME_IDX = 8; ++name_idx;}
		NAME[NAME_IDX] = (int)name[name_idx];
		++NAME_IDX; 	
	}
	return 0;
}

int check_occupy(int argc, char *argv[], long unsigned int cluster){
	FILE *input;
	if((input = fopen(argv[2], "rb")) == NULL){ perror(argv[2]); exit(1); }
	struct DirEntry dir_entry; struct BootEntry boot_entry;
	fread(&boot_entry, sizeof(struct BootEntry), 1, input);
	int **FAT_entry = (int**)malloc(boot_entry.BPB_FATSz32 * sizeof(int *));
	int i = 0;
	for(i = 0; i < boot_entry.BPB_FATSz32; ++i){FAT_entry[i] = (int *)malloc(boot_entry.BPB_BytsPerSec);}
	fseek(input, boot_entry.BPB_RsvdSecCnt * boot_entry.BPB_BytsPerSec, SEEK_SET);
#if 1
	for(i = 0; i < boot_entry.BPB_FATSz32; ++i){
		fseek(input, (boot_entry.BPB_RsvdSecCnt + i) * boot_entry.BPB_BytsPerSec, SEEK_SET);	
		fread(FAT_entry[i], boot_entry.BPB_BytsPerSec, 1, input);
	}
#endif
	fclose(input);
	if(FAT_entry[0][cluster] != 0){return 1;}
	else{return 0;}
}

int recover_file(int argc, char *argv[]){
	FILE *input;
	if((input = fopen(argv[2], "rb")) == NULL){ perror(argv[2]); exit(1); }	
	long unsigned int cluster = 0; int order = 1; int NAME[11]; int i = 0;
	struct DirEntry dir_entry; struct BootEntry boot_entry;
	fread(&boot_entry, sizeof(struct BootEntry), 1, input);
	long unsigned int data_start = (boot_entry.BPB_RsvdSecCnt + boot_entry.BPB_NumFATs * boot_entry.BPB_FATSz32) * boot_entry.BPB_BytsPerSec;
	while(1){	
		fseek(input, data_start + sizeof(struct DirEntry) * (order - 1), SEEK_SET);
		fread(&dir_entry, sizeof(struct DirEntry), 1, input);
		if(dir_entry.DIR_Name[0] == 0 && dir_entry.DIR_Name[1] == 0 && dir_entry.DIR_Name[2] == 0 && dir_entry.DIR_Name[3] == 0 && dir_entry.DIR_Name[4] == 0 && dir_entry.DIR_Name[5] == 0 && dir_entry.DIR_Name[6] == 0 && dir_entry.DIR_Name[7] == 0 && dir_entry.DIR_Name[8] == 0 && dir_entry.DIR_Name[9] == 0 && dir_entry.DIR_Name[10] == 0){printf("%s: error - file not found\n", argv[4]); fclose(input); return -1;}
		name_to_NAME(NAME, argv[4]);
		if(dir_entry.DIR_Name[0] == 0xE5){
			for(i = 1; i < 11; ++i){
				if(NAME[i] != dir_entry.DIR_Name[i]){break;}			
			}
		}
		if(i == 11){ break; }	
		++order;	 	
	}
	cluster = dir_entry.DIR_FstClusLO + dir_entry.DIR_FstClusHI * 0x10000;
	if(check_occupy(argc, argv, cluster)){printf("%s: error - fail to recover\n", argv[4]); return -1;};
	fseek(input, data_start + boot_entry.BPB_BytsPerSec * boot_entry.BPB_SecPerClus * (cluster - 2), SEEK_SET);
	char *content = (char *)malloc(sizeof(char) * dir_entry.DIR_FileSize); 
	int position = 0; FILE *output; 
	if( (output = fopen(argv[6], "w+")) == NULL){printf("%s: failed to open\n", argv[4]); free(content); fclose(output); fclose(input);return -1;}
	fseek(input, data_start + boot_entry.BPB_BytsPerSec * boot_entry.BPB_SecPerClus * (cluster - 2), SEEK_SET);
	fread(content, sizeof(char), dir_entry.DIR_FileSize, input);
	fwrite(content, sizeof(char), dir_entry.DIR_FileSize, output);
	printf("%s: recovered\n", argv[4]);
	free(content); fclose(output); fclose(input);	
	return 0;
}

int cleanse_file(int argc, char *argv[]){
	FILE *input;
	if((input = fopen(argv[2], "r+")) == NULL){ perror(argv[2]); exit(1); }	
	long unsigned int cluster = 0; int order = 1; int NAME[11]; int i = 0;	
	struct DirEntry dir_entry; struct BootEntry boot_entry;
	fread(&boot_entry, sizeof(struct BootEntry), 1, input);
	long unsigned int data_start = (boot_entry.BPB_RsvdSecCnt + boot_entry.BPB_NumFATs * boot_entry.BPB_FATSz32) * boot_entry.BPB_BytsPerSec;
	while(1){	
		fseek(input, data_start + sizeof(struct DirEntry) * (order - 1), SEEK_SET);
		fread(&dir_entry, sizeof(struct DirEntry), 1, input);
		if(dir_entry.DIR_Name[0] == 0 && dir_entry.DIR_Name[1] == 0 && dir_entry.DIR_Name[2] == 0 && dir_entry.DIR_Name[3] == 0 && dir_entry.DIR_Name[4] == 0 && dir_entry.DIR_Name[5] == 0 && dir_entry.DIR_Name[6] == 0 && dir_entry.DIR_Name[7] == 0 && dir_entry.DIR_Name[8] == 0 && dir_entry.DIR_Name[9] == 0 && dir_entry.DIR_Name[10] == 0){printf("%s: error - file not found\n", argv[4]); fclose(input); return -1;}
		name_to_NAME(NAME, argv[4]);
		if(dir_entry.DIR_Name[0] == 0xE5){
			for(i = 1; i < 11; ++i){
				if(NAME[i] != dir_entry.DIR_Name[i]){break;}			
			}
		}
		if(i == 11){ break; }	
		++order;	 	
	}
	if(dir_entry.DIR_FileSize == 0){printf("%s: error - fail to cleanse\n", argv[4]); fclose(input); return -1;}	
	cluster = dir_entry.DIR_FstClusLO + dir_entry.DIR_FstClusHI * 0x10000;
	if(check_occupy(argc, argv, cluster)){printf("%s: error - fail to cleanse\n", argv[4]); fclose(input); return -1;};
	char *zeros = (char *)malloc(sizeof(char) * dir_entry.DIR_FileSize);
	memset(zeros, 0, dir_entry.DIR_FileSize);
	fseek(input, data_start + boot_entry.BPB_BytsPerSec * boot_entry.BPB_SecPerClus * (cluster - 2), SEEK_SET);
	fwrite(zeros, dir_entry.DIR_FileSize, 1, input);
	printf("%s: cleansed\n", argv[4]);	
	return 0;
}

int main(int argc, char *argv[]){
	while(global_args_t_init(global_args) != 0);
	if(read_args(argc, argv) == -1){print_usage(argv); return -1; }
	if(global_args.i_num == 1){print_info(argc, argv);}
	else if(global_args.l_num == 1){list_directory(argc, argv);}
	else if(global_args.r_num == 1 && global_args.o_num == 1){recover_file(argc, argv);}
	else if(global_args.x_num == 1){cleanse_file(argc, argv);}
	return 0;
}
