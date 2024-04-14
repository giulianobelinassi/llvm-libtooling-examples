#include <clang/Basic/Version.h> // For CLANG_VERSION_MAJOR
#include "clang/Frontend/ASTUnit.h"
#include "clang/Frontend/CompilerInstance.h"

using namespace clang;
using namespace llvm;

/** Stringfy constant integer token in `s`.  */
#define STRINGFY_VALUE(s) STRINGFY(s)

/** Stringfy given expression `s`.  */
#define STRINGFY(s) #s

static void Print_Macros(ASTUnit *ast);
static void Print_Macro(const Preprocessor &pp, const IdentifierInfo *id, const MacroInfo *mi);

int main(int argc, char **argv)
{
  if (argc != 2) {
    llvm::outs() << "Usage: " << argv[0] << " <file-to-compile>\n";
    return 1;
  }

  const std::vector<const char *> clang_args =  {
    // For populating the PreprocessingRecord.
    "-v",
    "-Xclang", "-detailed-preprocessing-record",
    // For some reason libtooling do not pass the clang include folder.  Pass this then.
    "-I/usr/lib64/clang/" STRINGFY_VALUE(CLANG_VERSION_MAJOR) "/include",
    // Compile the file provided in argv[1].
    "-c", argv[1],
  };

  // Boilerplate code.
  DiagnosticOptions *diagopts = new DiagnosticOptions();
  IntrusiveRefCntPtr<DiagnosticsEngine> Diags = CompilerInstance::createDiagnostics(diagopts);

  /* Create the object that holds the command line options that will be passed
     to the compiler.  */
  clang::CreateInvocationOptions CIOpts;
  CIOpts.Diags = Diags;
  std::shared_ptr<CompilerInvocation> CInvok = clang::createInvocation(clang_args, std::move(CIOpts));

  /* Create the file manager, specifying the filesystem to be used as the real
     one (i.e. the disk itself).  */
  FileManager *FileMgr = new FileManager(FileSystemOptions(), vfs::getRealFileSystem());
  std::shared_ptr<PCHContainerOperations> PCHContainerOps = std::make_shared<PCHContainerOperations>();

  /* Finally, create the ASTUnit object.  */
  std::unique_ptr<ASTUnit> AST = ASTUnit::LoadFromCompilerInvocation(
      CInvok, PCHContainerOps, Diags, FileMgr, false, CaptureDiagsKind::None, 1,
      TU_Complete, false, false, false);

  /* Check if the compilation was successful.  */
  if (AST == nullptr || AST->getDiagnostics().hasErrorOccurred()) {
    llvm::outs() << "Error building the ASTUnit.\n";
    return 1;
  }

  /* Print function names with location.  */
  Print_Macros(AST.get());

  return 0;
}

/* Print the defined macros on the ASTUnit.  */
static void Print_Macros(ASTUnit *ast)
{
  Preprocessor &pp = ast->getPreprocessor();
  PreprocessingRecord *rec = pp.getPreprocessingRecord();

  /* For the PreprocessingRecord to be populated clang must be called with
     -Xclang -detailed-preprocessing-record.  Otherwise rec would be null.  */
  assert(rec && "PreprocessingRecord was not created");

  /* Iterate on the record.  */
  for (auto it= rec->begin(); it != rec->end(); ++it) {
    PreprocessedEntity *entity = *it;

    /* The PreprocessedEntity can be a MacroDefinitionRecord (i.e. a definition of a macro),
       a InclusionDirective (i.e. an #include)  or a MacroExpansion.  Notice that
       #if, or #undef are not modeled here.  For #undef you can figure its location
       my looking into the DefInfo from MacroDefintion object.  */
    if (MacroDefinitionRecord *macrodef = dyn_cast<MacroDefinitionRecord>(entity)) {
      MacroDefinition md = pp.getMacroDefinitionAtLoc(macrodef->getName(), macrodef->getSourceRange().getEnd());
      if (MacroInfo *mi = md.getMacroInfo()) {
        Print_Macro(pp, macrodef->getName(), mi);
      }
    }
  }
}

/* Function to print a macro my parsing the tokens.  Usually it is enough to
   just copy and paste the user code from the source code.  */
static void Print_Macro(const Preprocessor &pp, const IdentifierInfo *id, const MacroInfo *mi)
{
  /* Macro definition.  */
  llvm::outs() << "#define " << id->getName();
  if (mi->isFunctionLike()) {
    /* It is a macro with parameters (macro(a,b)).  We have to be careful with
       that.  */
    ArrayRef<const IdentifierInfo *> params = mi->params();
    unsigned n = mi->getNumParams();

    llvm::outs() << '(';
    for (unsigned i = 0; i < mi->getNumParams(); i++) {
      llvm::outs() << params[i]->getName();
      if (i != n-1) {
        llvm::outs() << ", ";
      }
    }
    llvm::outs() << ')';
  }
  llvm::outs() << ' ';

  /* Now the right side of the macro definition.  */
  for (const Token &tok : mi->tokens()) {
    llvm::outs() << pp.getSpelling(tok);
  }
  llvm::outs() << '\n';
}
