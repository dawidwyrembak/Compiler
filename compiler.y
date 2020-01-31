%{
#include "compiler.hpp"

extern int yylex();
extern int yylineno;
extern FILE *yyin;
int yyerror(const string str);
%}

%define parse.error verbose
%expect 2

%union {
    char* str;
    long long int num;
}


%token <str> _NUMBER
%token <str> _DECLARE _BEGIN _END _IF _ELSE _THEN _ENDIF
%token <str> _FOR _FROM _ENDFOR _WHILE _ENDWHILE _DO _ENDDO
%token <str> _WRITE _READ _IDENTIFIER _TO _DOWNTO _SCOLON _COLON _COMMA
%token <str> _LB _RB _ASSIGN _EQ _NEQ _LE _GE _LEQ _GEQ _PLUS _MINUS _TIMES _DIV _MOD
%type <str> value
%type <str> identifier

%%

program:
    _DECLARE declarations
    _BEGIN commands _END                                                                    {pushCommand("HALT");}
    | _BEGIN commands _END                                                                  {pushCommand("HALT");}
    ;

declarations:
    declarations _COMMA _IDENTIFIER                                                         {declareIdentifier($3, yylineno);}
    | declarations _COMMA _IDENTIFIER _LB _NUMBER _COLON _NUMBER _RB                        {declareArray($3, $5, $7, yylineno);}
    | _IDENTIFIER                                                                           {declareIdentifier($1, yylineno);}
    | _IDENTIFIER _LB _NUMBER _COLON _NUMBER _RB                                            {declareArray($1, $3, $5, yylineno);}
    ;

commands:
    commands command                                                                        ;
    | command                                                                               ;
    ;

command:
    identifier _ASSIGN                                                                      {assignFlagState(true);}
        expression _SCOLON                                                                  {assign($1,  yylineno);}
    | _IF                                                                                   {increaseLoopNumber();}
        condition _THEN commands ifBody
    | _WHILE                                                                                {beginWhile();}
        condition _DO commands _ENDWHILE                                                    {endWhile();}
    | _DO                                                                                   {beginWhile();}
        commands _WHILE condition _ENDDO                                                    {endDo();}
    | _FOR _IDENTIFIER _FROM value _TO value _DO                                            {beginToFor($2, $4, $6, yylineno);}
        commands _ENDFOR                                                                    {endToFor();}
    | _FOR _IDENTIFIER _FROM value _DOWNTO value _DO                                        {beginDownFor($2, $4, $6, yylineno);}
        commands _ENDFOR                                                                    {endDownFor();}
    | _READ identifier  _SCOLON                                                             {read($2, yylineno);}
    | _WRITE                                                                                {writeFlagState(true);}
        value _SCOLON                                                                       {write($3, yylineno);}
    ;

ifBody:
    _ELSE                                                                                   {ifElse();}
     commands _ENDIF                                                                        {ifElseEnd();}
     |   _ENDIF                                                                             {ifEnd();}
     ;

expression:
    value                                                                                   {expressionValue($1, yylineno);}
    | value _PLUS value                                                                     {addition($1, $3, yylineno);}
    | value _MINUS value                                                                    {subtraction($1, $3, false, yylineno);}
    | value _TIMES value                                                                    {multiplication($1, $3, yylineno);}
    | value _DIV value                                                                      {division($1, $3, yylineno);}
    | value _MOD value                                                                      {modulo($1, $3, yylineno);}
    ;

condition:
    value _EQ value                                                                         {eq($1, $3, yylineno);}
    | value _NEQ value                                                                      {neq($1, $3, yylineno);}
    | value _LE value                                                                       {le($1, $3, yylineno);}
    | value _GE value                                                                       {ge($1, $3, yylineno);}
    | value _LEQ value                                                                      {leq($1, $3, yylineno);}
    | value _GEQ value                                                                      {geq($1, $3, yylineno);}
    ;

value:
    _NUMBER                                                                                 {valueNumber($1, yylineno);}
    | identifier                                                                            ;
    ;

identifier:
    _IDENTIFIER                                                                             {pidentifier($1, yylineno);}
    | _IDENTIFIER _LB _IDENTIFIER _RB                                                       {pidentifierpidentifier($1, $3, yylineno);}
    | _IDENTIFIER _LB _NUMBER _RB                                                           {pidentifiernumber($1, $3, yylineno);}
    ;

%%

int yyerror(string e) {
    cout << "Error in line: " << yylineno << ", " << e << endl;
    exit(1);
}

int main(int argv, char* argc[]) {
    yyin = fopen(argc[1], "r");
    if (yyin == NULL){
        cout << "File doesn't exist"<< endl;
    }
    pushCommand("SUB 0");
    pushCommand("INC");
    pushCommand("STORE 1");
    yyparse();

    print(argc[2]);
	return 0;
}
