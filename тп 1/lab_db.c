#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// =====  ¿—“ŒÃÕ€≈ “»œ€ =====

typedef enum {
    RUNNING,
    READY,
    PAUSED,
    BLOCKED,
    DYING,
    SLEEPING
} Status;

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
} Time;

typedef struct Process {
    int pid;
    char* name;
    int priority;
    Time kern_tm;
    Time file_tm;
    int cpu_usage;
    Status status;
    struct Process* next;
} Process;

// ===== √ÀŒ¡¿À‹Õ€≈ œ≈–≈Ã≈ÕÕ€≈ =====

Process* head = NULL;
int process_count = 0;

// ===== —◊≈“◊» » œ¿Ãﬂ“» =====

int malloc_count = 0;
int calloc_count = 0;
int realloc_count = 0;
int free_count = 0;

// ===== ‘”Õ ÷»» œ¿Ãﬂ“» =====

void* my_malloc(size_t size) {
    malloc_count++;
    return malloc(size);
}

void* my_calloc(size_t count, size_t size) {
    calloc_count++;
    return calloc(count, size);
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

// ===== –¿¡Œ“¿ —Œ —œ»— ŒÃ =====

Process* creation_process() {
    Process* new_proc = (Process*)my_malloc(sizeof(Process));
    if (new_proc == NULL) return NULL;
    memset(new_proc, 0, sizeof(Process));
    new_proc->status = RUNNING;
    return new_proc;
}

void free_process(Process* proc) {
    if (proc == NULL) return;
    if (proc->name != NULL) my_free(proc->name);
    my_free(proc);
}

void append_process(Process* new_proc) {
    if (new_proc == NULL) return;
    if (head == NULL) {
        head = new_proc;
    }
    else {
        Process* current = head;
        while (current->next != NULL) current = current->next;
        current->next = new_proc;
    }
    process_count++;
}

Process* get_process_index(int index) {
    if (index < 0 || index >= process_count || head == NULL) return NULL;
    Process* current = head;
    for (int i = 0; i < index; i++) current = current->next;
    return current;
}

int delete_process(int index) {
    if (index < 0 || index >= process_count || head == NULL) return 0;
    Process* to_delete = NULL;
    if (index == 0) {
        to_delete = head;
        head = head->next;
    }
    else {
        Process* prev = head;
        for (int i = 0; i < index - 1; i++) prev = prev->next;
        to_delete = prev->next;
        prev->next = to_delete->next;
    }
    free_process(to_delete);
    process_count--;
    return 1;
}

void clear_allproc() {
    while (head != NULL) {
        Process* temp = head;
        head = head->next;
        free_process(temp);
    }
    process_count = 0;
}

// ===== ¬€¬Œƒ Œÿ»¡Œ  =====

void print_incorrect(FILE* output, const char* command) {
    fprintf(output, "incorrect:'");
    int count = 0;
    while (command[count] != '\0' && count < 20) {
        fprintf(output, "%c", command[count]);
        count++;
    }
    fprintf(output, "'\n");
}

// ===== œ¿–—»Õ√ =====

int pars_valid_int(const char* str) {
    if (str == NULL || *str == '\0') return 0;
    if (*str == '-') str++;
    if (*str == '\0') return 0;
    while (*str) {
        if (!isdigit(*str)) return 0;
        str++;
    }
    return 1;
}

int diapozon_int(const char* str, int* rez) {
    if (!pars_valid_int(str)) return 0;
    char* end;
    long val = strtol(str, &end, 10);
    if (val < -2147483647 - 1 || val > 2147483647) return 0;
    *rez = (int)val;
    return 1;
}

char* pars_str(const char* str) {
    if (str == NULL || *str != '"') return NULL;
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

int pars_time(const char* str, Time* t) {
    if (str == NULL || *str != '\'') return 0;
    str++;
    int h, m, s;
    if (sscanf(str, "%d:%d:%d", &h, &m, &s) != 3) return 0;
    if (h < 0 || h > 23 || m < 0 || m > 59 || s < 0 || s > 59) return 0;
    const char* p = str;
    while (*p && *p != '\'') p++;
    if (*p != '\'') return 0;
    t->hour = h;
    t->minute = m;
    t->second = s;
    return 1;
}

int pars_decimal(const char* str, int* rez) {
    if (str == NULL || rez == NULL) return 0;
    const char* p = str;
    int celoe = 0, zapita = 0, negetiv = 0, celoe_digits = 0, has_celoe = 0;

    if (*p == '-') {
        negetiv = 1;
        p++;
    }

    while (*p && isdigit(*p)) {
        celoe = celoe * 10 + (*p - '0');
        celoe_digits++;
        p++;
        has_celoe = 1;
    }

    if (celoe_digits > 3 || celoe > 999) return 0;

    if (*p == '.') {
        p++;
        if (!isdigit(*p) && !has_celoe) return 0;
        int frac_digits = 0, frac_value = 0;
        while (*p && isdigit(*p) && frac_digits < 2) {
            frac_value = frac_value * 10 + (*p - '0');
            frac_digits++;
            p++;
        }
        if (*p && isdigit(*p)) return 0;
        if (frac_digits == 1) zapita = frac_value * 10;
        else if (frac_digits == 2) zapita = frac_value;
    }

    while (*p && isspace(*p)) p++;
    if (*p != '\0') return 0;

    long long value = (long long)celoe * 100 + zapita;
    if (negetiv) value = -value;
    if (value < -2147483647 - 1 || value > 2147483647) return 0;

    *rez = (int)value;
    return 1;
}

int pars_status(const char* str, Status* status) {
    if (str == NULL || *str != '\'') return 0;
    str++;
    char name[20];
    int i = 0;
    while (*str && *str != '\'' && i < 19) {
        name[i++] = *str;
        str++;
    }
    name[i] = '\0';
    if (*str != '\'') return 0;
    for (int j = 0; j < 6; j++) {
        if (strcmp(name, status_names[j]) == 0) {
            *status = (Status)j;
            return 1;
        }
    }
    return 0;
}

// ===== ¬€¬Œƒ =====

void print_int(FILE* out, int value) {
    fprintf(out, "%d", value);
}

void print_str(FILE* out, const char* str) {
    fprintf(out, "\"");
    for (const char* p = str; *p; p++) {
        if (*p == '"' || *p == '\\') fprintf(out, "\\%c", *p);
        else fprintf(out, "%c", *p);
    }
    fprintf(out, "\"");
}

void print_time(FILE* out, Time t) {
    fprintf(out, "'%02d:%02d:%02d'", t.hour, t.minute, t.second);
}

void print_decimal(FILE* out, int value) {
    if (value < 0) {
        fprintf(out, "-");
        value = -value;
    }
    fprintf(out, "%d.%02d", value / 100, value % 100);
}

void print_status(FILE* out, Status status) {
    fprintf(out, "'%s'", status_names[status]);
}

// ===== —–¿¬Õ≈Õ»≈ =====

int compare_int(int a, int b) {
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

int compare_str(const char* a, const char* b) {
    if (a == NULL && b == NULL) return 0;
    if (a == NULL) return -1;
    if (b == NULL) return 1;
    return strcmp(a, b);
}

int compare_time(Time a, Time b) {
    if (a.hour != b.hour) return a.hour < b.hour ? -1 : 1;
    if (a.minute != b.minute) return a.minute < b.minute ? -1 : 1;
    if (a.second != b.second) return a.second < b.second ? -1 : 1;
    return 0;
}

int compare_decimal(int a, int b) {
    return compare_int(a, b);
}

int compare_status(Status a, Status b) {
    return compare_int(a, b);
}

// ===== –¿¡Œ“¿ —Œ —œ»— ¿Ã» «Õ¿◊≈Õ»… =====

int is_value_in_list(const char* list_str, const char* value) {
    if (list_str == NULL || *list_str != '[') return 0;
    const char* p = list_str + 1;
    while (*p == ' ') p++;
    if (*p == ']') return 0;
    while (*p && *p != ']') {
        while (*p == ' ') p++;
        if (*p != '\'') { p++; continue; }
        p++;
        char item[20] = { 0 };
        int i = 0;
        while (*p && *p != '\'' && i < 19) item[i++] = *p++;
        if (*p != '\'') break;
        p++;
        if (strcmp(value, item) == 0) return 1;
        while (*p == ' ' || *p == ',') p++;
    }
    return 0;
}

// ===== —“–” “”–¿ ”—ÀŒ¬»ﬂ =====

typedef struct Condition {
    char field_name[50];
    char operator[10];
    char value_str[256];
} Condition;

// ===== œ–Œ¬≈– ¿ ”—ÀŒ¬»… =====

int check_condition(Process* proc, Condition* cond) {
    if (!proc || !cond) return 0;

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
        return 0;
    }

    if (strcmp(cond->field_name, "name") == 0) {
        char* val = pars_str(cond->value_str);
        if (!val) return 0;
        if (!proc->name) { my_free(val); return 0; }
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

    if (strcmp(cond->field_name, "priority") == 0) {
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

    if (strcmp(cond->field_name, "kern_tm") == 0) {
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

    if (strcmp(cond->field_name, "file_tm") == 0) {
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

    if (strcmp(cond->field_name, "cpu_usage") == 0) {
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

    if (strcmp(cond->field_name, "status") == 0) {
        if (strcmp(cond->operator, "in") == 0) {
            return is_value_in_list(cond->value_str, status_names[proc->status]);
        }
        if (strcmp(cond->operator, "not_in") == 0) {
            return !is_value_in_list(cond->value_str, status_names[proc->status]);
        }
        Status val;
        if (!pars_status(cond->value_str, &val)) return 0;
        if (strcmp(cond->operator, "=") == 0) return proc->status == val;
        if (strcmp(cond->operator, "!=") == 0) return proc->status != val;
        return 0;
    }

    return 0;
}

int check_all_conditions(Process* proc, Condition* conditions, int cond_count) {
    if (!conditions || cond_count == 0) return 1;
    for (int i = 0; i < cond_count; i++) {
        if (!check_condition(proc, &conditions[i])) return 0;
    }
    return 1;
}

// ===== œ¿–—»Õ√ —œ»— ¿ œŒÀ≈… =====

char** parse_field_list(const char* str, int* count) {
    if (!str || !*str) {
        *count = 0;
        return NULL;
    }

    char* temp = my_malloc(strlen(str) + 1);
    if (!temp) {
        *count = 0;
        return NULL;
    }
    strcpy(temp, str);

    *count = 1;
    for (const char* p = str; *p; p++) {
        if (*p == ',') (*count)++;
    }

    char** result = my_malloc(*count * sizeof(char*));
    if (!result) {
        my_free(temp);
        *count = 0;
        return NULL;
    }

    int idx = 0;
    char* token = strtok(temp, ",");
    while (token && idx < *count) {
        while (*token == ' ') token++;
        char* end = token + strlen(token) - 1;
        while (end > token && (*end == ' ' || *end == '\t')) *end-- = '\0';
        result[idx] = my_malloc(strlen(token) + 1);
        if (result[idx]) strcpy(result[idx], token);
        idx++;
        token = strtok(NULL, ",");
    }

    my_free(temp);
    return result;
}

void free_field_list(char** list, int count) {
    if (!list) return;
    for (int i = 0; i < count; i++) {
        if (list[i]) my_free(list[i]);
    }
    my_free(list);
}

// ===== œ¿–—»Õ√ ”—ÀŒ¬»ﬂ =====

int parse_condition(const char* str, Condition* cond) {
    if (!str || !cond) return 0;

    char* temp = my_malloc(strlen(str) + 1);
    if (!temp) return 0;
    strcpy(temp, str);

    char* op_start = temp;
    while (*op_start && !strchr("=!<>/", *op_start)) op_start++;

    if (*op_start == '\0') {
        my_free(temp);
        return 0;
    }

    int field_len = op_start - temp;
    strncpy(cond->field_name, temp, field_len);
    cond->field_name[field_len] = '\0';

    char* end = cond->field_name + strlen(cond->field_name) - 1;
    while (end >= cond->field_name && (*end == ' ' || *end == '\t')) *end-- = '\0';

    char* op = op_start;
    char* val_start = NULL;

    if (strncmp(op, "<=", 2) == 0) {
        strcpy(cond->operator, "<=");
        val_start = op + 2;
    }
    else if (strncmp(op, ">=", 2) == 0) {
        strcpy(cond->operator, ">=");
        val_start = op + 2;
    }
    else if (strncmp(op, "!=", 2) == 0) {
        strcpy(cond->operator, "!=");
        val_start = op + 2;
    }
    else if (strncmp(op, "==", 2) == 0) {
        strcpy(cond->operator, "=");
        val_start = op + 2;
    }
    else if (strncmp(op, "in", 2) == 0) {
        strcpy(cond->operator, "in");
        val_start = op + 2;
        if (*val_start == '/') val_start++;
    }
    else if (strncmp(op, "not_in", 6) == 0) {
        strcpy(cond->operator, "not_in");
        val_start = op + 6;
        if (*val_start == '/') val_start++;
    }
    else {
        if (*op == '=') {
            strcpy(cond->operator, "=");
            val_start = op + 1;
        }
        else if (*op == '<') {
            strcpy(cond->operator, "<");
            val_start = op + 1;
        }
        else if (*op == '>') {
            strcpy(cond->operator, ">");
            val_start = op + 1;
        }
        else {
            my_free(temp);
            return 0;
        }
    }

    while (*val_start == ' ' || *val_start == '\t') val_start++;
    strcpy(cond->value_str, val_start);

    my_free(temp);
    return 1;
}

// ===== INSERT =====

void insert(const char* args, const char* full_command, FILE* output) {
    Process* proc = creation_process();
    if (!proc) {
        print_incorrect(output, full_command);
        return;
    }

    char* args_copy = my_malloc(strlen(args) + 1);
    if (!args_copy) {
        free_process(proc);
        print_incorrect(output, full_command);
        return;
    }
    strcpy(args_copy, args);

    int pid_set = 0, name_set = 0, priority_set = 0;
    int kern_set = 0, file_set = 0, cpu_set = 0, status_set = 0;
    int found = 0;

    char* token = strtok(args_copy, ",");
    while (token) {
        while (*token == ' ') token++;

        char* eq = strchr(token, '=');
        if (!eq) {
            my_free(args_copy);
            free_process(proc);
            print_incorrect(output, full_command);
            return;
        }

        *eq = '\0';
        char* field = token;
        char* value = eq + 1;

        char* end = field + strlen(field) - 1;
        while (end > field && (*end == ' ' || *end == '\t')) *end-- = '\0';
        while (*value == ' ' || *value == '\t') value++;

        if (strcmp(field, "pid") == 0) {
            if (pid_set) goto error;
            if (!diapozon_int(value, &proc->pid)) goto error;
            pid_set = 1; found++;
        }
        else if (strcmp(field, "name") == 0) {
            if (name_set) goto error;
            proc->name = pars_str(value);
            if (!proc->name) goto error;
            name_set = 1; found++;
        }
        else if (strcmp(field, "priority") == 0) {
            if (priority_set) goto error;
            if (!diapozon_int(value, &proc->priority)) goto error;
            priority_set = 1; found++;
        }
        else if (strcmp(field, "kern_tm") == 0) {
            if (kern_set) goto error;
            if (!pars_time(value, &proc->kern_tm)) goto error;
            kern_set = 1; found++;
        }
        else if (strcmp(field, "file_tm") == 0) {
            if (file_set) goto error;
            if (!pars_time(value, &proc->file_tm)) goto error;
            file_set = 1; found++;
        }
        else if (strcmp(field, "cpu_usage") == 0) {
            if (cpu_set) goto error;
            if (!pars_decimal(value, &proc->cpu_usage)) goto error;
            cpu_set = 1; found++;
        }
        else if (strcmp(field, "status") == 0) {
            if (status_set) goto error;
            if (!pars_status(value, &proc->status)) goto error;
            status_set = 1; found++;
        }
        else {
            goto error;
        }

        token = strtok(NULL, ",");
        continue;

    error:
        my_free(args_copy);
        free_process(proc);
        print_incorrect(output, full_command);
        return;
    }

    my_free(args_copy);

    if (found != 7) {
        free_process(proc);
        print_incorrect(output, full_command);
        return;
    }

    append_process(proc);
    fprintf(output, "insert:%d\n", process_count);
}

// ===== SELECT =====

void select_cmd(const char* args, const char* full_command, FILE* output) {
    if (!args || !*args) {
        print_incorrect(output, full_command);
        return;
    }

    char* args_copy = my_malloc(strlen(args) + 1);
    if (!args_copy) {
        print_incorrect(output, full_command);
        return;
    }
    strcpy(args_copy, args);

    // –Û˜ÌÓÈ Ô‡ÒËÌ„ ÔÓÎÂÈ
    char* fields_str = args_copy;
    while (*fields_str == ' ' || *fields_str == '\t') fields_str++;
    char* end = fields_str;
    while (*end && !isspace(*end)) end++;

    if (end == fields_str) {
        my_free(args_copy);
        print_incorrect(output, full_command);
        return;
    }

    char* temp_fields = my_malloc(end - fields_str + 1);
    strncpy(temp_fields, fields_str, end - fields_str);
    temp_fields[end - fields_str] = '\0';

    int field_count;
    char** field_list = parse_field_list(temp_fields, &field_count);
    my_free(temp_fields);

    if (!field_list || field_count == 0) {
        my_free(args_copy);
        if (field_list) free_field_list(field_list, field_count);
        print_incorrect(output, full_command);
        return;
    }

    // œ‡ÒËÌ„ ÛÒÎÓ‚ËÈ
    Condition conditions[100];
    int cond_count = 0;
    int error = 0;

    char* cond_str = end;
    while (*cond_str == ' ' || *cond_str == '\t') cond_str++;

    if (*cond_str) {
        char* cond_copy = my_malloc(strlen(cond_str) + 1);
        if (!cond_copy) {
            free_field_list(field_list, field_count);
            my_free(args_copy);
            print_incorrect(output, full_command);
            return;
        }
        strcpy(cond_copy, cond_str);

        char* token = strtok(cond_copy, " \t");
        while (token) {
            if (!parse_condition(token, &conditions[cond_count])) {
                error = 1;
                break;
            }
            cond_count++;
            token = strtok(NULL, " \t");
        }

        my_free(cond_copy);
    }

    if (error) {
        free_field_list(field_list, field_count);
        my_free(args_copy);
        print_incorrect(output, full_command);
        return;
    }

    int found = 0;
    Process* curr = head;
    while (curr) {
        if (check_all_conditions(curr, conditions, cond_count)) found++;
        curr = curr->next;
    }

    fprintf(output, "select:%d\n", found);

    curr = head;
    while (curr) {
        if (check_all_conditions(curr, conditions, cond_count)) {
            for (int i = 0; i < field_count; i++) {
                if (i > 0) fprintf(output, " ");
                if (strcmp(field_list[i], "pid") == 0) {
                    fprintf(output, "pid="); print_int(output, curr->pid);
                }
                else if (strcmp(field_list[i], "name") == 0) {
                    fprintf(output, "name="); print_str(output, curr->name);
                }
                else if (strcmp(field_list[i], "priority") == 0) {
                    fprintf(output, "priority="); print_int(output, curr->priority);
                }
                else if (strcmp(field_list[i], "kern_tm") == 0) {
                    fprintf(output, "kern_tm="); print_time(output, curr->kern_tm);
                }
                else if (strcmp(field_list[i], "file_tm") == 0) {
                    fprintf(output, "file_tm="); print_time(output, curr->file_tm);
                }
                else if (strcmp(field_list[i], "cpu_usage") == 0) {
                    fprintf(output, "cpu_usage="); print_decimal(output, curr->cpu_usage);
                }
                else if (strcmp(field_list[i], "status") == 0) {
                    fprintf(output, "status="); print_status(output, curr->status);
                }
            }
            fprintf(output, "\n");
        }
        curr = curr->next;
    }

    free_field_list(field_list, field_count);
    my_free(args_copy);
}

// ===== DELETE =====

void delete_cmd(const char* args, const char* full_command, FILE* output) {
    if (!args || !*args) {
        int del = process_count;
        clear_allproc();
        fprintf(output, "delete:%d\n", del);
        return;
    }

    char* args_copy = my_malloc(strlen(args) + 1);
    if (!args_copy) {
        print_incorrect(output, full_command);
        return;
    }
    strcpy(args_copy, args);

    Condition conditions[100];
    int cond_count = 0;
    int error = 0;

    char* token = strtok(args_copy, " \t");
    while (token) {
        if (!parse_condition(token, &conditions[cond_count])) {
            error = 1;
            break;
        }
        cond_count++;
        token = strtok(NULL, " \t");
    }

    if (error) {
        my_free(args_copy);
        print_incorrect(output, full_command);
        return;
    }

    int* indices = my_malloc(process_count * sizeof(int));
    if (!indices) {
        my_free(args_copy);
        print_incorrect(output, full_command);
        return;
    }

    int del_count = 0, idx = 0;
    Process* curr = head;
    while (curr) {
        if (check_all_conditions(curr, conditions, cond_count)) {
            indices[del_count++] = idx;
        }
        idx++;
        curr = curr->next;
    }

    for (int i = del_count - 1; i >= 0; i--) {
        delete_process(indices[i]);
    }

    my_free(indices);
    my_free(args_copy);
    fprintf(output, "delete:%d\n", del_count);
}

// ===== UPDATE =====

void update_field(Process* proc, const char* field, const char* value) {
    if (!proc || !field || !value) return;
    if (strcmp(field, "pid") == 0) diapozon_int(value, &proc->pid);
    else if (strcmp(field, "name") == 0) {
        if (proc->name) my_free(proc->name);
        proc->name = pars_str(value);
    }
    else if (strcmp(field, "priority") == 0) diapozon_int(value, &proc->priority);
    else if (strcmp(field, "kern_tm") == 0) pars_time(value, &proc->kern_tm);
    else if (strcmp(field, "file_tm") == 0) pars_time(value, &proc->file_tm);
    else if (strcmp(field, "cpu_usage") == 0) pars_decimal(value, &proc->cpu_usage);
    else if (strcmp(field, "status") == 0) pars_status(value, &proc->status);
}

void update_cmd(const char* args, const char* full_command, FILE* output) {
    if (!args || !*args) {
        print_incorrect(output, full_command);
        return;
    }

    char* args_copy = my_malloc(strlen(args) + 1);
    if (!args_copy) {
        print_incorrect(output, full_command);
        return;
    }
    strcpy(args_copy, args);

    char* updates_str = strtok(args_copy, " \t");
    if (!updates_str) {
        my_free(args_copy);
        print_incorrect(output, full_command);
        return;
    }

    char* updates_copy = my_malloc(strlen(updates_str) + 1);
    if (!updates_copy) {
        my_free(args_copy);
        print_incorrect(output, full_command);
        return;
    }
    strcpy(updates_copy, updates_str);

    char update_fields[10][50];
    char update_values[10][256];
    int update_count = 0;

    char* token = strtok(updates_copy, ",");
    while (token) {
        while (*token == ' ') token++;
        char* eq = strchr(token, '=');
        if (!eq) {
            my_free(updates_copy);
            my_free(args_copy);
            print_incorrect(output, full_command);
            return;
        }
        *eq = '\0';
        char* field = token;
        char* value = eq + 1;

        char* end = field + strlen(field) - 1;
        while (end > field && (*end == ' ' || *end == '\t')) *end-- = '\0';
        while (*value == ' ' || *value == '\t') value++;

        for (int i = 0; i < update_count; i++) {
            if (strcmp(update_fields[i], field) == 0) {
                my_free(updates_copy);
                my_free(args_copy);
                print_incorrect(output, full_command);
                return;
            }
        }

        strcpy(update_fields[update_count], field);
        strcpy(update_values[update_count], value);
        update_count++;
        token = strtok(NULL, ",");
    }

    my_free(updates_copy);

    if (update_count == 0) {
        my_free(args_copy);
        print_incorrect(output, full_command);
        return;
    }

    Condition conditions[100];
    int cond_count = 0;
    int error = 0;

    char* cond_str = strtok(NULL, "");
    if (cond_str) {
        char* cond_copy = my_malloc(strlen(cond_str) + 1);
        if (!cond_copy) {
            my_free(args_copy);
            print_incorrect(output, full_command);
            return;
        }
        strcpy(cond_copy, cond_str);

        token = strtok(cond_copy, " \t");
        while (token) {
            if (!parse_condition(token, &conditions[cond_count])) {
                error = 1;
                break;
            }
            cond_count++;
            token = strtok(NULL, " \t");
        }
        my_free(cond_copy);
    }

    if (error) {
        my_free(args_copy);
        print_incorrect(output, full_command);
        return;
    }

    int updated = 0;
    Process* curr = head;
    while (curr) {
        if (check_all_conditions(curr, conditions, cond_count)) {
            for (int i = 0; i < update_count; i++) {
                update_field(curr, update_fields[i], update_values[i]);
            }
            updated++;
        }
        curr = curr->next;
    }

    my_free(args_copy);
    fprintf(output, "update:%d\n", updated);
}

// ===== UNIQ =====

int compare_processes(Process* a, Process* b, char** fields, int count) {
    if (!a || !b || !fields || count == 0) return 0;
    for (int i = 0; i < count; i++) {
        if (strcmp(fields[i], "pid") == 0) {
            if (a->pid != b->pid) return 0;
        }
        else if (strcmp(fields[i], "name") == 0) {
            if (!a->name && !b->name) continue;
            if (!a->name || !b->name) return 0;
            if (strcmp(a->name, b->name) != 0) return 0;
        }
        else if (strcmp(fields[i], "priority") == 0) {
            if (a->priority != b->priority) return 0;
        }
        else if (strcmp(fields[i], "kern_tm") == 0) {
            if (compare_time(a->kern_tm, b->kern_tm) != 0) return 0;
        }
        else if (strcmp(fields[i], "file_tm") == 0) {
            if (compare_time(a->file_tm, b->file_tm) != 0) return 0;
        }
        else if (strcmp(fields[i], "cpu_usage") == 0) {
            if (a->cpu_usage != b->cpu_usage) return 0;
        }
        else if (strcmp(fields[i], "status") == 0) {
            if (a->status != b->status) return 0;
        }
    }
    return 1;
}

void uniq_cmd(const char* args, const char* full_command, FILE* output) {
    if (!args || !*args) {
        print_incorrect(output, full_command);
        return;
    }

    char* args_copy = my_malloc(strlen(args) + 1);
    if (!args_copy) {
        print_incorrect(output, full_command);
        return;
    }
    strcpy(args_copy, args);

    while (*args_copy == ' ' || *args_copy == '\t') args_copy++;
    char* end = args_copy + strlen(args_copy) - 1;
    while (end > args_copy && (*end == ' ' || *end == '\t')) *end-- = '\0';

    int field_count;
    char** field_list = parse_field_list(args_copy, &field_count);

    if (!field_list || field_count == 0) {
        if (field_list) free_field_list(field_list, field_count);
        my_free(args_copy);
        print_incorrect(output, full_command);
        return;
    }

    for (int i = 0; i < field_count; i++) {
        for (int j = i + 1; j < field_count; j++) {
            if (strcmp(field_list[i], field_list[j]) == 0) {
                free_field_list(field_list, field_count);
                my_free(args_copy);
                print_incorrect(output, full_command);
                return;
            }
        }
    }

    int* to_delete = my_malloc(process_count * sizeof(int));
    if (!to_delete) {
        free_field_list(field_list, field_count);
        my_free(args_copy);
        print_incorrect(output, full_command);
        return;
    }

    for (int i = 0; i < process_count; i++) to_delete[i] = 0;

    for (int i = process_count - 1; i >= 0; i--) {
        if (to_delete[i]) continue;
        Process* curr = get_process_index(i);
        for (int j = i - 1; j >= 0; j--) {
            if (to_delete[j]) continue;
            Process* other = get_process_index(j);
            if (compare_processes(curr, other, field_list, field_count)) {
                to_delete[j] = 1;
            }
        }
    }

    int del_count = 0;
    for (int i = 0; i < process_count; i++) if (to_delete[i]) del_count++;

    for (int i = process_count - 1; i >= 0; i--) {
        if (to_delete[i]) delete_process(i);
    }

    my_free(to_delete);
    free_field_list(field_list, field_count);
    my_free(args_copy);
    fprintf(output, "uniq:%d\n", del_count);
}

// ===== SORT =====

typedef struct {
    char field_name[50];
    int order;
} SortField;

int compare_for_sort(Process* a, Process* b, SortField* fields, int count) {
    if (!a || !b || !fields || count == 0) return 0;
    for (int i = 0; i < count; i++) {
        int cmp = 0;
        if (strcmp(fields[i].field_name, "pid") == 0) cmp = compare_int(a->pid, b->pid);
        else if (strcmp(fields[i].field_name, "name") == 0) cmp = compare_str(a->name, b->name);
        else if (strcmp(fields[i].field_name, "priority") == 0) cmp = compare_int(a->priority, b->priority);
        else if (strcmp(fields[i].field_name, "kern_tm") == 0) cmp = compare_time(a->kern_tm, b->kern_tm);
        else if (strcmp(fields[i].field_name, "file_tm") == 0) cmp = compare_time(a->file_tm, b->file_tm);
        else if (strcmp(fields[i].field_name, "cpu_usage") == 0) cmp = compare_decimal(a->cpu_usage, b->cpu_usage);
        else if (strcmp(fields[i].field_name, "status") == 0) return 0;
        if (cmp != 0) return fields[i].order == 0 ? cmp : -cmp;
    }
    return 0;
}

int parse_sort_fields(const char* str, SortField* fields, int* count) {
    if (!str || !*str || !fields || !count) return 0;

    char* temp = my_malloc(strlen(str) + 1);
    if (!temp) return 0;
    strcpy(temp, str);

    *count = 0;
    int error = 0;

    char* token = strtok(temp, ",");
    while (token && *count < 100) {
        while (*token == ' ') token++;
        char* eq = strchr(token, '=');
        if (!eq) { error = 1; break; }

        *eq = '\0';
        char* name = token;
        char* order = eq + 1;

        char* end = name + strlen(name) - 1;
        while (end > name && (*end == ' ' || *end == '\t')) *end-- = '\0';
        while (*order == ' ' || *order == '\t') order++;
        end = order + strlen(order) - 1;
        while (end > order && (*end == ' ' || *end == '\t')) *end-- = '\0';

        if (strcmp(name, "status") == 0) { error = 1; break; }

        for (int i = 0; i < *count; i++) {
            if (strcmp(fields[i].field_name, name) == 0) { error = 1; break; }
        }
        if (error) break;

        strcpy(fields[*count].field_name, name);
        if (strcmp(order, "asc") == 0) fields[*count].order = 0;
        else if (strcmp(order, "desc") == 0) fields[*count].order = 1;
        else { error = 1; break; }

        (*count)++;
        token = strtok(NULL, ",");
    }

    my_free(temp);
    return !error && *count > 0;
}

Process** list_to_array() {
    if (process_count == 0) return NULL;
    Process** arr = my_malloc(process_count * sizeof(Process*));
    if (!arr) return NULL;
    Process* curr = head;
    for (int i = 0; i < process_count; i++) {
        arr[i] = curr;
        curr = curr->next;
    }
    return arr;
}

void array_to_list(Process** arr, int count) {
    if (!arr || count == 0) {
        head = NULL;
        return;
    }
    head = arr[0];
    for (int i = 0; i < count - 1; i++) {
        arr[i]->next = arr[i + 1];
    }
    arr[count - 1]->next = NULL;
}

void quicksort(Process** arr, int left, int right, SortField* fields, int count) {
    if (left >= right) return;
    int p = left + (right - left) / 2;
    Process* pivot = arr[p];
    int i = left, j = right;
    while (i <= j) {
        while (i <= right && compare_for_sort(arr[i], pivot, fields, count) < 0) i++;
        while (j >= left && compare_for_sort(arr[j], pivot, fields, count) > 0) j--;
        if (i <= j) {
            Process* tmp = arr[i];
            arr[i] = arr[j];
            arr[j] = tmp;
            i++; j--;
        }
    }
    if (left < j) quicksort(arr, left, j, fields, count);
    if (i < right) quicksort(arr, i, right, fields, count);
}

void sort_cmd(const char* args, const char* full_command, FILE* output) {
    if (!args || !*args) {
        print_incorrect(output, full_command);
        return;
    }

    char* args_copy = my_malloc(strlen(args) + 1);
    if (!args_copy) {
        print_incorrect(output, full_command);
        return;
    }
    strcpy(args_copy, args);

    while (*args_copy == ' ' || *args_copy == '\t') args_copy++;
    char* end = args_copy + strlen(args_copy) - 1;
    while (end > args_copy && (*end == ' ' || *end == '\t')) *end-- = '\0';

    SortField fields[100];
    int count = 0;

    if (!parse_sort_fields(args_copy, fields, &count)) {
        my_free(args_copy);
        print_incorrect(output, full_command);
        return;
    }

    if (count == 0) {
        my_free(args_copy);
        print_incorrect(output, full_command);
        return;
    }

    Process** arr = list_to_array();
    if (!arr && process_count > 0) {
        my_free(args_copy);
        print_incorrect(output, full_command);
        return;
    }

    if (process_count > 0) {
        quicksort(arr, 0, process_count - 1, fields, count);
    }

    array_to_list(arr, process_count);

    if (arr) my_free(arr);
    my_free(args_copy);
    fprintf(output, "sort:%d\n", process_count);
}

// ===== MAIN =====

int main() {
    FILE* input = fopen("input.txt", "r");
    FILE* output = fopen("output.txt", "w");

    if (!output) {
        if (input) fclose(input);
        return 1;
    }

    char line[10000];

    if (input) {
        while (fgets(line, sizeof(line), input)) {
            line[strcspn(line, "\n")] = '\0';
            char* cr = strchr(line, '\r');
            if (cr) *cr = '\0';

            if (line[0] == '\0') continue;

            char* line_copy = my_malloc(strlen(line) + 1);
            if (!line_copy) continue;
            strcpy(line_copy, line);

            char* cmd = strtok(line_copy, " \t");
            if (!cmd) {
                my_free(line_copy);
                continue;
            }

            char* args = line;
            while (*args && !isspace(*args)) args++;
            while (*args == ' ' || *args == '\t') args++;

            if (strcmp(cmd, "insert") == 0) insert(args, line, output);
            else if (strcmp(cmd, "select") == 0) select_cmd(args, line, output);
            else if (strcmp(cmd, "delete") == 0) delete_cmd(args, line, output);
            else if (strcmp(cmd, "update") == 0) update_cmd(args, line, output);
            else if (strcmp(cmd, "uniq") == 0) uniq_cmd(args, line, output);
            else if (strcmp(cmd, "sort") == 0) sort_cmd(args, line, output);
            else print_incorrect(output, line);

            my_free(line_copy);
        }
        fclose(input);
    }

    fclose(output);

    FILE* memstat = fopen("memstat.txt", "w");
    if (memstat) {
        fprintf(memstat, "malloc:%d\n", malloc_count);
        fprintf(memstat, "calloc:%d\n", calloc_count);
        fprintf(memstat, "realloc:%d\n", realloc_count);
        fprintf(memstat, "free:%d\n", free_count);
        fclose(memstat);
    }

    clear_allproc();
    return 0;
}