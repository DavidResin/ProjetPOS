/**
 * @file db_create.c
 * @brief pictDB library: do_create implementation.
 *
 * @date 2 Nov 2015
 */

#include "pictDB.h"

/**
 *  @brief  Creates the database called db_filename. Writes the header and the
 *          preallocated empty metadata array to database file.
 *
 *  @param  db_filename :   The name of the file we will create
 *  @param  db_file :       The file to create our database on
 *
 *  @return An error code
 */
int do_create(const char* db_filename, struct pictdb_file* db_file)
{
    strncpy(db_file->header.db_name, CAT_TXT, MAX_DB_NAME);
    db_file->header.db_name[MAX_DB_NAME] = '\0';

    if (strlen(db_filename) > MAX_DB_NAME) {
        return ERR_INVALID_FILENAME;
    }

    strncpy(db_file->header.db_name, db_filename, MAX_DB_NAME);

    db_file->header.db_version = 0;
    db_file->header.num_files = 0;

    if (db_file->header.max_files > MAX_MAX_FILES) {
        db_file->header.max_files = MAX_MAX_FILES;
    }

    if ((db_file->metadata = calloc(db_file->header.max_files,
                                    sizeof(struct pict_metadata))) == NULL) {
        return ERR_OUT_OF_MEMORY;
    }

    for (int i = 0; i < db_file->header.max_files; i++) {
        db_file->metadata[i].is_valid = EMPTY;
    }

    FILE* file = fopen(db_filename, "w+b");

    if (file == NULL) {
        return ERR_IO;
    }

    db_file->fpdb = file;

    int ret = 0;
    int counter = 1;

    if ((ret = write_header(db_file, file, 0, 1))) {
        return ret;
    }

    for (int i = 0; i < db_file->header.max_files; i++) {
        if ((ret = write_metadata(db_file, file, i))) {
            return ret;
        }
        counter++;
    }

    printf("%d item(s) written\n", counter);
    return 0;
}
