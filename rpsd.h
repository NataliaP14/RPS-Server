#ifndef RPSD_H
#define RPSD_H

#define LIMIT 150

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <poll.h>
#include <netdb.h>
#include <pthread.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h> 
#include <sys/time.h>
#include "network.h"

typedef struct {
    int fd;
    char name[256];
    char move[16];
    int rematch;
    int waiting;
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

extern Player *active_players[LIMIT];
extern int player_count;
extern pthread_mutex_t player_lock;

MessageType get_message_type(char *buffer);
int parse_play(char *buffer, char *name);
int parse_move(char *buffer, char *move);
void play_logic(Player *player);
void move_logic(Player *player, const char *move);
void continue_logic(Player *player);
void quit_logic(Player *player);
int receiver(int socket, char *buffer, size_t length);
int sender(int socket, const char *message);
void wait_socket(int socket);
void begin(int socket, const char *opponent_name);
void result(int socket, char result, const char *move);
int server(int port);
void *register_player(void *arg);
void match_players(Player *player1, Player *player2);
void *game(void *arg);
char winner(const char *move1, const char *move2);
void add_active_player(Player *player);
void remove_active_player(Player *player);
void add_two_players();
int is_connected(Player *player);
void clean_disconnected_players();
void print_active_players();


#endif 
