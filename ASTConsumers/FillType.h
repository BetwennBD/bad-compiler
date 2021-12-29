//
// Created by Shakira on 2021/12/19.
//

#ifndef FRONTEND_FILLTYPE_H
#define FRONTEND_FILLTYPE_H
#include <stack>
#include<string>
#include<map>
#include<iostream>
#include<fstream>
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
    //为struct而设置的
    std::map<RecordType*,std::vector<std::string>> recordMembers;
    std::map<std::string,RecordType*>recordRef;
    std::map<RecordType*,std::vector<QualType>> recordMemberQTs;
    FunctionDecl* curFunction;
    TranslationUnitDecl* curRoot;
    int fornum=0;
    int fornew=0;
    std::ofstream outerror;
    //有个很大的问题就是declrefexpr必须要符号表才能确定，如果这个类不涉及符号表很多填充根本无法解决
    FillType()
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
        switch (E->getOp())
        {
            case UnaryOperator::_address_of:
            {
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
        std::cout<<"test:visit Binary \n";
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
                    if(E->getRHS()->getKind()!=Expr::k_IntegerLiteral&&
                    E->getRHS()->getKind()!=Expr::k_FloatingLiteral)
                    {
                        ImplicitCastExpr* curImplicit=new ImplicitCastExpr(imQualType,E->getRHS());
                        curImplicit->setValueKind(ExprValueKind::RValue);
                        E->setRHS(curImplicit);
                    }
                }
                if(equal==2||(E->getLHS()->getKind()==Stmt::k_DeclRefExpr&&E->getLHS()->isLValue()))
                {
                    if(E->getLHS()->getKind()!=Expr::k_IntegerLiteral&&
                       E->getLHS()->getKind()!=Expr::k_FloatingLiteral)
                    {
                        ImplicitCastExpr* curImplicit=new ImplicitCastExpr(imQualType,E->getLHS());
                        curImplicit->setValueKind(ExprValueKind::RValue);
                        E->setLHS(curImplicit);
                    }
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
                std::cout<<"test: assign\n";
                if(E->getLHS()->getQualType().isConst())
                {
                    outerror<<"Error,you cannot assign a RValue or a const value\n";
                    return true;
                }
                int equal=0;
                QualType imQualType=getLarger(E->getLHS()->getQualType(),E->getRHS()->getQualType(),equal);
                if(equal!=0||(E->getRHS()->getKind()==Stmt::k_DeclRefExpr&&E->getRHS()->isLValue()))
                {
                    if(E->getLHS()->getKind()!=Expr::k_IntegerLiteral&&
                       E->getLHS()->getKind()!=Expr::k_FloatingLiteral)
                    {
                        ImplicitCastExpr* curImplicit=
                                new ImplicitCastExpr(E->getLHS()->getQualType(),E->getRHS());
                        curImplicit->setValueKind(ExprValueKind::RValue);
                        E->setRHS(curImplicit);
                    }
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
        std::cout<<"test:visit int "<<E->getValue()<<"\n";
        BuiltInType* newType=new BuiltInType(BuiltInType::_int);
        QualType newQualType(newType);
        E->setType(newQualType);
        return true;
    }
    bool visitFloatingLiteral(FloatingLiteral* E)
    {
        std::cout<<"test:visit float "<<E->getValue()<<"\n";
        BuiltInType* newType=new BuiltInType(BuiltInType::_float);
        QualType newQualType(newType);
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
        E->setType(newQualType);
        return true;
    }
    //选占空间更大的数据类型返回
    QualType getLarger(QualType a,QualType b,int&equal)
    {
        //equal=0代表类型一样，=1代表和a类型一样，=2代表和b类型一样
        //a b都是简单类型
        std::cout<<"test: larger\n";
        int space[8]={0,4,1,1,2,4,4,8};
        Type* atype=a.getType();
        if(atype==nullptr)
        {
            outerror<<"Error, calculate operand is null";
            std::cout<<(short)atype->getKind()<<"\n";
        }
        Type* btype=b.getType();
        if(btype==nullptr)
        {
            outerror<<"Error, calculate operand is null";
            std::cout<<(short)btype->getKind()<<"\n";
        }
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
        std::cout<<"test: SelectorArray \n";
        if(!E->hasSubExpr())
        {
            outerror<<"Error, this SelectorArray doesn't hava a subexpr\n";
            return true;
        }
        int numSelectors=E->getNumSelectors();
        std::vector<Selector*> curSelectors=E->getSelectors();
        Expr* curSub=E->getSubExpr();
        QualType curQualType=curSub->getQualType();
        for(int i=0;i!=numSelectors;++i)
        {
            Selector * curSelector=curSelectors[i];
            if(curSelector== nullptr)
            {
                outerror<<"Error,curSelector is nullptr\n";
                return true;
            }
            switch (curSelector->getKind())
            {
                case Expr::k_DerefSelector:
                {
                    std::cout<<"test: pointer ";
                    std::cout<<"test type:"<<(short)curQualType.getTypeKind()<<"***\n";
                    if(curQualType.getTypeKind()!=Type::k_PointerType)
                    {
                        outerror<<"Error,there is not a PointerType in DerefSelector\n";
                        return true;
                    }
                    curQualType=*dynamic_cast<PointerType*>(curQualType.getType())->getPointeeType();
                    break;
                }
                case Expr::k_IndexSelector:
                {
                    std::cout<<"test: index  ";
                    std::cout<<"test type:"<<(short)curQualType.getTypeKind()<<"***\n";
                    if(curQualType.getTypeKind()==Type::k_BuiltInType)
                    {
                        outerror<<"Error,it is builtintype, not an array in IndexSelector\n";
                        return true;
                    }
                    curQualType.setType(dynamic_cast<ArrayType*>(curQualType.getType())->getElementType());
                    break;
                }
                case Expr::k_FieldSelector:
                {
                    std::cout<<"test:field\n";
                    Type* curType= curQualType.getType();
                    //curRecordType是unkowntype，根据它的名字找到对应的struct
                    if(curType->getKind()!=Type::k_UnknownType&&curType->getKind()!=Type::k_RecordType)
                    {
                        outerror<<"Error,it is not UnknownType or RecordType in FieldSelector\n";
                        return true;
                    }
                    std::string unkonwnName= dynamic_cast<UnknownType*>(curType)->getName();
                    RecordType* curRecordType=new RecordType();
                    if(recordRef.find(unkonwnName)!=recordRef.end())
                        curRecordType=recordRef.find(unkonwnName)->second;
                    std::string curMemberName= dynamic_cast<FieldSelector*>(curSelector)->getName();
                    std::vector<std::string>definedMembers=recordMembers.find(curRecordType)->second;
                    int k=0;
                    for(;k!=definedMembers.size();++k)
                    {
                        if(definedMembers[k]==curMemberName)
                            break;
                    }
                    if(k>=definedMembers.size())
                    {
                        outerror<<"Error,curMember do not belong to curRecord\n";
                        return true;
                    }
                    else
                    {
                        std::cout<<"add idx "<<k<<" to member "<<definedMembers[k]<<"\n";
                        dynamic_cast<FieldSelector*>(curSelector)->setIdx(k);
                        std::vector<QualType> tempQualTypes=recordMemberQTs.
                                find(dynamic_cast<RecordType*>(curRecordType))->second;
                        curQualType.setType(tempQualTypes[k].getType());
                        break;
                    }
                }
            }
        }
        std::cout<<"test:selector array basic type:"<<(short )curQualType.getTypeKind()<<"\n";
        E->setType(curQualType);
        return false;
    }
    bool visitRecordDecl(RecordDecl* D)
    {
        std::cout<<"test: visit record decl\n";
        if(D->isStruct())
        {
            RecordType* curRecordType=new RecordType(D->isStruct());
            std::vector<std::string> memberNames;
            std::vector<QualType>memberQTs;
            for(int i=0;i!=D->getNumDeclStmts();++i)
            {
                DeclStmt* curDeclStmt=D->getDeclStmt(i);
                for(int j=0;j!=curDeclStmt->getNumDecls();++j)
                {
                    VarDecl* curVarDecl=curDeclStmt->getDecl(j);
                    curRecordType->addMember(curVarDecl->getQualType());
                    memberNames.push_back(curVarDecl->getName());
                    memberQTs.push_back(curVarDecl->getQualType());
                }
            }
            if(D->hasTag())
            {
                recordMembers.insert(std::make_pair(curRecordType,memberNames));
                recordRef.insert(std::make_pair(D->getName(),curRecordType));
                recordMemberQTs.insert(std::make_pair(curRecordType,memberQTs));
            }
            else
            {
                //todo:对于无名struct
            }
        }
    }
    ~FillType()
    {
        outerror.close();
    }
};
#endif //FRONTEND_FILLTYPE_H
