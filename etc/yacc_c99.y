%token IDENTIFIER CONSTANT STRING_LITERAL SIZEOF
%token PTR_OP INC_OP DEC_OP LEFT_OP RIGHT_OP LE_OP GE_OP EQ_OP NE_OP
%token AND_OP OR_OP MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN
%token SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN
%token XOR_ASSIGN OR_ASSIGN TYPE_NAME

%token TYPEDEF EXTERN STATIC AUTO REGISTER INLINE RESTRICT
%token CHAR SHORT INT LONG SIGNED UNSIGNED FLOAT DOUBLE CONST VOLATILE VOID
%token BOOL COMPLEX IMAGINARY
%token STRUCT UNION ENUM ELLIPSIS

%token CASE DEFAULT IF ELSE SWITCH WHILE DO FOR GOTO CONTINUE BREAK RETURN

%start translation_unit
%%

primary_expression
	: IDENTIFIER                                                                    ## Ref
	| CONSTANT                                                                      ## NumLiteral
	| STRING_LITERAL                                                                ## StrLiteral
	| '(' expression ')'
	;

postfix_expression
	: primary_expression
	| postfix_expression '[' expression ']'                                         ## Selector
	| postfix_expression '(' ')'                                                    ## FuncCall
	| postfix_expression '(' argument_expression_list ')'                           ## FuncCall
	| postfix_expression '.' IDENTIFIER                                             ## Selector
	| postfix_expression PTR_OP IDENTIFIER                                          ## Selector
	| postfix_expression INC_OP                                                     ## PostOp
	| postfix_expression DEC_OP                                                     ## PostOp
	| '(' type_name ')' '{' initializer_list '}'
	| '(' type_name ')' '{' initializer_list ',' '}'
	;

argument_expression_list
	: assignment_expression
	| argument_expression_list ',' assignment_expression
	;

unary_expression
	: postfix_expression
	| INC_OP unary_expression                                                       ## PreOp
	| DEC_OP unary_expression                                                       ## PreOp
	| unary_operator cast_expression                                                ## PreOp
	| SIZEOF unary_expression
	| SIZEOF '(' type_name ')'
	;

unary_operator
	: '&'
	| '*'
	| '+'
	| '-'
	| '~'
	| '!'
	;

cast_expression
	: unary_expression
	| '(' type_name ')' cast_expression                                             ## CstExpr
	;

multiplicative_expression
	: cast_expression
	| multiplicative_expression '*' cast_expression                                 ## BinOp
	| multiplicative_expression '/' cast_expression                                 ## BinOp
	| multiplicative_expression '%' cast_expression                                 ## BinOp
	;

additive_expression
	: multiplicative_expression
	| additive_expression '+' multiplicative_expression                             ## BinOp
	| additive_expression '-' multiplicative_expression                             ## BinOp
	;

shift_expression
	: additive_expression
	| shift_expression LEFT_OP additive_expression                                  ## BinOp
	| shift_expression RIGHT_OP additive_expression                                 ## BinOp
	;

relational_expression
	: shift_expression
	| relational_expression '<' shift_expression                                    ## BinOp
	| relational_expression '>' shift_expression                                    ## BinOp
	| relational_expression LE_OP shift_expression                                  ## BinOp
	| relational_expression GE_OP shift_expression                                  ## BinOp
	;

equality_expression
	: relational_expression
	| equality_expression EQ_OP relational_expression                               ## BinOp
	| equality_expression NE_OP relational_expression                               ## BinOp
	;

and_expression
	: equality_expression
	| and_expression '&' equality_expression                                        ## BinOp
	;

exclusive_or_expression
	: and_expression
	| exclusive_or_expression '^' and_expression                                    ## BinOp
	;

inclusive_or_expression
	: exclusive_or_expression
	| inclusive_or_expression '|' exclusive_or_expression                           ## BinOp
	;

logical_and_expression
	: inclusive_or_expression
	| logical_and_expression AND_OP inclusive_or_expression                         ## BinOp
	;

logical_or_expression
	: logical_and_expression
	| logical_or_expression OR_OP logical_and_expression                            ## BinOp
	;

conditional_expression
	: logical_or_expression
	| logical_or_expression '?' expression ':' conditional_expression
	;

assignment_expression
	: conditional_expression
	| unary_expression assignment_operator assignment_expression                    ## BinOp
	;

assignment_operator
	: '='
	| MUL_ASSIGN
	| DIV_ASSIGN
	| MOD_ASSIGN
	| ADD_ASSIGN
	| SUB_ASSIGN
	| LEFT_ASSIGN
	| RIGHT_ASSIGN
	| AND_ASSIGN
	| XOR_ASSIGN
	| OR_ASSIGN
	;

expression
	: assignment_expression
	| expression ',' assignment_expression
	;

constant_expression
	: conditional_expression
	;

declaration
	: declaration_specifiers ';'                                                    ## Declaration
	| declaration_specifiers init_declarator_list ';'                               ## Declaration
	;

declaration_specifiers
	: storage_class_specifier
	| storage_class_specifier declaration_specifiers
	| type_specifier                                                                ## TypeSp
	| type_specifier declaration_specifiers                                         ## TypeSp
	| type_qualifier                                                                ## TypeQf
	| type_qualifier declaration_specifiers                                         ## TypeQf
	| function_specifier
	| function_specifier declaration_specifiers
	;

init_declarator_list
	: init_declarator
	| init_declarator_list ',' init_declarator
	;

init_declarator
	: declarator                                                                    ## Declarator
	| declarator '=' initializer                                                    ## Declarator
	;

storage_class_specifier
	: TYPEDEF
	| EXTERN
	| STATIC
	| AUTO
	| REGISTER
	;

type_specifier
	: VOID                                                                          ## CommonSpName
	| CHAR                                                                          ## CommonSpName
	| SHORT                                                                         ## CommonSpName
	| INT                                                                           ## CommonSpName
	| LONG                                                                          ## CommonSpName
	| FLOAT                                                                         ## CommonSpName
	| DOUBLE                                                                        ## CommonSpName
	| SIGNED
	| UNSIGNED                                                                      ## CommonSpName
	| BOOL                                                                          ## CommonSpName
	| PORT                                                                          ## CommonSpName
	| COMPLEX
	| IMAGINARY
	| struct_or_union_specifier
	| enum_specifier
	| TYPE_NAME
	;

struct_or_union_specifier
	: struct_or_union IDENTIFIER '{' struct_declaration_list '}'                    ## RecordDeclaration
	| struct_or_union '{' struct_declaration_list '}'                               ## RecordDeclaration
	| struct_or_union IDENTIFIER                                                    ## RecordDeclaration
	;

struct_or_union
	: STRUCT
	| UNION
	;

struct_declaration_list
	: struct_declaration
	| struct_declaration_list struct_declaration
	;

struct_declaration
	: specifier_qualifier_list struct_declarator_list ';'                           ## DeclarationInRecord
	;

specifier_qualifier_list
	: type_specifier specifier_qualifier_list                                       ## TypeSp
	| type_specifier                                                                ## TypeSp
	| type_qualifier specifier_qualifier_list                                       ## TypeQf
	| type_qualifier                                                                ## TypeQf
	;

struct_declarator_list
	: struct_declarator
	| struct_declarator_list ',' struct_declarator
	;

struct_declarator
	: declarator                                                                    ## Declarator
	| ':' constant_expression
	| declarator ':' constant_expression
	;

enum_specifier
	: ENUM '{' enumerator_list '}'                                                  ## EnumDeclaration
	| ENUM IDENTIFIER '{' enumerator_list '}'                                       ## EnumDeclaration
	| ENUM '{' enumerator_list ',' '}'                                              ## EnumDeclaration
	| ENUM IDENTIFIER '{' enumerator_list ',' '}'                                   ## EnumDeclaration
	| ENUM IDENTIFIER                                                               ## EnumDeclaration
	;

enumerator_list
	: enumerator
	| enumerator_list ',' enumerator
	;

enumerator
	: IDENTIFIER                                                                    ## DeclarationInEnum
	| IDENTIFIER '=' constant_expression                                            ## DeclarationInEnum
	;

type_qualifier
	: CONST                                                                         ## QfName
	| RESTRICT                                                                      ## QfName
	| VOLATILE                                                                      ## QfName
	;

function_specifier
	: INLINE
	;

declarator
	: pointer direct_declarator
	| direct_declarator
	;


direct_declarator
	: IDENTIFIER                                                                    ## DeclaratorName
	| '(' declarator ')'
	| direct_declarator '[' type_qualifier_list assignment_expression ']'
	| direct_declarator '[' type_qualifier_list ']'
	| direct_declarator '[' assignment_expression ']'                               ## Array
	| direct_declarator '[' STATIC type_qualifier_list assignment_expression ']'    ## StaticModifierArray
	| direct_declarator '[' type_qualifier_list STATIC assignment_expression ']'    ## StaticModifierArray
	| direct_declarator '[' type_qualifier_list '*' ']'
	| direct_declarator '[' '*' ']'
	| direct_declarator '[' ']'                                                     ## IncompleteArray
	| direct_declarator '(' parameter_type_list ')'
	| direct_declarator '(' identifier_list ')'
	| direct_declarator '(' ')'
	;

pointer
	: '*'                                                                           ## Pointer
	| '*' type_qualifier_list                                                       ## Pointer
	| '*' pointer
	| '*' type_qualifier_list pointer
	;

type_qualifier_list
	: type_qualifier
	| type_qualifier_list type_qualifier
	;


parameter_type_list
	: parameter_list
	| parameter_list ',' ELLIPSIS
	;

parameter_list
	: parameter_declaration
	| parameter_list ',' parameter_declaration
	;

parameter_declaration
	: declaration_specifiers declarator                                             ## FormalParamDef
	| declaration_specifiers abstract_declarator
	| declaration_specifiers
	;

identifier_list
	: IDENTIFIER
	| identifier_list ',' IDENTIFIER
	;

type_name
	: specifier_qualifier_list                                                      ## FullType
	| specifier_qualifier_list abstract_declarator
	;

abstract_declarator
	: pointer
	| direct_abstract_declarator
	| pointer direct_abstract_declarator
	;

direct_abstract_declarator
	: '(' abstract_declarator ')'
	| '[' ']'
	| '[' assignment_expression ']'
	| direct_abstract_declarator '[' ']'
	| direct_abstract_declarator '[' assignment_expression ']'
	| '[' '*' ']'
	| direct_abstract_declarator '[' '*' ']'
	| '(' ')'
	| '(' parameter_type_list ')'
	| direct_abstract_declarator '(' ')'
	| direct_abstract_declarator '(' parameter_type_list ')'
	;

initializer
	: assignment_expression
	| '{' initializer_list '}'
	| '{' initializer_list ',' '}'
	;

initializer_list
	: initializer
	| designation initializer
	| initializer_list ',' initializer
	| initializer_list ',' designation initializer
	;

designation
	: designator_list '='
	;

designator_list
	: designator
	| designator_list designator
	;

designator
	: '[' constant_expression ']'
	| '.' IDENTIFIER
	;

statement
	: labeled_statement                                                                     ## VirtualStmt
	| compound_statement                                                                    ## VirtualStmt
	| expression_statement                                                                  ## VirtualStmt
	| selection_statement                                                                   ## VirtualStmt
	| iteration_statement                                                                   ## VirtualStmt
	| jump_statement                                                                        ## VirtualStmt
	;

labeled_statement
	: IDENTIFIER ':' statement
	| CASE constant_expression ':' statement
	| DEFAULT ':' statement
	;

compound_statement
	: '{' '}'                                                                               ## CmpdStmt
	| '{' block_item_list '}'                                                               ## CmpdStmt
	;

block_item_list
	: block_item
	| block_item_list block_item
	;

block_item
	: declaration
	| statement
	;

expression_statement
	: ';'
	| expression ';'
	;

selection_statement
	: IF '(' expression ')' statement                                                       ## IfStmt
	| IF '(' expression ')' statement ELSE statement                                        ## IfElseStmt
	| SWITCH '(' expression ')' statement
	;

iteration_statement
	: WHILE '(' expression ')' statement                                                    ## WhileStmt
	| DO statement WHILE '(' expression ')' ';'                                             ## DoWhileStmt
	| FOR '(' expression_statement expression_statement ')' statement                       ## ForStmt
	| FOR '(' expression_statement expression_statement expression ')' statement            ## ForStmt
	| FOR '(' declaration expression_statement ')' statement                                ## ForStmt
	| FOR '(' declaration expression_statement expression ')' statement                     ## ForStmt
	;

jump_statement
	: GOTO IDENTIFIER ';'
	| CONTINUE ';'
	| BREAK ';'
	| RETURN ';'                                                                            ## Return
	| RETURN expression ';'                                                                 ## Return
	;

translation_unit
	: external_declaration
	| translation_unit external_declaration
	;

external_declaration
	: function_definition
	| declaration
	;

function_definition
	: declaration_specifiers declarator declaration_list compound_statement
	| declaration_specifiers declarator compound_statement                                  ## FuncDef
	;

declaration_list
	: declaration
	| declaration_list declaration
	;


%%
#include <stdio.h>

extern char yytext[];
extern int column;

void yyerror(char const *s)
{
	fflush(stdout);
	printf("\n%*s\n%*s\n", column, "^", column, s);
}
