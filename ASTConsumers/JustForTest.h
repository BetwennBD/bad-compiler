//
// Created by Shakira on 2021/12/21.
//

#ifndef FRONTEND_JUSTFORTEST_H
#define FRONTEND_JUSTFORTEST_H
#include <stack>
#include<string>
#include<map>
#include<iostream>
#include"include/AST/Expr.h"
#include "include/AST/Stmt.h"
#include"include/AST/Type.h"
#include "include/AST/RecursiveASTVisitor.h"
class JustForTest : public RecursiveASTVisitor<JustForTest> {
public:
    bool walkUpToTop() const { return false; }

    JustForTest()
            : RecursiveASTVisitor() {
        std::cout<<"Let us start test through printing******************\n";
    }
    bool visitTranslationUnitDecl(TranslationUnitDecl*D)
    {
        std::cout<<"ROOT~"<<std::endl;
        return true;
    }
    bool visitDeclRefExpr(DeclRefExpr*E)
    {
        std::cout<<"DeclRefExpr  ";
        QualType qt=E->getQualType();
        Type* t=qt.getType();
        std::cout<<E->getRefName()<<" ";
        std::cout<<dynamic_cast<BuiltInType*> (t)->getTypeTypeAsString()<<"\n";
        return true;
    }

    bool visitUnaryOperator(UnaryOperator* E)
    {
       std::string opnum[10]={ "_pre_inc","_post_inc"," _pre_dec",
                " _post_dec","_address_of"," _indirection"," _not"," _bit_not",
                "_unary_plus","_unary_minus" };
        QualType qt=E->getQualType();
        Type* t=qt.getType();
        std::cout<<"UnaryOperator "<<opnum[E->getOp()]<<" ";
        std::cout<<dynamic_cast<BuiltInType*> (t)->getTypeTypeAsString()<<"\n";
        return true;
    }
    bool visitBinaryOperator(BinaryOperator* E)
    {
        std::string opnum[29]={ "_add","_sub","_mul","_div","_mod","_and","_or","_xor",
                                "_lsh"," _rsh","_eq","_ne","_lt","_gt","_le","_ge","_log_and",
                                "_log_or","_assign","_add_assign","_sub_assign","_mul_assign",
                                "_div_assign","_mod_assign","_and_assign","_or_assign",
                                "_xor_assign","_lsh_assign","_rsh_assign"};
        QualType qt=E->getQualType();
        Type* t=qt.getType();
        std::cout<<"BinaryOperator "<<opnum[E->getOp()]<<" ";
        std::cout<<dynamic_cast<BuiltInType*> (t)->getTypeTypeAsString()<<"\n";
        return true;
    }
    bool visitIntegerLiteral(IntegerLiteral* E)
    {
        QualType qt=E->getQualType();
        Type* t=qt.getType();
        std::cout<<"IntegerLiteral ";
        std::cout<<dynamic_cast<BuiltInType*> (t)->getTypeTypeAsString()<<" ";
        std::cout<<E->getValue()<<"\n";
        return true;
    }
    bool visitFloatingLiteral(FloatingLiteral* E)
    {
        QualType qt=E->getQualType();
        Type* t=qt.getType();
        std::cout<<"FloatingLiteral ";
        std::cout<<dynamic_cast<BuiltInType*> (t)->getTypeTypeAsString()<<" ";
        std::cout<<E->getValue()<<"\n";
        return true;
    }
};

#endif //FRONTEND_JUSTFORTEST_H
