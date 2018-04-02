/**
 * @file db_delete.c
 * @brief pictDB library: do_delete implementation.
 *
 * @date 2 Nov 2015
 */

#include "pictDB.h"

/**
 *	@brief  Deletes the image with id "id", passed through the arguments, by
 *			dereferencing it and adjusting header data. This is done inside the
 *			file and the changes are written in the disk.
 *
 *  @param  id :        The id of the image to delete
 *  @param  db_file :   The file to modify
 *
 *  @return An error code
 */
int do_delete(const char* id, struct pictdb_file* db_file)
{
    if (db_file == NULL) {
        return ERR_INVALID_ARGUMENT;
    }

    size_t i = 0;

    if ((i = find_index(db_file, id)) == -1) {
        return ERR_INVALID_PICID;
    } else {
        db_file->metadata[i].is_valid = EMPTY;
        int ret = 0;

        if ((ret = write_metadata(db_file, db_file->fpdb, i)) ||
            (ret = write_header(db_file, db_file->fpdb, -1, 1))) {
            return ret;
        }

        return 0;
    }
}
