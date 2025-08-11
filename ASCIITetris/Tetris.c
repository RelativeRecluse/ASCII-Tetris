#include "TetrisHeader.h"

// ─────────────────────────────────────────────────────────────
// Globals and Definitions
// ─────────────────────────────────────────────────────────────

char textures[] = {'X', '@', '#', '$', '%', '&', '+'};
char board[ROWS][COLUMNS];
char screen[TITLE_HEIGHT][TITLE_WIDTH];
piece_ptr current_piece;
piece_ptr next_piece;
piece_t piece_a, piece_b;
int drop_delay = 500;
int score = 0;

piece_t piece_type[PIECE_COUNT] = {
    [PIECE_O] = {{0, PIECE_ROWS}, {
        {' ', ' ', ' ', ' '},
        {' ', 'X', 'X', ' '},
        {' ', 'X', 'X', ' '},
        {' ', ' ', ' ', ' '}
    }},
    [PIECE_I] = {{0, PIECE_ROWS}, {
        {' ', ' ', ' ', ' '},
        {'X', 'X', 'X', 'X'},
        {' ', ' ', ' ', ' '},
        {' ', ' ', ' ', ' '}
    }},
    [PIECE_T] = {{0, PIECE_ROWS}, {
        {' ', ' ', ' ', ' '},
        {'X', 'X', 'X', ' '},
        {' ', 'X', ' ', ' '},
        {' ', ' ', ' ', ' '}
    }},
    [PIECE_J] = {{0, PIECE_ROWS}, {
        {' ', ' ', ' ', ' '},
        {'X', 'X', 'X', ' '},
        {'X', ' ', ' ', ' '},
        {' ', ' ', ' ', ' '}
    }},
    [PIECE_L] = {{0, PIECE_ROWS}, {
        {' ', ' ', ' ', ' '},
        {'X', 'X', 'X', ' '},
        {' ', ' ', 'X', ' '},
        {' ', ' ', ' ', ' '}
    }},
    [PIECE_Z] = {{0, PIECE_ROWS}, {
        {' ', ' ', ' ', ' '},
        {'X', 'X', ' ', ' '},
        {' ', 'X', 'X', ' '},
        {' ', ' ', ' ', ' '}
    }},
    [PIECE_S] = {{0, PIECE_ROWS}, {
        {' ', ' ', ' ', ' '},
        {' ', 'X', 'X', ' '},
        {'X', 'X', ' ', ' '},
        {' ', ' ', ' ', ' '}
    }}
};


// ─────────────────────────────────────────────────────────────
// Cross-platform Input Handling
// ─────────────────────────────────────────────────────────────

void cross_platform_sleep(int milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}

#ifdef _WIN32
int key_pressed() { return _kbhit(); }
char get_input_char() { return _getch(); }
void enable_input_mode() {}
void disable_input_mode() {}
void move_cursor_to(int row, int col) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = { (SHORT)col, (SHORT)row };
    SetConsoleCursorPosition(hConsole, pos);
}
#else
void enable_input_mode() {
    struct termios term;
    tcgetattr(0, &term);
    term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(0, TCSANOW, &term);
}
void disable_input_mode() {
    struct termios term;
    tcgetattr(0, &term);
    term.c_lflag |= (ICANON | ECHO);
    tcsetattr(0, TCSANOW, &term);
}
int key_pressed() {
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}
char get_input_char() { return getchar(); }


#endif


// ─────────────────────────────────────────────────────────────
// Board Logic
// ─────────────────────────────────────────────────────────────

void initialize_board() {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLUMNS; j++) {
            if (i == ROWS - 1) board[i][j] = '=';
            else if (j == 0 || j == COLUMNS - 1) board[i][j] = '|';
            else board[i][j] = ' ';
        }
    }
}

void place_piece(char board[ROWS][COLUMNS], piece_ptr piece) {
    if (!piece) return;

    for (int i = 0; i < PIECE_ROWS; i++) {
        for (int j = 0; j < PIECE_COLUMNS; j++) {
            if (piece->piece_shape[i][j] != ' ') {
                int row = piece->position.row + i;
                int col = piece->position.column + j;

                if (row >= 0 && row < ROWS && col >= 0 && col < COLUMNS)
                    board[row][col] = piece->texture;
            }
        }
    }
}

void clear_marked_lines(int lines[], int count) {
    for (int idx = 0; idx < count; idx++) {
        int row = lines[idx];
        for (int k = row; k > 0; k--) {
            for (int j = 1; j < COLUMNS - 1; j++) {
                board[k][j] = board[k - 1][j];
            }
        }
        // Clear top row after shift
        for (int j = 1; j < COLUMNS - 1; j++) {
            board[0][j] = ' ';
        }
    }

    // Original Tetris scoring
    int line_scores[] = {0, 400, 1000, 3000, 12000};
    score += line_scores[count];
}


// Find full lines and return their indices and count
int find_full_lines(int lines_to_clear[]) {
    int count = 0;
    for (int i = 0; i < ROWS - 1; i++) {
        int full = 1;
        for (int j = 1; j < COLUMNS - 1; j++) {
            if (board[i][j] == ' ') {
                full = 0;
                break;
            }
        }
        if (full) {
            lines_to_clear[count++] = i;
        }
    }
    return count;
}

// Animate blinking of full lines before clearing
void animate_line_clear(int lines[], int count) {
    const int blink_times = 3;
    const int blink_delay = 200; // milliseconds

    for (int b = 0; b < blink_times; b++) {
        // Blank out lines
        for (int i = 0; i < count; i++) {
            int row = lines[i];
            for (int j = 1; j < COLUMNS - 1; j++) {
                board[row][j] = ' ';
            }
        }

#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif
        display_board_with_piece(board, current_piece);
        cross_platform_sleep(blink_delay);

        // Restore lines (using '#' or 'X' as block fill)
        for (int i = 0; i < count; i++) {
            int row = lines[i];
            for (int j = 1; j < COLUMNS - 1; j++) {
                board[row][j] = '#';  // You can choose any char for blinking effect
            }
        }

#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif
        display_board_with_piece(board, current_piece);
        cross_platform_sleep(blink_delay);
    }
}

int paused = 0;

void toggle_pause() {
    paused = !paused;
    // Render: normal board OR with "PAUSE"
if (paused) {
    char temp_board[ROWS][COLUMNS];
    memcpy(temp_board, board, sizeof(board));

    const char* msg = "PAUSE";
    int mid_row = ROWS / 2;
    int mid_col = (COLUMNS - (int)strlen(msg)) / 2;

    // Safety check so it never draws outside bounds
    if (mid_row >= 0 && mid_row < ROWS && mid_col >= 0) {
        for (int i = 0; i < (int)strlen(msg); i++) {
            if (mid_col + i < COLUMNS) {
                temp_board[mid_row][mid_col + i] = msg[i];
            }
        }
    }

    display_board_with_piece(temp_board, current_piece);
} else {
    display_board_with_piece(board, current_piece);
}
}

void display_board(char board[ROWS][COLUMNS]) {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLUMNS; c++) {
            putchar(board[r][c]);
        }
        putchar('\n');
    }
}


// ─────────────────────────────────────────────────────────────
// Title Screen
// ─────────────────────────────────────────────────────────────

void show_title_screen() {
    char screen[TITLE_HEIGHT][TITLE_WIDTH];

    // Fill screen with spaces
    for (int i = 0; i < TITLE_HEIGHT; i++) {
        for (int j = 0; j < TITLE_WIDTH; j++) {
            screen[i][j] = ' ';
        }
    }

    // Draw borders with fixed chars '=' top/bottom, '|' sides
    for (int j = 0; j < TITLE_WIDTH; j++) {
        screen[0][j] = '=';
        screen[TITLE_HEIGHT - 1][j] = '=';
    }
    for (int i = 1; i < TITLE_HEIGHT - 1; i++) {
        screen[i][0] = '|';
        screen[i][TITLE_WIDTH - 1] = '|';
    }

    // Title text placement
    const char* title = "ASCII TETRIS";
    int title_len = strlen(title);
    int row = TITLE_HEIGHT / 2;
    int col = (TITLE_WIDTH - title_len) / 2;

    if (col < 1) col = 1;
    if (col + title_len > TITLE_WIDTH - 1) col = TITLE_WIDTH - 1 - title_len;

    for (int i = 0; i < title_len; i++) {
        screen[row][col + i] = title[i];
    }

    // Instruction prompt placement
    const char* prompt = "Press ENTER to Start";
    int prompt_len = strlen(prompt);
    int pr = row + 2;
    if (pr >= TITLE_HEIGHT - 1) pr = TITLE_HEIGHT - 2;

    int pc = (TITLE_WIDTH - prompt_len) / 2;
    if (pc < 1) pc = 1;
    if (pc + prompt_len > TITLE_WIDTH - 1) pc = TITLE_WIDTH - 1 - prompt_len;

    // Place prompt safely with debug info
    for (int i = 0; i < prompt_len; i++) {
        int pos = pc + i;
        if (pos >= 1 && pos < TITLE_WIDTH - 1) {
            screen[pr][pos] = prompt[i];
        } else {
            printf("WARNING: prompt char '%c' at %d is out of bounds\n", prompt[i], pos);
        }
    }

    printf("TITLE at row %d, col %d\n", row, col);
    printf("PROMPT at row %d, col %d\n", pr, pc);

#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif

    for (int i = 0; i < TITLE_HEIGHT; i++) {
        for (int j = 0; j < TITLE_WIDTH; j++) {
            putchar(screen[i][j]);
        }
        putchar('\n');
    }
    fflush(stdout);

    while (1) {
        if (key_pressed()) {
            char c = get_input_char();
            if (c == '\n' || c == '\r' || c == 13 || c == 10) break;
        }
        cross_platform_sleep(10);
    }
}


// ─────────────────────────────────────────────────────────────
// Piece Logic
// ─────────────────────────────────────────────────────────────

void set_current_piece_from_next() {
    current_piece = next_piece;
    current_piece->position.row = 0;
    current_piece->position.column = 4;
}

int can_piece_drop(char board[ROWS][COLUMNS], piece_ptr piece) {
    return can_place_piece_at(board, piece, piece->position.row + 1, piece->position.column);
}

int can_place_piece_at(char board[ROWS][COLUMNS], piece_ptr piece, int test_row, int test_col) {
    for (int i = 0; i < PIECE_ROWS; i++) {
        for (int j = 0; j < PIECE_COLUMNS; j++) {
            if (piece->piece_shape[i][j] != ' ') {
                int board_row = test_row + i;
                int board_col = test_col + j;

                if (board_row < 0 || board_row >= ROWS || board_col < 0 || board_col >= COLUMNS)
                    return 0;

                if (board[board_row][board_col] != ' ')
                    return 0;
            }
        }
    }
    return 1;
}

void rotate_piece_clockwise(piece_ptr piece) {
    char temp[PIECE_ROWS][PIECE_COLUMNS];
    for (int i = 0; i < PIECE_ROWS; i++)
        for (int j = 0; j < PIECE_COLUMNS; j++)
            temp[j][PIECE_COLUMNS - 1 - i] = piece->piece_shape[i][j];

    for (int i = 0; i < PIECE_ROWS; i++)
        for (int j = 0; j < PIECE_COLUMNS; j++)
            piece->piece_shape[i][j] = temp[i][j];
}

void generate_random_piece_into(piece_ptr target) {
    piece_type_t index = rand() % PIECE_COUNT;
    char texture = textures[rand() % (sizeof(textures) / sizeof(char))];

    *target = piece_type[index];
    target->position.row = 0;
    target->position.column = 4;
    target->texture = texture;

}

void draw_piece_on_board(char temp_board[ROWS][COLUMNS], const piece_t *piece) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (piece->piece_shape[r][c] != ' ') {  // non-empty block in piece shape
                int board_row = piece->position.row + r;
                int board_col = piece->position.column + c;
                if (board_row >= 0 && board_row < ROWS && board_col >= 0 && board_col < COLUMNS) {
                    temp_board[board_row][board_col] = piece->piece_shape[r][c];
                }
            }
        }
    }
}



// ─────────────────────────────────────────────────────────────
// Drawing
// ─────────────────────────────────────────────────────────────

void display_board_with_piece(char board[ROWS][COLUMNS], piece_ptr piece) {
    char score_str[7];
    snprintf(score_str, sizeof(score_str), "%06d", score);

    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLUMNS; j++) {
            char c = board[i][j];

            if (piece) {
                int pr = i - piece->position.row;
                int pc = j - piece->position.column;
                if (pr >= 0 && pr < PIECE_ROWS && pc >= 0 && pc < PIECE_COLUMNS) {
                    if (piece->piece_shape[pr][pc] != ' ') c = piece->texture;
                }
            }
            printf("%c", c);
        }

        if (i == 1) printf("   SCORE");
        else if (i == 2) printf("   %s", score_str);
        else if (i == ROWS - 7) printf("   NEXT:");
        else if (i >= ROWS - 6 && i < ROWS - 2) {
            int pr = i - (ROWS - 6);
            printf("   ");
            for (int pc = 0; pc < PIECE_COLUMNS; pc++) {
                char c = next_piece->piece_shape[pr][pc];
                printf("%c", (c != ' ') ? next_piece->texture : ' ');
            }
        }

        printf("\n");
    }
}

void animate_game_over() {
    for (int i = ROWS - 2; i >= 0; i--) {
        for (int j = 1; j < COLUMNS - 1; j++)
            board[i][j] = 'X';

#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif

        display_board_with_piece(board, NULL);
        cross_platform_sleep(50);
    }
    cross_platform_sleep(400);

    for (int i = 0; i < ROWS - 1; i++)
        for (int j = 1; j < COLUMNS - 1; j++)
            board[i][j] = ' ';

    const char* message[] = {
        "GAME", "OVER", "", "PLEASE", "TRY", "AGAIN <3", NULL
    };

    int start_row = 4;
    int board_center = COLUMNS / 2;

    for (int i = 0; message[i] != NULL; i++) {
        const char* line = message[i];
        int len = strlen(line);
        int col_start = board_center - (len / 2);
        for (int j = 0; j < len; j++)
            if (col_start + j >= 1 && col_start + j < COLUMNS - 1)
                board[start_row + i][col_start + j] = line[j];
    }

#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif

    display_board_with_piece(board, NULL);
    cross_platform_sleep(3000);
}


// ─────────────────────────────────────────────────────────────
// Game Loop
// ─────────────────────────────────────────────────────────────

void start_tetris() {
    srand(time(NULL));
    show_title_screen();
    game_run();
}

void game_run() {
    enable_input_mode();
    initialize_board();
    generate_random_piece_into(&piece_a);
    current_piece = &piece_a;
    generate_random_piece_into(&piece_b);
    next_piece = &piece_b;

    int base_delay = 500;
    int fast_delay = 15;
    int current_drop_delay = base_delay;
    int fast_drop = 0;
    int paused = 0;

    while (1) {
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif

        if (paused) {
    char temp_board[ROWS][COLUMNS];

    // Copy board first
    memcpy(temp_board, board, sizeof(temp_board));

    // Draw active piece on top
    draw_piece_on_board(temp_board, current_piece);

    // Overlay "PAUSED" text so it stands out and covers piece & board
    const char* msg = "PAUSED";
    int mid_row = ROWS / 2;
    int mid_col = (COLUMNS - (int)strlen(msg)) / 2;

    for (int i = 0; i < (int)strlen(msg); i++) {
        if (mid_col + i < COLUMNS) {
            temp_board[mid_row][mid_col + i] = msg[i];
        }
    }

    // Now display the combined temp_board
    display_board(temp_board);
} else {
    display_board_with_piece(board, current_piece);
}


        // Handle input
        if (key_pressed()) {
            char input = get_input_char();

            if (input == 'p' || input == 'P') {
                paused = !paused; // Toggle pause on/off instantly
                continue;        // Restart loop immediately after toggling
            }

            if (!paused) {
                if (input == 'a' || input == 'A') {
                    int test_col = current_piece->position.column - 1;
                    if (can_place_piece_at(board, current_piece, current_piece->position.row, test_col))
                        current_piece->position.column--;
                } else if (input == 'd' || input == 'D') {
                    int test_col = current_piece->position.column + 1;
                    if (can_place_piece_at(board, current_piece, current_piece->position.row, test_col))
                        current_piece->position.column++;
                } else if (input == 's' || input == 'S') {
                    fast_drop = 1;
                } else if (input == 'r' || input == 'R' || input == ' ') {
                    piece_t temp = *current_piece;
                    rotate_piece_clockwise(&temp);
                    if (can_place_piece_at(board, &temp, temp.position.row, temp.position.column)) {
                        *current_piece = temp;
                    }
                }

#ifdef _WIN32
                FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
#endif
            }
        } else {
            if (!paused) {
                fast_drop = 0;
            }
        }

        if (paused) {
            // When paused, skip dropping and other logic
            cross_platform_sleep(100);
            continue;
        }

        // Falling piece timing logic
        int elapsed = 0;
        const int interval = 10;

        while (elapsed < current_drop_delay) {
            if (key_pressed()) {
                char input = get_input_char();

                if (input == 'p' || input == 'P') {
                    paused = !paused;
                    break; // break out of the drop delay loop to handle pause immediately
                }

                if (!paused) {
                    if (input == 'a' || input == 'A') {
                        int test_col = current_piece->position.column - 1;
                        if (can_place_piece_at(board, current_piece, current_piece->position.row, test_col))
                            current_piece->position.column--;
                    } else if (input == 'd' || input == 'D') {
                        int test_col = current_piece->position.column + 1;
                        if (can_place_piece_at(board, current_piece, current_piece->position.row, test_col))
                            current_piece->position.column++;
                    } else if (input == 's' || input == 'S') {
                        fast_drop = 1;
                    } else if (input == 'r' || input == 'R' || input == ' ') {
                        piece_t temp = *current_piece;
                        rotate_piece_clockwise(&temp);
                        if (can_place_piece_at(board, &temp, temp.position.row, temp.position.column)) {
                            *current_piece = temp;
                        }
                    }
                }

#ifdef _WIN32
                FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
#endif
            } else {
                if (!paused) {
                    fast_drop = 0;
                }
            }

            cross_platform_sleep(interval);
            elapsed += interval;
            current_drop_delay = fast_drop ? fast_delay : base_delay;

            if (paused) {
                break; // If paused during delay, exit early
            }
        }

        if (paused) {
            continue; // skip rest of loop if paused
        }

        // Handle piece drop
        if (can_piece_drop(board, current_piece)) {
            current_piece->position.row++;
        } else {
            place_piece(board, current_piece);

            int lines_to_clear[4];  // max 4 lines
            int count = find_full_lines(lines_to_clear);

            if (count > 0) {
                animate_line_clear(lines_to_clear, count);
                clear_marked_lines(lines_to_clear, count);
            }

            // Switch pieces
            if (current_piece == &piece_a) {
                current_piece = &piece_b;
                generate_random_piece_into(&piece_a);
                next_piece = &piece_a;
            } else {
                current_piece = &piece_a;
                generate_random_piece_into(&piece_b);
                next_piece = &piece_b;
            }

            if (!can_place_piece_at(board, current_piece,
                                     current_piece->position.row,
                                     current_piece->position.column)) {
                animate_game_over();
                break;
            }
        }
    }

    disable_input_mode();
}
