//
// Created by tangny on 2021/12/10.
//

#ifndef FRONTEND_ASTDUMPER_H
#define FRONTEND_ASTDUMPER_H

#include <stack>
#include<iostream>
#include<cstdlib>
#include<cstdio>
#include "include/AST/RecursiveASTVisitor.h"

class ASTDumper : public RecursiveASTVisitor<ASTDumper> {
private:
    std::stack< AbstractASTNode* >recordLevel;

public:
    bool walkUpToTop() const { return false; }

    void outSpace()
    {
        int n = recordLevel.size();
        for (int i = 0; i != n; ++i)
            std::cout << " ";
        std::cout << "| ";
    }
    void outForSA(SelectorArray* E)
    {
        std::cout<<"(type)";
        return;
        //输出格式: int [30], *(students[t].name)
        std::string outFormat="";
        int numSelectors=E->getNumSelectors();
        std::vector<Selector*> curSelectors=E->getSelectors();
        //大概率是个declrefexpr，有name，输出students
        Expr* curSub=E->getSubExpr();
        if(curSub->getKind()==Expr::k_DeclRefExpr)
        {
            outFormat+= dynamic_cast<DeclRefExpr*>(curSub)->getRefName();
        }
        QualType curQualType=curSub->getQualType();
        for(int i=0;i!=numSelectors;++i)
        {
            Selector * curSelector=curSelectors[i];
            switch (curSelector->getKind())
            {
                case Expr::k_DerefSelector:
                {
                    outFormat="*("+outFormat+")";
                    break;
                }
                case Expr::k_IndexSelector:
                {
                    std::string myindex;
                    Expr * indexExpr=dynamic_cast<IndexSelector*>(curSelector)->getIdxExpr();
                    if(indexExpr->getKind()==Expr::k_IntegerLiteral)
                    {
                        int h=dynamic_cast<IntegerLiteral*>(indexExpr)->getValue();
                        myindex=std::to_string(h);
                    }
                    else if(indexExpr->getKind()==Expr::k_DeclRefExpr)
                    {
                        myindex= dynamic_cast<DeclRefExpr*>(indexExpr)->getRefName();
                    }
                    else
                    {
                        myindex="expr";
                    }
                    outFormat+="["+myindex+"]";
                    break;
                }
                case Expr::k_FieldSelector:
                {
                    outFormat=outFormat+"."+ dynamic_cast<FieldSelector*>(curSelector)->getName();
                    break;
                }
            }
        }
       std::cout<<"("<<outFormat<<")";
    }
    void outType(QualType t)
    {
        std::cout<<"(type)";
        return;
       std::cout << "(";
        Type*cType=t.getType();
        if(cType==nullptr)
        {
            std::cout<<"null)";
            return;
        }
        if(t.isAtomic())
           std::cout<<"atomic ";
        if(t.isConst())
            std::cout<<"const ";
        if(t.isVolatile())
            std::cout<<"volatile ";
        if(t.isRestrict())
            std::cout<<"restrict ";
        std::string curType;
        if(cType->getKind()==Type::k_BuiltInType)
        {
            curType= dynamic_cast<BuiltInType*>(cType)->getTypeTypeAsString();
            std::cout <<curType;
        }
        else if(cType->getKind()==Type::k_IncompleteArrayType)
        {
            std::cout<<"it is incomlete";

            /*if(dynamic_cast<IncompleteArrayType*>(cType)->getElementType()== nullptr)
            {
                std::cout<<"its element\n";
                return;
            }
            Type *eleType= dynamic_cast<IncompleteArrayType*>(cType)->getElementType();
            if(dynamic_cast<IncompleteArrayType*>(cType)->getElementType()== nullptr)
            {
                std::cout<<"its element type is nullptr\n";
                return;
            }
            std::cout<< dynamic_cast<BuiltInType*>(eleType)->getTypeTypeAsString()<<"[]";*/
        }
        else if(cType->getKind()==Type::k_VariableArrayType)
        {
            std::cout<<"variable array";
            Type *eleType= dynamic_cast<VariableArrayType*>(cType)->getElementType();
            std::cout<< dynamic_cast<BuiltInType*>(eleType)->getTypeTypeAsString()<<"[expr]";
        }
        else if(cType->getKind()==Type::k_ConstArrayType)
        {
            //todo:记得加上pointer
            std::cout<<"const array ";
            Type *eleType= dynamic_cast<ConstArrayType*>(cType)->getElementType();
            std::vector<int> dimensionLength;
            dimensionLength.push_back(dynamic_cast<ConstArrayType*>(cType)->getLength());
            while(eleType->getKind()!=Type::k_BuiltInType)
            {
                if(eleType->getKind()==Type::k_ConstArrayType)
                {
                    dimensionLength.push_back(dynamic_cast<ConstArrayType*>(eleType)->getLength());
                }

                else if(eleType->getKind()==Type::k_PointerType)
                {
                    dimensionLength.push_back(-1);
                }
                eleType= dynamic_cast<ConstArrayType*>(eleType)->getElementType();
            }
            std::string basicType= dynamic_cast<BuiltInType*>(eleType)->getTypeTypeAsString();
            std::string wholeType;
            for(int i=0;i!=dimensionLength.size();++i)
            {
                if(dimensionLength[i]>=0)
                    wholeType=wholeType+"["+std::to_string(dimensionLength[i])+"]";
                else if(dimensionLength[i]==-1)
                    wholeType="*("+wholeType+")";
            }
            wholeType=basicType+wholeType;
            std::cout<<wholeType;
        }
        else
        {
            std::cout<<"Error,invalid type: "<<(short)cType->getKind()<<"\n";
        }
        std::cout << ")";
    }

    bool visitTranslationUnitDecl(TranslationUnitDecl *D) {
        std::cout << "TranslationUnitDecl\n";
        std::cout<<"do not support 2 dimensional array in outtype\n";
        return true;
    }
    bool cleanupTranslationUnitDecl() {
        return true;
    }
    bool visitValueDecl(ValueDecl* D)
    {
        outSpace();
        recordLevel.push(D);
        std::cout << "ValueDecl " << "[" << D->getName() << "] ";
        outType(D->getQualType());
        std::cout << "\n";
        return true;
    }
    bool cleanupValueDecl() {
        recordLevel.pop();
        return true;
    }
    bool visitDeclaratorDecl(DeclaratorDecl* D)
    {
        outSpace();
        recordLevel.push(D);
        std::cout << "DeclaratorDecl " << "[" << D->getName() << "] ";
        outType(D->getQualType());
        std::cout << "\n";
        return true;
    }
    bool cleanupDeclaratorDecl(){
        recordLevel.pop();
        return true;
    }
    bool visitRecordDecl(RecordDecl* D)
    {
        outSpace();
        recordLevel.push(D);
        std::cout << "RecordDecl " << "[" << D->getName() << "] ";
        std::cout << "\n";
        return true;
    }
    bool cleanupRecordDecl(){
        recordLevel.pop();
        return true;
    }
    bool visitSelectorArray(SelectorArray* D)
    {
        outSpace();
        recordLevel.push(D);
        std::cout << "SelectorArray " ;
        outForSA(D);
        std::cout << "\n";
        return true;
    }
    bool cleanupSelectorArray(){
        recordLevel.pop();
        return true;
    }
    bool visitVarDecl(VarDecl* D)
    {
        outSpace();
        recordLevel.push(D);
        std::cout << "VarDecl " << "[" << D->getName() << "] ";
        outType(D->getQualType());
        std::cout << "\n";
        return true;
    }
    bool cleanupVarDecl(){
        recordLevel.pop();
        return true;
    }
    bool visitFunctionDecl(FunctionDecl* D) {
        outSpace();
        recordLevel.push(D);
        std::cout << "FunctionDecl " << "[" << D->getName() << "] ";
        outType(D->getQualType());
        std::cout << "\n";
        return true;
    }
    bool cleanupFunctionDecl(){
        recordLevel.pop();
        return true;
    }
    bool visitParamVarDecl(ParamVarDecl* D)
    {
        outSpace();
        recordLevel.push(D);
        std::cout << "ParamVarDecl " << "[" << D->getName() << "] ";
        outType(D->getQualType());
        std::cout << "\n";
        return true;
    }
    bool cleanupParamVarDecl(){
        recordLevel.pop();
        return true;
    }
    bool visitCompoundStmt(CompoundStmt* S)
    {
        outSpace();
        recordLevel.push(S);
        std::cout << "CompoundStmt\n";
        return true;
    }
    bool cleanupCompoundStmt(){
        recordLevel.pop();
        return true;
    }
    bool visitDeclStmt(DeclStmt* S)
    {
        outSpace();
        recordLevel.push(S);
        std::cout << "DeclStmt\n";
        return true;
    }
    bool cleanupDeclStmt(){
        recordLevel.pop();
        return true;
    }
    bool visitWhileStmt(WhileStmt* S)
    {
        outSpace();
        recordLevel.push(S);
        std::cout << "WhileStmt\n";
        return true;
    }
    bool cleanupWhileStmt(){
        recordLevel.pop();
        return true;
    }
    bool visitDoWhileStmt(DoWhileStmt* S)
    {
        outSpace();
        recordLevel.push(S);
        std::cout << "DoWhileStmt\n";
        return true;
    }
    bool cleanupDoWhileStmt(){
        recordLevel.pop();
        return true;
    }
    bool visitForStmt(ForStmt* S)
    {
        outSpace();
        recordLevel.push(S);
        std::cout << "ForStmt\n";
        return true;
    }
    bool cleanupForStmt(){
        recordLevel.pop();
        return true;
    }
    bool visitIfStmt(IfStmt* S)
    {
        outSpace();
        recordLevel.push(S);
        std::cout << "IfStmt\n";
        return true;
    }
    bool cleanupIfStmt(){
        recordLevel.pop();
        return true;
    }
    bool visitReturnStmt(ReturnStmt* S)
    {
        outSpace();
        recordLevel.push(S);
        std::cout << "ReturnStmt\n";
        return true;
    }
    bool cleanupReturnStmt(){
        recordLevel.pop();
        return true;
    }
    bool visitUnaryOperator(UnaryOperator* S)
    {
        //觉得多余可删
        std::string opnum[10]={ "_pre_inc","_post_inc"," _pre_dec",
                                " _post_dec","_address_of"," _indirection"," _not"," _bit_not",
                                "_unary_plus","_unary_minus" };
        outSpace();
        recordLevel.push(S);
        //std::cout << "UnaryOperator " << "[Op#" << S->getOp() << "] ";
        std::cout << "UnaryOperator " << "[" << opnum[S->getOp()] << "] ";
        outType(S->getQualType());
        std::cout << "\n";
        return true;
    }
    bool cleanupUnaryOperator(){
        recordLevel.pop();
        return true;
    }
    bool visitBinaryOperator(BinaryOperator* S)
    {
        //觉得多余可删
        std::string opnum[29]={ "_add","_sub","_mul","_div","_mod","_and","_or","_xor",
                                "_lsh"," _rsh","_eq","_ne","_lt","_gt","_le","_ge","_log_and",
                                "_log_or","_assign","_add_assign","_sub_assign","_mul_assign",
                                "_div_assign","_mod_assign","_and_assign","_or_assign",
                                "_xor_assign","_lsh_assign","_rsh_assign"};
        outSpace();
        recordLevel.push(S);
        //std::cout << "BinaryOperator " << "[Op#" << S->getOp() << "] ";
        std::cout << "BinaryOperator " << "[" << opnum[S->getOp()] << "] ";
         outType(S->getQualType());
        std::cout << "\n";
        return true;
    }
    bool cleanupBinaryOperator(){
        recordLevel.pop();
        return true;
    }
    bool visitCallExpr(CallExpr* S)
    {
        outSpace();
        recordLevel.push(S);
        std::cout << "CallExpr";
        std::cout << "\n";
        return true;
    }
    bool cleanupCallExpr(){
        recordLevel.pop();
        return true;
    }
    bool visitDeclRefExpr(DeclRefExpr* S)
    {
        outSpace();
        recordLevel.push(S);
        std::cout << "DeclRefExpr " << "[" << S->getRefName() << "] ";
        outType(S->getQualType());
        std::cout << "\n";
        return true;
    }
    bool cleanupDeclRefExpr() {
        recordLevel.pop();
        return true;
    }
    bool visitIntegerLiteral(IntegerLiteral* S)
    {
        outSpace();
        recordLevel.push(S);
        std::cout << "IntegerLiteral "<<"[" << S->getValue() << "] ";
        outType(S->getQualType());
        std::cout << "\n";
        return true;
    }
    bool cleanupIntegerLiteral() {
        recordLevel.pop();
        return true;
    }
    bool visitFloatingLiteral(FloatingLiteral* S)
    {
        outSpace();
        recordLevel.push(S);
        std::cout << "FloatingLiteral "<<"[" << S->getValue() << "] ";
        outType(S->getQualType());
        std::cout << "\n";
        return true;
    }
    bool cleanupFloatingLiteral(){
        recordLevel.pop();
        return true;
    }
    bool visitStringLiteral(StringLiteral* S)
    {
        outSpace();
        recordLevel.push(S);
        std::cout << "StringLiteral "<<"[" << S->getString() << "] ";
        outType(S->getQualType());
        std::cout << "\n";
        return true;
    }
    bool cleanupStringLiteral(){
        recordLevel.pop();
        return true;
    }
    bool visitExplicitCastExpr(ExplicitCastExpr* S)
    {
        outSpace();
        recordLevel.push(S);
        std::cout << "ExplicitCastExpr ";
        outType(S->getPrevType());
        // 箭头左边是原类型，右边是cast之后的类型，觉得丑自己改
        // std::cout << "-->" << endl;
        outType(S->getCastType());
        std::cout << "\n";
        return true;
    }
    bool cleanupExplicitCastExpr() {
        recordLevel.pop();
        return true;
    }
    bool visitImplicitCastExpr(ImplicitCastExpr* S)
    {
        outSpace();
        recordLevel.push(S);
        std::cout << "ImplicitCastExpr ";
        outType(S->getPrevType());
        // std::cout << "-->" << endl;
        outType(S->getCastType());
        std::cout << "\n";
        return true;
    }
    bool cleanupImplicitCastExpr(){
        recordLevel.pop();
        return true;
    }
};



#endif //FRONTEND_ASTDUMPER_H