#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 512
#define ERROR_ARG_MSG "Error: Enter output filename"
#define ERROR_FILE_MSG "Error: Can't create new file"

int main(int argc, char** argv) {
    
    if (argc == 1) {
        printf(ERROR_ARG_MSG);
        return 1;
    }
    
    char* file_name = argv[1];
    
    int fd = open(file_name, O_WRONLY | O_CREAT, 0640);

    if (fd == -1) {
        printf(ERROR_FILE_MSG);
        return 1;
    }

    char write_buf[BUF_SIZE];
    char read_buf[BUF_SIZE];
    
    int zero_bytes = 0;
    int write_bytes = 0;

    int cur_readed, i;

    while (1) {
        cur_readed = read(STDIN_FILENO, read_buf, BUF_SIZE);
        
        if (!cur_readed) {
            break;
        }
        
        i = 0;
        
        while (i < cur_readed) {
            for (i; i < cur_readed && read_buf[i] != 0; i++) {
                write_buf[write_bytes] = read_buf[i];
                write_bytes++;
            }
        
            if (write_bytes != 0) {
                write(fd, write_buf, write_bytes);
                write_bytes = 0;
            }
        
            for (i; i < cur_readed && read_buf[i] == 0; i++) {
                zero_bytes++;
            }
        
            if (zero_bytes != 0) {
                lseek(fd, zero_bytes, SEEK_CUR);
                zero_bytes = 0;
            }
        }

    }
    
    return close(fd);
  
}
