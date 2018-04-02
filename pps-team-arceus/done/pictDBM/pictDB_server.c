/*
 * @file pictDB_server.c
 * @brief pictDB Server Manager.
 *
 * Picture Database Management Tool
 *
 * @date 22 May 2016
 */

#include "pictDB.h"
#include "libmongoose/mongoose.h"

#define POLL_DELTA_T 1000
#define MAX_QUERY_PARAM 5
#define URI_DELIM "&="

static const char* s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;
static struct pictdb_file myfile;
static int s_sig_received = 0;
static void signal_handler(int sig_num)
{
    signal(sig_num, signal_handler);
    s_sig_received = sig_num;
}

/**
 *  @brief  Parses a sequence of characters into a sequence of characters readable by the program
 *
 *  @param  result :        The tab of "strings" which would be interpreted further
 *  @param  tmp :    		Parts of the query string
 * 	@param 	src :			Query string
 * 	@param	delim :			Delimiters used to separate the strings
 * 	@param	len :			Length of the query string
 */
void split (char* result[], char* tmp, const char* src, const char* delim, size_t len)
{
    strncpy(tmp, src, len);
    tmp = strtok(tmp, delim);

    for (int i = 0; i < MAX_QUERY_PARAM; i++) {
        result[i] = tmp;
        tmp = strtok(NULL, delim);
    }
}

/**
 *  @brief  Sends an error message in case of failure
 *
 *  @param  nc :           	Message connection
 *  @param  error :    		Type of error
 */
void mg_error(struct mg_connection* nc, int error)
{
    mg_printf(nc, "HTTP/1.1 500\r\n"
              "ERROR: %s\n", ERROR_MESSAGES[error]);
    mg_send(nc, "", 0);
}

/**
 *  @brief  Prints the list of pictures contained in our database
 *
 *  @param  nc :           	Message connection
 *  @param  hm :    		Http message received
 */
void handle_list_call(struct mg_connection* nc, struct http_message* hm)
{
    char* printer = do_list(&myfile, JSON);

    if (printer != NULL) {
        mg_printf(nc, "HTTP/1.1 200 OK\r\n"
                  "Content-Type: application/json\r\n"
                  "Content-Length: %zu\r\n\r\n%s",
                  strlen(printer), printer);
        mg_send_http_chunk(nc, "", 0);
        free(printer);
    }
}

/**
 *  @brief  Reads an image in our database. Used to print the image on the screen and to quickly
 * 			compute the thumb image associated to the image.
 *
 *  @param  nc :           	Message connection
 *  @param  hm :    		Http message received
 */
void handle_read_call(struct mg_connection* nc, struct http_message* hm)
{
    size_t size = 0;
    size_t len = hm->query_string.len;
    char tmp[len + 1];
    char pict_id[MAX_PIC_ID + 1];
    char* tab = NULL;
    char* result[MAX_QUERY_PARAM];
    int code = RES_ORIG;
    int ret = 0;
    int pict_id_set = 0;
    int res_set = 0;

    tmp[len] = '\0';
    split(result, tmp, hm->query_string.p, URI_DELIM, len);

    for (int i = 0; i < MAX_QUERY_PARAM - 1 && result[i] != NULL; i++) {
        if (result[i + 1] == NULL) {
            mg_error(nc, ERR_NOT_ENOUGH_ARGUMENTS);
            return;
        } else if (!strcmp(result[i], "pict_id")) {
            if (pict_id_set) {
                mg_error(nc, ERR_INVALID_ARGUMENT);
                return;
            }

            strncpy(pict_id, result[i + 1], MAX_PIC_ID);
            pict_id[MAX_PIC_ID] = '\0';
            i++;
            pict_id_set = 1;
        } else if (!strcmp(result[i], "res")) {
            if (res_set) {
                mg_error(nc, ERR_INVALID_ARGUMENT);
                return;
            }

            code = resolution_atoi(result[i + 1]);
            i++;
            res_set = 1;
        }
    }

    if ((ret = do_read(pict_id, code, &tab, (uint32_t*) (&size), &myfile))) {
        mg_error(nc, ret);
        return;
    }

    mg_printf(nc, "HTTP/1.1 200 OK\r\n"
              "Content-Type: image/jpeg\r\n"
              "Content-Length: %zu\r\n\r\n",
              size);
    mg_send(nc, (void*) tab, size);

    free(tab);
}

/**
 *  @brief  Inserts an image in the database
 *
 *  @param  nc :           	Message connection
 *  @param  hm :    		Http message received
 */
void handle_insert_call(struct mg_connection* nc, struct http_message* hm)
{
    size_t image_size;
    char name[MAX_PIC_ID + 1];
    char dummy[MAX_PIC_ID + 1];
    int ret = 0;
    const char* image;

    mg_parse_multipart(hm->body.p, hm->body.len, dummy, MAX_PIC_ID, name, MAX_PIC_ID, &image, &image_size);
    name[MAX_PIC_ID] = '\0';

    if ((ret = do_insert((const char*) image, image_size, name, &myfile))) {
        mg_error(nc, ret);
        return;
    }

    mg_printf(nc, "HTTP/1.1 302 Found\r\n"
              "Location: http://localhost:%s/index.html\r\n\r\n",
              s_http_port);
    mg_send(nc, "", 0);
}

/**
 *  @brief  Deletes an image from the database
 *
 *  @param  nc :           	Message connection
 *  @param  hm :    		Http message received
 */
void handle_delete_call(struct mg_connection* nc, struct http_message* hm)
{
    size_t len = hm->query_string.len;
    char tmp[len + 1];
    char pict_id[MAX_PIC_ID + 1];
    char* result[MAX_QUERY_PARAM];
    int ret = 0;
    int pict_id_set = 0;

    tmp[len] = '\0';
    split(result, tmp, hm->query_string.p, URI_DELIM, len);

    for (int i = 0; i < MAX_QUERY_PARAM - 1 && result[i] != NULL; i++) {
        if (!strcmp(result[i], "pict_id")) {
            if (pict_id_set) {
                mg_error(nc, ERR_INVALID_ARGUMENT);
                return;
            }

            if (!strlen(result[i + 1])) {
                mg_error(nc, ERR_NOT_ENOUGH_ARGUMENTS);
                return;
            }

            strcpy(pict_id, result[i + 1]);
            pict_id[MAX_PIC_ID] = '\0';
            i++;
            pict_id_set = 1;
        }
    }

    if ((ret = do_delete(pict_id, &myfile))) {
        mg_error(nc, ret);
        return;
    }

    mg_printf(nc, "HTTP/1.1 302 Found\r\n"
              "Location: http://localhost:%s/index.html\r\n\r\n",
              s_http_port);
    mg_send(nc, "", 0);
}

/**
 *  @brief  Handles the last http message received : List, Read, Insert, Delete
 *
 *  @param  nc :           	Message connection
 *  @param  ev :    		Integer describing the event: In that case we want a Http message
 * 	@param	ev_data :		Used to transform it into the http message that we want to take care
 */
static void ev_handler(struct mg_connection* nc, int ev, void* ev_data)
{
    struct http_message* hm = (struct http_message*) ev_data;

    switch (ev) {
    case MG_EV_HTTP_REQUEST:
        if (mg_vcmp(&hm->uri, "/pictDB/list") == 0) {
            handle_list_call(nc, hm);
            nc->flags |= MG_F_SEND_AND_CLOSE;
        } else if (mg_vcmp(&hm->uri, "/pictDB/read") == 0) {
            handle_read_call(nc, hm);
            nc->flags |= MG_F_SEND_AND_CLOSE;
        } else if (mg_vcmp(&hm->uri, "/pictDB/insert") == 0) {
            handle_insert_call(nc, hm);
            nc->flags |= MG_F_SEND_AND_CLOSE;
        } else if (mg_vcmp(&hm->uri, "/pictDB/delete") == 0) {
            handle_delete_call(nc, hm);
            nc->flags |= MG_F_SEND_AND_CLOSE;
        } else {
            mg_serve_http(nc, hm, s_http_server_opts);
        }
        break;
    default:
        break;
    }
}

/********************************************************************//**
 * MAIN
 */
int main(int argc, char* argv[])
{
    if (VIPS_INIT(argv[0])) {
        vips_error_exit("unable to start VIPS");
    }

    struct mg_mgr mgr;
    struct mg_connection *nc;

    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    mg_mgr_init(&mgr, 0);

    nc = mg_bind(&mgr, s_http_port, ev_handler);

    if (nc == NULL) {
        printf("Unable to start listener at %s\n", s_http_port);
        return -1;
    }

    int ret = 0;
    argc--;
    argv++;

    const char* filename = argv[0];

    if (argc < 1) {
        ret = ERR_NOT_ENOUGH_ARGUMENTS;
    }

    if (!ret &&
        !(ret = do_open(filename, "r+b", &myfile))) {
        print_header(&(myfile.header));
        mg_set_protocol_http_websocket(nc);

        while (!s_sig_received) {
            mg_mgr_poll(&mgr, POLL_DELTA_T);
        }

        printf("Exiting on signal %d\n", s_sig_received);

        mg_mgr_free(&mgr);
        do_close(&myfile);
    } else {
        fprintf(stderr, "ERROR: %s\n", ERROR_MESSAGES[ret]);
    }

    vips_thread_shutdown();
    vips_shutdown();

    return 0;
}
