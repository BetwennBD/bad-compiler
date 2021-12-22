//
// Created by Shakira on 2021/12/21.
//
#ifndef FRONTEND_LEXER_H
#define FRONTEND_LEXER_H

#include<stdio.h>
#include<string>
#include<vector>
#include<stdio.h>
#include<fstream>
#include "include/Parser/CST.h"
struct Lexer{
    std::string sfile;
    Lexer(std::string ss)
    {
        sfile = ss;
    }
    std::string analysis(std::string yytext);
    std::vector<LexUnit> getAnalysis();
};

#endif //FRONTEND_LEXER_H
