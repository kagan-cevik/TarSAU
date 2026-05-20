#include "../include/sau.h"

/*
 * bundle() — Verilen metin dosyalarını tek bir .sau arşivine birleştirir.
 *
 * Parametreler:
 *   file_count : kaç tane giriş dosyası var
 *   files[]    : giriş dosyalarının yol/ad listesi
 *   output     : oluşturulacak .sau dosyasının adı
 *
 * Dönüş: 0 başarı, 1 hata
 */
int bundle(int file_count, char *files[], const char *output)
{
    FileInfo info[MAX_FILES];
    int      i;
    long     total_size = 0;

    /* ----------------------------------------------------------------
     * 1) Dosya sayısı kontrolü
     * ---------------------------------------------------------------- */
    if (file_count > MAX_FILES) {
        fprintf(stderr, "Hata: En fazla %d dosya arşivlenebilir!\n", MAX_FILES);
        return 1;
    }

    /* ----------------------------------------------------------------
     * 2) Her dosyayı kontrol et: var mı? metin mi? boyutu ne?
     * ---------------------------------------------------------------- */
    for (i = 0; i < file_count; i++) {
        struct stat st;

        /* Dosya var mı? */
        if (stat(files[i], &st) != 0) {
            fprintf(stderr, "%s giriş dosyasının formatı uyumsuzdur!\n", files[i]);
            return 1;
        }

        /* Düzenli dosya mı? (dizin, sembolik link vs. değil) */
        if (!S_ISREG(st.st_mode)) {
            fprintf(stderr, "%s giriş dosyasının formatı uyumsuzdur!\n", files[i]);
            return 1;
        }

        /* Metin dosyası mı? */
        if (!is_text_file(files[i])) {
            fprintf(stderr, "%s giriş dosyasının formatı uyumsuzdur!\n", files[i]);
            return 1;
        }

        /* Metadata'yı kaydet */
        strncpy(info[i].name, files[i], sizeof(info[i].name) - 1);
        info[i].name[sizeof(info[i].name) - 1] = '\0';
        info[i].perms = st.st_mode & 0777;  /* sadece rwxrwxrwx bitleri */
        info[i].size  = (long)st.st_size;

        total_size += info[i].size;

        /* 200 MB kontrolü */
        if ((unsigned long long)total_size > MAX_TOTAL_BYTES) {
            fprintf(stderr, "Hata: Toplam dosya boyutu 200 MB'ı geçemez!\n");
            return 1;
        }
    }

    /* ----------------------------------------------------------------
     * 3) Index (organizasyon) bölümünü bellekte oluştur.
     *    Format: |dosyaadi,izinler,boyut|  (her kayıt | ile ayrılır)
     *
     *    Örnek:
     *    |t1,644,1024|t2,755,2048|
     * ---------------------------------------------------------------- */

    /* Önce index metnini dinamik bir tampona yazıyoruz. */
    char  index_buf[1024 * 1024];  /* 1 MB'lık tampon yeterli */
    int   index_len = 0;
    char  entry[600];

    index_buf[0] = '\0';

    for (i = 0; i < file_count; i++) {
        /* Sadece dosya adını al (yolu değil) */
        const char *basename = strrchr(info[i].name, '/');
        basename = basename ? basename + 1 : info[i].name;

        /* İzinleri oktal string'e çevir (örn. 0644 → "644") */
        snprintf(entry, sizeof(entry), "|%s,%o,%ld|",
                 basename, (unsigned int)info[i].perms, info[i].size);

        strncat(index_buf, entry, sizeof(index_buf) - index_len - 1);
        index_len += (int)strlen(entry);
    }

    /* ----------------------------------------------------------------
     * 4) Arşiv dosyasını aç ve yaz.
     *
     *    Dosya yapısı:
     *    [10 bayt boyut alanı][index metni][dosya1 içeriği][dosya2 içeriği]...
     * ---------------------------------------------------------------- */
    FILE *out = fopen(output, "wb");
    if (!out) {
        fprintf(stderr, "Hata: %s dosyası oluşturulamadı: %s\n", output, strerror(errno));
        return 1;
    }

    /* 4a) İlk 10 bayt: index bölümünün boyutunu ASCII olarak yaz */
    char size_field[INDEX_SIZE_FIELD + 1];
    snprintf(size_field, sizeof(size_field), "%010d", index_len);
    fwrite(size_field, 1, INDEX_SIZE_FIELD, out);

    /* 4b) Index metnini yaz */
    fwrite(index_buf, 1, index_len, out);

    /* 4c) Her dosyanın içeriğini sırayla yaz (ayırıcı yok) */
    for (i = 0; i < file_count; i++) {
        FILE *in = fopen(info[i].name, "rb");
        if (!in) {
            fprintf(stderr, "Hata: %s okunamadı: %s\n", info[i].name, strerror(errno));
            fclose(out);
            remove(output);   /* Yarım kalan arşivi sil */
            return 1;
        }

        /* 4 KB'lık tamponla kopyala */
        char   copy_buf[4096];
        size_t n;
        while ((n = fread(copy_buf, 1, sizeof(copy_buf), in)) > 0) {
            fwrite(copy_buf, 1, n, out);
        }

        fclose(in);
    }

    fclose(out);

    printf("Dosyalar birleştirildi.\n");
    return 0;
}
