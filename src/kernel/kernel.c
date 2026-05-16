typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef signed char int8_t;
typedef unsigned long long uint64_t;
typedef unsigned long      uintptr_t;
 
#define NULL ((void*)0)
void clr_term_scr();
int gui_win_curr_x = 10;
int gui_win_curr_y = 10;
void termp(char* text, uint32_t color);
#include "gui.h"
 

 
struct idt_entry_t {
    uint16_t offset_low;     
    uint16_t selector;       
    uint8_t  ist;            
    uint8_t  types_attr;     
    uint16_t offset_mid;     
    uint32_t offset_high;    
    uint32_t reserved;       
} __attribute__((packed));

 
struct idt_ptr_t {
    uint16_t limit;          
    uint64_t base;           
} __attribute__((packed));

 
struct idt_entry_t idt[256];
struct idt_ptr_t   idtr;

 
extern void irq1_keyboard_wrapper();
extern void irq12_mouse_wrapper();
 
void dummy_interrupt_handler();

 
void init_idt();
void pic_remap();
void set_idt_gate(int vector, uint64_t handler, uint16_t selector, uint8_t attributes);
void kbd_interrupt_handler();
void mouse_interrupt_handler();
 
int is_on_title_bar(int mx, int my, Window* win) {
    if (!win) return 0;
    return (mx >= win->x && mx <= (win->x + win->w) && my >= win->y && my <= (win->y + 25));
}
typedef enum {
    false = 0,
    true = 1
} bool;
void process_mouse_byte(uint8_t data);
void alpha_x_title_print(int x, int y, const char* str, uint32_t color, int percent);
bool is_dragging = false;
int drag_offset_x = 0;
int drag_offset_y = 0;
 


 
Window terminal_win = {
    .x = 100, .y = 100,
    .w = 600, .h = 400,
    .bg_color = 0xFF000000,
    .title_color = 0xFFE94560,
    .title = "Terminal"
};

 
void create_window(Window* win) {
    if (!win) return;
    draw_rect(win->x, win->y, win->w, win->h, win->bg_color);
    draw_rect(win->x, win->y, win->w, 25, win->title_color);
}

uint32_t* GOP_FRAMEBUFFER = 0;
uint32_t  GOP_PIXELS_PER_SCANLINE = 0;

#define MAX_ROWS 25
#define MAX_COLS 80

#define GREEN_ON_BLACK    0x0A
#define WHITE_ON_BLACK    0x0F
#define YELLOW_ON_BLACK   0x0E
#define RED_ON_BLACK      0x04
#define LBLUE_ON_BLACK    0x09

#define MAX_FILES 10
#define MODE_TERMINAL 0
#define MODE_GUI      1
int current_mode = MODE_TERMINAL;
int global_cursor_x = 10;
int global_cursor_y = 30;

uint32_t MAX_COLS_PIXELS = 1024;
uint32_t MAX_HEIGHT = 768;

void draw_char(char c, int x, int y, uint32_t color);
void clear_screen();
int find_file_in_current_dir(char *filename);
void execute_command(char *cmd);
uint32_t vga_to_rgb(uint8_t vga_color) {
    switch (vga_color) {
        case 0x0A: return 0xFF55FF55;
        case 0x03: return 0xFF55FFFF;
        case 0x0F: return 0xFFFFFFFF;
        case 0x0E: return 0xFFFFFF55;
        case 0x04: return 0xFFAA0000;
        case 0x09: return 0xFF5555FF;
        default:   return 0xFFFFFFFF;
    }
}

typedef struct {
    char name[64];
    char content[1024];
    char folder[64];
    int is_active;
    int is_directory;
} File;

char current_dir[64] = "/";
File mrc_fs[MAX_FILES];
unsigned char scancode_to_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};
int mouse_x = 100;
int mouse_y = 100;
int mouse_phase = 0;
uint8_t mouse_data[3];
void resolve_path(char *target, int silent);
void silent_cd(char *target);
void kprint(char *message, unsigned char color);
void terminal_scroll();
int strcmp(char *s1, char *s2);
int strncmp(char *s1, char *s2, int n);
void strcpy(char *dest, char *src);

static int shift_pressed = 0;
char shell_buffer[256];
int shell_ptr = 0;

void outb(uint16_t port, uint8_t val) { asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port)); }
void outw(uint16_t port, uint16_t val) { asm volatile ("outw %0, %1" : : "a"(val), "Nd"(port)); }
unsigned char inb(uint16_t port) { unsigned char ret; asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port)); return ret; }
void draw_terminal_cursor(int x, int y, int state) {

    int cursor_height = 18;
    int cursor_width = 2;

    uint32_t color = state ? 0xFFFFFFFF : 0x00000000;

    for (int i = 0; i < cursor_height; i++) {
        for (int j = 0; j < cursor_width; j++) {
            int px = x + j;

            int py = y - 14 + i;

 
if (px >= 0 && (uint32_t)px < MAX_COLS_PIXELS && py >= 0 && (uint32_t)py < MAX_HEIGHT) {
    GOP_FRAMEBUFFER[py * GOP_PIXELS_PER_SCANLINE + px] = color;
}
        }
    }
}

 
int gui_mode_active = 0; 




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

void cmd_cat(char *path_input) {
    if (path_input[0] == '\0') {
        termp("\nusage: cat <file_path>", YELLOW_ON_BLACK);
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
 
void mouse_wait(uint8_t type);
void mouse_write(uint8_t write);
uint8_t mouse_read();
void handle_ps2_mouse();
void draw_mouse_cursor(int x, int y);
void clear_mouse_cursor(int x, int y);
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
            termp("\nerror: there's already something with that name", RED_ON_BLACK);
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

    if(created) termp("\nsuccessfully created", GREEN_ON_BLACK);
    else termp("\nerror: not enough space", RED_ON_BLACK);


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
    termp("\n directory: ", YELLOW_ON_BLACK);
    termp(current_dir, WHITE_ON_BLACK);
    termp("\n", WHITE_ON_BLACK);

    for(int i = 0; i < MAX_FILES; i++) {
        if(mrc_fs[i].is_active && strcmp(mrc_fs[i].folder, current_dir) == 0) {
            if(mrc_fs[i].is_directory) termp("[DIR] ", LBLUE_ON_BLACK);
            else termp("      ", WHITE_ON_BLACK);

            termp(mrc_fs[i].name, GREEN_ON_BLACK);
            termp("\n", WHITE_ON_BLACK);
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
        termp("\nerror: not found", RED_ON_BLACK);
        return;
    }


    clear_screen();
    termp(" ", YELLOW_ON_BLACK);
    termp(filename, WHITE_ON_BLACK);
    termp("\n(to save and exit, press ESC)\n\n", WHITE_ON_BLACK);


    termp(mrc_fs[file_idx].content, WHITE_ON_BLACK);

    int content_ptr = 0;

    while(mrc_fs[file_idx].content[content_ptr] != '\0') content_ptr++;

    while(1) {
        if (inb(0x64) & 0x01) {
            unsigned char scancode = inb(0x60);

            if (scancode == 0x01) break;

            if (!(scancode & 0x80)) {
                if (scancode == 0x1C) {
                    mrc_fs[file_idx].content[content_ptr++] = '\n';
                    termp("\n", WHITE_ON_BLACK);
                }
else if (scancode == 0x0E) {
    if (content_ptr > 0) {

        content_ptr--;
        mrc_fs[file_idx].content[content_ptr] = '\0';
    }
}
                else if (scancode < sizeof(scancode_to_ascii)) {
                    char ascii = scancode_to_ascii[scancode];
                    if (ascii != 0 && content_ptr < 127) {
                        mrc_fs[file_idx].content[content_ptr++] = ascii;
                        mrc_fs[file_idx].content[content_ptr] = '\0';
                        char t[2] = {ascii, '\0'};
                        termp(t, WHITE_ON_BLACK);
                    }
                }
            }
        }
    }


    clear_screen();
    termp("file is successfully saved\n", GREEN_ON_BLACK);
    termp("Alpha-X> ", WHITE_ON_BLACK);
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
                    termp("\nerror: not enough space", RED_ON_BLACK);
                    strcpy(current_dir, saved_dir);
                    return;
                }
            }


            silent_cd(folder_name);
        }

        if (path[p_idx] == '\0') break;
        p_idx++;
    }

    termp("\nsuccess", GREEN_ON_BLACK);
    strcpy(current_dir, saved_dir);
}
void execute_gui_command(char *cmd) {
     
     
     
    execute_command(cmd);
}

void resolve_path(char *target, int silent) {
    if (target[0] == '\0') return;

    if (strcmp(target, "/") == 0) {
        strcpy(current_dir, "/");
        if (!silent) termp("\nlocation: /", GREEN_ON_BLACK);
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
                        termp("\ncan't found that path (", RED_ON_BLACK);
                        termp(folder_name, RED_ON_BLACK);
                        termp(")", RED_ON_BLACK);
                    }
                    return;
                }
            }
        }
        if (path[p_idx] == '\0') break;
        p_idx++;
    }

    if (!silent) {
        termp("\nlocation: ", GREEN_ON_BLACK);
        termp(current_dir, GREEN_ON_BLACK);
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
        termp("\nsuccessfully removed", GREEN_ON_BLACK);
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
        termp("\nturning to root", YELLOW_ON_BLACK);
    } else {
        strcpy(current_dir, saved_dir);
    }
}

void terminal_scroll() {
    uint32_t line_height = 20;
    for (uint32_t y = 0; y < MAX_HEIGHT - line_height; y++) {
        for (uint32_t x = 0; x < MAX_COLS_PIXELS; x++) {
            GOP_FRAMEBUFFER[y * GOP_PIXELS_PER_SCANLINE + x] =
                GOP_FRAMEBUFFER[(y + line_height) * GOP_PIXELS_PER_SCANLINE + x];
        }
    }

    for (uint32_t y = MAX_HEIGHT - line_height; y < MAX_HEIGHT; y++) {
        for (uint32_t x = 0; x < MAX_COLS_PIXELS; x++) {
            GOP_FRAMEBUFFER[y * GOP_PIXELS_PER_SCANLINE + x] = 0;
        }
    }
    global_cursor_y -= line_height;
}

void clear_screen() {
    for (uint32_t i = 0; i < GOP_PIXELS_PER_SCANLINE * MAX_HEIGHT; i++) {
        GOP_FRAMEBUFFER[i] = 0;
    }
    global_cursor_x = 10;
    global_cursor_y = 30;
}
void change_resolution(uint32_t target_w, uint32_t target_h) {



    MAX_COLS_PIXELS = target_w;
    MAX_HEIGHT = target_h;


    clear_screen(0x00000000);


    global_cursor_x = 10;
    global_cursor_y = 40;

    termp("change resolution success\n", 0x0A);
}
#include "GeistMonoLight20ptb4.h"

uint32_t apply_intensity(uint32_t color, uint8_t intensity) {

    uint32_t alpha = intensity * 17;

    uint8_t r = ((color >> 16) & 0xFF) * alpha / 255;
    uint8_t g = ((color >> 8) & 0xFF) * alpha / 255;
    uint8_t b = (color & 0xFF) * alpha / 255;

    return (r << 16) | (g << 8) | b;
}
void draw_char(char c, int x, int y, uint32_t color) {
    if (c < 32 || c > 126) return;
    const GFXglyph *glyph = &GeistMonoLight20ptb4Glyphs[c - 32];
    const uint8_t  *bitmap = GeistMonoLight20ptb4Bitmaps;

    int scale = 2;
    uint32_t bo = glyph->bitmapOffset;
    uint8_t  w  = glyph->width;
    uint8_t  h  = glyph->height;
    int8_t   xo = glyph->xOffset / scale;
    int8_t   yo = glyph->yOffset / scale;

    for (int yy = 0; yy < h; yy += scale) {
        for (int xx = 0; xx < w; xx += scale) {
            int pixel_index = (yy * w) + xx;
            uint8_t byte_val = bitmap[bo + (pixel_index / 2)];
            uint8_t intensity = (pixel_index % 2 == 0) ? (byte_val >> 4) : (byte_val & 0x0F);

             
            if (intensity > 2) { 
                uint32_t final_pix = apply_intensity(color, intensity);

                int px = x + xo + (xx / scale);
                int py = y + yo + (yy / scale);

                if (px >= 0 && (uint32_t)px < MAX_COLS_PIXELS && py >= 0 && (uint32_t)py < MAX_HEIGHT) {
                    GOP_FRAMEBUFFER[py * GOP_PIXELS_PER_SCANLINE + px] = final_pix; 
                }
                 
            }
        }
    }
}
void kprint(char *message, unsigned char color) { 

    uint32_t rgb_color = vga_to_rgb(color);
    for (int i = 0; message[i] != '\0'; i++) {
        draw_terminal_cursor(global_cursor_x, global_cursor_y, 0);
        if (message[i] == '\n') {
            global_cursor_x = 10;
            global_cursor_y += 36;
        } else {
            draw_char(message[i], global_cursor_x, global_cursor_y, rgb_color);
            const GFXglyph *glyph = &GeistMonoLight20ptb4Glyphs[message[i] - 32];
            global_cursor_x += (glyph->xAdvance / 2);
        }
    }
}

void wait(long long cycles) {
    for(volatile long long i = 0; i < cycles; i++);
}
Window* focused_window = NULL;  
void acpi_shutdown(uint32_t PM1a_CNT_BLK, uint16_t SLP_TYP) {
    uint16_t shutdown_command = SLP_TYP | 0x2000;
    __asm__ __volatile__("cli");
    outw(PM1a_CNT_BLK, shutdown_command);
    while(1) { __asm__ __volatile__("hlt"); }
}



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


void execute_command(char *cmd) {
    if (strcmp(cmd, "help") == 0) {
        termp("\ncommands: HELP, VER, CLEAR, LS, CAT, TOUCH, EDIT, ECHO, HALT, MKDIR, CD, RM, CHRES", YELLOW_ON_BLACK);
    }
    else if (strcmp(cmd, "ver") == 0) {
        termp("\nMRC Alpha X", GREEN_ON_BLACK);
    }
    else if (strcmp(cmd, "ls") == 0) {
        cmd_ls();
    }
    else if (strcmp(cmd, "clear") == 0) {
        clr_term_scr();
    }
    else if (strncmp(cmd, "echo ", 5) == 0) {
        termp("\n", YELLOW_ON_BLACK);
        termp(cmd + 5, YELLOW_ON_BLACK);
    }
    else if (strcmp(cmd, "echo") == 0) {
        termp("\nusage: echo <text>", YELLOW_ON_BLACK);
    }
    else if (strncmp(cmd, "cat ", 4) == 0) {
        cmd_cat(cmd + 4);
    }
    else if (strcmp(cmd, "cat") == 0) {
        termp("\nusage: cat <file name>", YELLOW_ON_BLACK);
    }
    else if (strncmp(cmd, "touch ", 6) == 0) {
        cmd_touch(cmd + 6);
    }
    else if (strcmp(cmd, "touch") == 0) {
        termp("\nusage: touch <file name>", YELLOW_ON_BLACK);
    }
    else if (strncmp(cmd, "edit ", 5) == 0) {
        cmd_edit(cmd + 5);
    }
    else if (strcmp(cmd, "edit") == 0) {
        termp("\nusage: edit <file name>", YELLOW_ON_BLACK);
    }
    else if (strncmp(cmd, "mkdir ", 6) == 0) {
        cmd_mkdir(cmd + 6);
    }
    else if (strcmp(cmd, "mkdir") == 0) {
        termp("\nusage: mkdir <directory name>", YELLOW_ON_BLACK);
    }
    else if (strncmp(cmd, "cd ", 3) == 0) {
        cmd_cd(cmd + 3);
    }
    else if (strcmp(cmd, "cd") == 0) {
        termp("\nLocation: ", YELLOW_ON_BLACK);
        termp(current_dir, WHITE_ON_BLACK);
    }
    else if (strncmp(cmd, "rm ", 3) == 0) {
        cmd_rm(cmd + 3);
    }
    else if (strcmp(cmd, "rm") == 0) {
        termp("\nusage: rm <file name>", YELLOW_ON_BLACK);
    }
    else if (strcmp(cmd, "halt") == 0) {
        clear_screen();
        kprint("\nkilling all processes...", GREEN_ON_BLACK);
        wait(200000000);
        __asm__("cli");
        clear_screen();

        char *bye = "\n\n\n            G O O D B Y E !!            ";
        for(int i = 0; bye[i] != '\0'; i++) {
            char t[2] = {bye[i], '\0'};
            kprint(t, LBLUE_ON_BLACK);
            wait(5000000);
        }

        wait(750000000);

        acpi_shutdown(0x604, 0x2000);
    }


    else if (strncmp(cmd, "chres ", 6) == 0) {
        uint32_t w = 0, h = 0;
        char *p = cmd + 6;

        while (*p == ' ') p++;
        while (*p >= '0' && *p <= '9') w = w * 10 + (*p++ - '0');
        while (*p == ' ') p++;
        while (*p >= '0' && *p <= '9') h = h * 10 + (*p++ - '0');

        if (w > 0 && h > 0) {
            change_resolution(w, h);
        } else {
            termp("\nusage: chres <width> <height>", YELLOW_ON_BLACK);
        }
    }
    else if (strcmp(cmd, "chres") == 0) {
        termp("\nusage: chres <width> <height>", YELLOW_ON_BLACK);
    }
}
 
 

 
uint32_t lerp_color(uint32_t start_color, uint32_t end_color, int step, int max_steps) {
     
    uint8_t r1 = (start_color >> 16) & 0xFF;
    uint8_t g1 = (start_color >> 8) & 0xFF;
    uint8_t b1 = start_color & 0xFF;

    uint8_t r2 = (end_color >> 16) & 0xFF;
    uint8_t g2 = (end_color >> 8) & 0xFF;
    uint8_t b2 = end_color & 0xFF;

     
    uint8_t r = r1 + (r2 - r1) * step / max_steps;
    uint8_t g = g1 + (g2 - g1) * step / max_steps;
    uint8_t b = b1 + (b2 - b1) * step / max_steps;

    return (r << 16) | (g << 8) | b;
}
 
 
 
extern uint32_t* vesa_framebuffer;
void put_pixel(int x, int y, uint32_t color) {
     
    if (x >= 0 && (uint32_t)x < MAX_COLS_PIXELS && y >= 0 && (uint32_t)y < MAX_HEIGHT) {
        GOP_FRAMEBUFFER[y * GOP_PIXELS_PER_SCANLINE + x] = color;
    }
}
 
uint32_t blend_colors(uint32_t foreground, uint32_t background, uint8_t alpha) {
     
    uint8_t rb = (background >> 16) & 0xFF;
    uint8_t gb = (background >> 8) & 0xFF;
    uint8_t bb = background & 0xFF;

    uint8_t rf = (foreground >> 16) & 0xFF;
    uint8_t gf = (foreground >> 8) & 0xFF;
    uint8_t bf = foreground & 0xFF;

    uint8_t r = (rf * alpha + rb * (255 - alpha)) >> 8;
    uint8_t g = (gf * alpha + gb * (255 - alpha)) >> 8;
    uint8_t b = (bf * alpha + bb * (255 - alpha)) >> 8;

    return (r << 16) | (g << 8) | b;
}
void draw_anti_aliased_corner(int x, int y, uint32_t color, uint32_t bg_color) {
     
     
    
     
    uint32_t pixel1 = blend_colors(color, bg_color, 0x40); 
    put_pixel(x, y, pixel1); 

     
    uint32_t pixel2 = blend_colors(color, bg_color, 0xA0);
    put_pixel(x + 1, y, pixel2);
    put_pixel(x, y + 1, pixel2);
}
 

uint32_t isqrt(uint32_t n) {
    uint32_t root = 0;
    uint32_t bit = 1 << 30;  

    while (bit > n) bit >>= 2;

    while (bit != 0) {
        if (n >= root + bit) {
            n -= root + bit;
            root = (root >> 1) + bit;
        } else {
            root >>= 1;
        }
        bit >>= 2;
    }
    return root;
}
 
void draw_circle(int x0, int y0, int radius, uint32_t color) {
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x * x + y * y <= radius * radius) {
                put_pixel(x0 + x, y0 + y, color);
            }
        }
    }
}
void title_text(int x, int y, const char* str, int yd) {
     
     
     
    alpha_x_title_print(x + 1, y + 1, str, 0x000000, 40);

     
    alpha_x_title_print(x, y, str, 0xFFFFFFFF, 40);
}
void header_text(int x, int y, const char* str, int yd) {
     
     
     
    alpha_x_title_print(x + 2, y + 2, str, 0x8f8f8f, 100);

     
    alpha_x_title_print(x, y, str, 0xFF000000, 100);
}
bool is_inside_title_bar(int mx, int my, Window* win) {
     
    return (mx >= win->x && mx <= (win->x + win->w) &&
            my >= win->y && my <= (win->y + 30));
}
 
void draw_alpha_x_final_aqua(int x, int y, int w, int h, uint32_t bg_color) {
    int r = 9; 
    int title_h = 30;
    
     
    draw_rect(x, y, w, title_h, bg_color);
    for (int i = 0; i < title_h; i++) {
        uint32_t color = (i < 15) ? lerp_color(0xFFAEAEBD, 0xFF909099, i, 15) 
                                  : lerp_color(0xFF909099, 0xFFAEAEBD, i - 15, 15);
        int offset = 0;
        if (i < r) {
            int val = r * r - (r - i) * (r - i);
            offset = r - isqrt(val > 0 ? val : 0);
        }
        draw_rect(x + offset, y + i, w - (2 * offset), 1, color);
    }

     
     
 
 
 
    draw_rect(x, y + title_h, w, h - title_h, 0xFFE0E0E0);

     
    int shadow_spread = 10;  

    for (int i = 0; i < shadow_spread; i++) {
        for (int j = 0; j < w; j++) {
            int px = x + j;
            int py = y + title_h + i;

            if (px < MAX_COLS_PIXELS && py < MAX_HEIGHT) {
                uint32_t current_pixel = GOP_FRAMEBUFFER[py * GOP_PIXELS_PER_SCANLINE + px];
                int intensity;

                 
                 
                 
                 
                if (i == 0)      intensity = 35; 
                else if (i == 1) intensity = 25; 
                else             intensity = 18 - (i * 2); 

                if (intensity > 0) {
                    uint32_t r_c = ((current_pixel >> 16) & 0xFF) * (100 - intensity) / 100;
                    uint32_t g_c = ((current_pixel >> 8) & 0xFF) * (100 - intensity) / 100;
                    uint32_t b_c = (current_pixel & 0xFF) * (100 - intensity) / 100;

                    GOP_FRAMEBUFFER[py * GOP_PIXELS_PER_SCANLINE + px] = (r_c << 16) | (g_c << 8) | b_c;
                }
            }
        }
    }

     
    int btn_y = y + 15;
 
 
draw_circle(x + 20, y + 15, 6, 0xFFFF0000);  
draw_circle(x + 40, y + 15, 6, 0xFFFFFF00);  
draw_circle(x + 60, y + 15, 6, 0xFF00FF00);  
    
    title_text(x + 210, y + 20, "Starting to MRC", 0xFFFFFFFF);
    header_text(x + 195, y + 200, "Welcome", 0xFF000000);
}
void backup_mouse_area(int x, int y);
uint32_t lerp_color(uint32_t start_color, uint32_t end_color, int step, int max_steps);
void put_pixel(int x, int y, uint32_t color);
void stguimode() {
    clear_screen();         
    gui_mode_active = 1;     

     
 
draw_alpha_x_final_aqua(200, 100, 600, 400, 0x000000);  

	global_cursor_x = terminal_win.x + 12;
	global_cursor_y = terminal_win.y + 45;  
}
void write_to_window(Window* win, char* text, int* win_cursor_x, int* win_cursor_y, uint32_t color) {
     
     

    for (int i = 0; text[i] != '\0'; i++) {
         
        if (text[i] == '\n' || *win_cursor_x > (win->w - 20)) {
            *win_cursor_x = 10;
            *win_cursor_y += 25;  
        }

        if (text[i] != '\n') {
             
            int real_x = win->x + *win_cursor_x;
            int real_y = win->y + 35 + *win_cursor_y;  

             
            if (real_x < (win->x + win->w - 5) && real_y < (win->y + win->h - 5)) {
                draw_char(text[i], real_x, real_y, color);
            }

             
            const GFXglyph *glyph = &GeistMonoLight20ptb4Glyphs[text[i] - 32];
            *win_cursor_x += (glyph->xAdvance / 2);
        }
    }
}
 
float clamp(float x, float min, float max) {
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

float smoothstep(float edge0, float edge1, float x) {
    float t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

 
 
 
 

uint32_t get_pixel(int x, int y) {
     
    if (x < 0 || (uint32_t)x >= MAX_COLS_PIXELS || y < 0 || (uint32_t)y >= MAX_HEIGHT) {
        return 0;
    }
     
    return GOP_FRAMEBUFFER[y * MAX_COLS_PIXELS + x];
}

 
void draw_msdf_char(int x, int y, uint32_t color, const uint8_t* msdf_data) {
    int glyph_size = 32; 
    float screen_px_range = 4.0f;
    float edge_softness = 0.05f;

    for (int i = 0; i < glyph_size; i++) {
        for (int j = 0; j < glyph_size; j++) {
            int idx = (i * glyph_size + j) * 3;
            float r = (float)msdf_data[idx] / 255.0f;
            float g = (float)msdf_data[idx + 1] / 255.0f;
            float b = (float)msdf_data[idx + 2] / 255.0f;

             
            float sigDist = (r < g) ? ((b < r) ? r : (b < g ? b : g)) 
                                    : ((b < g) ? g : (b < r ? b : r));

             
            float dist = (sigDist - 0.5f) * screen_px_range + 0.5f;
             
float opacity = 1.0f;  

            if (opacity > 0.01f) {
                uint32_t bg = get_pixel(x + j, y + i);
                 
                uint32_t final_c = blend_colors(color, bg, (uint8_t)(opacity * 255));
                
                 
                put_pixel(x + j, y + i, final_c);
            }
        }
    }
}
void alpha_x_title_print(int x, int y, const char* str, uint32_t color, int percent) {
    while (*str) {
        int glyph_idx = (uint8_t)(*str) - 32;
        const GFXglyph *glyph = &GeistMonoLight20ptb4Glyphs[glyph_idx];
        const uint8_t *bitmap = &GeistMonoLight20ptb4Bitmaps[glyph->bitmapOffset];

         
        int scaled_w = (glyph->width * percent) / 100 + 2; 
        int scaled_h = (glyph->height * percent) / 100 + 2;
        int scaled_x_off = (glyph->xOffset * percent) / 100;
        int scaled_y_off = (glyph->yOffset * percent) / 100;

        for (int i = 0; i < scaled_h; i++) {
            for (int j = 0; j < scaled_w; j++) {
                 
                int src_x = (j * 100) / percent;
                int src_y = (i * 100) / percent;

                if (src_x >= glyph->width || src_y >= glyph->height) continue;

                 
                int pixel_num = src_y * glyph->width + src_x;
                uint8_t byte_val = bitmap[pixel_num / 2];
                uint8_t p1 = (pixel_num % 2 == 0) ? (byte_val >> 4) : (byte_val & 0x0F);

                 
                uint8_t alpha = p1 * 17;  

                 
                if (src_x + 1 >= glyph->width || src_y + 1 >= glyph->height) {
                    alpha = (alpha * 60) / 100;  
                }

                if (alpha > 5) {
                    int final_x = x + j + scaled_x_off;
                    int final_y = y + i + scaled_y_off;

                    if (final_x >= 0 && final_x < (int)MAX_COLS_PIXELS && final_y >= 0 && final_y < (int)MAX_HEIGHT) {
                        uint32_t bg = get_pixel(final_x, final_y);
                        put_pixel(final_x, final_y, blend_colors(color, bg, alpha));
                    }
                }
            }
        }
         
        x += ((glyph->xAdvance * percent) / 100) + 1;
        str++;
    }
}
void clr_term_scr() {
     
     
    draw_rect(terminal_win.x, terminal_win.y + 25, 
              terminal_win.w, terminal_win.h - 25, 
              terminal_win.bg_color);
              
     
    global_cursor_x = terminal_win.x + 5;
    global_cursor_y = terminal_win.y + 30;
}
void shell_input_handler() {
     
    uint8_t status = inb(0x64);

     
    if (status & 0x01) {
         
        if (status & 0x20) {
             
             
            inb(0x60); 
            return; 
        }

         
        unsigned char scancode = inb(0x60);



        if (scancode == 0x2A || scancode == 0x36) { shift_pressed = 1; return; }
        if (scancode == 0xAA || scancode == 0xB6) { shift_pressed = 0; return; }

if (scancode == 0x0E) {  
            if (shell_ptr > 0) {
                shell_ptr--;
                char last_char = shell_buffer[shell_ptr];
                const GFXglyph *glyph = &GeistMonoLight20ptb4Glyphs[last_char - 32];
                 
                global_cursor_x -= (glyph->xAdvance / 2); 
                
                 
                draw_rect(global_cursor_x, global_cursor_y - 25, (glyph->xAdvance / 2), 35, terminal_win.bg_color);
            }
            return;
        }

        if (scancode & 0x80) return;  

        if (scancode == 0x1C) {  
            shell_buffer[shell_ptr] = '\0';
            execute_command(shell_buffer);
            shell_ptr = 0;
            termp("\nAlpha-X> ", WHITE_ON_BLACK);
            return;
        }

        if (scancode < sizeof(scancode_to_ascii)) {
            char ascii = scancode_to_ascii[scancode];
            if (ascii != 0) {
                if (shift_pressed && ascii >= 'a' && ascii <= 'z') ascii -= 32;
                if (shell_ptr < 255) {
                    shell_buffer[shell_ptr++] = ascii;
                    char temp[2] = {ascii, '\0'};
                    termp(temp, WHITE_ON_BLACK);
                }
            }
        }
    }
}  
int cursor_visible = 0;
uint32_t timer_counter = 0;
void scroll_terminal() {
    uint32_t row_height = 30;     
    uint32_t header_height = 25;  
    
     
    clear_mouse_cursor(mouse_x, mouse_y);

     
     
    for (uint32_t y = terminal_win.y + header_height + row_height; y < (uint32_t)(terminal_win.y + terminal_win.h); y++) {
    for (uint32_t x = terminal_win.x; x < (uint32_t)(terminal_win.x + terminal_win.w); x++) {
             
            GOP_FRAMEBUFFER[(y - row_height) * GOP_PIXELS_PER_SCANLINE + x] = 
                GOP_FRAMEBUFFER[y * GOP_PIXELS_PER_SCANLINE + x];
        }
    }

     
     
    draw_rect(terminal_win.x, 
              terminal_win.y + terminal_win.h - row_height, 
              terminal_win.w, 
              row_height, 
              terminal_win.bg_color);  

     
    global_cursor_y -= row_height;

     
    backup_mouse_area(mouse_x, mouse_y);
    draw_mouse_cursor(mouse_x, mouse_y); 
}
void termp(char* text, uint32_t color) {
     
     
     
    uint32_t rgb_color = (color < 16) ? vga_to_rgb((unsigned char)color) : color;

    int scale = 2; 
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] == '\n') {
            global_cursor_x = terminal_win.x + 10;
            global_cursor_y += 30;
            continue;
        }

         
        draw_char(text[i], global_cursor_x, global_cursor_y, rgb_color);

        const GFXglyph *glyph = &GeistMonoLight20ptb4Glyphs[text[i] - 32];
        global_cursor_x += (glyph->xAdvance / scale); 

         
        if (global_cursor_x > (terminal_win.x + terminal_win.w - 20)) {
            global_cursor_x = terminal_win.x + 10;
            global_cursor_y += 30;
        }
        
         
        if (global_cursor_y > (terminal_win.y + terminal_win.h - 30)) {
            scroll_terminal();
        }
    }
}
void shell_loop() {
    while(1) {
         
        uint8_t status = inb(0x64);
        
        if (status & 0x01) {  
            if (status & 0x20) {
                handle_ps2_mouse();  
            } else {
                handle_ps2_mouse();  
            }
        }
        
         
    }
}
void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    while (timeout--) {
        if (type == 0 && (inb(0x64) & 1)) return;  
        if (type == 1 && !(inb(0x64) & 2)) return;  
    }
}

void mouse_write(uint8_t write) {
    mouse_wait(1);
    outb(0x64, 0xD4);
    mouse_wait(1);
    outb(0x60, write);
}

uint8_t mouse_read() {
    mouse_wait(0);
    return inb(0x60);
}
void mouse_init() {
    uint8_t _status;

     
    while (inb(0x64) & 1) inb(0x60);

     
    outb(0x64, 0xA8);

     
    mouse_wait(1);
    outb(0x64, 0x20);
    mouse_wait(0);
    _status = (inb(0x60) | 2); 

     
    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, _status);

     

     
    mouse_write(0xF6); 
    mouse_read();

     
    mouse_write(0xF3);
    mouse_read();
    mouse_write(200);  
    mouse_read();

     
    mouse_write(0xE8);
    mouse_read();
    mouse_write(0x03);  
    mouse_read();

     
    mouse_write(0xF4); 
    mouse_read();
}

void kprint_hex(uint64_t n) {
    char hex_chars[] = "0123456789ABCDEF";
    char buffer[19];
    buffer[0] = '0';
    buffer[1] = 'x';

    for (int i = 15; i >= 0; i--) {
        buffer[i + 2] = hex_chars[(n >> (i * 4)) & 0xF];
    }
    buffer[18] = '\0';
    termp(buffer, 0x0E);
}
static inline void outl(uint16_t port, uint32_t val) {
    asm volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    asm volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

#define VGA_CONTROL_REG 0x71400


void on_key_press(char key) {
    if (gui_mode_active) return;  

    if (shell_ptr < 255) {
		terminal_win.x = 100;
    terminal_win.y = 100;
    terminal_win.w = 600;
    terminal_win.h = 400;
    terminal_win.bg_color = 0xFF000000;
    terminal_win.title_color = 0xFF000000;
    strcpy(terminal_win.title, "Terminal");
        shell_buffer[shell_ptr++] = key;
        shell_buffer[shell_ptr] = '\0';
        char t[2] = {key, '\0'};
        termp(t, WHITE_ON_BLACK);
    }
}
 

 
uint32_t mouse_bg_buffer[12 * 20];

#define M_W 12
#define M_H 20
#define SHADOW_OFFSET 2

 
uint32_t mouse_backup[14 * 22];

unsigned char mouse_cursor_map[20][12] = {
	{2,0,0,0,0,0,0,0,0,0,0,0},
    {2,2,0,0,0,0,0,0,0,0,0,0},
    {2,1,2,0,0,0,0,0,0,0,0,0},
    {2,1,1,2,0,0,0,0,0,0,0,0},
    {2,1,1,1,2,0,0,0,0,0,0,0},
    {2,1,1,1,1,2,0,0,0,0,0,0},
    {2,1,1,1,1,1,2,0,0,0,0,0},
    {2,1,1,1,1,1,1,2,0,0,0,0},
    {2,1,1,1,1,1,1,1,2,0,0,0},
    {2,1,1,1,1,1,1,1,1,2,0,0},
    {2,1,1,1,1,1,1,1,1,1,2,0},
    {2,1,1,1,1,1,2,2,2,2,2,2},
    {2,1,1,2,1,1,2,0,0,0,0,0},
    {2,1,2,0,2,1,1,2,0,0,0,0},
    {2,2,0,0,2,1,1,2,0,0,0,0},
    {2,0,0,0,0,2,1,1,2,0,0,0},
    {0,0,0,0,0,2,1,1,2,0,0,0},
    {0,0,0,0,0,0,2,1,1,2,0,0},
    {0,0,0,0,0,0,2,1,1,2,0,0},
    {0,0,0,0,0,0,0,2,2,0,0,0}
};

 

void draw_aqua_button(int x, int y, uint32_t color) {
    int radius = 8;  

     
    for (int i = -radius; i <= radius; i++) {
        for (int j = -radius; j <= radius; j++) {
            if (j*j + i*i <= radius*radius) {
                int sx = x + j + 1;
                int sy = y + i + 1;
                uint32_t bg = GOP_FRAMEBUFFER[sy * GOP_PIXELS_PER_SCANLINE + sx];
                
                 
                uint32_t r = ((bg >> 16) & 0xFF) * 60 / 100;
                uint32_t g = ((bg >> 8) & 0xFF) * 60 / 100;
                uint32_t b = (bg & 0xFF) * 60 / 100;
                GOP_FRAMEBUFFER[sy * GOP_PIXELS_PER_SCANLINE + sx] = (r << 16) | (g << 8) | b;
            }
        }
    }

     
    for (int i = -radius; i <= radius; i++) {
        for (int j = -radius; j <= radius; j++) {
            if (j*j + i*i <= (radius-1)*(radius-1)) {
                 
                uint32_t final_color = color;
                if (i > 0) {
                    uint32_t r = ((color >> 16) & 0xFF) * 90 / 100;
                    uint32_t g = ((color >> 8) & 0xFF) * 90 / 100;
                    uint32_t b = (color & 0xFF) * 90 / 100;
                    final_color = (r << 16) | (g << 8) | b;
                }
                GOP_FRAMEBUFFER[(y + i) * GOP_PIXELS_PER_SCANLINE + (x + j)] = final_color;
            }
        }
    }
}

void draw_mouse_cursor(int x, int y) {
     
    for (int i = 0; i < M_H; i++) {
        for (int j = 0; j < M_W; j++) {
            
             
            if (mouse_cursor_map[i][j] != 0) {
                
                 
                 
                int sx = x + j + 1; 
                int sy = y + i + 1;

                if (sx < MAX_COLS_PIXELS && sy < MAX_HEIGHT) {
                    uint32_t bg = GOP_FRAMEBUFFER[sy * GOP_PIXELS_PER_SCANLINE + sx];
                    
                     
                    uint32_t r = ((bg >> 16) & 0xFF) * 65 / 100;
                    uint32_t g = ((bg >> 8) & 0xFF) * 65 / 100;
                    uint32_t b = (bg & 0xFF) * 65 / 100;
                    
                    GOP_FRAMEBUFFER[sy * GOP_PIXELS_PER_SCANLINE + sx] = (r << 16) | (g << 8) | b;
                }
            }
        }
    }

     
    for (int i = 0; i < M_H; i++) {
        for (int j = 0; j < M_W; j++) {
            uint32_t color;
            if (mouse_cursor_map[i][j] == 1) color = 0x000000;       
            else if (mouse_cursor_map[i][j] == 2) color = 0xFFFFFF;  
            else continue;

            int px = x + j;
            int py = y + i;
            if (px < MAX_COLS_PIXELS && py < MAX_HEIGHT) {
                GOP_FRAMEBUFFER[py * GOP_PIXELS_PER_SCANLINE + px] = color;
            }
        }
    }
}
 

 
 


 
 
void refresh_screen() {
     
    clear_screen(); 

     
     
    draw_alpha_x_final_aqua(terminal_win.x, terminal_win.y, terminal_win.w, terminal_win.h, terminal_win.bg_color);

     
    global_cursor_x = terminal_win.x + 12;
    global_cursor_y = terminal_win.y + 45;
}
void handle_ps2_mouse() {
     
while (inb(0x64) & 0x01) {
    uint8_t status = inb(0x64);
    uint8_t data = inb(0x60);

    if (!(status & 0x20)) continue;

    if (mouse_phase == 0 && !(data & 0x08)) {
         
         
        while (inb(0x64) & 0x01) inb(0x60); 
        mouse_phase = 0;
        continue; 
    }

        mouse_data[mouse_phase++] = data;

        if (mouse_phase == 3) {
        mouse_phase = 0;
        
         
        clear_mouse_cursor(mouse_x, mouse_y);

         
        int rel_x = mouse_data[1];
        int rel_y = mouse_data[2];
        if (mouse_data[0] & 0x10) rel_x -= 256;
        if (mouse_data[0] & 0x20) rel_y -= 256;

        mouse_x += rel_x;
        mouse_y -= rel_y;

         

         
bool left_button = (mouse_data[0] & 1);

if (left_button) {
    if (!is_dragging) {
         
         
        if (is_on_title_bar(mouse_x, mouse_y, &terminal_win)) {
            focused_window = &terminal_win;
            is_dragging = true;
            drag_offset_x = mouse_x - focused_window->x;
            drag_offset_y = mouse_y - focused_window->y;
        } 
         
    }

     
    if (is_dragging && focused_window != NULL) {
        focused_window->x = mouse_x - drag_offset_x;
        focused_window->y = mouse_y - drag_offset_y;

         
        if (focused_window->y < 0) focused_window->y = 0;

         
        refresh_screen(); 
    }
} else {
    is_dragging = false;
    focused_window = NULL;
}

			 
			backup_mouse_area(mouse_x, mouse_y);
			draw_mouse_cursor(mouse_x, mouse_y);
		}
	}
}
void backup_mouse_area(int x, int y) {
    for (int i = 0; i < 22; i++) {
        for (int j = 0; j < 14; j++) {
            int px = x + j;
            int py = y + i;
            if (px < MAX_COLS_PIXELS && py < MAX_HEIGHT) {
                mouse_backup[i * 14 + j] = GOP_FRAMEBUFFER[py * GOP_PIXELS_PER_SCANLINE + px];
            }
        }
    }
}

void clear_mouse_cursor(int x, int y) {
    for (int i = 0; i < 22; i++) {
        for (int j = 0; j < 14; j++) {
            int px = x + j;
            int py = y + i;
            if (px < MAX_COLS_PIXELS && py < MAX_HEIGHT) {
                GOP_FRAMEBUFFER[py * GOP_PIXELS_PER_SCANLINE + px] = mouse_backup[i * 14 + j];
            }
        }
    }
}
 
void set_idt_gate(int vector, uint64_t handler, uint16_t selector, uint8_t attributes) {
    idt[vector].offset_low  = (uint16_t)(handler & 0xFFFF);
    idt[vector].selector    = selector;
    idt[vector].ist         = 0;
    idt[vector].types_attr  = attributes;
    idt[vector].offset_mid  = (uint16_t)((handler >> 16) & 0xFFFF);
    idt[vector].offset_high = (uint32_t)((handler >> 32) & 0xFFFFFFFF);
    idt[vector].reserved    = 0;
}

 
void pic_remap() {
    outb(0x20, 0x11); wait(1000);
    outb(0xA0, 0x11); wait(1000);

    outb(0x21, 0x20); wait(1000); 
    outb(0xA1, 0x28); wait(1000); 

    outb(0x21, 0x04); wait(1000);
    outb(0xA1, 0x02); wait(1000);

    outb(0x21, 0x01); wait(1000);
    outb(0xA1, 0x01); wait(1000);

     
     
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
}

 
void init_idt() {
     
    pic_remap();

    idtr.limit = (sizeof(struct idt_entry_t) * 256) - 1;
    idtr.base  = (uint64_t)&idt;

     
    for(int i = 0; i < 256; i++) {
        set_idt_gate(i, (uint64_t)irq12_mouse_wrapper, 0x08, 0x8E);
    }

     
    set_idt_gate(0x21, (uint64_t)irq1_keyboard_wrapper, 0x08, 0x8E);
    set_idt_gate(0x2C, (uint64_t)irq12_mouse_wrapper, 0x08, 0x8E);

     
    __asm__ volatile("lidt %0" : : "m"(idtr));

     
    outb(0x21, 0x00);  
    outb(0xA1, 0x00);  

     
    __asm__ volatile("sti");
}
void thewakeup(unsigned long addr) { 
    uint32_t *tags = (uint32_t *)(addr + 8);
    while (tags[0] != 0) {
        if (tags[0] == 8) {
            GOP_FRAMEBUFFER = (uint32_t *)*((unsigned long long *)&tags[2]);
            GOP_PIXELS_PER_SCANLINE = tags[5];
            break;
        }
        tags += (tags[1] + 7) / 8 * 2;
    }
	if (GOP_FRAMEBUFFER == 0) {
        GOP_FRAMEBUFFER = (uint32_t*)0xFD000000;
        GOP_PIXELS_PER_SCANLINE = 1024;
    }
	clear_screen();
	kprint("[LOG] gop services is running\n", WHITE_ON_BLACK);
	init_fs();
	kprint("[LOG] filesystem is online\n", WHITE_ON_BLACK);
	mouse_init();
	kprint("[LOG] mouse driver setted\n", WHITE_ON_BLACK);
	kprint("[LOG] exiting loader\n", WHITE_ON_BLACK);
	kprint("[LOG] jumping to graphics mode\n", WHITE_ON_BLACK);
	clear_screen();
	stguimode();
}
void kmain(unsigned long addr) {
    thewakeup(addr);  

     
    init_idt();

     
    backup_mouse_area(mouse_x, mouse_y);
    draw_mouse_cursor(mouse_x, mouse_y);

     
     
     
     
    while(1) {
        __asm__ volatile("hlt");
    }
}
 
void kbd_interrupt_handler() {
    uint8_t status = inb(0x64);
    if (status & 0x01) {
        uint8_t scancode = inb(0x60);
         
        (void)scancode; 
    }
     
    outb(0x20, 0x20);
}
 
void process_mouse_byte(uint8_t data) {
    switch(mouse_phase) {
        case 0:
            if (data & 0x08) {  
                mouse_data[0] = data;
                mouse_phase++;
            }
            break;
        case 1:
            mouse_data[1] = data;
            mouse_phase++;
            break;
        case 2:
            mouse_data[2] = data;
            mouse_phase = 0;

             
            clear_mouse_cursor(mouse_x, mouse_y);

             
            int8_t rel_x = (int8_t)mouse_data[1];
            int8_t rel_y = (int8_t)mouse_data[2];

             
            mouse_x += rel_x;
            mouse_y -= rel_y;  

             
            if (mouse_x < 0) mouse_x = 0;
            if ((uint32_t)mouse_x >= MAX_COLS_PIXELS) mouse_x = MAX_COLS_PIXELS - 1;
            if (mouse_y < 0) mouse_y = 0;
            if ((uint32_t)mouse_y >= MAX_HEIGHT) mouse_y = MAX_HEIGHT - 1;

             
            backup_mouse_area(mouse_x, mouse_y);
            draw_mouse_cursor(mouse_x, mouse_y);
            break;
    }
}
 
void mouse_interrupt_handler() {
    uint8_t status = inb(0x64);
    
     
    if ((status & 0x01) && (status & 0x20)) {
        uint8_t data = inb(0x60);
        
         
        process_mouse_byte(data);
    }
    
     
    outb(0xA0, 0x20);  
    outb(0x20, 0x20);  
}
