//
// Created by Shakira on 2021/12/19.
//

#ifndef FRONTEND_FILLTYPE_H
#define FRONTEND_FILLTYPE_H
#include <stack>
#include<string>
#include<map>
#include<iostream>
#include"include/AST/Expr.h"
#include "include/AST/Stmt.h"
#include"include/AST/Type.h"
#include "include/AST/RecursiveASTVisitor.h"
class FillType : public RecursiveASTVisitor<FillType> {
public:
    //自底向上
    bool traverseInPreOrder() const { return false; }
    bool walkUpToTop() const { return false; }
    //符号表里面存储变量名和变量类型
    //tables空不空还能指示当前在函数里面还是全局
    std::vector<FunctionDecl*>tables;
    std::vector<FunctionDecl>allFunctions;
    FunctionDecl* curFunction;
    TranslationUnitDecl* curRoot;
    int fornum=0;
    int fornew=0;
    //有个很大的问题就是declrefexpr必须要符号表才能确定，如果这个类不涉及符号表很多填充根本无法解决
    FillType()
            : RecursiveASTVisitor()
    {
        tables.clear();
    }
    bool visitTranslationUnitDecl(TranslationUnitDecl*D)
    {
        curRoot=D;
        std::cout<<"it is the end"<<std::endl;
        return true;
    }
    bool visitUnaryOperator(UnaryOperator* E)
    {
        switch (E->getOp())
        {
            case UnaryOperator::_address_of:
            {
                //todo:要考虑qualifier吗
                QualType pointeeType=E->getSubExpr()->getType();
                PointerType*newType=new PointerType(&pointeeType);
                QualType newQualtype(newType);
                E->setType(newQualtype);
                break;
            }
            case UnaryOperator::_not:
            case UnaryOperator::_post_dec:
            case UnaryOperator::_post_inc:
            case UnaryOperator::_pre_dec:
            case UnaryOperator::_pre_inc:
            case UnaryOperator::_unary_minus:
            case UnaryOperator::_unary_plus:
                E->setType(E->getSubExpr()->getType());
                break;
            case UnaryOperator::_indirection:
            {
                //解引用，探究subexpr指向的变量
                QualType actualType=E->getSubExpr()->getType();
                PointerType*p=dynamic_cast<PointerType*>(actualType.getType());
                QualType pointeeType=*(p->getPointeeType());
                E->setType(pointeeType);
                break;
            }
        }
        return true;
    }
    bool visitBinaryOperator(BinaryOperator* E)
    {
        switch (E->getOp())
        {
            case BinaryOperator::_add:
            case BinaryOperator::_sub:
            case BinaryOperator::_mul:
            case BinaryOperator::_div:
            case BinaryOperator::_mod:
            {
                //todo:要考虑qualifier吗
                E->setType(getLarger(E->getLHS()->getType(),E->getRHS()->getType()));
                break;
            }
            case BinaryOperator::_assign:
            case BinaryOperator::_add_assign:
            case BinaryOperator::_sub_assign:
            case BinaryOperator::_mul_assign:
            case BinaryOperator::_div_assign:
            case BinaryOperator::_mod_assign:
            case BinaryOperator::_and_assign:
            case BinaryOperator::_or_assign:
            case BinaryOperator::_xor_assign:
            case BinaryOperator::_lsh_assign:
            case BinaryOperator::_rsh_assign:
            {
                //todo:要报错吗？可是类型之间都可以互相cast，有上面错可以报呢
                //todo:除非左边不能被赋值，左右值的判别？
                E->setType(E->getRHS()->getType());
                break;
            }
            default:
            {
                E->setType(E->getRHS()->getType());
                break;
            }
        }
        return true;
    }
    bool visitIntegerLiteral(IntegerLiteral* E)
    {
        BuiltInType* newType=new BuiltInType(BuiltInType::_int);
        QualType newQualType(newType);
        newQualType.setConst();
        E->setType(newQualType);
        return true;
    }
    bool visitFloatingLiteral(FloatingLiteral* E)
    {
        BuiltInType* newType=new BuiltInType(BuiltInType::_float);
        QualType newQualType(newType);
        newQualType.setConst();
        E->setType(newQualType);
        return true;
    }
    //选占空间更大的数据类型返回
    QualType getLarger(QualType a,QualType b)
    {
        int space[8]={0,4,1,1,2,4,4,8};
        Type* atype=a.getType();
        Type* btype=b.getType();
        short anum=dynamic_cast<BuiltInType*> (atype)->getTypeType();
        short bnum=dynamic_cast<BuiltInType*> (btype)->getTypeType();
        if(space[anum]>=space[bnum])
            return a;
        return b;
    }
};
#endif //FRONTEND_FILLTYPE_H
