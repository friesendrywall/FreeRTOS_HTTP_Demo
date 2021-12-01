/* Processed by yaROMFS version 1.04 */
/* yaRomfsCore.c must be included  */
#include <stdint.h>
#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include "http/httpROMFS.h"

#include "yaROMFSconfig.h"

extern const uint8_t yaROMFSDAT[];
static const _yaROMFSFILE * lookup(uint8_t * url, uint8_t * method);

const _yaROMFSFILE * yaROMFSfind(uint8_t *url, uint8_t *method) {
    return lookup(url, method);
}

static uint32_t hash(unsigned char *str, unsigned char *method) {
    uint32_t hash = 5381;
    int c;

    while (c = *str++) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    while (c = *method++) {
        c = toupper(c);
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

static const _yaROMFSFILE fileList[] = {
    { .hash = 0xDD4D8BB1, .contentType = "json", .length = 0, .script_ptr = checkbox_GET, .method = YAROMFS_GET }, /* GET /checkbox.php */
    { .hash = 0xBE4D35C6, .contentType = "txt", .length = 0, .script_ptr = custom_GET_txt, .method = YAROMFS_GET }, /* GET /api/custom.txt */
    { .hash = 0xDED470B4, .contentType = "txt", .length = 0, .script_ptr = choose_file_GET_txt, .method = YAROMFS_GET }, /* GET /api/selected.txt */
    { .hash = 0xCDC4EE9E, .contentType = "json", .length = 8192, .script_ptr = file_POST_php, .method = YAROMFS_POST }, /* POST /api/post_file.php */
    { .hash = 0x9E78D2AF, .contentType = "html", .data = &yaROMFSDAT[0x0], .length = 0x26D, .gz = 1 },/* /index.html */
    { .hash = 0x52526ABE, .contentType = "txt", .data = &yaROMFSDAT[0x26E], .length = 0x12, .gz = 0 },/* /choice1.txt */
    { .hash = 0x3E943F9F, .contentType = "txt", .data = &yaROMFSDAT[0x280], .length = 0x12, .gz = 0 },/* /choice2.txt */
    { .hash = 0x1C948E0E, .contentType = "txt", .data = &yaROMFSDAT[0x294], .length = 0x1C, .gz = 1 },/* /zipped.txt */
    { .hash = 0xB127C374, .redirect = 1, .contentType = "/" }, /* Redirect /test -> /*/
};


static const _yaROMFSFILE * lookup(uint8_t * url, uint8_t * method){
	switch (hash(url, method)){
    case 0xDD4D8BB1:
        return &fileList[0];
    case 0xBE4D35C6:
        return &fileList[1];
    case 0xDED470B4:
        return &fileList[2];
    case 0xCDC4EE9E:
        return &fileList[3];
    case 0x9E78D2AF:
        return &fileList[4];
    case 0x52526ABE:
        return &fileList[5];
    case 0x3E943F9F:
        return &fileList[6];
    case 0x1C948E0E:
        return &fileList[7];
    case 0xB127C374:
        return &fileList[8];
    default:
        return NULL;
	}
}

