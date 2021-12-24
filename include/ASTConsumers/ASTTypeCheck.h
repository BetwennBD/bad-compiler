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
    std::vector<FunctionDecl>allFunctions;
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
        curRoot->clearSymbolTable();
        return true;
    }
    //以下几个visit只涉及符号表的构造，无类型检查
    //VarDecl ParamVarDecl FunctionDecl CompoundStmt ForStmt
    bool visitVarDecl(VarDecl* D)
    {
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
            std::cout<<"ForStmt\n";
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
    //变量的声明
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
    bool visitReturnStmt(ReturnStmt* S)
    {
        //理论上，ReturnStmt里只有expr信息，它在fiiltype里面已经被填充好类型了
        if (S->hasRetValue())
        {
            curFunction = tables[tables.size() - 1];
            QualType shouldReturn = curFunction->getQualType();
            QualType actualReturn = S->getRetValue()->getQualType();
            if (canBeCasted(actualReturn,shouldReturn))
            {
                std::cout<<"A successful return in "<<curFunction->getName()<<"\n";
                //不必向下检查了
                return false;
            }
        }
        else
        {
            //当前不返回，那么函数必须是void的
            curFunction = tables[tables.size() - 1];
            QualType shouldReturn =curFunction->getQualType();
            QualType* shouldPointer=& shouldReturn;
            if (dynamic_cast<BuiltInType*>(shouldPointer)->typeEnum!= BuiltInType::_void)
            {
                std::cout << "Error, please check your return type " << std::endl;
                return false;
            }
        }
    }
    bool visitDeclRefExpr(DeclRefExpr*E)
    {
        //fillreference.h里已经填充好类型，而且它也不需要检查
       return true;
    }
    //检查重灾区——CallExpr
    bool visitCallExpr(CallExpr* E)
    {
        //获取函数名称和类型
        if(E->getArg(0)->getKind()!=Stmt::k_DeclRefExpr)
        {
            std::cerr<<"Error, check if the first parameter is declrefexpr"<<std::endl;
            return true;
        }
        //call的原函数，declrefexpr已经将自己的type变成函数的rettype了，可以放心使用
        DeclRefExpr* function= dynamic_cast<DeclRefExpr* >(E->getArg(0));
        std::string funcName=function->getRefName();
        QualType funcType=function->getQualType();
        E->setType(funcType);
        //allFucntions里面找到这个函数，并get到它的参数类型，与调用时的参数类型比对
        for(int i=0;i!=allFunctions.size();++i)
        {
            FunctionDecl curFunc=allFunctions[i];
            std::string curName=curFunc.getName();
            QualType curType=curFunc.getQualType();
            if(curName==funcName&&E->getNumArgs()==curFunc.getNumParams()+1)
            {
                bool flag=true;
                for(int j=0;j!=curFunc.getNumParams();++j)
                {
                    ParamVarDecl*D=curFunc.getParam(j);
                    QualType paraType = D->getQualType();
                    Expr* actualPara=E->getArg(j+1);
                    QualType actualType=actualPara->getQualType();
                    if(!canBeCasted(actualType,paraType))
                    {
                        flag=false;
                        break;
                    }
                }
                //若函数参数全部匹配上了
                if(flag)
                {
                    //todo:不全或者有简化
                    if(canBeCasted(funcType,curType))
                    {
                        //此处是函数转指针
                        PointerType* imType=new PointerType(&curType);
                        QualType imQualType(imType);
                        CopyQual(curType,imQualType);
                        std::cout<<"function implicit *************************\n";
                        ImplicitCastExpr* curImplicit=new ImplicitCastExpr(imQualType,E->getArg(0));
                        E->setArg(0,curImplicit);
                    }
                    for(int j=0;j!=curFunc.getNumParams();++j)
                    {
                        ParamVarDecl*D=curFunc.getParam(j);
                        QualType paraType = D->getQualType();
                        Expr* actualPara=E->getArg(j+1);
                        if(canBeCasted(actualPara->getQualType(),paraType))
                        {
                            //左转右
                            ImplicitCastExpr* curImplicit=new ImplicitCastExpr(paraType,actualPara);
                            curImplicit->setValueKind(ExprValueKind::RValue);
                            E->setArg(j+1,curImplicit);
                            std::cout<<"para implicitcast****************\n";
                        }
                    }
                    std::cout<<"A successful call "<<curName<<std::endl;
                    return true;
                }

            }
        }
        std::cout<<"Error,an invalid call function on "<<funcName<<"\n";
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
    void CopyQual(QualType a,QualType b)
    {
        //前面的qualier给后面
        if(a.isRestrict())
            b.setRestrict();
        if(a.isVolatile())
            b.setVolatile();
        if(a.isConst())
            b.setConst();
        if(a.isAtomic())
            b.setAtomic();
    }
    bool cleanupFunctionDecl(){
        //todo:如果不想导出符号表，这里可以clear，就行fillreference那样
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
#endif //FRONTEND_ASTTYPECHECK_H
