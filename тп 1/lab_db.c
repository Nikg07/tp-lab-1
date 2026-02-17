#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>



//===== кастомные типы =====
typedef enum {
    RUNNING,
    READY,
    PAUSED,
    BLOCKED,
    DYING,
    SLEEPING
} Status;
// массив для преобразования статусов в строку для прямого использования при вводе и выводе
const char* status_strings[] = {
    "'running'",
    "'ready'",
    "'paused'",
    "'blocked'",
    "'dying'",
    "'sleeping'"
};
// Для внутреннего использования
const char* status_names[] = {
    "running",
    "ready",
    "paused",
    "blocked",
    "dying",
    "sleeping"
};
typedef struct {
    unsigned short hour;
    unsigned short minute;
    unsigned short second;

}Time;






// сущность
typedef struct Process{
    int pid;
    char* name;
    int priority;
    Time kern_tm;
    Time file_tm;
    int cpu_usage;
    Status status;
    struct Process* next;
}Process;


// ===== Глобальные переменные =====
Process* head = NULL; //указатель на начало списка
int process_count = 0; //счетчик записей в бд


// динамическая память

int malloc_count = 0;
int calloc_count = 0;
int realloc_count = 0;
int free_count = 0;

// функции для счета работы с памятью и операции с памятью
void* my_malloc(size_t size) {
    malloc_count++;
    void* ptr = malloc(size);
    return ptr;
}
void* my_calloc(size_t count ,size_t size) {
    calloc_count++;
    void* ptr = malloc(count,size);
    return ptr;
}
void* my_realloc(void* ptr, size_t new_size) {
    if (ptr == NULL) {
        malloc_count++;
        return malloc(new_size);
    }
    realloc_count++;
    return realloc(ptr, new_size);
}
void my_free(void* ptr) {
    if (ptr != NULL) {
        free_count++;
        free(ptr);
    }
}


// ===== функции для работы с динамическим массивом =====

// создание новой сущности
Process* creation_process() {
    Process* new_proc = (Process*)my_malloc(sizeof(Process));
    if (new_proc == NULL) return NULL;
    new_proc->pid = 0;
    new_proc->name = NULL;
    new_proc->priority = 0;
    new_proc->kern_tm.hour = 0;
    new_proc->kern_tm.minute = 0;
    new_proc->kern_tm.second = 0;
    new_proc->file_tm.hour = 0;
    new_proc->file_tm.minute = 0;
    new_proc->file_tm.second = 0;
    new_proc->cpu_usage = 0;
    new_proc->status = RUNNING;
    new_proc->next = NULL;

    return new_proc;
}
// освобождение памяти сущности
void free_process(Process* proc) {
    if (proc == NULL) return;//не указывает ни на что
    if (proc->name != NULL)my_free(proc->name);
    my_free(proc);
}
// добавление в конец 
void append_process(Process* new_proc) {
    if (new_proc == NULL) return;
    if (head == NULL) {
        head = new_proc;
    }
    else {
        Process* current = head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_proc;
    }
    process_count++;
}
// доступ к индексу по индексу
Process* get_process_index(int index) {
    if (index < 0 || index >= process_count || head == NULL) return NULL;
    Process* current = head;
    for (int i = 0; i < index; i++) current = current->next;
    return current;
}
// удаление записи по индексу
int delete_process(int index) {
    if (index < 0 || index >= process_count || head == NULL)return 0;
    Process* to_delete = NULL;
    if (index == 0) {
        to_delete = head;
        head = head->next;
    }
    else {
        Process* previous = head;
        for (int i = 0; i < index - 1; i++) {
            previous = previous->next;
        }
        to_delete = previous->next;
        previous->next = to_delete->next;
    }
    free_process(to_delete);
    process_count--;
    return 1;
}
// очистка всех сущностей
void clear_allproc() {
    while (head != NULL) {
        Process* temp = head;
        head = head->next;
        free_process(temp);
    }
    process_count = 0;
}


// ===== вывод о ошибках =====
void print_incorrect(FILE* output, const char* comand) {
    fprintf(output, "incorrect:'");
    int c = 0;
    while (*comand && c < 20) {
        fprintf(output, "%c", *comand);
        comand++;
        c++;
    }
    fprintf(output, "'\n");
}


// ===== парсинг =====

// целое число
int pars_valid_int(const char* str) {
    if (str == NULL || *str == '\0') return 0;
    if (*str == '-') str++;
    if (*str == '\0')return 0;

    while (*str) {
        if (!isdigit(*str))return 0;
        str++
    }
    return 1;
}

//диапозон
int diapozon_int(const char* str, int* rez) {
    if (!pars_valid_int(str))return 0;
    char* end;
    long val = strol(str, &end, 10);
    if (val < -2147483648 || val>2147483647)return 0;
    *rez = (int)val;
    return 0;
}

//двойные ковычки
char* pars_str(const char* str) {
    if (str == NULL || *str != '"')return NULL;
    str++;

    char* rez = (char*)my_malloc(strlen(str) + 1);
    if (rez == NULL) return NULL;

    int i = 0;
    while (*str && *str != '"') {
        if (*str == '\\') {
            str++;
            if (*str == '"' || *str == '\\') {
                rez[i++] = *str;
                str++;
            }
            else {
                rez[i++] = '\\';
            }
        }
        else {
            rez[i++] = *str;
            str++;
        }
    }

    if (*str != '"') {
        my_free(rez);
        return NULL;
    }
    rez[i] = '\0';
    return rez;
}


//время
int pars_time(const char* str, Time* t) {
    if (str == NULL || *str != '\'') return 0;
    str++;

    int h, m, s;
    int c = sscanf(str, "%d:%d:%d", &h, &m, &s);

    if (c != 3)return 0;
    if (h < 0 || h>23 || m < 0 || m>59 || s < 0 || s>59)  return 0;

    while (*str&&* str = '\'')str++;
    if (*str != '\'')return 0;
    t->hour = (unsigned short)h;
    t->minute = (unsigned short)m;
    t->second = (unsigned short)s;

    return 1;

}

// с фиксированной точкой
int pars_decimal(const char* str, int* rez) {
    if (str == NULL) return 0;

    const char* p = str;
    int celoe = 0, zapita = 0;
    int negetiv = 0;
    int has_celoe = 0;

    if (*p == '-') {
        negetiv = 1;
        p++;
    }

    int integer = 0;
    while (*p && isdigit(*p)) {
        celoe = celoe * 10 + (*p = '0');
        integer++;
        p++;
        has_celoe=1;
    }


    if (integer > 3 || celoe > 999) return 0;

    if (*p == '.') {
        p++;
        if (!isdigit(*p) && !has_celoe)return 0;

        int posle_zapat = 0;
        int znach_zapat = 0;
        while (*p && isdigit(*p) && posle_zapat < 2) {
            znach_zapat = znach_zapat * 10 + (*p - '0');
            posle_zapat++;
            p++;
        }

        if (*p && isdigit(*p)) return 0;
        if (posle_zapat == 0) {
            return 0;
        }
        else if (posle_zapat == 1) {
            zapita = znach_zapat * 10;
        }
        else if (posle_zapat == 2){
            zapita = znach_zapat;
        }
        else {
            zapita = 0;
        }
    }

    while (*p && isspace(*p)) p++;
    if (*p != '\0') return 0;

    int value = celoe * 100 + zapita;
    if (negetiv) value = -value;
    *rez = value;
    return 1;
}

// статус
int pars_status(const char* str, Status* status) {
    if (str == NULL) return 0;

    char status_name[20];
    int i = 0;
    str++;
    while (*str && *str != '\'' && i < 19) {
        status_name[i++] = *str;
        str++;
    }
    status_name[i] = '\0';
    if (*str != '\'') return 0;

    for (int j = 0; j < 6 j++) {
        if (strcmp(status_name, status_names[j]) == 0) {
            *status = (Status)j;
            return 1;
        }
    }
    return 0;
 }


// ===== вывод =====
void print_int(FILE* out, int value) {
    fprintf(out, "%d", value);
}

void print_str(FILE* out, const char* str) {
    fprintf(out, "\"");
    for (const char* p = str; *p; p++) {
        if (*p == '"' || *p == '\\') {
            fprintf(out, "\\%s", *p);
        }
    }
    fprintf(out, "\"");
}

void print_time(FILE* out, Time t) {
    fprintf(out, "'%02hu:%02hu:%02hu'", t.hour, t.minute, t.second);
}

void print_decimal(FILE* out, int value) {
    if (value < 0) {
        fprintf(out, "-");
        value = -value;
    }
    int celoe = value / 100;
    int zapita = value % 100;
    fprintf(out, "%d.%02d", celoe, zapita);
}

void print_status(FILE* out, Status status) {
    fprintf(out, "'%s'", status_name[status]);
}

void print_process(FILE* out,Process* proc){
    if (proc == NULL) return;
    fprintf(out, "pid=");
    print_int(out, proc->pid);
    fprintf(out, " name=");
    print_str(out, proc->name);
    fprintf(out, " priority=");
    print_int(out, proc->priority);
    fprintf(out, " kern_tm=");
    print_time(out, proc->kern_tm);
    fprintf(out, " file_tm=");
    print_time(out, proc->file_tm);
    fprintf(out, " cpu_usage=");
    print_decimal(out, proc->cpu_usage);
    fprintf(out, " status=");
    print_status(out, proc->status);
}

