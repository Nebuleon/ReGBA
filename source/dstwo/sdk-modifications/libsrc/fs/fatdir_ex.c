#include <string.h>

#include <errno.h>

#include <ctype.h>

#include <unistd.h>

#include <sys/dir.h>


#include "fatdir.h"

#include "fatdir_ex.h"


#include "cache.h"

#include "file_allocation_table.h"

#include "partition.h"

#include "directory.h"

#include "bit_ops.h"

#include "filetime.h"

#include "unicode/unicode.h"

int dirnextl (DIR_ITER *dirState, char *filename, char *longFilename, struct stat *filestat)
{
	DIR_STATE_STRUCT* state = (DIR_STATE_STRUCT*) (dirState->dirStruct);

	
	// Make sure we are still using this entry

	if (!state->inUse) {

		errno = EBADF;
		return -1;

	}


	// Make sure there is another file to report on
	
	if (! state->validEntry) {
		errno = ENOENT;

		return -1;

	}


	// Get the filename
	
	strncpy (filename, state->currentEntry.d_name, MAX_FILENAME_LENGTH);

	// Get long filename
	_FAT_unicode_unicode_to_local( state->currentEntry.unicodeFilename, (u8 *)longFilename );


	// Get the stats, if requested

	if (filestat != NULL) {

		_FAT_directory_entryStat (state->partition, &(state->currentEntry), filestat);

	}

	
	// Look for the next entry for use next time

	state->validEntry = 
_FAT_directory_getNextEntry (state->partition, &(state->currentEntry));


	return 0;
}

int renamex( const char *oldName, const char *newName )
{
	PARTITION* partition = NULL;
	DIR_ENTRY oldDirEntry;
	DIR_ENTRY newDirEntry;
	const char *pathEnd;
	u32 dirCluster;
	
	// Get the partition this directory is on
	partition = _FAT_partition_getPartitionFromPath (oldName);
	
	if (partition == NULL) {
		errno = ENODEV;
		return -1;
	}
	
	// Make sure the same partition is used for the old and new names
	if (partition != _FAT_partition_getPartitionFromPath (newName)) {
		errno = EXDEV;
		return -1;
	}

	// Make sure we aren't trying to write to a read-only disc
	if (partition->readOnly) {
		errno = EROFS;
		return -1;
	}

	// Move the path pointer to the start of the actual path
	if (strchr (oldName, ':') != NULL) {
		oldName = strchr (oldName, ':') + 1;
	}
	if (strchr (oldName, ':') != NULL) {
		errno = EINVAL;
		return -1;
	}
	if (strchr (newName, ':') != NULL) {
		newName = strchr (newName, ':') + 1;
	}
	if (strchr (newName, ':') != NULL) {
		errno = EINVAL;
		return -1;
	}

	// Search for the file on the disc
	if (!_FAT_directory_entryFromPath (partition, &oldDirEntry, oldName, NULL)) {
		errno = ENOENT;
		return -1;
	}
	
	// Make sure there is no existing file / directory with the new name
	if (_FAT_directory_entryFromPath (partition, &newDirEntry, newName, NULL)) {
		errno = EEXIST;
		return -1;
	}

	// Create the new file entry
	// Get the directory it has to go in 
	pathEnd = strrchr (newName, DIR_SEPARATOR);
	if (pathEnd == NULL) {
		// No path was specified
		dirCluster = partition->cwdCluster;
		pathEnd = newName;
	} else {
		// Path was specified -- get the right dirCluster
		// Recycling newDirEntry, since it needs to be recreated anyway
		if (!_FAT_directory_entryFromPath (partition, &newDirEntry, newName, pathEnd) ||
			!_FAT_directory_isDirectory(&newDirEntry)) {
			errno = ENOTDIR;
			return -1;
		}
		dirCluster = _FAT_directory_entryGetCluster (newDirEntry.entryData);
		// Move the pathEnd past the last DIR_SEPARATOR
		pathEnd += 1;
	}

	// Copy the entry data
	memcpy (&newDirEntry, &oldDirEntry, sizeof(DIR_ENTRY));
	
	// Set the new name
	strncpy (newDirEntry.d_name, pathEnd, MAX_FILENAME_LENGTH - 1);
	
	// Write the new entry
	if (!_FAT_directory_addEntry (partition, &newDirEntry, dirCluster)) {
		errno = ENOSPC;
		return -1;
	}
	
	// Remove the old entry
	if (!_FAT_directory_removeEntry (partition, &oldDirEntry)) {
		errno = EIO;
		return -1;
	}
	
	// Flush any sectors in the disc cache
	if (!_FAT_cache_flush (partition->cache)) {
		errno = EIO;
		return -1;
	}

	return 0;
}
