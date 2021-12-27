//
// Created by Shakira on 2021/12/21.
//

#ifndef FRONTEND_FILLREFERENCE_H
#define FRONTEND_FILLREFERENCE_H
#include <stack>
#include<string>
#include<map>
#include<iostream>
#include"include/AST/Expr.h"
#include "include/AST/Stmt.h"
#include"include/AST/Type.h"
#include "include/AST/RecursiveASTVisitor.h"
class FillReference : public RecursiveASTVisitor<FillReference> {
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
    FillReference()
            : RecursiveASTVisitor()
    {
        tables.clear();
    }
    bool visitTranslationUnitDecl(TranslationUnitDecl*D)
    {
        curRoot=D;
        std::cout<<"alloha"<<std::endl;
        return true;
    }
    bool visitVarDecl(VarDecl* D)
    {
         short h=D->getQualType().getTypeKind();
        //变量声明，考虑了全局和局部两种情况
        std::string curName=D->getName();
        QualType curType=D->getQualType();
        if(fornum==fornew)
        {
            if (tables.empty())
            {
                curRoot->addSymbol(curName, curType);
            }
            else{
                curFunction = tables[tables.size() - 1];
                curFunction->addSymbol(curName, curType);
            }
        }
        else
        {
            std::cout<<"it is in for\n";
            fornew++;
            curFunction = tables[tables.size() - 1];
            curFunction->addSymbol(curName, curType);
        }
        return true;
    }
    bool visitParamVarDecl(ParamVarDecl* D)
    {
        curFunction=tables[tables.size() - 1];
        QualType curType = D->getQualType();
        std::string curName = D->getName();
        dynamic_cast<DeclContext*>(curFunction)->addSymbol(curName,curType);
        return true;
    }
    bool visitFunctionDecl(FunctionDecl* D)
    {
        tables.push_back(D);
        allFunctions.push_back(*D);
        std::cout<<"let us add a new function "<<D->getName()<<std::endl;
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
    bool visitDeclRefExpr(DeclRefExpr*E)
    {
        //这里是根据引用的原信息填充它的类型，不涉及检查类型
        bool flag=true;
        std::string curName=E->getRefName();
        std::cout<<"now in declref, "<<curName<<"\n";
        ValueDecl* v=E->getValueDecl();
        QualType curType;
        curFunction=tables[tables.size()-1];
        if(!curFunction->checkSymbol(curName,curType))
        {
            //其次，全局变量
            if(!curRoot->checkSymbol(curName,curType))
            {
                //再次，全局函数名
                for(int i=0;i!=allFunctions.size();++i)
                {
                    FunctionDecl  temp= allFunctions[i];
                    std::string qname=temp.getName();
                    curType=temp.getQualType();
                    if(curName!=qname)
                    {
                        flag=false;
                    }
                    else
                    {
                        std::cout<<"it is a function ";
                        Type* t=curType.getType();
                        std::cout<<dynamic_cast<BuiltInType*> (t)->getTypeTypeAsString()<<"\n";
                        flag=true;
                        break;
                    }

                }
            }
        }
        if(flag)
        {
            E->setType(curType);
            E->setValueKind(ExprValueKind::LValue);
            std::cout<<curName<<" is LValue\n";
        }
        else
        {
            //todo:为了后面方便filltype调试，先赋给不合法的引用一个类型，记得后面改成正经报错
            std::cout<<curName<<" is an invalid reference\n";
            BuiltInType* newType=new BuiltInType(BuiltInType::_long);
            QualType newQualType(newType);
            newQualType.setConst();
            E->setType(newQualType);
        }
        return  true;
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

};
#endif //FRONTEND_FILLREFERENCE_H
