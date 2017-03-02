/* A Bison parser, made by GNU Bison 2.7.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2012 Free Software Foundation, Inc.
   
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

#ifndef YY_CUOBJDUMP_CUOBJDUMP_PARSER_H_INCLUDED
# define YY_CUOBJDUMP_CUOBJDUMP_PARSER_H_INCLUDED
/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int cuobjdump_debug;
#endif

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     H_SEPARATOR = 258,
     H_ARCH = 259,
     H_CODEVERSION = 260,
     H_PRODUCER = 261,
     H_HOST = 262,
     H_COMPILESIZE = 263,
     H_IDENTIFIER = 264,
     CODEVERSION = 265,
     STRING = 266,
     FILENAME = 267,
     DECIMAL = 268,
     PTXHEADER = 269,
     ELFHEADER = 270,
     PTXLINE = 271,
     ELFLINE = 272,
     SASSLINE = 273,
     IDENTIFIER = 274,
     NEWLINE = 275
   };
#endif


#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
/* Line 2058 of yacc.c  */
#line 47 "cuobjdump.y"

	char* string_value;


/* Line 2058 of yacc.c  */
#line 82 "cuobjdump_parser.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE cuobjdump_lval;

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int cuobjdump_parse (void *YYPARSE_PARAM);
#else
int cuobjdump_parse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int cuobjdump_parse (void);
#else
int cuobjdump_parse ();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !YY_CUOBJDUMP_CUOBJDUMP_PARSER_H_INCLUDED  */
