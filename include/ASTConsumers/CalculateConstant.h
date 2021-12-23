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
                if(getConstValue(E->getSubExpr(),result))
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
                if(getConstValue(E->getSubExpr(),result))
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
                if(getConstValue(E->getSubExpr(),result))
                {
                    result=result+1;
                    Results.insert(std::make_pair(E,result));
                }
                break;
            }
            case UnaryOperator::_unary_plus:
            {
                float result;
                if(getConstValue(E->getSubExpr(),result))
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
        switch (E->getOp())
        {
            case BinaryOperator::_add:
            case BinaryOperator::_add_assign:
            {
                float lhs,rhs,result;
                if(getConstValue(E->getLHS(),lhs)&&getConstValue(E->getRHS(),rhs))
                {
                    result=lhs+rhs;
                    Results.insert(std::make_pair(E,result));
                }
                break;
            }
            case BinaryOperator::_sub:
            case BinaryOperator::_sub_assign:
            {
                float lhs,rhs,result;
                if(getConstValue(E->getLHS(),lhs)&&getConstValue(E->getRHS(),rhs))
                {
                    result=lhs-rhs;
                    Results.insert(std::make_pair(E,result));
                }
                break;
            }
            case BinaryOperator::_mul:
            case BinaryOperator::_mul_assign:
            {
                float lhs,rhs,result;
                if(getConstValue(E->getLHS(),lhs)&&getConstValue(E->getRHS(),rhs))
                {
                    result=lhs*rhs;
                    Results.insert(std::make_pair(E,result));
                }
                break;
            }
            case BinaryOperator::_div:
            case BinaryOperator::_div_assign:
            {
                float lhs,rhs,result;
                if(getConstValue(E->getLHS(),lhs)&&getConstValue(E->getRHS(),rhs))
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
                if(getConstValue(E->getLHS(),lhs)&&getConstValue(E->getRHS(),rhs))
                {
                    result=int(lhs)%int(rhs);
                    Results.insert(std::make_pair(E,result));
                }
                break;
            }
            case BinaryOperator::_assign:
            {
                float lhs,rhs,result;
                if(getConstValue(E->getRHS(),rhs))
                {
                    result=rhs;
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
                if(getConstValue(E->getLHS(),lhs)&&getConstValue(E->getRHS(),rhs))
                {
                    if(rhs==lhs)
                        result=1;
                    else
                        result=0;
                    Results.insert(std::make_pair(E,result));
                }
                break;
            }
            case BinaryOperator::_neq:
            {
                float lhs,rhs,result;
                if(getConstValue(E->getLHS(),lhs)&&getConstValue(E->getRHS(),rhs))
                {
                    if(rhs!=lhs)
                        result=1;
                    else
                        result=0;
                    Results.insert(std::make_pair(E,result));
                }
                break;
            }
            case BinaryOperator::_lt:            // less than
            {
                float lhs,rhs,result;
                if(getConstValue(E->getLHS(),lhs)&&getConstValue(E->getRHS(),rhs))
                {
                    if(lhs<rhs)
                        result=1;
                    else
                        result=0;
                    Results.insert(std::make_pair(E,result));
                }
                break;
            }
            case BinaryOperator::_gt:            // greater than
            {
                float lhs,rhs,result;
                if(getConstValue(E->getLHS(),lhs)&&getConstValue(E->getRHS(),rhs))
                {
                    if(lhs>rhs)
                        result=1;
                    else
                        result=0;
                    Results.insert(std::make_pair(E,result));
                }
                break;
            }
            case BinaryOperator::_leq:
            {
                float lhs,rhs,result;
                if(getConstValue(E->getLHS(),lhs)&&getConstValue(E->getRHS(),rhs))
                {
                    if(lhs<=rhs)
                        result=1;
                    else
                        result=0;
                    Results.insert(std::make_pair(E,result));
                }
                break;
            }
            case BinaryOperator::_geq:
            {
                float lhs,rhs,result;
                if(getConstValue(E->getLHS(),lhs)&&getConstValue(E->getRHS(),rhs))
                {
                    if(lhs>=rhs)
                        result=1;
                    else
                        result=0;
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
        Results.insert(std::make_pair(E,E->getValue()));
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
        QualType curType;
        curFunction=tables[tables.size()-1];
        if(!curFunction->checkSymbol(curName,curType))
        {
            if(!curRoot->checkSymbol(curName,curType))
            {
                for(int i=0;i!=allFunctions.size();++i)
                {
                    flag=false;
                    break;
                }
            }
        }
        if(flag)
        {
            if(D->getValueDecl()->getKind()==Decl::k_VarDecl)
            {
                Expr * curExpr=dynamic_cast<VarDecl*>(D->getValueDecl())->getInitializer();
                if(curExpr->getKind()==Expr::k_IntegerLiteral)
                {
                    result= dynamic_cast<IntegerLiteral*>(curExpr)->getValue();
                }
                else if(curExpr->getKind()==Expr::k_FloatingLiteral)
                {
                    result= dynamic_cast<IntegerLiteral*>(curExpr)->getValue();
                }
                else
                {
                    flag=false;
                }
            }
        }
        if(flag)
        {
            Results.insert(std::make_pair(D,result));
        }
        return true;
    }
    bool visitSelectorArray(SelectorArray*E)
    {
        //如果数组，指针，结构体指向的是常量
        //todo:
    }
    bool getConstValue(Expr* curExpr,float &constValue)
    {
        //return true means curExpr  has const value
       if(Results.find(curExpr)!=Results.end())
       {
           constValue=Results.find(dynamic_cast<DeclRefExpr*>(curExpr))->second;
           return true;
       }
       else
          return false;
    }
};

#endif //FRONTEND_CALCULATECONSTANT_H
