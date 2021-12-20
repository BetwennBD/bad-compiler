//
// Created by Shakira on 2021/12/19.
//

#ifndef FRONTEND_CALCULATECONSTANT_H
#define FRONTEND_CALCULATECONSTANT_H
#include <stack>
#include<iostream>
#include "include/AST/RecursiveASTVisitor.h"
//主要作用是计算所有的常数表达式，对于除零或者越界的情况warning
//针对各种operator进行计算，intliteral,binaryoperator
class CalculateConstant : public RecursiveASTVisitor<CalculateConstant> {
private:

public:

};
#endif //FRONTEND_CALCULATECONSTANT_H
