#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define return_defer(value) do { result = (value); goto defer; } while(0)

static void usage(const char *program)
{
    fprintf(stderr, "usage: %s <src> -o <dest> \n", program);
}

static char *shift_args(int *argc, char ***argv)
{
    if(*argc < 1) return NULL;
    --(*argc);
    return *((*argv)++);
}

static enum Num_Mode {
    NUM_MODE_HEX,
    NUM_MODE_BIN,
} num_mode = NUM_MODE_HEX;

int main(int argc, char **argv)
{
    int result = 0;

    FILE *fp = NULL;
    char *buf = NULL;
    long int size = 0;

    const char *program = shift_args(&argc, &argv);

    const char *src_file = shift_args(&argc, &argv);
    if(src_file == NULL)
    {
        fprintf(stderr, "%s: no argument provided\n", program);
        usage(program);
        return_defer(-1);
    }

    char *oflag = shift_args(&argc, &argv);
    if(oflag == NULL || strncmp(oflag, "-o", 2) != 0)
    {
        fprintf(stderr, "%s: missing output flag `-o`\n", program);
        return_defer(-1);
    }

    const char *dst_file = shift_args(&argc, &argv);
    if(dst_file == NULL)
    {
        fprintf(stderr, "%s: missing argument to flag `-o`\n", program);
        return_defer(-1);
    }

    fp = fopen(src_file, "rb");
    if(fp == NULL)
    {
        fprintf(stderr, "failed to open file `%s`: %s\n", src_file, strerror(errno));
        return_defer(-1);
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);

    fseek(fp, 0, SEEK_SET);

    buf = malloc(size);
    if(buf == NULL)
    {
        fprintf(stderr, "failed to allocate %ld bytes: %s\n", size, strerror(errno));
        return_defer(-1);
    }

    fread(buf, 1, size, fp);
    fclose(fp);
    fp = NULL;

    fp = fopen(dst_file, "w");
    if(fp == NULL)
    {
        fprintf(stderr, "failed to create file `%s`: %s\n", dst_file, strerror(errno));
        return_defer(-1);
    }

    long int i = 0;
    while(i < size)
    {
        if(buf[i] == '#')
        {
            if(i + 1 >= size)
            {
                fprintf(stderr, "unexpected EOF after `#`\n");
                return_defer(-1);
            }

            ++i;

            if(buf[i] == 'x') num_mode = NUM_MODE_HEX;
            else if(buf[i] == 'b') num_mode = NUM_MODE_BIN;

            ++i;
            continue;
        }

        if(!isxdigit(buf[i]) || isspace(buf[i]))
        {
            ++i;
            continue;
        }
        else
        {
            uint8_t byte = 0;

            if(num_mode == NUM_MODE_HEX)
            {
                size_t b = 0;
                while(b < 2)
                {
                    if(i >= size)
                    {
                        fprintf(stderr, "unexpected EOF\n");
                        return_defer(-1);
                    }
                    if(!isxdigit(buf[i]))
                    {
                        if(isspace(buf[i]))
                        {
                            ++i;
                            continue;
                        }

                        fprintf(stderr, "expected hex data at %ld but got %02X instead\n", i, buf[i]);
                        return_defer(-1);
                    }

                    if(isdigit(buf[i]))
                    {
                        byte |= (buf[i] - '0') << (4 - ( b * 4 ) );
                        ++b;
                        ++i;
                        continue;
                    }
                    else if(isalpha(buf[i]))
                    {
                        uint8_t value = 0;
                        switch(buf[i])
                        {
                            case 'a': case 'A': value = 0xA; break;
                            case 'b': case 'B': value = 0xB; break;
                            case 'c': case 'C': value = 0xC; break;
                            case 'd': case 'D': value = 0xD; break;
                            case 'e': case 'E': value = 0xE; break;
                            case 'f': case 'F': value = 0xF; break;
                            default: value = 0;
                        }

                        byte |= value << (4 - ( b * 4 ) );
                        ++b;
                        ++i;
                        continue;

                    }

                }

                fwrite(&byte, 1, 1, fp);

            }

            else if(num_mode == NUM_MODE_BIN)
            {
                size_t b = 0;
                while(b < 8)
                {
                    if(i >= size)
                    {
                        fprintf(stderr, "unexpected EOF\n");
                        return_defer(-1);
                    }
                    if(buf[i] != '0' && buf[i] != '1')
                    {
                        if(isspace(buf[i]))
                        {
                            ++i;
                            continue;
                        }

                        fprintf(stderr, "expected binary data at %ld but got %02X instead\n", i, buf[i]);
                        return_defer(-1);
                    }

                    if(buf[i] == '0') ++b;
                    else if(buf[i] == '1') byte |= 1 << (7 - b++);
                    ++i;
                }

                fwrite(&byte, 1, 1, fp);
                ++i;
            }
        }
    }

defer:

    if(buf) free(buf);
    if(fp) fclose(fp);
    return result;
}
