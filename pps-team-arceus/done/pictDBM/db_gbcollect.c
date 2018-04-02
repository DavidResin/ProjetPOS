/**
 * @file db_gbcollect.c
 *
 * @date 26 mai 2016
 */

#include "pictDB.h"
#include "image_content.h"

/**
 *  @brief  Copies only the valid data from the db_file called filename
 * 			into a new created db_file called with tempname, removes the
 * 			filename and renames the tempname with filename
 *
 *  @param 	db_file :       The file we want to clean
 * 	@param 	filename :		The name of the file from from which we want to clean
 * 							the invalid data
 * 	@param 	tempname :		The name of the file were we want to copy
 * 							the temporary data
 *
 *  @return An error code
 */
int do_gbcollect(struct pictdb_file* db_file, const char* filename, const char* tempname)
{
    int ret = 0;
    char* tab;
    uint32_t size = 0;

    if (db_file == NULL || db_file->fpdb == NULL) {
        return ERR_INVALID_ARGUMENT;
    }

    struct pictdb_file database;
    database.header.max_files = db_file->header.max_files;

    for (size_t i = 0; i < 2 * (NB_RES - 1); i++) {
        database.header.res_resized[i] = db_file->header.res_resized[i];
    }

    if ((ret = do_create(tempname, &database))) {
        return ret;
    }

    size_t index = 0;

    for (size_t i = 0; i < db_file->header.max_files; i++) {
        if (db_file->metadata[i].is_valid == NON_EMPTY) {
            if ((ret = do_read(db_file->metadata[i].pict_id, RES_ORIG, &tab, &size, db_file))) {
                do_close(&database);
                return ret;
            }

            if ((ret = do_insert(tab, size, db_file->metadata[i].pict_id, &database))) {
                free(tab);
                do_close(&database);
                return ret;
            }

            free(tab);

            for (size_t j = 0; j < NB_RES; j++) {
                if (db_file->metadata[i].offset[j]) {
                    if ((ret = lazily_resize(j, &database, index))) {
                        do_close(&database);
                        return ret;
                    }
                }
            }

            index++;
        }
    }

    strncpy(database.header.db_name, db_file->header.db_name, MAX_DB_NAME);
    database.header.db_version = db_file->header.db_version;
    if ((ret = write_header(&database, database.fpdb, 0, 0))) {
        do_close(&database);
        return ret;
    }

    do_close(db_file);
    do_close(&database);

    if (remove(filename) || rename(tempname, filename)) {
        return ERR_IO;
    }

    return 0;
}
