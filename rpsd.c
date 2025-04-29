#define _POSIX_C_SOURCE 200809L


#include "rpsd.h"

Player *active_players[LIMIT];
int player_count = 0;
pthread_mutex_t player_lock = PTHREAD_MUTEX_INITIALIZER;

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

    int just_space = 1;
    for (int i = 0; i < name_len; i++) {
        if (!isspace((unsigned char)name[i])) {
            just_space = 0;
            break;
        }
    }

    if (just_space) {
        return -1;
    }
    
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

    if (strcmp(temp_move, "ROCK") != 0 && strcmp(temp_move, "PAPER") != 0 && strcmp(temp_move, "SCISSORS") != 0) {
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

void play_logic(Player *player){
    //add active player, call add_active_player
    //add_active_player(player);
    //call wait
    wait_socket(player->fd);
}

void move_logic(Player *player, const char *move) {
    strcpy(player->move,move);
    
}

void continue_logic(Player *player) {
    player->rematch=1;
}


void quit_logic(Player *player) {
    if (!player) return;  

    player->rematch = 0;

    pthread_mutex_lock(&player_lock);

    for (int i = 0; i < player_count; i++) {
        if (active_players[i] == player) {
            remove_active_player(player);
            break;
        }
    }

    if (player->fd >= 0) {
        close(player->fd);
        player->fd = -1;
    }

    pthread_mutex_unlock(&player_lock);

}


void wait_socket(int socket) {
  //call sender
  sender(socket, "W|1||");
  
}

void flush_socket(int fd) {
    char buf[256];
    while (1) {
        struct pollfd pfd = { .fd = fd, .events = POLLIN };
        int ret = poll(&pfd, 1, 0);
        if (ret <= 0 || !(pfd.revents & POLLIN)) {
            break;
        }
        read(fd, buf, sizeof(buf));
    }
}


void begin(int socket, const char *opponent_name) {
    //use snprintf!
    char begin_str[256];
    snprintf(begin_str, sizeof(begin_str), "B|%s||", opponent_name);
    sender(socket, begin_str);
}


void result(int socket, char result, const char *move) {
    char result_str[256];
    snprintf(result_str, sizeof(result_str), "R|%c|%s||", result, move);
    sender(socket, result_str);
}

int server(int port) {
    char port_ch[16];
    sprintf(port_ch, "%d", port);
    int listener = open_listener(port_ch, 50);
    if (listener < 0) { perror("listener error"); exit(EXIT_FAILURE);}

    printf("Server started on port %d\n", port);

    while (1) {
        int *fd = malloc(sizeof(int));
        *fd = accept(listener, NULL, NULL);
        if (*fd < 0) {
            perror("acceot error");
            free(fd);
            continue;
        }
        pthread_t tid;
        if (pthread_create(&tid, NULL, register_player, fd) != 0) {
            perror("pthread_create error");
            close(*fd);
            free(fd);  
            continue;
        }
        pthread_detach(tid);
    }


    close(listener);
    return EXIT_SUCCESS;


}

int is_connected(Player *player) {
    if (!player || player->fd < 0) {
        return 0;
    }

    struct pollfd pfd = { .fd = player->fd, .events = POLLIN | POLLHUP | POLLERR };
    int ret = poll(&pfd, 1, 0);

    if (ret < 0) {
        perror("poll");
        return 0;
    }

    if (pfd.revents & (POLLHUP | POLLERR)) {
        return 0;
    }

    if (pfd.revents & POLLIN) {
        char buf;
        ssize_t n = recv(player->fd, &buf, 1, MSG_PEEK | MSG_DONTWAIT);
        if (n == 0) {
            return 0;
        } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            return 0;
        }
    }

    return 1; 
}

void add_two_players() {
    pthread_mutex_lock(&player_lock);

    for (int i = 0; i < player_count; ) {
        if (!active_players[i] || !is_connected(active_players[i])) {
            if (active_players[i]) {
                printf("Found disconnected player during matching: %s\n", active_players[i]->name);
                quit_logic(active_players[i]);
                
            }
            i++; 
        } else {
            i++;
        }
    }

    Player *player1 = NULL;
    Player *player2 = NULL;

    for (int i = 0; i < player_count; i++) {
        Player *p = active_players[i];
        if (p && p->waiting) {
            if (!player1) player1 = p;
            else if (!player2) {
                player2 = p;
                break;
            }
        }
    }

    if (player1 && player2) {
        player1->waiting = 0;
        player2->waiting = 0;

        remove_active_player(player1);
        remove_active_player(player2);

        pthread_mutex_unlock(&player_lock);
        match_players(player1, player2);
        return;
    }

    pthread_mutex_unlock(&player_lock);
}

void clean_disconnected_players() {
    pthread_mutex_lock(&player_lock);
    
    int i = 0;
    while (i < player_count) {
        Player *p = active_players[i];
        
        if (!p) {
            i++;
            continue;
        }
        
        if (!is_connected(p)) {
            printf("player disconnected while waiting: %s\n", p->name);
       
            for (int j = i; j < player_count - 1; j++) {
                active_players[j] = active_players[j+1];
            }
            player_count--;
            
            if (p->fd >= 0) {
                close(p->fd);
            }
            free(p);
        } else {
            i++;
        }
    }
    
    pthread_mutex_unlock(&player_lock);
}

void *register_player(void *arg) {
    int fd = *(int *)arg;
    free(arg);

    Player *player = malloc(sizeof(Player));
    if (!player) {
        perror("malloc failed");
        close(fd);
        return NULL;
    }

    player->fd = fd;
    player->move[0] = '\0';
    player->rematch = 0;
    player->waiting = 1;

    char buffer[256];
    if (receiver(player->fd, buffer, sizeof(buffer)) != 0) {
        close(fd);
        free(player);
        return NULL;
    }

    if (parse_play(buffer, player->name) != 0) {
        close(player->fd);
        free(player);
        return NULL;
    }

    clean_disconnected_players();

    pthread_mutex_lock(&player_lock);
 
    for (int i = 0; i < player_count; i++) {
        if (active_players[i] && strcmp(active_players[i]->name, player->name) == 0) {
            pthread_mutex_unlock(&player_lock);
            result(player->fd, 'L', "Logged in");
            close(player->fd);
            free(player);
            return NULL;
        }
    }
    
    wait_socket(player->fd);

    add_active_player(player);
    pthread_mutex_unlock(&player_lock);

    printf("Player registered: %s\n", player->name);


    add_two_players();
    return NULL;
}


void match_players(Player *player1, Player *player2) {
    if (!is_connected(player1)) {
        //printf("Player %s disconnected before match could start\n", player1->name);
        quit_logic(player1);
        free(player1);

        player2->waiting = 1;
        pthread_mutex_lock(&player_lock);
        add_active_player(player2);  
        pthread_mutex_unlock(&player_lock);
        return;
    }
    
    if (!is_connected(player2)) {
        //printf("Player %s disconnected before match could start\n", player2->name);
        quit_logic(player2);
        free(player2);
        
        player1->waiting = 1;
        pthread_mutex_lock(&player_lock);
        add_active_player(player1);  
        pthread_mutex_unlock(&player_lock);
        return;
    }

    Game *new_game = malloc(sizeof(Game));
    if (!new_game) {
        perror("allocation error");
        return;
    }
    new_game->player1 = player1;
    new_game->player2 = player2;


    pthread_t game_thread;
    if (pthread_create(&game_thread, NULL, (void *(*)(void *))game, new_game) != 0) {
        perror("pthread_create error");
        player1->waiting = 1;
        player2->waiting = 1;
        pthread_mutex_lock(&player_lock);
        add_active_player(player1);
        add_active_player(player2);
        pthread_mutex_unlock(&player_lock);
        free(new_game);
        return;
    }

    pthread_detach(game_thread);
    printf("Match started: %s vs %s\n", player1->name, player2->name);
}


void *game(void *arg) {
    Game *game = (Game *)arg;
    Player *player1 = game->player1;
    Player *player2 = game->player2;
    char buffer1[256];
    char buffer2[256];

 
    player1->rematch = 0;
    player2->rematch = 0;
    
    if (!is_connected(player1)) {
        //printf("Player %s disconnected before game could start\n", player1->name);
        
        if (is_connected(player2)) {
            player2->waiting = 1;
            pthread_mutex_lock(&player_lock);
            add_active_player(player2);
            pthread_mutex_unlock(&player_lock);
        } else {
            if (player2->fd >= 0) close(player2->fd);
            free(player2);
        }
        
        if (player1->fd >= 0) close(player1->fd);
        free(player1);
        free(game);
        return NULL;
    }
    
    if (!is_connected(player2)) {
        //printf("Player %s disconnected before game could start\n", player2->name);
        
        if (is_connected(player1)) {
            player1->waiting = 1;
            pthread_mutex_lock(&player_lock);
            add_active_player(player1);
            pthread_mutex_unlock(&player_lock);
        } else {
            if (player1->fd >= 0) close(player1->fd);
            free(player1);
        }
        
        if (player2->fd >= 0) close(player2->fd);
        free(player2);
        free(game);
        return NULL;
    }

    begin(player1->fd, player2->name);
    begin(player2->fd, player1->name);

    flush_socket(player1->fd);
    flush_socket(player2->fd);


    while (1) {
       
        memset(player1->move, 0, sizeof(player1->move));
        memset(player2->move, 0, sizeof(player2->move));
        
        //get player moves
        int player1_move = receiver(player1->fd, buffer1, sizeof(buffer1));
        int player2_move = receiver(player2->fd, buffer2, sizeof(buffer2));
        char move_p1[16] = "";
        char move_p2[16] = "";

        if (player1_move != 0 || parse_move(buffer1, move_p1) != 0) {
            result(player2->fd, 'F', "");
            printf("%s forfeited (disconnected)\n", player1->name);
  
            if (player1->fd >= 0) {
                close(player1->fd);
            }
            free(player1);
            player1 = NULL;
            
          
            player2->waiting = 1;
            pthread_mutex_lock(&player_lock);
            add_active_player(player2);
            pthread_mutex_unlock(&player_lock);
            
            free(game);
            return NULL;
        } else {
            move_logic(player1, move_p1);
        }

        if (player2_move != 0 || parse_move(buffer2, move_p2) != 0) {
            result(player1->fd, 'F', "");
            printf("%s forfeited (disconnected)\n", player2->name);
            
            if (player2->fd >= 0) {
                close(player2->fd);
            }
            free(player2);
            player2 = NULL;
            
            player1->waiting = 1;
            pthread_mutex_lock(&player_lock);
            add_active_player(player1);
            pthread_mutex_unlock(&player_lock);
            
            free(game);
            return NULL;
        } else {
            move_logic(player2, move_p2);
        }

        printf("%s played: %s\n", player1->name, move_p1);
        printf("%s played: %s\n", player2->name, move_p2);

        char calc_winner_p1 = winner(move_p1, move_p2);
        char calc_winner_p2 = winner(move_p2, move_p1);

        result(player1->fd, calc_winner_p1, move_p2);
        result(player2->fd, calc_winner_p2, move_p1);

        printf("%s %s against %s\n", player1->name, (calc_winner_p1 == 'W') ? "won" : (calc_winner_p1 == 'L') ? "lost" : "drew", player2->name);
        printf("%s %s against %s\n", player2->name, (calc_winner_p2 == 'W') ? "won" : (calc_winner_p2 == 'L') ? "lost" : "drew", player1->name);

        // Get user input for both players (continue or quit)
        int player1_input = receiver(player1->fd, buffer1, sizeof(buffer1));
        int player2_input = receiver(player2->fd, buffer2, sizeof(buffer2));

        MessageType type_p1 = (player1_input == 0) ? get_message_type(buffer1) : INVALID;
        MessageType type_p2 = (player2_input == 0) ? get_message_type(buffer2) : INVALID;


        player1->rematch = 0;
        player2->rematch = 0;

        if (type_p1 == CONTINUE) {
            player1->rematch = 1;
            printf("%s wants a rematch\n", player1->name);
        } else {
            printf("%s quit the game\n", player1->name);
        }

        if (type_p2 == CONTINUE) {
            player2->rematch = 1;
            printf("%s wants a rematch\n", player2->name);
        } else {
            printf("%s quit the game\n", player2->name);
        }

        //players quit
        if (!player1->rematch && !player2->rematch) {
          
            if (player1->fd >= 0) close(player1->fd);
            if (player2->fd >= 0) close(player2->fd);
            free(player1);
            free(player2);
            free(game);
            return NULL;
        }

        //player1 rematch, player2 quits
        if (player1->rematch && !player2->rematch) {
            
            player1->waiting = 1;
            pthread_mutex_lock(&player_lock);
            add_active_player(player1);
            pthread_mutex_unlock(&player_lock);

            add_two_players();
            //flush_socket(player1->fd);
            
            if (player2->fd >= 0) close(player2->fd);
            free(player2);
            free(game);
            return NULL;
        }

        //player1 quits, player2 rematches
        if (!player1->rematch && player2->rematch) {
            player2->waiting = 1;
            pthread_mutex_lock(&player_lock);
            add_active_player(player2);
            pthread_mutex_unlock(&player_lock);

            add_two_players();
            
            if (player1->fd >= 0) close(player1->fd);
            free(player1);
            free(game);
            return NULL;
        }
       
        //players continue
        if (player1->rematch && player2->rematch) {
            begin(player1->fd, player2->name);
            begin(player2->fd, player1->name);
            continue;
        }
    }

   
    if (player1 != NULL) {
        if (player1->fd >= 0) close(player1->fd);
        free(player1);
    }

    if (player2 != NULL) {
        if (player2->fd >= 0) close(player2->fd);
        free(player2);
    }

    free(game);
    return NULL;
}


char winner(const char *move1, const char *move2) {
    if (strcmp(move1, move2) == 0) {
        return 'D'; 
    }

    if ((strcmp(move1, "ROCK") == 0 && strcmp(move2, "SCISSORS") == 0) ||
        (strcmp(move1, "PAPER") == 0 && strcmp(move2, "ROCK") == 0) ||
        (strcmp(move1, "SCISSORS") == 0 && strcmp(move2, "PAPER") == 0)) {
        return 'W'; 
    }

    return 'L'; 
}



void add_active_player(Player *player) {
    if (player_count >= LIMIT) {
        return;
    } 

    active_players[player_count++] = player;
}


void remove_active_player(Player *player) {
    int found = 0;
    for (int i = 0; i < player_count; i++) {
        if (found) {
            active_players[i-1] = active_players[i];
        } else if (active_players[i] == player) {
            found = 1;
        }
    }
    
    if (found) {
        player_count--;
        if (player_count < LIMIT) {
            active_players[player_count] = NULL;
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Error: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[1]);

    return server(port);
}