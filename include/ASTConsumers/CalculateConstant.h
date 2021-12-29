//
// Created by Shakira on 2021/12/19.
//

#ifndef FRONTEND_CALCULATECONSTANT_H
#define FRONTEND_CALCULATECONSTANT_H
#include <stack>
#include<iostream>
#include<map>
#include<fstream>
#include <math.h>
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
    //所有有名字的整数变量及其大小放在forCalculateArray
    std::map<std::string,int> forCalculateArray;
    //帮助variable和constant的数组在此处做区分
    std::map<std::string, QualType> forRefArray;
    //所有有名字的const数组及其固定的大小
    std::map<std::string,std::vector<int>>arrayLength;
    //存const变量相关的表达式,用于判断数组是否是定长数组
    std::map<Expr*,int>constTabele;
    std::map<std::string,int>constName;
    int fornum=0;
    int fornew=0;
    std::ofstream outerror;
    //有个很大的问题就是declrefexpr必须要符号表才能确定，如果这个类不涉及符号表很多填充根本无法解决
    CalculateConstant()
            : RecursiveASTVisitor()
    {
        tables.clear();
        outerror.open("../output/OutforErrors.txt", std::ios::app);
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
                    if(constTabele.find(curSub)!=constTabele.end())
                    {
                        constTabele.insert(std::make_pair(E,result));
                    }

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
                    if(constTabele.find(curSub)!=constTabele.end())
                    {
                        constTabele.insert(std::make_pair(E,result));
                    }
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
                    if(constTabele.find(curSub)!=constTabele.end())
                    {
                        constTabele.insert(std::make_pair(E,result));
                    }
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
                    if(constTabele.find(curSub)!=constTabele.end())
                    {
                        constTabele.insert(std::make_pair(E,result));
                    }
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
                    if(constTabele.find(curLHS)!=constTabele.end()
                       &&constTabele.find(curRHS)!=constTabele.end())
                    {
                        constTabele.insert(std::make_pair(E,result));
                    }
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
                    if(constTabele.find(curLHS)!=constTabele.end()
                       &&constTabele.find(curRHS)!=constTabele.end())
                    {
                        constTabele.insert(std::make_pair(E,result));
                    }
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
                    if(constTabele.find(curLHS)!=constTabele.end()
                       &&constTabele.find(curRHS)!=constTabele.end())
                    {
                        constTabele.insert(std::make_pair(E,result));
                    }
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
                        outerror<<"Error, divisor can not be 0\n";
                        return true;
                    }
                    result=lhs/rhs;
                    std::cout<<"binary result "<<result<<"\n";
                    Results.insert(std::make_pair(E,result));
                    if(constTabele.find(curLHS)!=constTabele.end()
                       &&constTabele.find(curRHS)!=constTabele.end())
                    {
                        constTabele.insert(std::make_pair(E,result));
                    }
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
                    if(constTabele.find(curLHS)!=constTabele.end()
                       &&constTabele.find(curRHS)!=constTabele.end())
                    {
                        constTabele.insert(std::make_pair(E,result));
                    }
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
                    if(constTabele.find(curLHS)!=constTabele.end()
                       &&constTabele.find(curRHS)!=constTabele.end())
                    {
                        constTabele.insert(std::make_pair(E,result));
                    }
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
                    if(constTabele.find(curLHS)!=constTabele.end()
                       &&constTabele.find(curRHS)!=constTabele.end())
                    {
                        constTabele.insert(std::make_pair(E,result));
                    }
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
                    if(constTabele.find(curLHS)!=constTabele.end()
                       &&constTabele.find(curRHS)!=constTabele.end())
                    {
                        constTabele.insert(std::make_pair(E,result));
                    }
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
        constTabele.insert(std::make_pair(E,E->getValue()));
        return true;
    }
    bool visitFloatingLiteral(FloatingLiteral* E)
    {
        Results.insert(std::make_pair(E,E->getValue()));
        return true;
    }
    bool visitDeclRefExpr(DeclRefExpr*D)
    {
        //添加一个矫正unknowntype的declrefexpr

        //这里是因为一开始的fillrefernce没有对varaiable和constant做区分
        if(forRefArray.find(D->getRefName())!=forRefArray.end())
        {
            D->setType(forRefArray.find(D->getRefName())->second);
        }
        if(constName.find(D->getRefName())!=constName.end())
        {
            constTabele.insert(std::make_pair(D,constName.find(D->getRefName())->second));
        }
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
        //在这里整一个越界判断，针对大小确定的数组，并且地址要是常数
        if(!E->hasSubExpr())
        {
            outerror<<"Error, this SelectorArray doesn't hava a subexpr\n";
            return true;
        }
        int numSelectors=E->getNumSelectors();
        std::vector<Selector*> curSelectors=E->getSelectors();
        Expr* curSub=E->getSubExpr();
        std::string arrayName= dynamic_cast<DeclRefExpr*>(curSub)->getRefName();
        if(arrayLength.find(arrayName)!=arrayLength.end())
        {
            for(int i=0;i!=numSelectors;++i)
            {
                Selector * curSelector=curSelectors[i];
                std::cout<<"test:selectors\n";
                switch (curSelector->getKind())
                {
                    case Expr::k_DerefSelector:
                    {
                        curSub=curSelector->getSubExpr();
                        break;
                    }
                    case Expr::k_IndexSelector:
                    {
                        if(Results.find(dynamic_cast<IndexSelector*>
                                        (curSelector)->getIdxExpr())!=Results.end())
                        {
                            int numOfDimen=arrayLength.find(arrayName)->second.size();
                            int maxSize=arrayLength.find(arrayName)->second[numOfDimen-1-i];
                            int  curAddress=Results.find(dynamic_cast<IndexSelector*>
                                                        (curSelector)->getIdxExpr())->second;
                            float fAddress=Results.find(dynamic_cast<IndexSelector*>
                                                        (curSelector)->getIdxExpr())->second;
                            if(fabs(fAddress-(float)(curAddress))>=0.001)
                            {
                                outerror<<"Error,array's address should be an integer\n";
                            }
                            if(maxSize<=curAddress)
                            {
                                std::cout<<maxSize<<" <= "<<curAddress;
                                outerror<<"Error,array's address exceed its size\n";
                            }
                        }
                        curSub=curSelector->getSubExpr();
                        break;
                    }
                    case Expr::k_FieldSelector:
                    {
                        curSub=curSelector->getSubExpr();
                        break;
                    }
                }
            }
        }
        return true;
    }
    bool visitVarDecl(VarDecl* D)
    {
        if(D->getInitializer()!= nullptr)
        {
            if(D->getInitializer()->getKind()==Expr::k_IntegerLiteral)
            {
                std::cout<<"test:record int "<<D->getName()<<"\n";
                forCalculateArray.insert(std::make_pair(D->getName(),
                        dynamic_cast<IntegerLiteral*>(D->getInitializer())->getValue()));
               if(D->getQualType().isConst())
               {
                   constName.insert(std::make_pair(D->getName(),
                           dynamic_cast<IntegerLiteral*>(D->getInitializer())->getValue()));
               }
                std::cout<<"test:int ends\n";
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
                if(D->getQualType().isConst()&&
                constTabele.find(D->getInitializer())!=constTabele.end())
                {
                    constName.insert(std::make_pair(D->getName(),
                            constTabele.find(D->getInitializer())->second));
                }
            }
            else if(D->getInitializer()->getKind()==Expr::k_BinaryOperator)
            {
                std::cout<<" record int "<<D->getName()<<"\n";
                float a=Results.find(dynamic_cast<BinaryOperator*>(D->getInitializer()))->second;
                forCalculateArray.insert(std::make_pair(D->getName(),a));
                if(D->getQualType().isConst()&&
                   constTabele.find(D->getInitializer())!=constTabele.end())
                {
                    constName.insert(std::make_pair(D->getName(),
                            constTabele.find(D->getInitializer())->second));
                }
            }
        }
        if(D->getQualType().getTypeKind()==Type::k_VariableArrayType)
        {
            Type* DType=D->getQualType().getType();
            bool isconst= false;
            std::vector<int> dimensions;
            //目的是构造一个完整的arraytype
            while(DType->getKind()==Type::k_VariableArrayType)
            {
                Expr*curSizeExpr= dynamic_cast<VariableArrayType*>(DType)->getSizeExpr();
                //也许是个Declrefexpr,也许是个Integerliterial
                if(curSizeExpr->getKind()==Expr::k_IntegerLiteral)
                {
                    int curSize= dynamic_cast<IntegerLiteral*>(curSizeExpr)->getValue();
                    dimensions.push_back(curSize);
                    isconst=true;
                }
                else if (curSizeExpr->getKind()==Expr::k_DeclRefExpr)
                {
                    if(constTabele.find(curSizeExpr)==constTabele.end())
                    {
                        isconst==false;
                        break;
                    }
                    else
                    {
                        int curSize=constTabele.find(curSizeExpr)->second;
                        dimensions.push_back(curSize);
                        isconst=true;
                    }
                }
                else
                {
                    traverseExprHelper(curSizeExpr, "Calculate sizeExpr");
                    if(constTabele.find(curSizeExpr)==constTabele.end())
                    {
                        isconst==false;
                        break;
                    }
                    else
                    {
                        int curSize=constTabele.find(curSizeExpr)->second;
                        dimensions.push_back(curSize);
                        isconst=true;
                    }
                }
                DType= dynamic_cast<VariableArrayType*>(DType)->getElementType();
            }
            if(isconst)
            {
                ConstArrayType* newTypes[dimensions.size()];
                Type*temp=DType;
                //构造一个多层constarray
                for(int i=dimensions.size()-1;i>=0;--i)
                {
                    newTypes[i]=new ConstArrayType();
                    if(i==dimensions.size()-1)
                        newTypes[i]->setElementType(temp);
                    else
                        newTypes[i]->setElementType(newTypes[i+1]);
                    newTypes[i]->setLength(dimensions[dimensions.size()-1-i]);
                }
                QualType tempQT=D->getQualType();
                tempQT.setType(newTypes[0]);
                D->setQualType(tempQT);
                forRefArray.insert(std::make_pair(D->getName(),D->getQualType()));
                arrayLength.insert(std::make_pair(D->getName(),dimensions));
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
    ~CalculateConstant()
    {
        outerror.close();
    }
};

#endif //FRONTEND_CALCULATECONSTANT_H
