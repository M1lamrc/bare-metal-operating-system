typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

#define VIDEO_ADDRESS 0xB8000
#define MAX_ROWS 25
#define MAX_COLS 80
#define WHITE_ON_BLACK 0x07
#define GREEN_ON_BLACK 0x0A

// --- Global Değişkenler ---
static int global_cursor_offset = 0;
static int shift_pressed = 0;
char shell_buffer[256];
int shell_ptr = 0;

// --- Temel I/O ---
void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

unsigned char inb(uint16_t port) {
    unsigned char ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// --- Klavye Tablosu ---
unsigned char scancode_to_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

// --- Yardımcı Fonksiyonlar ---
int strcmp(char *s1, char *s2) {
    int i;
    for (i = 0; s1[i] == s2[i]; i++) {
        if (s1[i] == '\0') return 0;
    }
    return s1[i] - s2[i];
}

void update_cursor(int offset) {
    offset /= 2;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(offset & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((offset >> 8) & 0xFF));
}

void clear_screen() {
    unsigned char *vidptr = (unsigned char*)VIDEO_ADDRESS;
    for (int i = 0; i < MAX_ROWS * MAX_COLS * 2; i += 2) {
        vidptr[i] = ' ';
        vidptr[i+1] = WHITE_ON_BLACK;
    }
    global_cursor_offset = 0;
    update_cursor(0);
}

void kprint(char *message, unsigned char color) {
    unsigned char *vidptr = (unsigned char*)VIDEO_ADDRESS;
    int i = 0;
    while (message[i] != '\0') {
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

// --- Shell Mantığı ---
void execute_command(char *cmd) {
    if (strcmp(cmd, "help") == 0) {
        kprint("\nKomutlar: HELP, VER, CLEAR, STG (that command [the STG] means STart Gui, that command is unavailable for real GUI mode use)", WHITE_ON_BLACK);
    } 
    else if (strcmp(cmd, "ver") == 0) {
        kprint("\nCodename: MRC v0.0.1 Beta (closed for end user distribution)", GREEN_ON_BLACK);
    }
    else if (strcmp(cmd, "clear") == 0) {
        clear_screen();
    }
    else if (strcmp(cmd, "stg") == 0) {
        kprint("\nTrying to load", GREEN_ON_BLACK);
        kprint("\n-coming soon-", WHITE_ON_BLACK);
    }
    else if (cmd[0] != '\0') {
        kprint("\nUnsupported Command.", WHITE_ON_BLACK);
    }
}

void shell_input_handler() {
    if (inb(0x64) & 0x01) { 
        unsigned char scancode = inb(0x60);
        if (scancode == 0x2A || scancode == 0x36) { shift_pressed = 1; return; }
        if (scancode == 0xAA || scancode == 0xB6) { shift_pressed = 0; return; }
        
        if (scancode == 0x0E) { // Backspace
            if (shell_ptr > 0) {
                shell_ptr--;
                global_cursor_offset -= 2;
                unsigned char *vidptr = (unsigned char*)VIDEO_ADDRESS;
                vidptr[global_cursor_offset] = ' ';
                vidptr[global_cursor_offset + 1] = WHITE_ON_BLACK;
                update_cursor(global_cursor_offset);
            }
            return;
        }
		if (scancode == 0x4B) { 
    // Satır başına (C:\> kısmına) dayandıysak daha fazla sola gitme
    // Not: Satır başı offsetini her komut geldiğinde güncellemek en sağlıklısıdır
    // Şimdilik sadece mevcut satırın başındaki prompt'u koruyalım:
    int current_line_start = (global_cursor_offset / (MAX_COLS * 2)) * (MAX_COLS * 2) + 10;
    
    if (global_cursor_offset > current_line_start) { 
        global_cursor_offset -= 2;
        update_cursor(global_cursor_offset);
    }
    return;
}

// Sağ Ok Tuşu
if (scancode == 0x4D) {
    // Sadece shell_ptr kadar sağa gitmesine izin ver
    // Yani yazdığın metnin en sonuna kadar:
    int line_start_offset = (global_cursor_offset / (MAX_COLS * 2)) * (MAX_COLS * 2) + 10;
    int max_allowed_offset = line_start_offset + (shell_ptr * 2);

    if (global_cursor_offset < max_allowed_offset) {
        global_cursor_offset += 2;
        update_cursor(global_cursor_offset);
    }
    return;
}
        if (scancode & 0x80) return;
        if (scancode == 0x1C) { // Enter
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

// --- KERNEL ENTRY POINT ---
void kmain(void) {
    clear_screen();
    kprint("-boot success-\n", GREEN_ON_BLACK);
    kprint("MRC> ", WHITE_ON_BLACK);

    while(1) {
        shell_input_handler();
    }
}
