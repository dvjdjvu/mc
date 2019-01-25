#include <panel.h>
#include <string.h>
#include <menu.h>
#include <stdio.h>
#include <dirent.h>

#include <ustat.h>
#include <stdarg.h>
#include <openssl/md5.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/param.h>

#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <stdlib.h>
#include <curses.h>

#include <unistd.h>

#include <time.h>
#include <utime.h>

#include <dlfcn.h>

#include <locale.h>


#include "standart.h"

#define BUFSIZE 512
#define PERM_FILE (S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)

/**
 *  LINES и COLS зарезервированны, содержат размер окна
 */
#define Man_LINES LINES - 4
#define Man_COLS  COLS  - 5
#define MLINES    LINES - 4

#define SIZE 4

#define KLEN 0
#define PNKI 1

/**
 *  KEY
 */
#define ESC 0x1B
#define K5 0xD9
#define K4 0xC9
#define K6 0xDA
#define K7 0xDD
#define K12 0xA

#define UP 0x36
#define DOWN 0x37

#define KEY_COPY_PNKI 123456

int KLEN_PNKI = KLEN;


extern int daylight;
extern long timezone;
extern char *tzname[2];


void dir_init_wins(WINDOW **wins, int index, int y, int x, char *text);
void dir_win_show(WINDOW *win, char *label, int label_color);

void help_init_wins(WINDOW **win, int index, int y, int x, char *text);
void help_win_show(WINDOW *win);

void exit_init_wins(WINDOW **win, int index, int y, int x, char *text);
void exit_win_show(WINDOW *win);

void copy_init_wins(WINDOW **win, int index, int y, int x, char *text);
void copy_win_show(WINDOW *win);

void manager_init_wins(WINDOW **win, int index, int y, int x, char *text);
void manager_win_show(WINDOW *win);

void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string, chtype color);

void init_menu(const char * _dir);

int sel(struct dirent * d);
int versionsort(const void *a, const void *b);

typedef struct PANEL_DATA {
    bool on; // TRUE if panel is hidden
    int index; // Номер панели
} PANEL_DATA;


#define count_dir 4
#define count_ext 4

char *menu[count_dir] = {
    "Протоколы",
    "ТМИ",
    "Сообщения",
    "КСТР"
};

char *dir[count_dir] = {
    "PROTOCOL.XXX/",
    "PROTOCOL.TMI/",
    "PROTOCOL.SBB/",
    "FFFF/"
};

char *extension[count_dir] = {
    ".p",
    ".t",
    ".s",
    "."
};

char char_pnki[] = "Содержимое ПНКИ";
char char_klen[] = "Содержимое КЛЕН";

const char exit_str[] = "   Выйти из программы?    K4 - Да   K7 - Нет   ";
const char copy_str[] = "   Скопировать - K4  Отменить - K1   ";

double* size_files;

int n_choices;

ITEM **my_items;

MENU *my_menu;

WINDOW *_dir_wins[SIZE + 4];
WINDOW *help_wins;
WINDOW *exit_wins;
WINDOW *copy_wins;
WINDOW *win;

PANEL *_dir_panels[SIZE + 4];
PANEL *help_panels;
PANEL *exit_panels;
PANEL *copy_panels;
PANEL *top;
PANEL_DATA panel_datas[SIZE + 4];

#define buf_size 200

char **buf;

struct statvfs fs_pnki;
struct statvfs fs_klen;

#define MBYTE (1024*1024)
#define KBYTE (1024)

int argc;
char** argv;

bool *value;

struct dirent **entry;
int entry_count = 0;

int copy_on_off = 1;

void sig_winch(int signo) {
    struct winsize size;
    ioctl(fileno(stdout), TIOCGWINSZ, (char *) &size);
    resizeterm(size.ws_row, size.ws_col);
}

bool start = TRUE;

char *dir_time;

int main(int _argc, char* _argv[]) {

    int c, i, ch, ch1, ch2, index;
    int ch_on = 0;
    int y = 0, x = 0, insert = 2;
    int lines, cols, count, pid;
    double size;

    int open_file;
    DIR *open_dir;

    char *from, *to;
    unsigned char *md_buf_file2;

    // Переменные для папки копирования.

    FILE *fd;
    unsigned char *ser_numb;

    int dev_ou0 = 0, dev_ou1 = 0, AdrOU = 0;

    time_t t_time;
    struct tm time_t;
    int dir_size;

    setlocale(LC_CTYPE, "ru_RU.UTF8");

    argc = _argc;
    argv = (char **) calloc(sizeof (char), _argc);
    for (i = 0; i < _argc; i++) {
        argv[i] = (char *) calloc(sizeof (char), (strlen(_argv[i]) + 1));
        strcpy(argv[i], _argv[i]);
    }

    tzset();

    // Определение серийного номера КЛЕНа. /s_numb/ser_numb.txt

    ser_numb = (unsigned char*) calloc(sizeof (unsigned char), 8); //8-и значный цифровой номер.

    memset(ser_numb, '0', 8);

    if (fd = fopen("/s_numb/ser_numb.txt", "r")) {
        if (fread(ser_numb, sizeof (unsigned char), 8, fd)) {
        }
        fclose(fd);
    }

    // Определение номера Оконечного Устройства.

    if (dev_ou0 = mkio_open("/dev/manchester0")) {
        if (dev_ou1 = mkio_open("/dev/manchester1")) {
            if (rele_inp_detect(dev_ou0))
                AdrOU = 4;
            else
                AdrOU = 2;
        }
    }

    // Определение сегодняшней даты.

    time(&t_time);

    time_t = *localtime(&t_time);

    dir_size = strlen(argv[2]) + 255;
    dir_time = (char *) calloc(sizeof (char), dir_size);

    // Результат: Каталог - Серийный_номер_клена-номер_ОУ-дд.мм.гггг

    snprintf(dir_time, dir_size, "%s%s-%d-%.2d.%.2d.%.4d/", argv[2], ser_numb, AdrOU, time_t.tm_mday, time_t.tm_mon + 1, 1900 + time_t.tm_year);

    free(ser_numb);


    lines = Man_LINES;
    cols = Man_COLS;

    // инициализация ncurses

    /*
        Чтобы использовать экранный пакет, программы должны знать 
        характеристики терминала, и должно быть выделено пространство для curscr и stdscr. 
        Функция initscr() заботится об этих вещах.
     */
    initscr();

    signal(SIGWINCH, sig_winch);

    // читать один символ за раз, не ждать \n
    cbreak();
    // отключаем отображение символов, вводимых с клавиатуры
    noecho();
    // разрешить поддержку отображения функциональных клавиш
    keypad(stdscr, TRUE);
    // мигающий курсор, 0 - выключить, 1 - включить.
    curs_set(0);

    // инициализация цветовой палитры
    if (!has_colors()) {
        endwin();
        printf("\nОшибка! Не поддерживаются цвета! Работа программы невозможна!\n");
        return 1;
    } else {
        // разрешить назначение цветов
        start_color();
    }

    if (argc < 3) {
        mvprintw(1, 1, "Введены не все аргументы запуска программы.");
        mvprintw(2, 2, "Синтаксис запуска программы.");
        mvprintw(3, 3, "%s откуда_копировать куда_копировать\n", argv[0]);
        mvprintw(6, 4, "Нажмите любую клавишу для выключения ПК КЛЕН");
        if (getch()) {
            endwin();
            system("halt");
        }
        exit(0);
    }

    if (!(open_dir = opendir(argv[1]))) {
        mvprintw(1, 1, "Каталог %s отсутствует на ПК КЛЕН.\n", argv[1]);
        mvprintw(6, 1, "Нажмите любую клавишу для выключения ПК КЛЕН");
        if (getch()) {
            endwin();
            system("halt");
        }
        exit(0);
    } else {
        //closedir(argv[1]);
    }

    // инициализация наборов цвета, цвет шрифта, цвет фона
    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    init_pair(2, COLOR_WHITE, COLOR_CYAN);
    init_pair(3, COLOR_WHITE, COLOR_GREEN);
    init_pair(4, COLOR_WHITE, COLOR_BLACK);
    init_pair(5, COLOR_BLACK, COLOR_WHITE);
    init_pair(6, COLOR_BLACK, COLOR_GREEN);
    init_pair(7, COLOR_GREEN, COLOR_WHITE);
    init_pair(8, COLOR_BLUE, COLOR_WHITE);
    init_pair(9, COLOR_BLACK, COLOR_GREEN);
    init_pair(10, COLOR_MAGENTA, COLOR_GREEN);
    init_pair(11, COLOR_BLUE, COLOR_GREEN);

    // установить атрибуты фона для окна stdscr
    // устанавливаем цвет
    bkgdset(COLOR_PAIR(1));
    // очистить окно stdscr
    clear();
    // синхронизовать буфер
    refresh();
    // обновление и прорисовка
    //update_panels();

    md_buf_file2 = (unsigned char *) malloc((MD5_DIGEST_LENGTH * 2 + 1) * sizeof (unsigned char));
    md5(argv[0], md_buf_file2);

    // вывод подсказки на экране
    attron(COLOR_PAIR(8));
    mvprintw(LINES - 1, 0, "                                    СПО КИ ВКС=%s\t", md_buf_file2);
    attroff(COLOR_PAIR(8));
    attron(COLOR_PAIR(5));
    mvprintw(LINES - 1, 0, "(K4)-Помощь (K1)-Выход из программы ");
    attroff(COLOR_PAIR(5));

    free(md_buf_file2);

    // инициализируются окна
    for (i = 0; i < SIZE; i++) {
        dir_init_wins(_dir_wins, i, y, x, menu[i]);
        x += COLS / 4;
        panel_datas[i].index = i;
    }

    help_init_wins(_dir_wins, SIZE, 2, 3, "");
    panel_datas[SIZE].index = SIZE;
    panel_datas[SIZE].on = FALSE;

    manager_init_wins(_dir_wins, SIZE + 1, 2, 3, "");
    panel_datas[SIZE + 1].index = SIZE + 1;
    panel_datas[SIZE + 1].on = FALSE;

    exit_init_wins(_dir_wins, SIZE + 2, LINES / 2 - 3, COLS / 2 - strlen(exit_str) / 2, "");
    panel_datas[SIZE + 2].index = SIZE;
    panel_datas[SIZE + 2].on = FALSE;

    copy_init_wins(_dir_wins, SIZE + 3, LINES / 2 - 3, COLS / 2 - strlen(copy_str) / 2, "");
    panel_datas[SIZE + 3].index = SIZE;
    panel_datas[SIZE + 3].on = FALSE;

    // Create the window to be associated with the menu
    keypad(_dir_wins[SIZE + 1], TRUE);

    wnoutrefresh(_dir_wins[SIZE + 1]);

    // создание панелей на основе созданных окон
    for (i = 0; i < SIZE + 4; i++) {
        _dir_panels[i] = new_panel(_dir_wins[i]);
    }

    hide_panel(_dir_panels[SIZE]);
    hide_panel(_dir_panels[SIZE + 1]);
    hide_panel(_dir_panels[SIZE + 2]);
    hide_panel(_dir_panels[SIZE + 3]);

    // т.к. панель с индексом SIZE создавалась последней 
    // значит она и будет верхней
    top = _dir_panels[SIZE - 1];
    index = SIZE - 1;

    for (i = 0; i < SIZE; i++) {
        if (i + 1 < SIZE)
            set_panel_userptr(_dir_panels[i], _dir_panels[i + 1]);
        else
            set_panel_userptr(_dir_panels[i], _dir_panels[0]);
    }


    manager_win_show(_dir_wins[SIZE + 1]);

    if (KLEN_PNKI == KLEN) {
        wattron(_dir_wins[SIZE + 1], COLOR_PAIR(9));
        mvwprintw(_dir_wins[SIZE + 1], 1, 5, char_klen);
        wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(9));
    } else {
        wattron(_dir_wins[SIZE + 1], COLOR_PAIR(9));
        mvwprintw(_dir_wins[SIZE + 1], 1, 5, char_pnki);
        wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(9));
    }

    if (start == TRUE)
        start = FALSE;
    else {
        free(size_files);
        for (i = 0; i < n_choices; i++)
            free_item(my_items[i]);
        free(my_items);
        free(value);
        for (i = 0; i < n_choices; i++)
            free(buf[i]);
        free(buf);
        //unpost_menu(my_menu);
        free_menu(my_menu);
    }

    show_panel(_dir_panels[SIZE + 1]);

    // получение окна связанного с панелью, с которого мы уходим
    win = panel_window(top);
    // обнуляем цвет смененного окна
    wbkgdset(win, COLOR_PAIR(5));
    dir_win_show(win, menu[index], 5);

    top = (PANEL *) panel_userptr(_dir_panels[index]);
    top_panel(top);


    if (index == SIZE - 1) {
        panel_datas[index].on = FALSE;
        index = 0;
        panel_datas[index].on = TRUE;
    } else {
        panel_datas[index].on = FALSE;
        index++;
        panel_datas[index].on = TRUE;
    }


    init_menu(dir[index]);

    // получение окна связанного с панелью, на которое мы переходим
    win = panel_window(top);

    // выделяем выбранное окно цветом
    wbkgdset(win, COLOR_PAIR(4));
    dir_win_show(win, menu[index], 4);

    value = (bool *) calloc(sizeof (bool), item_count(my_menu));


    lines = Man_LINES;
    cols = Man_COLS;

    wattron(_dir_wins[SIZE + 1], COLOR_PAIR(11));
    mvwprintw(_dir_wins[SIZE + 1], lines - 7, 5,
            "Файл номер %d, всего файлов %d.\t\t",
            item_index(current_item(my_menu)) + 1, item_count(my_menu));
    wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(11));

    update_panels();
    doupdate();

    // Бесконечный цикл, пока не будет принудительное завершение.
    while ((ch = getch()) != 5555) {

        if (ch_on == 0) {
            ch1 = ch;
            ch_on = 1;
        } else {
            ch2 = ch;
            ch_on = 0;
        }

        if (ch == KEY_RIGHT && panel_datas[SIZE].on != TRUE &&
                panel_datas[SIZE + 2].on != TRUE && panel_datas[SIZE + 3].on != TRUE) {

            // Переход на панель справа от текущей.

            // устанавливаем указатели на следующее окно
            // для перехода при нажатии Tab на следующее окно
            for (i = 0; i < SIZE; i++) {
                if (i + 1 < SIZE)
                    set_panel_userptr(_dir_panels[i], _dir_panels[i + 1]);
                else
                    set_panel_userptr(_dir_panels[i], _dir_panels[0]);
            }

            manager_win_show(_dir_wins[SIZE + 1]);

            if (KLEN_PNKI == KLEN) {
                wattron(_dir_wins[SIZE + 1], COLOR_PAIR(9));
                mvwprintw(_dir_wins[SIZE + 1], 1, 5, char_klen);
                wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(9));
            } else {
                wattron(_dir_wins[SIZE + 1], COLOR_PAIR(9));
                mvwprintw(_dir_wins[SIZE + 1], 1, 5, char_pnki);
                wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(9));
            }

            if (start == TRUE)
                start = FALSE;
            else {
                free(size_files);
                for (i = 0; i < n_choices; i++)
                    free_item(my_items[i]);
                free(my_items);
                free(value);
                for (i = 0; i < n_choices; i++)
                    free(buf[i]);
                free(buf);
                //unpost_menu(my_menu);
                free_menu(my_menu);
            }

            show_panel(_dir_panels[SIZE + 1]);

            // получение окна связанного с панелью, с которого мы уходим
            win = panel_window(top);
            // обнуляем цвет смененного окна
            wbkgdset(win, COLOR_PAIR(5));
            dir_win_show(win, menu[index], 5);

            top = (PANEL *) panel_userptr(_dir_panels[index]);
            top_panel(top);

            if (index == SIZE - 1) {
                panel_datas[index].on = FALSE;
                index = 0;
                panel_datas[index].on = TRUE;
            } else {
                panel_datas[index].on = FALSE;
                index++;
                panel_datas[index].on = TRUE;
            }

            init_menu(dir[index]);

            // получение окна связанного с панелью, на которое мы переходим
            win = panel_window(top);

            // выделяем выбранное окно цветом
            wbkgdset(win, COLOR_PAIR(4));
            dir_win_show(win, menu[index], 4);

            value = (bool *) calloc(sizeof (bool), item_count(my_menu));


            lines = Man_LINES;
            cols = Man_COLS;

            wattron(_dir_wins[SIZE + 1], COLOR_PAIR(11));
            mvwprintw(_dir_wins[SIZE + 1], lines - 7, 5,
                    "Файл номер %d, всего файлов %d.\t\t",
                    item_index(current_item(my_menu)) + 1, item_count(my_menu));
            wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(11));

            usleep(100);
            update_panels();
            usleep(100);
            doupdate();
        } else if (ch == KEY_LEFT && panel_datas[SIZE].on != TRUE &&
                panel_datas[SIZE + 2].on != TRUE && panel_datas[SIZE + 3].on != TRUE) {

            // Переход на панель слева от текущей.

            // устанавливаем указатели на следующее окно
            // для перехода при нажатии Tab на следующее окно
            for (i = SIZE - 1; 0 <= i; i--) {
                if (0 < i)
                    set_panel_userptr(_dir_panels[i], _dir_panels[i - 1]);
                else
                    set_panel_userptr(_dir_panels[0], _dir_panels[SIZE - 1]);
            }

            manager_win_show(_dir_wins[SIZE + 1]);

            if (KLEN_PNKI == KLEN) {
                wattron(_dir_wins[SIZE + 1], COLOR_PAIR(9));
                mvwprintw(_dir_wins[SIZE + 1], 1, 5, char_klen);
                wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(9));
            } else {
                wattron(_dir_wins[SIZE + 1], COLOR_PAIR(9));
                mvwprintw(_dir_wins[SIZE + 1], 1, 5, char_pnki);
                wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(9));
                ;
            }

            if (start == TRUE)
                start = FALSE;
            else {
                free(size_files);
                for (i = 0; i < n_choices; i++)
                    free_item(my_items[i]);
                free(my_items);
                free(value);
                for (i = 0; i < n_choices; i++)
                    free(buf[i]);
                free(buf);

                //unpost_menu(my_menu);
                free_menu(my_menu);
            }

            show_panel(_dir_panels[SIZE + 1]);

            // получение окна связанного с панелью, с которого мы уходим
            win = panel_window(top);
            // обнуляем цвет смененного окна
            wbkgdset(win, COLOR_PAIR(5));
            dir_win_show(win, menu[index], 5);

            top = (PANEL *) panel_userptr(_dir_panels[index]);
            top_panel(top);

            if (index == 0) {
                panel_datas[index].on = FALSE;
                index = SIZE - 1;
                panel_datas[index].on = TRUE;
            } else {
                panel_datas[index].on = FALSE;
                index--;
                panel_datas[index].on = TRUE;
            }

            init_menu(dir[index]);

            // получение окна связанного с панелью, на которое мы переходим
            win = panel_window(top);

            // выделяем выбранное окно цветом
            wbkgdset(win, COLOR_PAIR(4));
            dir_win_show(win, menu[index], 4);

            value = (bool *) calloc(sizeof (bool), item_count(my_menu));

            lines = Man_LINES;
            cols = Man_COLS;

            wattron(_dir_wins[SIZE + 1], COLOR_PAIR(11));
            mvwprintw(_dir_wins[SIZE + 1], lines - 7, 5,
                    "Файл номер %d, всего файлов %d.\t\t",
                    item_index(current_item(my_menu)) + 1, item_count(my_menu));
            wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(11));

            usleep(100);
            update_panels();
            usleep(100);
            doupdate();
        } else if ((ch == 'h' || ch == K5) && panel_datas[SIZE + 2].on != TRUE &&
                panel_datas[SIZE + 3].on != TRUE) {

            // Вывод посдказки на экран.

            if (panel_datas[SIZE].on == FALSE) {
                show_panel(_dir_panels[SIZE]);
                panel_datas[SIZE].on = !panel_datas[SIZE].on;
            }

            update_panels();
            doupdate();
        } else if (ch == ESC) {

            // Запуск меню выхода из программы.
            // Если запущенно меню действий над файлами или подсказка то сначало убираются эти меню.

            if (panel_datas[SIZE + 3].on == TRUE) {
                hide_panel(_dir_panels[SIZE + 3]);
                panel_datas[SIZE + 3].on = FALSE;
            } else if (panel_datas[SIZE].on == FALSE && panel_datas[SIZE + 3].on == FALSE) {
                show_panel(_dir_panels[SIZE + 2]);
                panel_datas[SIZE + 2].on = TRUE;
            } else if (panel_datas[SIZE].on == TRUE) {
                hide_panel(_dir_panels[SIZE]);
                panel_datas[SIZE].on = !panel_datas[SIZE].on;
            }

            update_panels();
            doupdate();
        } else if ((ch == K5 || ch == K7 || ch == 'r' || ch == 't') &&
                panel_datas[SIZE + 2].on == TRUE && panel_datas[SIZE + 3].on != TRUE) {

            // При запущенном меню выхода из программы при нажатии K5 произойдёт выход, при нажатии K7 возврат в программу.

            if (ch == K5 || ch == 'r') {
                unpost_menu(my_menu);
                free_menu(my_menu);

                for (i = 0; i < n_choices; ++i)
                    free_item(my_items[i]);

                free(my_items);
                free(size_files);

                endwin();

                system("clear");

                return;
            } else if (ch == K7 || ch == 't') {
                hide_panel(_dir_panels[SIZE + 2]);
                panel_datas[SIZE + 2].on = FALSE;

                update_panels();
                doupdate();
            }
        } else if (ch == KEY_DOWN && panel_datas[SIZE].on == FALSE && start == FALSE
                && panel_datas[SIZE + 2].on != TRUE && panel_datas[SIZE + 3].on != TRUE) {

            menu_driver(my_menu, REQ_DOWN_ITEM);

            lines = Man_LINES;
            cols = Man_COLS;

            wattron(_dir_wins[SIZE + 1], COLOR_PAIR(11));
            mvwprintw(_dir_wins[SIZE + 1], lines - 7, 5,
                    "Файл номер %d, всего файлов %d.\t\t",
                    item_index(current_item(my_menu)) + 1, item_count(my_menu));
            wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(11));

            update_panels();
            doupdate();
        } else if (ch == DOWN && panel_datas[SIZE].on == FALSE && start == FALSE
                && panel_datas[SIZE + 2].on != TRUE && panel_datas[SIZE + 3].on != TRUE) {

            for (i = 0; i < 10; i++)
                menu_driver(my_menu, REQ_DOWN_ITEM);

            lines = Man_LINES;
            cols = Man_COLS;

            wattron(_dir_wins[SIZE + 1], COLOR_PAIR(11));
            mvwprintw(_dir_wins[SIZE + 1], lines - 7, 5,
                    "Файл номер %d, всего файлов %d.\t\t",
                    item_index(current_item(my_menu)) + 1, item_count(my_menu));
            wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(11));

            update_panels();
            doupdate();
        } else if (ch == KEY_UP && panel_datas[SIZE].on == FALSE && start == FALSE
                && panel_datas[SIZE + 2].on != TRUE && panel_datas[SIZE + 3].on != TRUE) {

            menu_driver(my_menu, REQ_UP_ITEM);

            lines = Man_LINES;
            cols = Man_COLS;

            wattron(_dir_wins[SIZE + 1], COLOR_PAIR(11));
            mvwprintw(_dir_wins[SIZE + 1], lines - 7, 5,
                    "Файл номер %d, всего файлов %d.\t\t",
                    item_index(current_item(my_menu)) + 1, item_count(my_menu));
            wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(11));

            update_panels();
            doupdate();
        } else if (ch == UP && panel_datas[SIZE].on == FALSE && start == FALSE
                && panel_datas[SIZE + 2].on != TRUE && panel_datas[SIZE + 3].on != TRUE) {

            for (i = 0; i < 10; i++)
                menu_driver(my_menu, REQ_UP_ITEM);

            lines = Man_LINES;
            cols = Man_COLS;

            wattron(_dir_wins[SIZE + 1], COLOR_PAIR(11));
            mvwprintw(_dir_wins[SIZE + 1], lines - 7, 5,
                    "Файл номер %d, всего файлов %d.\t\t",
                    item_index(current_item(my_menu)) + 1, item_count(my_menu));
            wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(11));

            update_panels();
            doupdate();
        } else if ((ch == ' ' || ch == '\n') && panel_datas[SIZE].on == FALSE
                && start == FALSE && panel_datas[SIZE + 2].on != TRUE && panel_datas[SIZE + 3].on != TRUE) {

            // Защита от копирования с ПНКИ в КЛЕН.
            if (KLEN_PNKI == KLEN || (KLEN_PNKI == PNKI && argc >= 4 && atoi(argv[3]) == KEY_COPY_PNKI)) {

                lines = Man_LINES;
                cols = Man_COLS;
                count = 0;
                size = 0;

                menu_driver(my_menu, REQ_TOGGLE_ITEM);

                if (item_value(current_item(my_menu)) == TRUE) {
                    item_opts_off(current_item(my_menu), O_SELECTABLE);
                    value[item_index(current_item(my_menu))] = TRUE;
                } else {
                    item_opts_on(current_item(my_menu), O_SELECTABLE);
                    value[item_index(current_item(my_menu))] = FALSE;
                }

                menu_driver(my_menu, REQ_DOWN_ITEM);

                for (i = 0; i < item_count(my_menu); i++)
                    if (value[i] == TRUE) {
                        count++;
                        size += size_files[i];
                    }

                wattron(_dir_wins[SIZE + 1], COLOR_PAIR(11));
                mvwprintw(_dir_wins[SIZE + 1], lines - 7, 5,
                        "Файл номер %d, всего файлов %d.\t\t",
                        item_index(current_item(my_menu)) + 1, item_count(my_menu));
                wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(11));

                if (free_memory(argv[0]) >= size) {
                    wattron(_dir_wins[SIZE + 1], COLOR_PAIR(9));
                    mvwprintw(_dir_wins[SIZE + 1], lines - 5, cols / 10,
                            "%25s\t%d\t%4.3lf mb.\t\t\t", "Отмечено файлов         ", count, size);
                    wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(9));

                    copy_on_off = 1;
                } else {
                    wattron(_dir_wins[SIZE + 1], COLOR_PAIR(10));
                    mvwprintw(_dir_wins[SIZE + 1], lines - 5, cols / 10,
                            "Отмечено файлов больше, чем может быть скопировано\t%d\t%4.3lf mb.", count, size);
                    wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(10));

                    copy_on_off = 0;
                }

                update_panels();
                doupdate();

            }
        } else if ((ch == 'q' || ch == K6) && panel_datas[SIZE].on == FALSE
                && start == FALSE && panel_datas[SIZE + 2].on != TRUE && panel_datas[SIZE + 3].on != TRUE) {

            // Защита от копирования с ПНКИ в КЛЕН.
            if (KLEN_PNKI == KLEN || (KLEN_PNKI == PNKI && argc >= 4 && atoi(argv[3]) == KEY_COPY_PNKI)) {

                lines = Man_LINES;
                cols = Man_COLS;
                count = 0;
                size = 0;

                if (insert == 0) {
                    for (i = 0; i < item_count(my_menu); i++)
                        if (item_value(my_items[i]) == FALSE) {
                            item_opts_on(my_items[i], O_SELECTABLE);
                            value[i] = FALSE;
                        }

                    for (i = 0; i < item_count(my_menu); i++)
                        if (value[i] == TRUE) {
                            count++;
                            size += size_files[i];
                        }

                    if (free_memory(argv[0]) >= size) {
                        wattron(_dir_wins[SIZE + 1], COLOR_PAIR(9));
                        mvwprintw(_dir_wins[SIZE + 1], lines - 5, cols / 10,
                                "%25s\t%d\t%4.3lf mb.\t\t\t", "Отмечено файлов         ", count, size);
                        wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(9));

                        copy_on_off = 1;
                    } else {
                        wattron(_dir_wins[SIZE + 1], COLOR_PAIR(10));
                        mvwprintw(_dir_wins[SIZE + 1], lines - 5, cols / 10,
                                "Отмечено файлов больше, чем может быть скопировано\t%d\t%4.3lf mb.", count, size);
                        wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(10));

                        copy_on_off = 0;
                    }

                    insert++;
                } else if (insert == 1) {
                    for (i = 0; i < item_count(my_menu); i++) {
                        item_opts_off(my_items[i], O_SELECTABLE);
                        value[i] = TRUE;
                    }

                    for (i = 0; i < item_count(my_menu); i++)
                        if (value[i] == TRUE) {
                            count++;
                            size += size_files[i];
                        }

                    if (free_memory(argv[0]) >= size) {
                        wattron(_dir_wins[SIZE + 1], COLOR_PAIR(9));
                        mvwprintw(_dir_wins[SIZE + 1], lines - 5, cols / 10,
                                "%25s\t%d\t%4.3lf mb.\t\t\t", "Отмечено файлов         ", count, size);
                        wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(9));

                        copy_on_off = 1;
                    } else {
                        wattron(_dir_wins[SIZE + 1], COLOR_PAIR(10));
                        mvwprintw(_dir_wins[SIZE + 1], lines - 5, cols / 10,
                                "Отмечено файлов больше, чем может быть скопировано\t%d\t%4.3lf mb.", count, size);
                        wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(10));

                        copy_on_off = 0;
                    }

                    insert++;
                } else if (insert == 2) {
                    // Сравнение каталогов.
                    for (i = 0; i < item_count(my_menu); i++) {

                        to = (char *) calloc(sizeof (char), strlen(argv[2]) + strlen(dir[index])
                                + strlen(item_name(my_items[i])) + 4);

                        strcat(to, argv[2]);
                        strcat(to, dir[index]);
                        strcat(to, item_name(my_items[i]));

                        if ((open_file = open(to, O_RDONLY)) == -1) {
                            item_opts_off(my_items[i], O_SELECTABLE);
                            value[i] = TRUE;
                            count++;
                            size += size_files[i];
                        } else {
                            item_opts_on(my_items[i], O_SELECTABLE);
                            value[i] = FALSE;
                        }

                        close(open_file);
                        free(to);
                    }

                    if (free_memory(argv[0]) >= size) {
                        wattron(_dir_wins[SIZE + 1], COLOR_PAIR(9));
                        mvwprintw(_dir_wins[SIZE + 1], lines - 5, cols / 10,
                                "%25s\t%d\t%4.3lf mb.\t\t\t", "Отмечено файлов         ", count, size);
                        wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(9));

                        copy_on_off = 1;
                    } else {
                        wattron(_dir_wins[SIZE + 1], COLOR_PAIR(10));
                        mvwprintw(_dir_wins[SIZE + 1], lines - 5, cols / 10,
                                "Отмечено файлов больше, чем может быть скопировано\t%d\t%4.3lf mb.", count, size);
                        wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(10));

                        copy_on_off = 0;
                    }

                    insert = 0;
                }

                update_panels();
                doupdate();
            }
        } else if ((ch == 'c' || ch == K7) && panel_datas[SIZE].on == FALSE && start == FALSE && panel_datas[SIZE + 2].on != TRUE && copy_on_off == 1) {

            // Вызов меню действий над выделенными файлами.
            // Меню вызывается если выделен хотя бы 1 файл и на ПНКИ есть свободное место.
            // Меню убирается при нажатии ESC.

            count = 0;

            for (i = 0; i < item_count(my_menu); i++)
                if (value[i] == TRUE)
                    count++;

            if (count > 0) {

                show_panel(_dir_panels[SIZE + 3]);

                panel_datas[SIZE + 3].on = TRUE;

                update_panels();
                doupdate();

            }

        } else if ((ch == K5 || ch == 's') && panel_datas[SIZE + 3].on == TRUE) {

            // Копируем файлы.

            if (!(open_dir = opendir(dir_time))) {
                mkdir(dir_time, PERM_FILE);
                chmod(dir_time, 0755);
            } else
                closedir(open_dir);

            hide_panel(_dir_panels[SIZE + 3]);
            panel_datas[SIZE + 3].on = FALSE;

            lines = Man_LINES;
            cols = Man_COLS;
            count = 0;

            for (i = 0; i < item_count(my_menu); i++)
                if (value[i] == TRUE)
                    count++;

            if (count == 0) {
                update_panels();
                doupdate();
            } else {


                wattron(_dir_wins[SIZE + 1], COLOR_PAIR(9));
                mvwprintw(_dir_wins[SIZE + 1], lines - 6, cols / 10,
                        "Начинаем копирование файлов.\t\t\t");
                wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(9));

                update_panels();
                doupdate();

                sleep(1);

                for (i = 0; i < item_count(my_menu); i++)
                    if (value[i] == TRUE) {
                        if (KLEN_PNKI == KLEN) {
                            from = (char *) calloc(sizeof (char),
                                    strlen(argv[1]) + strlen(dir[index]) +
                                    strlen(item_name(my_items[i])) + 4);
                            to = (char *) calloc(sizeof (char),
                                    strlen(dir_time) + strlen(dir[index]) +
                                    strlen(item_name(my_items[i])) + 4);

                            strcat(from, argv[1]);
                            strcat(from, dir[index]);
                            strcat(from, item_name(my_items[i]));

                            strcat(to, dir_time);
                            strcat(to, dir[index]);

                            if (!(open_dir = opendir(to))) {
                                mkdir(to, PERM_FILE);
                                chmod(to, 0755);
                            } else
                                closedir(open_dir);

                            strcat(to, item_name(my_items[i]));

                            if (copy_file(from, to) == 0 && touch(from, to) == 0 && md5sum(from, to) == 0) {

                                wattron(_dir_wins[SIZE + 1], COLOR_PAIR(9));
                                mvwprintw(_dir_wins[SIZE + 1], lines - 6, cols / 10,
                                        "Файл %10s скопирован.\t\t\t",
                                        item_name(my_items[i]));
                                wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(9));

                                update_panels();
                                doupdate();

                            } else {

                                wattron(_dir_wins[SIZE + 1], COLOR_PAIR(10));
                                mvwprintw(_dir_wins[SIZE + 1], lines - 6, cols / 10,
                                        "Невозможно скопировать файл %10s\t\t\t",
                                        item_name(my_items[i]));
                                wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(10));

                                unlink(to);

                                update_panels();
                                doupdate();
                                sleep(5);

                            }

                            free(from);
                            free(to);

                        } else if (KLEN_PNKI == PNKI && argc >= 4 && atoi(argv[3]) == KEY_COPY_PNKI) {
                            from = (char *) calloc(sizeof (char),
                                    strlen(dir_time) + strlen(dir[index]) +
                                    strlen(item_name(my_items[i])) + 4);
                            to = (char *) calloc(sizeof (char),
                                    strlen(argv[1]) + strlen(dir[index]) + strlen(dir_time) +
                                    strlen(item_name(my_items[i])) + 1);

                            strcat(from, dir_time);
                            strcat(from, dir[index]);
                            strcat(from, item_name(my_items[i]));

                            strcat(to, argv[1]);
                            strcat(to, dir[index]);

                            //if(!opendir(to))
                            //	mkdir(to, PERM_FILE);

                            strcat(to, item_name(my_items[i]));

                            wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(9));
                            mvwprintw(_dir_wins[SIZE + 1], lines - 6, cols / 10,
                                    "Идет копирование файла %10s",
                                    item_name(my_items[i]));
                            wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(9));

                            update_panels();
                            doupdate();

                            copy_file(from, to);
                            touch(from, to);

                            free(from);
                            free(to);
                        }
                    }
                for (i = 0; i < item_count(my_menu); i++)
                    if (item_value(my_items[i]) == FALSE) {
                        item_opts_on(my_items[i], O_SELECTABLE);
                        value[i] = FALSE;
                    }

                manager_win_show(_dir_wins[SIZE + 1]);
                init_menu(dir[index]);

                if (KLEN_PNKI == KLEN) {
                    wattron(_dir_wins[SIZE + 1], COLOR_PAIR(9));
                    mvwprintw(_dir_wins[SIZE + 1], 1, 5, char_klen);
                    wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(9));
                } else {
                    wattron(_dir_wins[SIZE + 1], COLOR_PAIR(9));
                    mvwprintw(_dir_wins[SIZE + 1], 1, 5, char_pnki);
                    wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(9));
                    ;
                }

                wattron(_dir_wins[SIZE + 1], COLOR_PAIR(11));
                mvwprintw(_dir_wins[SIZE + 1], lines - 7, 5,
                        "Файл номер %d, всего файлов %d\t\t",
                        item_index(current_item(my_menu)) + 1, item_count(my_menu));
                wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(11));

                wattron(_dir_wins[SIZE + 1], COLOR_PAIR(9));
                mvwprintw(_dir_wins[SIZE + 1], lines - 6, cols / 10,
                        "Копирование завершено.");
                wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(9));

                update_panels();
                doupdate();
            }

        } else if ((ch == 'w' || ch == K4) && panel_datas[SIZE].on == FALSE && start == FALSE
                && panel_datas[SIZE + 2].on != TRUE && panel_datas[SIZE + 3].on != TRUE) {

            manager_win_show(_dir_wins[SIZE + 1]);

            if (KLEN_PNKI == KLEN) {
                KLEN_PNKI = PNKI;
                wattron(_dir_wins[SIZE + 1], COLOR_PAIR(9));
                mvwprintw(_dir_wins[SIZE + 1], 1, 5, char_pnki);
                wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(9));
                ;
            } else {
                KLEN_PNKI = KLEN;
                wattron(_dir_wins[SIZE + 1], COLOR_PAIR(9));
                mvwprintw(_dir_wins[SIZE + 1], 1, 5, char_klen);
                wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(9));
            }

            if (start == TRUE) {
                start = FALSE;
            } else {
                free(size_files);
                for (i = 0; i < n_choices; i++)
                    free_item(my_items[i]);
                free(my_items);
                free(value);
                for (i = 0; i < n_choices; i++)
                    free(buf[i]);
                free(buf);

                //unpost_menu(my_menu);
                free_menu(my_menu);
            }

            init_menu(dir[index]);

            wbkgdset(win, COLOR_PAIR(4));
            dir_win_show(win, menu[index], 4);

            value = (bool *) calloc(sizeof (bool), item_count(my_menu));

            wattron(_dir_wins[SIZE + 1], COLOR_PAIR(11));
            mvwprintw(_dir_wins[SIZE + 1], lines - 7, 5,
                    "Файл номер %d, всего файлов %d.\t\t",
                    item_index(current_item(my_menu)) + 1, item_count(my_menu));
            wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(11));

            update_panels();
            doupdate();
        }
    }

    /* Unpost and free all the memory taken up */

    unpost_menu(my_menu);
    free_menu(my_menu);
    for (i = 0; i < n_choices; ++i)
        free_item(my_items[i]);

    free(dir_time);
    free(size_files);

    endwin();
    return 0;
}

// вывод окошек на экран

void dir_init_wins(WINDOW **wins, int index, int y, int x, char *text) {

    if (index != SIZE - 1)
        wins[index] = newwin(1, COLS / 4, y, x);
    else
        wins[index] = newwin(1, COLS / 4 + COLS % 4, y, x);

    dir_win_show(wins[index], text, 5);
    return;
}

// отрисовка рамки и заголовка окна

void dir_win_show(WINDOW *win, char *label, int label_color) {

    int startx, starty, height, width;

    getbegyx(win, starty, startx);
    getmaxyx(win, height, width);

    wbkgdset(win, COLOR_PAIR(label_color));
    wclear(win);
    print_in_middle(win, 0, 0, width, label, COLOR_PAIR(label_color));
    return;
}

void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string, chtype color) {

    int length, x, y;
    int temp;

    if (win == NULL)
        win = stdscr;
    getyx(win, y, x);
    if (startx != 0)
        x = startx;
    if (starty != 0)
        y = starty;
    if (width == 0)
        width = 80;

    length = strlen(string);
    temp = (width - length) / 2;
    x = startx + temp;
    wattron(win, color);
    mvwprintw(win, y, x, "%s", string);
    wattroff(win, color);
    wnoutrefresh(win);
    return;
}

void help_init_wins(WINDOW **win, int index, int y, int x, char *text) {

    win[index] = newwin(LINES - 4, COLS - 5, y, x);
    help_win_show(win[index]);
    return;
}

void help_win_show(WINDOW *win) {

    int startx, starty, height, width, length;
    int del = 5, i = 5;

    getbegyx(win, starty, startx);
    getmaxyx(win, height, width);

    wbkgdset(win, COLOR_PAIR(5));
    wclear(win);

    mvwaddstr(win, height / 2 - i--, width / del, "K1            - Закрыть окно помощи.");
    mvwaddstr(win, height / 2 - i--, width / del, "K5            - Просмотр содержимого ПНКИ/КЛЕН.");
    mvwaddstr(win, height / 2 - i--, width / del, "K6            - Отметить всё,");
    mvwaddstr(win, height / 2 - i--, width / del, "                снять отметку со свего,");
    mvwaddstr(win, height / 2 - i--, width / del, "                сравнить входной и выходной каталоги.");
    mvwaddstr(win, height / 2 - i--, width / del, "K7            - Копировать данные.");
    mvwaddstr(win, height / 2 - i--, width / del, "K12 ( Enter ) - Отметить/Снять отметку с файла.");
    mvwaddstr(win, height / 2 - i--, width / del, "");
    mvwaddstr(win, height / 2 - i--, width / del, "                Файлы которые отмеченны (черным цветом),");
    mvwaddstr(win, height / 2 - i--, width / del, "                    будут скопированны (при нажатии К7).");

    box(win, 0, 0);
    wnoutrefresh(win);
    return;
}

void exit_init_wins(WINDOW **win, int index, int y, int x, char *text) {

    win[index] = newwin(3, strlen(exit_str), y, x);
    exit_win_show(win[index]);
    return;
}

void exit_win_show(WINDOW *win) {

    int startx, starty, height, width, length;

    getbegyx(win, starty, startx);
    getmaxyx(win, height, width);

    wbkgdset(win, COLOR_PAIR(4));
    wclear(win);

    mvwaddstr(win, 1, 1, exit_str);

    box(win, 0, 0);
    wnoutrefresh(win);
    return;
}

void copy_init_wins(WINDOW **win, int index, int y, int x, char *text) {

    win[index] = newwin(3, strlen(copy_str), y, x);
    copy_win_show(win[index]);
    return;
}

void copy_win_show(WINDOW *win) {

    int startx, starty, height, width, length;

    getbegyx(win, starty, startx);
    getmaxyx(win, height, width);

    wbkgdset(win, COLOR_PAIR(4));
    wclear(win);

    mvwaddstr(win, 1, 1, copy_str);

    box(win, 0, 0);
    wnoutrefresh(win);
    return;
}

void manager_init_wins(WINDOW **win, int index, int y, int x, char *text) {
    win[index] = newwin(Man_LINES, Man_COLS, y, x);
    wbkgdset(win[index], COLOR_PAIR(3));
    manager_win_show(win[index]);
    return;
}

void manager_win_show(WINDOW *win) {

    wclear(win);

    box(win, 0, 0);

    if (statvfs(argv[0], &fs_pnki) >= 0) {
        size_t size_free = (geteuid() == 0) ? fs_pnki.f_frsize * fs_pnki.f_bfree : fs_pnki.f_frsize * fs_pnki.f_bavail;
        //size_t size_free = fs_pnki.f_frsize * fs_pnki.f_bavail;
        size_t size_all = fs_pnki.f_frsize * fs_pnki.f_blocks;

        int lines = Man_LINES;
        int cols = Man_COLS;
        int mbyte = MBYTE;

        wattron(win, COLOR_PAIR(9));
        mvwprintw(win, lines - 5, cols / 10, "%25s\t0\t%4.3lf мб.", "Отмечено файлов         ", 0);
        mvwprintw(win, lines - 4, cols / 10, "%25s\t\t%.5ld мб.", "Всего места на ПНКИ     ", size_all / mbyte);
        mvwprintw(win, lines - 3, cols / 10, "%25s\t\t%.5ld мб.", "Свободного места на ПНКИ", size_free / mbyte);
        wattroff(win, COLOR_PAIR(9));

    }

    wnoutrefresh(win);

    return;
}

int sel(struct dirent * d) {
    return 1; // всегда подтверждаем
}

int versionsort(const void *a, const void *b) {
    //return strverscmp((*(const struct dirent **) a)->d_name,
    //	(*(const struct dirent **) b)->d_name);

    return strverscmp((*(const struct dirent **) b)->d_name,
            (*(const struct dirent **) a)->d_name);
}

bool init_menu_start = TRUE;

void init_menu(const char *_dir) {

    int i, n, j, m;

    struct tm time;
    struct stat file_info;

    char *file;
    char *_directory;

    int mbyte = MBYTE;

    if (KLEN_PNKI == KLEN) {
        // Ищем данные на КЛЕНе.
        _directory = (char *) calloc(sizeof (char), strlen(argv[1]) + strlen(_dir) + 1);
        strcat(_directory, argv[1]);
    } else {
        // Ищем данные на ПНКИ.
        _directory = (char *) calloc(sizeof (char), strlen(dir_time) + strlen(_dir) + 1);
        strcat(_directory, dir_time);
    }

    strcat(_directory, _dir);

    if (init_menu_start == FALSE) {
        for (i = 0; i < entry_count; i++)
            free(entry[i]);
    }

    //entry_count = n = n_choices = scandir(_directory, &entry, sel, alphasort);
    entry_count = n = n_choices = scandir(_directory, &entry, sel, versionsort);
    //entry_count = n = n_choices = scandir(_directory, &entry, sel, strcmp);

    for (m = 0; m < count_dir; m++) {
        if (strcmp(dir[m], _dir) == 0) {
            for (i = 0; i < n_choices; ++i) {
                if (strcasestr(entry[i]->d_name, extension[m]) == 0)
                    n--;
            }

            buf = (char **) calloc(sizeof (char *), n);
            for (i = 0; i < n; i++) buf[i] = (char *) calloc(buf_size, sizeof (char));

            // Create items
            my_items = (ITEM **) calloc(sizeof (ITEM *), n + 1);

            size_files = (double *) malloc(sizeof (double) * n);

            for (i = 0, j = 0; i < n_choices; ++i) {
                if (strcasestr(entry[i]->d_name, extension[m]) != 0) {
                    if (KLEN_PNKI == KLEN) {
                        file = (char *) calloc(sizeof (char), (strlen(argv[1]) + strlen(_dir) + strlen(entry[i]->d_name)) + 1);
                        strcat(file, argv[1]);
                        strcat(file, _dir);
                        strcat(file, entry[i]->d_name);
                    } else {
                        file = (char *) calloc(sizeof (char), (strlen(dir_time) + strlen(_dir) + strlen(entry[i]->d_name)) + 1);
                        strcat(file, dir_time);
                        strcat(file, _dir);
                        strcat(file, entry[i]->d_name);
                    }

                    stat(file, &file_info);

                    if ((file_info.st_mode & S_IFMT) == S_IFREG) {

                        time = *localtime(&file_info.st_mtime);

                        snprintf(buf[j], buf_size, "%4.4lf mb. %.2d.%.2d.%.4d %.2d:%.2d",
                                (double) (file_info.st_size) / (double) (mbyte), time.tm_mday, time.tm_mon + 1,
                                1900 + time.tm_year, time.tm_hour, time.tm_min);

                        size_files[j] = (double) (file_info.st_size) / (double) (mbyte);

                        my_items[j] = new_item(entry[i]->d_name, buf[j]);

                        j++;

                    }

                    free(file);
                }
                if (j == n) break;
            }
            if (n <= 0) {
                wattron(_dir_wins[SIZE + 1], COLOR_PAIR(10));
                mvwprintw(_dir_wins[SIZE + 1], 2, 5, "Папка пуста.");
                wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(10));
            }
        }
    }


    if (init_menu_start == TRUE) init_menu_start = FALSE;

    n_choices = n;

    // Создаем меню
    my_menu = new_menu((ITEM **) my_items);
    menu_opts_off(my_menu, O_ONEVALUE);

    // A_STANDOUT A_REVERSE A_BOLD
    // цвет фона меню
    set_menu_back(my_menu, COLOR_PAIR(3));
    // подсветка выбранного значения
    set_menu_fore(my_menu, COLOR_PAIR(7) | A_REVERSE);
    // цвет выбранных значений
    set_menu_grey(my_menu, COLOR_PAIR(6)); // | A_REVERSE);

    set_menu_format(my_menu, 2, 8);

    // Set main window and sub window
    set_menu_win(my_menu, _dir_wins[SIZE + 1]);
    set_menu_sub(my_menu, derwin(_dir_wins[SIZE + 1], Man_LINES - 2, Man_COLS - 4, 2, 2));

    set_menu_format(my_menu, MLINES - 9, 1);

    // Set menu mark to the string " * "
    wattron(_dir_wins[SIZE + 1], COLOR_PAIR(10));
    set_menu_mark(my_menu, " -> ");
    wattroff(_dir_wins[SIZE + 1], COLOR_PAIR(10));

    post_menu(my_menu);

    free(_directory);

    system("clear");

    return;
}

/*

Возвращаемая структура struct statvfs содержит такие поля (в частности):

    Типа long:
    f_frsize                размер блока
    f_blocks                размер файловой системы в блоках
    f_bfree                 свободных блоков (для суперпользователя)
    f_bavail                свободных блоков (для всех остальных)
    f_files                 число I-nodes в файловой системе
    f_ffree                 свободных I-nodes (для суперпользователя)
    f_favail                свободных I-nodes (для всех остальных)
    Типа char *
    f_basetype              тип файловой системы: ufs, nfs, ...

 */
