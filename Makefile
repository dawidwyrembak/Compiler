all: compiler
	
parser: compiler.y	
		bison -t -d compiler.y
		
lexer: compiler.l
		flex -l compiler.l
		
compiler: lexer parser
		g++ -std=c++11 -o compiler lex.yy.c compiler.tab.c compiler.cpp 
		
clean:		
		rm lex.yy.c compiler.tab.c compiler.tab.h compiler 