
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
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

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 1 "parser.yy"

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Copyright 2005-2011 Google, Inc.
// All Rights Reserved.
//
// Author: wojciech@google.com (Wojciech Skut)
//         ttai@google.com (Terry Tai)
//
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

using namespace thrax;

#define YYPARSE_PARAM parm
#define YYLEX_PARAM parm
#define CTRL (static_cast<GrmCompiler*>(parm))

namespace thrax_rewriter {
  int yylex(void *, void *parm);
  int yyerror(const char *);


/* Line 189 of yacc.c  */
#line 123 "parser.cc"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     tDESCR = 258,
     tDQSTRING = 259,
     tQSTRING = 260,
     tINTEGER = 261,
     tFLOAT = 262,
     tLPAREN = 263,
     tRPAREN = 264,
     tLBRACE = 265,
     tRBRACE = 266,
     tPIPE = 267,
     tSTAR = 268,
     tPLUS = 269,
     tQMARK = 270,
     tDOT = 271,
     tCOMMA = 272,
     tAT = 273,
     tCOLON = 274,
     tSEMICOLON = 275,
     tEQUALS = 276,
     tCONCAT = 277,
     tUNARY = 278,
     tDOLLAR = 279,
     tLBRACKET = 280,
     tRBRACKET = 281,
     tANGLE_STRING = 282,
     tSLASH = 283,
     tUNDERSCORE = 284,
     tMINUS = 285,
     tREAD_STRINGS = 286,
     tREAD_WEIGHTED_STRINGS = 287,
     tCASE_INSENSITIVE = 288,
     tKEYWORD_EXPORT = 289,
     tKEYWORD_FUNC = 290,
     tKEYWORD_RETURN = 291,
     tKEYWORD_BYTE = 292,
     tKEYWORD_UTF8 = 293,
     tKEYWORD_AS = 294,
     tKEYWORD_IMPORT = 295
   };
#endif
/* Tokens.  */
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




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 52 "parser.yy"

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



/* Line 214 of yacc.c  */
#line 256 "parser.cc"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 268 "parser.cc"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

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
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
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
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  9
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   113

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  41
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  34
/* YYNRULES -- Number of rules.  */
#define YYNRULES  66
/* YYNRULES -- Number of states.  */
#define YYNSTATES  108

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   295

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
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
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint8 yyprhs[] =
{
       0,     0,     3,     7,     9,    10,    13,    14,    17,    18,
      21,    23,    25,    28,    32,    37,    43,    45,    47,    49,
      51,    53,    55,    57,    59,    62,    64,    68,    70,    74,
      76,    80,    83,    86,    90,    92,    96,    99,   102,   105,
     112,   117,   119,   123,   125,   127,   129,   133,   137,   141,
     145,   149,   151,   154,   156,   158,   160,   162,   164,   170,
     173,   178,   181,   184,   188,   190,   194
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      42,     0,    -1,    43,    44,    45,    -1,     1,    -1,    -1,
      70,    43,    -1,    -1,    71,    44,    -1,    -1,    46,    45,
      -1,    48,    -1,    47,    -1,     1,    20,    -1,    36,    52,
      20,    -1,    50,    21,    49,    20,    -1,    34,    50,    21,
      49,    20,    -1,    52,    -1,     3,    -1,    68,    -1,    63,
      -1,    62,    -1,    57,    -1,    56,    -1,    65,    -1,    60,
      53,    -1,    60,    -1,    61,    12,    54,    -1,    61,    -1,
      55,    30,    53,    -1,    53,    -1,     8,    52,     9,    -1,
      50,    58,    -1,    25,    26,    -1,    25,    59,    26,    -1,
      52,    -1,    52,    17,    59,    -1,    51,    13,    -1,    51,
      14,    -1,    51,    15,    -1,    51,    10,    66,    17,    66,
      11,    -1,    51,    10,    66,    11,    -1,    51,    -1,    61,
      18,    55,    -1,    55,    -1,    50,    -1,    67,    -1,    67,
      16,    37,    -1,    67,    16,    38,    -1,    67,    16,    62,
      -1,    67,    16,    57,    -1,    54,    19,    54,    -1,    54,
      -1,    64,    69,    -1,    64,    -1,     6,    -1,     4,    -1,
       5,    -1,    27,    -1,    40,    68,    39,    50,    20,    -1,
       1,    20,    -1,    35,    50,    72,    74,    -1,     1,    11,
      -1,    25,    26,    -1,    25,    73,    26,    -1,    50,    -1,
      50,    17,    73,    -1,    10,    45,    11,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   124,   124,   128,   135,   136,   143,   144,   151,   152,
     158,   163,   168,   174,   180,   183,   189,   194,   204,   205,
     206,   207,   208,   212,   217,   223,   229,   235,   241,   247,
     253,   259,   267,   269,   274,   278,   285,   289,   293,   297,
     308,   315,   321,   327,   332,   340,   345,   350,   355,   361,
     370,   376,   381,   387,   394,   399,   406,   413,   420,   424,
     430,   434,   440,   442,   447,   451,   457
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "tDESCR", "tDQSTRING", "tQSTRING",
  "tINTEGER", "tFLOAT", "tLPAREN", "tRPAREN", "tLBRACE", "tRBRACE",
  "tPIPE", "tSTAR", "tPLUS", "tQMARK", "tDOT", "tCOMMA", "tAT", "tCOLON",
  "tSEMICOLON", "tEQUALS", "tCONCAT", "tUNARY", "tDOLLAR", "tLBRACKET",
  "tRBRACKET", "tANGLE_STRING", "tSLASH", "tUNDERSCORE", "tMINUS",
  "tREAD_STRINGS", "tREAD_WEIGHTED_STRINGS", "tCASE_INSENSITIVE",
  "tKEYWORD_EXPORT", "tKEYWORD_FUNC", "tKEYWORD_RETURN", "tKEYWORD_BYTE",
  "tKEYWORD_UTF8", "tKEYWORD_AS", "tKEYWORD_IMPORT", "$accept", "grammar",
  "import_list", "func_list", "stmt_list", "stmt", "return_stmt",
  "rule_stmt", "rule_body", "descriptor", "atomic_obj", "obj",
  "concat_fst", "union_fst", "difference_fst", "grouped_obj",
  "funccall_obj", "funccall_arglist", "funccall_arguments",
  "repetition_fst", "composition_fst", "identifier_obj", "string_fst",
  "fst_with_output", "fst_with_weight", "number", "quoted_fst_string",
  "quoted_string", "weight", "import_request", "func_decl", "func_arglist",
  "func_arguments", "func_body", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    41,    42,    42,    43,    43,    44,    44,    45,    45,
      46,    46,    46,    47,    48,    48,    49,    50,    51,    51,
      51,    51,    51,    52,    53,    53,    54,    54,    55,    55,
      56,    57,    58,    58,    59,    59,    60,    60,    60,    60,
      60,    60,    61,    61,    62,    63,    63,    63,    63,    63,
      64,    64,    65,    65,    66,    67,    68,    69,    70,    70,
      71,    71,    72,    72,    73,    73,    74
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     3,     1,     0,     2,     0,     2,     0,     2,
       1,     1,     2,     3,     4,     5,     1,     1,     1,     1,
       1,     1,     1,     1,     2,     1,     3,     1,     3,     1,
       3,     2,     2,     3,     1,     3,     2,     2,     2,     6,
       4,     1,     3,     1,     1,     1,     3,     3,     3,     3,
       3,     1,     2,     1,     1,     1,     1,     1,     5,     2,
       4,     2,     2,     3,     1,     3,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     3,     0,     0,     0,     0,    59,    56,     0,     1,
       0,     0,     0,     0,     0,     5,     0,    61,    17,     0,
       0,     0,     0,     2,     0,    11,    10,     0,     7,     0,
       0,     0,    12,     0,    55,     0,    44,    41,     0,    29,
      51,    43,    22,    21,    25,    27,    20,    19,    53,    23,
      45,    18,     9,     0,    58,    62,    64,     0,     0,    60,
       0,     0,     0,    31,     0,    36,    37,    38,    13,     0,
       0,    24,     0,     0,    57,    52,     0,     0,    16,     0,
      63,     0,     0,    30,    32,    34,     0,    54,     0,    50,
      28,    26,    42,    46,    47,    49,    48,    14,    65,    66,
      15,     0,    33,    40,     0,    35,     0,    39
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     3,     4,    12,    23,    24,    25,    26,    77,    36,
      37,    78,    39,    40,    41,    42,    43,    63,    86,    44,
      45,    46,    47,    48,    49,    88,    50,    51,    75,     5,
      13,    31,    57,    59
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -49
static const yytype_int8 yypact[] =
{
       7,   -17,    12,    19,    22,    15,   -49,   -49,    -7,   -49,
      23,    33,    30,    22,   -17,   -49,    33,   -49,   -49,    20,
      24,    33,    77,   -49,    26,   -49,   -49,    31,   -49,    34,
       2,    55,   -49,    46,   -49,    77,    43,    73,    58,   -49,
      56,    54,   -49,   -49,    77,    41,   -49,   -49,    62,   -49,
      74,   -49,   -49,    77,   -49,   -49,    75,    65,    60,   -49,
      77,    84,    69,   -49,    91,   -49,   -49,   -49,   -49,    77,
      77,   -49,    77,    77,   -49,   -49,     1,    78,   -49,    33,
     -49,    88,    80,   -49,   -49,    85,    79,   -49,    59,   -49,
     -49,   -49,    54,   -49,   -49,   -49,   -49,   -49,   -49,   -49,
     -49,    77,   -49,   -49,    91,   -49,    90,   -49
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -49,   -49,    98,    93,   -12,   -49,   -49,   -49,    44,   -10,
     -49,   -22,   -35,   -48,    35,   -49,    36,   -49,     6,   -49,
     -49,    37,   -49,   -49,   -49,     5,   -49,   108,   -49,   -49,
     -49,   -49,    32,   -49
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -9
static const yytype_int8 yytable[] =
{
      38,    19,    27,     6,    18,    18,    29,    -4,     1,    71,
      -4,    33,    52,    61,    27,    -4,    14,     7,    -4,     9,
      56,    89,    -6,    10,    91,    -6,    -8,    20,    55,    18,
      -8,    20,    16,    18,    17,    90,    18,    -8,    93,    94,
      85,    -4,    -4,    -4,    32,    30,    81,     2,    27,    -4,
      -4,    -4,    53,    72,    54,     2,    -6,    11,    -6,    73,
      21,    20,    22,    18,    21,    58,    22,    60,    62,    56,
     103,    -8,    18,    34,     7,    69,   104,    35,    68,    85,
      18,    34,     7,    64,    70,    35,    65,    66,    67,    74,
      76,    80,    79,    83,    21,    84,    22,    87,    97,    99,
     100,   107,   101,    15,    82,   102,    28,   105,    92,   106,
       8,    98,    95,    96
};

static const yytype_uint8 yycheck[] =
{
      22,    11,    12,    20,     3,     3,    16,     0,     1,    44,
       3,    21,    24,    35,    24,     0,     1,     5,     3,     0,
      30,    69,     0,     1,    72,     3,     0,     1,    26,     3,
       0,     1,    39,     3,    11,    70,     3,    11,    37,    38,
      62,    34,    35,    36,    20,    25,    58,    40,    58,    34,
      35,    36,    21,    12,    20,    40,    34,    35,    36,    18,
      34,     1,    36,     3,    34,    10,    36,    21,    25,    79,
      11,    11,     3,     4,     5,    19,    17,     8,    20,   101,
       3,     4,     5,    10,    30,     8,    13,    14,    15,    27,
      16,    26,    17,     9,    34,    26,    36,     6,    20,    11,
      20,    11,    17,     5,    60,    26,    13,   101,    73,   104,
       2,    79,    76,    76
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     1,    40,    42,    43,    70,    20,     5,    68,     0,
       1,    35,    44,    71,     1,    43,    39,    11,     3,    50,
       1,    34,    36,    45,    46,    47,    48,    50,    44,    50,
      25,    72,    20,    50,     4,     8,    50,    51,    52,    53,
      54,    55,    56,    57,    60,    61,    62,    63,    64,    65,
      67,    68,    45,    21,    20,    26,    50,    73,    10,    74,
      21,    52,    25,    58,    10,    13,    14,    15,    20,    19,
      30,    53,    12,    18,    27,    69,    16,    49,    52,    17,
      26,    45,    49,     9,    26,    52,    59,     6,    66,    54,
      53,    54,    55,    37,    38,    57,    62,    20,    73,    11,
      20,    17,    26,    11,    17,    59,    66,    11
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
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



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */





/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

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
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
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
      if (yyn == 0 || yyn == YYTABLE_NINF)
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

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

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
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

/* Line 1455 of yacc.c  */
#line 125 "parser.yy"
    { GrammarNode* node = new GrammarNode((yyvsp[(1) - (3)].collection_node_type), (yyvsp[(2) - (3)].collection_node_type), (yyvsp[(3) - (3)].collection_node_type));
      (yyval.grammar_node_type) = node;
      CTRL->SetAst(static_cast<Node*>((yyval.grammar_node_type))); }
    break;

  case 3:

/* Line 1455 of yacc.c  */
#line 129 "parser.yy"
    { CTRL->Error("Generic parsing error.");
      (yyval.grammar_node_type) = NULL; }
    break;

  case 4:

/* Line 1455 of yacc.c  */
#line 135 "parser.yy"
    { (yyval.collection_node_type) = new CollectionNode(); }
    break;

  case 5:

/* Line 1455 of yacc.c  */
#line 137 "parser.yy"
    { (yyvsp[(2) - (2)].collection_node_type)->AddFront((yyvsp[(1) - (2)].import_node_type));
      (yyval.collection_node_type) = (yyvsp[(2) - (2)].collection_node_type); }
    break;

  case 6:

/* Line 1455 of yacc.c  */
#line 143 "parser.yy"
    { (yyval.collection_node_type) = new CollectionNode(); }
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 145 "parser.yy"
    { (yyvsp[(2) - (2)].collection_node_type)->AddFront((yyvsp[(1) - (2)].function_node_type));
      (yyval.collection_node_type) = (yyvsp[(2) - (2)].collection_node_type); }
    break;

  case 8:

/* Line 1455 of yacc.c  */
#line 151 "parser.yy"
    { (yyval.collection_node_type) = new CollectionNode(); }
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 153 "parser.yy"
    { (yyvsp[(2) - (2)].collection_node_type)->AddFront((yyvsp[(1) - (2)].statement_node_type));
      (yyval.collection_node_type) = (yyvsp[(2) - (2)].collection_node_type); }
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 159 "parser.yy"
    { StatementNode* node = new StatementNode(StatementNode::RULE_STATEMENTNODE);
      node->SetLine(CTRL->GetLexer()->line_number());
      node->Set((yyvsp[(1) - (1)].rule_node_type));
      (yyval.statement_node_type) = node; }
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 164 "parser.yy"
    { StatementNode* node = new StatementNode(StatementNode::RETURN_STATEMENTNODE);
      node->SetLine(CTRL->GetLexer()->line_number());
      node->Set((yyvsp[(1) - (1)].return_node_type));
      (yyval.statement_node_type) = node; }
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 169 "parser.yy"
    { CTRL->Error("Invalid statement (or previous statement).");
      (yyval.statement_node_type) = NULL; }
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 175 "parser.yy"
    { ReturnNode* node = new ReturnNode((yyvsp[(2) - (3)].node_type));
      (yyval.return_node_type) = node; }
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 181 "parser.yy"
    { RuleNode* node = new RuleNode((yyvsp[(1) - (4)].identifier_node_type), (yyvsp[(3) - (4)].node_type), RuleNode::DO_NOT_EXPORT);
      (yyval.rule_node_type) = node; }
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 184 "parser.yy"
    { RuleNode* node = new RuleNode((yyvsp[(2) - (5)].identifier_node_type), (yyvsp[(4) - (5)].node_type), RuleNode::EXPORT);
      (yyval.rule_node_type) = node; }
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 190 "parser.yy"
    { (yyval.node_type) = (yyvsp[(1) - (1)].node_type); }
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 195 "parser.yy"
    { const string& name = CTRL->GetLexer()->YYString();
      IdentifierNode* node = new IdentifierNode(name);
      node->SetLine(CTRL->GetLexer()->line_number());
      if (!node->IsValid())
        CTRL->Error(StringPrintf("Illegal identifier: %s", name.c_str()));
      (yyval.identifier_node_type) = node; }
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 204 "parser.yy"
    { (yyval.node_type) = (yyvsp[(1) - (1)].string_node_type); }
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 205 "parser.yy"
    { (yyval.node_type) = (yyvsp[(1) - (1)].node_type); }
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 206 "parser.yy"
    { (yyval.node_type) = (yyvsp[(1) - (1)].node_type); }
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 207 "parser.yy"
    { (yyval.node_type) = (yyvsp[(1) - (1)].node_type); }
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 208 "parser.yy"
    { (yyval.node_type) = (yyvsp[(1) - (1)].node_type); }
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 212 "parser.yy"
    { (yyval.node_type) = (yyvsp[(1) - (1)].node_type); }
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 218 "parser.yy"
    { FstNode* node = new FstNode(FstNode::CONCAT_FSTNODE);
      node->SetLine(CTRL->GetLexer()->line_number());
      node->AddArgument((yyvsp[(1) - (2)].node_type));
      node->AddArgument((yyvsp[(2) - (2)].node_type));
      (yyval.node_type) = node; }
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 224 "parser.yy"
    { (yyval.node_type) = (yyvsp[(1) - (1)].node_type); }
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 230 "parser.yy"
    { FstNode* node = new FstNode(FstNode::UNION_FSTNODE);
      node->SetLine(CTRL->GetLexer()->line_number());
      node->AddArgument((yyvsp[(1) - (3)].node_type));
      node->AddArgument((yyvsp[(3) - (3)].node_type));
      (yyval.node_type) = node; }
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 236 "parser.yy"
    { (yyval.node_type) = (yyvsp[(1) - (1)].node_type); }
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 242 "parser.yy"
    { FstNode* node = new FstNode(FstNode::DIFFERENCE_FSTNODE);
      node->SetLine(CTRL->GetLexer()->line_number());
      node->AddArgument((yyvsp[(1) - (3)].node_type));
      node->AddArgument((yyvsp[(3) - (3)].node_type));
      (yyval.node_type) = node; }
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 248 "parser.yy"
    { (yyval.node_type) = (yyvsp[(1) - (1)].node_type); }
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 254 "parser.yy"
    { (yyval.node_type) = (yyvsp[(2) - (3)].node_type); }
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 260 "parser.yy"
    { FstNode* node = new FstNode(FstNode::FUNCTION_FSTNODE);
      node->SetLine(CTRL->GetLexer()->line_number());
      node->AddArgument((yyvsp[(1) - (2)].identifier_node_type));
      node->AddArgument((yyvsp[(2) - (2)].collection_node_type));
      (yyval.node_type) = node; }
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 268 "parser.yy"
    { (yyval.collection_node_type) = new CollectionNode(); }
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 270 "parser.yy"
    { (yyval.collection_node_type) = (yyvsp[(2) - (3)].collection_node_type); }
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 275 "parser.yy"
    { CollectionNode* node = new CollectionNode();
      node->AddFront((yyvsp[(1) - (1)].node_type));
      (yyval.collection_node_type) = node; }
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 279 "parser.yy"
    { (yyvsp[(3) - (3)].collection_node_type)->AddFront((yyvsp[(1) - (3)].node_type));
      (yyval.collection_node_type) = (yyvsp[(3) - (3)].collection_node_type); }
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 286 "parser.yy"
    { RepetitionFstNode* node = new RepetitionFstNode(RepetitionFstNode::STAR);
      node->AddArgument((yyvsp[(1) - (2)].node_type));
      (yyval.node_type) = node; }
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 290 "parser.yy"
    { RepetitionFstNode* node = new RepetitionFstNode(RepetitionFstNode::PLUS);
      node->AddArgument((yyvsp[(1) - (2)].node_type));
      (yyval.node_type) = node; }
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 294 "parser.yy"
    { RepetitionFstNode* node = new RepetitionFstNode(RepetitionFstNode::QUESTION);
      node->AddArgument((yyvsp[(1) - (2)].node_type));
      (yyval.node_type) = node; }
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 298 "parser.yy"
    { if ((yyvsp[(3) - (6)].int_type) > (yyvsp[(5) - (6)].int_type))
        CTRL->Error(StringPrintf("Reversed repetition bounds: %d > %d", (yyvsp[(3) - (6)].int_type), (yyvsp[(5) - (6)].int_type)));
      if ((yyvsp[(3) - (6)].int_type) < 0)
        CTRL->Error(StringPrintf("Start bound must be non-negative: %d", (yyvsp[(3) - (6)].int_type)));
      if ((yyvsp[(5) - (6)].int_type) < 0)
        CTRL->Error(StringPrintf("End bound must be non-negative: %d", (yyvsp[(5) - (6)].int_type)));
      RepetitionFstNode* node = new RepetitionFstNode(RepetitionFstNode::RANGE);
      node->AddArgument((yyvsp[(1) - (6)].node_type));
      node->SetRange((yyvsp[(3) - (6)].int_type), (yyvsp[(5) - (6)].int_type));
      (yyval.node_type) = node; }
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 309 "parser.yy"
    { if ((yyvsp[(3) - (4)].int_type) < 0)
        CTRL->Error(StringPrintf("Repetition count should be non-negative: %d", (yyvsp[(3) - (4)].int_type)));
      RepetitionFstNode* node = new RepetitionFstNode(RepetitionFstNode::RANGE);
      node->AddArgument((yyvsp[(1) - (4)].node_type));
      node->SetRange((yyvsp[(3) - (4)].int_type), (yyvsp[(3) - (4)].int_type));
      (yyval.node_type) = node; }
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 316 "parser.yy"
    { (yyval.node_type) = (yyvsp[(1) - (1)].node_type); }
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 322 "parser.yy"
    { FstNode* node = new FstNode(FstNode::COMPOSITION_FSTNODE);
      node->SetLine(CTRL->GetLexer()->line_number());
      node->AddArgument((yyvsp[(1) - (3)].node_type));
      node->AddArgument((yyvsp[(3) - (3)].node_type));
      (yyval.node_type) = node; }
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 328 "parser.yy"
    { (yyval.node_type) = (yyvsp[(1) - (1)].node_type); }
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 333 "parser.yy"
    { FstNode* node = new FstNode(FstNode::IDENTIFIER_FSTNODE);
      node->SetLine(CTRL->GetLexer()->line_number());
      node->AddArgument((yyvsp[(1) - (1)].identifier_node_type));
      (yyval.node_type) = node; }
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 341 "parser.yy"
    { StringFstNode* node = new StringFstNode(StringFstNode::BYTE);
      node->AddArgument((yyvsp[(1) - (1)].string_node_type));
      node->SetLine((yyvsp[(1) - (1)].string_node_type)->getline());  // Get the line from the actual text line.
      (yyval.node_type) = node; }
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 346 "parser.yy"
    { StringFstNode* node = new StringFstNode(StringFstNode::BYTE);
      node->AddArgument((yyvsp[(1) - (3)].string_node_type));
      node->SetLine((yyvsp[(1) - (3)].string_node_type)->getline());
      (yyval.node_type) = node; }
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 351 "parser.yy"
    { StringFstNode* node = new StringFstNode(StringFstNode::UTF8);
      node->AddArgument((yyvsp[(1) - (3)].string_node_type));
      node->SetLine((yyvsp[(1) - (3)].string_node_type)->getline());
      (yyval.node_type) = node; }
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 356 "parser.yy"
    { StringFstNode* node = new StringFstNode(StringFstNode::SYMBOL_TABLE);
      node->AddArgument((yyvsp[(1) - (3)].string_node_type));
      node->AddArgument((yyvsp[(3) - (3)].node_type));
      node->SetLine((yyvsp[(1) - (3)].string_node_type)->getline());
      (yyval.node_type) = node; }
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 362 "parser.yy"
    { StringFstNode* node = new StringFstNode(StringFstNode::SYMBOL_TABLE);
      node->AddArgument((yyvsp[(1) - (3)].string_node_type));
      node->AddArgument((yyvsp[(3) - (3)].node_type));
      node->SetLine((yyvsp[(1) - (3)].string_node_type)->getline());
      (yyval.node_type) = node; }
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 371 "parser.yy"
    { FstNode* node = new FstNode(FstNode::REWRITE_FSTNODE);
      node->SetLine(CTRL->GetLexer()->line_number());
      node->AddArgument((yyvsp[(1) - (3)].node_type));
      node->AddArgument((yyvsp[(3) - (3)].node_type));
      (yyval.node_type) = node; }
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 377 "parser.yy"
    { (yyval.node_type) = (yyvsp[(1) - (1)].node_type); }
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 384 "parser.yy"
    { if (!static_cast<FstNode*>((yyvsp[(1) - (2)].node_type))->SetWeight((yyvsp[(2) - (2)].string_node_type)))
        CTRL->Error("Rules may have only one weight.");
      (yyval.node_type) = (yyvsp[(1) - (2)].node_type); }
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 388 "parser.yy"
    { (yyval.node_type) = (yyvsp[(1) - (1)].node_type); }
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 395 "parser.yy"
    { (yyval.int_type) = atoi(CTRL->GetLexer()->YYString().c_str()); }
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 400 "parser.yy"
    { StringNode* node = new StringNode(CTRL->GetLexer()->YYString());
      node->SetLine(CTRL->GetLexer()->line_number());
      (yyval.string_node_type) = node; }
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 407 "parser.yy"
    { StringNode* node = new StringNode(CTRL->GetLexer()->YYString());
      node->SetLine(CTRL->GetLexer()->line_number());
      (yyval.string_node_type) = node; }
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 414 "parser.yy"
    { StringNode* node = new StringNode(CTRL->GetLexer()->YYString());
      node->SetLine(CTRL->GetLexer()->line_number());
      (yyval.string_node_type) = node; }
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 421 "parser.yy"
    { ImportNode* node = new ImportNode((yyvsp[(2) - (5)].string_node_type), (yyvsp[(4) - (5)].identifier_node_type));
      node->SetLine(CTRL->GetLexer()->line_number());
      (yyval.import_node_type) = node; }
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 425 "parser.yy"
    { CTRL->Error("Invalid import statement.");
      (yyval.import_node_type) = NULL; }
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 431 "parser.yy"
    { FunctionNode* node = new FunctionNode((yyvsp[(2) - (4)].identifier_node_type), (yyvsp[(3) - (4)].collection_node_type), (yyvsp[(4) - (4)].collection_node_type));
      node->SetLine(node->GetName()->getline());  // Use the identifier's location.
      (yyval.function_node_type) = node; }
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 435 "parser.yy"
    { CTRL->Error("Invalid function declaration.");
      (yyval.function_node_type) = NULL; }
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 441 "parser.yy"
    { (yyval.collection_node_type) = new CollectionNode(); }
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 443 "parser.yy"
    { (yyval.collection_node_type) = (yyvsp[(2) - (3)].collection_node_type); }
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 448 "parser.yy"
    { CollectionNode* node = new CollectionNode();
      node->AddFront((yyvsp[(1) - (1)].identifier_node_type));
      (yyval.collection_node_type) = node; }
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 452 "parser.yy"
    { (yyvsp[(3) - (3)].collection_node_type)->AddFront((yyvsp[(1) - (3)].identifier_node_type));
      (yyval.collection_node_type) = (yyvsp[(3) - (3)].collection_node_type); }
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 458 "parser.yy"
    { (yyval.collection_node_type) = (yyvsp[(2) - (3)].collection_node_type); }
    break;



/* Line 1455 of yacc.c  */
#line 2127 "parser.cc"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
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
		      yytoken, &yylval);
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

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
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
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
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
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

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

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 461 "parser.yy"


// Local definitions, definitions that need the semantics of the
// token/type defines.

// yylex() calls Lexer::YYLex()
int yylex(void *, void *parm) {
  switch(CTRL->GetLexer()->YYLex()) {
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
      string connector = CTRL->GetLexer()->YYString();
      if (connector.length() != 1) {
        CTRL->Error(StringPrintf("Parse error - unknown connector: %s", connector.c_str()));
        return 0;
      }
      switch (CTRL->GetLexer()->YYString()[0]) {
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
        default:  CTRL->Error(StringPrintf("Parse error - unknown connector: %s", connector.c_str()));
                  return 0;
      }
    }
    case Lexer::KEYWORD: {
      string keyword = CTRL->GetLexer()->YYString();
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
        CTRL->Error(StringPrintf("Parse error - unknown keyword: %s", keyword.c_str()));
        return 0;
      }
    }
  }
  return 0;
}

int yyerror(const char *s) {
  cout << "Parse Failed: " << s << endl;
  return 0;
}

} // namespace thrax_rewriter

DECLARE_string(indir);

namespace thrax {

  bool GrmCompiler::ParseFile(const string &filename) {
    string local_grammar = JoinPath(FLAGS_indir, filename);
    VLOG(1) << "Parsing file: " << local_grammar;

    string contents;
    ReadFileToStringOrDie(local_grammar, &contents);

    return ParseContents(contents);
  }

  bool GrmCompiler::ParseContents(const string& contents) {
    success_ = true;
    lexer_.ScanString(contents);
    thrax_rewriter::yyparse(this);
    return success_;
  }

}  // namespace thrax

