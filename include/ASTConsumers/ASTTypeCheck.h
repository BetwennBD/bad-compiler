//
// Created by Shakira on 2021/12/19.
//

#ifndef FRONTEND_ASTTYPECHECK_H
#define FRONTEND_ASTTYPECHECK_H
#include <stack>
#include<string>
#include<map>
#include<iostream>
#include"include/AST/Expr.h"
#include "include/AST/Stmt.h"
#include"include/AST/Type.h"
#include "include/AST/RecursiveASTVisitor.h"
class ASTTypeCheck : public RecursiveASTVisitor<ASTTypeCheck> {
public:
    bool walkUpToTop() const { return false; }
    //符号表里面存储变量名和变量类型
    //tables空不空还能指示当前在函数里面还是全局
    std::vector<FunctionDecl*>tables;
    FunctionDecl* curFunction;
    TranslationUnitDecl* curRoot;
    int fornum=0;
    int fornew=0;
    ASTTypeCheck()
            : RecursiveASTVisitor()
    {
        tables.clear();
    }
    bool visitTranslationUnitDecl(TranslationUnitDecl*D)
    {
        curRoot=D;
        //函数为空，不需要typecheck
        std::cout<<"alloha"<<std::endl;
        return true;
    }
    bool visitVarDecl(VarDecl* D)
    {
        //变量声明，考虑了全局和局部两种情况
        std::string curName=D->getName();
        QualType curType=D->getQualType();
        std::cout<<curName<<std::endl;
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
        std::cout<<"varDecl\n";
        return true;
    }
    bool visitParamVarDecl(ParamVarDecl* D)
    {
        curFunction=tables[tables.size() - 1];
        QualType curType = D->getQualType();
        std::string curName = D->getName();
        dynamic_cast<DeclContext*>(curFunction)->addSymbol(curName,curType);
        std::cout<<"ParamVarDecl"<<std::endl;
        return true;
    }
    bool visitFunctionDecl(FunctionDecl* D)
    {
        tables.push_back(D);
        curRoot->addFunc(D);
        std::cout<<"let us add a new function "<<D->getName()<<std::endl;
        std::map<std::string, QualType>* firsttable=new std::map<std::string, QualType>();
        firsttable->clear();
        if(D->getNumParams()!=0)
            D->addSymbolTable(firsttable);
        std::cout<<"FunctionDecl"<<std::endl;
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
        std::cout<<"DeclStmt"<<std::endl;
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
    bool visitReturnStmt(ReturnStmt* S)
    {
        //todo：上次卡在这然后转向filltyoe
        if (S->hasRetValue())
        {
            curFunction = tables[tables.size() - 1];
            QualType shouldReturn = curFunction->getQualType();
            QualType actualReturn = S->getRetValue()->getType();
            if (canBeCasted(actualReturn,shouldReturn))
            {
                std::cerr << "Error, please check your return type " << std::endl;
                return true;
            }
            //自己递归计算expr
        }
        else
        {//当前不返回，那么函数必须是void的
            curFunction = tables[tables.size() - 1];
            QualType shouldReturn =curFunction->getQualType();
            QualType* shouldPointer=& shouldReturn;
            if (dynamic_cast<BuiltInType*>(shouldPointer)->typeEnum!= BuiltInType::_void)
            {
                std::cerr << "Error, please check your return type " << std::endl;
                return true;
            }
        }
        return true;
    }
    bool visitDeclRefExpr(DeclRefExpr*E)
    {
        //todo:这里有filltype的内容
        bool flag=true;
        std::string curName=E->getRefName();
        ValueDecl* v=E->getValueDecl();
        QualType curType;
        curFunction=tables[tables.size()-1];
        if(!curFunction->checkSymbol(curName,curType))
        {
            //其次，全局变量
            if(!curRoot->checkSymbol(curName,curType))
            {
                //再次，全局函数名
                for(int i=0;i!=curRoot->getNumFunc();++i)
                {
                    FunctionDecl* temp= dynamic_cast<FunctionDecl*>(curRoot->getFunc(i));
                    std::string qname=temp->getName();
                    curType=temp->getQualType();
                    if(curName!=qname)
                        flag=false;
                }
            }
        }
        std::cout<<"DeclRefExpr\n";
       if(flag)
       {
           E->setType(curType);
       }
        return  true;
    }
    bool visitCallExpr(CallExpr* E)
    {
        //获取函数名称和类型
        if(E->getArg(0)->getKind()!=Stmt::k_DeclRefExpr)
        {
            std::cerr<<"Error, check if the first parameter is declrefexpr"<<std::endl;
            return true;
        }
        DeclRefExpr* function= dynamic_cast<DeclRefExpr* >(E->getArg(0));
        std::string funcName=function->getRefName();
        QualType funcType=function->getType();
        //在globalcontext里面找到这个函数，并get到它的参数类型，与调用时的参数类型比对
        int h=curRoot->getNumFunc();
        for(int i=0;i!=curRoot->getNumFunc();++i)
        {
            FunctionDecl* curFunc=dynamic_cast<FunctionDecl*>(curRoot->getFunc(i));
            std::string curName=curFunction->getName();
            QualType curType=curFunction->getQualType();
            if(curName==funcName&&canBeCasted(funcType,curType))
            {
                //函数名字对了返回类型可以，这时候就该检查形参和实参。支持同名函数
                bool flag=true;
                std::cout<<"88888888888888888888888888\n";
                for(int j=0;j!=curFunc->getNumParams();++j)
                {
                    ParamVarDecl*D=curFunc->getParam(j);
                    QualType paraType = D->getQualType();
                    Expr* actualPara=E->getArg(j+1);
                    QualType actualType=actualPara->getType();
                    if(!canBeCasted(actualType,paraType))
                    {
                        flag=false;
                        break;
                    }
                }
                if(flag)
                {
                    std::cout<<"9999999999999999999999999\n";
                    //不需要管具体有什么类型转换，只需要判断是否可以cast然后在中间接上implicitcast节点
                    if(canBeCasted(funcType,curType)&&funcType!=curType)
                    {
                        ImplicitCastExpr* curImplicit=new ImplicitCastExpr(funcType,E->getArg(0));
                        E->setArg(0,curImplicit);
                    }
                    for(int j=0;j!=curFunc->getNumParams();++j)
                    {
                        ParamVarDecl*D=curFunc->getParam(j);
                        QualType paraType = D->getQualType();
                        Expr* actualPara=E->getArg(j+1);
                        if(canBeCasted(actualPara->getType(),paraType))
                        {
                            ImplicitCastExpr* curImplicit=new ImplicitCastExpr(paraType,actualPara);
                            E->setArg(j+1,curImplicit);
                            std::cerr<<"there is a iii"<<std::endl;
                        }
                    }
                    return true;
                }

            }
        }
        return true;
    }
    bool visitBinaryOperator(BinaryOperator* E)
    {
        std::cout<<"BinaryOperator\n";
        return true;
    }
    //判断第一个类型是否可以转化成第二个类型
    bool canBeCasted(QualType actual,QualType should)
    {
        //先提取出Type,不管Qualifier
        Type *aType=actual.getType();
        Type *sType=(should.getType());
        if(aType->getKind()==sType->getKind())
        {
            if(aType->getKind()==Type::k_BuiltInType)
            {
                short aBuilt=dynamic_cast<BuiltInType*>(aType)->getTypeType();
                short sBuilt=dynamic_cast<BuiltInType*>(sType)->getTypeType();
                if(aBuilt==sBuilt)
                    return true;
                if(aBuilt!=BuiltInType::_void)
                {
                    if(sBuilt==BuiltInType::_void)
                    {
                        std::cerr<<"Error, invalid implicitcast"<<std::endl;
                        return false;
                    }
                    return true;
                }
                else
                {
                    std::cerr<<"Error, you have to return a value"<<std::endl;
                    return false;
                }
            }
        }

    }
    bool cleanupFunctionDecl(){
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
#endif //FRONTEND_ASTTYPECHECK_H
