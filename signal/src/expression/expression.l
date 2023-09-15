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
 * Univerzitni 8, 301 00 Pilsen
 * Czech Republic
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
 * a) This file is available under the Apache License, Version 2.0.
 * b) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

 /* flex expression.l */

%option 8bit reentrant bison-bridge
%option warn noyywrap nodefault
%option nounistd

%option extra-type="TGlobal_Ast_Data*"

%{
#include <stdio.h>
#include <stdbool.h>

#ifdef _WIN32
  int isatty(int n) {return 0;}
  #define fileno _fileno
#endif


#define YY_DECL int yylex(YYSTYPE * yylval_param , yyscan_t yyscanner)


#include "expression.tab.hpp"
  #include "expression.h"
%}

%%

[ \t]	; // ignore all whitespace
[0-9]+\.[0-9]+ 	{ yylval->ast_node = new expression::CDouble(yytext); return T_DOUBLE; }    
[0-9]+		{ yylval->ast_node = new expression::CDouble(yytext); return T_DOUBLE; }
"level"		{yylval->ast_node = new expression::CVariable_level(); return T_DOUBLE;}
"is_level_event"		{yylval->ast_node = new expression::CVariable_is_level_event(); return T_BOOL;}
"is_info_event"		{yylval->ast_node = new expression::CVariable_is_info_event(); return T_BOOL;}
"is_control_event"		{yylval->ast_node = new expression::CVariable_is_control_event(); return T_BOOL;}
"true"		{yylval->ast_node = new expression::CConstant<bool>(true); return T_BOOL;}
"false"		{yylval->ast_node = new expression::CConstant<bool>(false); return T_BOOL;}
"+"		{return T_PLUS;}
"-"		{return T_MINUS;}
"*"		{return T_MULTIPLY;}
"/"		{return T_DIVIDE;}
"("		{return T_LEFT;}
")"		{return T_RIGHT;}
"!"		{return T_NOT;}
"<="		{return T_LTEQ;}
"<"		{return T_LT;}
"=="		{return T_EQ;}
"!="		{return T_NEQ;}
">="		{return T_GTEQ;}
">"		{return T_GT;}
"&&"		{return T_AND;}
"||"		{return T_OR;}
"^"		{return T_XOR;}
\n  {return T_ERROR;}
. {std::string err_msg {dsInvalid_expression_character}; err_msg += yytext; yyerror(yyscanner, err_msg.c_str()); return T_ERROR;}

%%