/* A Bison parser, made by GNU Bison 3.5.0.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2019 Free Software Foundation,
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

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

#ifndef YY_YY_EXPRESSION_TAB_HPP_INCLUDED
# define YY_YY_EXPRESSION_TAB_HPP_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
#line 42 "expression.y"

      #include "expression.h"    

#line 52 "expression.tab.hpp"

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    T_DOUBLE = 258,
    T_BOOL = 259,
    T_PLUS = 260,
    T_MINUS = 261,
    T_MULTIPLY = 262,
    T_DIVIDE = 263,
    T_LEFT = 264,
    T_RIGHT = 265,
    T_LT = 266,
    T_LTEQ = 267,
    T_EQ = 268,
    T_NEQ = 269,
    T_GT = 270,
    T_GTEQ = 271,
    T_AND = 272,
    T_OR = 273,
    T_XOR = 274,
    T_NOT = 275
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 80 "expression.y"

	expression::CAST_Node * ast_node;

#line 88 "expression.tab.hpp"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int yyparse (void* scanner);

#endif /* !YY_YY_EXPRESSION_TAB_HPP_INCLUDED  */
