/**
 * @file dedup.h
 * @brief deduplications tools
 *
 * @date 29 Apr 2016
 */

#ifndef PICTDBPRJ_DEDUP_H
#define PICTDBPRJ_DEDUP_H

#include "pictDB.h"

#include <stdint.h>

/*
 *	@brief	Deduplicates the image at index in the db_file if it appears more
 *  		than once
 *
 *	@param	db_file :	The file to analyse
 *	@param	index :		The index of the image to deduplicate
 *
 *	@return An error code
 */
int do_name_and_content_dedup(struct pictdb_file* db_file, const uint32_t index);

#ifdef __cplusplus
}
#endif
#endif