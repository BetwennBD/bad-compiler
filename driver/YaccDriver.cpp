//
// Created by tangny on 2021/12/29.
//

#include <iostream>
#include <ctime>
#include "include/Yacc/Grammar.h"
#include "include/Yacc/GrammarParser.h"
#include "include/Yacc/Constructor.h"
#include "include/ASTConsumers/IRGenerator.h"

int main() {
    clock_t start, end;
    start = clock();
    GrammarSet grammarSet;
    parseProducer(grammarSet, "../etc/yacc_c99.y");
    LALRconstructor lalrConstructor(&grammarSet);
    lalrConstructor.constructLR0Core();
    lalrConstructor.printItemSet("../output/item_set.txt");
    lalrConstructor.labelPropagate();
    lalrConstructor.extend();
    lalrConstructor.printItemSet("../output/item_set.txt");
    lalrConstructor.generateTable("../output/conflict.txt");
    lalrConstructor.printTable("../etc/action.txt", "../etc/goto.txt");

    end = clock();
    std::cout << "Total LALR takes " << double(end - start) / CLOCKS_PER_SEC << "s." << std::endl;
}