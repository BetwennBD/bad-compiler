//
// Created by tangny on 2021/12/8.
//

#include <cassert>
#include <iostream>
#include <cmath>
#include "include/AST/ASTBuilder.h"

TranslationUnitDecl* ASTBuilder::constructAST(CSTNode *head, int verbose) {
    assert(head != NULL && "ASTBuilder receives invalid concrete syntax tree.\n");

    while(!nodeStack.empty()) nodeStack.pop();
    TranslationUnitDecl *translationUnitDecl = new TranslationUnitDecl();
    nodeStack.push(translationUnitDecl);
    recursiveVisitCST(head);

    assert(nodeStack.size() == 1 && "ASTBuilder receives invalid concrete syntax tree.\n");
    return translationUnitDecl;
}

void ASTBuilder::recursiveVisitCST(CSTNode *node) {
#define LISTEN(TYPE)                            \
    if(node->getListener() == #TYPE)            \
        enter##TYPE(node);
#include "include/AST/ListenerTypes.inc"
#ifdef LISTEN
#undef LISTEN
#endif

    for(auto child : node->children)
        recursiveVisitCST(child);

#define LISTEN(TYPE)                            \
    if(node->getListener() == #TYPE)            \
        quit##TYPE(node);
#include "include/AST/ListenerTypes.inc"
}

#define SEC_GET_DECL(DECL)                                                                                  \
    assert(nodeStack.top()->isDecl());                                                                      \
    Decl *pd##DECL = dynamic_cast<Decl*>(nodeStack.top());                                                  \
    nodeStack.pop();                                                                                        \
    assert(pd##DECL->getKind() == Decl::k_##DECL);                                                          \
    DECL *p##DECL = dynamic_cast<DECL*>(pd##DECL);

#define SEC_GET_DECL_RANGE(DECL)                                                                            \
    assert(nodeStack.top()->isDecl());                                                                      \
    Decl *pd##DECL = dynamic_cast<Decl*>(nodeStack.top());                                                  \
    nodeStack.pop();                                                                                        \
    assert(pd##DECL->getKind() >= Decl::first_##DECL && pd##DECL->getKind() <= Decl::last_##DECL);          \
    DECL *p##DECL = dynamic_cast<DECL*>(pd##DECL);

#define SEC_GET_STMT(STMT)                                                                                  \
    assert(nodeStack.top()->isStmt());                                                                      \
    Stmt *pd##STMT = dynamic_cast<Stmt*>(nodeStack.top());                                                  \
    nodeStack.pop();                                                                                        \
    assert(pd##STMT->getKind() == Stmt::k_##STMT);                                                          \
    STMT *p##STMT = dynamic_cast<STMT*>(pd##STMT);

#define SEC_GET_STMT_RANGE(STMT)                                                                            \
    assert(nodeStack.top()->isStmt());                                                                      \
    Stmt *pd##STMT = dynamic_cast<Stmt*>(nodeStack.top());                                                  \
    nodeStack.pop();                                                                                        \
    assert(pd##STMT->getKind() >= Stmt::first_##STMT && pd##STMT->getKind() <= Stmt::last_##STMT);          \
    STMT *p##STMT = dynamic_cast<STMT*>(pd##STMT);

#define SEC_GET_QAULTYPE()                                                                                  \
    assert(nodeStack.top()->isQualType());                                                                  \
    QualType *p##QualType = dynamic_cast<QualType*>(nodeStack.top());                                       \
    nodeStack.pop();


// 处理变量(包括函数)的引用
void ASTBuilder::enterRef(CSTNode *node) {}

//Fixme:
// 因为语义分析前不知道具体类型
// 所以不定义类型
// 这应该不是错误
void ASTBuilder::quitRef(CSTNode *node) {
    assert(node->getChildren().size() == 1);
    CSTNode *pNode = node->getChildren()[0];
    assert(pNode->isTerminal());

    AbstractASTNode *parent = nodeStack.top();
    Expr *pDeclRefExpr = new DeclRefExpr(pNode->getId());
    exprCommonAction(parent, pDeclRefExpr, "Ref");
}

// 处理数字常量
void ASTBuilder::enterNumLiteral(CSTNode *node) {}

void ASTBuilder::quitNumLiteral(CSTNode *node) {
    assert(node->getChildren().size() == 1);
    CSTNode *pNode = node->getChildren()[0];
    assert(pNode->isTerminal());

    AbstractASTNode *parent = nodeStack.top();

    std::string s = pNode->getId();
    Expr *pNumLiteral;
    // 判断是浮点数还是整数
    if(s.find('E') == s.npos &&
        s.find('e') == s.npos &&
        s.find('.') == s.npos)
        pNumLiteral = new IntegerLiteral(std::stoi(s));
    else
        pNumLiteral = new FloatingLiteral(std::stod(s));

    exprCommonAction(parent, pNumLiteral, "NumLiteral");
}

// 处理数字常量
void ASTBuilder::enterStrLiteral(CSTNode *node) {}

void ASTBuilder::quitStrLiteral(CSTNode *node) {
    assert(node->getChildren().size() == 1);
    CSTNode *pNode = node->getChildren()[0];
    assert(pNode->isTerminal());

    AbstractASTNode *parent = nodeStack.top();
    StringLiteral *pStringLiteral = new StringLiteral(pNode->getId());

    exprCommonAction(parent, pStringLiteral, "StrLiteral");
}

// 处理数组和结构体元素选择
void ASTBuilder::enterSelector(CSTNode *node) {
    SelectorArray *selectorArray = new SelectorArray();

    // 三种selector('[]', '.', '->')的第二个词法单元都是终结符，可以据此判断类型
    assert(node->getChildren()[1]->isTerminal());
    if(node->getChildren()[1]->getType() == "[") {
        IndexSelector *indexSelector = new IndexSelector();
        selectorArray->addSelector(indexSelector);
    }
    if(node->getChildren()[1]->getType() == ".") {
        FieldSelector *fieldSelector = new FieldSelector();
        selectorArray->addSelector(fieldSelector);
    }
    else if(node->getChildren()[1]->getType() == "PTR_OP") {
        assert(0 && "In enterSelector: temporarily do not support PTR_OP");
    }
    else {
        assert(0 && "In enterSelector: invalid selector type");
    }

    nodeStack.push(selectorArray);
}

void ASTBuilder::quitSelector(CSTNode *node) {
    SEC_GET_STMT(SelectorArray)

    while(nodeStack.top()->isStmt()) {
        if(auto tempStmt = dynamic_cast<DerefSelector*>(nodeStack.top())) {
            pSelectorArray->addSelector(dynamic_cast<DerefSelector*>(tempStmt));
            nodeStack.pop();
            delete tempStmt;
        }
        else if(auto tempStmt = dynamic_cast<SelectorArray*>(nodeStack.top())) {
            // 这里假设若其上方有SelectorArray，则仅有一个selector
            // 所以加入后pSelectorArray直接用当前SelectorArray替换
            assert(tempStmt->getSelectors().size() == 1);
            for(auto selector : tempStmt->getSelectors())
                pSelectorArray->addSelector(selector);
            delete tempStmt;
            nodeStack.pop();
            tempStmt = pSelectorArray;
            return;
        }
        else break;
    }

    AbstractASTNode *parent = nodeStack.top();
    exprCommonAction(parent, pSelectorArray, "Selector");
}

// 处理函数调用
void ASTBuilder::enterFuncCall(CSTNode *node) {
    CallExpr *callExpr = new CallExpr();
    nodeStack.push(callExpr);
}

void ASTBuilder::quitFuncCall(CSTNode *node) {
    SEC_GET_STMT(CallExpr)

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_VirtualStmt) {
        dynamic_cast<VirtualStmt*>(parent)->setRealStmt(pCallExpr);
        return;
    }

    raiseRuleViolation("FuncCall", parent);
}

// 处理一元后缀运算符
void ASTBuilder::enterPostOp(CSTNode *node) {
    assert(node->getChildren().size() == 2 && node->getChildren()[1]->isTerminal());
    UnaryOperator *unaryOperator = new UnaryOperator();
    std::string opKind = node->getChildren()[1]->getType();

    if(opKind == "INC_OP")
        unaryOperator->setOp(UnaryOperator::_post_inc);
    else if(opKind == "DEC_OP")
        unaryOperator->setOp(UnaryOperator::_post_dec);
    else
        assert(false && "invalid Post Op");
    nodeStack.push(unaryOperator);
}

void ASTBuilder::quitPostOp(CSTNode *node) {
    SEC_GET_STMT(UnaryOperator)

    AbstractASTNode *parent = nodeStack.top();
    exprCommonAction(parent, pUnaryOperator, "PostOp");
}

// 处理一元前缀运算符
void ASTBuilder::enterPreOp(CSTNode *node) {
    assert(node->getChildren().size() == 2);
    std::string opKind;
    if(node->getChildren()[0]->isTerminal())
        opKind = node->getChildren()[0]->getType();
    else {
        CSTNode *pNode = node->getChildren()[0];
        assert(pNode->getChildren().size() == 1 && pNode->getChildren()[0]->isTerminal());
        opKind = pNode->getChildren()[0]->getType();
    }

    if(opKind == "*") {
        DerefSelector *derefSelector = new DerefSelector();
        nodeStack.push(derefSelector);
        return;
    }

    UnaryOperator *unaryOperator = new UnaryOperator();
    if(opKind == "INC_OP")
        unaryOperator->setOp(UnaryOperator::_pre_inc);
    else if(opKind == "DEC_OP")
        unaryOperator->setOp(UnaryOperator::_pre_dec);
    else if(opKind == "&")
        unaryOperator->setOp(UnaryOperator::_address_of);
    else if(opKind == "~")
        unaryOperator->setOp(UnaryOperator::_bit_not);
    else if(opKind == "!")
        unaryOperator->setOp(UnaryOperator::_not);
    else if(opKind == "+")
        unaryOperator->setOp(UnaryOperator::_unary_plus);
    else if(opKind == "-")
        unaryOperator->setOp(UnaryOperator::_unary_minus);
    else
        assert(false && "invalid Pre Op");
    nodeStack.push(unaryOperator);
}

void ASTBuilder::quitPreOp(CSTNode *node) {
    // 对解引用特别处理（即不处理，因为会在selector处处理）
    if(!node->getChildren()[0]->isTerminal()) {
        std::string opKind;
        opKind = node->getChildren()[0]->getChildren()[0]->getType();
        if(opKind == "*") return;
    }

    SEC_GET_STMT(UnaryOperator)

    AbstractASTNode *parent = nodeStack.top();
    exprCommonAction(parent, pUnaryOperator, "PreOp");
}

// 处理显式类型转换
void ASTBuilder::enterCstExpr(CSTNode *node) {
    CastExpr *castExpr = new CastExpr();
    nodeStack.push(castExpr);
}

void ASTBuilder::quitCstExpr(CSTNode *node) {
    SEC_GET_STMT_RANGE(CastExpr)

    assert(pCastExpr->getKind() == Stmt::k_ExplicitCastExpr);

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_CallExpr) {
        dynamic_cast<CallExpr*>(parent)->addArg(pCastExpr);
        return;
    }
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_VarDecl) {
        dynamic_cast<VarDecl*>(parent)->setInitializer(pCastExpr);
        return;
    }

    raiseRuleViolation("CstExpr", parent);
}

// 处理二元运算表达式
void ASTBuilder::enterBinOp(CSTNode *node) {
    assert(node->getChildren().size() == 3);

    std::string opKind;
    if(node->getChildren()[1]->isTerminal())
        opKind = node->getChildren()[1]->getType();
    else {
        CSTNode *pNode = node->getChildren()[1];
        assert(pNode->getChildren().size() == 1 && pNode->getChildren()[0]->isTerminal());
        opKind = pNode->getChildren()[0]->getType();
    }

    BinaryOperator *binaryOperator = new BinaryOperator();
    assert(opMap.find(opKind) != opMap.end());
    binaryOperator->setOp(opMap[opKind]);
    nodeStack.push(binaryOperator);
}

void ASTBuilder::quitBinOp(CSTNode *node) {
    SEC_GET_STMT(BinaryOperator)

    AbstractASTNode *parent = nodeStack.top();
    exprCommonAction(parent, pBinaryOperator, "BinOp");
}

// 处理变量声明
void ASTBuilder::enterDeclaration(CSTNode *node) {
    DeclStmt *declStmt = new DeclStmt();
    nodeStack.push(declStmt);
}

void ASTBuilder::quitDeclaration(CSTNode *node) {
    SEC_GET_STMT(DeclStmt);

    AbstractASTNode *parent = nodeStack.top();
    // 取消进入语句时定义的全局类型拷贝
    declTypeCp = QualType();
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_TranslationUnitDecl) {
        dynamic_cast<TranslationUnitDecl*>(parent)->addStmt(pDeclStmt);
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_CompoundStmt) {
        dynamic_cast<CompoundStmt*>(parent)->addStmt(pDeclStmt);
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_ForStmt) {
        dynamic_cast<ForStmt*>(parent)->setInit(pDeclStmt);
        return;
    }

    raiseRuleViolation("Declaration", parent);
}

// 处理基本类别(Int, Bool等)指示符
void ASTBuilder::enterCommonSpName(CSTNode *node) {}

//Fixme:
// 对于这类节点，有没有更优雅的实现
void ASTBuilder::quitCommonSpName(CSTNode *node) {
    assert(node->getChildren().size() == 1);
    CSTNode *pnode = node->getChildren()[0];
    assert(pnode->isTerminal());

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isQualType()) {
        if (pnode->getType() == "VOID") {
            dynamic_cast<QualType*>(parent)->setType(new BuiltInType(BuiltInType::_void));
            return;
        }
        if (pnode->getType() == "INT") {
            dynamic_cast<QualType*>(parent)->setType(new BuiltInType(BuiltInType::_int));
            return;
        }
        if (pnode->getType() == "CHAR") {
            dynamic_cast<QualType*>(parent)->setType(new BuiltInType(BuiltInType::_char));
            return;
        }
        if (pnode->getType() == "BOOL") {
            dynamic_cast<QualType*>(parent)->setType(new BuiltInType(BuiltInType::_bool));
            return;
        }
        if (pnode->getType() == "SHORT") {
            dynamic_cast<QualType*>(parent)->setType(new BuiltInType(BuiltInType::_short));
            return;
        }
        if (pnode->getType() == "LONG") {
            dynamic_cast<QualType*>(parent)->setType(new BuiltInType(BuiltInType::_long));
            return;
        }
        if (pnode->getType() == "FLOAT") {
            dynamic_cast<QualType*>(parent)->setType(new BuiltInType(BuiltInType::_float));
            return;
        }
        if (pnode->getType() == "DOUBLE") {
            dynamic_cast<QualType*>(parent)->setType(new BuiltInType(BuiltInType::_double));
            return;
        }
    }

    raiseRuleViolation("CommonSpName", parent);
}

// 处理基本Qualifier
void ASTBuilder::enterQfName(CSTNode *node) {}

void ASTBuilder::quitQfName(CSTNode *node) {
    assert(node->getChildren().size() == 1);
    CSTNode *pNode = node->getChildren()[0];
    assert(pNode->isTerminal());

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isQualType()) {
        if (pNode->getType() == "CONST") {
            dynamic_cast<QualType*>(parent)->setConst();
            return;
        }
        if (pNode->getType() == "VOLATILE") {
            dynamic_cast<QualType*>(parent)->setVolatile();
            return;
        }
        if (pNode->getType() == "RESTRICT") {
            dynamic_cast<QualType*>(parent)->setRestrict();
            return;
        }
        if (pNode->getType() == "ATOMIC") {
            dynamic_cast<QualType*>(parent)->setAtomic();
            return;
        }
    }

    raiseRuleViolation("QfName", parent);
}

// 处理复合变量(包括函数)定义的类型
//Fixme:
// 上方是否可能重叠类型
void ASTBuilder::enterTypeSp(CSTNode *node) {
    QualType *qualType = new QualType();
    nodeStack.push(qualType);
}

void ASTBuilder::quitTypeSp(CSTNode *node) {
    SEC_GET_QAULTYPE()

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_FunctionDecl) {
        dynamic_cast<FunctionDecl*>(parent)->setQualType(*pQualType);
        return;
    }
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_ParamVarDecl) {
        dynamic_cast<ParamVarDecl*>(parent)->setQualType(*pQualType);
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_DeclStmt) {
        // 为保证翻译正确性，一定要记得取消
        declTypeCp = *pQualType;
        return;
    }
    if(parent->isQualType()) {
        dynamic_cast<QualType*>(parent)->setType(pQualType->getType());
        return;
    }

    raiseRuleViolation("TypeSp", parent);
}

// 处理复合Qualifier
//Fixme:
// 上方是否可能重叠类型
void ASTBuilder::enterTypeQf(CSTNode *node) {
    QualType *qualType = new QualType();
    nodeStack.push(qualType);
}

void ASTBuilder::quitTypeQf(CSTNode *node) {
    SEC_GET_QAULTYPE()

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_FunctionDecl) {
        dynamic_cast<FunctionDecl*>(parent)->setQualType(*pQualType);
        return;
    }
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_ParamVarDecl) {
        dynamic_cast<ParamVarDecl*>(parent)->setQualType(*pQualType);
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_DeclStmt) {
        // 为保证翻译正确性，一定要记得取消
        declTypeCp = *pQualType;
        return;
    }
    if(parent->isQualType()) {
        if(pQualType->isConst())
            dynamic_cast<QualType*>(parent)->setConst();
        if(pQualType->isVolatile())
            dynamic_cast<QualType*>(parent)->setVolatile();
        if(pQualType->isRestrict())
            dynamic_cast<QualType*>(parent)->setRestrict();
        if(pQualType->isAtomic())
            dynamic_cast<QualType*>(parent)->setAtomic();
        return;
    }

    raiseRuleViolation("TypeQf", parent);
}

// 处理变量定义语句中的具体变量定义
void ASTBuilder::enterDeclarator(CSTNode *node) {
    // 若断言失败，则类型未初始化
    assert(!declTypeCp.isUncertainType());
    VarDecl *varDecl = new VarDecl();
    varDecl->setQualType(declTypeCp);
    nodeStack.push(varDecl);
}

void ASTBuilder::quitDeclarator(CSTNode *node) {
    SEC_GET_DECL(VarDecl);
    // 若断言失败，则类型未初始化
    assert(!(pVarDecl->getQualType().isUncertainType()));

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_DeclStmt) {
        dynamic_cast<DeclStmt*>(parent)->addDecl(pVarDecl);
        return;
    }

    raiseRuleViolation("FuncDef", parent);
}

// 为变量定义语句中的具体变量添加变量名
void ASTBuilder::enterDeclaratorName(CSTNode *node) {}

void ASTBuilder::quitDeclaratorName(CSTNode *node) {
    assert(node->getChildren().size() == 1);
    CSTNode *pNode = node->getChildren()[0];
    assert(pNode->isTerminal());

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_FunctionDecl) {
        dynamic_cast<FunctionDecl*>(parent)->setName(pNode->getId());
        return;
    }
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_VarDecl) {
        dynamic_cast<VarDecl*>(parent)->setName(pNode->getId());
        return;
    }
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_ParamVarDecl) {
        dynamic_cast<ParamVarDecl*>(parent)->setName(pNode->getId());
        return;
    }
    if(parent->isQualType() &&
            Type::first_ArrayType <= dynamic_cast<QualType*>(parent)->getTypeKind() &&
            dynamic_cast<QualType*>(parent)->getTypeKind() <= Type::last_ArrayType) {
        hasStoreName = true;
        storedName = pNode->getId();
        return;
    }
    raiseRuleViolation("DeclaratorName", parent);
}

// 处理正常数组定义
// 先默认为可变数组
// 在类型检查时判断是否可以是定长数组

//Fixme:
// 数组类型还未处理指示符的转移
// e.g. int a[const 10]
void ASTBuilder::enterArray(CSTNode *node) {
    QualType *qualType = new QualType();
    VariableArrayType *tempType = new VariableArrayType();
    qualType->setType(tempType);
    nodeStack.push(qualType);
}

void ASTBuilder::quitArray(CSTNode *node) {
    SEC_GET_QAULTYPE();

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_VarDecl) {
        //Fixme:
        // 数组部分还未检查
        // 如果数组部分出错
        // 此处为最有可能出错的地方
        QualType oldQualType = dynamic_cast<VarDecl*>(parent)->getQualType();
        VariableArrayType *newType = dynamic_cast<VariableArrayType*>(pQualType->getType());
        assert(newType);
        newType->setElementType(oldQualType.getType());
        oldQualType.setType(newType);
        dynamic_cast<VarDecl*>(parent)->setQualType(oldQualType);

        if(hasStoreName) {
            dynamic_cast<VarDecl*>(parent)->setName(storedName);
            hasStoreName = false;
        }

        return;
    }
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_ParamVarDecl) {
        QualType oldQualType = dynamic_cast<ParamVarDecl*>(parent)->getQualType();
        VariableArrayType *newType = dynamic_cast<VariableArrayType*>(pQualType->getType());
        assert(newType);
        newType->setElementType(oldQualType.getType());
        oldQualType.setType(newType);
        dynamic_cast<ParamVarDecl*>(parent)->setQualType(oldQualType);

        if(hasStoreName) {
            dynamic_cast<VarDecl*>(parent)->setName(storedName);
            hasStoreName = false;
        }

        return;
    }
    //Fixme:
    // 急着洗澡，写得仓促，可能有错
    if(parent->isQualType() && dynamic_cast<QualType*>(parent)->getTypeKind() == Type::k_VariableArrayType) {
        // 这里假设不会是incompleteType
        dynamic_cast<QualType*>(parent)->setType(pQualType->getType());
        return;
    }

    raiseRuleViolation("Array", parent);
}

// 处理static数组定义
// 基本与正常数组相同
void ASTBuilder::enterStaticModifierArray(CSTNode *) {
    QualType *qualType = new QualType();
    VariableArrayType *tempType = new VariableArrayType();
    tempType->setStatic();
    qualType->setType(tempType);
    nodeStack.push(qualType);
}

void ASTBuilder::quitStaticModifierArray(CSTNode *node) {
    SEC_GET_QAULTYPE();

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_VarDecl) {
        QualType oldQualType = dynamic_cast<VarDecl*>(parent)->getQualType();
        VariableArrayType *newType = dynamic_cast<VariableArrayType*>(pQualType->getType());
        assert(newType);
        newType->setElementType(oldQualType.getType());
        oldQualType.setType(newType);
        dynamic_cast<VarDecl*>(parent)->setQualType(oldQualType);

        if(hasStoreName) {
            dynamic_cast<VarDecl*>(parent)->setName(storedName);
            hasStoreName = false;
        }

        return;
    }
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_ParamVarDecl) {
        QualType oldQualType = dynamic_cast<ParamVarDecl*>(parent)->getQualType();
        VariableArrayType *newType = dynamic_cast<VariableArrayType*>(pQualType->getType());
        assert(newType);
        newType->setElementType(oldQualType.getType());
        oldQualType.setType(newType);
        dynamic_cast<ParamVarDecl*>(parent)->setQualType(oldQualType);

        if(hasStoreName) {
            dynamic_cast<VarDecl*>(parent)->setName(storedName);
            hasStoreName = false;
        }

        return;
    }
    if(parent->isQualType() && dynamic_cast<QualType*>(parent)->getTypeKind() == Type::k_VariableArrayType) {
        // 这里假设不会是incompleteType
        dynamic_cast<QualType*>(parent)->setType(pQualType->getType());
        return;
    }

    raiseRuleViolation("StaticArray", parent);
}

// 处理Incomplete数组定义
// 基本与正常数组相同
void ASTBuilder::enterIncompleteArray(CSTNode *) {
    QualType *qualType = new QualType();
    IncompleteArrayType *tempType = new IncompleteArrayType();
    qualType->setType(tempType);
    nodeStack.push(qualType);
}

void ASTBuilder::quitIncompleteArray(CSTNode *node) {
    SEC_GET_QAULTYPE();

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_VarDecl) {
        QualType oldQualType = dynamic_cast<VarDecl*>(parent)->getQualType();
        IncompleteArrayType *newType = dynamic_cast<IncompleteArrayType*>(pQualType->getType());
        assert(newType);
        newType->setElementType(oldQualType.getType());
        oldQualType.setType(newType);
        dynamic_cast<VarDecl*>(parent)->setQualType(oldQualType);

        if(hasStoreName) {
            dynamic_cast<VarDecl*>(parent)->setName(storedName);
            hasStoreName = false;
        }

        return;
    }
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_ParamVarDecl) {
        QualType oldQualType = dynamic_cast<ParamVarDecl*>(parent)->getQualType();
        IncompleteArrayType *newType = dynamic_cast<IncompleteArrayType*>(pQualType->getType());
        assert(newType);
        newType->setElementType(oldQualType.getType());
        oldQualType.setType(newType);
        dynamic_cast<ParamVarDecl*>(parent)->setQualType(oldQualType);

        if(hasStoreName) {
            dynamic_cast<VarDecl*>(parent)->setName(storedName);
            hasStoreName = false;
        }

        return;
    }

    raiseRuleViolation("IncompleteArray", parent);
}

// 处理指针定义
void ASTBuilder::enterPointer(CSTNode *) {}

void ASTBuilder::quitPointer(CSTNode *node) {

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_VarDecl) {
        PointerType *newType = new PointerType();
        QualType * qualType = new QualType(dynamic_cast<VarDecl*>(parent)->getQualType());
        newType->setPointeeType(qualType);
        dynamic_cast<VarDecl*>(parent)->setQualType(QualType(newType));
        return;
    }
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_ParamVarDecl) {
        PointerType *newType = new PointerType();
        QualType * qualType = new QualType(dynamic_cast<ParamVarDecl*>(parent)->getQualType());
        newType->setPointeeType(qualType);
        dynamic_cast<VarDecl*>(parent)->setQualType(QualType(newType));
        return;
    }

    raiseRuleViolation("Pointer", parent);
}

// 处理函数形参定义
void ASTBuilder::enterFormalParamDef(CSTNode *node) {
    ParamVarDecl *paramVarDecl = new ParamVarDecl();
    nodeStack.push(paramVarDecl);
}

void ASTBuilder::quitFormalParamDef(CSTNode *node) {
    SEC_GET_DECL(ParamVarDecl)

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_FunctionDecl) {
        dynamic_cast<FunctionDecl*>(parent)->addParam(pParamVarDecl);
        return;
    }

    raiseRuleViolation("FormalParamDef", parent);
}

// 处理显式类型转换中的转换类型
void ASTBuilder::enterFullType(CSTNode *node) {
    QualType *qualType = new QualType();
    nodeStack.push(qualType);
}

void ASTBuilder::quitFullType(CSTNode *node) {
    SEC_GET_QAULTYPE()

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_CastExpr) {
        nodeStack.pop();
        ExplicitCastExpr *explicitCastExpr = new ExplicitCastExpr(*pQualType,
                                                                  dynamic_cast<CastExpr*>(parent)->getCastedExpr());
        delete parent;
        nodeStack.push(explicitCastExpr);
        return;
    }

    raiseRuleViolation("FullType", parent);
}

/*// 处理变量定义中的简单初始化
void ASTBuilder::enterSingleInit(CSTNode *node) {
    Expr *expr = new Expr();
    nodeStack.push(expr);
}

void ASTBuilder::quitSingleInit(CSTNode *node) {
    SEC_GET_STMT_RANGE(Expr);

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_VarDecl) {
        dynamic_cast<VarDecl*>(parent)->setInitializer(pExpr);
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_ExplicitCastExpr) {
        dynamic_cast<CastExpr*>(parent)->setCastedExpr(pExpr);
        return;
    }

    raiseRuleViolation("SingleInit", parent);
}*/

// 处理虚拟Stmt节点
// 关于此节点的功能见其定义
void ASTBuilder::enterVirtualStmt(CSTNode *node) {
    VirtualStmt *virtualStmt = new VirtualStmt();
    nodeStack.push(virtualStmt);
}

void ASTBuilder::quitVirtualStmt(CSTNode *node) {
    SEC_GET_STMT(VirtualStmt)
    assert(pVirtualStmt->hasRealStmt());

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_CompoundStmt) {
        dynamic_cast<CompoundStmt*>(parent)->addStmt(pVirtualStmt->getRealStmt());
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_IfStmt) {
        if(!dynamic_cast<IfStmt*>(parent)->hasThen())
            dynamic_cast<IfStmt*>(parent)->setThen(pVirtualStmt->getRealStmt());
        else
            dynamic_cast<IfStmt*>(parent)->setElse(pVirtualStmt->getRealStmt());
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_DoWhileStmt) {
        dynamic_cast<WhileStmt*>(parent)->setBody(pVirtualStmt->getRealStmt());
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_WhileStmt) {
        dynamic_cast<WhileStmt*>(parent)->setBody(pVirtualStmt->getRealStmt());
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_ForStmt) {
        dynamic_cast<ForStmt*>(parent)->setBody(pVirtualStmt->getRealStmt());
        return;
    }

    raiseRuleViolation("VirtualStmt", parent);
}

// 处理复合语句
void ASTBuilder::enterCmpdStmt(CSTNode *node) {
    CompoundStmt *compoundStmt = new CompoundStmt();
    nodeStack.push(compoundStmt);
}

void ASTBuilder::quitCmpdStmt(CSTNode *node) {
    SEC_GET_STMT(CompoundStmt)

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_FunctionDecl) {
        dynamic_cast<FunctionDecl*>(parent)->setBody(pCompoundStmt);
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_VirtualStmt) {
        dynamic_cast<VirtualStmt*>(parent)->setRealStmt(pCompoundStmt);
        return;
    }

    raiseRuleViolation("CmpdStmt", parent);
}

// 处理If语句
// 这里的实现将IF和IF_ELSE分开
// 仅为安全性考虑，因为不确定移入-规约冲突是否被正确解决
void ASTBuilder::enterIfStmt(CSTNode *node) {
    IfStmt *ifStmt = new IfStmt();
    nodeStack.push(ifStmt);
}

void ASTBuilder::quitIfStmt(CSTNode *node) {
    SEC_GET_STMT(IfStmt)
    assert(pIfStmt->hasThen());

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_VirtualStmt) {
        dynamic_cast<VirtualStmt*>(parent)->setRealStmt(pIfStmt);
        return;
    }

    raiseRuleViolation("IfStmt", parent);
}

// 处理If-Else语句
// 这里的实现将IF和IF_ELSE分开
// 仅为安全性考虑，因为不确定移入-规约冲突是否被正确解决
void ASTBuilder::enterIfElseStmt(CSTNode *node) {
    IfStmt *ifStmt = new IfStmt();
    nodeStack.push(ifStmt);
}

void ASTBuilder::quitIfElseStmt(CSTNode *node) {
    SEC_GET_STMT(IfStmt)
    assert(pIfStmt->hasElse());

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_VirtualStmt) {
        dynamic_cast<VirtualStmt*>(parent)->setRealStmt(pIfStmt);
        return;
    }

    raiseRuleViolation("IfElseStmt", parent);
}

// 处理While循环
// 这里的实现将WHILE和DO_WHILE分开
// 因为执行时有区别
void ASTBuilder::enterDoWhileStmt(CSTNode *node) {
    DoWhileStmt *doWhileStmt = new DoWhileStmt();
    nodeStack.push(doWhileStmt);
}

void ASTBuilder::quitDoWhileStmt(CSTNode *node) {
    SEC_GET_STMT(DoWhileStmt)

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_VirtualStmt) {
        dynamic_cast<VirtualStmt*>(parent)->setRealStmt(pDoWhileStmt);
        return;
    }

    raiseRuleViolation("DoWhileStmt", parent);
}

// 处理While循环
// 这里的实现将WHILE和DO_WHILE分开
// 因为参数设置上有区别
void ASTBuilder::enterWhileStmt(CSTNode *node) {
    WhileStmt *whileStmt = new WhileStmt();
    nodeStack.push(whileStmt);
}

void ASTBuilder::quitWhileStmt(CSTNode *node) {
    SEC_GET_STMT(WhileStmt)

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_VirtualStmt) {
        dynamic_cast<VirtualStmt*>(parent)->setRealStmt(pWhileStmt);
        return;
    }

    raiseRuleViolation("WhileStmt", parent);
}

// 处理For循环
void ASTBuilder::enterForStmt(CSTNode *node) {
    ForStmt *forStmt = new ForStmt();
    nodeStack.push(forStmt);
}

void ASTBuilder::quitForStmt(CSTNode *node) {
    SEC_GET_STMT(ForStmt)

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_VirtualStmt) {
        dynamic_cast<VirtualStmt*>(parent)->setRealStmt(pForStmt);
        return;
    }

    raiseRuleViolation("ForStmt", parent);
}

// 处理返回语句
void ASTBuilder::enterReturn( CSTNode* node ) {
    ReturnStmt *returnStmt = new ReturnStmt();
    nodeStack.push(returnStmt);
}

void ASTBuilder::quitReturn( CSTNode* node ) {
    SEC_GET_STMT(ReturnStmt);

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_VirtualStmt) {
        dynamic_cast<VirtualStmt*>(parent)->setRealStmt(pReturnStmt);
        return;
    }

    raiseRuleViolation("Return", parent);
}

// 处理函数定义
void ASTBuilder::enterFuncDef( CSTNode* node ) {
    FunctionDecl *functionDecl = new FunctionDecl("__NULL", QualType());
    nodeStack.push(functionDecl);
}

void ASTBuilder::quitFuncDef( CSTNode* node ) {
    SEC_GET_DECL(FunctionDecl);

    AbstractASTNode *parent = nodeStack.top();
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_TranslationUnitDecl) {
        dynamic_cast<TranslationUnitDecl*>(parent)->addDecl(pFunctionDecl);
        return;
    }

    raiseRuleViolation("FuncDef", parent);
}

void ASTBuilder::exprCommonAction(AbstractASTNode *parent, Expr *pExpr, std::string ctx) {
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_CallExpr) {
        dynamic_cast<CallExpr*>(parent)->addArg(pExpr);
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_ExplicitCastExpr) {
        dynamic_cast<CastExpr*>(parent)->setCastedExpr(pExpr);
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_UnaryOperator) {
        dynamic_cast<UnaryOperator*>(parent)->setSubExpr(pExpr);
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_BinaryOperator) {
        auto pParent = dynamic_cast<BinaryOperator*>(parent);
        if(!pParent->hasLHS()) {
            pParent->setLHS(pExpr);
        }
        else if(!pParent->hasRHS()) {
            pParent->setRHS(pExpr);
        }
        else raiseRuleViolation(ctx, parent);

        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_VirtualStmt) {
        dynamic_cast<VirtualStmt*>(parent)->setRealStmt(pExpr);
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_SelectorArray) {
        auto pParent = dynamic_cast<SelectorArray*>(parent);
        if(!pParent->hasSubExpr())
            pParent->setSubExpr(pExpr);
        else {
            int numSelectors = pParent->getNumSelectors();
            auto tempSelector = pParent->getSelector(numSelectors - 1);
            if (auto tempIndexSelector = dynamic_cast<IndexSelector*>(tempSelector)) {
                tempIndexSelector->setIdxExpr(pExpr);
            }
            else if(auto tempFieldSelector = dynamic_cast<FieldSelector*>(tempSelector)) {
                assert(pExpr->getKind() == Stmt::k_DeclRefExpr);
                tempFieldSelector->setName(dynamic_cast<DeclRefExpr*>(pExpr)->getRefName());
            }
        }

        return;
    }
    if(parent->isQualType() && dynamic_cast<QualType*>(parent)->getTypeKind() == Type::k_VariableArrayType) {
        if(VariableArrayType* tempType =
                dynamic_cast<VariableArrayType*>(dynamic_cast<QualType*>(parent)->getType()))
            tempType->setSizeExpr(pExpr);
        return;
    }
    // 循环，选择条件
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_IfStmt) {
        dynamic_cast<IfStmt*>(parent)->setCond(pExpr);
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_DoWhileStmt) {
        dynamic_cast<DoWhileStmt*>(parent)->setCond(pExpr);
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_WhileStmt) {
        dynamic_cast<WhileStmt*>(parent)->setCond(pExpr);
        return;
    }
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_ForStmt) {
        if(!dynamic_cast<ForStmt*>(parent)->hasCond())
            dynamic_cast<ForStmt*>(parent)->setCond(pExpr);
        else if(!dynamic_cast<ForStmt*>(parent)->hasInc())
            dynamic_cast<ForStmt*>(parent)->setInc(pExpr);
        else std::cout << "Error in IfStmt ( attempt to fill when both condition and Inc are filled )." << std::endl;
        return;
    }
    // 为变量初始化
    if(parent->isStmt() && dynamic_cast<Stmt*>(parent)->getKind() == Stmt::k_ReturnStmt) {
        dynamic_cast<ReturnStmt*>(parent)->setRetValue(pExpr);
        return;
    }
    if(parent->isDecl() && dynamic_cast<Decl*>(parent)->getKind() == Decl::k_VarDecl) {
        dynamic_cast<VarDecl*>(parent)->setInitializer(pExpr);
        return;
    }

    raiseRuleViolation(ctx, parent);
}

void raiseRuleViolation( std::string ctx, AbstractASTNode* parent ) {
    std::cout << "Error in " << ctx << " ( Parent is ";
    assert(!parent->isQualType());
    std::cout << (parent->isDecl() ? "Decl:" : "Stmt:") << (parent->isDecl() ?
                    static_cast<short>(dynamic_cast<Decl*>(parent)->getKind()) :
                    static_cast<short>(dynamic_cast<Stmt*>(parent)->getKind())) << " )." << std::endl;
}