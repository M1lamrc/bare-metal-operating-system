typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

#define VIDEO_ADDRESS 0xB8000
#define MAX_ROWS 25
#define MAX_COLS 80

#define GREEN_ON_BLACK    0x0A
#define CYAN_ON_BLACK     0x03
#define WHITE_ON_BLACK    0x0F
#define YELLOW_ON_BLACK   0x0E
#define RED_ON_BLACK      0x04
#define LBLUE_ON_BLACK    0x09


#define MAX_FILES 10
typedef struct { 
    char name[64]; 
    char content[1024];
    char folder[64];
    int is_active;
    int is_directory;
} File;
char current_dir[64] = "/";
File mrc_fs[MAX_FILES];i

void resolve_path(char *target, int silent);
void silent_cd(char *target);
void kprint(char *message, unsigned char color);
void update_cursor(int offset);
void terminal_scroll();
int strcmp(char *s1, char *s2);
int strncmp(char *s1, char *s2, int n);
void strcpy(char *dest, char *src);

static int global_cursor_offset = 0;
static int shift_pressed = 0;
char shell_buffer[256];
int shell_ptr = 0;

void outb(uint16_t port, uint8_t val) { asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port)); }
void outw(uint16_t port, uint16_t val) { asm volatile ("outw %0, %1" : : "a"(val), "Nd"(port)); }
unsigned char inb(uint16_t port) { unsigned char ret; asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port)); return ret; }

void strcpy(char *dest, char *src) {
    int i = 0;
    while (src[i] != '\0') { dest[i] = src[i]; i++; }
    dest[i] = '\0';
}

int strcmp(char *s1, char *s2) {
    int i;
    for (i = 0; s1[i] == s2[i]; i++) { if (s1[i] == '\0') return 0; }
    return s1[i] - s2[i];
}

int strncmp(char *s1, char *s2, int n) {
    for (int i = 0; i < n; i++) {
        if (s1[i] != s2[i]) return s1[i] - s2[i];
        if (s1[i] == '\0') return 0;
    }
    return 0;
}

void update_cursor(int offset) {
    offset /= 2;
    outb(0x3D4, 0x0F); outb(0x3D5, (uint8_t)(offset & 0xFF));
    outb(0x3D4, 0x0E); outb(0x3D5, (uint8_t)((offset >> 8) & 0xFF));
}

void terminal_scroll() {
    unsigned char *vidptr = (unsigned char*)VIDEO_ADDRESS;
    int line_size = MAX_COLS * 2; 
    for (int i = 0; i < (MAX_ROWS - 1) * line_size; i++) {
        vidptr[i] = vidptr[i + line_size];
    }
    int last_line_start = (MAX_ROWS - 1) * line_size;
    for (int i = 0; i < line_size; i += 2) {
        vidptr[last_line_start + i] = ' ';              
        vidptr[last_line_start + i + 1] = WHITE_ON_BLACK; 
    }
    global_cursor_offset = last_line_start; 
    update_cursor(global_cursor_offset);
}
void clear_screen() {
    unsigned char *vidptr = (unsigned char*)VIDEO_ADDRESS;
    for (int i = 0; i < MAX_ROWS * MAX_COLS * 2; i += 2) {
        vidptr[i] = ' '; vidptr[i+1] = WHITE_ON_BLACK;
    }
    global_cursor_offset = 0;
    update_cursor(0);
}

void kprint(char *message, unsigned char color) {
    unsigned char *vidptr = (unsigned char*)VIDEO_ADDRESS;
    int i = 0;
    while (message[i] != '\0') {
        if (global_cursor_offset >= MAX_ROWS * MAX_COLS * 2) {
            terminal_scroll();
        }
        if (message[i] == '\n') {
            global_cursor_offset = (global_cursor_offset / (MAX_COLS * 2) + 1) * (MAX_COLS * 2);
        } else {
            vidptr[global_cursor_offset] = message[i];
            vidptr[global_cursor_offset + 1] = color;
            global_cursor_offset += 2;
        }
        i++;
    }
    update_cursor(global_cursor_offset);
}
void wait(long long cycles) { 
    for(volatile long long i = 0; i < cycles; i++); 
}
void acpi_shutdown(uint32_t PM1a_CNT_BLK, uint16_t SLP_TYP) {
    uint16_t shutdown_command = SLP_TYP | 0x2000;
    __asm__ __volatile__("cli");
    outw(PM1a_CNT_BLK, shutdown_command);
    while(1) { __asm__ __volatile__("hlt"); }
}
unsigned char scancode_to_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};
int find_file_in_current_dir(char *filename) {
    for(int i = 0; i < MAX_FILES; i++) {
        if(mrc_fs[i].is_active && 
           !mrc_fs[i].is_directory &&
           strcmp(mrc_fs[i].name, filename) == 0 && 
           strcmp(mrc_fs[i].folder, current_dir) == 0) {
            return i;
        }
    }
    return -1;
}

void cmd_cat(char *path_input) {
    if (path_input[0] == '\0') {
        kprint("\nusage: cat <file_path>", YELLOW_ON_BLACK);
        return;
    }

    char saved_dir[64];
    strcpy(saved_dir, current_dir);

    char path_part[64];
    char filename[16];
    int last_slash = -1;


    for (int i = 0; path_input[i] != '\0'; i++) {
        if (path_input[i] == '/') last_slash = i;
    }

    if (last_slash != -1) {

        int i;
        for (i = 0; i < last_slash; i++) {
            path_part[i] = path_input[i];
        }
        path_part[i] = '\0';
        

        if (last_slash == 0) {
            strcpy(path_part, "/");
        }


        strcpy(filename, path_input + last_slash + 1);


        silent_cd(path_part);
    } else {

        strcpy(filename, path_input);
    }

    int idx = find_file_in_current_dir(filename);
    
    if (idx != -1) {
        kprint("\n", WHITE_ON_BLACK);
        kprint(mrc_fs[idx].content, YELLOW_ON_BLACK);
        kprint("\n", WHITE_ON_BLACK);
    } else {
        kprint("\nerror: can't find the ", RED_ON_BLACK);
        kprint(filename, RED_ON_BLACK);
    }


    strcpy(current_dir, saved_dir);
}
void cmd_touch(char *path_input) {
    if (path_input[0] == '\0') return;

    char saved_dir[64];
    strcpy(saved_dir, current_dir);

    char path_part[64];
    char filename[16];
    int last_slash = -1;

    for (int i = 0; path_input[i] != '\0'; i++) {
        if (path_input[i] == '/') last_slash = i;
    }

    if (last_slash != -1) {
        int i;
        for (i = 0; i < last_slash; i++) path_part[i] = path_input[i];
        path_part[i] = '\0';
        if (last_slash == 0) strcpy(path_part, "/");
        
        strcpy(filename, path_input + last_slash + 1);
        
        silent_cd(path_part);
    } else {
        strcpy(filename, path_input);
    }


    for(int i = 0; i < MAX_FILES; i++) {
        if(mrc_fs[i].is_active && 
           strcmp(mrc_fs[i].name, filename) == 0 && 
           strcmp(mrc_fs[i].folder, current_dir) == 0) {
            kprint("\nerror: there's already something with that name", RED_ON_BLACK);
            strcpy(current_dir, saved_dir);
            return;
        }
    }

    int created = 0;
    for(int i = 0; i < MAX_FILES; i++) {
        if(!mrc_fs[i].is_active) {
            strcpy(mrc_fs[i].name, filename);
            strcpy(mrc_fs[i].folder, current_dir);
            strcpy(mrc_fs[i].content, "");
            mrc_fs[i].is_active = 1;
            mrc_fs[i].is_directory = 0;
            created = 1;
            break;
        }
    }

    if(created) kprint("\nsuccessfully created", GREEN_ON_BLACK);
    else kprint("\nerror: not enough space", RED_ON_BLACK);


    strcpy(current_dir, saved_dir);
}
void init_fs() {
    for(int i = 0; i < MAX_FILES; i++) mrc_fs[i].is_active = 0;
    
    strcpy(mrc_fs[0].name, "note.txt");
    strcpy(mrc_fs[0].content, "sample text");
    strcpy(mrc_fs[0].folder, "/");
    mrc_fs[0].is_active = 1;
    mrc_fs[0].is_directory = 0;
}
void cmd_ls() {
    kprint("\n directory: ", YELLOW_ON_BLACK);
    kprint(current_dir, CYAN_ON_BLACK);
    kprint("\n", WHITE_ON_BLACK);

    for(int i = 0; i < MAX_FILES; i++) {
        if(mrc_fs[i].is_active && strcmp(mrc_fs[i].folder, current_dir) == 0) {
            if(mrc_fs[i].is_directory) kprint("[DIR] ", LBLUE_ON_BLACK);
            else kprint("      ", WHITE_ON_BLACK);
            
            kprint(mrc_fs[i].name, GREEN_ON_BLACK);
            kprint("\n", WHITE_ON_BLACK);
        }
    }
}
void cmd_edit(char *filename) {
    int file_idx = -1;

    for(int i = 0; i < MAX_FILES; i++) {
        if(mrc_fs[i].is_active && strcmp(mrc_fs[i].name, filename) == 0) {
            file_idx = i;
            break;
        }
    }

    if(file_idx == -1) {
        kprint("\nerror: not found", RED_ON_BLACK);
        return;
    }


    clear_screen();
    kprint(" ", YELLOW_ON_BLACK);
    kprint(filename, CYAN_ON_BLACK);
    kprint("\n(to save and exit, press ESC)\n\n", WHITE_ON_BLACK);
    

    kprint(mrc_fs[file_idx].content, WHITE_ON_BLACK);
    
    int content_ptr = 0; 

    while(mrc_fs[file_idx].content[content_ptr] != '\0') content_ptr++;

    while(1) {
        if (inb(0x64) & 0x01) {
            unsigned char scancode = inb(0x60);
            
            if (scancode == 0x01) break;
            
            if (!(scancode & 0x80)) {
                if (scancode == 0x1C) {
                    mrc_fs[file_idx].content[content_ptr++] = '\n';
                    kprint("\n", WHITE_ON_BLACK);
                }
else if (scancode == 0x0E) {
    if (content_ptr > 0) {

        content_ptr--;
        mrc_fs[file_idx].content[content_ptr] = '\0';

        if (global_cursor_offset > 0) {
            global_cursor_offset -= 2;
            
            unsigned char *vidptr = (unsigned char*)VIDEO_ADDRESS;
            vidptr[global_cursor_offset] = ' ';
            vidptr[global_cursor_offset + 1] = WHITE_ON_BLACK;
            
            update_cursor(global_cursor_offset);
        }
    }
}
                else if (scancode < sizeof(scancode_to_ascii)) {
                    char ascii = scancode_to_ascii[scancode];
                    if (ascii != 0 && content_ptr < 127) {
                        mrc_fs[file_idx].content[content_ptr++] = ascii;
                        mrc_fs[file_idx].content[content_ptr] = '\0';
                        char t[2] = {ascii, '\0'};
                        kprint(t, WHITE_ON_BLACK);
                    }
                }
            }
        }
    }


    clear_screen();
    kprint("file is successfully saved\n", GREEN_ON_BLACK);
    kprint("MRC> ", WHITE_ON_BLACK);
}
void cmd_mkdir(char *path_input) {
    if (path_input[0] == '\0') return;

    char saved_dir[64];
    strcpy(saved_dir, current_dir);


    if (path_input[0] == '/') {
        strcpy(current_dir, "/");
    }

    char path[64];
    strcpy(path, path_input);
    
    int p_idx = (path[0] == '/') ? 1 : 0;
    char folder_name[32];

    while (1) {
        int f_idx = 0;
        while (path[p_idx] != '/' && path[p_idx] != '\0') {
            folder_name[f_idx++] = path[p_idx++];
        }
        folder_name[f_idx] = '\0';

        if (f_idx > 0) {

            int found_idx = -1;
            for (int i = 0; i < MAX_FILES; i++) {
                if (mrc_fs[i].is_active && mrc_fs[i].is_directory &&
                    strcmp(mrc_fs[i].name, folder_name) == 0 &&
                    strcmp(mrc_fs[i].folder, current_dir) == 0) {
                    found_idx = i;
                    break;
                }
            }

            if (found_idx == -1) {

                int created = 0;
                for (int i = 0; i < MAX_FILES; i++) {
                    if (!mrc_fs[i].is_active) {
                        strcpy(mrc_fs[i].name, folder_name);
                        strcpy(mrc_fs[i].folder, current_dir);
                        mrc_fs[i].is_active = 1;
                        mrc_fs[i].is_directory = 1;
                        created = 1;
                        break;
                    }
                }
                if (!created) {
                    kprint("\nerror: not enough space", RED_ON_BLACK);
                    strcpy(current_dir, saved_dir);
                    return;
                }
            }


            silent_cd(folder_name);
        }

        if (path[p_idx] == '\0') break;
        p_idx++;
    }

    kprint("\nsuccess", GREEN_ON_BLACK);
    strcpy(current_dir, saved_dir);
}

void resolve_path(char *target, int silent) {
    if (target[0] == '\0') return;

    if (strcmp(target, "/") == 0) {
        strcpy(current_dir, "/");
        if (!silent) kprint("\nKonum: /", GREEN_ON_BLACK);
        return;
    }

    char path[64];
    strcpy(path, target);

    if (path[0] == '/') {
        strcpy(current_dir, "/");
    }

    int p_idx = (path[0] == '/') ? 1 : 0;
    char folder_name[32];

    while (1) {
        int f_idx = 0;
        while (path[p_idx] != '/' && path[p_idx] != '\0') {
            folder_name[f_idx++] = path[p_idx++];
        }
        folder_name[f_idx] = '\0';

        if (f_idx > 0) {
            if (strcmp(folder_name, "..") == 0) {
                if (strcmp(current_dir, "/") != 0) {
                    int len = 0; while(current_dir[len] != '\0') len++;
                    int i = len - 1;
                    while (i > 0 && current_dir[i] != '/') { current_dir[i] = '\0'; i--; }
                    if (i > 0 && current_dir[i] == '/') current_dir[i] = '\0';
                }
            } else {
                int found_idx = -1;
                for (int i = 0; i < MAX_FILES; i++) {
                    if (mrc_fs[i].is_active && mrc_fs[i].is_directory &&
                        strcmp(mrc_fs[i].name, folder_name) == 0 &&
                        strcmp(mrc_fs[i].folder, current_dir) == 0) {
                        found_idx = i;
                        break;
                    }
                }

                if (found_idx != -1) {
                    if (strcmp(current_dir, "/") != 0) {
                        int l = 0; while(current_dir[l] != '\0') l++;
                        current_dir[l] = '/'; current_dir[l+1] = '\0';
                    }
                    int clen = 0; while(current_dir[clen] != '\0') clen++;
                    strcpy(current_dir + (clen == 1 && current_dir[0] == '/' ? 1 : clen), folder_name);
                } else {
                    if (!silent) {
                        kprint("\ncan't found that path (", RED_ON_BLACK);
                        kprint(folder_name, RED_ON_BLACK);
                        kprint(")", RED_ON_BLACK);
                    }
                    return; 
                }
            }
        }
        if (path[p_idx] == '\0') break;
        p_idx++;
    }

    if (!silent) {
        kprint("\nlocation: ", GREEN_ON_BLACK);
        kprint(current_dir, GREEN_ON_BLACK);
    }
}

void cmd_cd(char *target) {
    resolve_path(target, 0);
}

void silent_cd(char *target) {
    resolve_path(target, 1);
}
void cmd_rm(char *path_input) {
    if (path_input[0] == '\0') return;

    char saved_dir[64];
    strcpy(saved_dir, current_dir);

    char path_part[64];
    char target_name[16];
    char target_full_path[64];
    int last_slash = -1;

    for (int i = 0; path_input[i] != '\0'; i++) {
        if (path_input[i] == '/') last_slash = i;
    }

    if (last_slash != -1) {
        int i;
        for (i = 0; i < last_slash; i++) path_part[i] = path_input[i];
        path_part[i] = '\0';
        if (last_slash == 0) strcpy(path_part, "/");
        strcpy(target_name, path_input + last_slash + 1);
        silent_cd(path_part);
    } else {
        strcpy(target_name, path_input);
    }


    strcpy(target_full_path, current_dir);
    if (strcmp(target_full_path, "/") != 0) {
        int len = 0; while(target_full_path[len]) len++;
        target_full_path[len] = '/';
        target_full_path[len+1] = '\0';
    }
    int tlen = 0; while(target_full_path[tlen]) tlen++;
    strcpy(target_full_path + (tlen == 1 && target_full_path[0] == '/' ? 1 : tlen), target_name);


    int idx = -1;
    for(int i = 0; i < MAX_FILES; i++) {
        if(mrc_fs[i].is_active && strcmp(mrc_fs[i].name, target_name) == 0 && strcmp(mrc_fs[i].folder, current_dir) == 0) {
            idx = i; break;
        }
    }

    if(idx != -1) {

        if(mrc_fs[idx].is_directory) {
            for(int i = 0; i < MAX_FILES; i++) {
                if(mrc_fs[i].is_active) {
                    int j = 0; int match = 1;
                    while(target_full_path[j] != '\0') {
                        if(mrc_fs[i].folder[j] != target_full_path[j]) { match = 0; break; }
                        j++;
                    }
                    if(match) { mrc_fs[i].is_active = 0; mrc_fs[i].name[0] = '\0'; }
                }
            }
        }
        mrc_fs[idx].is_active = 0;
        mrc_fs[idx].name[0] = '\0';
        kprint("\nsuccessfully removed", GREEN_ON_BLACK);
    }


    int j = 0;
    int path_was_cut = 1;
    while (target_full_path[j] != '\0') {
        if (saved_dir[j] != target_full_path[j]) {
            path_was_cut = 0;
            break;
        }
        j++;
    }

    if (path_was_cut && (saved_dir[j] == '/' || saved_dir[j] == '\0')) {
        strcpy(current_dir, "/");
        kprint("\nturning to root", YELLOW_ON_BLACK);
    } else {
        strcpy(current_dir, saved_dir);
    }
}
void execute_command(char *cmd) {
    if (strcmp(cmd, "help") == 0) kprint("\ncommands: HELP, VER, CLEAR, LS, CAT, TOUCH, EDIT, ECHO, HALT, MKDIR, CD, RM", YELLOW_ON_BLACK);
    else if (strcmp(cmd, "ver") == 0) kprint("\nCodename: MRC v0.0.3 Beta", GREEN_ON_BLACK);
    else if (strcmp(cmd, "ls") == 0) cmd_ls();
    else if (strcmp(cmd, "clear") == 0) clear_screen();
    else if (strncmp(cmd, "echo ", 5) == 0) { kprint("\n", YELLOW_ON_BLACK); kprint(cmd + 5, YELLOW_ON_BLACK); }
    else if (strcmp(cmd, "echo") == 0) { kprint("\nusage: echo <text>", YELLOW_ON_BLACK); }
else if (strncmp(cmd, "cat ", 4) == 0) {
    char *filename = cmd + 4;
    if (filename[0] == '\0' || filename[0] == ' ') {
        kprint("\nusage: cat <file name>", YELLOW_ON_BLACK);
    } else {
        cmd_cat(filename);
    }
}
else if (strcmp(cmd, "cat") == 0) {
    kprint("\nusage: cat <file name>", YELLOW_ON_BLACK);
}
else if (strncmp(cmd, "touch ", 6) == 0) {
    char *filename = cmd + 6;
    if (filename[0] == '\0' || filename[0] == ' ') {
        kprint("\nusage: touch <file name>", YELLOW_ON_BLACK);
    } else {
        cmd_touch(filename);
    }
}
else if (strcmp(cmd, "touch") == 0) {
    kprint("\nusage: touch <file name>", YELLOW_ON_BLACK);
}
else if (strcmp(cmd, "halt") == 0) {
    clear_screen();
    wait(100000000);
    kprint("\nkilling all processes...", GREEN_ON_BLACK);
    wait(200000000);
    __asm__("cli"); 
    clear_screen();
    

    char *bye = "\n\n\n\n\n\n\n\n\n\n                                 G O O D B Y E !!        ";
    for(int i = 0; bye[i] != '\0'; i++) {
        char t[2] = {bye[i], '\0'};
        kprint(t, LBLUE_ON_BLACK);
        wait(5000000);
    }
    
    wait(750000000);
    acpi_shutdown(0x604, 0x2000);
}
else if (strncmp(cmd, "edit ", 5) == 0) {
    cmd_edit(cmd + 5);
}
else if (strncmp(cmd, "edit", 5) == 0) {
    kprint("\nusage: edit <file name>", YELLOW_ON_BLACK);
}
else if (strncmp(cmd, "rm ", 3) == 0) {
    char *filename = cmd + 3;
    if (filename[0] == '\0' || filename[0] == ' ') {
        kprint("\nusage: rm <file name>", YELLOW_ON_BLACK);
    } else {
        cmd_rm(filename);
    }
}
else if (strcmp(cmd, "rm") == 0) {
    kprint("\nusage: rm <file name>", YELLOW_ON_BLACK);
}
else if (strcmp(cmd, "mkdir ") == 0) {
    kprint("\nusage: mkdir <directory name>", YELLOW_ON_BLACK);
}
else if (strncmp(cmd, "mkdir ", 6) == 0) {
    cmd_mkdir(cmd + 6);
}
else if (strcmp(cmd, "mkdir") == 0) {
    kprint("\nusage: mkdir <directory name>", YELLOW_ON_BLACK);
}
else if (strcmp(cmd, "cd ") == 0) {
    kprint("\nCurrent directory: ", YELLOW_ON_BLACK);
    kprint(current_dir, WHITE_ON_BLACK);
}
else if (strncmp(cmd, "cd ", 3) == 0) {
    cmd_cd(cmd + 3);
}
else if (strcmp(cmd, "cd") == 0) {
    kprint("\nLocation: ", YELLOW_ON_BLACK);
    kprint(current_dir, WHITE_ON_BLACK);
}
    else if (cmd[0] != '\0') kprint("\nUnknown command.", RED_ON_BLACK);
}


void shell_input_handler() {
    if (inb(0x64) & 0x01) { 
        unsigned char scancode = inb(0x60);
        if (scancode == 0x2A || scancode == 0x36) { shift_pressed = 1; return; }
        if (scancode == 0xAA || scancode == 0xB6) { shift_pressed = 0; return; }

        if (scancode == 0x4B) {
            int line_start = (global_cursor_offset / (MAX_COLS * 2)) * (MAX_COLS * 2) + 10;
            if (global_cursor_offset > line_start) { global_cursor_offset -= 2; update_cursor(global_cursor_offset); }
            return;
        }
        if (scancode == 0x4D) {
            int line_start = (global_cursor_offset / (MAX_COLS * 2)) * (MAX_COLS * 2) + 10;
            if (global_cursor_offset < line_start + (shell_ptr * 2)) { global_cursor_offset += 2; update_cursor(global_cursor_offset); }
            return;
        }
        if (scancode == 0x0E) {
            int line_start = (global_cursor_offset / (MAX_COLS * 2)) * (MAX_COLS * 2) + 10;
            if (global_cursor_offset > line_start) {
                global_cursor_offset -= 2;
                ((unsigned char*)VIDEO_ADDRESS)[global_cursor_offset] = ' ';
                ((unsigned char*)VIDEO_ADDRESS)[global_cursor_offset + 1] = WHITE_ON_BLACK;
                if (shell_ptr > 0) shell_ptr--;
                update_cursor(global_cursor_offset);
            }
            return;
        }
        if (scancode & 0x80) return;
        if (scancode == 0x1C) {
            shell_buffer[shell_ptr] = '\0';
            execute_command(shell_buffer);
            shell_ptr = 0;
            kprint("\nMRC> ", WHITE_ON_BLACK);
            return;
        }
        if (scancode < sizeof(scancode_to_ascii)) {
            char ascii = scancode_to_ascii[scancode];
            if (ascii != 0) {
                if (shift_pressed && ascii >= 'a' && ascii <= 'z') ascii -= 32;
                if (shell_ptr < 255) {
                    shell_buffer[shell_ptr++] = ascii;
                    char temp[2] = {ascii, '\0'};
                    kprint(temp, WHITE_ON_BLACK);
                }
            }
        }
    }
}



void kmain(void) {
    init_fs();
    clear_screen();
    kprint("\n\n\n\n\n\n\n\n\n\n                                W E L C O M E ! !                                \n", GREEN_ON_BLACK);

    wait(800000000); 

    clear_screen();

    kprint("\nVERSION - MRC v0.0.3 Beta\n", CYAN_ON_BLACK);
    kprint("\nMRC> ", WHITE_ON_BLACK);

    while(1) {
        shell_input_handler();
    }
}
