//
// Created by Shakira on 2021/12/30.
//

#ifndef FRONTEND_ADDVARIMPLICIT_H
#define FRONTEND_ADDVARIMPLICIT_H
#include <stack>
#include<string>
#include<map>
#include<iostream>
#include<fstream>
#include"include/AST/Expr.h"
#include "include/AST/Stmt.h"
#include"include/AST/Type.h"
#include "include/AST/RecursiveASTVisitor.h"
class AddVarImplicit : public RecursiveASTVisitor<AddVarImplicit> {
public:
    bool walkUpToTop() const { return false; }
    //符号表里面存储变量名和变量类型
    //tables空不空还能指示当前在函数里面还是全局
    std::vector<FunctionDecl*>tables;
    std::vector<FunctionDecl>allFunctions;
    FunctionDecl* curFunction;
    TranslationUnitDecl* curRoot;
    int fornum=0;
    int fornew=0;
    bool success=true;
    std::ofstream outerror;
    std::ofstream zqtest;
    AddVarImplicit()
            : RecursiveASTVisitor()
    {
        outerror.open("../output/OutforErrors.txt", std::ios::app);
        zqtest.open("../output/zqtest.txt", std::ios::app);
        zqtest<<"varimplicit***************\n";
        tables.clear();
    }
    bool visitTranslationUnitDecl(TranslationUnitDecl*D)
    {
        curRoot=D;
        curRoot->clearSymbolTable();
        tables.empty();
        zqtest<<"alloha"<<std::endl;
        return true;
    }
    bool visitVarDecl(VarDecl* D)
    {
        if(D->getInitializer()!= nullptr)
        {
            QualType  leftType=D->getQualType();
            QualType rightType=D->getInitializer()->getQualType();
            if(leftType!=rightType||D->getInitializer()->isLValue())
            {
                ImplicitCastExpr* curImplicit=new ImplicitCastExpr(leftType,D->getInitializer());
                D->setInitializer(curImplicit);
            }
        }
        //变量声明，考虑了全局和局部两种情况
        //针对用函数返回值声明数组大小的时候做的检查
        std::string curName=D->getName();
        QualType curType=D->getQualType();
        if(fornum==fornew)
        {
            if(curRoot->checkSymbol(curName,curType))
            {
                success=false;
                //outerror<<"Error,"<<curName<<" has already declared!\n";
                return true;
            }
            curRoot->addSymbol(curName, curType);
            if(!tables.empty())
            {
                curFunction = tables[tables.size() - 1];
                if(curFunction->checkSymbol(curName,curType))
                {
                    success=false;
                   // outerror<<"Error,"<<curName<<" has already declared!\n";
                    return true;
                }
                curFunction->addSymbol(curName, curType);
            }
        }
        else
        {
            zqtest<<"it is in for\n";
            fornew++;
            curFunction = tables[tables.size() - 1];
            if(curFunction->checkSymbol(curName,curType))
            {
                success=false;
               // outerror<<"Error,"<<curName<<" has already declared!\n";
                return true;
            }
            curFunction->addSymbol(curName, curType);
        }
        return true;
    }
    bool visitParamVarDecl(ParamVarDecl* D)
    {
        curFunction=tables[tables.size() - 1];
        QualType curType = D->getQualType();
        std::string curName = D->getName();
        if(curFunction->checkSymbol(curName,curType))
        {
            success=false;
           // outerror<<"Error,"<<curName<<" has already declared!\n";
            return true;
        }
        dynamic_cast<DeclContext*>(curFunction)->addSymbol(curName,curType);
        return true;
    }
    bool visitFunctionDecl(FunctionDecl* D)
    {
        tables.push_back(D);
        allFunctions.push_back(*D);
        std::map<std::string, QualType>* firsttable=new std::map<std::string, QualType>();
        firsttable->clear();
        if(D->getNumParams()!=0)
        {
            D->addSymbolTable(firsttable);
        }
        return true;
    }
    bool visitCompoundStmt(CompoundStmt* S)
    {
        std::map<std::string, QualType>*newtable=new std::map<std::string, QualType>();
        curFunction=tables[tables.size()-1];
        curFunction->addSymbolTable(newtable);
        return true;
    }
    //局部变量的声明
    bool visitDeclStmt(DeclStmt* S)
    {
        //空的
        return true;
    }
    bool visitForStmt(ForStmt*S)
    {
        fornum++;
        std::map<std::string, QualType>*newtable=new std::map<std::string, QualType>();
        curFunction=tables[tables.size()-1];
        curFunction->addSymbolTable(newtable);
        return true;
    }

    bool visitBinaryOperator(BinaryOperator* E)
    {
        return true;
    }
    bool cleanupFunctionDecl(){
        tables[tables.size()-1]->clearSymbolTable();
        tables.erase(tables.end()-1);
        return true;
    }
    bool cleanupCompoundStmt()
    {
        curFunction=tables[tables.size()-1];
        curFunction->exitCurSymbolTable();
        return true;
    }
    bool cleanupForStmt()
    {
        fornum--;
        fornew--;
        curFunction=tables[tables.size()-1];
        curFunction->exitCurSymbolTable();
        return true;
    }
    bool isSuccessful()
    {
        return success;
    }
    ~AddVarImplicit()
    {
        curRoot->clearSymbolTable();
        outerror.close();
        zqtest.close();
    }
};
#endif //FRONTEND_ADDVARIMPLICIT_H
