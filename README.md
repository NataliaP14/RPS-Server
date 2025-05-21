# Rock-Paper-Scissors Server (Multithreaded)

This project implements a multithreaded rock-paper-scissors (RPS) game server in C.

## Features

- Accepts multiple client connections via TCP sockets
- Handles client messages:
  - `P|<name>||`: Register player
  - `M|<move>||`: Submit move (`ROCK`, `PAPER`, `SCISSORS`)
  - `C`: Request rematch
  - `Q`: Quit the game
- Player matchmaking and game session handling
- Player disconnects
- Synchronization using `pthread_mutex`
- Message validation and parsing
- I/O with polling

## Protocol

The server communicates with clients using a custom protocol with `|` delimiters.

### Client 

- `P|<name>||` — Play request with player name
- `M|<move>||` — Submit move (must be `ROCK`, `PAPER`, or `SCISSORS`)
- `C` — Continue/rematch request
- `Q` — Quit

### Server 

- `B|<opponent_name>||` — Begin match
- `R|<result>|<opponent_move>||` — Match result
- `W|1||` — Wait message