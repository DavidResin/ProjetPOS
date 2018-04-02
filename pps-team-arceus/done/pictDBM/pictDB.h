/**
 * @file pictDB.h
 * @brief Main header file for pictDB core library.
 *
 * Defines the format of the data structures that will be stored on the disk
 * and provides interface functions.
 *
 * The picture database starts with exactly one header structure
 * followed by exactly pictdb_header.max_files metadata
 * structures. The actual content is not defined by these structures
 * because it should be stored as raw bytes appended at the end of the
 * database file and addressed by offsets in the metadata structure.
 *
 * @date 2 Nov 2015
 */

#ifndef PICTDBPRJ_PICTDB_H
#define PICTDBPRJ_PICTDB_H

#include "error.h" 			/* not needed here, but provides it as required by
                    		 * all functions of this lib.
                    		 */
#include <stdio.h> 			// for FILE
#include <stdint.h> 		// for uint32_t, uint64_t
#include <stdlib.h> 		// for malloc, calloc
#include <string.h>
#include <openssl/sha.h> 	// for SHA256_DIGEST_LENGTH
#include <vips/vips.h>		// for vips

#define CAT_TXT "EPFL PictDB binary"

/* constraints */
#define MAX_DB_NAME 	31  	// max. size of a PictDB name
#define MAX_PIC_ID 		127  	// max. size of a picture id
#define MAX_SUFFIX_SIZE 10 		// max. size of a suffix name
#define MAX_MAX_FILES 	100000  // max. number of files in a database
#define MAX_THUMB_RES 	128		// max. thumbnail resolution
#define MAX_SMALL_RES 	512		// max. small resolution

/* default database values */
#define DEF_MAX_FILES 10		// default number of files in a database
#define DEF_THUMB_RES 64		// default thumbnail resolution
#define DEF_SMALL_RES 256       // default small resolution     		 

/* For is_valid in pictdb_metadata */
#define EMPTY 		0
#define NON_EMPTY 	1

// pictDB library internal codes for different picture resolutions.
#define RES_THUMB 0
#define RES_SMALL 1
#define RES_ORIG  2
#define NB_RES    3

#ifdef __cplusplus
extern "C" {
#endif

/*structure for the header*/
struct pictdb_header {
    char 			db_name[MAX_DB_NAME + 1];
    uint32_t		db_version;
    uint32_t 		num_files;
    uint32_t 		max_files;
    uint16_t  		res_resized[2 * (NB_RES - 1)];
    uint32_t 		unused_32;
    uint64_t 		unused_64;
};

/*structure of the metadata*/
struct pict_metadata {
    char			pict_id[MAX_PIC_ID + 1];
    unsigned char	SHA[SHA256_DIGEST_LENGTH];
    uint32_t		res_orig[2];
    uint32_t		size[NB_RES];
    uint64_t		offset[NB_RES];
    uint16_t		is_valid;
    uint16_t		unused_16;
};

/*structure of the file*/
struct pictdb_file {
    FILE*					fpdb;
    struct pictdb_header	header;
    struct pict_metadata*	metadata;
};

/*modes de fonctionnement pour do_list*/
enum do_list_mode {
    STDOUT,
    JSON
};

/**
 *  @brief  Prints database header informations.
 *
 *  @param  header :    The header to be displayed.
 */
void print_header (const struct pictdb_header* header);

/**
 *  @brief  Prints picture metadata informations.
 *
 *  @param  metadata :  The metadata of one picture.
 */
void print_metadata (const struct pict_metadata* metadata);

/**
 *  @brief  Prints the database to stdout if list = STDOUT, or returns a message
 *          containing the header and metadata in JSON format if list = JSON
 *
 *  @param  db_file :       The database to print
 *  @param  do_list_mode :  The output mode
 *
 *  @return NULL if list = STDOUT, the JSON message if list = JSON, or an error
 *          message if something goes wrong
 */
char* do_list(const struct pictdb_file* db_file, enum do_list_mode);

/**
 *  @brief  Creates the database called db_filename. Writes the header and the
 *          preallocated empty metadata array to database file.
 *
 *  @param  db_filename :   The name of the file we will create
 *  @param  db_file :       The file to create our database on
 *
 *  @return An error code
 */
int do_create(const char* db_filename, struct pictdb_file* db_file);

/**
 *  @brief  Opens and checks the database called db_filename and writes it to
 *			db_file
 *
 *  @param  db_filename :   The name of the database
 *  @param  open_mode :     The opening mode for the database
 *  @param  db_file :       The memory structure with header and metadata
 *
 *	@return An error code
 */
int do_open(const char* db_filename, const char* open_mode,
            struct pictdb_file* db_file);

/**
 *  @brief  Checks whether open_mode is part of modes
 *
 *  @param  open_mode : The open mode to check
 *  @param  modes :     The valid modes
 *  @param  num_modes : The amount of modes in the "modes" array
 *
 *  @return 0 if the mode checks, 1 otherwise
 */
int mode_check(const char* open_mode, const char** modes, const int num_modes);

/**
 *  @brief  Closes the database called db_file.
 *
 *  @param  db_file :   The memory structure with header and metadata
 */
void do_close(struct pictdb_file* db_file);

/**
 *  @brief  Deletes the image with id "id", passed through the arguments, by
 *          dereferencing it and adjusting header data. This is done inside the
 *          file and the changes are written in the disk.
 *
 *  @param  id :        The id of the image to delete
 *  @param  db_file :   The file to modify
 *
 *  @return An error code
 */
int do_delete(const char* id, struct pictdb_file* db_file);

/**
 *  @brief  Updates the header of db_file->fpdb by incrementing the version
 *			number and adding modif to the number of files, we consider that the
 *			num_files verification is made beforehand
 *
 *  @param  db_file :   The pictdb_file to edit
 *  @param  file :      The file to write in
 *  @param  modif :     The amount of files added (negative value if files were
 *  					removed)
 *  @param  update :    Determines if the version number should be incremented
 *
 *  @return 0 if writing was successful, ERR_IO otherwise
 */
int write_header(struct pictdb_file* db_file, FILE* file, int modif,
                 int update);

/**
 *  @brief  Updates the image metadata at index in db_file->fpdb, writing the
 *			metadata in the file
 *
 *  @param  db_file :   The pictdb_file to edit
 *  @param  file :      The file to write in
 *  @param  index :     The index at which to update the metadata
 *
 *  @return 0 if writing was successful, ERR_IO otherwise
 */
int write_metadata(struct pictdb_file* db_file, FILE* file, int index);

/**
 *  @brief  Compares the two sha values
 *
 *  @param  sha1 :  The first sha to compare
 *  @param  sha2 :  The second sha to compare
 *
 *  @return Returns -1 if sha1 < sha2, 0 if sha1 == sha2, 1 if sha1 > sha2
 */
int compare_sha(const unsigned char* sha1, const unsigned char* sha2);

/**
 *  @brief  Returns the resolution value that corresponds to the input string,
 *          -1 if the string is invalid
 *
 *  @param  string :    The string to read
 *
 *  @return The resolution code that matches the string or -1 if it is invalid
 */
int resolution_atoi(const char* string);

/**
 *  @brief  Reads an image of index id, resoution code and size size in db_file
 *			and puts it in tab. If the image does not exist in the resolution
 *			yet, creates it and repercutes the changes to eventual copies of the
 *			image.
 *
 *  @param  id :		The id of the picture we want to read
 *  @param  code :     	The code representing the resolution
 *  @param  tab :   	A tab of bytes
 *  @param  size :    	The size of the image
 *  @param  db_file :  	The file from where we read the image
 *
 *  @return An error code
 */
int do_read(char* id, int code, char** tab, uint32_t* size,
            struct pictdb_file* db_file);

/**
 *  @brief	Inserts an image contained in tab, of size size, with name pict_id
 *			into the first free index of db_file. Deduplicates eventual
 *			identical pictures in the process.
 *
 *	@param	tab :		An array of bytes that contains the image to insert
 *	@param	size :		The size of the image to insert
 *	@param	pict_id :	The id to give to the picture
 *	@param	db_file :	The file to add the picture to
 *
 *	@return An error code
 */
int do_insert(const char* tab, size_t size, char* pict_id,
              struct pictdb_file* db_file);

/**
 *  @brief  Writes the total size of file into size
 *
 *  @param  file : The file to read into
 *  @param  size : A pointer to write the file size into
 *
 *  @return An error code
 */
int get_file_size(FILE* file, size_t* size);

/**
 *  @brief  Reads an image from file and stores it in tab.
 *
 *  @param  file :      The file to read into
 *  @param  tab :       A pointer to the array to store the image
 *  @param  size :      A pointer to the size of the image to read
 *  @param  offset :    The offset of the image to read in the file
 *
 *  @return An error code
 */
int read_disk_image(FILE* file, char** tab, size_t size, size_t offset);

/**
 *  @brief  Writes an image of size size to the end of file from tab. Writes
 *          the image's offset in offset
 *
 *  @param  file :      The file to write into
 *  @param  tab :       An array to read the image into
 *  @param  size :      The size of the image
 *  @param  offset :    The size_t pointer to write the image's offset into
 *
 *  @return An error code
 */
int write_disk_image(FILE* file, const char* tab, size_t size, size_t* offset);

/**
 *  @brief  Creates a file name composed of an image name followed by a
 *			resolution name, the two split by a '_', and writes it into name.
 *			The name will be cut if longer than MAX_PIC_ID
 *
 *  @param  name :      A pointer to a char array that will contain the name
 *  @param  pictID :    The string to use in the name
 *  @param  code :      The resolution code to transform in a suffix
 *
 *  @return An error code
 */
int create_name(char** name, char* pictID, int code);

/**
 *  @brief  Finds the index of the valid image with id pict_id in db_file.
 *			Returns the index or -1 if the image wasn't found or the database is
 *			NULL.
 *
 *  @param  db_file :   The database to search into
 *  @param  pict_id :   The id to look for
 *
 *  @return The index of the picture or -1 if it wasn't found
 */
size_t find_index(struct pictdb_file* db_file, const char* pict_id);

/**
 *
 */
int do_gbcollect(struct pictdb_file* db_file, const char* filename, const char* tempname);

#ifdef __cplusplus
}
#endif
#endif
