#include <string.h>
#include <iostream>
#include <unistd.h>
#include <fstream>
#include <math.h>
#include <vector>
#include <stack>
#include <map>
#include <algorithm>

using namespace std;

typedef struct {
	string name;
    string type;
    long long int size;
    long long int memory;
    bool isInitialized;
	long long int begin;
	bool isLoopIterator;
} Identifier;

typedef struct {
    long long int position;
    long long int loopNumber;
} Jump;

typedef struct {
	Identifier name;
	Identifier index;
} Array;


void declareIdentifier(char* a, int yylineno);
void createIdentifier(Identifier *i, string name, long long int size, string type, long long int begin, bool loopIterator);
void declareArray (char* a, char* b, char* c, int yylineno);
void insertIdentifier(string key, Identifier i);

void read(char* a,int yylineno);
void write(char* a,int yylineno);
void assign(char* a, int yylineno);
void expressionValue(char* a, int yylineno);

void valueNumber(char* a, int yylineno);
void pidentifier(char* a, int yylineno);
void pidentifierpidentifier(char* a, char* b, int yylineno);
void pidentifiernumber(char* a, char* b, int yylineno);

void addition(char* aa, char* bb, int yylineno);
void subtraction(char* aa, char* bb, bool removing, int yylineno);
void multiplication(char* aa, char* bb, int yylineno);
void division(char* aa, char* bb, int yylineno);
void modulo(char* aa, char* bb, int yylineno);

void eq(char* aa, char* bb, int yylineno);
void neq(char* aa, char* bb, int yylineno);
void le(char* aa, char* bb, int yylineno);
void ge(char* aa, char* bb, int yylineno);
void leq(char* aa, char* bb, int yylineno);
void geq(char* aa, char* bb, int yylineno);

void ifEnd();
void ifElseEnd();
void ifElse();
void beginWhile();
void endWhile();
void endDo();

void beginToFor(char* aa, char* bb, char* cc, int yylineno);
void beginDownFor(char* aa, char* bb, char* cc, int yylineno);
void endToFor();
void endDownFor();

void pushCommand(string command);
void generateNumber(string number);
string decToBin(long long int decimal);
void print(char* out);
void increaseLoopNumber();
void createJump(Jump *j, long long int position, long long int loopNumber);
void writeFlagState(bool state);
void assignFlagState(bool state);
