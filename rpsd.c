#define _POSIX_C_SOURCE 200809L

#include <stdio.h>       
#include <stdlib.h>     
#include <string.h>      
#include <unistd.h>      
#include <sys/socket.h>  
#include <netdb.h>       
#include <pthread.h> 
    
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


MessageType get_messsage_type();
int parse_play();
int parse_move();
int parse_continue();
int parse_quit();
void play_logic(Player *player, const char *name);
void move_logic(Player *player, const char *move);
void continue_logic(Player *player, int rematch);
void quit_logic(Player *player);
int receiver(int socket, char *buffer, size_t len);
int sender(int sock, const char *message);
void wait(int socket);
void begin(int socket);
void result(int socket, char result, const char **move);
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
    
};

void continue_logic(Player *player, int rematch){
    player->rematch=rematch;
};

void quit_logic(Player *player){
    player->rematch=0;
};

