//
// Created by tangny on 2021/12/6.
//

// Stmt 基类及其派生类

// 在组织类顺序时，尽量使属于同一层的类排在一起
// 派生自同一类型的派生类排在一起
// 具体参考Decl头文件说明

//Fixme：
// 析构函数还均未声明

#ifndef FRONTEND_STMT_H
#define FRONTEND_STMT_H

#include <vector>
#include <string>
#include "include/AST/Type.h"
#include "include/AST/AbstractASTNode.h"

class Decl;
class Expr;

// Stmt基类
// 所有Stmt均应直接或间接继承该类
class Stmt : public AbstractASTNode {
public:
    //作用同Decl中Kind相同
    enum Kind : short {
        k_Stmt,
        k_CompoundStmt,
        k_DeclStmt,
        k_WhileStmt,
        k_DoWhileStmt,
        k_ForStmt,
        k_IfStmt,
        k_ValueStmt,
        k_ReturnStmt,
        k_Expr,
        k_UnaryOperator,
        k_Selector,
        k_DerefSelector,
        k_IndexSelector,
        k_FieldSelector,
        k_SelectorArray,
        k_BinaryOperator,
        k_CallExpr,
        k_DeclRefExpr,
        k_IntegerLiteral,
        k_FloatingLiteral,
        k_StringLiteral,
        k_CastExpr,
        k_ExplicitCastExpr,
        k_ImplicitCastExpr,
        k_VirtualStmt,                          // 非实际AST节点
        first_Expr = k_Expr,
        last_Expr = k_ImplicitCastExpr,
        first_CastExpr = k_CastExpr,
        last_CastExpr = k_ImplicitCastExpr
    };

    short stmtKind;

    Stmt() {
        basicKind = bk_Stmt;
        stmtKind = k_Stmt;
    }

    // 返回派生类类型
    Kind getKind() const { return static_cast<Kind>(stmtKind); };
};

class CompoundStmt : public Stmt {
public:
    std::vector< Stmt* > stmts;

    CompoundStmt()
    : Stmt() {
        stmtKind = k_CompoundStmt;
        stmts.resize(0);
    }

    int getNumStmts() const { return stmts.size(); }

    Stmt* getStmt(int pos) {
        assert(pos < stmts.size() && "Asking for stmt out of bound.");
        return stmts[pos];
    }

    void addStmt( Stmt *stmt ) { stmts.emplace_back(stmt); }
};

class DeclStmt : public Stmt {
public:
    //Fixme:
    // 这里Decl或许应该改成更高级别的派生类
    std::vector< VarDecl* > decls;

    DeclStmt()
    : Stmt() {
        stmtKind = k_DeclStmt;
        decls.resize(0);
    }

    int getNumDecls() const { return decls.size(); }

    VarDecl* getDecl(int pos) {
        assert(pos < decls.size() && "Asking for decl out of bound.");
        return decls[pos];
    }

    void addDecl( VarDecl *decl ) { decls.emplace_back(decl); }
};

class WhileStmt : public Stmt {
protected:
    Expr *condition;
    Stmt *whileBody;

public:
    WhileStmt()
    : Stmt() {
        stmtKind = k_WhileStmt;
        condition = nullptr;
        whileBody = nullptr;
    }

    Expr* getCond() { return condition; }

    void setCond( Expr *_condition ) { condition = _condition; }

    bool hasCond() const { return condition != nullptr; }

    Stmt* getBody() { return whileBody; }

    void setBody( Stmt *_whileBody ) { whileBody = _whileBody; }

    bool hasBody() const { return whileBody != nullptr; }
};

class DoWhileStmt : public Stmt {
protected:
    Expr *condition;
    Stmt *whileBody;

public:
    DoWhileStmt()
    : Stmt() {
        stmtKind = k_DoWhileStmt;
        condition = nullptr;
        whileBody = nullptr;
    }

    Expr* getCond() { return condition; }

    void setCond( Expr *_condition ) { condition = _condition; }

    bool hasCond() const { return condition != nullptr; }

    Stmt* getBody() { return whileBody; }

    void setBody( Stmt *_whileBody ) { whileBody = _whileBody; }

    bool hasBody() const { return whileBody != nullptr; }
};

class ForStmt : public Stmt {
protected:
    DeclStmt *initDecls;
    Expr *condition;
    Expr *increment;
    Stmt *forBody;

public:
    ForStmt()
    : Stmt() {
        stmtKind = k_ForStmt;
        initDecls = nullptr;
        condition = nullptr;
        increment = nullptr;
        forBody = nullptr;
    }

    DeclStmt* getInit() { return initDecls; }

    void setInit( DeclStmt *_initDecls ) { initDecls = _initDecls; }

    bool hasInit() const { return initDecls != nullptr; }

    Expr* getCond() { return condition; }

    void setCond( Expr *_condition ) { condition = _condition; }

    bool hasCond() const { return condition != nullptr; }

    Expr* getInc() { return increment; }

    void setInc( Expr *_increment ) { increment = _increment; }

    bool hasInc() const { return increment != nullptr; }

    Stmt* getBody() { return forBody; }

    void setBody( Stmt *_forBody ) { forBody = _forBody; }

    bool hasBody() const { return forBody != nullptr; }
};

class IfStmt : public Stmt {
protected:
    Expr *condition;
    Stmt *thenBody;
    Stmt *elseBody;

public:
    IfStmt()
    : Stmt() {
        stmtKind = k_IfStmt;
        condition = nullptr;
        thenBody = nullptr;
        elseBody = nullptr;
    }

    Expr* getCond() { return condition; }

    void setCond( Expr *_condition ) { condition = _condition; }

    bool hasCond() const { return condition != nullptr; }

    Stmt* getThen() { return thenBody; }

    void setThen( Stmt *_thenBody ) { thenBody = _thenBody; }

    bool hasThen() const { return thenBody != nullptr; }

    Stmt* getElse() { return elseBody; }

    void setElse( Stmt *_elseBody ) { elseBody = _elseBody; }

    bool hasElse() const { return elseBody != nullptr; }
};

class ValueStmt : public Stmt {
public:
    ValueStmt()
    : Stmt() {
        stmtKind = k_ValueStmt;
    }
};

class ReturnStmt : public Stmt {
protected:
    Expr *retValue;

public:
    ReturnStmt()
    : Stmt() {
        stmtKind = k_ReturnStmt;
        retValue = nullptr;
    }

    ReturnStmt( Expr *_retValue )
    : Stmt() {
        stmtKind = k_ReturnStmt;
        retValue = _retValue;
    }

    Expr* getRetValue() { return retValue; }

    void setRetValue( Expr *_retValue ) { retValue = _retValue; }

    bool hasRetValue() const { return (retValue != nullptr); }
};


// 虚拟Stmt类
// 用于辅助建立AST
// 因为各Stmt子类需要互相挂载，非常复杂
// 故设计此节点作为中间节点
// 理论上可将代码复杂度从N*N降低到2*N
// 筛选时间复杂度降低到原先的1/2~1/3 (常数时间复杂度)
class VirtualStmt : public Stmt {
protected:
    Stmt *realStmt;

public:
    VirtualStmt()
    : Stmt() {
        stmtKind = k_VirtualStmt;
        realStmt = nullptr;
    }

    Stmt* getRealStmt() { return realStmt; }

    void setRealStmt( Stmt *_realStmt ) { realStmt = _realStmt; }

    bool hasRealStmt() const { return realStmt != nullptr; }
};

#endif //FRONTEND_STMT_H
