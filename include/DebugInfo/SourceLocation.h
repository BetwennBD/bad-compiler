//
// Created by tangny on 2021/12/30.
//

#ifndef BAD_COMPILER_SOURCELOCATION_H
#define BAD_COMPILER_SOURCELOCATION_H

class SourceLocation {
public:
    int row, col;

    SourceLocation(int _row = -1, int _col = -1)
    : row(_row), col(_col) {}
};

#endif //BAD_COMPILER_SOURCELOCATION_H
