//
// Created by tangny on 2021/12/8.
//

// Expr是Stmt的派生类
// 由于其中继承关系复杂，独立性高
// 故单独写一个文件

//Fixme：
// 析构函数还均未声明

//Fixme：
// 左值右值还未声明
// 是否可以在构造函数中声明？

#ifndef FRONTEND_EXPR_H
#define FRONTEND_EXPR_H

#include <vector>
#include <string>
#include <cassert>
#include "include/AST/Type.h"
#include "include/AST/Stmt.h"
#include "include/AST/Specifiers.h"

class ValueDecl;

class Expr : public ValueStmt {
protected:
    ExprValueKind valueKind;

    QualType exprType;

public:
    Expr()
    : ValueStmt() {
        stmtKind = k_Expr;
        //Fixme:
        // 或许不应该先默认为右值
        valueKind = ExprValueKind::RValue;
    }

    bool isLValue() const { return valueKind == static_cast<ExprValueKind>(0); }

    bool isRValue() const { return valueKind == static_cast<ExprValueKind>(1); }

    void setValueKind( ExprValueKind _valueKind ) { valueKind = _valueKind; }

    QualType getQualType() const { return exprType; }

    void setType( QualType _exprType ) { exprType =  _exprType; }
};

class UnaryOperator : public Expr {
public:
    Expr *subExpr;

    enum Op {
        _pre_inc,
        _post_inc,
        _pre_dec,
        _post_dec,
        _address_of,        // &
        _indirection,       // *
        _not,               // !
        _bit_not,           // ~
        _unary_plus,        // +
        _unary_minus        // -
    };

    short opCode;

public:
    UnaryOperator()
    : Expr() {
        stmtKind = k_UnaryOperator;
        subExpr = nullptr;
    }

    Expr* getSubExpr() { return subExpr; }

    bool hasSubExpr() const { return subExpr != nullptr; }

    void setSubExpr( Expr * _subExpr ) { subExpr = _subExpr; }

    Op getOp() const { return static_cast<Op>(opCode); }

    void setOp( Op _opcode ) { opCode = _opcode; }

    bool isRef() { return opCode == _address_of; }

    bool isDeref() { return opCode == _indirection; }
};

class Selector : public Expr {
public:
    Expr *subExpr;

public:
    Selector()
    : Expr() {
        stmtKind = k_Selector;
        subExpr = nullptr;
    }

    Expr* getSubExpr() { return subExpr; }

    bool hasSubExpr() const { return subExpr != nullptr; }

    void setSubExpr( Expr * _subExpr ) { subExpr = _subExpr; }
};

class DerefSelector : public Selector {
public:
    DerefSelector()
    : Selector() {
        stmtKind = k_DerefSelector;
    }
};

class IndexSelector : public Selector {
public:
    Expr *idxExpr;

public:
    IndexSelector()
    : Selector() {
        stmtKind = k_IndexSelector;
        idxExpr = nullptr;
    }

    IndexSelector( Expr *_idxExpr )
    : Selector() {
        stmtKind = k_IndexSelector;
        idxExpr = _idxExpr;
    }

    Expr* getIdxExpr() { return idxExpr; }

    void setIdxExpr( Expr *_idxExpr ) { idxExpr = _idxExpr; }

    bool hasIdxExpr() const { return idxExpr != nullptr; }
};

class FieldSelector : public Selector {
public:
    int idx;
    std::string name;

public:
    FieldSelector()
    : Selector() {
        stmtKind = k_FieldSelector;
        idx = 0;
    }

    FieldSelector( std::string _name )
    : Selector() {
        stmtKind = k_FieldSelector;
        name = _name;
        idx = 0;
    }

    int getIdx() { return idx; }

    void setIdx( int _idx ) { idx = _idx; }

    std::string getName() { return name; }

    void setName( std::string _name ) { name = _name; }
};

class SelectorArray : public Expr {
public:
    Expr *subExpr;
    std::vector<Selector*> selectors;

public:
    SelectorArray()
    : Expr() {
        stmtKind = k_SelectorArray;
        subExpr = nullptr;
    }

    Expr* getSubExpr() { return subExpr; }

    bool hasSubExpr() const { return subExpr != nullptr; }

    void setSubExpr( Expr * _subExpr ) { subExpr = _subExpr; }

    int getNumSelectors() const { return selectors.size(); }

    void addSelector( Selector *_selector ) { selectors.emplace_back(_selector); }

    std::vector<Selector*>& getSelectors() { return selectors; }

    Selector* getSelector( int pos ) {
        assert(pos < selectors.size());
        return selectors[pos];
    }
};

class BinaryOperator : public Expr {
public:
    Expr *LHS;
    Expr *RHS;

    enum Op {
        // 算数运算符
        _add,
        _sub,
        _mul,
        _div,
        _mod,
        _and,
        _or,
        _xor,
        _lsh,           // left shift
        _rsh,           // right shift
        // 逻辑运算符
        _eq,
        _neq,
        _lt,            // less than
        _gt,            // greater than
        _leq,
        _geq,
        _log_and,
        _log_or,
        // 赋值运算符
        _assign,
        _add_assign,
        _sub_assign,
        _mul_assign,
        _div_assign,
        _mod_assign,
        _and_assign,
        _or_assign,
        _xor_assign,
        _lsh_assign,
        _rsh_assign,
        // 区域
        first_arithmetic = _add,
        last_arithmetic = _rsh,
        first_logic = _eq,
        last_logic = _log_or,
        first_assign = _assign,
        last_assign = _rsh_assign
    };

    short opCode;

public:

    BinaryOperator()
    : Expr() {
        stmtKind = k_BinaryOperator;
        LHS = nullptr;
        RHS = nullptr;
    }

    Expr* getLHS() { return LHS; }

    bool hasLHS() const { return LHS != nullptr; }

    void setLHS( Expr * _LHS ) { LHS = _LHS; }

    Expr* getRHS() { return RHS; }

    bool hasRHS() const { return RHS != nullptr; }

    void setRHS( Expr * _RHS ) { RHS = _RHS; }

    Op getOp() const { return static_cast<Op>(opCode); }

    void setOp( Op _opcode ) { opCode = _opcode; }

    bool isArithmeticOp() const { return (first_arithmetic <= opCode && opCode <= last_arithmetic); }

    bool isLogicOp() const { return (first_logic <= opCode && opCode <= last_logic); }

    bool isAssignOp() const { return (first_assign <= opCode && opCode <= last_assign); }
};

class CastExpr : public Expr {
protected:
    Expr *castedExpr;

public:
    CastExpr()
    : Expr() {
        stmtKind = k_CastExpr;
        castedExpr = nullptr;
    }

    CastExpr( QualType _newType, Expr *_castedExpr )
    : Expr() {
        stmtKind = k_CastExpr;
        exprType = _newType;
        valueKind = _castedExpr->isLValue() ? ExprValueKind::LValue : ExprValueKind::RValue;
        castedExpr = _castedExpr;
    }

    QualType getCastType() const { return exprType; }

    void setCastType( QualType _castType ) { exprType = _castType; }

    QualType getPrevType() const { return castedExpr->getQualType(); }

    Expr* getCastedExpr() { return castedExpr; }

    void setCastedExpr( Expr *_castedExpr ) { castedExpr = _castedExpr; }
};

class CallExpr : public Expr {
protected:
    // 零号参数是函数指针
    std::vector< Expr* > args;

public:
    CallExpr()
    : Expr() {
        stmtKind = k_CallExpr;
        args.resize(0);
    }

    int getNumArgs() const { return args.size(); }

    Expr* getArg( int pos ) {
        assert(pos < args.size() && "Asking for arg out of bound.");
        return args[pos];
    }

    void setArg( int pos, Expr *_expr ) {
        assert(pos < args.size() && "changing an arg out of bound.");
        args[pos] = _expr;
    }

    void addArg( Expr *arg ) { args.emplace_back(arg); }
};

class DeclRefExpr : public Expr {
protected:
    std::string refName;
    ValueDecl *valueDecl;

public:
    DeclRefExpr( std::string _refName )
    : Expr() {
        stmtKind = k_DeclRefExpr;
        refName = _refName;
        valueDecl = nullptr;
    }

    std::string getRefName() { return refName; }

    void setRefName( std::string _refName ) { refName = _refName; }

    ValueDecl* getValueDecl() { return valueDecl; }

    void setValueDecl( ValueDecl *_valueDecl ) { valueDecl = _valueDecl; }
};

class IntegerLiteral : public Expr {
protected:
    int valueLiteral;

public:
    IntegerLiteral(int _valueLiteral)
    : Expr() {
        stmtKind = k_IntegerLiteral;
        valueLiteral = _valueLiteral;
    }

    int getValue() const { return valueLiteral; }

    void setValue( int _valueLiteral ) { valueLiteral = _valueLiteral; }
};

class FloatingLiteral : public Expr {
protected:
    double valueLiteral;

public:
    FloatingLiteral(double _valueLiteral)
            : Expr() {
        stmtKind = k_FloatingLiteral;
        valueLiteral = _valueLiteral;
    }

    double getValue() const { return valueLiteral; }

    void setValue( double _valueLiteral ) { valueLiteral = _valueLiteral; }
};

class StringLiteral : public Expr {
protected:
    std::string stringContent;

public:
    StringLiteral(std::string _stringContent)
    : Expr() {
        stmtKind = k_StringLiteral;
        stringContent = _stringContent;
    }

    std::string getString() const { return stringContent; }

    void setString( std::string _stringContent ) { stringContent = _stringContent; }
};

class ExplicitCastExpr : public CastExpr {
public:
    ExplicitCastExpr()
    : CastExpr() {
        stmtKind = k_ExplicitCastExpr;
    }

    ExplicitCastExpr( QualType _newType, Expr *_castedExpr )
    : CastExpr(_newType, _castedExpr) {
        stmtKind = k_ExplicitCastExpr;
    }
};

class ImplicitCastExpr : public CastExpr {
public:
    ImplicitCastExpr()
    : CastExpr() {
        stmtKind = k_ImplicitCastExpr;
    }

    ImplicitCastExpr( QualType _newType, Expr *_castedExpr )
    : CastExpr(_newType, _castedExpr) {
        stmtKind = k_ImplicitCastExpr;
    }
};

#endif //FRONTEND_EXPR_H
