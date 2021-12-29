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
#include "include/ASTConsumers/JustForTest.h"
#include "include/ASTConsumers/ASTTypeCheck.h"
#include "include/ASTConsumers/CalculateConstant.h"
#include  "include/Lexer/Lexer.h"
//#include "include/ASTConsumers/IRGenerator.h"

std::vector<LexUnit> constructTestCase( const char* );

int main() {
    clock_t start, end;
    start = clock();
    GrammarSet grammarSet;
    parseProducer(grammarSet, "../etc/yacc_c99.y");
    LALRconstructor lalrConstructor(&grammarSet);
    lalrConstructor.constructLR0Core();
    lalrConstructor.labelPropagate();
    lalrConstructor.extend();
    lalrConstructor.printItemSet("../output/item_set.txt");
    lalrConstructor.generateTable("../output/conflict.txt");
    lalrConstructor.printTable("../output/action.txt", "../output/goto.txt");
    end = clock();
//    std::cout << "Total LALR takes " << double(end - start) / CLOCKS_PER_SEC << "s." << std::endl;

    CSTBuilder cstBuilder(&lalrConstructor, &grammarSet);
    std::vector<LexUnit> testVec = constructTestCase("../etc/tokens4.txt");

    CSTNode *head = cstBuilder.constructCST(testVec, 0);

    if(head == NULL)
        std::cout << "create CST failed";

    ASTBuilder astBuilder;
    TranslationUnitDecl* translationUnitDecl = astBuilder.constructAST(head);
    if(translationUnitDecl == NULL)
        std::cout << "create AST failed";

    std::cout<<"fillreference\n";
     FillReference fillReference;
    fillReference.traverseTranslationUnitDecl(translationUnitDecl);
    std::cout<<"filltype\n";
     FillType fillType;
    fillType.traverseTranslationUnitDecl(translationUnitDecl);
     std::cout<<"typecheck\n";
     ASTTypeCheck astTypeCheck;
     astTypeCheck.traverseTranslationUnitDecl(translationUnitDecl);
    std::cout<<"calculateConstant\n";
     CalculateConstant calculateConstant;
     calculateConstant.traverseTranslationUnitDecl(translationUnitDecl);
    std::cout<<"dumper\n";
    ASTDumper astDumper;
    astDumper.traverseTranslationUnitDecl(translationUnitDecl);
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
