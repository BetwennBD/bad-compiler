//
// Created by tangny on 2021/12/10.
//

#ifndef DECL
#define DECL(TYPE, BASE)
#endif

#ifndef TRANSLATIONUNITDECL
#define TRANSLATIONUNITDECL(TYPE, BASE) DECL(TYPE, BASE)
#endif
TRANSLATIONUNITDECL(TranslationUnitDecl, Decl)
#undef TRANSLATIONUNITDECL

#ifndef NAMEDDECL
#define NAMEDDECL(TYPE, BASE) DECL(TYPE, BASE)
#endif
NAMEDDECL(NamedDecl, Decl)
#undef NAMEDDECL

#ifndef VALUEDECL
#define VALUEDECL(TYPE, BASE) DECL(TYPE, BASE)
#endif
VALUEDECL(ValueDecl, NamedDecl)
#undef VALUEDECL

#ifndef DECLARATORDECL
#define DECLARATORDECL(TYPE, BASE) DECL(TYPE, BASE)
#endif
DECLARATORDECL(DeclaratorDecl, ValueDecl)
#undef DECLARATORDECL

#ifndef FUNCTIONDECL
#define FUNCTIONDECL(TYPE, BASE) DECL(TYPE, BASE)
#endif
FUNCTIONDECL(FunctionDecl, DeclaratorDecl)
#undef FUNCTIONDECL

#ifndef VARDECL
#define VARDECL(TYPE, BASE) DECL(TYPE, BASE)
#endif
VARDECL(VarDecl, DeclaratorDecl)
#undef VARDECL

#ifndef PARAMVARDECL
#define PARAMVARDECL(TYPE, BASE) DECL(TYPE, BASE)
#endif
PARAMVARDECL(ParamVarDecl, VarDecl)
#undef PARAMVARDECL

#undef DECL