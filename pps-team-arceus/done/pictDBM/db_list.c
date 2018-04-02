/**
 * 	@file db_list.c
 * 	@brief 	implementation of a function which preints the header of the files
 *			and the metadata only of the images in the file.
 *
 * 	The fonction do_list(file) print le header of the file in the argument and
 *	then verifies the number of images inside the file. If there is no image
 *	inside the file it will print "<< empty database >>" if not it will go
 *	through the images and print the metadata.
 *
 *  @date 14 mar 2016
 */

#include "pictDB.h"
#include <json-c/json.h>

#define ERROR_MSG_SIZE 64

/**
 *  @brief  Prints the database to stdout if list = STDOUT, or returns a message
 *          containing the header and metadata in JSON format if list = JSON
 *
 *  @param  db_file :       The database to print
 *  @param  do_list_mode :  The output mode
 *
 *  @return NULL if list = STDOUT, the JSON message if list = JSON, or an error
 *          message if something goes wrong
 */
char* do_list(const struct pictdb_file* db_file, enum do_list_mode list)
{
    if (list == STDOUT) {
        if (db_file == NULL) {
            char* message = calloc(ERROR_MSG_SIZE, sizeof(char));
            strcpy(message, "Impossible d'afficher le fichier: NullPointer\n");
            return message;
        }

        print_header(&(db_file->header));

        if (db_file->header.num_files == 0) {
            printf("<< empty database >>\n");
        } else for (size_t i = 0; i < db_file->header.max_files; i++) {
                if (db_file->metadata[i].is_valid == NON_EMPTY) {
                    print_metadata(&(db_file->metadata[i]));
                }
            }
        return NULL;
    } else if (list == JSON) {
        struct json_object* temp = json_object_new_array();
        struct json_object* result = json_object_new_object();

        for (size_t i = 0; i < db_file->header.max_files; i++) {
            if (db_file->metadata[i].is_valid == NON_EMPTY) {
                json_object_array_add(temp, json_object_new_string(db_file->metadata[i].pict_id));
            }
        }

        if (temp == NULL) {
            char* message = calloc(ERROR_MSG_SIZE, sizeof(char));
            strcpy(message, "The json_array couldn't be created.\n");
            return message;
        }

        if (result == NULL) {
            char* message = calloc(ERROR_MSG_SIZE, sizeof(char));
            strcpy(message, "The json_object couldn't be created.\n");
            return message;
        }

        const char* pics = "Pictures";
        json_object_object_add(result, pics, temp);
        const char* string = json_object_to_json_string(result);
        char* message = calloc(strlen(string) + 1, sizeof(char));
        strcpy(message, string);
        json_object_put(result);

        return message;
    } else {
        char* message = calloc(ERROR_MSG_SIZE, sizeof(char));
        strcpy(message, "unimplemented do_list mode\n");
        return message;
    }
    return NULL;
}
