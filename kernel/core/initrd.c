#include <kernel/initrd.h>
#include <stdint.h>
#include <string.h>

#define BLOCK_SIZE 512

/* The ustar header; numbers are octal text. */
struct tar_header {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char checksum[8];
    char type;
    char link_name[100];
    char magic[6];
    char version[2];
    char user_name[32];
    char group_name[32];
    char device_major[8];
    char device_minor[8];
    char prefix[155];
    char padding[12];
};

_Static_assert(sizeof(struct tar_header) == BLOCK_SIZE, "a tar header is one block");

static const char *archive;
static size_t archive_size;

void initrd_init(const void *data, size_t size)
{
    archive = data;
    archive_size = size;
}

/* False for anything but octal digits, optionally ended by NUL or space. */
static bool parse_octal(const char *field, size_t length, uint64_t *value)
{
    *value = 0;
    for (size_t i = 0; i < length && field[i] != '\0' && field[i] != ' '; i++) {
        if (field[i] < '0' || field[i] > '7' || *value >> 61)
            return false;
        *value = *value * 8 + (uint64_t)(field[i] - '0');
    }
    return true;
}

/* Compares a field that is NUL-terminated only when shorter than length. */
static const char *match_field(const char *path, const char *field, size_t length)
{
    size_t i = 0;

    for (; i < length && field[i] != '\0'; i++)
        if (path[i] != field[i])
            return NULL;
    return path + i;
}

/* Long names are split into prefix "/" name. */
static bool matches(const struct tar_header *header, const char *path)
{
    if (header->prefix[0] != '\0') {
        path = match_field(path, header->prefix, sizeof(header->prefix));
        if (path == NULL || *path != '/')
            return false;
        path++;
    }
    path = match_field(path, header->name, sizeof(header->name));
    return path != NULL && *path == '\0';
}

/* The next regular file's header, moving offset past it; NULL at the end of
   the archive or where it is malformed. */
static const struct tar_header *next_file(size_t *offset, uint64_t *size)
{
    while (archive_size - *offset >= BLOCK_SIZE) {
        const struct tar_header *header = (const struct tar_header *)(archive + *offset);

        /* The archive ends with zero blocks. */
        if (header->name[0] == '\0')
            return NULL;
        if (!parse_octal(header->size, sizeof(header->size), size)
                || *size > archive_size - *offset - BLOCK_SIZE)
            return NULL;

        *offset += BLOCK_SIZE + (*size + BLOCK_SIZE - 1) / BLOCK_SIZE * BLOCK_SIZE;
        /* The last file's padding may be cut off. */
        if (*offset > archive_size)
            *offset = archive_size;

        if (header->type == '0' || header->type == '\0')
            return header;
    }
    return NULL;
}

bool initrd_find(const char *path, const void **data, size_t *size)
{
    size_t offset = 0;
    uint64_t file_size;

    for (const struct tar_header *header = next_file(&offset, &file_size); header != NULL;
            header = next_file(&offset, &file_size)) {
        if (matches(header, path)) {
            *data = (const char *)header + BLOCK_SIZE;
            *size = file_size;
            return true;
        }
    }
    return false;
}

/* Appends a field that is NUL-terminated only when shorter than length. */
static bool append_field(char *name, size_t size, size_t *used, const char *field, size_t length)
{
    for (size_t i = 0; i < length && field[i] != '\0'; i++) {
        if (*used + 1 >= size)
            return false;
        name[(*used)++] = field[i];
    }
    name[*used] = '\0';
    return true;
}

bool initrd_name(size_t index, char *name, size_t size)
{
    size_t offset = 0;
    uint64_t file_size;
    const struct tar_header *header;
    size_t used = 0;

    if (size == 0)
        return false;
    name[0] = '\0';

    do {
        header = next_file(&offset, &file_size);
        if (header == NULL)
            return false;
    } while (index-- > 0);

    if (header->prefix[0] != '\0'
            && (!append_field(name, size, &used, header->prefix, sizeof(header->prefix))
                || !append_field(name, size, &used, "/", 1)))
        return false;
    return append_field(name, size, &used, header->name, sizeof(header->name));
}
