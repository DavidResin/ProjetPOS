/**
 * @file db_read.c
 * @brief pictDB library: do_read implementation.
 *
 * @date 6 May 2015
 */

#include "pictDB.h"
#include "dedup.h"
#include "image_content.h"

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
            struct pictdb_file* db_file)
{
    if (db_file == NULL || size == NULL || tab == NULL) {
        return ERR_IO;
    }

    if (id == NULL) {
        return ERR_INVALID_ARGUMENT;
    }

    size_t i = 0, temp = 0;
    int ret = 0;

    if ((i = find_index(db_file, id)) == -1) {
        return ERR_FILE_NOT_FOUND;
    }
    uint64_t offset = db_file->metadata[i].offset[RES_ORIG];

    if (db_file->metadata[i].offset[code] == 0) {
        if ((ret = lazily_resize(code, db_file, i)) ||
            (ret = do_name_and_content_dedup(db_file, i))) { //Avoid to resize every image
            return ret;
        }
    }

    db_file->metadata[i].offset[RES_ORIG] = offset;
    *size = db_file->metadata[i].size[code];
    temp = (size_t) *size;

    if ((*tab = calloc(*size, sizeof(char))) == NULL) {
        return ERR_OUT_OF_MEMORY;
    }

    if ((ret = read_disk_image(db_file->fpdb, tab, temp,
                               db_file->metadata[i].offset[code]))) {
        free(*tab);
        return ret;
    }

    return 0;
}
