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


%{

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "expression.tab.h"

typedef void * yyscan_t;

extern int yylex(YYSTYPE * yylval_param , yyscan_t yyscanner);
extern int yyparse( yyscan_t yyscanner);


int yylex_init (yyscan_t* scanner);
int yylex_destroy ( yyscan_t yyscanner );

typedef void * YY_BUFFER_STATE;
YY_BUFFER_STATE yy_scan_string ( const char *yy_str , yyscan_t yyscanner );
void yy_delete_buffer (YY_BUFFER_STATE  b , yyscan_t yyscanner);

void yyerror(yyscan_t scanner, char const *msg);
%}

%define api.pure full
%lex-param {void* scanner}
%parse-param {void* scanner}


%union {
	double dval;
	bool bval;
} ;

%token<bval> T_BOOL
%token<dval> T_DOUBLE
%token T_PLUS T_MINUS T_MULTIPLY T_DIVIDE T_LEFT T_RIGHT T_LT T_LTEQ T_EQ T_NEQ T_GT T_GTEQ T_AND T_OR
%left T_PLUS T_MINUS
%left T_MULTIPLY T_DIVIDE
%left T_AND T_OR

%type<bval> bool_expression
%type<dval> expression

%start calculation

%%

calculation:
	   | bool_expression { printf("\tResult: %i\n", $1); }
;


expression: T_DOUBLE				{ $$ = $1; }
	  | expression T_PLUS expression	{ $$ = $1 + $3; }
	  | expression T_MINUS expression	{ $$ = $1 - $3; }
	  | expression T_MULTIPLY expression	{ $$ = $1 * $3; }
	  | expression T_DIVIDE expression	{ $$ = $1 / $3; }
	  | T_LEFT expression T_RIGHT		{ $$ = $2; }
;

bool_expression: T_BOOL 			{$$ = $1; }
	    | T_LEFT bool_expression T_RIGHT	{ $$ = $2; }
  	  | bool_expression T_AND bool_expression { $$ = $1 && $3; }
  	  | bool_expression T_OR bool_expression { $$ = $1 || $3; }
  	  | expression T_LT expression		{ $$ = $1 < $3; }
  	  | expression T_LTEQ expression	{ $$ = $1 <= $3; }
  	  | expression T_EQ expression		{ $$ = $1 == $3; }
  	  | expression T_NEQ expression		{ $$ = $1 != $3; }
  	  | expression T_GT expression		{ $$ = $1 > $3; }
  	  | expression T_GTEQ expression	{ $$ = $1 >= $3; }
;


%%

int main() {
  yyscan_t scanner; 
  yylex_init(&scanner);

	const char* test = "4+3<8";
  YY_BUFFER_STATE buffer = yy_scan_string(test, scanner);
  yyparse(scanner);
   yy_delete_buffer(buffer, scanner);
  
  
  yylex_destroy(scanner);
  
	return 0;
}

void yyerror(yyscan_t scanner, char const *msg) {
	fprintf(stderr, "Parse error: %s\n", msg);
	exit(1);
}