
#include "gui.h"
#include "api.h"
#include "vga.h"
#include "vbe.h"
#include "keyboard.h"
#include "mouse.h"

static bool gui_active = false;
static int active_window = 0;
static bool window_visible[5] = {true, false, false, false, false};
static bool start_menu_open = false;
static int start_menu_selected = 0;

static char current_dir[64] = "/";
static int file_selected_index = 0;
static int file_count = 0;
static VirtualFile dir_files[12];

static int service_selected_index = 0;
static int service_count = 0;
static SystemService sys_services[10];

static const char *viewer_title = "Notepad - Dokumen";
static const char *viewer_content = "";

static char term_lines[9][56];
static int term_line_count = 0;
static char term_input[50] = "";
static int term_input_len = 0;

static void draw_window_vga(const char *title, int x, int y, int w, int h, bool focused, bool is_terminal = false);
static void window_write_text_vga(int wx, int wy, int ww, int wh, int rel_x, int rel_y, const char *text, uint8_t color);
static void refresh_file_list();
static void refresh_services_list();
static int mystrcmp(const char *s1, const char *s2);
static void term_init();
static void term_print(const char *text);
static void term_execute(const char *cmd);

static void gui_draw_window_vbe(const char *title, int x, int y, int w, int h, bool focused, bool is_terminal = false);
static void gui_render_vbe();
static void gui_render_vga();

bool is_in_gui_mode()
{
    return gui_active;
}

void gui_start()
{
    gui_active = true;
    active_window = 0;
    window_visible[0] = true;
    window_visible[1] = false;
    window_visible[2] = false;
    window_visible[3] = false;
    window_visible[4] = false;
    start_menu_open = false;
    start_menu_selected = 0;

    term_init();

    refresh_file_list();
    refresh_services_list();

    gui_render();
}

void gui_stop()
{
    gui_active = false;
    VGA::terminal.clear();
}

static void mystrcpy(char *dest, const char *src)
{
    int i = 0;
    while (src[i] != '\0')
    {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static void term_init()
{
    term_line_count = 0;
    for (int i = 0; i < 9; i++) term_lines[i][0] = '\0';
    term_print("Linux Bash Shell Emulator v3.0");
    term_print("Ketik 'help' untuk bantuan, 'neofetch' untuk info.");
    term_print("");
    term_input[0] = '\0';
    term_input_len = 0;
}

static void term_print(const char *text)
{
    if (term_line_count >= 9)
    {
        for (int i = 1; i < 9; i++)
        {
            mystrcpy(term_lines[i - 1], term_lines[i]);
        }
        term_line_count = 8;
    }

    int j = 0;
    while (text[j] != '\0' && j < 55)
    {
        term_lines[term_line_count][j] = text[j];
        j++;
    }
    term_lines[term_line_count][j] = '\0';
    term_line_count++;
}

static void term_execute(const char *cmd)
{
    char prompt_cmd[128] = "ariel@trieternalx:~$ ";
    int p_idx = 21;
    for (int i = 0; cmd[i] != '\0'; i++) prompt_cmd[p_idx++] = cmd[i];
    prompt_cmd[p_idx] = '\0';
    term_print(prompt_cmd);

    if (mystrcmp(cmd, "") == 0)
    {
        return;
    }
    else if (mystrcmp(cmd, "help") == 0)
    {
        term_print("Perintah tersedia: help, clear, neofetch, ls, uname, shutdown");
    }
    else if (mystrcmp(cmd, "clear") == 0)
    {
        term_line_count = 0;
        for (int i = 0; i < 9; i++) term_lines[i][0] = '\0';
    }
    else if (mystrcmp(cmd, "uname") == 0)
    {
        term_print("Linux trieternalx 3.0-hybrid-i686 i386 GNU/Linux");
    }
    else if (mystrcmp(cmd, "ls") == 0)
    {
        term_print("documents/   system/   games/   welcome.txt");
    }
    else if (mystrcmp(cmd, "shutdown") == 0)
    {
        term_print("Mematikan sistem via shell...");
        api_shutdown();
    }
    else if (mystrcmp(cmd, "neofetch") == 0)
    {
        term_print("   /\\_/\\     ariel@trieternalx");
        term_print("  ( o.o )    -----------------");
        term_print("   > ^ <     OS: TrieternalX-OS (VBE GUI & Window Manager)");
        term_print("  /     \\    Kernel: x86 freestanding C++");
        term_print("  \\_|_|_/    Uptime: 10 mins");
        term_print("             RAM: 24 MB / 128 MB");
    }
    else
    {
        char err_msg[80] = "bash: ";
        int e_idx = 6;
        for (int i = 0; cmd[i] != '\0' && e_idx < 60; i++) err_msg[e_idx++] = cmd[i];
        err_msg[e_idx++] = ':';
        err_msg[e_idx++] = ' ';
        const char *suffix = "command not found";
        for (int i = 0; suffix[i] != '\0'; i++) err_msg[e_idx++] = suffix[i];
        err_msg[e_idx] = '\0';
        term_print(err_msg);
    }
}

void gui_render()
{
    if (!gui_active) return;

    if (vbe_is_active())
    {
        gui_render_vbe();
    }
    else
    {
        gui_render_vga();
    }
}

static void gui_draw_window_vbe(const char *title, int x, int y, int w, int h, bool focused, bool is_terminal)
{

    vbe_draw_rect(x + 6, y + 6, w, h, 0x020617);

    uint32_t bg_color = is_terminal ? 0x090d16 : 0x1e293b;
    vbe_draw_rect(x, y, w, h, bg_color);

    uint32_t border_color = focused ? 0x3b82f6 : 0x475569;
    vbe_draw_rect(x, y, w, 2, border_color);
    vbe_draw_rect(x, y + h - 2, w, 2, border_color);
    vbe_draw_rect(x, y, 2, h, border_color);
    vbe_draw_rect(x + w - 2, y, 2, h, border_color);

    uint32_t title_bg = focused ? 0x2563eb : 0x334155;
    vbe_draw_rect(x + 2, y + 2, w - 4, 28, title_bg);

    vbe_draw_string(x + 12, y + 10, title, 0xFFFFFF);

    vbe_draw_rect(x + w - 28, y + 6, 22, 20, 0xef4444);
    vbe_draw_string(x + w - 21, y + 8, "X", 0xFFFFFF);
}

static void gui_render_vbe()
{

    vbe_clear(0x0f172a);

    vbe_draw_line(0, 0, 800, 600, 0x1e293b);
    vbe_draw_line(0, 200, 800, 500, 0x1e293b);
    vbe_draw_rect(600, 50, 150, 150, 0x131e35);

    vbe_draw_rect(30, 40, 24, 18, 0x94a3b8);
    vbe_draw_rect(33, 43, 18, 12, 0x1e293b);
    vbe_draw_rect(40, 58, 4, 4, 0x64748b);
    vbe_draw_rect(35, 62, 14, 2, 0x475569);
    vbe_draw_string(15, 70, "My Computer", 0xf8fafc);

    vbe_draw_rect(30, 122, 24, 16, 0xeab308);
    vbe_draw_rect(30, 118, 10, 4, 0xeab308);
    vbe_draw_rect(33, 124, 18, 12, 0xfef08a);
    vbe_draw_string(15, 145, "My Documents", 0xf8fafc);

    vbe_draw_rect(30, 196, 24, 18, 0x1e293b);
    vbe_draw_string(34, 201, ">_", 0x10b981);
    vbe_draw_string(10, 220, "Linux Shell", 0x10b981);

    vbe_draw_rect(30, 270, 22, 22, 0x3b82f6);
    vbe_draw_string(37, 274, "?", 0xFFFFFF);
    vbe_draw_string(15, 298, "User Guide", 0x3b82f6);

    vbe_draw_rect(0, 560, 800, 40, 0x1e293b);
    vbe_draw_rect(0, 558, 800, 2, 0x3b82f6);

    vbe_draw_rect(8, 566, 85, 28, 0x2563eb);
    vbe_draw_string(24, 574, "[ Start ]", 0xFFFFFF);

    const char *tabs[5] = {"[F1] Help", "[F2] Files", "[F3] Htop", "[F4] Notepad", "[F5] Shell"};
    int tab_x = 105;
    for (int w = 0; w < 5; w++)
    {
        if (window_visible[w])
        {
            bool focused = (w == active_window);
            uint32_t tab_bg = focused ? 0x1d4ed8 : 0x334155;
            vbe_draw_rect(tab_x, 566, 110, 28, tab_bg);
            vbe_draw_string(tab_x + 10, 574, tabs[w], 0xFFFFFF);
            tab_x += 120;
        }
    }

    vbe_draw_rect(685, 565, 105, 30, 0x0f172a);
    size_t total_mb = api_get_ram_total() / (1024 * 1024);
    size_t used_mb = api_get_ram_used() / (1024 * 1024);

    char tray_txt[20] = "[24M/128M]";
    tray_txt[1] = '0' + (used_mb / 10);
    tray_txt[2] = '0' + (used_mb % 10);
    tray_txt[8] = '0' + (total_mb / 100);
    tray_txt[9] = '0' + ((total_mb % 100) / 10);
    tray_txt[10] = '0' + (total_mb % 10);
    vbe_draw_string(695, 573, tray_txt, 0x10b981);

    for (int w = 0; w < 5; w++)
    {
        if (window_visible[w] && w != active_window)
        {
            if (w == 0) gui_draw_window_vbe(" Bantuan & Navigasi (Win95 Help) ", 180, 80, 500, 380, false);
            else if (w == 1) gui_draw_window_vbe(" File Explorer (Windows Explorer) ", 140, 100, 560, 420, false);
            else if (w == 2) gui_draw_window_vbe(" System Monitor (htop Linux) ", 160, 120, 520, 400, false);
            else if (w == 3) gui_draw_window_vbe(viewer_title, 200, 150, 440, 300, false);
            else if (w == 4) gui_draw_window_vbe(" Terminal Bash (ariel@trieternalx) ", 150, 130, 520, 360, false, true);
        }
    }

    if (window_visible[active_window])
    {
        int w = active_window;
        if (w == 0)
        {
            int wx = 180, wy = 80, ww = 500, wh = 380;
            gui_draw_window_vbe(" Bantuan & Navigasi (Win95 Help) ", wx, wy, ww, wh, true);

            vbe_draw_string(wx + 20, wy + 50, "TrieternalX-OS GUI Desktop v3.0", 0x10b981);
            vbe_draw_string(wx + 20, wy + 85, "Navigasi Keyboard:", 0xFFFFFF);
            vbe_draw_string(wx + 40, wy + 115, "[F1] Jendela Bantuan & Panduan", 0x3b82f6);
            vbe_draw_string(wx + 40, wy + 145, "[F2] Buka Windows File Explorer", 0x3b82f6);
            vbe_draw_string(wx + 40, wy + 175, "[F3] Jalankan htop System Monitor", 0x3b82f6);
            vbe_draw_string(wx + 40, wy + 205, "[F4] Buka Linux Terminal Emulator", 0x3b82f6);
            vbe_draw_string(wx + 40, wy + 235, "[F5] Buka/Tutup Start Menu", 0x3b82f6);
            vbe_draw_string(wx + 40, wy + 265, "[Esc] Keluar GUI ke CLI Mode", 0xef4444);
            vbe_draw_string(wx + 20, wy + 310, "Tutup Jendela Fokus: Tekan [Space] + [C]", 0x94a3b8);
        }
        else if (w == 1)
        {
            int wx = 140, wy = 100, ww = 560, wh = 420;
            gui_draw_window_vbe(" File Explorer (Windows Explorer) ", wx, wy, ww, wh, true);

            vbe_draw_string(wx + 20, wy + 50, "Folder: ", 0xFFFFFF);
            vbe_draw_string(wx + 90, wy + 50, current_dir, 0xeab308);
            vbe_draw_line(wx + 20, wy + 72, wx + ww - 20, wy + 72, 0x475569);

            for (int i = 0; i < file_count; i++)
            {
                int item_y = wy + 85 + i * 24;
                bool selected = (i == file_selected_index);
                uint32_t text_color = selected ? 0xFFFFFF : 0xcbd5e1;

                if (selected)
                {
                    vbe_draw_rect(wx + 20, item_y - 2, ww - 40, 22, 0x2563eb);
                }

                if (dir_files[i].is_dir)
                {
                    vbe_draw_rect(wx + 30, item_y + 2, 16, 12, 0xeab308);
                    vbe_draw_rect(wx + 30, item_y, 6, 3, 0xeab308);
                    vbe_draw_string(wx + 55, item_y + 2, dir_files[i].name, text_color);
                    vbe_draw_string(wx + 400, item_y + 2, "<FOLDER>", 0xeab308);
                }
                else
                {
                    vbe_draw_rect(wx + 30, item_y, 14, 15, 0xf8fafc);
                    vbe_draw_line(wx + 30, item_y, wx + 44, item_y, 0x94a3b8);
                    vbe_draw_string(wx + 55, item_y + 2, dir_files[i].name, text_color);

                    int bytes = dir_files[i].size;
                    char size_str[32];
                    int idx = 0;
                    if (bytes == 0) {
                        size_str[idx++] = '0';
                    } else {
                        char tmp[16];
                        int tidx = 0;
                        while(bytes > 0) {
                            tmp[tidx++] = '0' + (bytes % 10);
                            bytes /= 10;
                        }
                        for(int j = tidx - 1; j >= 0; j--) {
                            size_str[idx++] = tmp[j];
                        }
                    }
                    size_str[idx++] = ' ';
                    size_str[idx++] = 'B';
                    size_str[idx] = '\0';
                    vbe_draw_string(wx + 400, item_y + 2, size_str, 0x94a3b8);
                }
            }
            vbe_draw_string(wx + 20, wy + wh - 30, "Kontrol: [Panah] Navigasi | [Enter] Buka | [Backspace] Naik", 0x3b82f6);
        }
        else if (w == 2)
        {
            int wx = 160, wy = 120, ww = 520, wh = 400;
            gui_draw_window_vbe(" System Monitor (htop Linux) ", wx, wy, ww, wh, true);

            vbe_draw_string(wx + 20, wy + 50, "CPU [", 0xFFFFFF);
            vbe_draw_rect(wx + 60, wy + 50, 150, 12, 0x475569);
            vbe_draw_rect(wx + 60, wy + 50, 45, 12, 0x22c55e);
            vbe_draw_string(wx + 220, wy + 50, "24.8%] Tasks: 6", 0xFFFFFF);

            vbe_draw_string(wx + 20, wy + 70, "Mem [", 0xFFFFFF);
            vbe_draw_rect(wx + 60, wy + 70, 150, 12, 0x475569);
            vbe_draw_rect(wx + 60, wy + 70, (used_mb * 150) / total_mb, 12, 0x10b981);

            char ram_str[32];
            ram_str[0] = '0' + (used_mb / 10);
            ram_str[1] = '0' + (used_mb % 10);
            ram_str[2] = 'M';
            ram_str[3] = '/';
            ram_str[4] = '0' + (total_mb / 100);
            ram_str[5] = '0' + ((total_mb % 100) / 10);
            ram_str[6] = '0' + (total_mb % 10);
            ram_str[7] = 'M';
            ram_str[8] = '\0';
            vbe_draw_string(wx + 220, wy + 70, ram_str, 0xFFFFFF);

            vbe_draw_rect(wx + 20, wy + 95, ww - 40, 20, 0x334155);
            vbe_draw_string(wx + 30, wy + 98, "PID  USER      SERVICE        STATUS  DESCRIPTION", 0xFFFFFF);

            for (int i = 0; i < service_count; i++)
            {
                int item_y = wy + 125 + i * 22;
                bool selected = (i == service_selected_index);
                uint32_t text_color = selected ? 0xFFFFFF : 0xcbd5e1;

                if (selected)
                {
                    vbe_draw_rect(wx + 20, item_y - 2, ww - 40, 20, 0x2563eb);
                }

                char pid_str[4] = "10 ";
                pid_str[1] = '0' + i;
                vbe_draw_string(wx + 30, item_y, pid_str, text_color);
                vbe_draw_string(wx + 75, item_y, "root", text_color);
                vbe_draw_string(wx + 150, item_y, sys_services[i].name, text_color);

                if (sys_services[i].is_running)
                {
                    vbe_draw_string(wx + 300, item_y, "RUNNING", 0x22c55e);
                }
                else
                {
                    vbe_draw_string(wx + 300, item_y, "STOPPED", 0xef4444);
                }
            }
            vbe_draw_string(wx + 20, wy + wh - 30, "Pilih dengan [Panah], Tekan [Enter] untuk Toggle Service", 0x3b82f6);
        }
        else if (w == 3)
        {
            int wx = 200, wy = 150, ww = 440, wh = 300;
            gui_draw_window_vbe(viewer_title, wx, wy, ww, wh, true);

            int cur_y = wy + 50;
            int char_idx = 0;
            char line_buf[100];
            int line_char_idx = 0;
            while(viewer_content[char_idx] != '\0')
            {
                char c = viewer_content[char_idx];
                if (c == '\n' || line_char_idx >= 46)
                {
                    line_buf[line_char_idx] = '\0';
                    vbe_draw_string(wx + 20, cur_y, line_buf, 0xFFFFFF);
                    cur_y += 20;
                    line_char_idx = 0;
                }
                else
                {
                    line_buf[line_char_idx++] = c;
                }
                char_idx++;
            }
            if (line_char_idx > 0)
            {
                line_buf[line_char_idx] = '\0';
                vbe_draw_string(wx + 20, cur_y, line_buf, 0xFFFFFF);
            }
            vbe_draw_string(wx + 20, wy + wh - 30, "Tekan [Backspace] atau [F4] untuk tutup.", 0x3b82f6);
        }
        else if (w == 4)
        {
            int wx = 150, wy = 130, ww = 520, wh = 360;
            gui_draw_window_vbe(" Terminal Bash (ariel@trieternalx) ", wx, wy, ww, wh, true, true);

            for (int i = 0; i < term_line_count; i++)
            {
                vbe_draw_string(wx + 20, wy + 50 + i * 22, term_lines[i], 0x10b981);
            }

            char input_line[128] = "ariel@trieternalx:~$ ";
            int idx = 21;
            for (int i = 0; i < term_input_len; i++)
            {
                input_line[idx++] = term_input[i];
            }
            input_line[idx++] = '_';
            input_line[idx] = '\0';
            vbe_draw_string(wx + 20, wy + 50 + term_line_count * 22, input_line, 0xFFFFFF);
        }
    }

    if (start_menu_open)
    {
        int sx = 8, sy = 330, sw = 220, sh = 224;

        vbe_draw_rect(sx + 4, sy + 4, sw, sh, 0x020617);

        vbe_draw_rect(sx, sy, sw, sh, 0x1e293b);

        vbe_draw_rect(sx, sy, sw, 2, 0x3b82f6);
        vbe_draw_rect(sx, sy + sh - 2, sw, 2, 0x3b82f6);
        vbe_draw_rect(sx, sy, 2, sh, 0x3b82f6);
        vbe_draw_rect(sx + sw - 2, sy, 2, sh, 0x3b82f6);

        const char *menu_items[7] = {
            "1. Terminal Linux",
            "2. Windows Explorer",
            "3. htop Sys Monitor",
            "4. Win95 Help Menu",
            "--------------------",
            "5. Reboot System",
            "6. Shutdown System"
        };

        for (int i = 0; i < 7; i++)
        {
            int item_y = sy + 10 + i * 28;
            bool selected = (i == start_menu_selected);
            uint32_t text_color = selected ? 0xFFFFFF : 0xcbd5e1;

            if (i == 4)
            {
                vbe_draw_line(sx + 10, item_y + 10, sx + sw - 10, item_y + 10, 0x475569);
            }
            else
            {
                if (selected)
                {
                    vbe_draw_rect(sx + 6, item_y - 2, sw - 12, 24, 0x2563eb);
                }
                vbe_draw_string(sx + 20, item_y + 2, menu_items[i], text_color);
            }
        }
    }

    reset_mouse_cursor_vbe();
    draw_mouse_cursor_vbe(mouse_get_x(), mouse_get_y());
}

static void draw_window_vga(const char *title, int x, int y, int w, int h, bool focused, bool is_terminal)
{
    uint8_t window_bg = VGA::COLOR_LIGHT_GREY;
    uint8_t text_color = VGA::COLOR_BLACK;
    uint8_t shadow_color = (0 << 4) | 8;

    if (is_terminal)
    {
        window_bg = VGA::COLOR_BLACK;
        text_color = VGA::COLOR_WHITE;
    }

    api_draw_rect(x + 1, y + h, w, 1, ' ', shadow_color);
    api_draw_rect(x + w, y + 1, 1, h, ' ', shadow_color);

    api_draw_rect(x, y, w, h, ' ', (window_bg << 4) | text_color);

    uint8_t border_color = focused ? VGA::COLOR_WHITE : VGA::COLOR_DARK_GREY;
    uint8_t border_attr = (window_bg << 4) | border_color;

    api_draw_rect(x, y, 1, 1, (char)201, border_attr);
    api_draw_rect(x + w - 1, y, 1, 1, (char)187, border_attr);
    api_draw_rect(x, y + h - 1, 1, 1, (char)200, border_attr);
    api_draw_rect(x + w - 1, y + h - 1, 1, 1, (char)188, border_attr);

    api_draw_rect(x + 1, y, w - 2, 1, (char)205, border_attr);
    api_draw_rect(x + 1, y + h - 1, w - 2, 1, (char)205, border_attr);
    api_draw_rect(x, y + 1, 1, h - 2, (char)186, border_attr);
    api_draw_rect(x + w - 1, y + 1, 1, h - 2, (char)186, border_attr);

    uint8_t title_bg = focused ? VGA::COLOR_BLUE : VGA::COLOR_DARK_GREY;
    uint8_t title_attr = (title_bg << 4) | VGA::COLOR_WHITE;

    api_draw_rect(x + 1, y + 1, w - 2, 1, ' ', title_attr);

    uint16_t *vga_buffer = (uint16_t *)0xB8000;
    int t_len = 0;
    while (title[t_len] != '\0') t_len++;
    if (t_len > w - 10) t_len = w - 10;

    for (int i = 0; i < t_len; i++)
    {
        vga_buffer[(y + 1) * 80 + x + 2 + i] = title[i] | (title_attr << 8);
    }

    int close_x = x + w - 5;
    vga_buffer[(y + 1) * 80 + close_x]     = '[' | (title_attr << 8);
    vga_buffer[(y + 1) * 80 + close_x + 1] = 'X' | (((title_bg << 4) | VGA::COLOR_LIGHT_RED) << 8);
    vga_buffer[(y + 1) * 80 + close_x + 2] = ']' | (title_attr << 8);
}

static void window_write_text_vga(int wx, int wy, int ww, int wh, int rel_x, int rel_y, const char *text, uint8_t color)
{
    int abs_x = wx + rel_x;
    int abs_y = wy + rel_y;
    if (abs_x >= wx + ww - 1 || abs_y >= wy + wh - 1) return;

    uint16_t *vga_buffer = (uint16_t *)0xB8000;
    int len = 0;
    while (text[len] != '\0')
    {
        if (text[len] == '\n')
        {
            abs_y++;
            abs_x = wx + rel_x;
            if (abs_y >= wy + wh - 1) break;
        }
        else
        {
            if (abs_x < wx + ww - 1)
            {
                vga_buffer[abs_y * 80 + abs_x] = text[len] | (color << 8);
                abs_x++;
            }
        }
        len++;
    }
}

static void gui_render_vga()
{
    uint8_t desktop_color = (3 << 4) | 11;
    api_draw_rect(0, 0, 80, 24, ' ', desktop_color);

    uint8_t icon_color = (3 << 4) | 15;
    window_write_text_vga(0, 0, 80, 25, 2, 2, " [ ] My Computer ", icon_color);
    window_write_text_vga(0, 0, 80, 25, 2, 5, " [ ] My Documents", icon_color);
    window_write_text_vga(0, 0, 80, 25, 2, 8, " [>] Linux Terminal", (3 << 4) | 10);
    window_write_text_vga(0, 0, 80, 25, 2, 11, " [?] Bantuan (Help)", (3 << 4) | 11);

    api_draw_rect(0, 24, 80, 1, ' ', (7 << 4) | 0);
    uint16_t *vga_buffer = (uint16_t *)0xB8000;

    api_draw_rect(0, 24, 9, 1, ' ', (8 << 4) | 15);
    const char *start_txt = "[ Start ]";
    for (int i = 0; start_txt[i] != '\0'; i++)
    {
        vga_buffer[24 * 80 + i] = start_txt[i] | (((8 << 4) | 15) << 8);
    }

    api_draw_rect(0, 23, 80, 1, (char)196, (3 << 4) | 7);

    const char *tabs[5] = {"[F1] Help", "[F2] Files", "[F3] Htop", "[F4] Notepad", "[F5] Shell"};
    int tab_x = 11;
    for (int w = 0; w < 5; w++)
    {
        if (window_visible[w])
        {
            bool focused = (w == active_window);
            uint8_t tab_color = focused ? ((1 << 4) | 15) : ((8 << 4) | 7);

            int len = 0;
            while(tabs[w][len] != '\0') len++;

            api_draw_rect(tab_x, 24, len + 2, 1, ' ', tab_color);
            for(int j = 0; j < len; j++)
            {
                vga_buffer[24 * 80 + tab_x + 1 + j] = tabs[w][j] | (tab_color << 8);
            }
            tab_x += len + 4;
        }
    }

    size_t total_mb = api_get_ram_total() / (1024 * 1024);
    size_t used_mb = api_get_ram_used() / (1024 * 1024);

    char tray_txt[20] = "[24M/128M]";
    tray_txt[1] = '0' + (used_mb / 10);
    tray_txt[2] = '0' + (used_mb % 10);
    tray_txt[8] = '0' + (total_mb / 100);
    tray_txt[9] = '0' + ((total_mb % 100) / 10);
    tray_txt[10] = '0' + (total_mb % 10);

    for (int i = 0; tray_txt[i] != '\0'; i++)
    {
        vga_buffer[24 * 80 + 80 - 13 + i] = tray_txt[i] | (((8 << 4) | 10) << 8);
    }

    for (int w = 0; w < 5; w++)
    {
        if (window_visible[w] && w != active_window)
        {
            if (w == 0) draw_window_vga(" Bantuan & Navigasi (Win95 Help) ", 18, 5, 48, 14, false);
            else if (w == 1) draw_window_vga(" File Explorer (Windows Explorer) ", 12, 3, 58, 17, false);
            else if (w == 2) draw_window_vga(" System Monitor (htop Linux) ", 14, 4, 54, 17, false);
            else if (w == 3) draw_window_vga(viewer_title, 20, 6, 45, 12, false);
            else if (w == 4) draw_window_vga(" Terminal Bash (ariel@trieternalx) ", 15, 5, 52, 14, false, true);
        }
    }

    if (window_visible[active_window])
    {
        int w = active_window;
        if (w == 0)
        {
            int wx = 18, wy = 5, ww = 48, wh = 14;
            draw_window_vga(" Bantuan & Navigasi (Win95 Help) ", wx, wy, ww, wh, true);

            uint8_t text_color = (7 << 4) | 0;
            uint8_t blue_text = (7 << 4) | 1;

            window_write_text_vga(wx, wy, ww, wh, 3, 2, "TrieternalX-OS TUI Desktop v2.5", (7 << 4) | 4);
            window_write_text_vga(wx, wy, ww, wh, 3, 4, "Navigasi Keyboard:", text_color);
            window_write_text_vga(wx, wy, ww, wh, 5, 5, "[F1] Jendela Bantuan & Panduan", blue_text);
            window_write_text_vga(wx, wy, ww, wh, 5, 6, "[F2] Buka Windows File Explorer", blue_text);
            window_write_text_vga(wx, wy, ww, wh, 5, 7, "[F3] Jalankan htop System Monitor", blue_text);
            window_write_text_vga(wx, wy, ww, wh, 5, 8, "[F4] Buka Linux Terminal Emulator", blue_text);
            window_write_text_vga(wx, wy, ww, wh, 5, 9, "[F5] Buka/Tutup Start Menu", blue_text);
            window_write_text_vga(wx, wy, ww, wh, 5, 10, "[Esc] Keluar GUI ke CLI Mode", (7 << 4) | 12);

            window_write_text_vga(wx, wy, ww, wh, 3, 12, "Tutup Jendela Fokus: Tekan [Space] + [C]", text_color);
        }
        else if (w == 1)
        {
            int wx = 12, wy = 3, ww = 58, wh = 17;
            draw_window_vga(" File Explorer (Windows Explorer) ", wx, wy, ww, wh, true);

            uint8_t text_color = (7 << 4) | 0;
            uint8_t yellow_text = (7 << 4) | 6;

            window_write_text_vga(wx, wy, ww, wh, 3, 2, "Folder: ", text_color);
            window_write_text_vga(wx, wy, ww, wh, 11, 2, current_dir, yellow_text);
            window_write_text_vga(wx, wy, ww, wh, 3, 3, "----------------------------------------------------", text_color);

            for (int i = 0; i < file_count; i++)
            {
                int item_y = 4 + i;
                bool selected = (i == file_selected_index);
                uint8_t item_color = selected ? ((1 << 4) | 15) : ((7 << 4) | 0);

                api_draw_rect(wx + 2, wy + item_y, ww - 4, 1, ' ', selected ? (1 << 4) : (7 << 4));

                if (dir_files[i].is_dir)
                {
                    window_write_text_vga(wx, wy, ww, wh, 4, item_y, "[FOLDER]", selected ? ((1 << 4) | 14) : ((7 << 4) | 6));
                    window_write_text_vga(wx, wy, ww, wh, 13, item_y, dir_files[i].name, item_color);
                }
                else
                {
                    window_write_text_vga(wx, wy, ww, wh, 4, item_y, "[FILE]  ", selected ? ((1 << 4) | 10) : ((7 << 4) | 2));
                    window_write_text_vga(wx, wy, ww, wh, 13, item_y, dir_files[i].name, item_color);

                    char sz_str[10];
                    sz_str[0] = ' ';
                    int sz = dir_files[i].size;
                    int sz_idx = 1;
                    if (sz >= 100) {
                        sz_str[sz_idx++] = '0' + (sz / 100);
                        sz %= 100;
                    }
                    if (dir_files[i].size >= 10) {
                        sz_str[sz_idx++] = '0' + (sz / 10);
                        sz %= 10;
                    }
                    sz_str[sz_idx++] = '0' + sz;
                    sz_str[sz_idx++] = 'B';
                    sz_str[sz_idx] = '\0';
                    window_write_text_vga(wx, wy, ww, wh, ww - 12, item_y, sz_str, selected ? ((1 << 4) | 7) : ((7 << 4) | 8));
                }
            }

            window_write_text_vga(wx, wy, ww, wh, 3, wh - 2, "Kontrol: [Panah] Navigasi | [Enter] Buka | [Backspace] Naik", (7 << 4) | 1);
        }
        else if (w == 2)
        {
            int wx = 14, wy = 4, ww = 54, wh = 17;
            draw_window_vga(" System Monitor (htop Linux) ", wx, wy, ww, wh, true);

            uint8_t text_color = (7 << 4) | 0;

            window_write_text_vga(wx, wy, ww, wh, 3, 2, "CPU [||||||||                      24.8%] Tasks: 6", text_color);
            window_write_text_vga(wx, wy, ww, wh, 3, 3, "Mem [|||||                         24MB/128MB]", text_color);
            int filled = (24 * 16) / 128;
            api_draw_rect(wx + 8, wy + 3, 16, 1, ' ', (8 << 4));
            api_draw_rect(wx + 8, wy + 3, filled + 1, 1, '|', (8 << 4) | 10);

            window_write_text_vga(wx, wy, ww, wh, 3, 5, "PID  USER      PR  NI  SERVICE STATUS  DESCRIPTION", (8 << 4) | 15);

            for (int i = 0; i < service_count; i++)
            {
                int item_y = 6 + i;
                bool selected = (i == service_selected_index);
                uint8_t item_color = selected ? ((1 << 4) | 15) : ((7 << 4) | 0);

                api_draw_rect(wx + 2, wy + item_y, ww - 4, 1, ' ', selected ? (1 << 4) : (7 << 4));

                char pid_str[4] = " 0 ";
                pid_str[1] = '1' + i;
                window_write_text_vga(wx, wy, ww, wh, 3, item_y, pid_str, item_color);
                window_write_text_vga(wx, wy, ww, wh, 8, item_y, "root", item_color);
                window_write_text_vga(wx, wy, ww, wh, 18, item_y, sys_services[i].name, item_color);

                if (sys_services[i].is_running)
                {
                    window_write_text_vga(wx, wy, ww, wh, 37, item_y, "RUN", selected ? ((1 << 4) | 10) : ((7 << 4) | 2));
                }
                else
                {
                    window_write_text_vga(wx, wy, ww, wh, 37, item_y, "OFF", selected ? ((1 << 4) | 12) : ((7 << 4) | 4));
                }
            }

            window_write_text_vga(wx, wy, ww, wh, 3, wh - 2, "Pilih dengan [Panah], Tekan [Enter] untuk Toggle Service", (7 << 4) | 1);
        }
        else if (w == 3)
        {
            int wx = 20, wy = 6, ww = 45, wh = 12;
            draw_window_vga(viewer_title, wx, wy, ww, wh, true);

            uint8_t text_color = (7 << 4) | 0;
            window_write_text_vga(wx, wy, ww, wh, 3, 2, viewer_content, text_color);

            window_write_text_vga(wx, wy, ww, wh, 3, wh - 2, "Tekan [Backspace] atau [F4] untuk tutup.", (7 << 4) | 1);
        }
        else if (w == 4)
        {
            int wx = 15, wy = 5, ww = 52, wh = 14;
            draw_window_vga(" Terminal Bash (ariel@trieternalx) ", wx, wy, ww, wh, true, true);

            uint8_t term_text_color = (0 << 4) | 10;

            for (int i = 0; i < term_line_count; i++)
            {
                window_write_text_vga(wx, wy, ww, wh, 2, 2 + i, term_lines[i], term_text_color);
            }

            char current_input_line[128];
            mystrcpy(current_input_line, "ariel@trieternalx:~$ ");
            int c_idx = 21;
            for (int i = 0; i < term_input_len; i++)
            {
                current_input_line[c_idx++] = term_input[i];
            }
            current_input_line[c_idx++] = '_';
            current_input_line[c_idx] = '\0';

            window_write_text_vga(wx, wy, ww, wh, 2, 2 + term_line_count, current_input_line, (0 << 4) | 15);
        }
    }

    if (start_menu_open)
    {
        int sx = 0, sy = 12, sw = 25, sh = 11;
        api_draw_rect(sx, sy, sw, sh, ' ', (7 << 4) | 0);

        uint8_t s_border = (7 << 4) | 15;
        api_draw_rect(sx, sy, sw, 1, (char)205, s_border);
        api_draw_rect(sx + sw - 1, sy, 1, sh, (char)186, s_border);

        api_draw_rect(sx, sy + 1, 3, sh - 1, ' ', (1 << 4));

        const char *menu_items[7] = {
            "1. Terminal Linux",
            "2. Windows Explorer",
            "3. htop Sys Monitor",
            "4. Win95 Help Menu",
            "--------------------",
            "5. Reboot System",
            "6. Shutdown System"
        };

        for (int i = 0; i < 7; i++)
        {
            int item_y = sy + 1 + i;
            bool selected = (i == start_menu_selected);
            uint8_t item_color = selected ? ((1 << 4) | 15) : ((7 << 4) | 0);

            if (i == 4)
            {
                window_write_text_vga(sx, sy, sw, sh, 4, 1 + i, "--------------------", (7 << 4) | 8);
            }
            else
            {
                api_draw_rect(sx + 3, item_y, sw - 4, 1, ' ', selected ? (1 << 4) : (7 << 4));
                window_write_text_vga(sx, sy, sw, sh, 4, 1 + i, menu_items[i], item_color);
            }
        }
    }
}

void gui_handle_key(uint8_t scancode, char c)
{
    if (scancode == 0x3F)
    {
        start_menu_open = !start_menu_open;
        if (start_menu_open) start_menu_selected = 0;
        gui_render();
        return;
    }

    if (start_menu_open)
    {
        if (scancode == 0x50)
        {
            start_menu_selected++;
            if (start_menu_selected == 4) start_menu_selected++;
            if (start_menu_selected > 6) start_menu_selected = 0;
            gui_render();
            return;
        }
        else if (scancode == 0x48)
        {
            start_menu_selected--;
            if (start_menu_selected == 4) start_menu_selected--;
            if (start_menu_selected < 0) start_menu_selected = 6;
            gui_render();
            return;
        }
        else if (c == 27 || scancode == 0x01)
        {
            start_menu_open = false;
            gui_render();
            return;
        }
        else if (c == '\n')
        {
            start_menu_open = false;
            if (start_menu_selected == 0)
            {
                active_window = 4;
                window_visible[4] = true;
            }
            else if (start_menu_selected == 1)
            {
                active_window = 1;
                window_visible[1] = true;
                refresh_file_list();
            }
            else if (start_menu_selected == 2)
            {
                active_window = 2;
                window_visible[2] = true;
                refresh_services_list();
            }
            else if (start_menu_selected == 3)
            {
                active_window = 0;
                window_visible[0] = true;
            }
            else if (start_menu_selected == 5)
            {
                api_reboot();
            }
            else if (start_menu_selected == 6)
            {
                api_shutdown();
            }
            gui_render();
            return;
        }
    }

    if (scancode == 0x3B)
    {
        active_window = 0;
        window_visible[0] = true;
        gui_render();
        return;
    }
    else if (scancode == 0x3C)
    {
        active_window = 1;
        window_visible[1] = true;
        refresh_file_list();
        gui_render();
        return;
    }
    else if (scancode == 0x3D)
    {
        active_window = 2;
        window_visible[2] = true;
        refresh_services_list();
        gui_render();
        return;
    }
    else if (scancode == 0x3E)
    {
        if (active_window == 3)
        {
            window_visible[3] = false;
            active_window = 1;
        }
        else
        {
            active_window = 3;
            window_visible[3] = true;
        }
        gui_render();
        return;
    }
    else if (scancode == 0x01)
    {
        gui_stop();
        VGA::terminal.write("Keluar dari GUI. Kembali ke kernel shell.\n");
        VGA::terminal.write("TrieternalXOS > ");
        return;
    }

    if (window_visible[active_window])
    {
        int w = active_window;

        if (w == 1)
        {
            if (scancode == 0x50)
            {
                if (file_selected_index < file_count - 1)
                {
                    file_selected_index++;
                    gui_render();
                }
            }
            else if (scancode == 0x48)
            {
                if (file_selected_index > 0)
                {
                    file_selected_index--;
                    gui_render();
                }
            }
            else if (c == '\n')
            {
                if (file_count > 0)
                {
                    VirtualFile selected = dir_files[file_selected_index];
                    if (selected.is_dir)
                    {
                        if (mystrcmp(current_dir, "/") == 0)
                        {
                            size_t path_idx = 0;
                            current_dir[path_idx++] = '/';
                            for (int j = 0; selected.name[j] != '\0'; j++)
                            {
                                current_dir[path_idx++] = selected.name[j];
                            }
                            current_dir[path_idx] = '\0';
                        }
                        refresh_file_list();
                        file_selected_index = 0;
                        gui_render();
                    }
                    else
                    {
                        char file_full_path[128];
                        size_t p_idx = 0;
                        for (int j = 0; current_dir[j] != '\0'; j++) file_full_path[p_idx++] = current_dir[j];
                        if (file_full_path[p_idx - 1] != '/') file_full_path[p_idx++] = '/';
                        for (int j = 0; selected.name[j] != '\0'; j++) file_full_path[p_idx++] = selected.name[j];
                        file_full_path[p_idx] = '\0';

                        const char *content = api_read_file(file_full_path);
                        if (content != nullptr)
                        {
                            viewer_title = selected.name;
                            viewer_content = content;

                            active_window = 3;
                            window_visible[3] = true;
                            gui_render();
                        }
                    }
                }
            }
            else if (c == '\b')
            {
                if (mystrcmp(current_dir, "/") != 0)
                {
                    current_dir[0] = '/';
                    current_dir[1] = '\0';
                    refresh_file_list();
                    file_selected_index = 0;
                    gui_render();
                }
            }
        }
        else if (w == 2)
        {
            if (scancode == 0x50)
            {
                if (service_selected_index < service_count - 1)
                {
                    service_selected_index++;
                    gui_render();
                }
            }
            else if (scancode == 0x48)
            {
                if (service_selected_index > 0)
                {
                    service_selected_index--;
                    gui_render();
                }
            }
            else if (c == '\n')
            {
                if (service_count > 0)
                {
                    api_toggle_service(sys_services[service_selected_index].name);
                    refresh_services_list();
                    gui_render();
                }
            }
        }
        else if (w == 3)
        {
            if (c == '\b' || c == 27 || scancode == 0x01)
            {
                window_visible[3] = false;
                active_window = 1;
                gui_render();
            }
        }
        else if (w == 4)
        {
            if (c == '\n')
            {
                term_input[term_input_len] = '\0';
                term_execute(term_input);
                term_input_len = 0;
                term_input[0] = '\0';
                gui_render();
            }
            else if (c == '\b')
            {
                if (term_input_len > 0)
                {
                    term_input_len--;
                    term_input[term_input_len] = '\0';
                    gui_render();
                }
            }
            else if (c != 0 && term_input_len < 40)
            {
                term_input[term_input_len++] = c;
                term_input[term_input_len] = '\0';
                gui_render();
            }
        }
    }
}

static void refresh_file_list()
{
    file_count = api_list_files(current_dir, dir_files, 12);
}

static void refresh_services_list()
{
    service_count = api_get_services(sys_services, 10);
}

static int mystrcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}
