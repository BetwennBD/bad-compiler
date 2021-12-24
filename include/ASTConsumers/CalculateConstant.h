//
// Created by Shakira on 2021/12/19.
//

#ifndef FRONTEND_CALCULATECONSTANT_H
#define FRONTEND_CALCULATECONSTANT_H
#include <stack>
#include<iostream>
#include<map>
#include "include/AST/RecursiveASTVisitor.h"
//主要作用是计算所有的常数表达式，对于除零或者越界的情况warning
//针对各种operator进行计算，intliteral,binaryoperator
class CalculateConstant : public RecursiveASTVisitor<CalculateConstant> {
public:
//自底向上
    bool traverseInPreOrder() const { return false; }
    bool walkUpToTop() const { return false; }
    std::map<Expr*,float> Results;
    std::vector<FunctionDecl*>tables;
    std::vector<FunctionDecl>allFunctions;
    FunctionDecl* curFunction;
    TranslationUnitDecl* curRoot;
    //仅用作计算简单的数组表达式的size，功能有限
    std::map<std::string,int> forCalculateArray;
    int fornum=0;
    int fornew=0;
    //有个很大的问题就是declrefexpr必须要符号表才能确定，如果这个类不涉及符号表很多填充根本无法解决
    CalculateConstant()
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
        Expr* curSub=E->getSubExpr();
        if(curSub->getKind()==Expr::k_ImplicitCastExpr)
            curSub= dynamic_cast<ImplicitCastExpr*>(curSub)->getCastedExpr();
        switch (E->getOp())
        {
            case UnaryOperator::_address_of:
            {
                break;
            }
            case UnaryOperator::_unary_minus:
            case UnaryOperator::_not:
            {
                float result;
                if(getConstValue(curSub,result))
                {
                    result=-result;
                    Results.insert(std::make_pair(E,result));
                }
                break;
            }
            case UnaryOperator::_post_dec:
            case UnaryOperator::_pre_dec:
            {
                float result;
                if(getConstValue(curSub,result))
                {
                    result=result-1;
                    Results.insert(std::make_pair(E,result));
                }
                break;
            }
            case UnaryOperator::_post_inc:
            case UnaryOperator::_pre_inc:
            {
                float result;
                if(getConstValue(curSub,result))
                {
                    result=result+1;
                    Results.insert(std::make_pair(E,result));
                }
                break;
            }
            case UnaryOperator::_unary_plus:
            {
                float  result;
                if(getConstValue(curSub,result))
                {
                    result=result;
                    Results.insert(std::make_pair(E,result));
                }
                break;
            }
            case UnaryOperator::_indirection:
            {
                break;
            }
        }
        return true;
    }
    bool visitBinaryOperator(BinaryOperator* E)
    {
        Expr* curLHS=E->getLHS();
        if(curLHS->getKind()==Expr::k_ImplicitCastExpr)
            curLHS= dynamic_cast<ImplicitCastExpr*>(curLHS)->getCastedExpr();
        Expr* curRHS=E->getRHS();
        if(curRHS->getKind()==Expr::k_ImplicitCastExpr)
            curRHS= dynamic_cast<ImplicitCastExpr*>(curRHS)->getCastedExpr();
        switch (E->getOp())
        {
            case BinaryOperator::_add:
            case BinaryOperator::_add_assign:
            {
                float lhs,rhs,result;
                if(getConstValue(curLHS,lhs)&&getConstValue(curRHS,rhs))
                {
                    result=lhs+rhs;
                    std::cout<<"binary result "<<result<<"\n";
                    Results.insert(std::make_pair(E,result));
                }
                break;
            }
            case BinaryOperator::_sub:
            case BinaryOperator::_sub_assign:
            {
                float lhs,rhs,result;
                if(getConstValue(curLHS,lhs)&&getConstValue(curRHS,rhs))
                {
                    result=lhs-rhs;
                    std::cout<<"binary result "<<result<<"\n";
                    Results.insert(std::make_pair(E,result));
                }
                break;
            }
            case BinaryOperator::_mul:
            case BinaryOperator::_mul_assign:
            {
                float lhs,rhs,result;
                if(getConstValue(curLHS,lhs)&&getConstValue(curRHS,rhs))
                {
                    result=lhs*rhs;
                    std::cout<<"binary result "<<result<<"\n";
                    Results.insert(std::make_pair(E,result));
                }
                break;
            }
            case BinaryOperator::_div:
            case BinaryOperator::_div_assign:
            {
                float lhs,rhs,result;
                if(getConstValue(curLHS,lhs)&&getConstValue(curRHS,rhs))
                {
                    if(rhs==0)
                    {
                        std::cout<<"Error, divisor can not be 0\n";
                        return true;
                    }
                    result=lhs/rhs;
                    Results.insert(std::make_pair(E,result));
                }
                break;
            }
            case BinaryOperator::_mod:
            case BinaryOperator::_mod_assign:
            {
                float lhs,rhs,result;
                if(getConstValue(curLHS,lhs)&&getConstValue(curRHS,rhs))
                {
                    result=int(lhs)%int(rhs);
                    std::cout<<"binary result "<<result<<"\n";
                    Results.insert(std::make_pair(E,result));
                }
                break;
            }
            case BinaryOperator::_assign:
            {
                float lhs,rhs,result;
                if(getConstValue(curRHS,rhs))
                {
                    result=rhs;
                    std::cout<<"binary result "<<result<<"\n";
                    Results.insert(std::make_pair(E,result));
                }
                break;
            }
            case BinaryOperator::_and_assign:
            case BinaryOperator::_or_assign:
            case BinaryOperator::_xor_assign:
            case BinaryOperator::_lsh_assign:
            case BinaryOperator::_rsh_assign:
            {
                break;
            }
            case BinaryOperator::_eq:
            {
                float lhs,rhs,result;
                if(getConstValue(curLHS,lhs)&&getConstValue(curRHS,rhs))
                {
                    if(rhs==lhs)
                        result=1;
                    else
                        result=0;
                    std::cout<<"binary result "<<result<<"\n";
                    Results.insert(std::make_pair(E,result));
                }
                break;
            }
            case BinaryOperator::_neq:
            {
                float lhs,rhs,result;
                if(getConstValue(curLHS,lhs)&&getConstValue(curRHS,rhs))
                {
                    if(rhs!=lhs)
                        result=1;
                    else
                        result=0;
                    std::cout<<"binary result "<<result<<"\n";
                    Results.insert(std::make_pair(E,result));
                }
                break;
            }
            case BinaryOperator::_lt:            // less than
            {
                float lhs,rhs,result;
                if(getConstValue(curLHS,lhs)&&getConstValue(curRHS,rhs))
                {
                    if(lhs<rhs)
                        result=1;
                    else
                        result=0;
                    std::cout<<"binary result "<<result<<"\n";
                    Results.insert(std::make_pair(E,result));
                }
                break;
            }
            case BinaryOperator::_gt:            // greater than
            {
                float lhs,rhs,result;
                if(getConstValue(curLHS,lhs)&&getConstValue(curRHS,rhs))
                {
                    if(lhs>rhs)
                        result=1;
                    else
                        result=0;
                    std::cout<<"binary result "<<result<<"\n";
                    Results.insert(std::make_pair(E,result));
                }
                break;
            }
            case BinaryOperator::_leq:
            {
                float lhs,rhs,result;
                if(getConstValue(curLHS,lhs)&&getConstValue(curRHS,rhs))
                {
                    if(lhs<=rhs)
                        result=1;
                    else
                        result=0;
                    std::cout<<"binary result "<<result<<"\n";
                    Results.insert(std::make_pair(E,result));
                }
                break;
            }
            case BinaryOperator::_geq:
            {
                float lhs,rhs,result;
                if(getConstValue(curLHS,lhs)&&getConstValue(curRHS,rhs))
                {
                    if(lhs>=rhs)
                        result=1;
                    else
                        result=0;
                    std::cout<<"binary result "<<result<<"\n";
                    Results.insert(std::make_pair(E,result));
                }
                break;
            }
            default:
            {
                break;
            }
        }
        return true;
    }
    bool visitIntegerLiteral(IntegerLiteral* E)
    {
        std::cout<<"integer "<<(float)E->getValue()<<"\n";
        Results.insert(std::make_pair(E,(float)E->getValue()));
        return true;
    }
    bool visitFloatingLiteral(FloatingLiteral* E)
    {
        Results.insert(std::make_pair(E,E->getValue()));
        return true;
    }
    bool visitDeclRefExpr(DeclRefExpr*D)
    {
        float result;
        bool flag=true;
        std::string curName=D->getRefName();
        if(forCalculateArray.find(D->getRefName())==forCalculateArray.end())
            return true;
        result=forCalculateArray.find(D->getRefName())->second;
        std::cout<<"we can find the value "<<D->getRefName()<<" "<<result<<"\n";
        Results.insert(std::make_pair(D,result));
        return true;
    }
    bool visitSelectorArray(SelectorArray*E)
    {
        //如果数组，指针，结构体指向的是常量
        //todo:
    }
    bool visitVarDecl(VarDecl* D)
    {
        if(D->getInitializer()!= nullptr)
        {
            if(D->getInitializer()->getKind()==Expr::k_IntegerLiteral)
            {
                std::cout<<" record int "<<D->getName()<<"\n";
                forCalculateArray.insert(std::make_pair(D->getName(),
                        dynamic_cast<IntegerLiteral*>(D->getInitializer())->getValue()));
            }
            else if(D->getInitializer()->getKind()==Expr::k_FloatingLiteral)
            {
                forCalculateArray.insert(std::make_pair(D->getName(),
                        dynamic_cast<FloatingLiteral*>(D->getInitializer())->getValue()));
            }
            else if(D->getInitializer()->getKind()==Expr::k_UnaryOperator)
            {
                std::cout<<" record int "<<D->getName()<<"\n";
                float a=Results.find(dynamic_cast<UnaryOperator*>(D->getInitializer()))->second;
                forCalculateArray.insert(std::make_pair(D->getName(),a));
            }
            else if(D->getInitializer()->getKind()==Expr::k_BinaryOperator)
            {
                std::cout<<" record int "<<D->getName()<<"\n";
                float a=Results.find(dynamic_cast<BinaryOperator*>(D->getInitializer()))->second;
                forCalculateArray.insert(std::make_pair(D->getName(),a));
            }
        }
        //fill size of array, turn variablearray into constantarray;
        //todo:check selectorArray's num,which can not exceed array's size
        if(D->getQualType().getTypeKind()==Type::k_VariableArrayType)
        {
            std::cout<<"it is a variable array\n";
            Expr*curSizeExpr= dynamic_cast<VariableArrayType*>(D->getQualType().getType())->getSizeExpr();
            //也许是个Declrefexpr,也许是个Integerliterial
            if(curSizeExpr->getKind()==Expr::k_IntegerLiteral)
            {
                int curSize= dynamic_cast<IntegerLiteral*>(curSizeExpr)->getValue();
                ConstArrayType *newType=new ConstArrayType();
                newType->setElementType(dynamic_cast<VariableArrayType*>(
                                                D->getQualType().getType())->getElementType());
                newType->setLength(curSize);
                QualType newQualType=D->getQualType();
                newQualType.setType(newType);
                D->setQualType(newQualType);
            }
            else if (curSizeExpr->getKind()==Expr::k_DeclRefExpr)
            {
                int curSize=forCalculateArray.find
                        (dynamic_cast<DeclRefExpr*>(curSizeExpr)->getRefName())->second;
                ConstArrayType *newType=new ConstArrayType();
                newType->setElementType(dynamic_cast<VariableArrayType*>(
                                                D->getQualType().getType())->getElementType());
                newType->setLength(curSize);
                QualType newQualType=D->getQualType();
                newQualType.setType(newType);
                D->setQualType(newQualType);
            }
            //开始勇起来了
            //真要遍历的话
            else
            {
                //todo:更多处理
            }
        }
        return true;

    }
    bool getConstValue(Expr* curExpr,float &constValue)
    {
        //return true means curExpr  has const value
       if(Results.find(curExpr)!=Results.end())
       {
           constValue=Results.find(curExpr)->second;
           return true;

       }
       return false;
    }
};

#endif //FRONTEND_CALCULATECONSTANT_H
