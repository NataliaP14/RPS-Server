#define _POSIX_C_SOURCE 200809L

#include <stdio.h>       
#include <stdlib.h>     
#include <string.h>      
#include <unistd.h>      
#include <sys/socket.h>  
#include <netdb.h>       
#include <pthread.h> 
#include <ctype.h>
    
typedef struct {
    int fd;
    char name[256];
    char move[16];
    int rematch;
} Player;

typedef struct {
    Player *player1;
    Player *player2;
} Game;

typedef enum {
    PLAY,
    MOVE,
    CONTINUE,
    QUIT,
    INVALID = -1
} MessageType;

char **active_players = NULL;
int player_count = 0;


MessageType get_messsage_type(char *buffer);
int parse_play(char *buffer, char *name);
int parse_move(char *buffer, char *move);
int parse_continue(char *cont);
int parse_quit(char *quit);
void play_logic(Player *player, const char *name);
void move_logic(Player *player, const char *move);
void continue_logic(Player *player);
void quit_logic(Player *player);
int receiver(int socket, char *buffer, size_t length);
int sender(int socket, const char *message);
void wait(int socket);
void begin(int socket);
void result(int socket, char result, const char *move);
int server();
Player *register_player(int listner);
void match_players(Player *player1, Player *player2);
void game(void *arg);
char winner(const char *move1, const char *move2);
int active_player(const char *name);
void add_active_player(const char *name);
void remove_active_player(const char *name);
int main(int argc, char **argv);


MessageType get_message_type(char *buffer) {
    if (strcmp(buffer, "C") == 0) {
        return CONTINUE;
    }
    if (strcmp(buffer, "Q") == 0) {
        return QUIT;
    }

    if (buffer[1] == '|') {
        switch (buffer[0]) {
            case 'P': return PLAY;
            case 'M': return MOVE;
        }
    }

    return INVALID;
}

int parse_play(char *buffer, char *name) {
    char temp_buffer[256];
    strncpy(temp_buffer, buffer, sizeof(temp_buffer));
    temp_buffer[256] = '\0';

    char *type = strtok(temp_buffer, "|");
    char *input = strtok(NULL, "|");
    char *end = strtok(NULL, "|");

    if (!type || strcmp(type, "P") != 0) {
        return -1;
    }

    if (!input || !end) {
        return -1;
    }

    strncpy(name, input, 255);
    name[255] = '\0';
    return 0;
    
}

int parse_move(char *buffer, char *move) {
    char temp_buffer[256];
    strncpy(temp_buffer, buffer, sizeof(temp_buffer));
    temp_buffer[256] = '\0';

    char *type = strtok(temp_buffer, "|");
    char *input = strtok(NULL, "|");
    char *end = strtok(NULL, "|");

    if (!type || strcmp(type, "M") != 0) {
        return -1;
    }

    if (!input || !end) {
        return -1;
    }

    int i;
    for (i = 0; input[i] && i < 15; i++) {
        move[i] = toupper((unsigned char)input[i]);
    }
    move[i] = '\0';

    if (strcmp(move, "ROCK") != 0 && strcmp(move, "PAPER")!= 0 && strcmp(move, "SCISSORS") != 0) {
        return -1;
    }
    
    return 0;
}

int receiver(int socket, char *buffer, size_t length) {
    ssize_t n = read(socket, buffer, length - 1);
    if (n < 0) {
        perror("Socket read error");
        return -1;
    }

    if (n == 0) {
        return -1;
    }

    buffer[n] = '\0';

    return 0;
}

int sender(int socket, const char *message) {
    ssize_t n = write(socket, message, strlen(message));

    if (n < 0) {
        perror("Socket write error");
        return -1;
    }

    return 0;
}

void play_logic(Player *player, const char *name){
    strcpy(player->name, name);
    //add active player, call add_active_player
    //wait();
}

void move_logic(Player *player, const char *move){
    strcpy(player->move,move);
    
}

void continue_logic(Player *player){
    player->rematch=1;
}

void quit_logic(Player *player){
    player->rematch=0;
    close(player->fd);
    remove_active_player(player->name);
}

void wait(int socket) {

}

void begin(int socket) {

}

void result(int socket, char result, const char *move) {
    
}

int server() {

}

Player *register_player(int listener) {
    int fd = accept(listener, NULL, NULL);
    if (fd < 0) {
        perror("accept failed");
        return NULL;
    }

    char buffer[256];
    if (receiver(fd, buffer, sizeof(buffer)) != 0) {
        close(fd);
        return NULL;
    }

    char name[256];
    if (parse_play(buffer, name) != 0) {
        close(fd);
        return NULL;
    }

    //check for duplicate name
    for (int i = 0; i < player_count; i++) {
        if (strcmp(active_players[i], name) == 0) {
            sender(fd, "R|L|Logged in||");
            close(fd);
            return NULL;
        }
    }

    Player *player = malloc(sizeof(Player));
    if (!player) {
        perror("malloc failed");
        close(fd);
        return NULL;
    }

    player->fd = fd; player->move[0] = '\0'; player->rematch = 0;

    play_logic(player, name);
    add_active_player(name);

    return player;

}

void match_players(Player *player1, Player *player2) {
    Game *new_game = malloc(sizeof(Game));
    if (!new_game) {
        perror("allocation error");
        return;
    }
    new_game->player1 = player1;
    new_game->player2 = player2;

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        //game is running inside of child process
        game((void *)new_game);
        free(new_game);
        exit(EXIT_SUCCESS);
    }

}

void game(void *arg) {
    Game *game = (Game *)arg;
    Player *player1 = game->player1;
    Player *player2 = game->player2;
    char buffer1[256];
    char buffer2[256];

    begin(player1->fd);
    begin(player2->fd);

    while (1) {
        //get user input for both players (their moves)
        int player1_move = receiver(player1->fd, buffer1, sizeof(buffer1));
        int player2_move = receiver(player2->fd, buffer2, sizeof(buffer2));
        char move_p1[16] = "";
        char move_p2[16] = "";

        if (player1_move == 0 && parse_move(buffer1, move_p1) == 0) {
            move_logic(player1, move_p1);
        }

        if (player2_move == 0 && parse_move(buffer2, move_p2) == 0) {
            move_logic(player2, move_p2);
        }

        //p1 forfeit
        if (move_p1[0] == '\0') {
            result(player2->fd, 'F', "");
        //p2 forfeit
        } else if (move_p2[0] == '\0') {
            result(player1->fd, 'F', "");
        } else if (move_p1[0] == '\0' && move_p2[0] == '\0') {
            break;
        } else {
            //determine winner
            char calc_winner_p1 = winner(move_p1, move_p2);
            char calc_winner_p2 = winner(move_p2, move_p1);
            result(player1->fd, calc_winner_p1, move_p2);
            result(player2->fd, calc_winner_p2, move_p1);
        }

        //get user input for both players (continue or quit)
        int player1_input = receiver(player1->fd, buffer1, sizeof(buffer1));
        int player2_input = receiver(player2->fd, buffer2, sizeof(buffer2));

        MessageType type_p1 = INVALID;
        MessageType type_p2 = INVALID;

        if (player1_input == 0) {
            type_p1 = get_message_type(buffer1);
        }
        if (player2_input == 0) {
            type_p2 = get_message_type(buffer2);
        }

        if (type_p1 == CONTINUE) {
            continue_logic(player1);
        } else if (type_p1 == QUIT) {
            quit_logic(player1);
        }

        if (type_p2 == CONTINUE) {
            continue_logic(player2);
        } else if (type_p2 == QUIT) {
            quit_logic(player2);
        }

        //players continue
        if (player1->rematch && player2->rematch) {
            begin(player1->fd);
            begin(player2->fd);
            continue;
        //players quit
        } else if (!player1->rematch && !player2->rematch) {
            break;
        //player1 quits and player2 continues    
        } else if (!player1->rematch && player2->rematch) {
            wait(player2->fd);
            break;
        //player1 continues and player2 quits    
        } else if (player1->rematch && !player2->rematch) {
            wait(player1->fd);
            break;
        }


    }

   
    free(player1);
    free(player2);
    free(game);

}

char winner(const char *move1, const char *move2) {

}

int active_player(const char *name) {

}

void add_active_player(const char *name) {

}

void remove_active_player(const char *name) {

}

int main(int argc, char **argv) {

    return EXIT_SUCCESS;
}



