#include "file_utils.h"

Err HexifyFile(const char *filename)
{
    Err err = RN_SUCCESS;

    FILE *src{};
    FILE *dst{};
    do {
        src = fopen(filename, "r");
        if (!src) {
            err = RN_BAD_FILE_READ; break;
        }

        char dstFilename[1024]{};
        snprintf(dstFilename, sizeof(dstFilename), "%s.hex", filename);
        dst = fopen(dstFilename, "w");
        if (!dst) {
            err = RN_BAD_FILE_WRITE; break;
        }

        uint8_t c = 0;
        int i = 1;
        while (fread(&c, 1, 1, src)) {
            fprintf(dst, "%02x", c);
            fputc(i % 32 ? ' ' : '\n', dst);
            i++;
        }
    } while (0);

    if (src) fclose(src);
    if (dst) fclose(dst);
    return err;
}

Err MakeBackup(const char *filename)
{
    Err err = RN_SUCCESS;

    FILE *src{};
    FILE *dst{};
    char dstFilename[1024]{};
    char dstFilenameTok[1024]{};
    do {
        src = fopen(filename, "r");
        if (!src) {
            err = RN_BAD_FILE_READ; break;
        }

        strcpy(dstFilenameTok, filename);
        const char *namePart = strtok(dstFilenameTok, ".");
        const char *fileExt = "bak"; //strtok(NULL, ".");

        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        snprintf(dstFilename, sizeof(dstFilename), "%s_%d%02d%02d_%02d%02d%02d.%s",
            namePart,
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
            fileExt
        );

        dst = fopen(dstFilename, "w");
        if (!dst) {
            err = RN_BAD_FILE_WRITE; break;
        }

        char buffer[4096];
        while (1) {
            size_t bytesRead = fread(buffer, 1, sizeof(buffer), src);
            if (!bytesRead) {
                break;
            }

            size_t bytesWritten = fwrite(buffer, 1, bytesRead, dst);
            if (bytesWritten != bytesRead) {
                err = RN_BAD_FILE_WRITE; break;
            }
        }
        if (err) break;  // in case i add more code below this..?
    } while (0);

    if (src) fclose(src);
    if (dst) fclose(dst);

#if 0
    // TODO(dlb)[cleanup]: This is because sha1sum lied to me and I had to diff the maps
    if (!err) {
        assert(dstFilename);
        HexifyFile(filename);
        HexifyFile(dstFilename);
    }
#endif
    return err;
}