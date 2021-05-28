#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <signal.h>

#define MAX_PROC 100
#define MAX_NAME_LEN 120
#define MAX_ARGS_LEN 120

int MAX_FILE_NAME_LEN = 200;

char name[MAX_PROC][MAX_NAME_LEN];
char *args[MAX_PROC][MAX_ARGS_LEN + 1];  // +1 для NULL
bool respawnable[MAX_PROC];

pid_t pids[MAX_PROC];
int pid_tries[MAX_PROC];

int proc_numbers;
int counter_procs = 0;

int arg_c;
char **arg_v;

// создаем название файла процесса ну или получаем его 
char *get_filename(int proc_num) {
    
    char *filename = malloc((size_t) MAX_FILE_NAME_LEN);
    sprintf(filename, "/tmp/%i%s", proc_num, name[proc_num]);
    
    int i;
    
    for (i = 5; i < strlen(filename); i++) {
        if (filename[i] == '/') {  //заменяем, например, bin/ls на bin_ls
            filename[i] = '_';
        }
    }
    
    return filename;
}

// создаем файл для процесса, записываем туда PID
void create_file(int proc) {
    
    syslog(LOG_NOTICE, "Create file for process %i...\n", proc); 
    char *filename = malloc((size_t) MAX_FILE_NAME_LEN);
    strcpy(filename, get_filename(proc));

    FILE *fp = fopen(filename, "wa");

    if (fp == NULL) {
        syslog(LOG_WARNING, "Error: can't open file: %s\n", filename);
        exit(1);
    }

    fprintf(fp, "%d", pids[proc]);
    fclose(fp);
}

// удаляем файл процесса по его завершении
void delete_file(int proc) {
    
    syslog(LOG_NOTICE, "Delete file for process %i...\n", proc); 
    char filename[MAX_FILE_NAME_LEN];
    strcpy(filename, get_filename(proc));
    
    if (remove(filename) == -1) {
        syslog(LOG_WARNING, "Error: can't delete %s\n", filename);
        exit(1);
    }
}

// создаем процесс для каждой программы, обрабатываем
void start_proc(int proc) {
    
    int cpid = fork();

    switch (cpid) {
        case -1:
            syslog(LOG_WARNING, "%s\n", "Error: can't fork");
            exit(1);
        case 0:
            syslog(LOG_NOTICE, "Exec program %s\n", name[proc]);
            execvp(name[proc], args[proc]); // мы в потомке, выполняемся и выходим

            if (errno != 0) {
                syslog(LOG_WARNING, "Error: exec program  %s\n", name[proc]);
                exit(1);
            }

            exit(0);
        default:
            pids[proc] = cpid;  // мы в родителе, сохраняем PID потомка
            create_file(proc); // и записываем его в файл
            counter_procs++;
    }
}

// запускаем процессы и ждем их выполнения
void run_programs() {

    for (int i = 0; i < proc_numbers; i++) {
        start_proc(i);
    }

    while (counter_procs) { // пока есть процессы, которых нужно дождаться
        int status;
        pid_t cpid = wait(&status);

        for (int i = 0; i < proc_numbers; i++) { // смотрим, какой именно процесс закончился
            if (pids[i] == cpid) {
                syslog(LOG_NOTICE, "Child %d with pid %d finished\n", i, cpid);
                delete_file(i); // удаляем файл процесса
                counter_procs--;
                
                if (status != 0 && pid_tries[i] < 50) { // если не получилось запустить процесс,
                    pid_tries[i]++; // повторяем снова не больше 50 раз
                    start_proc(i);
                } else if (status != 0 && pid_tries[i] == 50) { // после многих попыток процесс не удалось запустить
                    syslog(LOG_WARNING, "%s %s\n", "Error: Can't start program ", name[i]);
                }

                if (respawnable[i] && status == 0) { // если у программы опция respawn и 
                    start_proc(i); // запускаем бесконечное кол-во раз
                }
            }
        }
    }
}

// считываем конфиг. файл
void read_config() {
    
    if (arg_c != 2) {
        syslog(LOG_WARNING, "Error: Need two arguments\n");
        exit(1);
    }
    
    char *cfile = arg_v[1];
    
    FILE *fp = fopen(cfile, "r");

    if (fp == NULL) {
        syslog(LOG_WARNING, "Error: Can't open file\n");
        exit(1);
    }
    
    char *line = NULL;
    size_t len = 0;
    char *line_part = NULL;

    int counter = 0;
    int proc = 0;
    
    while ((getline(&line, &len, fp)) != -1) {
        while ((line_part = strsep(&line, " "))) {
            if (strcmp(line_part, "wait\n") == 0) { // последний аргумент
                args[proc][counter + 1] = NULL;
                respawnable[proc] = false;
            } else if (strcmp(line_part, "respawn\n") == 0){ // последний аргумент 
                args[proc][counter + 1] = NULL;
                respawnable[proc] = true;
            } else {
                if (counter == 0) { // имя программы, первый аргумент 
                    strcpy(name[proc], line_part);
                    args[proc][counter] = (char *) malloc(strlen(name[proc]));
                    strcpy(args[proc][counter], name[proc]);
                } else { // остальные аргументы
                    args[proc][counter] = (char *) malloc(MAX_ARGS_LEN);
                    strcpy(args[proc][counter], line_part);
                }
            }
            counter++;
        }
        counter = 0;
        proc++;
    }
    
    proc_numbers = proc;

    fclose(fp);
    
    run_programs();
}

// обработчик сигнала HUP
void sighup_handler() {
    for (int i = 0; i < proc_numbers; i++) {
        if (pids[i] != 0) {
            kill(pids[i], SIGKILL);
        }
    }
    
    syslog(LOG_NOTICE, "SIGHUP: Read config and restart programs...");
    read_config();
}

int main(int argc, char *argv[]) {

    arg_c = argc;
    arg_v = argv;
    
    signal(SIGHUP, (void (*)(int)) sighup_handler);

    int pid = fork();

    switch (pid) {
        case -1:
            syslog(LOG_WARNING, "%s\n", "Error: can't create demon");
            exit(1);
        case 0:
            setsid(); //закрываем терминал и создаем сессию в корневом каталоге
            chdir("/");
            close((int) stdin);
            close((int) stdout);
            close((int) stderr);
            read_config();
            exit(0);
        default:
            return 0;
    }
}
