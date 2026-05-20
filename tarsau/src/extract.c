#include "../include/sau.h"

/*
 * extract() — Bir .sau arşivini belirtilen dizine açar.
 *
 * Parametreler:
 *   archive  : açılacak .sau dosyasının adı
 *   dest_dir : dosyaların yazılacağı dizin (NULL ise mevcut dizin kullanılır)
 *
 * Dönüş: 0 başarı, 1 hata
 */
int extract(const char *archive, const char *dest_dir)
{
    /* ----------------------------------------------------------------
     * 1) Arşiv dosyasını aç
     * ---------------------------------------------------------------- */
    FILE *fp = fopen(archive, "rb");
    if (!fp) {
        fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
        return 1;
    }

    /* ----------------------------------------------------------------
     * 2) İlk 10 baytı oku → index bölümünün boyutunu öğren
     * ---------------------------------------------------------------- */
    char size_field[INDEX_SIZE_FIELD + 1];
    if (fread(size_field, 1, INDEX_SIZE_FIELD, fp) != INDEX_SIZE_FIELD) {
        fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
        fclose(fp);
        return 1;
    }
    size_field[INDEX_SIZE_FIELD] = '\0';

    int index_len = atoi(size_field);
    if (index_len <= 0) {
        fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
        fclose(fp);
        return 1;
    }

    /* ----------------------------------------------------------------
     * 3) Index bölümünü oku
     * ---------------------------------------------------------------- */
    char *index_buf = (char *)malloc(index_len + 1);
    if (!index_buf) {
        fprintf(stderr, "Bellek hatası!\n");
        fclose(fp);
        return 1;
    }

    if (fread(index_buf, 1, index_len, fp) != (size_t)index_len) {
        fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
        free(index_buf);
        fclose(fp);
        return 1;
    }
    index_buf[index_len] = '\0';

    /* ----------------------------------------------------------------
     * 4) Index bölümünü parse et
     *    Format: |dosyaadi,izinler,boyut|dosyaadi2,izinler2,boyut2|
     *
     *    Adım adım:
     *    - '|' ile tokenlara böl
     *    - Her token içindeki virgülleri ayır
     * ---------------------------------------------------------------- */
    FileInfo entries[MAX_FILES];
    int      file_count = 0;

    char *buf_copy = (char *)malloc(index_len + 1);  /* strtok orijinali bozar, kopyasını al */
    if (!buf_copy) {
        fprintf(stderr, "Bellek hatası!\n");
        free(index_buf);
        fclose(fp);
        return 1;
    }
    strcpy(buf_copy, index_buf);
    char *token    = strtok(buf_copy, "|");

    while (token != NULL && file_count < MAX_FILES) {
        /* Token boşsa (baştaki/sondaki | yüzünden) atla */
        if (strlen(token) == 0) {
            token = strtok(NULL, "|");
            continue;
        }

        /* Token: "dosyaadi,izinler,boyut" */
        char  fname[512];
        unsigned int perms_val;
        long  size_val;

        if (sscanf(token, "%511[^,],%o,%ld", fname, &perms_val, &size_val) != 3) {
            fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
            free(buf_copy);
            free(index_buf);
            fclose(fp);
            return 1;
        }

        strncpy(entries[file_count].name, fname, sizeof(entries[0].name) - 1);
        entries[file_count].name[sizeof(entries[0].name) - 1] = '\0';
        entries[file_count].perms = (mode_t)perms_val;
        entries[file_count].size  = size_val;
        file_count++;

        token = strtok(NULL, "|");
    }

    free(buf_copy);
    free(index_buf);

    /* ----------------------------------------------------------------
     * 5) Hedef dizini oluştur (gerekirse)
     * ---------------------------------------------------------------- */
    const char *outdir = (dest_dir && strlen(dest_dir) > 0) ? dest_dir : ".";
    make_dirs(outdir);  /* Dizin yoksa oluştur */

    /* ----------------------------------------------------------------
     * 6) Her dosyayı arşivden oku ve hedef dizine yaz
     *
     *    fp şu an veri bölümünün başında duruyor (index bittikten sonra).
     *    Her dosyanın boyutunu index'ten biliyoruz, o kadar bayt okuruz.
     * ---------------------------------------------------------------- */
    int i;
    for (i = 0; i < file_count; i++) {
        /* Tam yolu oluştur: outdir/dosyaadi */
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", outdir, entries[i].name);

        FILE *out = fopen(full_path, "wb");
        if (!out) {
            fprintf(stderr, "Hata: %s oluşturulamadı: %s\n", full_path, strerror(errno));
            fclose(fp);
            return 1;
        }

        /* Dosya içeriğini boyutu kadar kopyala */
        long  remaining = entries[i].size;
        char  copy_buf[4096];
        while (remaining > 0) {
            size_t to_read = (remaining < (long)sizeof(copy_buf))
                             ? (size_t)remaining
                             : sizeof(copy_buf);
            size_t n = fread(copy_buf, 1, to_read, fp);
            if (n == 0) {
                fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
                fclose(out);
                fclose(fp);
                return 1;
            }
            fwrite(copy_buf, 1, n, out);
            remaining -= (long)n;
        }

        fclose(out);

        /* 7) Orijinal izinleri geri yükle (chmod) */
        if (chmod(full_path, entries[i].perms) != 0) {
            /* İzin hatası ölümcül değil, sadece uyar */
            fprintf(stderr, "Uyarı: %s için izinler ayarlanamadı.\n", full_path);
        }
    }

    fclose(fp);

    printf("%s dizininde", outdir);
    for (i = 0; i < file_count; i++) {
        printf(" %s", entries[i].name);
        if (i < file_count - 1) printf(",");
    }
    printf(" dosyaları açıldı.\n");

    return 0;
}
