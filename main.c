#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <wchar.h>

#define STR_HEADER_IMPLEMENTATION
#include "str.h"

typedef enum {
    PAWN, ROOK, KNIGHT, BISHOP, QUEEN, KING, NONE
} PieceType;

#define MAX_MOVES 300
#define COORD_TO_INDEX(x, y) y * 8 + x
#define INDEX_TO_COORD_X(index) index % 8
#define INDEX_TO_COORD_Y(index) index / 8

typedef enum {
    BLACK, WHITE, COLOR_NONE
} PieceColor;

typedef struct {
    PieceType type;
    PieceColor color;
    char repr;
    size_t index;
}  Piece;

typedef struct {
    size_t size;
    Piece *pieces;
    PieceColor current;
    int king_pos[2];
} Board;

typedef struct {
    size_t start;
    size_t end;
    Piece capture;
    bool can_capture;
} Move;

Move move_new(size_t start, size_t end, Piece capture, bool can_capture) {
    return (Move){.start=start, .end=end, .capture=capture, .can_capture=can_capture};
}

Piece piece_new(PieceType type, PieceColor color, char repr, size_t index) {
    return (Piece){.type=type, color=color, .repr=repr, .index=index};
}

void add_piece(Piece *pieces, size_t pos, Piece piece) {
    pieces[pos] = piece;
}

void make_move(Move move, Board *board) {
    board->current = (PieceColor)(!board->current);
    Piece piece = board->pieces[move.start];
    board->pieces[move.start] = piece_new(NONE, COLOR_NONE, '-', move.start);
    board->pieces[move.end] = piece;
    board->pieces[move.end].index = move.end;
    
    if (piece.type == KING) {
        board->king_pos[piece.color] = move.end;
    }
}

void unmake_move(Move move, Board *board) {
    Piece piece = board->pieces[move.end];
    
    board->pieces[move.start] = piece;
    piece.index = move.start;
    board->pieces[move.start].index = move.start;
    
    board->pieces[move.end] = piece_new(NONE, COLOR_NONE, '-', move.end);
    if (move.capture.type != NONE) {
        board->pieces[move.end] = piece_new(move.capture.type, move.capture.color, move.capture.repr, move.end);
    }

    if (piece.type == KING) {
        board->king_pos[piece.color] = move.start;
    }
    board->current = (PieceColor)(!board->current);
}

Board board_init(const char *fen) {
    Piece *pieces = malloc(sizeof(Piece) * 64);

    for (size_t i = 0; i < 64; i++) {
        add_piece(pieces, i, piece_new(NONE, COLOR_NONE, '-', i));
    }

    Str fen_string = str_new((char *)fen);

    int black_king_pos = 0;
    int white_king_pos = 0;

    size_t ptr = 0;
    Str token;
    while (fen_string.consumption < fen_string.length) {
        token = str_token(&fen_string, '/');
        for (size_t i = 0; i < token.length; i++) {
            ptr += atoi(&token.string[i]);
            if (!atoi(&token.string[i])) {
                if (ptr < 64) {
                    switch (token.string[i]) {
                        case 'r':
                            add_piece(pieces, ptr, piece_new(ROOK, BLACK, token.string[i], ptr));
                            ptr++;
                            break;
                        case 'n':
                            add_piece(pieces, ptr, piece_new(KNIGHT, BLACK, token.string[i], ptr));
                            ptr++;
                            break;
                        case 'k':
                            black_king_pos = ptr;
                            add_piece(pieces, ptr, piece_new(KING, BLACK, token.string[i], ptr));
                            ptr++;
                            break;
                        case 'p':
                            add_piece(pieces, ptr, piece_new(PAWN, BLACK, token.string[i], ptr));
                            ptr++;
                            break;
                        case 'q':
                            add_piece(pieces, ptr, piece_new(QUEEN, BLACK, token.string[i], ptr));
                            ptr++;
                            break;
                        case 'b':
                            add_piece(pieces, ptr, piece_new(BISHOP, BLACK, token.string[i], ptr));
                            ptr++;
                            break;

                        case 'R':
                            add_piece(pieces, ptr, piece_new(ROOK, WHITE, token.string[i], ptr));
                            ptr++;
                            break;
                        case 'N':
                            add_piece(pieces, ptr, piece_new(KNIGHT, WHITE, token.string[i], ptr));
                            ptr++;
                            break;
                        case 'K':
                            white_king_pos = ptr;
                            add_piece(pieces, ptr, piece_new(KING, WHITE, token.string[i], ptr));
                            ptr++;
                            break;
                        case 'P':
                            add_piece(pieces, ptr, piece_new(PAWN, WHITE, token.string[i], ptr));
                            ptr++;
                            break;
                        case 'Q':
                            add_piece(pieces, ptr, piece_new(QUEEN, WHITE, token.string[i], ptr));
                            ptr++;
                            break;
                        case 'B':
                            add_piece(pieces, ptr, piece_new(BISHOP, WHITE, token.string[i], ptr));
                            ptr++;
                            break;
                    }
                }
            }
        }
        str_free(token);
    }

    str_free(fen_string);

    return (Board){.size=64, pieces=pieces, .current=WHITE, .king_pos={black_king_pos, white_king_pos}};
}

void board_print(Board *board) {
    for (size_t i = 0; i < board->size; i++) {
        size_t x = i % 8;
        size_t y = i / 8;
        if (board->pieces[i].index == i) {
            printf(" %c ", board->pieces[i].repr);
            if (x == 7) {
                printf("%d \n", (8 - (i + 1) / 8) + 1);
            }
        }
    }
    printf(" a  b  c  d  e  f  g  h\n");
}

void board_free(Board *board) {
    free(board->pieces);
}

typedef struct {
    Move *moves;
    size_t length;
} Moves;

bool add_move(Move move, Moves *moves, Piece piece, Board *board) {
    PieceColor color = piece.color;
    int x = move.end % 8;
    int y = move.end / 8;
    if (x >= 0 && x < 8 && y >= 0 && y < 8) {
        if (move.capture.color == COLOR_NONE) {
            moves->moves[moves->length] = move;
            moves->length++;
            return false; // no need to break
        }
        else if (move.capture.color == piece.color) {
            return true; // we ran into a friendly piece, the move is invalid!
        }
        else {
            if (move.can_capture) {
                moves->moves[moves->length] = move;
                moves->length++;
                return true; // we hit an enemy piece, allow the move (capture) but no further moves in that direction
            }
            return true; // if the piece can't capture and we have run into an enemy piece we must break immediately!
        }
    }
    return true;
}

void generate_horizontal_moves(Board *board, Moves *moves, Piece piece, int distance) {
    for (int dir = -1; dir <= 1; dir+=2) {
        for (int i = 1; i < distance; i++) {
            Move move = move_new(piece.index, piece.index + i * dir, board->pieces[piece.index + i * dir], true);
            int x = move.end % 8;
            int y = move.end / 8;

            if (y == piece.index / 8) {
                if (add_move(move, moves, piece, board)) {
                    break;
                }
            }
        }
    }
}

void generate_vertical_moves(Board *board, Moves *moves, Piece piece, int distance) {
    for (int dir = -1; dir <= 1; dir+=2) {
        for (int i = 1; i < distance; i++) {
            Move move = move_new(piece.index, piece.index + i * (dir * 8), board->pieces[piece.index + i * (dir * 8)], true);
            int x = move.end % 8;
            int y = move.end / 8;

            if (y >= 0 && y < 8) {
                if (x == piece.index % 8) {
                    if (add_move(move, moves, piece, board)) {
                        break;
                    }
                }
            }
        }
    }
}

void generate_diagonal_moves(Board *board, Moves *moves, Piece piece, int distance) {
    int start_x = piece.index % 8;
    int start_y = piece.index / 8;
    for (int dir = -1; dir <= 1; dir+=2) {
        for (int i = 1; i < distance; i++) {
            int x = start_x + i * dir;
            int y = start_y + i * dir;
            
            if (x >= 0 && x < 8 && y >= 0 && y < 8) {
                int index = y * 8 + x;
                Move move = move_new(piece.index, index, board->pieces[index], true);

                if (add_move(move, moves, piece, board)) {
                    break;
                }
            }
        }
    }
    for (int dir = -1; dir <= 1; dir+=2) {
        for (int i = 1; i < distance; i++) {
            int x = start_x - i * dir;
            int y = start_y + i * dir;
            
            if (x >= 0 && x < 8 && y >= 0 && y < 8) {
                int index = y * 8 + x;
                Move move = move_new(piece.index, index, board->pieces[index], true);

                if (add_move(move, moves, piece, board)) {
                    break;
                }
            }
        }
    }
}

void generate_knight_move(Board *board, Moves *moves, Piece piece, int dx, int dy) {
    int x = INDEX_TO_COORD_X(piece.index) + dx;
    int y = INDEX_TO_COORD_Y(piece.index) + dy;
    if (x >= 0 && x < 8 && y >= 0 && y < 8) {
        size_t target_index = COORD_TO_INDEX(x, y);
        if (target_index >= 0) {
            Move move = move_new(piece.index, target_index, board->pieces[target_index], true);
            add_move(move, moves, piece, board);
        }
    }
}

void generate_moves_for_piece(Board *board, Moves *moves, Piece piece);

bool king_in_check(Board *board, PieceColor color) {
    // we will soon speed up this algorithm
    for (size_t i = 0; i < 64; i++) {
        Piece piece = board->pieces[i];
        if (piece.color != color) { //this is an enemy piece to the color, it could be checking us!
            Moves moves = (Moves){.moves=malloc(sizeof(Move) * MAX_MOVES), .length=0};
            generate_moves_for_piece(board, &moves, piece);
            for (size_t j = 0; j < moves.length; j++) {
                if (moves.moves[j].end == board->king_pos[color]) {
                    free(moves.moves);
                    return true;
                } 
            }
            free(moves.moves);
        }
    }
    return false;
}   

Moves generate_moves(Board *board) {
    Moves moves = (Moves){.moves=malloc(sizeof(Move) * MAX_MOVES), .length=0};
    for (size_t i = 0; i < 64; i++) {
        Piece piece = board->pieces[i];
        if (piece.color == board->current) {
            generate_moves_for_piece(board, &moves, piece);
        }
    }
    return moves;
}

void generate_moves_for_piece(Board *board, Moves *moves, Piece piece) {
    switch (piece.type) {
        case QUEEN:
            generate_vertical_moves(board, moves, piece, 8);
            generate_horizontal_moves(board, moves, piece, 8);
            generate_diagonal_moves(board, moves, piece, 8);
            break;
        case ROOK:
            generate_vertical_moves(board, moves, piece, 8);
            generate_horizontal_moves(board, moves, piece, 8);
            break;
        case BISHOP:
            generate_diagonal_moves(board, moves, piece, 8);
            break;
        case KING:
            generate_vertical_moves(board, moves, piece, 2);
            generate_horizontal_moves(board, moves, piece, 2);
            generate_diagonal_moves(board, moves, piece, 2);
            break;
        case KNIGHT:
            generate_knight_move(board, moves, piece, 1, -2);
            generate_knight_move(board, moves, piece, -1, -2);

            generate_knight_move(board, moves, piece, 1, 2);
            generate_knight_move(board, moves, piece, -1, 2);

            generate_knight_move(board, moves, piece, 2, -1);
            generate_knight_move(board, moves, piece, -2, -1);

            generate_knight_move(board, moves, piece, 2, 1);
            generate_knight_move(board, moves, piece, -2, 1);
            break;
        case PAWN:
            int direction = piece.color == WHITE ? 1 : -1;

            int x = INDEX_TO_COORD_X(piece.index);
            int y = INDEX_TO_COORD_Y(piece.index);
            size_t target_index = COORD_TO_INDEX(x, (y - direction));
            Move move = move_new(piece.index, target_index, board->pieces[target_index], false);
            add_move(move, moves, piece, board);

            Piece one_above = board->pieces[target_index];

            if (x + 1 < 8) {
                Piece right = board->pieces[COORD_TO_INDEX(x + 1, (y - direction))];
                if (right.type != NONE) {
                    target_index = COORD_TO_INDEX(x + 1, (y - direction));
                    Move move = move_new(piece.index, target_index, board->pieces[target_index], true);
                    add_move(move, moves, piece, board);   
                }
            }

            if (x - 1 >= 0) {
                Piece left = board->pieces[COORD_TO_INDEX(x - 1, (y - direction))];
                if (left.type != NONE) {
                    target_index = COORD_TO_INDEX(x - 1, (y - direction));
                    Move move = move_new(piece.index, target_index, board->pieces[target_index], true);
                    add_move(move, moves, piece, board);   
                }
            }

            if (one_above.color == COLOR_NONE) {
                if (piece.color == WHITE && INDEX_TO_COORD_Y(piece.index) == 6) {
                    x = INDEX_TO_COORD_X(piece.index);
                    y = INDEX_TO_COORD_Y(piece.index) - direction * 2;
                    target_index = COORD_TO_INDEX(x, y);

                    Move move = move_new(piece.index, target_index, board->pieces[target_index], false);
                    add_move(move, moves, piece, board);   
                }

                if (piece.color == BLACK && INDEX_TO_COORD_Y(piece.index) == 1) {
                    x = INDEX_TO_COORD_X(piece.index);
                    y = INDEX_TO_COORD_Y(piece.index) - direction * 2;
                    target_index = COORD_TO_INDEX(x, y);

                    Move move = move_new(piece.index, target_index, board->pieces[target_index], false);
                    add_move(move, moves, piece, board);   
                }
            }

            break;
    }
}

Moves filter_legal_moves(Board *board, Moves moves) {
    PieceColor current_color = board->current;
    Moves legal_moves = (Moves){.moves=malloc(sizeof(Move) * MAX_MOVES), .length=0};

    for (size_t i = 0; i < moves.length; i++) {
        make_move(moves.moves[i], board);

        if (!king_in_check(board, current_color)) {
            legal_moves.moves[legal_moves.length] = moves.moves[i];
            legal_moves.length++;
        }

        unmake_move(moves.moves[i], board);    
    }

    free(moves.moves);
    return legal_moves;
}


long unsigned int board_perft(Board *board, size_t depth) {
    // board_print(board);
    // printf("\n");
    Moves moves = filter_legal_moves(board, generate_moves(board));
    long unsigned int nodes = 0;

    if (depth == 1) {
        free(moves.moves);
        return (long unsigned int)moves.length;
    }

    for (int i = 0; i < moves.length; i++) {
        make_move(moves.moves[i], board);
        long unsigned int num_nodes = board_perft(board, depth - 1);
        nodes += num_nodes;

        if (depth == 3) {
            int startx = INDEX_TO_COORD_X((moves.moves[i].start));
            int starty = INDEX_TO_COORD_Y((moves.moves[i].start));

            int endx = INDEX_TO_COORD_X((moves.moves[i].end));
            int endy = INDEX_TO_COORD_Y((moves.moves[i].end));
            // printf("%c%d%c%d: %ld\n", startx + 97, 8 - starty, endx + 97, 8 - endy, num_nodes);
        }

        unmake_move(moves.moves[i], board);
    }

    free(moves.moves);

    return nodes;
}

void board_play(Board *board) {
    char line[256];

    while (true) {
        board_print(board);
        printf("%s >>> ", board->current ? "white" : "black");
        fgets(line, sizeof(line), stdin);
        printf("\n");
        char piece_row = line[0];
        char piece_col = line[1];

        char target_row = line[2];
        char target_col = line[3];

        size_t start = COORD_TO_INDEX(((piece_row - 97)), (7 - (piece_col - 49)));
        size_t end = COORD_TO_INDEX(((target_row - 97)), (7 - (target_col - 49)));

        Moves moves = filter_legal_moves(board, generate_moves(board));

        for (int i = 0; i < moves.length; i++) {
            if (moves.moves[i].start == start && moves.moves[i].end == end) {
                make_move(moves.moves[i], board);
            }
        }
        free(moves.moves);
    }
}

int main(void) {
    printf("Welcome to Starfish!\n");

    Board board = board_init("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    printf("perft(5) = %ld\n", board_perft(&board, 5));
    
    board_free(&board);
    return 0;
}