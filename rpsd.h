#ifndef RPSD_H
#define RPSD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include "network.h"

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

extern Player *waiting;
extern char **active_players;
extern int player_count;

MessageType get_message_type(char *buffer);
int parse_play(char *buffer, char *name);
int parse_move(char *buffer, char *move);
void play_logic(Player *player, const char *name);
void move_logic(Player *player, const char *move);
void continue_logic(Player *player);
void quit_logic(Player *player);
int receiver(int socket, char *buffer, size_t length);
int sender(int socket, const char *message);
void wait_socket(int socket);
void begin(int socket, const char *opponent_name);
void result(int socket, char result, const char *move);
int server(int port);
Player *register_player(int listener);
void match_players(Player *player1, Player *player2);
void game(void *arg);
char winner(const char *move1, const char *move2);
void add_active_player(const char *name);
void remove_active_player(const char *name);

#endif 
