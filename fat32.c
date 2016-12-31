#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <linux/msdos_fs.h>
#include <ctype.h>

#define DIRECTORYENTITYSIZE 	32	//bytes
#define SECTORSPERCLUSTER 	8
#define SECTORSIZE 		512   //bytes
#define BLOCKSIZE  		4096  // bytes - do not change this value
#define TO_HEX(i) (i <= 9 ? '0' + i : 'A' - 10 + i)
#define MAXFILENAMELENGTH	20
char diskname[48]; 
int  disk_fd; 

unsigned char volumesector[SECTORSIZE]; 

int get_sector (unsigned char *buf, int snum)
{
	off_t offset; 
	int n; 
	offset = snum * SECTORSIZE; 
	lseek (disk_fd, offset, SEEK_SET); 
	n  = read (disk_fd, buf, SECTORSIZE); 
	if (n == SECTORSIZE) 
		return (0); 
	else {
		printf ("sector number %d invalid or read error.\n", snum); 
		exit (1); 
	}
}

int set_sector (unsigned char *buf, int snum)
{
	off_t offset; 
	int n; 
	offset = snum * SECTORSIZE; 
	lseek (disk_fd, offset, SEEK_SET); 
	n  = write (disk_fd, buf, SECTORSIZE); 
	if (n == SECTORSIZE) 
		return (0); 
	else {
		printf ("sector number %d invalid or read error.\n", snum); 
		exit (1); 
	}
}

/*print sector info*/
void print_sector (unsigned char *s)
{
	int i;

	for (i = 0; i < SECTORSIZE; ++i) {
		printf ("%02x ", (unsigned char) s[i]); 
		if ((i+1) % 16 == 0)
			printf ("\n"); 
	}
	printf ("\n");
}

/* print the content of the root directory */
void print_rootdir() {

	get_sector (volumesector, 0); 

	struct fat_boot_sector *bsp; 
	bsp = (struct fat_boot_sector *) volumesector;
	
	/* read the number of sectors per fat at byte 36*/
	unsigned int sectors_per_fat_length = bsp->fat32.length;

	/* calculate beginning of the root folder boot sec +reserved secs + 2*fat secs*/
	unsigned int root_folder_begin_sec = 32 + 2*sectors_per_fat_length;
	
	printf("Content of the root directory:\n");
	int i;
	/* get root sectors and read each root directory entity */
	for(i = root_folder_begin_sec; i < root_folder_begin_sec + SECTORSPERCLUSTER; i++) {
		unsigned char sector[SECTORSIZE]; 
		get_sector(sector, i);

		int entity_per_sector_length = SECTORSIZE / DIRECTORYENTITYSIZE;
		int j;
		
		/* read each directory entity */
		for(j = 0; j < entity_per_sector_length; j++) {
			struct msdos_dir_entry *mde;
			unsigned char entity[DIRECTORYENTITYSIZE]; 
			int k;
			/* copy 32 byte entity from root sector*/
			for(k = 0; k < 32; k++) {
				entity[k] = sector[j*DIRECTORYENTITYSIZE+k];
			}
			mde = (struct msdos_dir_entry *) entity;
			if(entity[0] == 0)
				continue;
			/* name of the entity*/ 
			unsigned char* name = mde->name;
			/* attribute */
			unsigned int attribute = mde->attr;
			/* Creation date */
			unsigned int creation_date = mde->date;
			/* Creation time */
			unsigned int creation_time = mde->time;
			/* Last access date */
			unsigned int adate = mde->adate;
			/* First cluster low bytes */
			unsigned int cluster_low = mde->start;
			/* First cluster high bytes*/
			unsigned int cluster_high = mde->starthi;
			/* File size  */
			unsigned int file_size = mde->size;
			
			printf("File name: %s, Attribute: %d, Creation date: %d, Creation time: %d, Last access date: %d, File size: %d\n", name, attribute, creation_date, creation_time, adate, 				file_size);
			
		}
	}
}

/*print the numbers of the clusters allocated to a file*/
void print_blocks_allocated(char* filename) {
	get_sector (volumesector, 0); 

	struct fat_boot_sector *bsp; 
	bsp = (struct fat_boot_sector *) volumesector;
	
	/* read the number of sectors per fat at byte 36*/
	unsigned int sectors_per_fat_length = bsp->fat32.length;
	
	/* calculate beginning of the root folder boot sec +reserved secs + 2*fat secs*/
	unsigned int root_folder_begin_sec = 32 + 2*sectors_per_fat_length;
	
	printf("\nBlocks allocated:\n");
	int count = 0;
	int i;
	/* get root sectors and read each root directory entity */
	for(i = root_folder_begin_sec; i < root_folder_begin_sec + SECTORSPERCLUSTER; i++) {
		unsigned char sector[SECTORSIZE]; 
		get_sector(sector, i);

		int entity_per_sector_length = SECTORSIZE / DIRECTORYENTITYSIZE;
		int j;
		
		/* read each directory entity */
		for(j = 0; j < entity_per_sector_length; j++) {
			struct msdos_dir_entry *mde;
			unsigned char entity[DIRECTORYENTITYSIZE]; 
			int k;
			/* copy 32 byte entity from root sector*/
			for(k = 0; k < DIRECTORYENTITYSIZE; k++) {
				entity[k] = sector[j*DIRECTORYENTITYSIZE+k];
			}
			mde = (struct msdos_dir_entry *) entity;
			if(entity[0] == 0)
				continue;
			/* name of the entity*/ 
			unsigned char* name = mde->name;
			char *namec = (char*)name;
			char *token = strtok(namec," ");
			char* cfilename = malloc(sizeof(token));
			char* extension;
			strcpy(cfilename,token);
			if(token!=NULL) {
				token = strtok(NULL," ");
				extension = malloc(sizeof(token));
				strcpy(extension,token);
			}

			char* name_with_extension;
			char dot[] =".";
			name_with_extension = malloc(strlen(cfilename)+2+strlen(extension)); 
			strcpy(name_with_extension, cfilename); 
			strcat(name_with_extension, dot); 
			strcat(name_with_extension, extension);
			if(!strcasecmp(name_with_extension, filename)) {
				/* First cluster low bytes */
				unsigned int cluster_low = mde->start;
				/* First cluster high bytes*/
				unsigned int cluster_high = mde->starthi;
				
				int x = cluster_low;
				int y = cluster_high;
				/* 32 bit cluster number */
				char res[9];
				res[0] = TO_HEX(((y & 0xF000) >> 12));   
				res[1] = TO_HEX(((y & 0x0F00) >> 8));
				res[2] = TO_HEX(((y & 0x00F0) >> 4));
				res[3] = TO_HEX((y & 0x000F));
				sprintf(&res[4],"%04x",x);
				int cluster_no;
				sscanf(res,"%x",&cluster_no);
				if(cluster_no != 0 && cluster_no!=1) {
					count++;
					/* lookup FAT by comparing both for inconsistency*/
					do {

						printf("%d)Cluster no:%d\n",count,cluster_no);
						/* get the starting file sector*/
						int start_sect_no1 = 32 + (cluster_no*4) / SECTORSIZE;
						int start_sect_no2 = 32 + sectors_per_fat_length + (cluster_no*4) / SECTORSIZE;

						unsigned char fat1Sector[SECTORSIZE]; 
						unsigned char fat2Sector[SECTORSIZE]; 
						get_sector(fat1Sector, start_sect_no1);
						get_sector(fat2Sector, start_sect_no2);
						int start_offset = (cluster_no*4) % SECTORSIZE;
						int firstlowbyte = fat1Sector[start_offset];
						int secondlowbyte =fat1Sector[start_offset+1];
						int thirdlowbyte =fat1Sector[start_offset+2];
						int highestbyte =fat1Sector[start_offset+3];
						x = firstlowbyte;
						y = secondlowbyte;
						int z = thirdlowbyte;
						int t = highestbyte;
						char res[9];
						res[0] = TO_HEX(((t & 0xF0) >> 4));   
						res[1] = TO_HEX(((t & 0x0F)));
						res[2] = TO_HEX(((z & 0xF0) >> 4));   
						res[3] = TO_HEX(((z & 0x0F)));
						res[4] = TO_HEX(((y & 0xF0) >> 4));   
						res[5] = TO_HEX(((y & 0x0F)));
						sprintf(&res[6],"%02x",x);
						int merge;
						sscanf(res,"%x",&merge);
						cluster_no = merge;
						count++;
					}
					while(cluster_no != 268435455) ;
					
					printf("%d)Cluster no:%d\n",count,cluster_no);
					printf("The numbers of the clusters allocated to file %s is %d\n", name_with_extension,count);
					
				}
				break;
				
			}
			
		}
	}
}

void delete_file(char* filename) {
	/*delete for both FAT*/
	get_sector (volumesector, 0); 

	struct fat_boot_sector *bsp; 
	bsp = (struct fat_boot_sector *) volumesector;
	
	/* read the number of sectors per fat at byte 36*/
	unsigned int sectors_per_fat_length = bsp->fat32.length;
	
	/* calculate beginning of the root folder boot sec +reserved secs + 2*fat secs*/
	unsigned int root_folder_begin_sec = 32 + 2*sectors_per_fat_length;
	
	printf("Blocks allocated:\n");
	int count = 0;
	int i;
	/* get root sectors and read each root directory entity */
	for(i = root_folder_begin_sec; i < root_folder_begin_sec + SECTORSPERCLUSTER; i++) {
		unsigned char sector[SECTORSIZE]; 
		get_sector(sector, i);

		int entity_per_sector_length = SECTORSIZE / DIRECTORYENTITYSIZE;
		int j;
		
		/* read each directory entity */
		for(j = 0; j < entity_per_sector_length; j++) {
			struct msdos_dir_entry *mde;
			unsigned char entity[DIRECTORYENTITYSIZE]; 
			int k;
			/* copy 32 byte entity from root sector*/
			for(k = 0; k < DIRECTORYENTITYSIZE; k++) {
				entity[k] = sector[j*DIRECTORYENTITYSIZE+k];
			}
			mde = (struct msdos_dir_entry *) entity;
			if(entity[0] == 0)
				continue;
			/* name of the entity*/ 
			unsigned char* name = mde->name;
			char *namec = (char*)name;
			char *token = strtok(namec," ");
			char* cfilename = malloc(sizeof(token));
			char* extension;
			strcpy(cfilename,token);
			if(token!=NULL) {
				token = strtok(NULL," ");
				extension = malloc(sizeof(token));
				strcpy(extension,token);
			}

			char* name_with_extension;
			char dot[] =".";
			name_with_extension = malloc(strlen(cfilename)+2+strlen(extension)); 
			strcpy(name_with_extension, cfilename); 
			strcat(name_with_extension, dot); 
			strcat(name_with_extension, extension);
			
			if(!strcasecmp(name_with_extension, filename)) {
				/*remove the file from the directory*/
				for(k = 0; k < DIRECTORYENTITYSIZE; k++) {
					sector[j*DIRECTORYENTITYSIZE+k] = 0;
				}
				set_sector(sector,i);
				/* First cluster low bytes */
				unsigned int cluster_low = mde->start;
				/* First cluster high bytes*/
				unsigned int cluster_high = mde->starthi;
				
				int x = cluster_low;
				int y = cluster_high;
				/* 32 bit cluster number */
				char res[9];
				res[0] = TO_HEX(((y & 0xF000) >> 12));   
				res[1] = TO_HEX(((y & 0x0F00) >> 8));
				res[2] = TO_HEX(((y & 0x00F0) >> 4));
				res[3] = TO_HEX((y & 0x000F));
				sprintf(&res[4],"%04x",x);
				int cluster_no;
				sscanf(res,"%x",&cluster_no);
				if(cluster_no != 0 && cluster_no!=1) {
					count++;
					/* lookup FAT by comparing both for inconsistency*/
					do {

						/* get the starting file sector*/
						int start_sect_no1 = 32 + (cluster_no*4) / SECTORSIZE;
						int start_sect_no2 = 32 + sectors_per_fat_length + (cluster_no*4) / SECTORSIZE;

						unsigned char fat1Sector[SECTORSIZE]; 
						unsigned char fat2Sector[SECTORSIZE]; 
						get_sector(fat1Sector, start_sect_no1);
						get_sector(fat2Sector, start_sect_no2);
						int start_offset = (cluster_no*4) % SECTORSIZE;
						
						int firstlowbyte = fat1Sector[start_offset];
						int secondlowbyte = fat1Sector[start_offset+1];
						int thirdlowbyte = fat1Sector[start_offset+2];
						int highestbyte = fat1Sector[start_offset+3];
						fat1Sector[start_offset] = 0;
						fat1Sector[start_offset+1] = 0;
						fat1Sector[start_offset+2] = 0;
						fat1Sector[start_offset+3] = 0;
						fat2Sector[start_offset] = 0;
						fat2Sector[start_offset+1] = 0;
						fat2Sector[start_offset+2] = 0;
						fat2Sector[start_offset+3] = 0;
						set_sector(fat1Sector,start_sect_no1);
						set_sector(fat2Sector,start_sect_no2);
						x = firstlowbyte;
						y = secondlowbyte;
						int z = thirdlowbyte;
						int t = highestbyte;
						char res[9];
						res[0] = TO_HEX(((t & 0xF0) >> 4));   
						res[1] = TO_HEX(((t & 0x0F)));
						res[2] = TO_HEX(((z & 0xF0) >> 4));   
						res[3] = TO_HEX(((z & 0x0F)));
						res[4] = TO_HEX(((y & 0xF0) >> 4));   
						res[5] = TO_HEX(((y & 0x0F)));
						sprintf(&res[6],"%02x",x);
						int merge;
						sscanf(res,"%x",&merge);
						cluster_no = merge;
					}
					while(cluster_no != 268435455) ;
					
				}
				printf("File %s successfully deleted\n", name_with_extension);

				break;
				
			}
			
			
		}
	}
}

void print_volume_info() {
	get_sector (volumesector, 0); 
	struct fat_boot_sector *bsp; 
	bsp = (struct fat_boot_sector *) volumesector;
	
	/* read the number of sectors per fat at byte 36*/
	unsigned int sectors_per_fat_length = bsp->fat32.length;
	
}



int main(int argc, char *argv[])
{

	if (argc < 2) {
		printf ("wrong usage\n"); 
		exit (1); 
	}

	strcpy (diskname, argv[1]); 
	
        disk_fd = open (diskname, O_RDWR); 
	if (disk_fd < 0) {
		printf ("could not open the disk image\n"); 
		exit (1); 
	}
	char choice[40];
	strcpy (choice, argv[2]);
	char choice2[40];
	strcpy (choice2, argv[3]);

	if(strcmp(choice,"-p")) {
		if(strcmp(choice2,"volumeinfo")) {
			print_volume_info();
		}
		else if(strcmp(choice2,"rootdir")) {
			print_rootdir();
		}
		else if(strcmp(choice2,"blocks")) {
			char choice3[40]; 
			strcpy (choice3, argv[4]);
			print_blocks_allocated(choice3);
		}
	}
	else if(strcmp(choice,"-d")) {
		delete_file(choice2);
	}
	
	close (disk_fd); 

	return (0); 
}
