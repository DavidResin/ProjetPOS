/* ** NOTE: undocumented in Doxygen
 * @file db_utils.c
 * @brief implementation of several tool functions for pictDB
 *
 * @date 2 Nov 2015
 */

#include "pictDB.h"

#include <stdint.h> // for uint8_t
#include <stdio.h> // for sprintf
#include <openssl/sha.h> // for SHA256_DIGEST_LENGTH
#include <inttypes.h> // for PRIu16 - PRIu32- PRIu64
#include <string.h> // for strlen

/**
 *  @brief  Converts a SHA to a string
 *
 *  @param  SHA :           The SHA to convert
 *  @param  sha_string :    The string to write in
 */
static void
sha_to_string (const unsigned char* SHA, char* sha_string)
{
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        sprintf(&sha_string[i * 2], "%02x", SHA[i]);
    }

    sha_string[2 * SHA256_DIGEST_LENGTH] = '\0';
}

/**
 * @brief   Prints a header to stdout
 *
 * @param   header :    The header to print
 */
void print_header (const struct pictdb_header* header)
{

    if (header == NULL) {
        fprintf(stderr, "Impossible d'afficher le header: NullPointer\n");
        return;
    }

    const uint16_t* res = header->res_resized;
    //implemented function which prints the header of the file
    printf("*****************************************\n");
    printf("**********DATABASE HEADER START**********\n");
    printf("DB NAME: %31s\n", 					header->db_name);
    printf("VERSION: %" PRIu32 "\n",			header->db_version);
    printf("IMAGE COUNT: %" PRIu32 "\t\t", 		header->num_files);
    printf("MAX IMAGES: %" PRIu32 "\n", 		header->max_files);
    printf("THUMBNAIL: %" PRIu16 " x %" PRIu16 "\t",
           res[2 * RES_THUMB], res[2 * RES_THUMB + 1]);
    printf("SMALL: %" PRIu16 " x %" PRIu16 "\n",
           res[2 * RES_SMALL], res[2 * RES_SMALL + 1]);
    printf("***********DATABASE HEADER END***********\n");
    printf("*****************************************\n");
}

/**
 * @brief   Prints a metadata to stdout
 *
 * @param   metadata :    The metadata to print
 */
void print_metadata (const struct pict_metadata* metadata)
{

    if (metadata == NULL) {
        fprintf(stderr, "Impossible d'afficher la metadata: NullPointer\n");
        return;
    }

    char sha_printable[2*SHA256_DIGEST_LENGTH+1];
    sha_to_string(metadata->SHA, sha_printable);
    //implemented function printing the metadata of the images
    printf("PICTURE ID: %s\n", 					metadata->pict_id);
    printf("SHA: %31s\n", 						sha_printable);
    printf("VALID: %" PRIu16 "\n", 				metadata->is_valid);
    printf("UNUSED: %" PRIu16 "\n", 			metadata->unused_16);
    printf("OFFSET ORIG. : %" PRIu64 "\t\t", 	metadata->offset[RES_ORIG]);
    printf("SIZE ORIG. : %" PRIu32 "\n", 		metadata->size[RES_ORIG]);
    printf("OFFSET THUMB.: %" PRIu64 "\t\t", 	metadata->offset[RES_THUMB]);
    printf("SIZE THUMB.: %" PRIu32 "\n", 		metadata->size[RES_THUMB]);
    printf("OFFSET SMALL : %" PRIu64 "\t\t", 	metadata->offset[RES_SMALL]);
    printf("SIZE SMALL : %" PRIu32 "\n", 		metadata->size[RES_SMALL]);
    printf("ORIGINAL: %" PRIu32 " x %" PRIu32 "\n",
           metadata->res_orig[0], metadata->res_orig[1]);
    printf("*****************************************\n");
}

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
            struct pictdb_file* db_file)
{
    if (db_filename == NULL || open_mode == NULL || db_file == NULL) {
        return ERR_INVALID_ARGUMENT;
    }

    const char* modes[] = {"rb", "rb+", "r+b", "wb", "wb+",
                           "w+b", "ab", "ab+", "a+b"
                          };
    const int num_modes = sizeof(modes) / sizeof(char*);

    if (!mode_check(open_mode, modes, num_modes)) {
        return ERR_INVALID_ARGUMENT;
    }

    if (strlen(db_filename) > MAX_DB_NAME) {
        return ERR_INVALID_FILENAME;
    }

    if (db_file->header.max_files > MAX_MAX_FILES) {
        db_file->header.max_files = MAX_MAX_FILES;
    }

    FILE* temp = fopen(db_filename, open_mode);

    if (temp == NULL) {
        return ERR_IO;
    }

    db_file->fpdb = temp;

    if (fread(&(db_file->header), sizeof(struct pictdb_header), 1, temp) != 1) {
        do_close(db_file);
        return ERR_IO;
    }

    if ((db_file->metadata = calloc(db_file->header.max_files,
                                    sizeof(struct pict_metadata))) == NULL) {
        do_close(db_file);
        return ERR_OUT_OF_MEMORY;
    }

    if (fread(db_file->metadata, sizeof(struct pict_metadata),
              db_file->header.max_files, temp) != db_file->header.max_files) {
        do_close(db_file);
        return ERR_IO;
    }

    return 0;
}

/**
 *	@brief  Checks whether open_mode is part of modes
 *
 *  @param  open_mode : The open mode to check
 *  @param  modes :     The valid modes
 *  @param  num_modes : The amount of modes in the "modes" array
 *
 *  @return 0 if the mode checks, 1 otherwise
 */
int mode_check(const char* open_mode, const char** modes, const int num_modes)
{
    if (open_mode == NULL || modes == NULL) {
        return 0;
    }

    for (size_t i = 0; i < num_modes; i++) {
        if (modes[i] == NULL) {
            return 0;
        }

        if (!strcmp(open_mode, modes[i])) {
            return 1;
        }
    }

    return 0;
}

/**
 *  @brief  Closes the database called db_file.
 *
 *  @param  db_file :   The memory structure with header and metadata
 */
void do_close(struct pictdb_file* db_file)
{
    if (db_file != NULL) {
        if (db_file->fpdb != NULL) {
            fclose(db_file->fpdb);
            db_file->fpdb = NULL;
        }

        if (db_file->metadata != NULL) {
            free(db_file->metadata);
            db_file->metadata = NULL;
        }
    }
}

/**
 *  @brief  Updates the header of db_file->fpdb by incrementing the version
 *			number and adding modif to the number of files, we consider that the
 *			num_files verification is made beforehand
 *
 *  @param  db_file :   The pictdb_file to edit
 *  @param  file :      The file to write in
 *  @param  modif :     The amount of files added (negative value if files were
 *                      removed)
 *  @param  update :    Determines if the version number should be incremented
 *
 *  @return 0 if writing was successful, ERR_IO otherwise
 */
int write_header(struct pictdb_file* db_file, FILE* file, int modif, int update)
{
    if (db_file == NULL || file == NULL) {
        return ERR_INVALID_ARGUMENT;
    }

    if (fseek(file, 0, SEEK_SET)) {
        return ERR_IO;
    }

    if (update) {
        db_file->header.db_version += 1;
    }

    db_file->header.num_files += modif;

    return !fwrite(&(db_file->header), sizeof(struct pictdb_header), 1, file);
}

/**
 *  @brief  Updates the image metadata at index in db_file->fpdb, writing the
 *          metadata in the file
 *
 *  @param  db_file :   The pictdb_file to edit
 *  @param  file :      The file to write in
 *  @param  index :     The index at which to update the metadata
 *
 *  @return 0 if writing was successful, ERR_IO otherwise
 */
int write_metadata(struct pictdb_file* db_file, FILE* file, int index)
{
    if (db_file == NULL || file == NULL) {
        return ERR_INVALID_ARGUMENT;
    }

    if (fseek(file, sizeof(struct pictdb_header) + index *
              sizeof(struct pict_metadata), SEEK_SET)) {
        return ERR_IO;
    }
    return !fwrite(&(db_file->metadata[index]),
                   sizeof(struct pict_metadata), 1, file);
}

/**
 *  @brief  Compares the two sha values
 *
 *  @param  sha1 :  The first sha to compare
 *  @param  sha2 :  The second sha to compare
 *
 *  @return -1 if sha1 < sha2, 0 if sha1 == sha2, 1 if sha1 > sha2 or if one
 *          of the shas is NULL
 */
int compare_sha(const unsigned char* sha1, const unsigned char* sha2)
{
    if(sha1 == NULL || sha2 == NULL) {
        return 1;
    }

    return memcmp(sha1, sha2, SHA256_DIGEST_LENGTH);
}

/**
 *  @brief  Returns the resolution value that corresponds to the input string,
 *          -1 if the string is invalid
 *
 *  @param  string :    The string to read
 *
 *  @return The resolution code that matches the string or -1 if it is invalid
 */
int resolution_atoi(const char* string)
{
    if (string == NULL) {
        return -1;
    }

    if (!strcmp("thumb", string) || !strcmp("thumbnail", string)) {
        return RES_THUMB;
    } else if (!strcmp("small", string)) {
        return RES_SMALL;
    } else if (!strcmp("orig", string) || !strcmp("original", string)) {
        return RES_ORIG;
    }

    return -1;
}

/**
 *  @brief  Writes the total size of file into size
 *
 *  @param  file : The file to read into
 *  @param  size : A pointer to write the file size into
 *
 *  @return An error code
 */
int get_file_size(FILE* file, size_t* size)
{
    if (size == NULL) {
        return ERR_INVALID_ARGUMENT;
    }

    if (file == NULL ||
        fseek(file, 0, SEEK_END) ||
        (*size = ftell(file)) == 0) {
        return ERR_IO;
    }

    return 0;
}

/**
 *  @brief  Reads an image from file and stores it in tab.
 *
 *  @param  file :      The file to read into
 *  @param  tab :       A pointer to the array to store the image
 *  @param  size :      The size of the image to read
 *  @param  offset :    The offset of the image to read in the file
 *
 *  @return An error code
 */
int read_disk_image(FILE* file, char** tab, size_t size, size_t offset)
{
    if (tab == NULL || *tab == NULL) {
        return ERR_INVALID_ARGUMENT;
    }

    int a = 0, b = 0, c = 0;

    if ((a = (file == NULL)) ||
        (b = fseek(file, offset, SEEK_SET)) ||
        (c = !fread(*tab, sizeof(char), size, file))) {
        return ERR_IO;
    }

    return 0;
}

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
int write_disk_image(FILE* file, const char* tab, size_t size, size_t* offset)
{
    int ret = 0;
    if (tab == NULL) {
        return ERR_INVALID_ARGUMENT;
    }

    if (file == NULL) {
        return ERR_IO;
    }

    if (*offset == 0 &&
        (ret = get_file_size(file, offset))) {
        return ret;
    }

    if (!fwrite(tab, sizeof(char), size, file)) {
        return ERR_IO;
    }

    return 0;
}

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
int create_name(char** name, char* pictID, int code)
{
    if (name == NULL || pictID == NULL) {
        return ERR_INVALID_ARGUMENT;
    }

    if (code < 0 || code >= NB_RES) {
        return ERR_RESOLUTIONS;
    }

    if ((*name = calloc(MAX_PIC_ID + MAX_SUFFIX_SIZE + 1,
                        sizeof(char))) == NULL) {
        return ERR_OUT_OF_MEMORY;
    }

    strncpy(*name, pictID, MAX_PIC_ID);
    (*name)[MAX_PIC_ID] = '\0';
    char* temp = &(*name)[strlen(*name)];

    switch (code) {
    case RES_THUMB:
        strncpy(temp, "_thumb.jpg", MAX_SUFFIX_SIZE);
        break;
    case RES_SMALL:
        strncpy(temp, "_small.jpg", MAX_SUFFIX_SIZE);
        break;
    case RES_ORIG:
        strncpy(temp, "_orig.jpg\0", MAX_SUFFIX_SIZE);
        break;
    }

    return 0;
}

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
size_t find_index(struct pictdb_file* db_file, const char* pict_id)
{
    if (db_file == NULL || pict_id == NULL) {
        return -1;
    }

    for (size_t i = 0; i < db_file->header.max_files; i++) {
        if (db_file->metadata[i].is_valid == NON_EMPTY &&
            !strcmp(db_file->metadata[i].pict_id, pict_id)) {
            return i;
        }
    }
    return -1;
}