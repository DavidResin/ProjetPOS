/**
 * @file db_insert.c
 *
 * @date 2 mai 2016
 */

#include "pictDB.h"
#include "dedup.h"
#include "image_content.h"

#include <openssl/sha.h>

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
              struct pictdb_file* db_file)
{
    if (db_file == NULL) {
        return ERR_INVALID_ARGUMENT;
    }

    if (db_file->header.num_files >= db_file->header.max_files) {
        return ERR_FULL_DATABASE;
    }

    for (size_t i = 0; i < db_file->header.max_files; i++) {
        if (db_file->metadata[i].is_valid == EMPTY) {
            SHA256((unsigned char*) tab, size, db_file->metadata[i].SHA);
            strncpy(db_file->metadata[i].pict_id, pict_id, MAX_PIC_ID + 1);
            db_file->metadata[i].size[RES_ORIG] = (uint32_t) size;

            int ret = 0;

            if ((ret = do_name_and_content_dedup(db_file, i))) {
                return ret;
            }

            if (db_file->metadata[i].offset[RES_ORIG] == 0) {
                uint32_t height = 0;
                uint32_t width = 0;

                if ((ret = get_resolution(&height, &width, tab, size))) {
                    return ret;
                }

                db_file->metadata[i].res_orig[0] = width;
                db_file->metadata[i].res_orig[1] = height;

                if ((ret = write_disk_image(db_file->fpdb, tab, size,
                                            &(db_file->metadata[i].offset[RES_ORIG])))) {
                    return ret;
                }

                db_file->metadata[i].offset[RES_SMALL] = 0;
                db_file->metadata[i].offset[RES_THUMB] = 0;

                db_file->metadata[i].size[RES_SMALL] = 0;
                db_file->metadata[i].size[RES_THUMB] = 0;
            }

            db_file->metadata[i].is_valid = NON_EMPTY;

            if ((ret = write_header(db_file, db_file->fpdb, 1, 1)) ||
                (ret = write_metadata(db_file, db_file->fpdb, i))) {
                return ret;
            }

            return 0;
        }
    }

    return ERR_IO;
}
