/* A Bison parser, made by GNU Bison 3.7.5.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30705

/* Bison version string.  */
#define YYBISON_VERSION "3.7.5"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 1 "main/parser.yy"

// Bison parser for FST generation.

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <thrax/compat/utils.h>
#include <thrax/collection-node.h>
#include <thrax/fst-node.h>
#include <thrax/function-node.h>
#include <thrax/grammar-node.h>
#include <thrax/identifier-node.h>
#include <thrax/import-node.h>
#include <thrax/node.h>
#include <thrax/return-node.h>
#include <thrax/rule-node.h>
#include <thrax/statement-node.h>
#include <thrax/string-node.h>
#include <thrax/grm-compiler.h>
#include <thrax/lexer.h>

DECLARE_bool(always_export);

using namespace thrax;

class FuncOrStmt {
public:
  CollectionNode *funcs_;
  CollectionNode *stmts_;
};

namespace thrax_rewriter {
  int yylex(void *, GrmCompilerParserInterface *parm);
  int yyerror(GrmCompilerParserInterface *, const char *);

#line 106 "main/parser.cc"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif


/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    tDESCR = 258,                  /* tDESCR  */
    tDQSTRING = 259,               /* tDQSTRING  */
    tQSTRING = 260,                /* tQSTRING  */
    tINTEGER = 261,                /* tINTEGER  */
    tFLOAT = 262,                  /* tFLOAT  */
    tLPAREN = 263,                 /* tLPAREN  */
    tRPAREN = 264,                 /* tRPAREN  */
    tLBRACE = 265,                 /* tLBRACE  */
    tRBRACE = 266,                 /* tRBRACE  */
    tPIPE = 267,                   /* tPIPE  */
    tSTAR = 268,                   /* tSTAR  */
    tPLUS = 269,                   /* tPLUS  */
    tQMARK = 270,                  /* tQMARK  */
    tDOT = 271,                    /* tDOT  */
    tCOMMA = 272,                  /* tCOMMA  */
    tAT = 273,                     /* tAT  */
    tCOLON = 274,                  /* tCOLON  */
    tSEMICOLON = 275,              /* tSEMICOLON  */
    tEQUALS = 276,                 /* tEQUALS  */
    tCONCAT = 277,                 /* tCONCAT  */
    tUNARY = 278,                  /* tUNARY  */
    tDOLLAR = 279,                 /* tDOLLAR  */
    tLBRACKET = 280,               /* tLBRACKET  */
    tRBRACKET = 281,               /* tRBRACKET  */
    tANGLE_STRING = 282,           /* tANGLE_STRING  */
    tSLASH = 283,                  /* tSLASH  */
    tUNDERSCORE = 284,             /* tUNDERSCORE  */
    tMINUS = 285,                  /* tMINUS  */
    tREAD_STRINGS = 286,           /* tREAD_STRINGS  */
    tREAD_WEIGHTED_STRINGS = 287,  /* tREAD_WEIGHTED_STRINGS  */
    tCASE_INSENSITIVE = 288,       /* tCASE_INSENSITIVE  */
    tKEYWORD_EXPORT = 289,         /* tKEYWORD_EXPORT  */
    tKEYWORD_FUNC = 290,           /* tKEYWORD_FUNC  */
    tKEYWORD_RETURN = 291,         /* tKEYWORD_RETURN  */
    tKEYWORD_BYTE = 292,           /* tKEYWORD_BYTE  */
    tKEYWORD_UTF8 = 293,           /* tKEYWORD_UTF8  */
    tKEYWORD_AS = 294,             /* tKEYWORD_AS  */
    tKEYWORD_IMPORT = 295          /* tKEYWORD_IMPORT  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define tDESCR 258
#define tDQSTRING 259
#define tQSTRING 260
#define tINTEGER 261
#define tFLOAT 262
#define tLPAREN 263
#define tRPAREN 264
#define tLBRACE 265
#define tRBRACE 266
#define tPIPE 267
#define tSTAR 268
#define tPLUS 269
#define tQMARK 270
#define tDOT 271
#define tCOMMA 272
#define tAT 273
#define tCOLON 274
#define tSEMICOLON 275
#define tEQUALS 276
#define tCONCAT 277
#define tUNARY 278
#define tDOLLAR 279
#define tLBRACKET 280
#define tRBRACKET 281
#define tANGLE_STRING 282
#define tSLASH 283
#define tUNDERSCORE 284
#define tMINUS 285
#define tREAD_STRINGS 286
#define tREAD_WEIGHTED_STRINGS 287
#define tCASE_INSENSITIVE 288
#define tKEYWORD_EXPORT 289
#define tKEYWORD_FUNC 290
#define tKEYWORD_RETURN 291
#define tKEYWORD_BYTE 292
#define tKEYWORD_UTF8 293
#define tKEYWORD_AS 294
#define tKEYWORD_IMPORT 295

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 40 "main/parser.yy"

  int                        int_type;
  FunctionNode*              function_node_type;
  ImportNode*                import_node_type;
  ReturnNode*                return_node_type;
  RuleNode*                  rule_node_type;
  IdentifierNode*            identifier_node_type;
  CollectionNode*            collection_node_type;
  StatementNode*             statement_node_type;
  FstNode*                   fst_node_type;
  StringNode*                string_node_type;
  GrammarNode*               grammar_node_type;
  Node*                      node_type;
  FuncOrStmt*                function_or_statement_node_type;

#line 252 "main/parser.cc"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int yyparse (GrmCompilerParserInterface *parm);


/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_tDESCR = 3,                     /* tDESCR  */
  YYSYMBOL_tDQSTRING = 4,                  /* tDQSTRING  */
  YYSYMBOL_tQSTRING = 5,                   /* tQSTRING  */
  YYSYMBOL_tINTEGER = 6,                   /* tINTEGER  */
  YYSYMBOL_tFLOAT = 7,                     /* tFLOAT  */
  YYSYMBOL_tLPAREN = 8,                    /* tLPAREN  */
  YYSYMBOL_tRPAREN = 9,                    /* tRPAREN  */
  YYSYMBOL_tLBRACE = 10,                   /* tLBRACE  */
  YYSYMBOL_tRBRACE = 11,                   /* tRBRACE  */
  YYSYMBOL_tPIPE = 12,                     /* tPIPE  */
  YYSYMBOL_tSTAR = 13,                     /* tSTAR  */
  YYSYMBOL_tPLUS = 14,                     /* tPLUS  */
  YYSYMBOL_tQMARK = 15,                    /* tQMARK  */
  YYSYMBOL_tDOT = 16,                      /* tDOT  */
  YYSYMBOL_tCOMMA = 17,                    /* tCOMMA  */
  YYSYMBOL_tAT = 18,                       /* tAT  */
  YYSYMBOL_tCOLON = 19,                    /* tCOLON  */
  YYSYMBOL_tSEMICOLON = 20,                /* tSEMICOLON  */
  YYSYMBOL_tEQUALS = 21,                   /* tEQUALS  */
  YYSYMBOL_tCONCAT = 22,                   /* tCONCAT  */
  YYSYMBOL_tUNARY = 23,                    /* tUNARY  */
  YYSYMBOL_tDOLLAR = 24,                   /* tDOLLAR  */
  YYSYMBOL_tLBRACKET = 25,                 /* tLBRACKET  */
  YYSYMBOL_tRBRACKET = 26,                 /* tRBRACKET  */
  YYSYMBOL_tANGLE_STRING = 27,             /* tANGLE_STRING  */
  YYSYMBOL_tSLASH = 28,                    /* tSLASH  */
  YYSYMBOL_tUNDERSCORE = 29,               /* tUNDERSCORE  */
  YYSYMBOL_tMINUS = 30,                    /* tMINUS  */
  YYSYMBOL_tREAD_STRINGS = 31,             /* tREAD_STRINGS  */
  YYSYMBOL_tREAD_WEIGHTED_STRINGS = 32,    /* tREAD_WEIGHTED_STRINGS  */
  YYSYMBOL_tCASE_INSENSITIVE = 33,         /* tCASE_INSENSITIVE  */
  YYSYMBOL_tKEYWORD_EXPORT = 34,           /* tKEYWORD_EXPORT  */
  YYSYMBOL_tKEYWORD_FUNC = 35,             /* tKEYWORD_FUNC  */
  YYSYMBOL_tKEYWORD_RETURN = 36,           /* tKEYWORD_RETURN  */
  YYSYMBOL_tKEYWORD_BYTE = 37,             /* tKEYWORD_BYTE  */
  YYSYMBOL_tKEYWORD_UTF8 = 38,             /* tKEYWORD_UTF8  */
  YYSYMBOL_tKEYWORD_AS = 39,               /* tKEYWORD_AS  */
  YYSYMBOL_tKEYWORD_IMPORT = 40,           /* tKEYWORD_IMPORT  */
  YYSYMBOL_YYACCEPT = 41,                  /* $accept  */
  YYSYMBOL_grammar = 42,                   /* grammar  */
  YYSYMBOL_import_list = 43,               /* import_list  */
  YYSYMBOL_func_or_stmt_list = 44,         /* func_or_stmt_list  */
  YYSYMBOL_stmt_list = 45,                 /* stmt_list  */
  YYSYMBOL_stmt = 46,                      /* stmt  */
  YYSYMBOL_return_stmt = 47,               /* return_stmt  */
  YYSYMBOL_rule_stmt = 48,                 /* rule_stmt  */
  YYSYMBOL_rule_body = 49,                 /* rule_body  */
  YYSYMBOL_descriptor = 50,                /* descriptor  */
  YYSYMBOL_atomic_obj = 51,                /* atomic_obj  */
  YYSYMBOL_obj = 52,                       /* obj  */
  YYSYMBOL_concat_fst = 53,                /* concat_fst  */
  YYSYMBOL_union_fst = 54,                 /* union_fst  */
  YYSYMBOL_difference_fst = 55,            /* difference_fst  */
  YYSYMBOL_grouped_obj = 56,               /* grouped_obj  */
  YYSYMBOL_funccall_obj = 57,              /* funccall_obj  */
  YYSYMBOL_funccall_arglist = 58,          /* funccall_arglist  */
  YYSYMBOL_funccall_arguments = 59,        /* funccall_arguments  */
  YYSYMBOL_repetition_fst = 60,            /* repetition_fst  */
  YYSYMBOL_composition_fst = 61,           /* composition_fst  */
  YYSYMBOL_identifier_obj = 62,            /* identifier_obj  */
  YYSYMBOL_string_fst = 63,                /* string_fst  */
  YYSYMBOL_fst_with_output = 64,           /* fst_with_output  */
  YYSYMBOL_fst_with_weight = 65,           /* fst_with_weight  */
  YYSYMBOL_number = 66,                    /* number  */
  YYSYMBOL_quoted_fst_string = 67,         /* quoted_fst_string  */
  YYSYMBOL_quoted_string = 68,             /* quoted_string  */
  YYSYMBOL_weight = 69,                    /* weight  */
  YYSYMBOL_import_request = 70,            /* import_request  */
  YYSYMBOL_func_decl = 71,                 /* func_decl  */
  YYSYMBOL_func_arglist = 72,              /* func_arglist  */
  YYSYMBOL_func_arguments = 73,            /* func_arguments  */
  YYSYMBOL_func_body = 74                  /* func_body  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                            \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  9
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   126

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  41
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  34
/* YYNRULES -- Number of rules.  */
#define YYNRULES  72
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  116

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   295


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   113,   113,   118,   125,   126,   133,   137,   140,   147,
     148,   154,   159,   164,   167,   173,   179,   185,   191,   196,
     207,   208,   209,   210,   211,   215,   220,   226,   232,   238,
     244,   250,   256,   262,   270,   272,   277,   281,   286,   291,
     295,   300,   308,   312,   316,   320,   331,   338,   344,   350,
     355,   363,   368,   373,   378,   384,   393,   399,   404,   410,
     417,   422,   429,   436,   443,   447,   453,   457,   463,   465,
     470,   474,   480
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "tDESCR", "tDQSTRING",
  "tQSTRING", "tINTEGER", "tFLOAT", "tLPAREN", "tRPAREN", "tLBRACE",
  "tRBRACE", "tPIPE", "tSTAR", "tPLUS", "tQMARK", "tDOT", "tCOMMA", "tAT",
  "tCOLON", "tSEMICOLON", "tEQUALS", "tCONCAT", "tUNARY", "tDOLLAR",
  "tLBRACKET", "tRBRACKET", "tANGLE_STRING", "tSLASH", "tUNDERSCORE",
  "tMINUS", "tREAD_STRINGS", "tREAD_WEIGHTED_STRINGS", "tCASE_INSENSITIVE",
  "tKEYWORD_EXPORT", "tKEYWORD_FUNC", "tKEYWORD_RETURN", "tKEYWORD_BYTE",
  "tKEYWORD_UTF8", "tKEYWORD_AS", "tKEYWORD_IMPORT", "$accept", "grammar",
  "import_list", "func_or_stmt_list", "stmt_list", "stmt", "return_stmt",
  "rule_stmt", "rule_body", "descriptor", "atomic_obj", "obj",
  "concat_fst", "union_fst", "difference_fst", "grouped_obj",
  "funccall_obj", "funccall_arglist", "funccall_arguments",
  "repetition_fst", "composition_fst", "identifier_obj", "string_fst",
  "fst_with_output", "fst_with_weight", "number", "quoted_fst_string",
  "quoted_string", "weight", "import_request", "func_decl", "func_arglist",
  "func_arguments", "func_body", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_int16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295
};
#endif

#define YYPACT_NINF (-75)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-10)

#define yytable_value_is_error(Yyn) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int8 yypact[] =
{
       1,   -13,    11,    15,    10,    18,   -75,   -75,     8,   -75,
       3,   -75,    59,    59,    73,   -75,    10,   -75,   -75,    46,
     -75,    10,   -13,   -75,    59,   -75,   -75,    47,    49,   -75,
      73,    55,    69,    65,   -75,    72,    56,   -75,   -75,    73,
      13,   -75,   -75,    68,   -75,    76,   -75,   -75,    73,   -75,
      74,    73,    25,    86,    91,    61,   -75,    95,   -75,   -75,
     -75,   -75,    73,    73,   -75,    73,    73,   -75,   -75,     2,
      82,   -75,   -75,    83,   -75,    89,    81,    21,   -75,   -75,
     -75,    92,    93,    94,    87,   -75,    32,   -75,   -75,   -75,
      56,   -75,   -75,   -75,   -75,   -75,   -75,    59,   -75,    88,
     101,    21,    67,    67,    67,   -75,   -75,    95,   -75,   -75,
     -75,   -75,   -75,   -75,   103,   -75
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,     3,     0,     0,     0,     0,    65,    62,     0,     1,
       0,    19,     0,     0,     0,     2,     0,    12,    11,     0,
      13,     0,     0,     5,     0,    67,    14,     0,     0,    61,
       0,    50,    47,     0,    31,    57,    49,    24,    23,    27,
      29,    22,    21,    59,    25,    51,    20,     7,     0,     8,
       0,     0,     0,     0,     0,     0,    33,     0,    42,    43,
      44,    15,     0,     0,    26,     0,     0,    63,    58,     0,
       0,    18,    64,     0,    68,    70,     0,     0,    66,    32,
      34,    38,    40,    36,     0,    60,     0,    56,    30,    28,
      48,    52,    53,    55,    54,    16,    17,     0,    69,     0,
       0,     0,     0,     0,     0,    35,    46,     0,    71,    72,
      10,    39,    41,    37,     0,    45
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -75,   -75,   110,    17,    16,   -74,   -75,   -75,    70,    -4,
     -75,    12,   -33,    -6,    50,   -75,    51,   -75,   -14,   -75,
     -75,    53,   -75,   -75,   -75,    19,   -75,   116,   -75,    29,
     -75,   -75,    22,   -75
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,     3,     4,    15,   100,    16,    17,    18,    70,    31,
      32,    83,    34,    35,    36,    37,    38,    56,    84,    39,
      40,    41,    42,    43,    44,    86,    45,    46,    68,    20,
      21,    53,    76,    78
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int8 yytable[] =
{
      19,    -4,     1,   101,    -4,    11,    64,     6,    27,    28,
      -6,    10,    19,    11,    25,     9,     7,    19,    -4,    22,
      50,    -4,    99,    26,    11,    65,    33,   101,    11,     5,
      88,    66,    -9,    47,     5,    -4,    -4,    -4,    49,    91,
      92,     2,    54,   106,    12,    13,    14,    24,    75,   107,
       2,    74,    -4,    -4,    -4,    12,    87,    14,     2,    89,
      71,     2,    11,    71,    11,    29,     7,    48,    51,    30,
      11,    29,     7,    19,    52,    30,    11,    29,     7,    57,
      55,    30,    58,    59,    60,    61,    63,    80,   111,   112,
     113,    62,    69,    75,    72,    67,    77,    19,    81,    82,
      79,    85,    95,    96,    81,    82,    97,    98,    26,   102,
     103,   104,   109,   105,   115,    23,    90,   110,     8,   108,
      93,    73,    94,     0,     0,     0,   114
};

static const yytype_int8 yycheck[] =
{
       4,     0,     1,    77,     3,     3,    39,    20,    12,    13,
       0,     1,    16,     3,    11,     0,     5,    21,     0,     1,
      24,     3,     1,    20,     3,    12,    14,   101,     3,     0,
      63,    18,    11,    16,     5,    34,    35,    36,    21,    37,
      38,    40,    30,    11,    34,    35,    36,    39,    52,    17,
      40,    26,    34,    35,    36,    34,    62,    36,    40,    65,
      48,    40,     3,    51,     3,     4,     5,    21,    21,     8,
       3,     4,     5,    77,    25,     8,     3,     4,     5,    10,
      25,     8,    13,    14,    15,    20,    30,    26,   102,   103,
     104,    19,    16,    97,    20,    27,    10,   101,    37,    38,
       9,     6,    20,    20,    37,    38,    17,    26,    20,    17,
      17,    17,    11,    26,    11,     5,    66,   101,     2,    97,
      69,    51,    69,    -1,    -1,    -1,   107
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     1,    40,    42,    43,    70,    20,     5,    68,     0,
       1,     3,    34,    35,    36,    44,    46,    47,    48,    50,
      70,    71,     1,    43,    39,    11,    20,    50,    50,     4,
       8,    50,    51,    52,    53,    54,    55,    56,    57,    60,
      61,    62,    63,    64,    65,    67,    68,    44,    21,    44,
      50,    21,    25,    72,    52,    25,    58,    10,    13,    14,
      15,    20,    19,    30,    53,    12,    18,    27,    69,    16,
      49,    52,    20,    49,    26,    50,    73,    10,    74,     9,
      26,    37,    38,    52,    59,     6,    66,    54,    53,    54,
      55,    37,    38,    57,    62,    20,    20,    17,    26,     1,
      45,    46,    17,    17,    17,    26,    11,    17,    73,    11,
      45,    59,    59,    59,    66,    11
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_int8 yyr1[] =
{
       0,    41,    42,    42,    43,    43,    44,    44,    44,    45,
      45,    46,    46,    46,    46,    47,    48,    48,    49,    50,
      51,    51,    51,    51,    51,    52,    53,    53,    54,    54,
      55,    55,    56,    57,    58,    58,    59,    59,    59,    59,
      59,    59,    60,    60,    60,    60,    60,    60,    61,    61,
      62,    63,    63,    63,    63,    63,    64,    64,    65,    65,
      66,    67,    68,    69,    70,    70,    71,    71,    72,    72,
      73,    73,    74
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     2,     1,     0,     2,     0,     2,     2,     0,
       2,     1,     1,     1,     2,     3,     4,     5,     1,     1,
       1,     1,     1,     1,     1,     1,     2,     1,     3,     1,
       3,     1,     3,     2,     2,     3,     1,     3,     1,     3,
       1,     3,     2,     2,     2,     6,     4,     1,     3,     1,
       1,     1,     3,     3,     3,     3,     3,     1,     2,     1,
       1,     1,     1,     1,     5,     2,     4,     2,     2,     3,
       1,     3,     3
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (parm, YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
# ifndef YY_LOCATION_PRINT
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif


# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, parm); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, GrmCompilerParserInterface *parm)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (parm);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yykind < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yykind], *yyvaluep);
# endif
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, GrmCompilerParserInterface *parm)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep, parm);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule, GrmCompilerParserInterface *parm)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)], parm);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, parm); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, GrmCompilerParserInterface *parm)
{
  YY_USE (yyvaluep);
  YY_USE (parm);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}






/*----------.
| yyparse.  |
`----------*/

int
yyparse (GrmCompilerParserInterface *parm)
{
/* Lookahead token kind.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

    /* Number of syntax errors so far.  */
    int yynerrs = 0;

    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    goto yyexhaustedlab;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex (&yylval, parm);
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* grammar: import_list func_or_stmt_list  */
#line 114 "main/parser.yy"
    { GrammarNode* node = new GrammarNode((yyvsp[-1].collection_node_type), (yyvsp[0].function_or_statement_node_type)->funcs_, (yyvsp[0].function_or_statement_node_type)->stmts_);
      delete (yyvsp[0].function_or_statement_node_type);
      (yyval.grammar_node_type) = node;
      parm->SetAst(fst::WrapUnique(fst::implicit_cast<Node*>((yyval.grammar_node_type)))); }
#line 1412 "main/parser.cc"
    break;

  case 3: /* grammar: error  */
#line 119 "main/parser.yy"
    { parm->Error("Generic parsing error.");
      (yyval.grammar_node_type) = NULL; }
#line 1419 "main/parser.cc"
    break;

  case 4: /* import_list: %empty  */
#line 125 "main/parser.yy"
    { (yyval.collection_node_type) = new CollectionNode(); }
#line 1425 "main/parser.cc"
    break;

  case 5: /* import_list: import_request import_list  */
#line 127 "main/parser.yy"
    { (yyvsp[0].collection_node_type)->AddFront((yyvsp[-1].import_node_type));
      (yyval.collection_node_type) = (yyvsp[0].collection_node_type); }
#line 1432 "main/parser.cc"
    break;

  case 6: /* func_or_stmt_list: %empty  */
#line 133 "main/parser.yy"
    { (yyval.function_or_statement_node_type) = new FuncOrStmt();
      (yyval.function_or_statement_node_type)->funcs_ = new CollectionNode();
      (yyval.function_or_statement_node_type)->stmts_ = new CollectionNode();
    }
#line 1441 "main/parser.cc"
    break;

  case 7: /* func_or_stmt_list: stmt func_or_stmt_list  */
#line 138 "main/parser.yy"
    { (yyvsp[0].function_or_statement_node_type)->stmts_->AddFront((yyvsp[-1].statement_node_type));
      (yyval.function_or_statement_node_type) = (yyvsp[0].function_or_statement_node_type); }
#line 1448 "main/parser.cc"
    break;

  case 8: /* func_or_stmt_list: func_decl func_or_stmt_list  */
#line 141 "main/parser.yy"
    { (yyvsp[0].function_or_statement_node_type)->funcs_->AddFront((yyvsp[-1].function_node_type));
      (yyval.function_or_statement_node_type) = (yyvsp[0].function_or_statement_node_type); }
#line 1455 "main/parser.cc"
    break;

  case 9: /* stmt_list: %empty  */
#line 147 "main/parser.yy"
    { (yyval.collection_node_type) = new CollectionNode(); }
#line 1461 "main/parser.cc"
    break;

  case 10: /* stmt_list: stmt stmt_list  */
#line 149 "main/parser.yy"
    { (yyvsp[0].collection_node_type)->AddFront((yyvsp[-1].statement_node_type));
      (yyval.collection_node_type) = (yyvsp[0].collection_node_type); }
#line 1468 "main/parser.cc"
    break;

  case 11: /* stmt: rule_stmt  */
#line 155 "main/parser.yy"
    { StatementNode* node = new StatementNode(StatementNode::RULE_STATEMENTNODE);
      node->SetLine(parm->GetLexer()->line_number());
      node->Set((yyvsp[0].rule_node_type));
      (yyval.statement_node_type) = node; }
#line 1477 "main/parser.cc"
    break;

  case 12: /* stmt: return_stmt  */
#line 160 "main/parser.yy"
    { StatementNode* node = new StatementNode(StatementNode::RETURN_STATEMENTNODE);
      node->SetLine(parm->GetLexer()->line_number());
      node->Set((yyvsp[0].return_node_type));
      (yyval.statement_node_type) = node; }
#line 1486 "main/parser.cc"
    break;

  case 13: /* stmt: import_request  */
#line 165 "main/parser.yy"
    { parm->Error("import statements must occur in the first block of the grammar.");
      (yyval.statement_node_type) = NULL; }
#line 1493 "main/parser.cc"
    break;

  case 14: /* stmt: error tSEMICOLON  */
#line 168 "main/parser.yy"
    { parm->Error("Invalid statement (or previous statement).");
      (yyval.statement_node_type) = NULL; }
#line 1500 "main/parser.cc"
    break;

  case 15: /* return_stmt: tKEYWORD_RETURN obj tSEMICOLON  */
#line 174 "main/parser.yy"
    { ReturnNode* node = new ReturnNode((yyvsp[-1].node_type));
      (yyval.return_node_type) = node; }
#line 1507 "main/parser.cc"
    break;

  case 16: /* rule_stmt: descriptor tEQUALS rule_body tSEMICOLON  */
#line 180 "main/parser.yy"
    { RuleNode* node = new RuleNode((yyvsp[-3].identifier_node_type), (yyvsp[-1].node_type),
                                    FST_FLAGS_always_export ?
                                    RuleNode::EXPORT :
                                    RuleNode::DO_NOT_EXPORT);
      (yyval.rule_node_type) = node; }
#line 1517 "main/parser.cc"
    break;

  case 17: /* rule_stmt: tKEYWORD_EXPORT descriptor tEQUALS rule_body tSEMICOLON  */
#line 186 "main/parser.yy"
    { RuleNode* node = new RuleNode((yyvsp[-3].identifier_node_type), (yyvsp[-1].node_type), RuleNode::EXPORT);
      (yyval.rule_node_type) = node; }
#line 1524 "main/parser.cc"
    break;

  case 18: /* rule_body: obj  */
#line 192 "main/parser.yy"
    { (yyval.node_type) = (yyvsp[0].node_type); }
#line 1530 "main/parser.cc"
    break;

  case 19: /* descriptor: tDESCR  */
#line 197 "main/parser.yy"
    { const std::string& name = parm->GetLexer()->YYString();
      int begin_pos = parm->GetLexer()->YYBeginPos();
      IdentifierNode* node = new IdentifierNode(name, begin_pos);
      node->SetLine(parm->GetLexer()->line_number());
      if (!node->IsValid())
        parm->Error(StringPrintf("Illegal identifier: %s", name.c_str()));
      (yyval.identifier_node_type) = node; }
#line 1542 "main/parser.cc"
    break;

  case 20: /* atomic_obj: quoted_string  */
#line 207 "main/parser.yy"
                   { (yyval.node_type) = (yyvsp[0].string_node_type); }
#line 1548 "main/parser.cc"
    break;

  case 21: /* atomic_obj: string_fst  */
#line 208 "main/parser.yy"
                   { (yyval.node_type) = (yyvsp[0].node_type); }
#line 1554 "main/parser.cc"
    break;

  case 22: /* atomic_obj: identifier_obj  */
#line 209 "main/parser.yy"
                   { (yyval.node_type) = (yyvsp[0].node_type); }
#line 1560 "main/parser.cc"
    break;

  case 23: /* atomic_obj: funccall_obj  */
#line 210 "main/parser.yy"
                   { (yyval.node_type) = (yyvsp[0].node_type); }
#line 1566 "main/parser.cc"
    break;

  case 24: /* atomic_obj: grouped_obj  */
#line 211 "main/parser.yy"
                   { (yyval.node_type) = (yyvsp[0].node_type); }
#line 1572 "main/parser.cc"
    break;

  case 25: /* obj: fst_with_weight  */
#line 215 "main/parser.yy"
                  { (yyval.node_type) = (yyvsp[0].node_type); }
#line 1578 "main/parser.cc"
    break;

  case 26: /* concat_fst: repetition_fst concat_fst  */
#line 221 "main/parser.yy"
    { FstNode* node = new FstNode(FstNode::CONCAT_FSTNODE);
      node->SetLine(parm->GetLexer()->line_number());
      node->AddArgument((yyvsp[-1].node_type));
      node->AddArgument((yyvsp[0].node_type));
      (yyval.node_type) = node; }
#line 1588 "main/parser.cc"
    break;

  case 27: /* concat_fst: repetition_fst  */
#line 227 "main/parser.yy"
    { (yyval.node_type) = (yyvsp[0].node_type); }
#line 1594 "main/parser.cc"
    break;

  case 28: /* union_fst: composition_fst tPIPE union_fst  */
#line 233 "main/parser.yy"
    { FstNode* node = new FstNode(FstNode::UNION_FSTNODE);
      node->SetLine(parm->GetLexer()->line_number());
      node->AddArgument((yyvsp[-2].node_type));
      node->AddArgument((yyvsp[0].node_type));
      (yyval.node_type) = node; }
#line 1604 "main/parser.cc"
    break;

  case 29: /* union_fst: composition_fst  */
#line 239 "main/parser.yy"
    { (yyval.node_type) = (yyvsp[0].node_type); }
#line 1610 "main/parser.cc"
    break;

  case 30: /* difference_fst: difference_fst tMINUS concat_fst  */
#line 245 "main/parser.yy"
    { FstNode* node = new FstNode(FstNode::DIFFERENCE_FSTNODE);
      node->SetLine(parm->GetLexer()->line_number());
      node->AddArgument((yyvsp[-2].node_type));
      node->AddArgument((yyvsp[0].node_type));
      (yyval.node_type) = node; }
#line 1620 "main/parser.cc"
    break;

  case 31: /* difference_fst: concat_fst  */
#line 251 "main/parser.yy"
    { (yyval.node_type) = (yyvsp[0].node_type); }
#line 1626 "main/parser.cc"
    break;

  case 32: /* grouped_obj: tLPAREN obj tRPAREN  */
#line 257 "main/parser.yy"
    { (yyval.node_type) = (yyvsp[-1].node_type); }
#line 1632 "main/parser.cc"
    break;

  case 33: /* funccall_obj: descriptor funccall_arglist  */
#line 263 "main/parser.yy"
    { FstNode* node = new FstNode(FstNode::FUNCTION_FSTNODE);
      node->SetLine(parm->GetLexer()->line_number());
      node->AddArgument((yyvsp[-1].identifier_node_type));
      node->AddArgument((yyvsp[0].collection_node_type));
      (yyval.node_type) = node; }
#line 1642 "main/parser.cc"
    break;

  case 34: /* funccall_arglist: tLBRACKET tRBRACKET  */
#line 271 "main/parser.yy"
    { (yyval.collection_node_type) = new CollectionNode(); }
#line 1648 "main/parser.cc"
    break;

  case 35: /* funccall_arglist: tLBRACKET funccall_arguments tRBRACKET  */
#line 273 "main/parser.yy"
    { (yyval.collection_node_type) = (yyvsp[-1].collection_node_type); }
#line 1654 "main/parser.cc"
    break;

  case 36: /* funccall_arguments: obj  */
#line 278 "main/parser.yy"
    { CollectionNode* node = new CollectionNode();
      node->AddFront((yyvsp[0].node_type));
      (yyval.collection_node_type) = node; }
#line 1662 "main/parser.cc"
    break;

  case 37: /* funccall_arguments: obj tCOMMA funccall_arguments  */
#line 282 "main/parser.yy"
    { (yyvsp[0].collection_node_type)->AddFront((yyvsp[-2].node_type));
      (yyval.collection_node_type) = (yyvsp[0].collection_node_type); }
#line 1669 "main/parser.cc"
    break;

  case 38: /* funccall_arguments: tKEYWORD_BYTE  */
#line 287 "main/parser.yy"
    { CollectionNode* node = new CollectionNode();
      StringNode* nnode = new StringNode("byte");
      node->AddFront(nnode);
      (yyval.collection_node_type) = node; }
#line 1678 "main/parser.cc"
    break;

  case 39: /* funccall_arguments: tKEYWORD_BYTE tCOMMA funccall_arguments  */
#line 292 "main/parser.yy"
    { StringNode* nnode = new StringNode("byte");
      (yyvsp[0].collection_node_type)->AddFront(nnode);
      (yyval.collection_node_type) = (yyvsp[0].collection_node_type); }
#line 1686 "main/parser.cc"
    break;

  case 40: /* funccall_arguments: tKEYWORD_UTF8  */
#line 296 "main/parser.yy"
    { CollectionNode* node = new CollectionNode();
      StringNode* nnode = new StringNode("utf8");
      node->AddFront(nnode);
      (yyval.collection_node_type) = node; }
#line 1695 "main/parser.cc"
    break;

  case 41: /* funccall_arguments: tKEYWORD_UTF8 tCOMMA funccall_arguments  */
#line 301 "main/parser.yy"
    { StringNode* nnode = new StringNode("utf8");
      (yyvsp[0].collection_node_type)->AddFront(nnode);
      (yyval.collection_node_type) = (yyvsp[0].collection_node_type); }
#line 1703 "main/parser.cc"
    break;

  case 42: /* repetition_fst: atomic_obj tSTAR  */
#line 309 "main/parser.yy"
    { RepetitionFstNode* node = new RepetitionFstNode(RepetitionFstNode::STAR);
      node->AddArgument((yyvsp[-1].node_type));
      (yyval.node_type) = node; }
#line 1711 "main/parser.cc"
    break;

  case 43: /* repetition_fst: atomic_obj tPLUS  */
#line 313 "main/parser.yy"
    { RepetitionFstNode* node = new RepetitionFstNode(RepetitionFstNode::PLUS);
      node->AddArgument((yyvsp[-1].node_type));
      (yyval.node_type) = node; }
#line 1719 "main/parser.cc"
    break;

  case 44: /* repetition_fst: atomic_obj tQMARK  */
#line 317 "main/parser.yy"
    { RepetitionFstNode* node = new RepetitionFstNode(RepetitionFstNode::QUESTION);
      node->AddArgument((yyvsp[-1].node_type));
      (yyval.node_type) = node; }
#line 1727 "main/parser.cc"
    break;

  case 45: /* repetition_fst: atomic_obj tLBRACE number tCOMMA number tRBRACE  */
#line 321 "main/parser.yy"
    { if ((yyvsp[-3].int_type) > (yyvsp[-1].int_type))
        parm->Error(StringPrintf("Reversed repetition bounds: %d > %d", (yyvsp[-3].int_type), (yyvsp[-1].int_type)));
      if ((yyvsp[-3].int_type) < 0)
        parm->Error(StringPrintf("Start bound must be non-negative: %d", (yyvsp[-3].int_type)));
      if ((yyvsp[-1].int_type) < 0)
        parm->Error(StringPrintf("End bound must be non-negative: %d", (yyvsp[-1].int_type)));
      RepetitionFstNode* node = new RepetitionFstNode(RepetitionFstNode::RANGE);
      node->AddArgument((yyvsp[-5].node_type));
      node->SetRange((yyvsp[-3].int_type), (yyvsp[-1].int_type));
      (yyval.node_type) = node; }
#line 1742 "main/parser.cc"
    break;

  case 46: /* repetition_fst: atomic_obj tLBRACE number tRBRACE  */
#line 332 "main/parser.yy"
    { if ((yyvsp[-1].int_type) < 0)
        parm->Error(StringPrintf("Repetition count should be non-negative: %d", (yyvsp[-1].int_type)));
      RepetitionFstNode* node = new RepetitionFstNode(RepetitionFstNode::RANGE);
      node->AddArgument((yyvsp[-3].node_type));
      node->SetRange((yyvsp[-1].int_type), (yyvsp[-1].int_type));
      (yyval.node_type) = node; }
#line 1753 "main/parser.cc"
    break;

  case 47: /* repetition_fst: atomic_obj  */
#line 339 "main/parser.yy"
    { (yyval.node_type) = (yyvsp[0].node_type); }
#line 1759 "main/parser.cc"
    break;

  case 48: /* composition_fst: composition_fst tAT difference_fst  */
#line 345 "main/parser.yy"
    { FstNode* node = new FstNode(FstNode::COMPOSITION_FSTNODE);
      node->SetLine(parm->GetLexer()->line_number());
      node->AddArgument((yyvsp[-2].node_type));
      node->AddArgument((yyvsp[0].node_type));
      (yyval.node_type) = node; }
#line 1769 "main/parser.cc"
    break;

  case 49: /* composition_fst: difference_fst  */
#line 351 "main/parser.yy"
    { (yyval.node_type) = (yyvsp[0].node_type); }
#line 1775 "main/parser.cc"
    break;

  case 50: /* identifier_obj: descriptor  */
#line 356 "main/parser.yy"
    { FstNode* node = new FstNode(FstNode::IDENTIFIER_FSTNODE);
      node->SetLine(parm->GetLexer()->line_number());
      node->AddArgument((yyvsp[0].identifier_node_type));
      (yyval.node_type) = node; }
#line 1784 "main/parser.cc"
    break;

  case 51: /* string_fst: quoted_fst_string  */
#line 364 "main/parser.yy"
    { StringFstNode* node = new StringFstNode(StringFstNode::BYTE);
      node->AddArgument((yyvsp[0].string_node_type));
      node->SetLine((yyvsp[0].string_node_type)->getline());  // Get the line from the actual text line.
      (yyval.node_type) = node; }
#line 1793 "main/parser.cc"
    break;

  case 52: /* string_fst: quoted_fst_string tDOT tKEYWORD_BYTE  */
#line 369 "main/parser.yy"
    { StringFstNode* node = new StringFstNode(StringFstNode::BYTE);
      node->AddArgument((yyvsp[-2].string_node_type));
      node->SetLine((yyvsp[-2].string_node_type)->getline());
      (yyval.node_type) = node; }
#line 1802 "main/parser.cc"
    break;

  case 53: /* string_fst: quoted_fst_string tDOT tKEYWORD_UTF8  */
#line 374 "main/parser.yy"
    { StringFstNode* node = new StringFstNode(StringFstNode::UTF8);
      node->AddArgument((yyvsp[-2].string_node_type));
      node->SetLine((yyvsp[-2].string_node_type)->getline());
      (yyval.node_type) = node; }
#line 1811 "main/parser.cc"
    break;

  case 54: /* string_fst: quoted_fst_string tDOT identifier_obj  */
#line 379 "main/parser.yy"
    { StringFstNode* node = new StringFstNode(StringFstNode::SYMBOL_TABLE);
      node->AddArgument((yyvsp[-2].string_node_type));
      node->AddArgument((yyvsp[0].node_type));
      node->SetLine((yyvsp[-2].string_node_type)->getline());
      (yyval.node_type) = node; }
#line 1821 "main/parser.cc"
    break;

  case 55: /* string_fst: quoted_fst_string tDOT funccall_obj  */
#line 385 "main/parser.yy"
    { StringFstNode* node = new StringFstNode(StringFstNode::SYMBOL_TABLE);
      node->AddArgument((yyvsp[-2].string_node_type));
      node->AddArgument((yyvsp[0].node_type));
      node->SetLine((yyvsp[-2].string_node_type)->getline());
      (yyval.node_type) = node; }
#line 1831 "main/parser.cc"
    break;

  case 56: /* fst_with_output: union_fst tCOLON union_fst  */
#line 394 "main/parser.yy"
    { FstNode* node = new FstNode(FstNode::REWRITE_FSTNODE);
      node->SetLine(parm->GetLexer()->line_number());
      node->AddArgument((yyvsp[-2].node_type));
      node->AddArgument((yyvsp[0].node_type));
      (yyval.node_type) = node; }
#line 1841 "main/parser.cc"
    break;

  case 57: /* fst_with_output: union_fst  */
#line 400 "main/parser.yy"
    { (yyval.node_type) = (yyvsp[0].node_type); }
#line 1847 "main/parser.cc"
    break;

  case 58: /* fst_with_weight: fst_with_output weight  */
#line 407 "main/parser.yy"
    { if (!fst::down_cast<FstNode*>((yyvsp[-1].node_type))->SetWeight((yyvsp[0].string_node_type)))
        parm->Error("Rules may have only one weight.");
      (yyval.node_type) = (yyvsp[-1].node_type); }
#line 1855 "main/parser.cc"
    break;

  case 59: /* fst_with_weight: fst_with_output  */
#line 411 "main/parser.yy"
    { (yyval.node_type) = (yyvsp[0].node_type); }
#line 1861 "main/parser.cc"
    break;

  case 60: /* number: tINTEGER  */
#line 418 "main/parser.yy"
    { (yyval.int_type) = atoi(parm->GetLexer()->YYString().c_str()); }
#line 1867 "main/parser.cc"
    break;

  case 61: /* quoted_fst_string: tDQSTRING  */
#line 423 "main/parser.yy"
    { StringNode* node = new StringNode(parm->GetLexer()->YYString());
      node->SetLine(parm->GetLexer()->line_number());
      (yyval.string_node_type) = node; }
#line 1875 "main/parser.cc"
    break;

  case 62: /* quoted_string: tQSTRING  */
#line 430 "main/parser.yy"
    { StringNode* node = new StringNode(parm->GetLexer()->YYString());
      node->SetLine(parm->GetLexer()->line_number());
      (yyval.string_node_type) = node; }
#line 1883 "main/parser.cc"
    break;

  case 63: /* weight: tANGLE_STRING  */
#line 437 "main/parser.yy"
    { StringNode* node = new StringNode(parm->GetLexer()->YYString());
      node->SetLine(parm->GetLexer()->line_number());
      (yyval.string_node_type) = node; }
#line 1891 "main/parser.cc"
    break;

  case 64: /* import_request: tKEYWORD_IMPORT quoted_string tKEYWORD_AS descriptor tSEMICOLON  */
#line 444 "main/parser.yy"
    { ImportNode* node = new ImportNode((yyvsp[-3].string_node_type), (yyvsp[-1].identifier_node_type));
      node->SetLine(parm->GetLexer()->line_number());
      (yyval.import_node_type) = node; }
#line 1899 "main/parser.cc"
    break;

  case 65: /* import_request: error tSEMICOLON  */
#line 448 "main/parser.yy"
    { parm->Error("Invalid import statement.");
      (yyval.import_node_type) = NULL; }
#line 1906 "main/parser.cc"
    break;

  case 66: /* func_decl: tKEYWORD_FUNC descriptor func_arglist func_body  */
#line 454 "main/parser.yy"
    { FunctionNode* node = new FunctionNode((yyvsp[-2].identifier_node_type), (yyvsp[-1].collection_node_type), (yyvsp[0].collection_node_type));
      node->SetLine(node->GetName()->getline());  // Use the identifier's location.
      (yyval.function_node_type) = node; }
#line 1914 "main/parser.cc"
    break;

  case 67: /* func_decl: error tRBRACE  */
#line 458 "main/parser.yy"
    { parm->Error("Invalid function declaration.");
      (yyval.function_node_type) = NULL; }
#line 1921 "main/parser.cc"
    break;

  case 68: /* func_arglist: tLBRACKET tRBRACKET  */
#line 464 "main/parser.yy"
    { (yyval.collection_node_type) = new CollectionNode(); }
#line 1927 "main/parser.cc"
    break;

  case 69: /* func_arglist: tLBRACKET func_arguments tRBRACKET  */
#line 466 "main/parser.yy"
    { (yyval.collection_node_type) = (yyvsp[-1].collection_node_type); }
#line 1933 "main/parser.cc"
    break;

  case 70: /* func_arguments: descriptor  */
#line 471 "main/parser.yy"
    { CollectionNode* node = new CollectionNode();
      node->AddFront((yyvsp[0].identifier_node_type));
      (yyval.collection_node_type) = node; }
#line 1941 "main/parser.cc"
    break;

  case 71: /* func_arguments: descriptor tCOMMA func_arguments  */
#line 475 "main/parser.yy"
    { (yyvsp[0].collection_node_type)->AddFront((yyvsp[-2].identifier_node_type));
      (yyval.collection_node_type) = (yyvsp[0].collection_node_type); }
#line 1948 "main/parser.cc"
    break;

  case 72: /* func_body: tLBRACE stmt_list tRBRACE  */
#line 481 "main/parser.yy"
    { (yyval.collection_node_type) = (yyvsp[-1].collection_node_type); }
#line 1954 "main/parser.cc"
    break;


#line 1958 "main/parser.cc"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (parm, YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, parm);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, parm);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;


#if !defined yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (parm, YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturn;
#endif


/*-------------------------------------------------------.
| yyreturn -- parsing is finished, clean up and return.  |
`-------------------------------------------------------*/
yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, parm);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, parm);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 484 "main/parser.yy"


// Local definitions, definitions that need the semantics of the
// token/type defines.

// yylex() calls Lexer::YYLex()
int yylex(void *, GrmCompilerParserInterface *parm) {
  switch(parm->GetLexer()->YYLex()) {
    case Lexer::EOS:
      return 0;
    case Lexer::QUOTED_STRING:
      return tQSTRING;
    case Lexer::DOUBLE_QUOTED_STRING:
      return tDQSTRING;
    case Lexer::DESCRIPTOR:
      return tDESCR;
    case Lexer::INTEGER:
      return tINTEGER;
    case Lexer::FLOAT:
      return tFLOAT;
    case Lexer::ANGLE_STRING:
      return tANGLE_STRING;
    case Lexer::CONNECTOR: {
      std::string connector = parm->GetLexer()->YYString();
      if (connector.length() != 1) {
        parm->Error(StringPrintf("Parse error - unknown connector: %s", connector.c_str()));
        return 0;
      }
      switch (parm->GetLexer()->YYString()[0]) {
        case '$': return tDOLLAR;
        case '(': return tLPAREN;
        case ')': return tRPAREN;
        case '*': return tSTAR;
        case '+': return tPLUS;
        case ',': return tCOMMA;
        case '-': return tMINUS;
        case '.': return tDOT;
        case '/': return tSLASH;
        case ':': return tCOLON;
        case ';': return tSEMICOLON;
        case '=': return tEQUALS;
        case '?': return tQMARK;
        case '@': return tAT;
        case '[': return tLBRACKET;
        case ']': return tRBRACKET;
        case '_': return tUNDERSCORE;
        case '{': return tLBRACE;
        case '}': return tRBRACE;
        case '|': return tPIPE;
        default:  parm->Error(StringPrintf("Parse error - unknown connector: %s", connector.c_str()));
                  return 0;
      }
    }
    case Lexer::KEYWORD: {
      std::string keyword = parm->GetLexer()->YYString();
      if (keyword == "export") {
        return tKEYWORD_EXPORT;
      } else if (keyword == "as") {
        return tKEYWORD_AS;
      } else if (keyword == "func") {
        return tKEYWORD_FUNC;
      } else if (keyword == "import") {
        return tKEYWORD_IMPORT;
      } else if (keyword == "return") {
        return tKEYWORD_RETURN;
      } else if (keyword == "byte") {
        return tKEYWORD_BYTE;
      } else if (keyword == "utf8") {
        return tKEYWORD_UTF8;
      } else {
        parm->Error(StringPrintf("Parse error - unknown keyword: %s", keyword.c_str()));
        return 0;
      }
    }
  }
  return 0;
}

int yyerror(GrmCompilerParserInterface *, const char *s) {
  std::cout << "Parse Failed: " << s << std::endl;
  return 0;
}

} // namespace thrax_rewriter

namespace thrax {

int CallParser(GrmCompilerParserInterface* compiler) {
  return thrax_rewriter::yyparse(compiler);
}

}  // namespace thrax
