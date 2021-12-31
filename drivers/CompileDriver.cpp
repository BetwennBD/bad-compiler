#include <iostream>
#include <vector>
#include <fstream>
#include <cstring>

#include "include/Yacc/Grammar.h"
#include "include/Yacc/GrammarParser.h"
#include "include/Yacc/Constructor.h"
#include "include/Parser/CST.h"
#include "include/Parser/CSTBuilder.h"
#include "include/AST/ASTBuilder.h"
#include "include/ASTConsumers/ASTDumper.h"
#include "include/ASTConsumers/FillType.h"
#include "include/ASTConsumers/FillReference.h"
#include "include/ASTConsumers/CalculateConstant.h"
#include "include/ASTConsumers/ASTTypeCheck.h"
#include "include/ASTConsumers/AddVarImplicit.h"
#include "include/Lexer/Lexer.h"
#include "include/IRGenerator/IRGenerator.h"

#include "llvm/CodeGen/CommandFlags.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Support/SourceMgr.h"

int main(int Argc, const char **Argv) {
    bool print_ast = false;
    bool print_llvm_ir = false;

    std::vector<std::string> iFileNames;
    std::vector<std::string> oFileNames;


    for(int i = 1;i < Argc;++i) {
        if(strcmp(Argv[i], "--help") == 0) {
            return 0;
        }
        else if(strcmp(Argv[i], "--print-ast") == 0)
            print_ast = true;
        else if(strcmp(Argv[i], "--print-llvm-ir") == 0)
            print_llvm_ir = true;
        else {
            std::string iFileName = std::string(Argv[i]);
            iFileNames.emplace_back(iFileName);
            std::string oFileName = iFileName;
            int pos = oFileName.find(".txt");
            if(pos == std::string::npos) {
                llvm::errs() << "Illegal File Type";
                return 0;
            }
            oFileName.replace(pos, 4, ".asm");
            oFileNames.emplace_back(oFileName);
        }
    }

    llvm::InitLLVM X(Argc, Argv);

    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmPrinters();
    llvm::InitializeAllAsmParsers();

//    llvm::InitializeNativeTarget();
//    llvm::InitializeNativeTargetAsmPrinter();
//    llvm::InitializeNativeTargetAsmParser();

    GrammarSet grammarSet;
    parseProducer(grammarSet, "../etc/yacc_c99.y");
    LALRconstructor lalrConstructor(&grammarSet, 89, 68, 400);

//    auto targetTriple = llvm::sys::getDefaultTargetTriple();
    auto targetTriple = "mips-unknown-linux-gnu";

    std::string Error;
    auto Target = llvm::TargetRegistry::lookupTarget(targetTriple, Error);

    if (!Target) {
        llvm::errs() << Error;
        return 1;
    }

    auto CPU = "generic";
    auto Features = "";

    llvm::TargetOptions opt;
    auto RM = llvm::Optional<llvm::Reloc::Model>();
    auto targetMachine =
            Target->createTargetMachine(targetTriple, CPU, Features, opt, RM);

    for (int i = 0;i < iFileNames.size();++i) {
        std::ifstream testOpenFile(iFileNames[i]);
        if(!testOpenFile) {
            llvm::errs() << "Could not find input file: " << iFileNames[i] << "\n";
            return 1;
        }
        testOpenFile.close();

        Lexer lexer;
        std::vector<LexUnit> lexVec = lexer.getAnalysis(iFileNames[i]);

        CSTBuilder cstBuilder(&lalrConstructor, &grammarSet);
        CSTNode *cst = cstBuilder.constructCST(lexVec, 0);
        if(cst == NULL) {
            llvm::errs() << "create CST failed\n";
            return 1;
        }

        ASTBuilder astBuilder;
        TranslationUnitDecl* ast = astBuilder.constructAST(cst);
        if(ast == NULL) {
            llvm::errs() << "create CST failed\n";
            return 1;
        }

        FillReference fillReference;
        fillReference.traverseTranslationUnitDecl(ast);
        FillType fillType;
        fillType.traverseTranslationUnitDecl(ast);
        CalculateConstant calculateConstant;
        calculateConstant.traverseTranslationUnitDecl(ast);
        ASTTypeCheck astTypeCheck;
        astTypeCheck.traverseTranslationUnitDecl(ast);
        AddVarImplicit addVarImplicit;
        addVarImplicit.traverseTranslationUnitDecl(ast);

        if(print_ast) {
            ASTDumper astDumper;
            astDumper.traverseTranslationUnitDecl(ast);
        }

        auto oFilename = oFileNames[i];
        std::error_code EC;
        llvm::raw_fd_ostream dest(oFilename, EC, llvm::sys::fs::OF_None);

        if (EC) {
            llvm::errs() << "Could not open file: " << EC.message();
            return 1;
        }

        llvm::legacy::PassManager pass;
        auto FileType = llvm::CGFT_AssemblyFile;

        if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
            llvm::errs() << "TheTargetMachine can't emit a file of this type\n";
            return 1;
        }

        IRGenerator irGenerator(targetTriple, targetMachine->createDataLayout(), iFileNames[i]);
        irGenerator.emitLLVMIR(ast, print_llvm_ir);

        pass.run(*irGenerator.getModule());
        dest.flush();

        deleteCST(cst);
    }
}