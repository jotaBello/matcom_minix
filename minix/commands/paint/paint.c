#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <signal.h>

/* Estructura del paquete del mouse (protocolo PS/2) */
typedef struct {
    unsigned char buttons;
    signed char dx;
    signed char dy;
} MousePacket;

#define LEFT_BTN  0x01
#define RIGHT_BTN 0x02
#define CURSOR_CHAR  "+"
#define BRUSH_CHAR   "*"
#define MOUSE_DEV    "/dev/mouse"

/* Dimensiones de la pantalla */
int rows, cols;

/* Posición actual del cursor */
int cur_x, cur_y;

/* Canvas: lo que está dibujado en pantalla */
char canvas[100][200];

struct termios orig_termios;

/* Mover el cursor de la terminal a (row, col) */
void move_to(int row, int col) {
    printf("\033[%d;%dH", row + 1, col + 1);
    fflush(stdout);
}

/* Limpiar pantalla */
void clear_screen() {
    printf("\033[2J");
    fflush(stdout);
}

/* Ocultar/mostrar cursor de terminal */
void hide_cursor() { printf("\033[?25l"); fflush(stdout); }
void show_cursor() { printf("\033[?25h"); fflush(stdout); }

/* Restaurar terminal al salir */
void cleanup(int sig) {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    show_cursor();
    clear_screen();
    move_to(0, 0);
    printf("Saliendo de paint...\n");
    exit(0);
}

/* Obtener tamaño de la terminal */
void get_terminal_size() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    rows = w.ws_row;
    cols = w.ws_col;
    if (rows <= 0) rows = 24;
    if (cols <= 0) cols = 80;
}

/* Dibujar todo el canvas */
void redraw_canvas() {
    int r, c;
    for (r = 0; r < rows; r++) {
        for (c = 0; c < cols; c++) {
            move_to(r, c);
            if (cur_x == c && cur_y == r) {
                printf("\033[7m%s\033[0m", CURSOR_CHAR); /* cursor invertido */
            } else {
                putchar(canvas[r][c]);
            }
        }
    }
    fflush(stdout);
}

/* Dibujar solo la celda (row, col) */
void draw_cell(int row, int col) {
    move_to(row, col);
    if (cur_x == col && cur_y == row) {
        printf("\033[7m%s\033[0m", CURSOR_CHAR);
    } else {
        putchar(canvas[row][col]);
    }
    fflush(stdout);
}

int main() {
    int mouse_fd;
    MousePacket pkt;
    int r, c;

    /* Señales para salir limpio */
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    /* Obtener tamaño de terminal */
    get_terminal_size();

    /* Inicializar canvas con espacios */
    for (r = 0; r < rows; r++)
        for (c = 0; c < cols; c++)
            canvas[r][c] = ' ';

    /* Posición inicial: centro */
    cur_x = cols / 2;
    cur_y = rows / 2;

    /* Modo raw para la terminal */
    struct termios raw;
    tcgetattr(STDIN_FILENO, &orig_termios);
    raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    /* Abrir el mouse */
    mouse_fd = open(MOUSE_DEV, O_RDONLY);
    if (mouse_fd < 0) {
        perror("No se pudo abrir /dev/mouse");
        cleanup(0);
    }

    /* Dibujar pantalla inicial */
    clear_screen();
    hide_cursor();
    redraw_canvas();

    /* Bucle principal */
    while (1) {
        /* Leer paquete del mouse (3 bytes PS/2) */
        int n = read(mouse_fd, &pkt, sizeof(pkt));
        if (n < (int)sizeof(pkt)) continue;

        int old_x = cur_x;
        int old_y = cur_y;

        /* Actualizar posición */
        cur_x += pkt.dx;
        cur_y -= pkt.dy; /* Y está invertido */

        /* Limitar a la pantalla */
        if (cur_x < 0) cur_x = 0;
        if (cur_x >= cols) cur_x = cols - 1;
        if (cur_y < 0) cur_y = 0;
        if (cur_y >= rows) cur_y = rows - 1;

        /* Click izquierdo: pintar */
        if (pkt.buttons & LEFT_BTN) {
            canvas[cur_y][cur_x] = '*';
        }

        /* Click derecho: borrar */
        if (pkt.buttons & RIGHT_BTN) {
            canvas[cur_y][cur_x] = ' ';
        }

        /* Redibujar celda anterior y nueva */
        draw_cell(old_y, old_x);
        draw_cell(cur_y, cur_x);
    }

    close(mouse_fd);
    cleanup(0);
    return 0;
}