//
// Created by tangny on 2021/12/7.
//

// Type用于表示语言中的类型系统
// 被包含在Decl和Stmt内需要使用类型的类中

#ifndef FRONTEND_TYPE_H
#define FRONTEND_TYPE_H

#include <string>
#include "AbstractASTNode.h"

class Expr;

// Type基类
// 所有Type均应直接或间接继承该类
class Type {
public:
    enum Kind : short {
        k_Type,
        k_BuiltInType,
        k_PointerType,
        k_ArrayType,
        k_ConstArrayType,
        k_VariableArrayType,
        k_IncompleteArrayType,
        k_RecordType,
        k_EnumType,
        k_UnknownType,
        first_ArrayType = k_ArrayType,
        last_ArrayType = k_IncompleteArrayType
    };

    short typeKind;

    Type() {
        typeKind = k_Type;
    }

    virtual ~Type() {}

    Kind getKind() const { return static_cast<Kind>(typeKind); }
};

// C++目前共有4种Qualifier
// 分别为Const，Volatile，Restrict，Atomic
// 为可扩展性设置
// 本项目除了Const外大概率不会涉及其他
class Qualifier {
protected:
    enum QualEnum {
        Const = 0x1,
        Volatile = 0x2,
        Restrict = 0x4,
        Atomic = 0x8
    } qualEnum;

public:
    QualEnum getQual() const { return qualEnum; }

    Qualifier() : qualEnum(static_cast<QualEnum>(0)) {};

    bool isConst() const { return (qualEnum & 0x1) != 0; }

    bool isVolatile() const { return (qualEnum & 0x2) != 0; }

    bool isRestrict() const { return (qualEnum & 0x4) != 0; }

    bool isAtomic() const { return (qualEnum & 0x8) != 0; }

    void setConst() { qualEnum = static_cast<QualEnum>(qualEnum | 0x1); }

    void removeConst() { qualEnum = static_cast<QualEnum>(qualEnum & 0xE); }

    void setVolatile() { qualEnum = static_cast<QualEnum>(qualEnum | 0x2); }

    void removeVolatile() { qualEnum = static_cast<QualEnum>(qualEnum & 0xD); }

    void setRestrict() { qualEnum = static_cast<QualEnum>(qualEnum | 0x4); }

    void removeRestrict() { qualEnum = static_cast<QualEnum>(qualEnum & 0xB); }

    void setAtomic() { qualEnum = static_cast<QualEnum>(qualEnum | 0x8); }

    void removeAtomic() { qualEnum = static_cast<QualEnum>(qualEnum & 0x7); }
};

// 可以包含Qualifier的Type
class QualType : public AbstractASTNode {
protected:
    Qualifier qualifier;
    Type *type;

public:
    QualType() {
        basicKind = bk_QualType;
        type = nullptr;
    }

    QualType(Type *_type)
    : qualifier(), type(_type) {
        basicKind = bk_QualType;
    }

    Type* getType() { return type; }

    void setType( Type *_type ) { type = _type; }

    Type::Kind getTypeKind() const { return type->getKind(); }

    bool isUncertainType() const { return type == nullptr; }

    bool isConst() const { return qualifier.isConst(); }

    bool isVolatile() const { return qualifier.isVolatile(); }

    bool isRestrict() const { return qualifier.isRestrict(); }

    bool isAtomic() const { return qualifier.isAtomic(); }

    void setConst() { qualifier.setConst(); }

    void removeConst() { qualifier.removeConst(); }

    void setVolatile() { qualifier.setVolatile(); }

    void removeVolatile() { qualifier.removeVolatile(); }

    void setRestrict() { qualifier.setRestrict(); }

    void removeRestrict() { qualifier.removeRestrict(); }

    void setAtomic() { qualifier.setAtomic(); }

    void removeAtomic() { qualifier.removeAtomic(); }

    //Fixme:
    // 错误实现，请勿使用
    // 要完整实现需考虑Type的每个派生类
    bool operator != (QualType cmp) const {
        if(qualifier.getQual() != cmp.qualifier.getQual())
            return true;
        if(type->getKind() != cmp.getType()->getKind())
            return true;
        return false;
    }
    bool operator == (QualType cmp) const {
        if(qualifier.getQual() != cmp.qualifier.getQual())
            return false;
        if(type->getKind() != cmp.getType()->getKind())
            return false;
        return true;
    }
};

class BuiltInType : public Type {
public:
    // 枚举类中所涉及基本类型都是C++保留字
    // 所以在前面加下划线区分
    enum TypeEnum : short {
        _void,
        _int,
        _unsigned,
        _char,
        _bool,
        _short,
        _long,
        _float,
        _double
    };

    short typeEnum;

    BuiltInType( TypeEnum _typeEnum )
    : Type() {
        typeKind = k_BuiltInType;
        typeEnum = _typeEnum;
    }

    TypeEnum getTypeType() const { return static_cast<TypeEnum>(typeEnum); };

    std::string getTypeTypeAsString() const {
        switch(typeEnum) {
            case(_void):      return "void";
            case(_int):       return "int";
            case(_unsigned):  return "unsigned";
            case(_char):      return "char";
            case(_bool):      return "bool";
            case(_short):     return "short";
            case(_long):      return "long";
            case(_float):     return "float";
            case(_double):    return "double";
            default:          return "invalid type";
        }
    }

    void setTypeType( TypeEnum _typeEnum ) { typeEnum = _typeEnum; }
};

class PointerType : public Type {
protected:
    QualType *pointeeType;

public:
    PointerType()
    : Type() {
        typeKind = k_PointerType;
        pointeeType = nullptr;
    }

    PointerType( QualType *_pointeeType )
    : Type() {
        typeKind = k_PointerType;
        pointeeType = _pointeeType;
    }

    QualType* getPointeeType() const { return pointeeType; }

    void setPointeeType( QualType *_pointeeType ) { pointeeType = _pointeeType; }

    bool hasPointeeType() const { return pointeeType->getTypeKind() != Type::k_Type; }
};

class ArrayType : public Type {
protected:
    // 非常重要！
    // 需要注意，和指针类型不同的是：
    // 指针类型的指向类型是QualType
    // 而数组的值类型是Type
    Type *elementType;
    //
    enum SizeModifier {
        m_Normal,
        m_Static,
        m_Star
    } sizeModifier;

public:
    ArrayType()
    : Type() {
        typeKind = k_ArrayType;
        elementType = nullptr;
        sizeModifier = m_Normal;
    }

    Type* getElementType() const { return elementType; }

    void setElementType( Type *_valueType ) { elementType = _valueType; }

    bool hasElementType() const { return elementType->getKind() != k_Type; }

    void setNormal() { sizeModifier = m_Normal; }

    void setStatic() { sizeModifier = m_Static; }

    void setStar() { sizeModifier = m_Star; }

    bool isNormal() { return sizeModifier == m_Normal; }

    bool isStatic() { return sizeModifier == m_Static; }

    bool isStar() { return sizeModifier == m_Star; }
};

class ConstArrayType : public ArrayType {
protected:
    // 数组长度
    int length;

public:
    ConstArrayType()
    : ArrayType() {
        typeKind = k_ConstArrayType;
        sizeModifier = m_Normal;
    }

    int getLength() const { return length; }

    void setLength(int _length) { length = _length; }
};

class IncompleteArrayType : public ArrayType {
public:
    IncompleteArrayType()
    : ArrayType() {
        typeKind = k_IncompleteArrayType;
        sizeModifier = m_Normal;
    }
};

class VariableArrayType : public ArrayType {
protected:
    Expr *sizeExpr;

public:
    VariableArrayType()
    : ArrayType() {
        typeKind = k_VariableArrayType;
        sizeModifier = m_Normal;
        sizeExpr = nullptr;
    }

    Expr* getSizeExpr() const { return sizeExpr; }

    void setSizeExpr(Expr *_sizeExpr) { sizeExpr = _sizeExpr; }
};

class RecordType : public Type {
public:
    std::vector<QualType> members;
    std::string name;
    bool isS;

    RecordType()
    : Type() {
        typeKind = k_RecordType;
        name = "";
        isS = 1;
    }

    RecordType( bool _isS )
    : Type() {
        typeKind = k_RecordType;
        isS = _isS;
    }

    int getNumMembers() const { return members.size(); }

    QualType getMember( int pos ) {
        assert(pos < members.size());
        return members[pos];
    }

    void addMember(QualType _qualType) { members.emplace_back(_qualType); }

    bool isStruct() const { return isS ; }

    bool isUnion() const { return !isS ; }

    void setStruct() { isS = 1; }

    void setUnion() { isS = 0; }

    std::string getName() { return name; }

    void setName( std::string _name ) { name = _name; }
};

class EnumType : public Type {
public:
    std::vector<std::pair<std::string, int>> members;
    std::string name;

    EnumType()
    : Type() {
        typeKind = k_EnumType;
        members.resize(0);
        name = "";
    }

    int getNumMembers() const { return members.size(); }

    std::pair<std::string, int> getMember( int pos ) {
        assert(pos < members.size());
        return members[pos];
    }

    void addMember(std::string _name, int _value) {
        members.emplace_back(std::make_pair(_name, _value));
    }

    std::string getName() { return name; }

    void setName( std::string _name ) { name = _name; }
};

class UnknownType : public Type {
public:
    std::string name;

    UnknownType()
    : Type() {
        typeKind = k_UnknownType;
        name = "";
    }

    UnknownType( std::string _name )
    : Type() {
        typeKind = k_UnknownType;
        name = _name;
    }

    std::string getName() const { return name; }

    void setName( std::string _name ) { name = _name; }

    bool hasName() const { return name != ""; }
};

#endif //FRONTEND_TYPE_H
