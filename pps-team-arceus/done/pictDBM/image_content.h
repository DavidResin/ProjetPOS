/**
 * @file image_content.h
 *
 * @date 14 mar 2016
 */

#ifndef PICTDBPRJ_IMAGE_CONTENT_H
#define PICTDBPRJ_IMAGE_CONTENT_H

#include "pictDB.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 	@brief 	Resizes the image from db_file at index with the code resolution
 *
 *	@param	code :		The code for the resolution we want
 *	@param	db_file :	The file to work on
 *	@param	index :		The index of the image
 *
 *	@return An error code
 */
int lazily_resize(int code, struct pictdb_file* db_file, size_t index);

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
                   size_t image_size);

#ifdef __cplusplus
}
#endif
#endif
