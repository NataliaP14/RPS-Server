#define _POSIX_C_SOURCE 200809L

#include "rpsd.h"

Player *waiting = NULL;
char **active_players = NULL;
int player_count = 0;

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
    temp_buffer[255] = '\0';

    size_t len = strlen(temp_buffer);
    if (len > 0 && temp_buffer[len - 1] == '\n') {
        temp_buffer[len - 1] = '\0';
    }

    if (strncmp(temp_buffer, "P|", 2) != 0) {
        return -1;
    }
    
    if (len < 4 || strcmp(&temp_buffer[len-2], "||") != 0) {
        return -1;
    }
    
    
    char *name_start = temp_buffer + 2;  
    int name_len = len - 4; 
    
    if (name_len <= 0) {
        return -1;
    }
    
    strncpy(name, name_start, name_len);
    name[name_len] = '\0';
    
    return 0;
    
}

int parse_move(char *buffer, char *move) {
    char temp_buffer[256];
    strncpy(temp_buffer, buffer, sizeof(temp_buffer));
    temp_buffer[255] = '\0';

    size_t len = strlen(temp_buffer);
    if (len > 0 && temp_buffer[len - 1] == '\n') {
        temp_buffer[len - 1] = '\0';
    }

    if (strncmp(temp_buffer, "M|", 2) != 0) {
        return -1; 
    }

    strtok(temp_buffer, "|"); 
    char *input = strtok(NULL, "|");
    char *end = strtok(NULL, "|");
    char *extra = strtok(NULL, "|");

    if (!input || extra != NULL) {
        return -1; 
    }

    char temp_move[16];
    strncpy(temp_move, input, sizeof(temp_move) - 1);
    temp_move[sizeof(temp_move) - 1] = '\0';

    for (int i = 0; temp_move[i]; i++) {
        temp_move[i] = toupper((unsigned char)temp_move[i]);
    }

    if (strcmp(temp_move, "ROCK") != 0 &&
        strcmp(temp_move, "PAPER") != 0 &&
        strcmp(temp_move, "SCISSORS") != 0) {
        return -1;
    }

    strcpy(move, temp_move);
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
    //add active player, call add_active_player
    //call wait
    wait_socket(player->fd);
}

void move_logic(Player *player, const char *move){
    strcpy(player->move,move);
    
}

void continue_logic(Player *player){
    player->rematch=1;
}

void quit_logic(Player *player){

    player->rematch = 0;
    if (player->fd >= 0) {
        close(player->fd);
        player->fd = -1;  
    } 
    //call remvoe active player
    
}

void wait_socket(int socket) {
  //call sender
  
}

void begin(int socket, const char *opponent_name) {
    //use snprintf!
}


//TODO: nat
void result(int socket, char result, const char *move) {
    
}

int server(int port) {
    char port_ch[16];
    sprintf(port_ch, "%d", port);
    int listener = open_listener(port_ch, 50);
    if (listener < 0) { perror("listener error"); exit(EXIT_FAILURE);}

    printf("Server started on port %d\n", port);

    while (1) {
        Player *player = register_player(listener);
        if (!player) {
            continue;
        }

        if (waiting == NULL) {
            waiting = player;
            //printf("%s is waiting for an opponent.", player->name);
        } else {
            match_players(waiting, player);
            waiting = NULL;
        }
        
    }

    close(listener);
    return EXIT_SUCCESS;


}

Player *register_player(int listener) {
    Player *player = malloc(sizeof(Player));
    player->fd = accept(listener, NULL, NULL);
    if (player->fd < 0) {
        perror("accept failed");
        return NULL;
    }

    char buffer[256];
    if (receiver(player->fd, buffer, sizeof(buffer)) != 0) {
        close(player->fd);
        free(player);
        return NULL;
    }

    if (parse_play(buffer, player->name) != 0) {
        close(player->fd);
        free(player);
        return NULL;
    }

    //check for duplicate name
    for (int i = 0; i < player_count; i++) {
        if (strcmp(active_players[i], player->name) == 0) {
            sender(player->fd, "R|L|Logged in||");
            close(player->fd);
            free(player);
            return NULL;
        }
    }

    if (!player) {
        perror("malloc failed");
        close(player->fd);
        free(player);
        return NULL;
    }

    player->move[0] = '\0'; player->rematch = 0;

    play_logic(player, player->name);
    add_active_player(player->name);

    printf("Player registered: %s\n", player->name);

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
        free(new_game);
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        //game is running inside of child process
        game((void *)new_game);
        //free(new_game);
        exit(EXIT_SUCCESS);
    } else {
        printf("Match started: %s vs %s\n", player1->name, player2->name);
        close(player1->fd);
        close(player2->fd);
        free(player1);
        free(player2);
        free(new_game);
        
    }

}

void game(void *arg) {
    Game *game = (Game *)arg;
    Player *player1 = game->player1;
    Player *player2 = game->player2;
    char buffer1[256];
    char buffer2[256];

    begin(player1->fd, player2->name);
    begin(player2->fd, player1->name);

    while (1) {
        //get user input for both players (their moves)
        int player1_move = receiver(player1->fd, buffer1, sizeof(buffer1));
        int player2_move = receiver(player2->fd, buffer2, sizeof(buffer2));
        char move_p1[16] = "";
        char move_p2[16] = "";
        //printf("Received from player1: %s\n", buffer1);

        if (player1_move == 0 && parse_move(buffer1, move_p1) == 0) {
            move_logic(player1, move_p1);
        } else {
            quit_logic(player1);
            result(player2->fd, 'F', ""); 
            printf("%s forfeited\n", player1->name);
            if (player1 != NULL) free(player1);
            if (player2 != NULL) free(player2);
            player1 = NULL;
            player2 = NULL;
            break;
        }

        if (player2_move == 0 && parse_move(buffer2, move_p2) == 0) {
            move_logic(player2, move_p2);
        } else {
            quit_logic(player2);
            result(player1->fd, 'F', ""); 
            printf("%s forfeited\n", player2->name);
            if (player1 != NULL) free(player1);
            if (player2 != NULL) free(player2);
            player1 = NULL;
            player2 = NULL;
            break;
        }

        printf("%s played: %s\n", player1->name, move_p1);
        printf("%s played: %s\n", player2->name, move_p2);

        //determine winner
        char calc_winner_p1 = winner(move_p1, move_p2);
        char calc_winner_p2 = winner(move_p2, move_p1);

        result(player1->fd, calc_winner_p1, move_p2);
        result(player2->fd, calc_winner_p2, move_p1);

        printf("%s %s against %s\n", player1->name, (calc_winner_p1 == 'W') ? "won" : (calc_winner_p1 == 'L') ? "lost" : "drew", player2->name);
        printf("%s %s against %s\n", player2->name, (calc_winner_p2 == 'W') ? "won" : (calc_winner_p2 == 'L') ? "lost" : "drew", player1->name);


        //get user input for both players (continue or quit)
        int player1_input = receiver(player1->fd, buffer1, sizeof(buffer1));
        int player2_input = receiver(player2->fd, buffer2, sizeof(buffer2));

        MessageType type_p1 = (player1_input == 0) ? get_message_type(buffer1) : INVALID;
        MessageType type_p2 = (player2_input == 0) ? get_message_type(buffer2) : INVALID;

        if (type_p1 == CONTINUE) {
            continue_logic(player1);
            printf("%s wants a rematch\n", player1->name);
        } else {
            quit_logic(player1);
            printf("%s quit the game\n", player1->name);
        }

        if (type_p2 == CONTINUE) {
            continue_logic(player2);
            printf("%s wants a rematch\n", player2->name);
        } else {
            quit_logic(player2);
            printf("%s quit the game\n", player2->name);
        }

        //players quit
        if ((!player1->rematch && !player2->rematch) || (player1->rematch && !player2->rematch) || (!player1->rematch && player2->rematch)) {
            break;
        }
       
        //players continue
        if (player1->rematch && player2->rematch) {
            begin(player1->fd, player2->name);
            begin(player2->fd, player1->name);
            continue;
        }
    }

    if (player1 != NULL && player1->fd >= 0) {
        close(player1->fd);
    }
    if (player2 != NULL && player2->fd >= 0) {
        close(player2->fd);
    }
    if (player1 != NULL) {
        free(player1);
    }
    if (player2 != NULL) {
        free(player2);
    }
    free(game);


}


char winner(const char *move1, const char *move2) {
    if (strcmp(move1, move2) == 0) {
        return 'T';  // Tie
    }
    if ((strcmp(move1, "ROCK") == 0 && strcmp(move2, "SCISSORS") == 0) ||
        (strcmp(move1, "PAPER") == 0 && strcmp(move2, "ROCK") == 0) ||
        (strcmp(move1, "SCISSORS") == 0 && strcmp(move2, "PAPER") == 0)) {
        return 'W';  // Win
    }
    return 'L';  // Loss
}


//TODO: nat
void add_active_player(const char *name) {

}

//TODO: nat
void remove_active_player(const char *name) {

}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Error: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[1]);

    return server(port);
}



