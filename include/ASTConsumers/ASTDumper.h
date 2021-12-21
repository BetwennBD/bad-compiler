//
// Created by tangny on 2021/12/10.
//

#ifndef FRONTEND_ASTDUMPER_H
#define FRONTEND_ASTDUMPER_H

#include <stack>
#include<iostream>
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

    void outType(QualType t)
    {
        /*std::cout << "(";
        std::string curQual;
        std::string curType;
        if(t.isAtomic())
           std::cout<<"atomic ";
        if(t.isConst())
            std::cout<<"const ";
        if(t.isVolatile())
            std::cout<<"volatile ";
        if(t.isRestrict())
            std::cout<<"restrict ";
        Type*cType=t.getType();
        if(cType->getKind()==Type::k_BuiltInType)
        {
            curType= dynamic_cast<BuiltInType*>(cType)->getTypeTypeAsString();
        }
        else if(cType->getKind()==Type::k_ArrayType)
            //todo:要输出成 int[30]这种格式，后面记得改
            curType="Array";
        else if(cType->getKind()==Type::k_PointerType)
            curType="Pointer";
        std::cout <<curType;
        std::cout << ")";*/
        std::cout<<"( type )";
    }

    bool visitTranslationUnitDecl(TranslationUnitDecl *D) {
        std::cout << "TranslationUnitDecl\n";
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
        outSpace();
        recordLevel.push(S);
        std::cout << "UnaryOperator " << "[Op#" << S->getOp() << "] ";
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
        outSpace();
        recordLevel.push(S);
        std::cout << "BinaryOperator " << "[Op#" << S->getOp() << "] ";
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