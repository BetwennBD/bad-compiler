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
    RecordType* curNoTagRecord;
    FunctionDecl* curFunction;
    TranslationUnitDecl* curRoot;
    int fornum=0;
    int fornew=0;
    std::ofstream outerror;
    std::ofstream zqtest;
    bool success=true;
    //有个很大的问题就是declrefexpr必须要符号表才能确定，如果这个类不涉及符号表很多填充根本无法解决
    FillType()
            : RecursiveASTVisitor()
    {
        tables.clear();
        outerror.open("../output/OutforErrors.txt", std::ios::app);
        zqtest.open("../output/zqtest.txt", std::ios::app);
        zqtest<<"filltype*************************\n";
    }
    bool visitTranslationUnitDecl(TranslationUnitDecl*D)
    {
        curRoot=D;
        zqtest<<"it is the end"<<std::endl;
        return true;
    }
    bool visitUnaryOperator(UnaryOperator* E)
    {
        if(E->getSubExpr()->getQualType().getTypeKind()==Type::k_BuiltInType)
        {
            if(dynamic_cast<BuiltInType*>(E->getSubExpr()->getQualType().getType())
            ->getTypeTypeAsString()=="port")
            {
                success=false;
                outerror<<"Error, port can not be calculated by unary operator\n";
                return true;
            }
        }
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
        zqtest<<"test:visit Binary \n";
        switch (E->getOp())
        {
            case BinaryOperator::_add:
            case BinaryOperator::_sub:
            case BinaryOperator::_mul:
            case BinaryOperator::_div:
            case BinaryOperator::_mod:
            {
                if(E->getRHS()->getQualType().getTypeKind()==Type::k_BuiltInType)
                {
                    if(dynamic_cast<BuiltInType*>(E->getRHS()->getQualType().getType())
                               ->getTypeTypeAsString()=="port")
                    {
                        success=false;
                        outerror<<"Error, port can not be calculated in current binary operator\n";
                        return true;
                    }
                }
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
                zqtest<<"test: assign\n";
                if(E->getLHS()->getQualType().isConst())
                {
                    success=false;
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
                if(E->getRHS()->getQualType().getTypeKind()==Type::k_BuiltInType)
                {
                    if(dynamic_cast<BuiltInType*>(E->getRHS()->getQualType().getType())
                               ->getTypeTypeAsString()=="port")
                    {
                        success=false;
                        outerror<<"Error, port can not be calculated in current binary operator\n";
                        return true;
                    }
                }
                BuiltInType* newType=new BuiltInType(BuiltInType::_bool);
                QualType newQualType(newType);
                E->setType(newQualType);
                E->setValueKind(ExprValueKind::RValue);
                break;
            }
            case BinaryOperator::_log_and:
            case BinaryOperator:: _log_or:
            {
                QualType curQT=E->getLHS()->getQualType();
                if(curQT.getTypeKind()!=Type::k_BuiltInType||
                   dynamic_cast<BuiltInType*>(curQT.getType())->getTypeType()!=BuiltInType::_bool)
                {
                    QualType newQT=curQT;
                    newQT.removeConst();
                    BuiltInType* newType=new BuiltInType(BuiltInType::_bool);
                    newQT.setType(newType);
                    ImplicitCastExpr* curImplicit=new ImplicitCastExpr(newQT,E->getLHS());
                    E->setLHS(curImplicit);
                }

                QualType curQT2=E->getRHS()->getQualType();
                if(curQT2.getTypeKind()!=Type::k_BuiltInType||
                   dynamic_cast<BuiltInType*>(curQT2.getType())->getTypeType()!=BuiltInType::_bool)
                {
                    QualType newQT=curQT;
                    newQT.removeConst();
                    BuiltInType* newType=new BuiltInType(BuiltInType::_bool);
                    newQT.setType(newType);
                    ImplicitCastExpr* curImplicit=new ImplicitCastExpr(newQT,E->getRHS());
                    E->setRHS(curImplicit);
                }

                BuiltInType* newType=new BuiltInType(BuiltInType::_bool);
                QualType newQualType(newType);
                E->setType(newQualType);
                E->setValueKind(ExprValueKind::RValue);
            }
            default:
            {
                if(E->getRHS()->getQualType().getTypeKind()==Type::k_BuiltInType)
                {
                    if(dynamic_cast<BuiltInType*>(E->getRHS()->getQualType().getType())
                               ->getTypeTypeAsString()=="port")
                    {
                        success=false;
                        outerror<<"Error, port can not be calculated in current binary operator\n";
                        return true;
                    }
                }
                E->setType(E->getRHS()->getQualType());
                break;
            }
        }
        return true;
    }
    bool visitIntegerLiteral(IntegerLiteral* E)
    {
        zqtest<<"test:visit int "<<E->getValue()<<"\n";
        BuiltInType* newType=new BuiltInType(BuiltInType::_int);
        QualType newQualType(newType);
        E->setType(newQualType);
        E->setValueKind(ExprValueKind::RValue);
        return true;
    }
    bool visitFloatingLiteral(FloatingLiteral* E)
    {
        zqtest<<"test:visit float "<<E->getValue()<<"\n";
        BuiltInType* newType=new BuiltInType(BuiltInType::_float);
        QualType newQualType(newType);
        E->setType(newQualType);
        E->setValueKind(ExprValueKind::RValue);
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
        E->setValueKind(ExprValueKind::RValue);
        return true;
    }
    //选占空间更大的数据类型返回
    QualType getLarger(QualType a,QualType b,int&equal)
    {
        //equal=0代表类型一样，=1代表和a类型一样，=2代表和b类型一样
        //a b都是简单类型
        zqtest<<"test: larger\n";
        int space[8]={0,4,1,1,2,4,4,8};
        Type* atype=a.getType();
        if(atype==nullptr)
        {
            success=false;
            outerror<<"Error, calculate operand is null";
            zqtest<<(short)atype->getKind()<<"\n";
        }
        Type* btype=b.getType();
        if(btype==nullptr)
        {
            success=false;
            outerror<<"Error, calculate operand is null";
            zqtest<<(short)btype->getKind()<<"\n";
        }
        short anum=dynamic_cast<BuiltInType*> (atype)->getTypeType();
        short bnum=dynamic_cast<BuiltInType*> (btype)->getTypeType();
        if(anum==bnum)
            equal=0;
        else if(space[anum]>=space[bnum])
        {
            equal=1;
            return a;
        }
        else
           equal=2;
        return b;
    }
    bool visitSelectorArray(SelectorArray*E) {
        //还要填充subexpr的类型
        zqtest<<"test: SelectorArray \n";
        if(!E->hasSubExpr())
        {
            success=false;
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
                success=false;
                outerror<<"Error,curSelector is nullptr\n";
                return true;
            }
            switch (curSelector->getKind())
            {
                case Expr::k_DerefSelector:
                {
                    zqtest<<"test: pointer ";
                    zqtest<<"test type:"<<(short)curQualType.getTypeKind()<<"***\n";
                    if(curQualType.getTypeKind()!=Type::k_PointerType)
                    {
                        success=false;
                        outerror<<"Error,there is not a PointerType in DerefSelector\n";
                        return true;
                    }
                    curQualType=*dynamic_cast<PointerType*>(curQualType.getType())->getPointeeType();
                    break;
                }
                case Expr::k_IndexSelector:
                {
                    zqtest<<"test: index  ";
                    zqtest<<"test type:"<<(short)curQualType.getTypeKind()<<"***\n";
                    if(curQualType.getTypeKind()==Type::k_BuiltInType)
                    {
                        success=false;
                        outerror<<"Error,it is builtintype, not an array in IndexSelector\n";
                        return true;
                    }
                    curQualType.setType(dynamic_cast<ArrayType*>(curQualType.getType())->getElementType());
                    break;
                }
                case Expr::k_FieldSelector:
                {
                    zqtest<<"test:field\n";
                    Type* curType= curQualType.getType();
                    if(curType->getKind()!=Type::k_UnknownType&&curType->getKind()!=Type::k_RecordType)
                    {
                        success=false;
                        outerror<<"Error,it is not UnknownType or RecordType in FieldSelector\n";
                        return true;
                    }
                    if(curType->getKind()==Type::k_UnknownType)
                    {
                        std::string unkname= dynamic_cast<UnknownType*>(curType)->getName();
                        if(recordRef.find(unkname)== recordRef.end())
                        {
                            curType=curNoTagRecord;
                        }
                        else
                            curType=recordRef.find(unkname)->second;
                        //TODO:局限性
                        if(curSub->getKind()==Expr::k_DeclRefExpr)
                        {
                           QualType tempQT=dynamic_cast<DeclRefExpr*>(curSub)->getQualType();
                           tempQT.setType(curType);
                            dynamic_cast<DeclRefExpr*>(curSub)->setType(tempQT);
                        }
                    }
                    std::string curMemberName= dynamic_cast<FieldSelector*>(curSelector)->getName();
                    std::vector<std::string>definedMembers=recordMembers.
                            find(dynamic_cast<RecordType*>(curType))->second;
                    int k=0;
                    for(;k!=definedMembers.size();++k)
                    {
                        if(definedMembers[k]==curMemberName)
                            break;
                    }
                    if(k>=definedMembers.size())
                    {
                        success=false;
                        outerror<<"Error,curMember do not belong to curRecord\n";
                        return true;
                    }
                    else
                    {
                        zqtest<<"add idx "<<k<<" to member "<<definedMembers[k]<<"\n";
                        dynamic_cast<FieldSelector*>(curSelector)->setIdx(k);
                        std::vector<QualType> tempQualTypes=recordMemberQTs.
                                find(dynamic_cast<RecordType*>(curType))->second;
                        curQualType.setType(tempQualTypes[k].getType());
                        break;
                    }
                }
            }
        }
        zqtest<<"test:selector array basic type:"<<(short )curQualType.getTypeKind()<<"\n";
        E->setType(curQualType);
        return false;
    }
    bool visitRecordDecl(RecordDecl* D)
    {
        //处理struct和union
        zqtest<<"test: visit record decl\n";
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
            curRecordType->setName(D->getName());
            recordMembers.insert(std::make_pair(curRecordType,memberNames));
            recordRef.insert(std::make_pair(D->getName(),curRecordType));
            recordMemberQTs.insert(std::make_pair(curRecordType,memberQTs));
        }
        else
        {
            curNoTagRecord=curRecordType;
            recordMembers.insert(std::make_pair(curRecordType,memberNames));
            recordMemberQTs.insert(std::make_pair(curRecordType,memberQTs));
        }
        return true;
    }
    bool visitVarDecl(VarDecl* D)
    {
        //清理unkonwnType,记得分清楚返回的是指针还是拷贝
        std::string curName=D->getName();
        QualType newQT=D->getQualType();
        Type * tempType=newQT.getType();
        Type* hh=new Type();
        bool  onetype=true;
        while(tempType->getKind()!=Type::k_BuiltInType
             &&tempType->getKind()!=Type::k_UnknownType)
        {
            onetype=false;
           if(tempType->getKind()==Type::k_VariableArrayType)
          {
             hh=tempType;
             tempType= dynamic_cast<VariableArrayType*>(tempType)->getElementType();
           }
           else if(tempType->getKind()==Type::k_PointerType)
           {
               hh=tempType;
               tempType= dynamic_cast<PointerType*>(tempType)->getPointeeType()->getType();
           }
           else
               break;
        }
        if(tempType->getKind()==Type::k_UnknownType)
        {
            zqtest<<"test: vardecl changes unknown type\n";
            std::string unknownName= dynamic_cast<UnknownType*>(tempType)->getName();
            if(recordRef.find(unknownName)!=recordRef.end())
            {
                RecordType* B=recordRef.find(unknownName)->second;
                if(hh->getKind()==Type::k_VariableArrayType)
                    dynamic_cast<VariableArrayType*>(hh)->setElementType(B);
                else if(hh->getKind()==Type::k_PointerType)
                {
                    QualType*t= dynamic_cast<PointerType*>(hh)->getPointeeType();
                    t->setType(B);
                }
                if(onetype)
                {
                    newQT.setType(B);
                }
                D->setQualType(newQT);
            }
            else
            {
                RecordType* B=curNoTagRecord;
                if(hh->getKind()==Type::k_VariableArrayType)
                    dynamic_cast<VariableArrayType*>(hh)->setElementType(B);
                else if(hh->getKind()==Type::k_PointerType)
                {
                    QualType*t= dynamic_cast<PointerType*>(hh)->getPointeeType();
                    t->setType(B);
                }
                if(onetype)
                {
                    newQT.setType(B);
                }
                D->setQualType(newQT);
            }
        }
        return true;
    }
    //
    bool visitFunctionDecl(FunctionDecl* D)
    {
        tables.push_back(D);
        allFunctions.push_back(*D);
        zqtest<<"let us add a new function "<<D->getName()<<std::endl;
        std::map<std::string, QualType>* firsttable=new std::map<std::string, QualType>();
        firsttable->clear();
        if(D->getNumParams()!=0)
        {
            D->addSymbolTable(firsttable);
        }
        return true;
    }
    bool visitCallExpr(CallExpr* E)
    {
        DeclRefExpr* function= dynamic_cast<DeclRefExpr* >(E->getArg(0));
        std::string funcName=function->getRefName();
        QualType funcType;
        for(int i=0;i!=allFunctions.size();++i)
        {
            FunctionDecl curFunc=allFunctions[i];
            std::string curName=curFunc.getName();
            if(curName==funcName)
            {
                funcType=curFunc.getQualType();
                break;
            }
        }
        E->setType(funcType);
        return true;
    }
    bool cleanupFunctionDecl(){
        tables[tables.size()-1]->clearSymbolTable();
        tables.erase(tables.end()-1);
        return true;
    }
    //到此为止
    bool visitForStmt(ForStmt*S)
    {
        Expr* curCondition=S->getCond();
        QualType curQT=curCondition->getQualType();
        curQT.removeConst();
        if(curQT.getTypeKind()!=Type::k_BuiltInType||
           dynamic_cast<BuiltInType*>(curQT.getType())->getTypeType()!=BuiltInType::_bool)
        {
            QualType newQT=curQT;
            BuiltInType* newType=new BuiltInType(BuiltInType::_bool);
            newQT.setType(newType);
            ImplicitCastExpr* curImplicit=new ImplicitCastExpr(newQT,S->getCond());
            S->setCond(curImplicit);
        }
        return true;
    }
    bool visitWhileStmt(WhileStmt*S)
    {
        Expr* curCondition=S->getCond();
        QualType curQT=curCondition->getQualType();
        curQT.removeConst();
        if(curQT.getTypeKind()!=Type::k_BuiltInType||
           dynamic_cast<BuiltInType*>(curQT.getType())->getTypeType()!=BuiltInType::_bool)
        {
            QualType newQT=curQT;
            BuiltInType* newType=new BuiltInType(BuiltInType::_bool);
            newQT.setType(newType);
            ImplicitCastExpr* curImplicit=new ImplicitCastExpr(newQT,S->getCond());
            S->setCond(curImplicit);
        }
        return true;
    }
    bool visitIfStmt(IfStmt*S)
    {
        Expr* curCondition=S->getCond();
        QualType curQT=curCondition->getQualType();
        curQT.removeConst();
        if(curQT.getTypeKind()!=Type::k_BuiltInType||
           dynamic_cast<BuiltInType*>(curQT.getType())->getTypeType()!=BuiltInType::_bool)
        {
            QualType newQT=curQT;
            BuiltInType* newType=new BuiltInType(BuiltInType::_bool);
            newQT.setType(newType);
            ImplicitCastExpr* curImplicit=new ImplicitCastExpr(newQT,S->getCond());
            S->setCond(curImplicit);
        }
        return true;
    }
    bool isSuccessful()
    {
        return success;
    }
    ~FillType()
    {
        outerror.close();
        zqtest.close();
        curRoot->clearSymbolTable();
    }
};
#endif //FRONTEND_FILLTYPE_H
