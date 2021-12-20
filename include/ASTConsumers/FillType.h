//
// Created by Shakira on 2021/12/19.
//

#ifndef FRONTEND_FILLTYPE_H
#define FRONTEND_FILLTYPE_H
#include <stack>
#include<string>
#include<map>
#include<iostream>
#include"include/AST/Expr.h"
#include "include/AST/Stmt.h"
#include"include/AST/Type.h"
#include "include/AST/RecursiveASTVisitor.h"
class FillType : public RecursiveASTVisitor<FillType> {
    //自底向上
    bool traverseInPreOrder() const { return false; }
public:

}
#endif //FRONTEND_FILLTYPE_H
