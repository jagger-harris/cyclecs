#ifndef ECS_COMPONENT_BOARD_H
#define ECS_COMPONENT_BOARD_H

enum player { NONE = 0, X, O };

struct board {
    enum player board[9];
    enum player owner;
};

#endif // ECS_COMPONENT_BOARD_H
