#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stddef.h>
#include <stdbool.h>
#include <limits.h>

#define NELEMS(x) (sizeof(x) / sizeof(*(x)))

const char USAGE[] = "USAGE: reader TTY";

void printBinary(char data, char * const out){
    for (size_t i = 0; i < CHAR_BIT; ++i){
        if(data & (1 << (CHAR_BIT - 1 - i)))
            out[i] = '1';
        else
            out[i] = '0';
    }
}

int main(int argc, char** argv){
    if (argc != 2){
        printf("%s\n", USAGE);
        return -1;
    }

    // must fsync after writes if opening in O_RDWR mode
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0){
        perror("Could not open TTY");
        return -2;
    }

    setbuf(stdout, NULL);

    char buf[256];
    char binary[] = "0b00000000";
    while (true) {
        ssize_t r = read(fd, buf, NELEMS(buf));
        if (r < 0) {
            perror("Error reading from device.");
            return 1;
        }
        for (size_t i = 0; i < (size_t) r; i++) {
            printBinary(buf[i], &binary[2]);
            printf("Read: %s\n", binary);
        }
    }

    return 0;
}
