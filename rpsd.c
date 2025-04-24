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


MessageType get_messsage_type(char *buffer);
int parse_play(char *buffer, char *name);
int parse_move(char *buffer, char *move);
int parse_continue(char *cont);
int parse_quit(char *quit);
void play_logic(Player *player, const char *name);
void move_logic(Player *player, const char *move);
void continue_logic(Player *player, int rematch);
void continue_logic(Player *player);
void quit_logic(Player *player);
int receiver(int socket, char *buffer, size_t length);
int sender(int socket, const char *message);
void wait(int socket);
void begin(int socket);
void result(int socket, char result, const char *move);
int server();
Player *accept_player(int listener);
Player *register_player(int listner);
void match_players(Player *player1, Player *player2);
void game(void *arg);
char winner(const char *move1, const char *move2);
int active_player(const char *name);
void add_active_player(const char *name);
void remove_active_player(const char *name);
int main(int argc, char **argv);


void play_logic(Player *player, const char *name){
    player->fd = 0;
    strcpy(player->name, name);
    //add active player
    //wait();
}

void move_logic(Player *player, const char *move){
    strcpy(player->move,move);
    
}

void continue_logic(Player *player, int rematch){
    player->rematch=rematch;
}

void quit_logic(Player *player){
    player->rematch=0;
}

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

int parse_continue(char *cont) {
    if (strcmp(cont, "C") != 0) {
        return -1;
    }

    return 0;
}

int parse_quit(char *quit) {
    if (strcmp(quit, "Q") != 0) {
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



