#include "../include/sau.h"

/* ------------------------------------------------------------------ */
/*  Yardımcı: Dosyanın metin (ASCII) dosyası olup olmadığını kontrol  */
/*                                                                      */
/*  Strateji: Dosyayı ikili modda oku, her bayt 0-127 arasındaysa     */
/*  metin dosyasıdır. 0x00 (null byte) varsa binary sayarız.           */
/* ------------------------------------------------------------------ */
int is_text_file(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) return 0;

    unsigned char buf[4096];
    size_t n;

    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
        size_t i;
        for (i = 0; i < n; i++) {
            /* Null byte varsa kesinlikle binary dosyadır */
            if (buf[i] == 0x00) {
                fclose(fp);
                return 0;
            }
            /*
             * UTF-8 çok baytlı karakterlere izin ver:
             * 0x80-0xFF arası baytlar UTF-8'de Türkçe karakterler için
             * kullanılır (ş, ğ, ı, ç, ö, ü vb.). Bunları reddetme.
             */
        }
    }

    fclose(fp);
    return 1;  /* Tüm baytlar ASCII → metin dosyası */
}

/* ------------------------------------------------------------------ */
/*  Yardımcı: Verilen yolu (ve gerekirse üst dizinleri) oluştur.      */
/*  "mkdir -p" mantığını C ile uygular.                                */
/* ------------------------------------------------------------------ */
void make_dirs(const char *path)
{
    char tmp[1024];
    char *p;

    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    /* Her '/' karakterinde durarak dizini adım adım oluştur */
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);   /* Hata görmezden gel (zaten varsa sorun yok) */
            *p = '/';
        }
    }
    mkdir(tmp, 0755);  /* Son bileşeni de oluştur */
}

/* ------------------------------------------------------------------ */
/*  main() — Komut satırı argümanlarını işle, ilgili fonksiyonu çağır */
/*                                                                      */
/*  Kullanım:                                                           */
/*    tarsau -b dosya1 dosya2 ... [-o arşiv.sau]                       */
/*    tarsau -a arşiv.sau [hedef_dizin]                                */
/* ------------------------------------------------------------------ */
int main(int argc, char *argv[])
{
    /* En az 2 argüman gerekli: program_adi + parametre */
    if (argc < 2) {
        fprintf(stderr, "Kullanim:\n");
        fprintf(stderr, "  %s -b dosya1 dosya2 ... [-o arsiv.sau]\n", argv[0]);
        fprintf(stderr, "  %s -a arsiv.sau [hedef_dizin]\n", argv[0]);
        return 1;
    }

    /* ---------------------------------------------------------------
     * -b : Arşiv oluştur (bundle)
     * --------------------------------------------------------------- */
    if (strcmp(argv[1], "-b") == 0) {
        char *input_files[MAX_FILES];
        int   file_count = 0;
        const char *output = DEFAULT_ARCHIVE;

        /* argv[2]'den itibaren dosya adlarını topla.
           -o gelince dur, ondan sonraki argüman çıktı adı. */
        int i;
        for (i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-o") == 0) {
                /* -o'dan sonra arşiv adı gelmeli */
                if (i + 1 < argc) {
                    output = argv[i + 1];
                    i++;  /* -o'nun değerini de atla */
                } else {
                    fprintf(stderr, "Hata: -o parametresinden sonra arşiv adı belirtilmelidir.\n");
                    return 1;
                }
            } else {
                if (file_count >= MAX_FILES) {
                    fprintf(stderr, "Hata: En fazla %d dosya arşivlenebilir!\n", MAX_FILES);
                    return 1;
                }
                input_files[file_count++] = argv[i];
            }
        }

        if (file_count == 0) {
            fprintf(stderr, "Hata: En az bir giriş dosyası belirtmelisiniz.\n");
            return 1;
        }

        return bundle(file_count, input_files, output);
    }

    /* ---------------------------------------------------------------
     * -a : Arşivi aç (extract)
     * --------------------------------------------------------------- */
    else if (strcmp(argv[1], "-a") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Hata: -a parametresinden sonra arşiv dosyası adı belirtilmelidir.\n");
            return 1;
        }

        const char *archive  = argv[2];
        const char *dest_dir = (argc >= 4) ? argv[3] : ".";

        /* .sau uzantısı var mı? */
        const char *ext = strrchr(archive, '.');
        if (!ext || strcmp(ext, ".sau") != 0) {
            fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
            return 1;
        }

        return extract(archive, dest_dir);
    }

    /* ---------------------------------------------------------------
     * Bilinmeyen parametre
     * --------------------------------------------------------------- */
    else {
        fprintf(stderr, "Hata: Bilinmeyen parametre: %s\n", argv[1]);
        fprintf(stderr, "Kullanim:\n");
        fprintf(stderr, "  %s -b dosya1 dosya2 ... [-o arsiv.sau]\n", argv[0]);
        fprintf(stderr, "  %s -a arsiv.sau [hedef_dizin]\n", argv[0]);
        return 1;
    }
}
