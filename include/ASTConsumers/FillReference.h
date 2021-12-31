//
// Created by Shakira on 2021/12/21.
//

#ifndef FRONTEND_FILLREFERENCE_H
#define FRONTEND_FILLREFERENCE_H
#include <stack>
#include<string>
#include<map>
#include<iostream>
#include<fstream>
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
    std::ofstream outerror;
    std::ofstream zqtest;
    bool success=true;
    FillReference()
            : RecursiveASTVisitor()
    {
        outerror.open("../output/OutforErrors.txt", std::ios::out);
        zqtest.open("../output/zqtest.txt", std::ios::out);
        zqtest.close();
        zqtest.open("../output/zqtest.txt", std::ios::app);
        zqtest<<"fillreference************\n";
        tables.clear();
    }
    bool visitTranslationUnitDecl(TranslationUnitDecl*D)
    {
        curRoot=D;
        zqtest<<"alloha"<<std::endl;
        return true;
    }
    bool visitVarDecl(VarDecl* D)
    {
        //变量声明，考虑了全局和局部两种情况
        //针对用函数返回值声明数组大小的时候做的检查
        Type* tempType=D->getQualType().getType();
        while(tempType->getKind()!=Type::k_BuiltInType
              ||tempType->getKind()!=Type::k_UnknownType)
        {
            if(tempType->getKind()==Type::k_VariableArrayType)
            {
                Expr*curSizeExpr= dynamic_cast<VariableArrayType*>(tempType)->getSizeExpr();
                if(curSizeExpr->getKind()==Expr::k_CallExpr)
                {
                    for(int i=0;i!=allFunctions.size();++i)
                    {
                        FunctionDecl  temp= allFunctions[i];
                        std::string qname=temp.getName();
                        Expr* qcallfunc= dynamic_cast<CallExpr* >(curSizeExpr)->getArg(0);
                        std::string qcallname= dynamic_cast<DeclRefExpr*>(qcallfunc)->getRefName();
                        if(qcallname==qname)
                        {
                            bool curFlag=false;
                            Type * curTemp=allFunctions[i].getQualType().getType();
                            if(curTemp->getKind()==Type::k_BuiltInType)
                            {
                                if(dynamic_cast<BuiltInType*>(curTemp)->getTypeType()==BuiltInType::_int)
                                    curFlag=true;
                            }
                            if(!curFlag)
                            {
                                outerror<<"Error, array size should be an integer\n";
                                success=false;
                            }
                        }
                    }

                }
                tempType= dynamic_cast<VariableArrayType*>(tempType)->getElementType();
            }
            else if(tempType->getKind()==Type::k_PointerType)
            {
                tempType= dynamic_cast<PointerType*>(tempType)->getPointeeType()->getType();
            }
            else
                break;
        }
        std::string curName=D->getName();
        QualType curType=D->getQualType();
        if(fornum==fornew)
        {
            if(curRoot->checkSymbol(curName,curType))
            {
                success=false;
               // outerror<<"Error,"<<curName<<" has already declared!\n";
                return true;
            }
            curRoot->addSymbol(curName, curType);
            if(!tables.empty())
            {
                curFunction = tables[tables.size() - 1];
                if(curFunction->checkSymbol(curName,curType))
                {
                    success=false;
                    //outerror<<"Error,"<<curName<<" has already declared!\n";
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
               // outerror<<"Error,"<<curName<<" has already declared!\n";
                success=false;
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
           // outerror<<"Error,"<<curName<<" has already declared!\n";
            success=false;
            return true;
        }
        dynamic_cast<DeclContext*>(curFunction)->addSymbol(curName,curType);
        return true;
    }
    bool visitFunctionDecl(FunctionDecl* D)
    {
        tables.push_back(D);
        allFunctions.push_back(*D);
        zqtest<<"let us add a new function "<<D->getName()<<std::endl;
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
        zqtest<<"now in declref, "<<curName<<"\n";
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
                        zqtest<<"it is a function ";
                        Type* t=curType.getType();
                        zqtest<<dynamic_cast<BuiltInType*> (t)->getTypeTypeAsString()<<"\n";
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
            zqtest<<curName<<" is LValue\n";
        }
        else
        {
           outerror<<"Error,"+curName+" is an invalid reference\n";
            success=false;
            BuiltInType* newType=new BuiltInType(BuiltInType::_long);
            QualType newQualType(newType);
            newQualType.setConst();
            E->setType(newQualType);
        }
        return  true;
    }
    bool visitSelectorArray(SelectorArray*E)
    {
        if(!E->hasSubExpr())
        {
            outerror<<"Error, this SelectorArray doesn't hava a subexpr\n";
            success=false;
            return true;
        }
        int numSelectors=E->getNumSelectors();
        std::vector<Selector*> curSelectors=E->getSelectors();
        Expr* curSub=E->getSubExpr();
        for(int i=0;i!=numSelectors;++i)
        {
            Selector * curSelector=curSelectors[i];
            switch (curSelector->getKind())
            {
                case Expr::k_DerefSelector:
                {
                    curSub=curSelector->getSubExpr();
                    break;
                }
                case Expr::k_IndexSelector:
                {
                    Expr* curidx=dynamic_cast<IndexSelector*>
                            (curSelector)->getIdxExpr();
                    if(curidx->getKind()==Expr::k_CallExpr) {
                        for (int i = 0; i != allFunctions.size(); ++i) {
                            FunctionDecl temp = allFunctions[i];
                            std::string qname = temp.getName();
                            Expr *qcallfunc = dynamic_cast<CallExpr * >(curidx)->getArg(0);
                            std::string qcallname = dynamic_cast<DeclRefExpr *>(qcallfunc)->getRefName();
                            if (qcallname == qname) {
                                bool curFlag = false;
                                Type *curTemp = allFunctions[i].getQualType().getType();
                                if (curTemp->getKind() == Type::k_BuiltInType) {
                                    if (dynamic_cast<BuiltInType *>(curTemp)->getTypeType() == BuiltInType::_int)
                                        curFlag = true;
                                }
                                if (!curFlag) {
                                    success=false;
                                    outerror << "Error, array index should be an integer\n";
                                }
                            }
                        }
                    }
                    curSub=curSelector->getSubExpr();
                    break;
                }
                case Expr::k_FieldSelector:
                {
                    curSub=curSelector->getSubExpr();
                    break;
                }
            }
        }


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
    ~FillReference()
    {
        curRoot->clearSymbolTable();
        outerror.close();
        zqtest<<"\n";
        zqtest.close();
    }
};
#endif //FRONTEND_FILLREFERENCE_H
