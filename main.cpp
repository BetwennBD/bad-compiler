/*
#include <iostream>
#include <ctime>
#include "include/Yacc/Grammar.h"
#include "include/Yacc/GrammarParser.h"
#include "include/Yacc/Constructor.h"
#include "include/Parser/CST.h"
#include "include/Parser/CSTBuilder.h"
#include "include/AST/ASTBuilder.h"
#include "include/ASTConsumers/ASTDumper.h"
#include "include/ASTConsumers/FillType.h"
#include "include/ASTConsumers/FillReference.h"
#include "include/ASTConsumers/CalculateConstant.h"
#include "include/ASTConsumers/ASTTypeCheck.h"
#include  "include/Lexer/Lexer.h"
#include "include/ASTConsumers/IRGenerator.h"

std::vector<LexUnit> constructTestCase( const char* );

int main() {
    clock_t start, end;
    start = clock();
    GrammarSet grammarSet;
    parseProducer(grammarSet, "../etc/yacc_c99.y");
    LALRconstructor lalrConstructor(&grammarSet, 89, 68, 400);
//    LALRconstructor lalrConstructor(&grammarSet);
//    lalrConstructor.constructLR0Core();
//    lalrConstructor.printItemSet("../output/item_set.txt");
//    lalrConstructor.labelPropagate();
//    lalrConstructor.extend();
//    lalrConstructor.printItemSet("../output/item_set.txt");
//    lalrConstructor.generateTable("../output/conflict.txt");
//    lalrConstructor.printTable("../etc/action.txt", "../etc/goto.txt");
//    std::cout << lalrConstructor.numTerminal << ' ' << lalrConstructor.numNonTerminal << ' ' << lalrConstructor.numItem << std::endl;

    end = clock();
//    std::cout << "Total LALR takes " << double(end - start) / CLOCKS_PER_SEC << "s." << std::endl;

    CSTBuilder cstBuilder(&lalrConstructor, &grammarSet);
    //std::vector<LexUnit> testVec = constructTestCase("../etc/tokens4.txt");
    Lexer mylex("../etc/source.txt");
    std::vector<LexUnit> testVec=mylex.getAnalysis();
//    testVec = constructTestCase("../etc/tokens4.txt");

    CSTNode *head = cstBuilder.constructCST(testVec, 0);

    if(head == NULL)
        std::cout << "create CST failed";
//    printCST(head, 0);

    ASTBuilder astBuilder;
    TranslationUnitDecl* translationUnitDecl = astBuilder.constructAST(head);
    if(translationUnitDecl == NULL)
        std::cout << "create AST failed";

    FillReference fillReference;
    fillReference.traverseTranslationUnitDecl(translationUnitDecl);
    FillType fillType;
    fillType.traverseTranslationUnitDecl(translationUnitDecl);
    CalculateConstant calculateConstant;
    calculateConstant.traverseTranslationUnitDecl(translationUnitDecl);
    ASTTypeCheck astTypeCheck;
    astTypeCheck.traverseTranslationUnitDecl(translationUnitDecl);

    ASTDumper astDumper;
    astDumper.traverseTranslationUnitDecl(translationUnitDecl);

    IRGenerator irGenerator;
    irGenerator.emitLLVMIR(translationUnitDecl);
    deleteCST(head);
}

std::vector<LexUnit> constructTestCase(const char* filename) {
    std::ifstream inputFile(filename);
    std::string inputId, inputType;
    std::vector<LexUnit> vec;
    while(inputFile >> inputId) {
        inputFile >> inputType;
        vec.push_back(LexUnit(inputId, inputType));
    }
    return vec;
}
*/
