#ifndef TETRIS_HEADER_H
#define TETRIS_HEADER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#endif

#define ROWS 19
#define COLUMNS 12
#define PIECE_ROWS 4
#define PIECE_COLUMNS 4
#define NUM_PIECE_TYPES PIECE_COUNT
#define TITLE_WIDTH 30
#define TITLE_HEIGHT 25

typedef struct {
    int row;
    int column;
} position_t;

typedef struct {
    position_t position;
    char piece_shape[4][4];
    char texture;
} piece_t, *piece_ptr;

// Core Game
void start_tetris();
void game_run();

// Board
void initialize_board();
void display_board_with_piece(char board[ROWS][COLUMNS], piece_ptr piece);
void clear_full_lines();
void animate_game_over();

// Piece
void generate_random_piece_into(piece_ptr target);
int can_place_piece_at(char board[ROWS][COLUMNS], piece_ptr piece, int row, int col);
int can_piece_drop(char board[ROWS][COLUMNS], piece_ptr piece);
void rotate_piece_clockwise(piece_ptr piece);
void place_piece(char board[ROWS][COLUMNS], piece_ptr piece);
typedef enum {
    PIECE_O,
    PIECE_I,
    PIECE_T,
    PIECE_J,
    PIECE_L,
    PIECE_Z,
    PIECE_S,
    PIECE_COUNT
} piece_type_t;



// Input
int key_pressed();
char get_input_char();
void enable_input_mode();
void disable_input_mode();

// Utils
void cross_platform_sleep(int milliseconds);

#endif // TETRIS_HEADER_H

