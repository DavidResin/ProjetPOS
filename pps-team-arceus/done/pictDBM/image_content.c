/**
 * @file image_content.c
 *
 * @date 14 mar 2016
 */

#include "pictDB.h"

// ======================================================================
/**
 * 	@brief 	Computes the shrinking factor (keeping aspect ratio)
 *			(CODE FROM WEEK 2)
 *
 * 	@param 	image :					The image to be resized.
 * 	@param 	max_thumbnail_width :	The maximum width allowed for thumbnail
 *									creation.
 * 	@param 	max_thumbnail_height :	The maximum height allowed for thumbnail
 *									creation.
 *
 * 	@return	The shrinking factor
 */
double
shrink_value(const VipsImage* image,
             int max_thumbnail_width,
             int max_thumbnail_height)
{
    const double h_shrink = (double) max_thumbnail_width
                            / (double) image->Xsize;
    const double v_shrink = (double) max_thumbnail_height
                            / (double) image->Ysize;
    return h_shrink > v_shrink ? v_shrink : h_shrink ;
}

/**
 * 	@brief 	Resizes the image from db_file at index with the code resolution
 *			(CODE FROM WEEK 2)
 *
 *	@param	code :		The code for the resolution we want
 *	@param	db_file :	The file to work on
 *	@param	index :		The index of the image
 *
 *	@return An error code
 */
int lazily_resize(int code, struct pictdb_file* db_file, size_t index)
{
    if (db_file == NULL) {
        return ERR_INVALID_ARGUMENT;
    }

    if (code >= NB_RES || code < 0) {
        return ERR_RESOLUTIONS;
    }

    if (index >= db_file->header.max_files) {
        return ERR_INVALID_PICID;
    }

    if (code != RES_ORIG && db_file->metadata[index].offset[code] == 0) {
        size_t size = db_file->metadata[index].size[RES_ORIG];
        size_t olen = 0;
        void* buffer = NULL;
        char* obuf = NULL;
        int ret = 0;

        if ((buffer = calloc(size, sizeof(char))) == NULL) {
            return ERR_OUT_OF_MEMORY;
        }

        if ((ret = read_disk_image(db_file->fpdb, (char**) &buffer,
                                   size, db_file->metadata[index].offset[RES_ORIG]))) {
            free(buffer);
            return ret;
        }

        VipsObject* process = VIPS_OBJECT(vips_image_new());
        VipsImage** thumbs = (VipsImage**) vips_object_local_array(process, 1);

        if (vips_jpegload_buffer(buffer, size, thumbs, NULL)) {
            g_object_unref(process);
            free(buffer);
            return ERR_OUT_OF_MEMORY;
        }

        double ratio = shrink_value(*thumbs,
                                    db_file->header.res_resized[2 * code],
                                    db_file->header.res_resized[2 * code + 1]);

#if VIPS_MAJOR_VERSION > 7 || (VIPS_MAJOR_VERSION == 7 && MINOR_VERSION > 40)

        // was only introduced in libvips 7.42
        vips_resize(thumbs[0], &thumbs[0], ratio, NULL);

#else

        if (ratio < 1.0) {
            ratio = (int) (1./ratio) + 1.0;
            vips_shrink(thumbs[0], &thumbs[0], ratio, ratio, NULL);
        }

#endif

        if (vips_jpegsave_buffer(thumbs[0], (void**) &obuf, &olen, NULL)) {
            g_object_unref(process);
            free(buffer);
            return ERR_OUT_OF_MEMORY;
        }

        db_file->metadata[index].size[code] = olen;

        if ((ret = write_disk_image(db_file->fpdb, obuf, olen,
                                    &(db_file->metadata[index].offset[code]))) ||
            (ret = write_metadata(db_file, db_file->fpdb, index)) ||
            (ret = write_header(db_file, db_file->fpdb, 0, 0))) {
            g_object_unref(process);
            g_free(obuf);
            free(buffer);
            return ret;
        }

        g_object_unref(process);
        g_free(obuf);
        free(buffer);
    }

    return 0;
}

/**
 *  @brief	Extracts the resolution values of image_buffer with size image_size
 *			using vips and writes them in height and width
 *
 *	@param	height :		The height to write into
 *	@param	width :			The width to write into
 *	@param	image_buffer :	The buffer that contains the image
 *	@param	image_size :	The size of the image
 *
 *	@return An error code
 */
int get_resolution(uint32_t* height, uint32_t* width, const char* image_buffer,
                   size_t image_size)
{
    VipsObject* process = VIPS_OBJECT(vips_image_new());
    VipsImage** thumbs = (VipsImage**) vips_object_local_array(process, 1);

    if (vips_jpegload_buffer((void*) image_buffer, image_size, thumbs, NULL)) {
        g_object_unref(process);
        return ERR_VIPS;
    }

    *width = thumbs[0]->Xsize;
    *height = thumbs[0]->Ysize;

    g_object_unref(process);

    return 0;
}
