#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>

#define WIDTH 80
#define HEIGHT 24
#define PADDLE_HEIGHT 3
#define BALL_SPEED 100000  // Adjust ball speed (microseconds)

// Adjustable parameters
int max_score = 5;

// Game state variables
int player1_y, player2_y;
int ball_x, ball_y, ball_velocity_x, ball_velocity_y;
int score_player1 = 0, score_player2 = 0;
struct termios original_termios;

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
}

void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &original_termios);
    atexit(disable_raw_mode);

    struct termios raw = original_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Function to initialize game
void initialize_game() {
    player1_y = HEIGHT / 2;
    player2_y = HEIGHT / 2;
    ball_x = WIDTH / 2;
    ball_y = HEIGHT / 2;
    srand(time(NULL));
    ball_velocity_x = (rand() % 2 == 0) ? -1 : 1;
    ball_velocity_y = (rand() % 2 == 0) ? -1 : 1;
}

// Function to render the title screen
void render_title_screen() {
    printf("\033[2J\033[H"); // Clear screen and move cursor to top

    const char *title[] = {
        " ______                 ",
        " | ___ \\                ",
        " | |_/ /__  _ __   __ _ ",
        " |  __/ _ \\| '_ \\ / _` |",
        " | | | (_) | | | | (_| |",
        " \\_|  \\___/|_| |_|\\__, |",
        "                   __/ |",
        "                   |___/"
    };
    int title_lines = sizeof(title) / sizeof(title[0]);

    // Draw title
    for (int i = 0; i < title_lines; i++) {
        printf("\033[%d;%dH%s", HEIGHT / 4 + i, (int)(WIDTH - strlen(title[i])) / 2, title[i]);
    }

    printf("\033[%d;%dHGitHub: zgris", HEIGHT / 2 + 7, (WIDTH - 12) / 2);
    printf("\033[%d;%dHPress 'space' to start", HEIGHT / 2 + 9, (WIDTH - 21) / 2);

    // Controls for players
    printf("\033[%d;%dHLeft Player: 'w' to move up, 's' to move down", HEIGHT / 2 + 11, (WIDTH - 40) / 2);
    printf("\033[%d;%dHRight Player: 'o' to move up, 'l' to move down", HEIGHT / 2 + 12, (WIDTH - 41) / 2);

    fflush(stdout); // Ensure everything is printed immediately without flicker
}

// Function to handle keyboard input
void handle_input(bool *in_title_screen) {
    struct timeval timeout = {0, 0};
    fd_set set;
    char buf[1];

    // Set up the file descriptor set
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);

    // Handle all input keys available at the time
    while (select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout) > 0) {
        read(STDIN_FILENO, buf, 1);
        switch (buf[0]) {
            case 'w': if (player1_y > 0) player1_y--; break;
            case 's': if (player1_y < HEIGHT - PADDLE_HEIGHT) player1_y++; break;
            case 'o': if (player2_y > 0) player2_y--; break;
            case 'l': if (player2_y < HEIGHT - PADDLE_HEIGHT) player2_y++; break;
            case ' ': if (*in_title_screen) { *in_title_screen = false; initialize_game(); } break;
        }
        // Refresh the set and timeout for subsequent reads
        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);
    }
}

// Update game logic
void update_game() {
    ball_x += ball_velocity_x;
    ball_y += ball_velocity_y;

    if (ball_y <= 0 || ball_y >= HEIGHT - 1) {
        ball_velocity_y = -ball_velocity_y;
    }

    if (ball_x == 1 && ball_y >= player1_y && ball_y < player1_y + PADDLE_HEIGHT) {
        ball_velocity_x = -ball_velocity_x;
    } else if (ball_x == WIDTH - 2 && ball_y >= player2_y && ball_y < player2_y + PADDLE_HEIGHT) {
        ball_velocity_x = -ball_velocity_x;
    }

    if (ball_x < 0) {
        score_player2++;
        initialize_game();
    } else if (ball_x >= WIDTH) {
        score_player1++;
        initialize_game();
    }
}

// Render game screen
void render_game() {
    printf("\033[H");  // Reset cursor to top left
    printf("Score: %d - %d\n", score_player1, score_player2);

    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            if ((j == 0 && i >= player1_y && i < player1_y + PADDLE_HEIGHT) ||
                (j == WIDTH - 1 && i >= player2_y && i < player2_y + PADDLE_HEIGHT)) {
                printf("|");
            } else if (i == ball_y && j == ball_x) {
                printf("O");
            } else if (i == 0 || i == HEIGHT - 1) {
                printf("-");
            } else {
                printf(" ");
            }
        }
        printf("\n");
    }
}

// Initialize terminal settings
void initialize_terminal() {
    enable_raw_mode();
    printf("\033[?25l"); // Hide cursor
}

// Clean up terminal settings
void cleanup_terminal() {
    printf("\033[?25h"); // Show cursor
}

// Main function
int main() {
    initialize_terminal();
    bool in_title_screen = true;
    render_title_screen();

    while (true) {
        handle_input(&in_title_screen);
        if (in_title_screen) {
            render_title_screen();
        } else {
            update_game();
            render_game();
        }
        if (score_player1 >= max_score || score_player2 >= max_score) {
            in_title_screen = true;
            score_player1 = score_player2 = 0;
            render_title_screen();
        }
        usleep(BALL_SPEED); // Adjust ball speed with the BALL_SPEED parameter
    }

    cleanup_terminal();
    return 0;
}

