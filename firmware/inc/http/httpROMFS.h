/* Processed by yaROMFS version 1.04 */
#ifndef YAROMFS_H
#define YAROMFS_H

#include <stdint.h>

#define YAROMFS_GET  1
#define YAROMFS_POST 2
#define YAROMFS_HANDLE_INVALID NULL

typedef struct {
    uint8_t * data;
    int32_t len;
    int32_t responseCode;
    uint8_t gz : 1;
    uint8_t preservePOST : 1;
    uint8_t sendFile : 1;
} _httpResponse;

typedef struct {
    const uint8_t * contentType;
    uint32_t hash;
    const uint8_t * data;
    uint32_t length;
    _httpResponse (*script_ptr)(uint8_t * data, uint32_t len, uint8_t * urlParams);
    uint8_t method;
    uint8_t gz : 1;
    uint8_t redirect : 1;
} _yaROMFSFILE;

typedef struct {
    const _yaROMFSFILE *file;
    uint32_t position;
    _httpResponse scriptResponse;
} YAROMFSFILE_HANDLE;

const _yaROMFSFILE * yaROMFSfind(uint8_t *url, uint8_t *method);
YAROMFSFILE_HANDLE * yaromfs_fopen(uint8_t *url, uint8_t *method, uint8_t *restData, uint32_t length);
uint32_t yaromfs_fread(YAROMFSFILE_HANDLE *handle, uint8_t *buf, uint32_t len);
uint32_t yaromfs_f_length(YAROMFSFILE_HANDLE *handle);
void yaromfs_fclose(YAROMFSFILE_HANDLE *handle);
const uint8_t * yaromfs_redirect(YAROMFSFILE_HANDLE *handle);
uint32_t yaromfs_is_gz(YAROMFSFILE_HANDLE *handle);
;const uint8_t * yaromfs_contentType(YAROMFSFILE_HANDLE *handle);
uint32_t yaromfs_postExists(uint8_t *url);
uint32_t yaromfs_responseCode(YAROMFSFILE_HANDLE *handle);
uint32_t yaromfs_preservePost(YAROMFSFILE_HANDLE *handle);

/* user defined scripts located in source */
extern _httpResponse checkbox_GET(uint8_t * data, uint32_t len, uint8_t * urlParams);
extern _httpResponse custom_GET_txt(uint8_t * data, uint32_t len, uint8_t * urlParams);
extern _httpResponse choose_file_GET_txt(uint8_t * data, uint32_t len, uint8_t * urlParams);
extern _httpResponse file_POST_php(uint8_t * data, uint32_t len, uint8_t * urlParams);
#define MAX_FILE_NAME_LENGTH (34)


#endif