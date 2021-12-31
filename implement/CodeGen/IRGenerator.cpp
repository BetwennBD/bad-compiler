//
// Created by tangny on 2021/12/14.
//

#include "include/IRGenerator/IRGenerator.h"

// 辅助函数
void IRGenerator::writeGlobalVariable(std::string name, llvm::Value *value) {
    builder->CreateStore(value, globals[name]);
}

llvm::Value* IRGenerator::readGlobalVariable(DeclRefExpr *expr) {
    return builder->CreateLoad(convertType(expr->getQualType()), globals[expr->getRefName()]);
}

void IRGenerator::writeLocalVariable(llvm::BasicBlock *BB, std::string name, llvm::Value *value) {
    currentBlockInfo[BB].defs[name] = value;
}

llvm::Value* IRGenerator::readLocalVariable(llvm::BasicBlock *BB, DeclRefExpr *expr) {
    auto value = currentBlockInfo[BB].defs.find(expr->getRefName());
    if(value != currentBlockInfo[BB].defs.end()) {
        return value->second;
    }
    return readLocalVariableRecursive(BB, expr);
}

llvm::Value* IRGenerator::readLocalVariableRecursive(llvm::BasicBlock *BB, DeclRefExpr *expr) {
    llvm::Value *value = nullptr;
    // 块未密封
    if(!currentBlockInfo[BB].sealed) {
        llvm::Type *Ty = convertType(expr->getQualType());
        if(Ty->isStructTy() || Ty->isArrayTy())
            Ty = llvm::PointerType::getUnqual(Ty);
        llvm::PHINode *phi = addEmptyPhi(BB, Ty);
        currentBlockInfo[BB].incompletePhis[phi] = expr;
        value = phi;
    }
    // 块有且仅有一个前块
    else if(auto *predBB = BB->getSinglePredecessor()) {
        value = readLocalVariable(predBB, expr);
    }
    // 块有多个前块
    else {
        llvm::Type *Ty = convertType(expr->getQualType());
        if(Ty->isStructTy() || Ty->isArrayTy())
            Ty = llvm::PointerType::getUnqual(Ty);
        llvm::PHINode *phi = addEmptyPhi(BB, Ty);
        writeLocalVariable(BB, expr->getRefName(), phi);
        addPhiOperands(BB, expr, phi);
        value = phi;
    }

    writeLocalVariable(BB, expr->getRefName(), value);
    return value;
}

void IRGenerator::writeVariable(llvm::BasicBlock *BB, std::string name, llvm::Value *value) {
    if(ports.find(name) != ports.end())
        writePort(name, value);

    if(globals.find(name) != globals.end())
        writeGlobalVariable(name, value);
    else
        writeLocalVariable(BB, name, value);
}

llvm::Value* IRGenerator::readVariable(llvm::BasicBlock *BB, DeclRefExpr *expr) {
    if(ports.find(expr->getRefName()) != ports.end())
        return readPort(expr);

    if(globals.find(expr->getRefName()) != globals.end())
        return readGlobalVariable(expr);
    else
        return readLocalVariable(BB, expr);
}

llvm::PHINode* IRGenerator::addEmptyPhi(llvm::BasicBlock *BB, llvm::Type *type) {
    return BB->empty()
        ? llvm::PHINode::Create(type, 0, "", BB)
        : llvm::PHINode::Create(type, 0, "", &BB->front());
}

llvm::Value* IRGenerator::readPort(DeclRefExpr *expr) {
    llvm::Value *addrAddr;
    if(!(addrAddr = ports[expr->getRefName()])) {
        llvm::report_fatal_error("In writePort: cannot find specified port");
    }

    llvm::Value *addr = builder->CreateLoad(Int32PtrTy, addrAddr);
    return builder->CreateLoad(Int32Ty, addr);
}

void IRGenerator::writePort(std::string name, llvm::Value *value) {
    llvm::Value *addrAddr;
    if(!(addrAddr = ports[name])) {
        llvm::report_fatal_error("In writePort: cannot find specified port");
    }

    llvm::Value *addr = builder->CreateLoad(Int32PtrTy, addrAddr);
    builder->CreateStore(value, addr);
}

void IRGenerator::addPhiOperands(llvm::BasicBlock *BB, DeclRefExpr *expr, llvm::PHINode *phi) {
    for(auto iter = llvm::pred_begin(BB), end = llvm::pred_end(BB);iter != end;++iter)
        phi->addIncoming(readLocalVariable(*iter, expr), *iter);
}

void IRGenerator::sealBlock(llvm::BasicBlock *BB) {
    for(auto phiPair : currentBlockInfo[BB].incompletePhis) {
        addPhiOperands(BB, phiPair.second, phiPair.first);
    }

    currentBlockInfo[BB].incompletePhis.clear();
    currentBlockInfo[BB].sealed = true;
}

llvm::Type* IRGenerator::convertType( QualType qualType ) {
    Type *myType = qualType.getType();
    if(llvm::Type *cache = typeCache[myType]) {
        return cache;
    }

    if(auto biType = dynamic_cast<BuiltInType*>(myType)) {
        switch(biType->getTypeType()) {
            case(BuiltInType::_int):
            case(BuiltInType::_unsigned):
                return Int32Ty;
            case(BuiltInType::_bool):
                return Int1Ty;
            case(BuiltInType::_void):
                return VoidTy;
            case(BuiltInType::_char):
                return Int8Ty;
            case(BuiltInType::_float):
                return FloatTy;
            case(BuiltInType::_double):
                llvm_unreachable("In convertType: current system do not support double");
        }
    }
    else if(auto pType = dynamic_cast<PointerType*>(myType)) {
        llvm::Type *T = llvm::PointerType::get(convertType(QualType(*pType->getPointeeType())), 0);
        return T;
    }
    else if(auto caType = dynamic_cast<ConstArrayType*>(myType)) {
        //Fixme:
        // 不知道多维数组是否可以递归生成
        llvm::Type *component = convertType(QualType(caType->getElementType()));
        int numElements = caType->getLength();
        llvm::Type *T = llvm::ArrayType::get(component, numElements);
        return typeCache[myType] = T;
    }
    else if(auto caType = dynamic_cast<IncompleteArrayType*>(myType)) {
        //Fixme:
    }
    else if(auto rcType = dynamic_cast<RecordType*>(myType)) {
        llvm::SmallVector<llvm::Type*, 5> llvmMembers;
        for(auto iniM : rcType->members)
            llvmMembers.push_back(convertType(iniM));
        llvm::Type *T = llvm::StructType::create(llvmMembers, rcType->getName(), false);
        return typeCache[myType] = T;
    }

    std::cout << "Got type#" << qualType.getType()->getKind() << std::endl;
    llvm_unreachable("In convertType: Unsupported type");
}

// 释放llvm函数ir
llvm::Function* IRGenerator::emitFunctionDecl(FunctionDecl *D) {
    llvm::Function *F = module->getFunction(D->getName());

    if(!F) {
        //Fixme:
        // 还未处理数组参数
        int a = 10;
        llvm::SmallVector<llvm::Type *, 5> paramTypes;
        for(int i = 0;i < D->getNumParams();++i)
            paramTypes.push_back(convertType(D->getParam(i)->getQualType()));
        llvm::FunctionType *FT =
                llvm::FunctionType::get(convertType(D->getQualType()), paramTypes, false);
        F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, D->getName(), *module);
    }

    if(!F) {
        std::cout << "cannot create function.\n" << std::endl;
        return nullptr;
    }

    int idx = 0;
    std::string tempName;
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(*context, "entry", F);
    setCurrentBlock(BB);

    for(auto &arg : F->args()) {
        tempName = D->getParam(idx)->getName();
        arg.setName(tempName);
        //Fixme: 没有理解
        llvm::Argument *Arg = &arg;
        writeLocalVariable(curBlock, tempName, Arg);
        idx++;
    }
    currentFunction = F;

    hasRet = false;
    emitCompoundStmt(D->getBody());

    if(!hasRet) {
        if(convertType(D->getQualType()) == VoidTy)
            builder->CreateRetVoid();
        else
            llvm::report_fatal_error("In emitFunctionDecl: function requires return value");
    }

    verifyFunction(*F);
    return F;
}

// 除Expr外Stmt的IR生成函数
void IRGenerator::emitCompoundStmt(CompoundStmt *S) {
    int numStmts = S->getNumStmts();
    for(int i = 0;i < numStmts;++i)
        emitStmtHelper(S->getStmt(i));

    //不要忘记密封最后一个块
    sealBlock(curBlock);
}

void IRGenerator::emitGlobalDecl(DeclStmt *S) {
    for(auto varDecl : S->varDecls) {
        //Fixme: 全局变量不能设置初始值
        llvm::GlobalVariable *gv = new llvm::GlobalVariable(
                *module,
                convertType(varDecl->getQualType()),
                varDecl->getQualType().isConst(),
                llvm::GlobalValue::PrivateLinkage,
                nullptr,
                varDecl->getName());

        // 设置初始值
        if(varDecl->hasInitializer()) {
            if(auto intLit = dynamic_cast<IntegerLiteral*>(varDecl->getInitializer())) {
                gv->setInitializer(llvm::ConstantInt::get(Int32Ty, intLit->getValue()));
            }
        }

        globals[varDecl->getName()] = gv;
    }
}

void IRGenerator::emitLocalDecl(DeclStmt *S) {
    int numDecls = S->getNumDecls();
    for(auto varDecl : S->varDecls) {
        llvm::Value *init;
        if(varDecl->getQualType().getTypeKind() == Type::k_BuiltInType) {
            BuiltInType *builtInType = dynamic_cast<BuiltInType*>(varDecl->getQualType().getType());
            if(varDecl->hasInitializer()) {
                if(builtInType->getTypeType() == BuiltInType::_port) {
                    // llvm::Type *pointeeType = convertType(*dynamic_cast<PointerType*>(varDecl->getQualType().getType())->getPointeeType());
                    llvm::Type *pointerType = Int32PtrTy;
                    llvm::Value *ptAddr = builder->CreateAlloca(pointerType);
                    // std::cout << init->getType()->getPointerElementType()->getPointerElementType()->isPointerTy() << std::endl;
                    llvm::Value *newAddr = builder->CreateIntToPtr(emitExprHelper(varDecl->getInitializer()), Int32PtrTy);
                    builder->CreateStore(newAddr, ptAddr);
                    ports[varDecl->getName()] = ptAddr;
                    return;

                    llvm::Value *v = llvm::ConstantInt::get(Int32Ty, 1);
                }
                else
                    init = emitExprHelper(varDecl->getInitializer());
            }
            else {
                switch(builtInType->getTypeType()) {
                    case(BuiltInType::_int):
                        init = llvm::ConstantInt::get(Int32Ty, 0, true);
                        break;
                    case(BuiltInType::_unsigned):
                        init = llvm::ConstantInt::get(Int32Ty, 0, false);
                        break;
                    case(BuiltInType::_bool):
                        init = llvm::ConstantInt::get(Int1Ty, 1);
                        break;
                    case(BuiltInType::_float):
                        init = llvm::ConstantInt::get(FloatTy, 0);
                        break;
                    case(BuiltInType::_char):
                        init = llvm::ConstantInt::get(Int8Ty, 0);
                        break;
                    case(BuiltInType::_short):
                        init = llvm::ConstantInt::get(Int16Ty, 0);
                        break;
                    default:
                        llvm::report_fatal_error("In emitLocalDecl: current builtin type should be initialized explicitly");
                }
            }
        }
        // 指针类型暂不支持值的初始化
        else if(varDecl->getQualType().getTypeKind() == Type::k_PointerType) {
            if(!varDecl->hasInitializer()) {
                llvm::Type *pointeeType = convertType(
                        *dynamic_cast<PointerType *>(varDecl->getQualType().getType())->getPointeeType());
                init = builder->CreateAlloca(pointeeType);
            }
            else {
                init = builder->CreateIntToPtr(emitExprHelper(varDecl->getInitializer()), Int32PtrTy);
            }
        }
        else if(varDecl->getQualType().getTypeKind() == Type::k_ConstArrayType) {
            llvm::Type *arrayType = convertType(varDecl->getQualType());
            init = builder->CreateAlloca(arrayType);
        }
        else if(varDecl->getQualType().getTypeKind() == Type::k_RecordType) {
            llvm::Type *recordType = convertType(varDecl->getQualType());
            init = builder->CreateAlloca(recordType);
        }

        writeLocalVariable(curBlock,
                           varDecl->getName(),
                           init);
    }
}

void IRGenerator::emitForStmt(ForStmt *S) {
    llvm::BasicBlock *forInitBB =
            S->hasInit() ? createBasicBlock("for.init") : nullptr;
    llvm::BasicBlock *forCondBB;
//    llvm::BasicBlock *forIncBB =
//            S->hasInc() ? createBasicBlock("for.inc") : nullptr;
    llvm::BasicBlock *forBodyBB =
            createBasicBlock("for.body");
    llvm::BasicBlock *afterForBB =
            createBasicBlock("after.for");

    if(forInitBB) {
        builder->CreateBr(forInitBB);
        sealBlock(curBlock);
        setCurrentBlock(forInitBB);
        emitLocalDecl(S->getInit());
    }

    if(curBlock->empty() && currentBlockInfo[curBlock].defs.empty()) {
        curBlock->setName("for.cond");
        forCondBB = curBlock;
    }
    else {
        forCondBB = createBasicBlock("for.cond");
        builder->CreateBr(forCondBB);
        sealBlock(curBlock);
        setCurrentBlock(forCondBB);
    }

    llvm::Value *cond = emitExprHelper(S->getCond());
    builder->CreateCondBr(cond, forBodyBB, afterForBB);

    setCurrentBlock(forBodyBB);
    emitStmtHelper(S->getBody());
    //Fixme: 将Inc直接生成在前块中是否有问题？
    if(S->hasInc()) emitExprHelper(S->getInc());
    builder->CreateBr(forCondBB);

    sealBlock(forCondBB);
    sealBlock(curBlock);

    setCurrentBlock(afterForBB);
}

void IRGenerator::emitIfStmt(IfStmt *S) {
    llvm::BasicBlock *thenBB =
            createBasicBlock("if.then");
    llvm::BasicBlock *elseBB =
            S->hasElse() ? createBasicBlock("if.else") : nullptr;
    llvm::BasicBlock *afterIfBB =
            createBasicBlock("after.if");

    llvm::Value *cond = emitExprHelper(S->getCond());
    builder->CreateCondBr(cond, thenBB, elseBB ? elseBB : afterIfBB);
    sealBlock(curBlock);

    setCurrentBlock(thenBB);
    emitStmtHelper(S->getThen());
    // 不判断是否有terminator会错，为什么?
    if (!curBlock->getTerminator()) {
        builder->CreateBr(afterIfBB);
    }
    sealBlock(curBlock);

    if(elseBB) {
        setCurrentBlock(elseBB);
        emitStmtHelper(S->getElse());
        // 不判断是否有terminator会错，为什么?
        if (!curBlock->getTerminator()) {
            builder->CreateBr(afterIfBB);
        }
        sealBlock(curBlock);
    }

    setCurrentBlock(afterIfBB);
}

void IRGenerator::emitWhileStmt(WhileStmt *S) {
    llvm::BasicBlock *whileCondBB;
    llvm::BasicBlock *whileBodyBB =
            createBasicBlock("while.body");
    llvm::BasicBlock *afterWhileBB =
            createBasicBlock("after.while");

    if(curBlock->empty() && currentBlockInfo[curBlock].defs.empty()) {
        curBlock->setName("while.cond");
        whileCondBB = curBlock;
    }
    else {
        whileCondBB = createBasicBlock("while.cond");
        builder->CreateBr(whileCondBB);
        sealBlock(curBlock);
        setCurrentBlock(whileCondBB);
    }

    llvm::Value *cond = emitExprHelper(S->getCond());
    builder->CreateCondBr(cond, whileBodyBB, afterWhileBB);

    setCurrentBlock(whileBodyBB);
    emitStmtHelper(S->getBody());
    builder->CreateBr(whileCondBB);

    sealBlock(whileCondBB);
    // 这里密封的不一定是whileBody
    // 因为在处理Body的时候curBlock可能已经转变？
    sealBlock(curBlock);

    setCurrentBlock(afterWhileBB);
}

void IRGenerator::emitDoWhileStmt(DoWhileStmt *S) {
    llvm::BasicBlock *whileCondBB =
            createBasicBlock("while.cond");
    llvm::BasicBlock *whileBodyBB;
    llvm::BasicBlock *afterWhileBB =
            createBasicBlock("after.while");

    if(curBlock->empty() && currentBlockInfo[curBlock].defs.empty()) {
        curBlock->setName("while.body");
        whileBodyBB = curBlock;
    }
    else {
        whileBodyBB = createBasicBlock("while.body");
        builder->CreateBr(whileBodyBB);
        sealBlock(curBlock);
        setCurrentBlock(whileBodyBB);
    }

    emitStmtHelper(S->getBody());
    builder->CreateBr(whileCondBB);

    setCurrentBlock(whileCondBB);
    llvm::Value *cond = emitExprHelper(S->getCond());
    builder->CreateCondBr(cond, whileBodyBB, afterWhileBB);

    sealBlock(whileBodyBB);
    sealBlock(curBlock);

    setCurrentBlock(afterWhileBB);
}

void IRGenerator::emitReturnStmt(ReturnStmt *S) {
    hasRet = true;
    if(S->hasRetValue()) {
        llvm::Value *retVal = emitExprHelper(S->getRetValue());
        builder->CreateRet(retVal);
    }
    else builder->CreateRetVoid();
}

// Expr的IR生成函数
llvm::Value* IRGenerator::emitBinaryOperator(BinaryOperator *E) {
    if(E->isAssignOp()) return emitAssignOperator(E);

    if(E->getQualType().getTypeKind() != Type::k_BuiltInType)
        llvm::report_fatal_error("In emitBinaryOperator: Unsupported type for operator");

    BuiltInType::TypeEnum typeEnum = dynamic_cast<BuiltInType*>(E->getLHS()->getQualType().getType())->getTypeType();

    llvm::Value *LHS = emitExprHelper(E->getLHS());
    llvm::Value *RHS = emitExprHelper(E->getRHS());
    llvm::Value *result = nullptr;

    //Fixme:
    // 未支持signed以外数据类型
    // 未支持lsh，rsh
    switch(E->getOp()) {
        case(BinaryOperator::_add):
            if(typeEnum == BuiltInType::_int)
                result = builder->CreateNSWAdd(LHS, RHS);
            break;
        case(BinaryOperator::_sub):
            if(typeEnum == BuiltInType::_int)
                result = builder->CreateNSWSub(LHS, RHS);
            break;
        case(BinaryOperator::_mul):
            if(typeEnum == BuiltInType::_int)
                result = builder->CreateNSWMul(LHS, RHS);
            break;
        case(BinaryOperator::_div):
            if(typeEnum == BuiltInType::_int)
                result = builder->CreateSDiv(LHS, RHS);
            break;
        case(BinaryOperator::_mod):
            if(typeEnum == BuiltInType::_int)
                result = builder->CreateSRem(LHS, RHS);
            break;
        case(BinaryOperator::_and):
            if(typeEnum == BuiltInType::_int)
                result = builder->CreateAnd(LHS, RHS);
            break;
        case(BinaryOperator::_or):
            if(typeEnum == BuiltInType::_int)
                result = builder->CreateOr(LHS, RHS);
            break;
        case(BinaryOperator::_xor):
            if(typeEnum == BuiltInType::_int)
                result = builder->CreateXor(LHS, RHS);
            break;
        case(BinaryOperator::_eq):
            if(typeEnum == BuiltInType::_int) {
                result = builder->CreateICmpEQ(LHS, RHS);
            }
            break;
        case(BinaryOperator::_neq):
            if(typeEnum == BuiltInType::_int)
                result = builder->CreateICmpNE(LHS, RHS);
            break;
        case(BinaryOperator::_lt):
            if(typeEnum == BuiltInType::_int)
                result = builder->CreateICmpSLT(LHS, RHS);
            break;
        case(BinaryOperator::_gt):
            if(typeEnum == BuiltInType::_int)
                result = builder->CreateICmpSGT(LHS, RHS);
            break;
        case(BinaryOperator::_leq):
            if(typeEnum == BuiltInType::_int)
                result = builder->CreateICmpSLE(LHS, RHS);
            break;
        case(BinaryOperator::_geq):
            if(typeEnum == BuiltInType::_int)
                result = builder->CreateICmpSGE(LHS, RHS);
            break;
        case(BinaryOperator::_log_and):
            if(typeEnum == BuiltInType::_bool)
                result = builder->CreateLogicalAnd(LHS, RHS);
            break;
        case(BinaryOperator::_log_or):
            if(typeEnum == BuiltInType::_bool)
                result = builder->CreateLogicalOr(LHS, RHS);
            break;
        case(BinaryOperator::_lsh):
            if(typeEnum == BuiltInType::_int)
                result = builder->CreateShl(LHS, RHS);
            break;
        case(BinaryOperator::_rsh):
            if(typeEnum == BuiltInType::_int)
                result = builder->CreateAShr(LHS, RHS);
            else if(typeEnum == BuiltInType::_unsigned)
                result = builder->CreateLShr(LHS, RHS);
            break;
    }
    if(result == nullptr)
        llvm::report_fatal_error("In emitBinaryOperator: Unsupported type for operator");
    return result;
}

llvm::Value* IRGenerator::emitUnaryOperator(UnaryOperator *E) {
    llvm::Value *result = emitExprHelper(E->getSubExpr());

    //Todo: 处理取地址

    // 处理算数符号
    if(E->getQualType().getTypeKind() != Type::k_BuiltInType)
        llvm::report_fatal_error("In emitUnaryOperator: Unsupported type for operator");

    BuiltInType::TypeEnum typeEnum = dynamic_cast<BuiltInType*>(E->getSubExpr()->getQualType().getType())->getTypeType();

    //Fixme:
    // 大量符号未实现
    switch(E->getOp()) {
        case(UnaryOperator::_unary_minus):
            if(typeEnum == BuiltInType::_int)
                result = builder->CreateNeg(result);
            break;
        case(UnaryOperator::_bit_not):
            if(typeEnum == BuiltInType::_int)
                result = builder->CreateNot(result);
            break;
    }

    if(result == nullptr)
        llvm::report_fatal_error("In emitUnaryOperator: Unsupported type for operator");
    return result;
}

llvm::Value* IRGenerator::emitSelectorArray(SelectorArray *E) {
    //Fixme: 未支持CallExpr作为subExpr
    if(E->getSubExpr()->getKind() != Stmt::k_DeclRefExpr)
        llvm::report_fatal_error("In emitSelectorArray: Unsupported select object");

    DeclRefExpr *subExpr = dynamic_cast<DeclRefExpr*>(E->getSubExpr());
    llvm::Value *value = readVariable(curBlock, subExpr);

    auto selectors = E->getSelectors();
    for(auto I = selectors.begin(), E = selectors.end();I != E;)
        if(auto exactI = dynamic_cast<DerefSelector*>(*I)) {
            value = builder->CreateLoad(value->getType()->getPointerElementType(), value);
            ++I;
        }
        else if(auto exactI = dynamic_cast<IndexSelector*>(*I)) {
            llvm::SmallVector<llvm::Value*, 4> idxList;
            idxList.push_back(llvm::ConstantInt::get(Int32Ty, 0));
            while(I != E) {
                if(auto sel = dynamic_cast<IndexSelector*>(*I)) {
                    idxList.push_back(emitExprHelper(sel->getIdxExpr()));
                    ++I;
                }
                else break;
            }
            value = builder->CreateInBoundsGEP(value->getType()->getPointerElementType(), value, idxList);
            value = builder->CreateLoad(value->getType()->getPointerElementType(), value);
        }
        else if(auto exactI = dynamic_cast<FieldSelector*>(*I)) {
            llvm::SmallVector<llvm::Value*, 4> idxList;
            idxList.push_back(llvm::ConstantInt::get(Int32Ty, 0));
            while(I != E) {
                if(auto sel = dynamic_cast<FieldSelector*>(*I)) {
                    idxList.push_back(llvm::ConstantInt::get(Int32Ty ,sel->getIdx()));
                    ++I;
                }
                else break;
            }
            //Fixme: 未检查，大概率有错
            value = builder->CreateInBoundsGEP(value->getType()->getPointerElementType(), value, idxList);
            value = builder->CreateLoad(value->getType()->getPointerElementType(), value);
        }
        else llvm::report_fatal_error("In emitSelectorArray: Unexpected error in emitSelectorArray");

    return value;
}

llvm::Value* IRGenerator::emitAssignOperator(BinaryOperator *E) {
    assert(E->isAssignOp());
    // 若为+=等op，需先计算其值
    //Fixme:
    // 未支持lsh，rsh
    BinaryOperator *modifiedE = new BinaryOperator(*E);
    llvm::Value *assignSource;
    switch(E->getOp()) {
        case(BinaryOperator::_assign):
            assignSource = emitExprHelper(E->getRHS());
            break;
        case(BinaryOperator::_add_assign):
            modifiedE->setOp(BinaryOperator::_add);
            assignSource = emitExprHelper(modifiedE);
            break;
        case(BinaryOperator::_sub_assign):
            modifiedE->setOp(BinaryOperator::_sub);
            assignSource = emitExprHelper(modifiedE);
            break;
        case(BinaryOperator::_mul_assign):
            modifiedE->setOp(BinaryOperator::_mul);
            assignSource = emitExprHelper(modifiedE);
            break;
        case(BinaryOperator::_div_assign):
            modifiedE->setOp(BinaryOperator::_div);
            assignSource = emitExprHelper(modifiedE);
            break;
        case(BinaryOperator::_and_assign):
            modifiedE->setOp(BinaryOperator::_and);
            assignSource = emitExprHelper(modifiedE);
            break;
        case(BinaryOperator::_or_assign):
            modifiedE->setOp(BinaryOperator::_or);
            assignSource = emitExprHelper(modifiedE);
            break;
        case(BinaryOperator::_xor_assign):
            modifiedE->setOp(BinaryOperator::_xor);
            assignSource = emitExprHelper(modifiedE);
            break;
        default:
            llvm::report_fatal_error("In emitAssignOperator: Unsupported type for operator");
    }
    delete modifiedE;

    if(E->getLHS()->getKind() ==  Stmt::k_DeclRefExpr) {
        writeVariable(curBlock,
                      dynamic_cast<DeclRefExpr*>(E->getLHS())->getRefName(),
                      assignSource);
    }

    // 处理数组，解引用，结构体
    if(E->getLHS()->getKind() ==  Stmt::k_SelectorArray) {
        SelectorArray *selectorArray = dynamic_cast<SelectorArray*>(E->getLHS());
        llvm::SmallVector<llvm::Value*, 4> idxList;
        idxList.push_back(llvm::ConstantInt::get(Int32Ty, 0));
        llvm::Value *base;
        if(auto subExpr = dynamic_cast<DeclRefExpr*>(selectorArray->getSubExpr()))
            base = readVariable(curBlock, subExpr);
        else
            llvm::report_fatal_error("In emitAssignOperator(SelectorArray): target is not a lvalue");

        int numS = selectorArray->getNumSelectors(), cnt = 0;
        for(auto selector : selectorArray->selectors) {
            cnt ++;
            if(auto idxSelector = dynamic_cast<IndexSelector*>(selector)) {
                idxList.push_back(emitExprHelper(idxSelector->getIdxExpr()));
            }
            else if(auto fieldSelector = dynamic_cast<FieldSelector*>(selector)) {
                idxList.push_back(llvm::ConstantInt::get(Int32Ty, fieldSelector->getIdx()));
            }
            else if(auto derefSelector = dynamic_cast<DerefSelector*>(selector)) {
                if(idxList.size() > 1) {
                    base = builder->CreateInBoundsGEP(base->getType()->getPointerElementType(), base, idxList);
                    base = builder->CreateLoad(base->getType()->getPointerElementType(), base);
                    idxList.resize(1);
                }
            }
            else {
                llvm::report_fatal_error("In emitAssignOperator(SelectorArray): invalid selector type");
            }
        }

        if(idxList.size() > 1) {
            base = builder->CreateInBoundsGEP(base->getType()->getPointerElementType(), base, idxList);
        }

        if (base->getType()->isPointerTy())
            builder->CreateStore(assignSource, base);
        else
            llvm::report_fatal_error("In emitAssignOperator: not a pointer type");
    }

    return assignSource;
}

llvm::Value* IRGenerator::emitDeclRefExpr(DeclRefExpr *E) {
    return readVariable(curBlock, E);
}

llvm::Value* IRGenerator::emitIntegerLiteral(IntegerLiteral *E) {
    return llvm::ConstantInt::get(Int32Ty, E->getValue());
}

llvm::Value* IRGenerator::emitFloatingLiteral(FloatingLiteral *E) {
    return llvm::ConstantFP::get(FloatTy, E->getValue());
}

llvm::Value* IRGenerator::emitCallExpr(CallExpr *E) {
    DeclRefExpr *funcCallRef;
    if(ImplicitCastExpr *implicitCastExpr = dynamic_cast<ImplicitCastExpr*>(E->getArg(0)))
        funcCallRef = dynamic_cast<DeclRefExpr*>(implicitCastExpr->getCastedExpr());
    else funcCallRef = dynamic_cast<DeclRefExpr*>(E->getArg(0));

    if(!funcCallRef)
        llvm::report_fatal_error("In emitCallExpr: invalid function call");
    llvm::Function *callee = module->getFunction(funcCallRef->getRefName());
    if(!callee)
        llvm::report_fatal_error("In emitCallExpr: cannot find function");

    int numArgs = callee->arg_size();
    if(numArgs + 1 != E->getNumArgs())
        llvm::report_fatal_error("In emitCallExpr: prototype mismatch");
    llvm::SmallVector<llvm::Value*, 5> args;
    for(int i = 0;i < numArgs;++i)
        args.push_back(emitExprHelper(E->getArg(i + 1)));
    return builder->CreateCall(callee, args);
}

llvm::Value* IRGenerator::emitCastExpr(CastExpr *E) {
    Type *castType = E->getCastType().getType();
    llvm::Value *oldValue = emitExprHelper(E->getCastedExpr());

    if(auto exactCastType = dynamic_cast<PointerType*>(castType)) {
        if(oldValue->getType() == Int32Ty || oldValue->getType() == Int16Ty)
            return builder->CreateIntToPtr(oldValue, Int32PtrTy);
    }

    if(auto exactCastType = dynamic_cast<BuiltInType*>(castType)) {
        BuiltInType::TypeEnum type = exactCastType->getTypeType();
        switch (type) {
            case (BuiltInType::_int):
                if (oldValue->getType() == Int1Ty)
                    return builder->CreateZExt(oldValue, Int32Ty);
                else if (oldValue->getType() == Int32Ty)
                    return oldValue;
                break;
            case (BuiltInType::_bool):
                if (oldValue->getType() == Int32Ty)
                    return builder->CreateICmpNE(oldValue, llvm::ConstantInt::get(Int32Ty, 0));
                else if (oldValue->getType() == Int8Ty)
                    return builder->CreateICmpNE(oldValue, llvm::ConstantInt::get(Int8Ty, 0));
                else if (oldValue->getType() == Int16Ty)
                    return builder->CreateICmpNE(oldValue, llvm::ConstantInt::get(Int16Ty, 0));
                else if (oldValue->getType() == Int1Ty)
                    return oldValue;
                break;
        }
    }

    //Fixme:
    // 还没处理的类型转换
    std::cout << "In emitCastExpr: cast is not executed\n";
    return oldValue;
}

// 对多种可能性语句的IR释放helper函数
void IRGenerator::emitStmtHelper(Stmt *S) {
    switch(S->getKind()) {
        case(Stmt::k_CompoundStmt):
            emitCompoundStmt(dynamic_cast<CompoundStmt*>(S));
            break;
        // 这里仅处理局部变量声明
        case(Stmt::k_DeclStmt):
            emitLocalDecl(dynamic_cast<DeclStmt*>(S));
            break;
        case(Stmt::k_ReturnStmt):
            emitReturnStmt(dynamic_cast<ReturnStmt*>(S));
            break;
        case(Stmt::k_ForStmt):
            emitForStmt(dynamic_cast<ForStmt*>(S));
            break;
        case(Stmt::k_WhileStmt):
            emitWhileStmt(dynamic_cast<WhileStmt*>(S));
            break;
        case(Stmt::k_DoWhileStmt):
            emitDoWhileStmt(dynamic_cast<DoWhileStmt*>(S));
            break;
        case(Stmt::k_IfStmt):
            emitIfStmt(dynamic_cast<IfStmt*>(S));
            break;
        default:
            emitExprHelper(dynamic_cast<Expr*>(S));
    }
}

llvm::Value* IRGenerator::emitExprHelper( Expr *E ) {
    switch(E->getKind()) {
        case(Stmt::k_BinaryOperator):
            return emitBinaryOperator(dynamic_cast<BinaryOperator*>(E));
        case(Stmt::k_UnaryOperator):
            return emitUnaryOperator(dynamic_cast<UnaryOperator*>(E));
        case(Stmt::k_SelectorArray):
            return emitSelectorArray(dynamic_cast<SelectorArray*>(E));
        case(Stmt::k_DeclRefExpr):
            return emitDeclRefExpr(dynamic_cast<DeclRefExpr*>(E));
        case(Stmt::k_IntegerLiteral):
            return emitIntegerLiteral(dynamic_cast<IntegerLiteral*>(E));
        case(Stmt::k_FloatingLiteral):
            return emitFloatingLiteral(dynamic_cast<FloatingLiteral*>(E));
//        case(Stmt::k_StringLiteral):
//            traverseStringLiteral(dynamic_cast<StringLiteral*>(E));
//            break;
        case(Stmt::k_ImplicitCastExpr):
        case(Stmt::k_ExplicitCastExpr):
            return emitCastExpr(dynamic_cast<CastExpr*>(E));
        case(Stmt::k_CallExpr):
            return emitCallExpr(dynamic_cast<CallExpr*>(E));
        default:
            llvm_unreachable("In emitExprHelper: Unsupported Statement/Expression");
    }
}