//#include <iostream>
//#include <vector>
//
//#include "include/Yacc/Grammar.h"
//#include "include/Yacc/GrammarParser.h"
//#include "include/Yacc/Constructor.h"
//#include "include/Parser/CST.h"
//#include "include/Parser/CSTBuilder.h"
//#include "include/AST/ASTBuilder.h"
//#include "include/ASTConsumers/ASTDumper.h"
//#include "include/ASTConsumers/FillType.h"
//#include "include/ASTConsumers/FillReference.h"
//#include "include/ASTConsumers/CalculateConstant.h"
//#include "include/ASTConsumers/ASTTypeCheck.h"
//#include "include/Lexer/Lexer.h"
//#include "include/ASTConsumers/IRGenerator.h"
//
//#include "llvm/CodeGen/CommandFlags.h"
//#include "llvm/IR/IRPrintingPasses.h"
//#include "llvm/IR/LegacyPassManager.h"
//#include "llvm/Support/CommandLine.h"
//#include "llvm/Support/InitLLVM.h"
//#include "llvm/Support/Host.h"
//#include "llvm/Support/MemoryBuffer.h"
//#include "llvm/MC/TargetRegistry.h"
//#include "llvm/Support/TargetSelect.h"
//#include "llvm/Support/ToolOutputFile.h"
//#include "llvm/Support/WithColor.h"
//#include "llvm/Support/FileSystem.h"
//#include "llvm/Target/TargetMachine.h"
//#include "llvm/MC/TargetRegistry.h"
//#include "llvm/Target/TargetOptions.h"
//#include "llvm/Support/SourceMgr.h"
//
///*static llvm::codegen::RegisterCodeGenFlags CGF;
//
//static llvm::cl::list<std::string>
//        InputFiles(llvm::cl::Positional,
//                   llvm::cl::desc("<input-files>"));
//
//static llvm::cl::opt<std::string>
//        MTriple("mtriple",
//                llvm::cl::desc("Override target triple for module"));
//
//static llvm::cl::opt<bool>
//        EmitLLVM("emit-llvm",
//                 llvm::cl::desc("Emit IR code instead of assembler"),
//                 llvm::cl::init(false));
//
//static const char *Head = "minisys - Minisys compiler";
//
//void printVersion(llvm::raw_ostream &OS) {
//    OS << "  Default target: "
//       << llvm::sys::getDefaultTargetTriple() << "\n";
//    std::string CPU(llvm::sys::getHostCPUName());
//    OS << "  Host CPU: " << CPU << "\n";
//    OS << "\n";
//    OS.flush();
//    llvm::TargetRegistry::printRegisteredTargetsForVersion(
//            OS);
//    exit(EXIT_SUCCESS);
//}
//
//llvm::TargetMachine *
//createTargetMachine(const char *Argv0) {
//    llvm::Triple Triple = llvm::Triple(
//            !MTriple.empty()
//            ? llvm::Triple::normalize(MTriple)
//            : llvm::sys::getDefaultTargetTriple());
//
//    llvm::TargetOptions TargetOptions =
//            llvm::codegen::InitTargetOptionsFromCodeGenFlags(Triple);
//    std::string CPUStr = llvm::codegen::getCPUStr();
//    std::string FeatureStr = llvm::codegen::getFeaturesStr();
//
//    std::string Error;
//    const llvm::Target *Target =
//            llvm::TargetRegistry::lookupTarget(llvm::codegen::getMArch(), Triple,
//                                               Error);
//
//    if (!Target) {
//        llvm::WithColor::error(llvm::errs(), Argv0) << Error;
//        return nullptr;
//    }
//
//    llvm::TargetMachine *TM = Target->createTargetMachine(
//            Triple.getTriple(), CPUStr, FeatureStr, TargetOptions,
//            llvm::Optional<llvm::Reloc::Model>(llvm::codegen::getRelocModel()));
//    return TM;
//}
//
//bool emit(llvm::StringRef Argv0, llvm::Module *M,
//          llvm::TargetMachine *TM,
//          llvm::StringRef InputFilename) {
//    llvm::CodeGenFileType FileType = llvm::codegen::getFileType();
//    std::string OutputFilename;
//    if (InputFilename == "-") {
//        OutputFilename = "-";
//    } else {
//        if (InputFilename.endswith(".c"))
//            OutputFilename = InputFilename.drop_back(2).str();
//        else if(InputFilename.endswith(".txt"))
//            OutputFilename = InputFilename.drop_back(4).str();
//        else
//            OutputFilename = InputFilename.str();
//        switch (FileType) {
//            case llvm::CGFT_AssemblyFile:
//                OutputFilename.append(EmitLLVM ? ".ll" : ".s");
//                break;
//            case llvm::CGFT_ObjectFile:
//                OutputFilename.append(".o");
//                break;
//            case llvm::CGFT_Null:
//                OutputFilename.append(".null");
//                break;
//        }
//    }
//
//    // Open the file.
//    std::error_code EC;
//    llvm::sys::fs::OpenFlags OpenFlags = llvm::sys::fs::OF_None;
//    if (FileType == llvm::CGFT_AssemblyFile)
//        OpenFlags |= llvm::sys::fs::OF_Text;
//    auto Out = std::make_unique<llvm::ToolOutputFile>(
//            OutputFilename, EC, OpenFlags);
//    if (EC) {
//        llvm::WithColor::error(llvm::errs(), Argv0) << EC.message() << '\n';
//        return false;
//    }
//
//    llvm::legacy::PassManager PM;
//    if (FileType == llvm::CGFT_AssemblyFile && EmitLLVM) {
//        PM.add(createPrintModulePass(Out->os()));
//    } else {
//        if (TM->addPassesToEmitFile(PM, Out->os(), nullptr,
//                                    FileType)) {
//            llvm::WithColor::error() << "No support for file type\n";
//            return false;
//        }
//    }
//    PM.run(*M);
//    Out->keep();
//    return true;
//}*/
//
//int main(int Argc, const char **Argv) {
//    llvm::InitLLVM X(Argc, Argv);
//
//    llvm::InitializeAllTargets();
//    llvm::InitializeAllTargetMCs();
//    llvm::InitializeAllAsmPrinters();
//    llvm::InitializeAllAsmParsers();
////    llvm::InitializeNativeTarget();
////    llvm::InitializeNativeTargetAsmPrinter();
////    llvm::InitializeNativeTargetAsmParser();
//
//
//
////    if (llvm::codegen::getMCPU() == "help" ||
////        std::any_of(llvm::codegen::getMAttrs().begin(), llvm::codegen::getMAttrs().end(),
////                    [](const std::string &a) {
////                        return a == "help";
////                    })) {
////        auto Triple = llvm::Triple(LLVM_DEFAULT_TARGET_TRIPLE);
////        std::string ErrMsg;
////        if (auto target = llvm::TargetRegistry::lookupTarget(
////                Triple.getTriple(), ErrMsg)) {
////            llvm::errs() << "Targeting " << target->getName()
////                         << ". ";
////            // this prints the available CPUs and features of the
////            // target to stderr...
////            target->createMCSubtargetInfo(Triple.getTriple(),
////                                          llvm::codegen::getCPUStr(),
////                                          llvm::codegen::getFeaturesStr());
////        } else {
////            llvm::errs() << ErrMsg << "\n";
////            exit(EXIT_FAILURE);
////        }
////        exit(EXIT_SUCCESS);
////    }
//
//    llvm::TargetMachine *TM = createTargetMachine(Argv[0]);
//    if (!TM)
//        exit(EXIT_FAILURE);
//
//    for (const auto &F : InputFiles) {
//        llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>>
//                FileOrErr = llvm::MemoryBuffer::getFile(F);
//        if (std::error_code BufferError =
//                FileOrErr.getError()) {
//            llvm::WithColor::error(llvm::errs(), Argv[0])
//                    << "Error reading " << F << ": "
//                    << BufferError.message() << "\n";
//        }
//
//        llvm::SourceMgr SrcMgr;
//
//        // Tell SrcMgr about this buffer, which is what the
//        // parser will pick up.
//        SrcMgr.AddNewSourceBuffer(std::move(*FileOrErr),
//                                  llvm::SMLoc());
//
//        Lexer lexer("../etc/source.txt");
//        std::vector<LexUnit> lexVec = lexer.getAnalysis();
//
//        GrammarSet grammarSet;
//        parseProducer(grammarSet, "../etc/yacc_c99.y");
//        LALRconstructor lalrConstructor(&grammarSet, 89, 68, 400);
//
//        CSTBuilder cstBuilder(&lalrConstructor, &grammarSet);
//        CSTNode *cst = cstBuilder.constructCST(lexVec, 0);
//        if(cst == NULL) {
//            std::cout << "create CST failed";
//            return 0;
//        }
//
//        ASTBuilder astBuilder;
//        TranslationUnitDecl* ast = astBuilder.constructAST(cst);
//        if(ast == NULL) {
//            std::cout << "create CST failed";
//            return 0;
//        }
//
//        FillReference fillReference;
//        fillReference.traverseTranslationUnitDecl(ast);
//        FillType fillType;
//        fillType.traverseTranslationUnitDecl(ast);
//        CalculateConstant calculateConstant;
//        calculateConstant.traverseTranslationUnitDecl(ast);
//        ASTTypeCheck astTypeCheck;
//        astTypeCheck.traverseTranslationUnitDecl(ast);
//
//        ASTDumper astDumper;
//        astDumper.traverseTranslationUnitDecl(ast);
//
//        deleteCST(cst);
//    }
//}