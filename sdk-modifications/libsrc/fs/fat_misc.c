//fat_misc.c
//v1.0

#include "fat_misc.h"
#include "fs_api.h"

static unsigned int _usedSecNums;

static int strFindFromEnd( char *str,char strValue  )
{
	int pos = 0,i = 0,strNum = 0;
	while(1)
	{
		if( (*str)!=0 )
		{
			strNum++;
			str++;
		}
		else
		{
			break;
		}
	}
	pos = strNum;
	for( i=0;i<strNum;i++ )
	{
		str--;
		pos--;
		if( (*str) == strValue||(*str) == ':' )
		{
			return pos;
		}
		if( (*str)== 0 )
		{
			return -1;
		}
	}
	return -1; 
}

int getDirSize( const char * path, int includeSubdirs, unsigned int * dirSize )
{
    char dirPath[MAX_FILENAME_LENGTH];
	unsigned int size = 0;
    if( "" == path ){
        return false;
    }

    memset( dirPath,0,MAX_FILENAME_LENGTH );
    strcpy( dirPath,path );

    if( dirPath[strlen(dirPath)-1] != '/' )
        dirPath[strlen(dirPath)] = '/';
    if( strlen(dirPath) > MAX_FILENAME_LENGTH )
        return false;

    DIR_STATE_STRUCT *dir;
	dir = fat_opendir((const char*)dirPath);
    if (dir == NULL)
        return false;
    
    struct stat stat_buf;
	DIR_ENTRY *currentEntry;
    char* filename;

	while((currentEntry = fat_readdir_ex(dir, &stat_buf)) != NULL)
	{
		filename = currentEntry->d_name;

        if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0)
            continue;

        if (!(stat_buf.st_mode & S_IFDIR)) {
			size += (stat_buf.st_size+511)/512;
			_usedSecNums +=(stat_buf.st_size+511)/512;
		}
		else if (includeSubdirs)
		{
			// calculate the size recursively 
            unsigned int subDirSize = 0;
			char dirPathBuffer[MAX_FILENAME_LENGTH];

            memset( dirPathBuffer,0,MAX_FILENAME_LENGTH );
            strcpy( dirPathBuffer,dirPath );
            memset( dirPath,0,MAX_FILENAME_LENGTH );
            sprintf( dirPath,"%s%s",dirPathBuffer,filename );
            int succ = getDirSize( dirPath, includeSubdirs, &subDirSize );
            if( succ ) {
                size += (subDirSize+511)/512;
                _usedSecNums +=(subDirSize+511)/512;
            }
            memset( dirPath,0,MAX_FILENAME_LENGTH );
            strcpy( dirPath,dirPathBuffer );
        }
    }

	fat_closedir(dir);

    *dirSize = size;
    return true;
}

int fat_getDiskTotalSpace( char * diskName, unsigned int * diskSpace )
{
	if( !strcmp("",diskName) )
		return false;

	unsigned int len = strlen(diskName);
	if( *(diskName+len-1) != '/' ){
		*(diskName+len) = '/';
	}

	PARTITION * diskPartition = _FAT_partition_getPartitionFromPath( diskName );
	if( NULL == diskPartition )
		return false;

	*diskSpace = (unsigned int)diskPartition->numberOfSectors;
	return true;
}

int fat_getDiskSpaceInfo( char * diskName, unsigned int * total, unsigned int * used, unsigned int * freeSpace )
{
	_usedSecNums = 0;

	if( !strcmp("",diskName) )
		return -1;
    if( !fat_getDiskTotalSpace(diskName, total) )
		return -1;
	if( !getDirSize(diskName, true, used) )
		return -1;

    *used = _usedSecNums;
    if( *total <= *used ){
   	 	*freeSpace = 0;
   	}else{
    	*freeSpace = *total - *used;
  	}

    return 0;
}
