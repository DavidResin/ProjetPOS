/**
 * @file dedup.c
 * @brief deduplications tools
 *
 * @date 29 Apr 2016
 */

#include "pictDB.h"

/*
 *	@brief	Deduplicates the image at index in the db_file if it appears more
 *  		than once
 *
 *	@param	db_file :	The file to analyse
 *	@param	index :		The index of the image to deduplicate
 *
 *	@return An error code
 */
int do_name_and_content_dedup(struct pictdb_file* db_file, uint32_t index)
{
    int ret = 0;

    for (uint32_t i = 0; i < db_file->header.max_files; i++) {
        if (i != index && db_file->metadata[i].is_valid == NON_EMPTY) {
            if (!strcmp(db_file->metadata[i].pict_id,
                        db_file->metadata[index].pict_id)) {
                db_file->metadata[index].is_valid = EMPTY;
                return ERR_DUPLICATE_ID;
            }

            if (!compare_sha(db_file->metadata[i].SHA,
                             db_file->metadata[index].SHA)) {
                uint32_t copyTo = 0;
                uint32_t copyFrom = 0;

                for (size_t j = 0; j < NB_RES; j++) {
                    copyTo = index;
                    copyFrom = i;

                    //If the other metadata is missing an image, we swap the process
                    if (db_file->metadata[i].offset[j] == 0) {
                        copyTo = i;
                        copyFrom = index;
                    }

                    db_file->metadata[copyTo].offset[j] =
                        db_file->metadata[copyFrom].offset[j];
                    db_file->metadata[copyTo].size[j] =
                        db_file->metadata[copyFrom].size[j];
                }

                db_file->metadata[index].res_orig[0] =
                    db_file->metadata[i].res_orig[0];
                db_file->metadata[index].res_orig[1] =
                    db_file->metadata[i].res_orig[1];

                if ((ret = write_header(db_file, db_file->fpdb, 0, 0)) ||
                    (ret = write_metadata(db_file, db_file->fpdb, i)) ||
                    (ret = write_metadata(db_file, db_file->fpdb, index))) {
                    return ret;
                }

                return 0;
            }
        }
    }
    db_file->metadata[index].offset[RES_ORIG] = 0;

    return 0;
}
