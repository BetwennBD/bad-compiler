//
// Created by tangny on 2021/12/6.
//

#include <string>
#include <iostream>
#include "include/Parser/CST.h"

LexUnit::LexUnit(){}

LexUnit::LexUnit(std::string _id, std::string _type, int _row, int _col)
: id(_id), type(_type), sourceLoc(_row, _col)
{}

CSTNode::CSTNode(){}

CSTNode::CSTNode(int _termType, std::string _id, std::string _type, SourceLocation _sourceLoc)
: termType(_termType), id(_id), type(_type), sourceLoc(_sourceLoc)
{
    familyPos = 0;
    // 默认监听器无动作
    listener = "Pass";
}

void deleteCST( CSTNode *node ) {
    if(node == NULL) return;
    for(auto ch : node->children)
        deleteCST(ch);
    delete node;
}

void printCST(CSTNode *node, int depth) {
    std::cout << std::string(depth, ' ');
    std::cout << '-' << node->getType();
    if(node->getId() != "")
        std::cout << " \'" << node->getId() << "\'";
    std::cout << std::endl;

    for(auto i : node->children)
        printCST(i, depth + 1);
}