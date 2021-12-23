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
                QualType pointeeType=E->getSubExpr()->getQualType();
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
                E->setType(E->getSubExpr()->getQualType());
                break;
            case UnaryOperator::_indirection:
            {
                //解引用，探究subexpr指向的变量
                QualType actualType=E->getSubExpr()->getQualType();
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
                int equal=3;
                QualType imQualType=getLarger(E->getLHS()->getQualType(),E->getRHS()->getQualType(),equal);
                //binaryoperator的子节点是binaryoperator的情况下同样支持
                if(equal==1||(E->getRHS()->getKind()==Stmt::k_DeclRefExpr&&E->getRHS()->isLValue()))
                {
                    ImplicitCastExpr* curImplicit=new ImplicitCastExpr(imQualType,E->getRHS());
                    curImplicit->setValueKind(ExprValueKind::RValue);
                    E->setRHS(curImplicit);
                    std::cout<<"there is a impli in binary r************\n";
                }
                if(equal==2||(E->getLHS()->getKind()==Stmt::k_DeclRefExpr&&E->getLHS()->isLValue()))
                {
                    ImplicitCastExpr* curImplicit=new ImplicitCastExpr(imQualType,E->getLHS());
                    curImplicit->setValueKind(ExprValueKind::RValue);
                    std::cout<<"there is a impli in binary l**************\n";
                    E->setLHS(curImplicit);
                }
                E->setType(imQualType);
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
                int equal=0;
                QualType imQualType=getLarger(E->getLHS()->getQualType(),E->getRHS()->getQualType(),equal);
                if(equal!=0||(E->getRHS()->getKind()==Stmt::k_DeclRefExpr&&E->getRHS()->isLValue()))
                {
                    ImplicitCastExpr* curImplicit=
                            new ImplicitCastExpr(E->getLHS()->getQualType(),E->getRHS());
                    curImplicit->setValueKind(ExprValueKind::RValue);
                    E->setRHS(curImplicit);
                    std::cout<<"there is a impli in binary _f_assign*******\n";
                }
                E->setType(E->getLHS()->getQualType());
                break;
            }
            case BinaryOperator::_eq:
            case BinaryOperator::_neq:
            case BinaryOperator::_lt:            // less than
            case BinaryOperator::_gt:            // greater than
            case BinaryOperator::_leq:
            case BinaryOperator::_geq:
            {
                BuiltInType* newType=new BuiltInType(BuiltInType::_bool);
                QualType newQualType(newType);
                E->setType(newQualType);
                E->setValueKind(ExprValueKind::RValue);
                break;
            }
            default:
            {
                E->setType(E->getRHS()->getQualType());
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
    bool visitStringLiteral(StringLiteral* E)
    {
        ConstArrayType* newType=new ConstArrayType();
        newType->setLength(E->getString().size()+1);
        BuiltInType* eleType=new BuiltInType(BuiltInType::_char);
        newType->setElementType(eleType);
        QualType newQualType(newType);
        newQualType.setConst();
        E->setType(newQualType);
        return true;
    }
    //选占空间更大的数据类型返回
    QualType getLarger(QualType a,QualType b,int&equal)
    {
        //equal=0代表类型一样，=1代表和a类型一样，=2代表和b类型一样
        //只针对builtintype,否则函数报错
        int space[8]={0,4,1,1,2,4,4,8};
        Type* atype=a.getType();
        Type* btype=b.getType();
        short anum=dynamic_cast<BuiltInType*> (atype)->getTypeType();
        short bnum=dynamic_cast<BuiltInType*> (btype)->getTypeType();
        if(anum==bnum)
            equal=0;
        if(space[anum]>=space[bnum])
        {
            equal=1;
            return a;
        }
        equal=2;
        return b;
    }
    bool visitSelectorArray(SelectorArray*E) {
        //还要填充subexpr的类型
        if(!E->hasSubExpr())
        {
            std::cout<<"Error, this SelectorArray doesn't hava a subexpr\n";
            return true;
        }
        int numSelectors=E->getNumSelectors();
        std::vector<Selector*> curSelectors=E->getSelectors();
        Expr* curSub=E->getSubExpr();
        QualType curQualType=curSub->getQualType();
        for(int i=0;i!=numSelectors;++i)
        {
            Selector * curSelector=curSelectors[i];
            switch (curSelector->getKind())
            {
                case Expr::k_DerefSelector:
                {
                    curQualType=*dynamic_cast<PointerType*>(curQualType.getType())->getPointeeType();
                    break;
                }
                case Expr::k_IndexSelector:
                {
                    curQualType.setType(dynamic_cast<ArrayType*>(curQualType.getType())->getElementType());
                    break;
                }
                case Expr::k_FieldSelector:
                {
                    //todo:struct待完善
                    break;
                }
            }
        }
        E->setType(curQualType);
        return false;
    }
};
#endif //FRONTEND_FILLTYPE_H
