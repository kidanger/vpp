#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

struct chunk {
    struct chunk* next;
    size_t size;
    size_t cursor;
    size_t totalsize;
    char data[0];
};

struct bigbuf {
    size_t chunksize;
    struct chunk* currentchunk;
    struct chunk* lastchunk;
    struct chunk* writing;
};

struct bigbuf bigbuf_init(size_t chunksize)
{
    struct bigbuf buf;
    buf.chunksize = chunksize;
    buf.currentchunk = NULL;
    buf.lastchunk = NULL;
    buf.writing = NULL;
    return buf;
}

size_t bigbuf_ask_write(struct bigbuf* buf, char** data)
{
    struct chunk* c;
    if (buf->writing) {
        c = buf->writing;
    } else {
        c = malloc(sizeof(struct chunk) + buf->chunksize);
        c->totalsize = buf->chunksize;
        c->next = 0;
        c->cursor = 0;
        c->size = 0;
        buf->writing = c;
    }

    *data = c->data;
    return c->totalsize;
}

void bigbuf_commit_write(struct bigbuf* buf, size_t size)
{
    struct chunk* c = buf->writing;
    c->size = size;
    buf->lastchunk = buf->lastchunk ? (buf->lastchunk->next = c) : c;
    if (!buf->currentchunk) buf->currentchunk = c;

    buf->chunksize = buf->chunksize == size ?
                      (buf->chunksize << 1) :
                      ((buf->chunksize >> 1) | (1<<8));
    buf->writing = NULL;
}

void bigbuf_write(struct bigbuf* buf, char* data, size_t size)
{
    while (size) {
        char* d;
        size_t s = bigbuf_ask_write(buf, &d);
        if (s > size) s = size;
        memcpy(d, data, s);
        data += s;
        size -= s;
        bigbuf_commit_write(buf, s);
    }
}

size_t bigbuf_ask_read(struct bigbuf* buf, char** data)
{
    struct chunk* c = buf->currentchunk;

    *data = c->data + c->cursor;
    return c->size;
}

void bigbuf_commit_read(struct bigbuf* buf, size_t size)
{
    struct chunk* c = buf->currentchunk;
    c->cursor += size;
    c->size -= size;

    if (c->size == 0) {
        struct chunk* next = c->next;
        free(c);
        buf->currentchunk = next;
        if (buf->lastchunk == c)
            buf->lastchunk = NULL;
    }
}

bool bigbuf_has_data(struct bigbuf* buf)
{
    return buf->currentchunk != NULL;
}

void bigbuf_free(struct bigbuf* buf)
{
    if (buf->writing)
        free(buf->writing);
    struct chunk* c = buf->currentchunk;
    while (c) {
        struct chunk* n = c->next;
        free(c);
        c = n;
    }
    buf->currentchunk = NULL;
    buf->lastchunk = NULL;
    buf->writing = NULL;
}

