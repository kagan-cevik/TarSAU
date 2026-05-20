#ifndef SAU_H
#define SAU_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>   /* stat(), chmod() için */
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

/* ------------------------------------------------------------------ */
/*  Sabitler                                                            */
/* ------------------------------------------------------------------ */
#define MAX_FILES        32          /* Arşive eklenebilecek max dosya */
#define MAX_TOTAL_BYTES  (200ULL * 1024 * 1024)  /* 200 MB            */
#define INDEX_SIZE_FIELD 10          /* İlk 10 bayt: index bölüm boyu  */
#define DEFAULT_ARCHIVE  "a.sau"     /* -o belirtilmezse kullanılır    */

/* ------------------------------------------------------------------ */
/*  Dosya metadata yapısı                                               */
/*  Her giriş dosyası için ad, izin ve boyut tutulur.                  */
/* ------------------------------------------------------------------ */
typedef struct {
    char   name[512];   /* Dosya yolu/adı           */
    mode_t perms;       /* stat()'tan gelen st_mode  */
    long   size;        /* Bayt cinsinden boyut      */
} FileInfo;

/* ------------------------------------------------------------------ */
/*  Fonksiyon prototipleri                                              */
/* ------------------------------------------------------------------ */

/* bundle.c */
int bundle(int file_count, char *files[], const char *output);

/* extract.c */
int extract(const char *archive, const char *dest_dir);

/* utils (main.c içinde tanımlı küçük yardımcılar) */
int  is_text_file(const char *path);
void make_dirs(const char *path);

#endif /* SAU_H */
