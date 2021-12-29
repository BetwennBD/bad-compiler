//
// Created by Shakira on 2021/12/21.
//
#ifndef FRONTEND_LEXER_H
#define FRONTEND_LEXER_H

#include<stdio.h>
#include<string>
#include<vector>
#include<fstream>
#include "include/Parser/CST.h"

class Lexer{
public:
    std::string analysis( std::string );
    std::vector<LexUnit> getAnalysis( std::string );
};

#endif //FRONTEND_LEXER_H
