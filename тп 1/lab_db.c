#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ===== КАСТОМНЫЕ ТИПЫ =====

// Перечисление для статуса процесса (enum)
typedef enum {
    RUNNING,
    READY,
    PAUSED,
    BLOCKED,
    DYING,
    SLEEPING
} Status;

// Массив для преобразования статуса в строку с кавычками (для ввода/вывода)
const char* status_strings[] = {
    "'running'",
    "'ready'",
    "'paused'",
    "'blocked'",
    "'dying'",
    "'sleeping'"
};

// Массив для внутреннего использования (без кавычек)
const char* status_names[] = {
    "running",
    "ready",
    "paused",
    "blocked",
    "dying",
    "sleeping"
};

// Структура для хранения времени (time)
typedef struct {
    unsigned short hour;    // часы 0-23
    unsigned short minute;  // минуты 0-59
    unsigned short second;  // секунды 0-59
} Time;

// Сущность - запись о процессе
typedef struct Process {
    int pid;                 // PID процесса (int)
    char* name;              // командная строка (string) - динамическая память
    int priority;            // приоритет (int)
    Time kern_tm;            // время в kernelmode (time)
    Time file_tm;            // время работы с файлом (time)
    int cpu_usage;           // загрузка CPU (decimal(3,2)) - хранение в сотых долях
    Status status;           // статус процесса (enum)
    struct Process* next;    // указатель на следующий элемент списка
} Process;

// ===== ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ =====

Process* head = NULL;        // указатель на начало списка
int process_count = 0;       // количество записей в БД

// ===== СЧЕТЧИКИ ДЛЯ MEMSTAT.TXT =====

int malloc_count = 0;
int calloc_count = 0;
int realloc_count = 0;
int free_count = 0;

// ===== ФУНКЦИИ ДЛЯ ПОДСЧЕТА ВЫЗОВОВ ПАМЯТИ =====

void* my_malloc(size_t size) {
    malloc_count++;
    void* ptr = malloc(size);
    return ptr;
}

void* my_calloc(size_t count, size_t size) {
    calloc_count++;
    void* ptr = calloc(count, size);
    return ptr;
}

void* my_realloc(void* ptr, size_t new_size) {
    if (ptr == NULL) {
        malloc_count++;  // realloc с NULL считается как malloc
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

// ===== ФУНКЦИИ ДЛЯ РАБОТЫ СО СПИСКОМ =====

// Создание новой сущности
Process* creation_process() {
    Process* new_proc = (Process*)my_malloc(sizeof(Process));
    if (new_proc == NULL) return NULL;

    // Инициализация всех полей значениями по умолчанию
    new_proc->pid = 0;
    new_proc->name = NULL;           // память под строку будет выделена позже
    new_proc->priority = 0;
    new_proc->kern_tm.hour = 0;
    new_proc->kern_tm.minute = 0;
    new_proc->kern_tm.second = 0;
    new_proc->file_tm.hour = 0;
    new_proc->file_tm.minute = 0;
    new_proc->file_tm.second = 0;
    new_proc->cpu_usage = 0;
    new_proc->status = RUNNING;      // статус по умолчанию
    new_proc->next = NULL;            // следующего элемента пока нет

    return new_proc;
}

// Освобождение памяти сущности
void free_process(Process* proc) {
    if (proc == NULL) return;

    // Сначала освобождение строки name (если была выделена)
    if (proc->name != NULL) {
        my_free(proc->name);
        proc->name = NULL;
    }

    // Затем освобождение самой структуры
    my_free(proc);
}

// Добавление записи в конец списка (всегда в конец!)
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

// Доступ к записи по индексу
Process* get_process_index(int index) {
    if (index < 0 || index >= process_count || head == NULL) return NULL;

    Process* current = head;
    for (int i = 0; i < index; i++) {
        current = current->next;
    }
    return current;
}

// Удаление записи по индексу
int delete_process(int index) {
    if (index < 0 || index >= process_count || head == NULL) return 0;

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

// Очистка всей базы данных
void clear_allproc() {
    while (head != NULL) {
        Process* temp = head;
        head = head->next;
        free_process(temp);
    }
    process_count = 0;
}

// ===== ФУНКЦИЯ ДЛЯ ВЫВОДА ОШИБОК =====
// Формат: incorrect:'первые 20 символов команды'
void print_incorrect(FILE* output, const char* command) {
    fprintf(output, "incorrect:'");
    int count = 0;
    while (*command && count < 20) {
        // Вывод символа как есть (экранирование не требуется по заданию)
        fprintf(output, "%c", *command);
        command++;
        count++;
    }
    fprintf(output, "'\n");
}

// ===== ФУНКЦИИ ПАРСИНГА =====

// Проверка, является ли строка целым числом (для int)
int pars_valid_int(const char* str) {
    if (str == NULL || *str == '\0') return 0;

    // Пропуск возможного знака минуса
    if (*str == '-') str++;
    if (*str == '\0') return 0;  // только минус - не число

    // Проверка, что все символы - цифры
    while (*str) {
        if (!isdigit(*str)) return 0;
        str++;
    }
    return 1;
}

// Парсинг целого числа с проверкой диапазона (для int)
int diapozon_int(const char* str, int* rez) {
    if (!pars_valid_int(str)) return 0;

    char* end;
    long val = strtol(str, &end, 10);

    // Проверка диапазона для int
    if (val < -2147483648 || val > 2147483647) return 0;

    *rez = (int)val;
    return 1;  // все нормально, комфортабеле
}

// Парсинг строки в двойных кавычках с обработкой экранирования (для string)
char* pars_str(const char* str) {
    if (str == NULL || *str != '"') return NULL;

    str++;  // пропуск открывающей кавычки

    // Выделение памяти под результат
    char* rez = (char*)my_malloc(strlen(str) + 1);
    if (rez == NULL) return NULL;

    int i = 0;
    while (*str && *str != '"') {
        if (*str == '\\') {
            // Обработка экранирования
            str++;
            if (*str == '"' || *str == '\\') {
                rez[i++] = *str;
                str++;
            }
            else {
                // Неизвестный эскэйп - просто слэш 
                rez[i++] = '\\';
            }
        }
        else {
            rez[i++] = *str;
            str++;
        }
    }

    // Проверка наличие закрывающей кавычки
    if (*str != '"') {
        my_free(rez);
        return NULL;
    }

    rez[i] = '\0';
    return rez;
}

// Парсинг времени (формат: 'часы:минуты:секунды')
int pars_time(const char* str, Time* t) {
    if (str == NULL || *str != '\'') return 0;

    str++;  // пропуск открывающей кавычки

    int h, m, s;
    int c = sscanf(str, "%d:%d:%d", &h, &m, &s);

    if (c != 3) return 0;

    // Проверка корректности значений
    if (h < 0 || h > 23 || m < 0 || m > 59 || s < 0 || s > 59) return 0;

    // поиск закрывающей кавычки
    const char* p = str;
    while (*p && *p != '\'') p++;
    if (*p != '\'') return 0;

    t->hour = (unsigned short)h;
    t->minute = (unsigned short)m;
    t->second = (unsigned short)s;

    return 1;
}

// Парсинг decimal(3,2) - хранение в сотых 5.00 -> 500 чтоб было комфортабеле
int pars_decimal(const char* str, int* rez) {
    if (str == NULL || rez == NULL) return 0;

    const char* p = str;
    int celoe = 0;
    int zapita = 0;
    int negetiv = 0;
    int celoe_digits = 0;
    int has_celoe = 0;

    // Обработка знака
    if (*p == '-') {
        negetiv = 1;
        p++;
    }

    // Чтение целой части
    while (*p && isdigit(*p)) {
        celoe = celoe * 10 + (*p - '0');
        celoe_digits++;
        p++;
        has_celoe = 1;
    }

    // Проверка целой части (макс 3 цифры, не больше 999, а то бан)
    if (celoe_digits > 3 || celoe > 999) return 0;

    // Если есть десятичная точка
    if (*p == '.') {
        p++;

        // Проверка, что после точки есть цифры
        if (!isdigit(*p) && !has_celoe) return 0;

        int posle_zapat = 0;
        int znach_zapat = 0;

        // Чтение дробной части (макс 2 цифры)
        while (*p && isdigit(*p) && posle_zapat < 2) {
            znach_zapat = znach_zapat * 10 + (*p - '0');
            posle_zapat++;
            p++;
        }

        // Если после дробной части есть еще цифры - 
        if (*p && isdigit(*p)) return 0;

        // Преобразование дробной части в сотые доли
        if (posle_zapat == 1) {
            zapita = znach_zapat * 10;
        }
        else if (posle_zapat == 2) {
            zapita = znach_zapat;
        }
        else {
            zapita = 0;
        }
    }

    // Проверка на лишние символы в конце (пропускаем пробелы)
    while (*p && isspace(*p)) p++;
    if (*p != '\0') return 0;

    // Формирование результата в сотых долях
    int value = celoe * 100 + zapita;
    if (negetiv) value = -value;

    *rez = value;
    return 1;
}

// Парсинг статуса из строки с кавычками (для enum)
int pars_status(const char* str, Status* status) {
    if (str == NULL || *str != '\'') return 0;

    str++;  // пропуск открывающей кавычки

    char status_name[20];
    int i = 0;

    // имя статуса до закрывающей кавычки
    while (*str && *str != '\'' && i < 19) {
        status_name[i++] = *str;
        str++;
    }
    status_name[i] = '\0';

    // наличие закрывающей кавычки иначе бан
    if (*str != '\'') return 0;

    // Сравнение с известными статусами
    for (int j = 0; j < 6; j++) {
        if (strcmp(status_name, status_names[j]) == 0) {
            *status = (Status)j;
            return 1;
        }
    }

    return 0;  // статус не найден,бан,админ топ
}

// ===== ФУНКЦИИ ВЫВОДА =====

// Вывод целого числа (без лидирующих нулей)
void print_int(FILE* out, int value) {
    fprintf(out, "%d", value);
}

// Вывод строки с экранированием (в двойных кавычках)
void print_str(FILE* out, const char* str) {
    fprintf(out, "\"");
    for (const char* p = str; *p; p++) {
        if (*p == '"' || *p == '\\') {
            fprintf(out, "\\%c", *p);  // экранирование
        }
        else {
            fprintf(out, "%c", *p);
        }
    }
    fprintf(out, "\"");
}

// Вывод времени с лидирующими нулями (в одинарных кавычках)
void print_time(FILE* out, Time t) {
    fprintf(out, "'%02hu:%02hu:%02hu'", t.hour, t.minute, t.second);
}

// Вывод decimal(3,2) - всегда 2 знака после запятой
void print_decimal(FILE* out, int value) {
    if (value < 0) {
        fprintf(out, "-");
        value = -value;
    }
    int celoe = value / 100;
    int zapita = value % 100;
    fprintf(out, "%d.%02d", celoe, zapita);
}

// Вывод статуса (в одинарных кавычках)
void print_status(FILE* out, Status status) {
    fprintf(out, "'%s'", status_names[status]);
}

// Вывод полной записи процесса
void print_process(FILE* out, Process* proc) {
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

// ===== ФУНКЦИИ СРАВНЕНИЯ =====
// Все возвращают: -1 если a < b, 0 если a == b, 1 если a > b

// Сравнение двух целых чисел
int compare_int(int a, int b) {
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

// Сравнение двух строк
int compare_str(const char* a, const char* b) {
    if (a == NULL && b == NULL) return 0;
    if (a == NULL) return -1;
    if (b == NULL) return 1;
    return strcmp(a, b);
}

// Сравнение двух временных меток
int compare_time(Time a, Time b) {
    if (a.hour < b.hour) return -1;
    if (a.hour > b.hour) return 1;

    if (a.minute < b.minute) return -1;
    if (a.minute > b.minute) return 1;

    if (a.second < b.second) return -1;
    if (a.second > b.second) return 1;

    return 0;
}

// Сравнение двух decimal (в сотых долях)
int compare_decimal(int a, int b) {
    return compare_int(a, b);
}

// Сравнение двух статусов (для внутреннего использования, например, сортировки)
int compare_status(Status a, Status b) {
    return compare_int(a, b);
}

// ===== ФУНКЦИИ ДЛЯ РАБОТЫ СО СПИСКАМИ ЗНАЧЕНИЙ =====

// Проверка, находится ли значение в списке вида ['a','b','c']
// Используется для операторов in/not_in
int is_value_in_list(const char* list_str, const char* value) {
    if (list_str == NULL || *list_str != '[') return 0;

    const char* p = list_str;
    p++;  // пропуск '['

    // Пропуск пробелов
    while (*p == ' ') p++;

    // Пустой список
    if (*p == ']') return 0;

    while (*p && *p != ']') {
        // Пропуск пробелов
        while (*p == ' ') p++;

        // Должна быть кавычка
        if (*p != '\'') {
            p++;
            continue;
        }
        p++;  // пропускаем кавычку

        //значение на базу
        char item[20];
        int i = 0;
        while (*p && *p != '\'' && i < 19) {
            item[i++] = *p;
            p++;
        }
        item[i] = '\0';

        // Проверка на закрывающую кавычку
        if (*p != '\'') break;
        p++;  // пропуск кавычки

        // Сравнение с искомым значением
        if (strcmp(value, item) == 0) {
            return 1;  // нашли
        }

        // Пропуск запятой и пробелов
        while (*p == ' ' || *p == ',') p++;
    }

    return 0;  // не входит
}

// ===== СТРУКТУРА ДЛЯ ХРАНЕНИЯ УСЛОВИЯ =====
typedef struct Condition {
    char field_name[50];     // имя поля (pid, name, priority, kern_tm, file_tm, cpu_usage, status)
    char operator[10];       // оператор (=, !=, <, >, <=, >=, in, not_in)
    char value_str[256];     // строковое представление значения
} Condition;

// ===== ПРОВЕРКА УСЛОВИЙ =====

// Проверка одного условия для конкретной записи
// Возвращает 1 если условие выполняется, 0 если нет
int check_condition(Process* proc, Condition* cond) {
    if (proc == NULL || cond == NULL) return 0;

    // Проверка существования поля
    if (strcmp(cond->field_name, "pid") != 0 &&
        strcmp(cond->field_name, "name") != 0 &&
        strcmp(cond->field_name, "priority") != 0 &&
        strcmp(cond->field_name, "kern_tm") != 0 &&
        strcmp(cond->field_name, "file_tm") != 0 &&
        strcmp(cond->field_name, "cpu_usage") != 0 &&
        strcmp(cond->field_name, "status") != 0) {
        return 0;  // неизвестное поле
    }

    // ===== PID (int) =====
    // Разрешены операторы: =, !=, <, >, <=, >=
    if (strcmp(cond->field_name, "pid") == 0) {
        int val;
        if (!diapozon_int(cond->value_str, &val)) return 0;

        int cmp = compare_int(proc->pid, val);

        if (strcmp(cond->operator, "=") == 0) return cmp == 0;
        if (strcmp(cond->operator, "!=") == 0) return cmp != 0;
        if (strcmp(cond->operator, "<") == 0) return cmp < 0;
        if (strcmp(cond->operator, ">") == 0) return cmp > 0;
        if (strcmp(cond->operator, "<=") == 0) return cmp <= 0;
        if (strcmp(cond->operator, ">=") == 0) return cmp >= 0;

        return 0;  // неизвестный оператор
    }

    // ===== NAME (string) =====
    // Разрешены операторы: =, !=, <, >, <=, >= (алфавитное сравнение)
    else if (strcmp(cond->field_name, "name") == 0) {
        char* val = pars_str(cond->value_str);
        if (val == NULL) return 0;

        // Проверка, что имя в записи существует
        if (proc->name == NULL) {
            my_free(val);
            return 0;
        }

        int cmp = compare_str(proc->name, val);
        my_free(val);

        if (strcmp(cond->operator, "=") == 0) return cmp == 0;
        if (strcmp(cond->operator, "!=") == 0) return cmp != 0;
        if (strcmp(cond->operator, "<") == 0) return cmp < 0;
        if (strcmp(cond->operator, ">") == 0) return cmp > 0;
        if (strcmp(cond->operator, "<=") == 0) return cmp <= 0;
        if (strcmp(cond->operator, ">=") == 0) return cmp >= 0;

        return 0;
    }

    // ===== PRIORITY (int) =====
    // Разрешены операторы: =, !=, <, >, <=, >=
    else if (strcmp(cond->field_name, "priority") == 0) {
        int val;
        if (!diapozon_int(cond->value_str, &val)) return 0;

        int cmp = compare_int(proc->priority, val);

        if (strcmp(cond->operator, "=") == 0) return cmp == 0;
        if (strcmp(cond->operator, "!=") == 0) return cmp != 0;
        if (strcmp(cond->operator, "<") == 0) return cmp < 0;
        if (strcmp(cond->operator, ">") == 0) return cmp > 0;
        if (strcmp(cond->operator, "<=") == 0) return cmp <= 0;
        if (strcmp(cond->operator, ">=") == 0) return cmp >= 0;

        return 0;
    }

    // ===== KERN_TM (time) =====
    // Разрешены операторы: =, !=, <, >, <=, >=
    else if (strcmp(cond->field_name, "kern_tm") == 0) {
        Time val;
        if (!pars_time(cond->value_str, &val)) return 0;

        int cmp = compare_time(proc->kern_tm, val);

        if (strcmp(cond->operator, "=") == 0) return cmp == 0;
        if (strcmp(cond->operator, "!=") == 0) return cmp != 0;
        if (strcmp(cond->operator, "<") == 0) return cmp < 0;
        if (strcmp(cond->operator, ">") == 0) return cmp > 0;
        if (strcmp(cond->operator, "<=") == 0) return cmp <= 0;
        if (strcmp(cond->operator, ">=") == 0) return cmp >= 0;

        return 0;
    }

    // ===== FILE_TM (time) =====
    // Разрешены операторы: =, !=, <, >, <=, >=
    else if (strcmp(cond->field_name, "file_tm") == 0) {
        Time val;
        if (!pars_time(cond->value_str, &val)) return 0;

        int cmp = compare_time(proc->file_tm, val);

        if (strcmp(cond->operator, "=") == 0) return cmp == 0;
        if (strcmp(cond->operator, "!=") == 0) return cmp != 0;
        if (strcmp(cond->operator, "<") == 0) return cmp < 0;
        if (strcmp(cond->operator, ">") == 0) return cmp > 0;
        if (strcmp(cond->operator, "<=") == 0) return cmp <= 0;
        if (strcmp(cond->operator, ">=") == 0) return cmp >= 0;

        return 0;
    }

    // ===== CPU_USAGE (decimal) =====
    // Разрешены операторы: =, !=, <, >, <=, >=
    else if (strcmp(cond->field_name, "cpu_usage") == 0) {
        int val;
        if (!pars_decimal(cond->value_str, &val)) return 0;

        int cmp = compare_decimal(proc->cpu_usage, val);

        if (strcmp(cond->operator, "=") == 0) return cmp == 0;
        if (strcmp(cond->operator, "!=") == 0) return cmp != 0;
        if (strcmp(cond->operator, "<") == 0) return cmp < 0;
        if (strcmp(cond->operator, ">") == 0) return cmp > 0;
        if (strcmp(cond->operator, "<=") == 0) return cmp <= 0;
        if (strcmp(cond->operator, ">=") == 0) return cmp >= 0;

        return 0;
    }

    // ===== STATUS (enum) =====
    // Разрешены ТОЛЬКО операторы: =, !=, in, not_in
    else if (strcmp(cond->field_name, "status") == 0) {

        // Оператор IN - статус входит в список
        if (strcmp(cond->operator, "in") == 0) {
            const char* current_status = status_names[proc->status];
            return is_value_in_list(cond->value_str, current_status);
        }

        // Оператор NOT_IN - статус НЕ входит в список
        else if (strcmp(cond->operator, "not_in") == 0) {
            const char* current_status = status_names[proc->status];
            return !is_value_in_list(cond->value_str, current_status);
        }

        // Оператор равенства =
        else if (strcmp(cond->operator, "=") == 0) {
            Status val;
            if (!pars_status(cond->value_str, &val)) return 0;
            return proc->status == val;  // прямое сравнение
        }

        // Оператор неравенства !=
        else if (strcmp(cond->operator, "!=") == 0) {
            Status val;
            if (!pars_status(cond->value_str, &val)) return 0;
            return proc->status != val;  // прямое сравнение
        }

        // Любые другие операторы (<, >, <=, >=) для enum - бан сразу!
        return 0;
    }

    return 0;  // неизвестное поле
}

// Проверка всех условий для записи (логическое И)
int check_all_conditions(Process* proc, Condition* conditions, int cond_count) {
    // Если условий нет - запись подходит
    if (conditions == NULL || cond_count == 0) return 1;

    for (int i = 0; i < cond_count; i++) {
        if (!check_condition(proc, &conditions[i])) {
            return 0;  // хотя бы одно условие не выполняется
        }
    }
    return 1;  // все условия выполняются
}