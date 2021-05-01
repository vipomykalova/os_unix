#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 512  //размер буффера для считывания с файла
#define ERROR_ARG_MSG "Error: Enter output filename"
#define ERROR_FILE_MSG "Error: Can't create new file"

int main(int argc, char** argv) {
    
    if (argc == 1) {
        printf(ERROR_ARG_MSG);
        return 1;
    }
    
    char* file_name = argv[1]; //сохраняем имя файла в отдельную переменную
    
    int fd = open(file_name, O_WRONLY | O_CREAT, 0640); //получаем файловый дескриптор

    if (fd == -1) {
        printf(ERROR_FILE_MSG);
        return 1;
    }

    char write_buf[BUF_SIZE]; // буффер для записи
    char read_buf[BUF_SIZE]; // буффер для считывания
    
    int zero_bytes = 0; // количество нулевых байтов (или их смещение)
    int write_bytes = 0; // количесто ненулевых байтов, их записываем в новый файл

    int cur_readed, i; // текущее количество считанных байтов; счетчик

    while (1) {
        cur_readed = read(STDIN_FILENO, read_buf, BUF_SIZE); // считываем кусок данных из стандартного ввода
        
        if (!cur_readed) { // если данных нет, то мы всё обработали и можем выходить из программы
            break;
        }
        
        i = 0;
        
        while (i < cur_readed) {
            for (i; i < cur_readed && read_buf[i] != 0; i++) { // считываем ненулевые байты, переносим их в буффер для записи, увеличиваем счетчик 
                write_buf[write_bytes] = read_buf[i];
                write_bytes++;
            }
        
            if (write_bytes != 0) { // если уже считали ненулевые байты, то переписываем их в файл, обнуляем счетчик
                write(fd, write_buf, write_bytes);
                write_bytes = 0;
            }
        
            for (i; i < cur_readed && read_buf[i] == 0; i++) { // считываем нулевые байты, увеличиваем счетчик
                zero_bytes++;
            }
        
            if (zero_bytes != 0) { // если считали нулевые байты, создаем дыру в файле, обнуляем счетчик 
                lseek(fd, zero_bytes, SEEK_CUR);
                zero_bytes = 0;
            }
        }

    }
    
    return close(fd);
  
}
