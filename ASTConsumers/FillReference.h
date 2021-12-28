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
    //为struct而设置的
    std::map<RecordType*,std::vector<std::string>> recordMembers;
    std::map<std::string,RecordType*>recordRef;
    std::map<RecordType*,std::vector<QualType>> recordMemberQTs;
    int fornum=0;
    int fornew=0;
    std::ofstream outerror;
    FillReference()
            : RecursiveASTVisitor()
    {
        outerror.open("../output/OutforErrors.txt", std::ios::out);
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
        //变量声明，考虑了全局和局部两种情况
        std::string curName=D->getName();
        QualType curType=D->getQualType();
        //清理unkonwnType,记得分清楚返回的是指针还是拷贝
        QualType newQT=D->getQualType();
        Type * tempType=newQT.getType();
        Type* hh=new Type();
        while(tempType->getKind()!=Type::k_BuiltInType
        ||tempType->getKind()!=Type::k_UnknownType)
        {
            if(tempType->getKind()==Type::k_VariableArrayType)
            {
                hh=tempType;
                tempType= dynamic_cast<VariableArrayType*>(tempType)->getElementType();
            }
            else if(tempType->getKind()==Type::k_PointerType)
            {
                hh=tempType;
                tempType= dynamic_cast<PointerType*>(tempType)->getPointeeType()->getType();
            }
            else
                break;
        }
        if(tempType->getKind()==Type::k_UnknownType)
        {
            std::cout<<"test: vardecl changes unknown type\n";
            std::string unknownName= dynamic_cast<UnknownType*>(tempType)->getName();
            if(recordRef.find(unknownName)!=recordRef.end())
            {
               /*RecordType* A=new RecordType();
               RecordType* B=recordRef.find(unknownName)->second;
               A->name=B->getName();
               for(int i=0;i!=B->getNumMembers();++i)
               {
                   A->addMember(B->getMember(i));
               }
               if(B->isStruct())
               A->setStruct();
               dynamic_cast<VariableArrayType*>(hh)->setElementType(A);
               D->setQualType(newQT);*/
            }
            else
            {
                //无名struct，后面再想想咋整
            }
        }
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
            outerror<<"Error,"+curName+" is an invalid reference\n";
            BuiltInType* newType=new BuiltInType(BuiltInType::_long);
            QualType newQualType(newType);
            newQualType.setConst();
            E->setType(newQualType);
        }
        return  true;
    }
    bool visitRecordDecl(RecordDecl* D)
    {
        std::cout<<"test: visit record decl\n";
        if(D->isStruct())
        {
            RecordType* curRecordType=new RecordType(D->isStruct());
            std::vector<std::string> memberNames;
            std::vector<QualType>memberQTs;
            for(int i=0;i!=D->getNumDeclStmts();++i)
            {
                DeclStmt* curDeclStmt=D->getDeclStmt(i);
                for(int j=0;j!=curDeclStmt->getNumDecls();++j)
                {
                    VarDecl* curVarDecl=curDeclStmt->getDecl(j);
                    curRecordType->addMember(curVarDecl->getQualType());
                    memberNames.push_back(curVarDecl->getName());
                    memberQTs.push_back(curVarDecl->getQualType());
                }
            }
            if(D->hasTag())
            {
                recordMembers.insert(std::make_pair(curRecordType,memberNames));
                recordRef.insert(std::make_pair(D->getName(),curRecordType));
                recordMemberQTs.insert(std::make_pair(curRecordType,memberQTs));
            }
            else
            {
                //todo:对于无名struct
            }
        }
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
    ~FillReference()
    {
        outerror.close();
    }
};
#endif //FRONTEND_FILLREFERENCE_H
