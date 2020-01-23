/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Copyright (c) since 2018 University of West Bohemia.
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Univerzitni 8
 * 301 00, Pilsen
 * 
 * 
 * Purpose of this software:
 * This software is intended to demonstrate work of the diabetes.zcu.cz research
 * group to other scientists, to complement our published papers. It is strictly
 * prohibited to use this software for diagnosis or treatment of any medical condition,
 * without obtaining all required approvals from respective regulatory bodies.
 *
 * Especially, a diabetic patient is warned that unauthorized use of this software
 * may result into severe injure, including death.
 *
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under these license terms is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *      GPLv3 license.
 * b) For any other use, especially commercial use, you must contact us and
 *       obtain specific terms and conditions for the use of the software.
 * c) When publishing work with results obtained using this software, you agree to cite the following paper:
 *       Tomas Koutny and Martin Ubl, "Parallel software architecture for the next generation of glucose
 *       monitoring", Procedia Computer Science, Volume 141C, pp. 279-286, 2018
 */

 /* bison -o expression.tab.cpp -d expression.y */


 %code requires {
      #include "expression.h"    
}


%{

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "expression.tab.hpp"
#include "../../../../common/utils/DebugHelper.h"
#include "../../../../common/utils/string_utils.h"

#define YY_EXTRA_TYPE  expression::CAST_Node**
YY_EXTRA_TYPE  yyget_extra ( void* scanner );

int yylex(YYSTYPE * yylval_param , void* yyscanner);
void yyerror(void* scanner, char const *msg);
  
  
  
#define DUnary_Operator(name, op) new expression::name{op};
#define DBinary_Operator(name, op1, op2) new expression::name{op1, op2};

%}

%define api.pure full
%lex-param {void* scanner}
%parse-param {void* scanner}


%union {
	expression::CAST_Node * ast_node;
} ;

/*%token<ast_node> T_ASTNODE*/
%token<ast_node> T_DOUBLE
%token<ast_node> T_BOOL                                                                     
%token T_PLUS T_MINUS T_MULTIPLY T_DIVIDE T_LEFT T_RIGHT T_LT T_LTEQ T_EQ T_NEQ T_GT T_GTEQ T_AND T_OR T_XOR T_NOT
%left T_AND T_XOR T_OR
%left T_EQ T_NEQ  
%left T_LT T_LTEQ T_GT T_GTEQ
%left T_PLUS T_MINUS
%left T_MULTIPLY T_DIVIDE
%left T_NOT

%type<ast_node> bool_expression
%type<ast_node> expression
%destructor {delete $$;} <ast_node>

%start calculation

%%

calculation:
	   | bool_expression { *(yyget_extra(scanner)) = $1; }
;


expression: T_DOUBLE				{ $$ = $1; }
	  | expression T_PLUS expression	{ $$ = DBinary_Operator(CPlus, $1, $3) }
	  | expression T_MINUS expression	{ $$ = DBinary_Operator(CMinus, $1, $3) }
	  | expression T_MULTIPLY expression	{ $$ = DBinary_Operator(CMul, $1, $3) }
	  | expression T_DIVIDE expression	{ $$ = DBinary_Operator(CDiv, $1, $3) }
	  | T_LEFT expression T_RIGHT		{ $$ = $2; }
;
          
bool_expression: T_BOOL 			{$$ = $1; }
	    | T_LEFT bool_expression T_RIGHT	{ $$ = $2; }
  	  | expression T_LT expression		{ $$ = DBinary_Operator(CLT, $1, $3) }
  	  | expression T_LTEQ expression	{ $$ = DBinary_Operator(CLTEQ, $1, $3) }
  	  | expression T_EQ expression		{ $$ = DBinary_Operator(CEQ, $1, $3) }
  	  | expression T_NEQ expression		{ $$ = DBinary_Operator(CNEQ, $1, $3) }
  	  | expression T_GT expression		{ $$ = DBinary_Operator(CGT, $1, $3) }
  	  | expression T_GTEQ expression	{ $$ = DBinary_Operator(CGTEQ, $1, $3) }  
  	  | bool_expression T_AND bool_expression { $$ = DBinary_Operator(CAND, $1, $3) }
  	  | bool_expression T_OR bool_expression { $$ = DBinary_Operator(COR, $1, $3) }
      | bool_expression T_XOR bool_expression { $$ = DBinary_Operator(CXOR, $1, $3) }
      | T_NOT bool_expression  	{ $$ = DUnary_Operator(CNot, $2) } 
;



%%

#include "lex.yy.c"

CExpression Parse_AST_Tree(const std::wstring& wstr) {
  expression::CAST_Node *ast_tree = nullptr; 

  yyscan_t scanner; 
  yylex_init(&scanner);

  yylex_init_extra(&ast_tree, &scanner );

  const std::string src = Narrow_WString(wstr);	
  YY_BUFFER_STATE buffer = yy_scan_string(src.c_str(), scanner);
  const auto rc = yyparse(scanner);
  yy_delete_buffer(buffer, scanner);
  
  
  yylex_destroy(scanner);
  
  return rc == 0 ? CExpression{ast_tree} : nullptr;
}

void yyerror(void* scanner, char const *msg) {
    dprintf("Parse error: ");
    dprintf(msg);
    dprintf("\n");
}