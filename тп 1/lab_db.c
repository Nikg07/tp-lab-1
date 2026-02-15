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



