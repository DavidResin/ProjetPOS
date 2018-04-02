/**
 * @file pictDBM.c
 * @brief pictDB Manager: command line interpretor for pictDB core commands.
 *
 * Picture Database Management Tool
 *
 * @date 2 Nov 2015
 */

#include "pictDB.h"
#include "image_content.h"
#include "pictDBM_tools.h"

#define COMMAND_COUNT 7

typedef int (*command)(int, char**);

typedef struct {
    const char* chaine;
    command comm;
} command_mapping;

/********************************************************************//**
 * Displays some explanations.
 ********************************************************************** */
int help(int args, char *argv[])
{
    if (args > 1) {
        return ERR_INVALID_ARGUMENT;
    }

    puts("pictDBM [COMMAND] [ARGUMENTS]");

    puts("\thelp: displays this help.");

    puts("\tlist <dbfilename>: list pictDB content.");

    puts("\tcreate <dbfilename>: create a new pictDB.");
    puts("\t\toptions are:");
    puts("\t\t\t-max_files <MAX_FILES>: maximum number of files.");
    puts("\t\t\t\t\t\t\t\t\tdefault value is 10");
    puts("\t\t\t\t\t\t\t\t\tmaximum value is 100000");
    puts("\t\t\t-thumb_res <X_RES> <Y_RES>: resolution for thumbnail images.");
    puts("\t\t\t\t\t\t\t\t\tdefault value is 64x64");
    puts("\t\t\t\t\t\t\t\t\tmaximum value is 128x128");
    puts("\t\t\t-small_res <X_RES> <Y_RES>: resolution for small images.");
    puts("\t\t\t\t\t\t\t\t\tdefault value is 256x256");
    puts("\t\t\t\t\t\t\t\t\tmaximum value is 512x512");

    puts("\tread <dbfilename> <pictID> [original|orig|thumbnail|thumb|small]:");
    puts("\t\tread an image from the pictDB and save it to a file.");
    puts("\t\tdefault resolution is \"original\".");

    puts("\tinsert <dbfilename> <pictID> <filename>:"
         " insert a new image in the pictDB.");

    puts("\tdelete <dbfilename> <pictID>: delete picture pictID from pictDB.");

    puts("\tgc <dbfilename> <tmp dbfilename>: performs garbage collecting on pictDB."
         " Requires a temporary filename for copying the pictDB.");

    return 0;
}

/********************************************************************//**
 * Opens pictDB file and calls do_list command.
 ********************************************************************** */
int do_list_cmd(int args, char *argv[])
{
    int ret = 0;

    if (args < 2) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    const char* filename = argv[1];
    struct pictdb_file myfile;

    if ((ret = do_open(filename, "rb", &myfile))) {
        return ret;
    }

    char* printer = do_list(&myfile, STDOUT);

    if (printer != NULL) {
        puts(printer);
        free(printer);
    }
    do_close(&myfile);

    return 0;
}

/********************************************************************//**
 * Prepares and calls do_create command.
********************************************************************** */
int do_create_cmd(int args, char *argv[])
{
    if (args < 2) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    uint32_t max_files = DEF_MAX_FILES;
    uint16_t thumb_res_X = DEF_THUMB_RES;
    uint16_t thumb_res_Y = DEF_THUMB_RES;
    uint16_t small_res_X = DEF_SMALL_RES;
    uint16_t small_res_Y = DEF_SMALL_RES;

    uint16_t value_16 = 0;
    uint32_t value_32 = 0;

    for (int i = 2; i < args; i++) {
        if (!strcmp(argv[i], "-max_files")) {
            if (args <= i + 1) {
                return ERR_NOT_ENOUGH_ARGUMENTS;
            }

            value_32 = atouint32(argv[i + 1]);

            if (value_32 <= 0 || value_32 > MAX_MAX_FILES) {
                return ERR_MAX_FILES;
            }

            max_files = value_32;
            i++;
        } else if (!strcmp(argv[i], "-thumb_res")) {
            if (args <= i + 2) {
                return ERR_NOT_ENOUGH_ARGUMENTS;
            }

            value_16 = atouint16(argv[i + 1]);

            if (value_16 <= 0 || value_16 > MAX_THUMB_RES) {
                return ERR_RESOLUTIONS;
            }

            thumb_res_X = value_16;
            value_16 = atouint16(argv[i + 2]);

            if (value_16 <= 0 || value_16 > MAX_THUMB_RES) {
                return ERR_RESOLUTIONS;
            }

            thumb_res_Y = value_16;
            i += 2;
        } else if (!strcmp(argv[i], "-small_res")) {
            if (args <= i + 2) {
                return ERR_NOT_ENOUGH_ARGUMENTS;
            }

            value_16 = atouint16(argv[i + 1]);

            if (value_16 <= 0 || value_16 > MAX_SMALL_RES) {
                return ERR_RESOLUTIONS;
            }

            small_res_X = value_16;
            value_16 = atouint16(argv[i + 2]);

            if (value_16 <= 0 || value_16 > MAX_SMALL_RES) {
                return ERR_RESOLUTIONS;
            }

            small_res_Y = value_16;
            i += 2;
        } else {
            return ERR_INVALID_ARGUMENT;
        }
    }

    puts("Create");

    struct pictdb_file database;
    const char* filename = argv[1];

    database.header.max_files = max_files;
    const uint16_t temp[sizeof database.header.res_resized] =
    {thumb_res_X, thumb_res_Y, small_res_X, small_res_Y};
    memcpy(database.header.res_resized, temp, sizeof temp);

    int ret = do_create(filename, &database);
    do_close(&database);
    return ret;
}

/********************************************************************//**
 * Reads a picture from the database and calls the do_read command
 */
int do_read_cmd(int args, char *argv[])
{
    if (args < 3) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    char* filename = argv[1];
    char* pict_id = argv[2];
    int code = RES_ORIG;
    char* tab;
    uint32_t size = 0;
    FILE* file = NULL;
    char* name = NULL;
    struct pictdb_file myfile;
    int ret = 0;
    size_t dummy = 1;

    if (args > 3 && (code = resolution_atoi(argv[3])) == -1) {
        return ERR_INVALID_ARGUMENT;
    }

    if ((ret = do_open(filename, "r+b", &myfile))) {
        return ret;
    }

    if((ret = do_read(pict_id, code, &tab, &size, &myfile))) {
        do_close(&myfile);
        return ret;
    }

    if ((ret = create_name(&name, pict_id, code))) {
        free(tab);
        do_close(&myfile);
        return ret;
    }

    if ((file = fopen(name, "wb")) == NULL) {
        free(name);
        free(tab);
        do_close(&myfile);
        return ERR_IO;
    }

    ret = write_disk_image(file, tab, size, &dummy);

    free(name);
    free(tab);
    fclose(file);
    do_close(&myfile);
    return ret;
}

/********************************************************************//**
 * Inserts a picture in the database and calls the do_insert command
 */
int do_insert_cmd(int args, char *argv[])
{
    if (args < 4) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    char* filename = argv[1];
    char* pict_id = argv[2];
    char* imagename = argv[3];
    char* tab = NULL;
    size_t size = 0;
    struct pictdb_file myfile;
    FILE* file = NULL;
    int ret = 0;

    if ((file = fopen(imagename, "rb")) == NULL) {
        return ERR_IO;
    }

    if ((ret = get_file_size(file, &size))) {
        fclose(file);
        return ret;
    }

    if ((tab = calloc(size, sizeof(char))) == NULL) {
        fclose(file);
        return ERR_OUT_OF_MEMORY;
    }

    if ((ret = read_disk_image(file, &tab, size, 0)) ||
        (ret = do_open(filename, "r+b", &myfile))) {
        free(tab);
        fclose(file);
        return ret;
    }

    ret = do_insert(tab, size, pict_id, &myfile);
    free(tab);
    fclose(file);
    do_close(&myfile);
    return ret;
}

/********************************************************************//**
 * Deletes a picture from the database and calls the do_delete command
 */
int do_delete_cmd(int args, char *argv[])
{
    if (args < 3) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    const char* filename = argv[1];
    const char* pictID = argv[2];
    int ret = 0;
    size_t len = strlen(pictID);
    struct pictdb_file myfile;

    if ((ret = do_open(filename, "rb+", &myfile))) {
        return ret;
    }

    if (len > MAX_PIC_ID || len == 0) {
        do_close(&myfile);
        return ERR_INVALID_PICID;
    }

    ret = do_delete(pictID, &myfile);
    do_close(&myfile);
    return ret;
}

/********************************************************************//**
 * Cleans the database and calls do_gbcollect command
 */
int do_gc_cmd(int args, char *argv[])
{
    if (args < 3) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    int ret = 0;
    const char* filename = argv[1];
    const char* tempname = argv[2];

    struct pictdb_file myfile;
    if ((ret = do_open(filename, "rb", &myfile))) {
        do_close(&myfile);
        return ret;
    }

    ret = do_gbcollect(&myfile, filename, tempname);
    do_close(&myfile);
    return ret;
}

/********************************************************************//**
 * MAIN
 */
int main(int argc, char* argv[])
{
    if (VIPS_INIT(argv[0])) {
        vips_error_exit("unable to start VIPS");
    }

    command_mapping helper =    {"help",    help};
    command_mapping list =      {"list",    do_list_cmd};
    command_mapping create =    {"create",  do_create_cmd};
    command_mapping read =      {"read",    do_read_cmd};
    command_mapping insert =    {"insert",  do_insert_cmd};
    command_mapping delete =    {"delete",  do_delete_cmd};
    command_mapping gc =        {"gc",      do_gc_cmd};

    command_mapping commands[] = {helper, list, create, read, insert, delete, gc};

    int ret = 0;
    argc--;
    argv++;

    if (argc < 1) {
        ret = ERR_NOT_ENOUGH_ARGUMENTS;
    } else {
        int not_In = 1;

        for (size_t i = 0; i < COMMAND_COUNT; i++) {
            if (!strcmp(commands[i].chaine, argv[0])) {
                ret = commands[i].comm(argc, argv);
                not_In = 0;
            }
        }

        if (not_In) {
            ret = ERR_INVALID_COMMAND;
        }
    }

    if (ret) {
        fprintf(stderr, "ERROR: %s\n", ERROR_MESSAGES[ret]);
        commands[0].comm(1, NULL);
    }

    vips_thread_shutdown(); //This sensibly reduces memory leaks
    vips_shutdown();
    return ret;
}
