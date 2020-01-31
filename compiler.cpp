#include "compiler.hpp"

long long int memoryIterator=12; // iterator komórek pamięci
long long int loopNumber = 0; // numer pętli - przyda się w zagnieżdżonych

bool writeFlag = false;  // flaga sprawdzająca inicjalizację zmiennej - raczej write
bool assignFlag = false; // flaga sprawdzająca inicjalizację zmiennej - raczej assign

map<string, Identifier> identifiers;  // mapa wszystkich zmiennych
vector<string> commands;  // komendy
vector<Jump> jumps; // tablica jumpów
vector<Identifier> fors; // tablica forów
stack<Array> arrays; // stos tablic - max 2 zmienne na stosie bo pid(pid), pid(num)

void createIdentifier(Identifier *i, string name, long long int size, string type, long long int begin, bool isLoopIterator){
    i->name = name;
    i->memory = memoryIterator;
    i->size = size;
    i->type = type;
    i->begin = begin;
    i->isInitialized = false;
    i->isLoopIterator = isLoopIterator;
}

void insertIdentifier(string key, Identifier i) {
    identifiers.insert(make_pair(key, i));
    memoryIterator += i.size;
}

void declareIdentifier(char* a, int yylineno) {
    if(identifiers.find(a)!=identifiers.end()){
        cout << "Error in line " << yylineno-1  << ", repeated declaration of variable " << a <<  endl;
        exit(1);
    }
    else {
        Identifier i;
        createIdentifier(&i, a, 1, "VAR", 0, false);
        insertIdentifier(a, i);
    }
}


void declareArray (char* a, char* b, char* c, int yylineno) {
    if (stoll(b) > stoll(c)){
        cout << "Error in line " << yylineno  << ", invalid range of array " << a << endl;
        exit(1);
    }
    else {
        Identifier i;
        long long int size  = 1 + stoll(c) - stoll(b) + 1;
        createIdentifier(&i, a, size,  "TAB", stoll(b), false);
        insertIdentifier(a, i);
    }
}


void pidentifier(char* a, int yylineno) {
    if (identifiers.find(a) == identifiers.end()){
        cout << "Error in line " << yylineno  << ", variable "<< a << " hasn't been declared " << endl;
        exit(1);
    }
    if(identifiers.at(a).type=="TAB"){
        cout << "Error in line " << yylineno  << ", incorrect using array " << a << endl;
        exit(1);
    }
    if(identifiers.at(a).isInitialized==false){
        if(writeFlag){
            cout << "Error in line " << yylineno  << ", you must initialize variable " << a << endl;
            exit(1);
        }
    }
}

void pidentifierpidentifier(char* a, char* b, int yylineno){
    if (identifiers.find(a) == identifiers.end()){
        cout << "Error in line " << yylineno  << ", variable "<< a << " hasn't been declared " << endl;
        exit(1);
    }
    if (identifiers.find(b) == identifiers.end()){
        cout << "Error in line " << yylineno  << ", variable "<< b << " hasn't been declared " << endl;
        exit(1);
    }

   Identifier name = identifiers.at(a);
   Identifier index = identifiers.at(b);

   if (name.type != "TAB"){
       cout << "Error in line " << yylineno  << ", incorrect reference to variable "<< a << endl;
       exit(1);
   }

   Array array;
   array.name = name;
   array.index = index;
   arrays.push(array);
}


void pidentifiernumber(char* a, char* b, int yylineno){
    if (identifiers.find(a) == identifiers.end()){
        cout << "Error in line " << yylineno  << ", variable "<< a << " hasn't been declared " << endl;
        exit(1);
    }
    if (stoll(b) < identifiers.at(a).begin){
        cout << "Error in line " << yylineno  << ", the range of array exceeded"<< endl;
        exit(1);
    }
    if (stoll(b) > identifiers.at(a).begin + identifiers.at(a).size){
        cout << "Error in line " << yylineno  << ", the range of array exceeded"<< endl;
        exit(1);
    }

    if (identifiers.at(a).type != "TAB"){
        cout << "Error in line " << yylineno  << ", incorrect reference to variable "<< a << endl;
        exit(1);
    }

    Identifier i = identifiers.at(a);
    long long int temp=memoryIterator;
    memoryIterator=i.memory+stoll(b)-i.begin+1;

    Identifier index;
    createIdentifier(&index, b, 1, "NUM", 0, false);
    insertIdentifier(b, index);

    Array array;
    array.name = i;
    array.index = index;
    arrays.push(array);

    memoryIterator=temp+1;
}

void valueNumber(char* a, int yylineno) {
    Identifier i;
    createIdentifier(&i, a, 1, "NUM", 0, false);
    insertIdentifier(a, i);
    assignFlagState(false);
}


void expressionValue(char* a, int yylineno){
    Identifier i = identifiers.at(a);

    if(identifiers.at(a).isInitialized==false){
        if(assignFlag){
            cout << "Error in line " << yylineno  << ", you must initialize variable " << a << endl;
            exit(1);
        }
    }

    if(i.type == "NUM") {
        generateNumber(i.name);
        identifiers.erase(i.name);
        memoryIterator--;
    }
    else if (i.type == "TAB") {
        if(arrays.top().index.type == "NUM"){
            pushCommand("LOAD " + to_string(arrays.top().index.memory));
            identifiers.erase(arrays.top().index.name);
            memoryIterator--;
            arrays.pop();
        }
        else{
            generateNumber(to_string(i.begin));
            pushCommand("STORE 7");
            generateNumber(to_string(i.memory+1));
            pushCommand("ADD "+ to_string(arrays.top().index.memory));
            pushCommand("SUB 7");
            pushCommand("LOADI 0");
            arrays.pop();
        }
    }
    else {
        pushCommand("LOAD " + to_string(i.memory));
    }
}

void assign(char* a, int yylineno){
    Identifier i = identifiers.at(a);
    if(i.isLoopIterator){
        cout << "Error in line " << yylineno  << ", you can't modify loop iterator" << endl;
        exit(1);
    }
    if (i.type == "TAB") {
        if(arrays.top().index.type == "NUM"){
            pushCommand("STORE " + to_string(arrays.top().index.memory));
            identifiers.erase(arrays.top().index.name);
            memoryIterator--;
            arrays.pop();
        }
        else{
            pushCommand("STORE 8");
            generateNumber(to_string(i.begin));
            pushCommand("STORE 7");
            generateNumber(to_string(i.memory+1));
            pushCommand("ADD "+ to_string(arrays.top().index.memory));
            pushCommand("SUB 7");

            pushCommand("STORE 10");
            pushCommand("LOAD 8");
            pushCommand("STOREI 10");
            arrays.pop();
        }
    }
    else {
        pushCommand("STORE " + to_string(i.memory));
    }
    assignFlagState(false);
    identifiers.at(a).isInitialized = true;
}

void read(char* a, int yylineno) {
    Identifier i = identifiers.at(a);
    if(i.isLoopIterator){
        cout << "Error in line " << yylineno  << ", you can't modify loop iterator" << endl;
        exit(1);
    }
    if (i.type == "TAB") {
        if(arrays.top().index.type == "NUM"){
            pushCommand("GET");
            pushCommand( "STORE " + to_string(arrays.top().index.memory));
            identifiers.erase(arrays.top().index.name);
            memoryIterator--;
            arrays.pop();
        }
        else{
            generateNumber(to_string(i.begin));
            pushCommand("STORE 7");
            generateNumber(to_string(i.memory+1));
            pushCommand("ADD "+ to_string(arrays.top().index.memory));
            pushCommand("SUB 7");
            pushCommand("STORE 10");
            pushCommand("GET");
            pushCommand("STOREI 10");
            arrays.pop();
        }
    }
    else {
        pushCommand("GET");
        pushCommand("STORE " + to_string(i.memory));
    }
    identifiers.at(a).isInitialized = true;
}


void write(char* a, int yylineno) {
    Identifier i = identifiers.at(a);
    if(i.type == "NUM") {
        generateNumber(i.name);
        identifiers.erase(i.name);
        memoryIterator--;
    }
    else if (i.type == "VAR") {
        pushCommand("LOAD " + to_string(i.memory));
    }
    else if (i.type == "TAB") {
        if(arrays.top().index.type == "NUM"){
            pushCommand("LOAD " + to_string(arrays.top().index.memory));
            identifiers.erase(arrays.top().index.name);
            memoryIterator--;
            arrays.pop();
        }
        else{
            generateNumber(to_string(i.begin));
            pushCommand("STORE 7");
            generateNumber(to_string(i.memory+1));
            pushCommand("ADD "+ to_string(arrays.top().index.memory));
            pushCommand("SUB 7");
            pushCommand("LOADI 0");
            arrays.pop();
        }
    }
    writeFlagState(false);
    pushCommand("PUT");
}


void addition(char* aa, char* bb, int yylineno) {
    Identifier a = identifiers.at(aa);
    Identifier b = identifiers.at(bb);
    if(a.type == "NUM" && b.type == "NUM") {
        generateNumber(to_string(stoll(a.name) + stoll(b.name)));
        identifiers.erase(a.name);
        memoryIterator--;
        identifiers.erase(b.name);
        memoryIterator--;
    }
    else if(a.type == "NUM" && b.type == "VAR") {
        generateNumber(a.name);
        pushCommand("ADD " + to_string(b.memory));
        identifiers.erase(a.name);
        memoryIterator--;
    }
    else if(a.type == "VAR" && b.type == "NUM") {
        generateNumber(b.name);
        pushCommand("STORE 2");
        pushCommand("LOAD "+to_string(a.memory));
        pushCommand("ADD 2");
        identifiers.erase(b.name);
        memoryIterator--;
    }
    else if(a.type == "VAR" && b.type == "VAR") {
        pushCommand("LOAD " + to_string(a.memory));
        pushCommand("ADD " + to_string(b.memory));
    }
    else if(a.type == "TAB" && b.type =="TAB"){
        if(arrays.top().index.type=="NUM"){
            Array temp = arrays.top();
            arrays.pop();

            if(arrays.top().index.type=="NUM"){
                pushCommand("LOAD " + to_string( arrays.top().index.memory));
                pushCommand("ADD " + to_string( temp.index.memory));
                identifiers.erase(arrays.top().index.name);
                memoryIterator--;
                identifiers.erase(temp.index.name);
                memoryIterator--;
                arrays.pop();
            }
            else{
                generateNumber(to_string(a.begin));
                pushCommand("STORE 7");
                generateNumber(to_string(a.memory+1));
                pushCommand("ADD "+ to_string(arrays.top().index.memory));
                pushCommand("SUB 7");
                pushCommand("STORE 9");
                pushCommand("LOADI 9");
                pushCommand("ADD 9");

                identifiers.erase(temp.index.name);
                memoryIterator--;
                arrays.pop();
            }
        }
        else{
            Array temp = arrays.top();
            arrays.pop();

            if(arrays.top().index.type=="NUM"){
                generateNumber(to_string(b.begin));
                pushCommand("STORE 7");
                generateNumber(to_string(b.memory+1));
                pushCommand("ADD "+ to_string(temp.index.memory));
                pushCommand("SUB 7");
                pushCommand("LOADI 0");
                pushCommand("STORE 9");
                pushCommand("LOAD " + to_string(arrays.top().index.memory));
                pushCommand("ADD 9");

                identifiers.erase(arrays.top().index.name);
                memoryIterator--;
                arrays.pop();
            }
            else{
                generateNumber(to_string(a.begin));
                pushCommand("STORE 7");
                generateNumber(to_string(a.memory+1));
                pushCommand("ADD "+ to_string(arrays.top().index.memory));
                pushCommand("SUB 7");
                pushCommand("STORE 9");
                generateNumber(to_string(b.begin));
                pushCommand("STORE 7");
                generateNumber(to_string(b.memory+1));
                pushCommand("ADD "+ to_string(temp.index.memory));
                pushCommand("SUB 7");
                pushCommand("LOADI 0");
                pushCommand("STORE 6");
                pushCommand("LOADI 9");
                pushCommand("ADD 6");
                arrays.pop();
            }
        }
    }
    else if(a.type == "TAB" && b.type =="VAR"){
        if(arrays.top().index.type=="NUM"){
            pushCommand("LOAD " + to_string(arrays.top().index.memory));
            pushCommand("ADD " + to_string(b.memory));
            identifiers.erase(arrays.top().index.name);
            memoryIterator--;
            arrays.pop();
        }
        else{
            generateNumber(to_string(a.begin));
            pushCommand("STORE 7");
            generateNumber(to_string(a.memory+1));
            pushCommand("ADD "+ to_string(arrays.top().index.memory));
            pushCommand("SUB 7");
            pushCommand("LOADI 0");
            pushCommand("ADD "+ to_string(b.memory));
            arrays.pop();
        }
    }
    else if(a.type == "VAR" && b.type =="TAB"){
        if(arrays.top().index.type=="NUM"){
            pushCommand("LOAD " + to_string(a.memory));
            pushCommand("ADD " + to_string( arrays.top().index.memory));
            identifiers.erase(arrays.top().index.name);
            memoryIterator--;
            arrays.pop();
        }
        else{
            generateNumber(to_string(b.begin));
            pushCommand("STORE 7");
            generateNumber(to_string(b.memory+1));
            pushCommand("ADD "+ to_string(arrays.top().index.memory));
            pushCommand("SUB 7");
            pushCommand("LOADI 0");
            pushCommand("STORE 9");
            pushCommand("LOAD "+to_string(a.memory));
            pushCommand("ADD 9");
            arrays.pop();
        }
    }
    else if(a.type == "TAB" && b.type =="NUM"){
        if(arrays.top().index.type=="NUM"){
            generateNumber(b.name);
            pushCommand("STORE 2");
            pushCommand("LOAD "+ to_string( arrays.top().index.memory));
            pushCommand("ADD 2");

            identifiers.erase(b.name);
            memoryIterator--;
            identifiers.erase(arrays.top().index.name);
            memoryIterator--;
            arrays.pop();
        }
        else{
            generateNumber(b.name);
            pushCommand("STORE 9");
            generateNumber(to_string(a.begin));
            pushCommand("STORE 7");
            generateNumber(to_string(a.memory+1));
            pushCommand("ADD "+ to_string(arrays.top().index.memory));
            pushCommand("SUB 7");
            pushCommand("LOADI 0");
            pushCommand("ADD 9");
            identifiers.erase(b.name);
            memoryIterator--;
            arrays.pop();
        }
    }
    else if(a.type == "NUM" && b.type =="TAB"){
        if(arrays.top().index.type == "NUM"){
            generateNumber(a.name);
            pushCommand("ADD " + to_string( arrays.top().index.memory));
            identifiers.erase(a.name);
            memoryIterator--;
            identifiers.erase(arrays.top().index.name);
            memoryIterator--;
            arrays.pop();
        }
        else {
            generateNumber(to_string(b.begin));
            pushCommand("STORE 7");
            generateNumber(to_string(b.memory+1));
            pushCommand("ADD "+ to_string(arrays.top().index.memory));
            pushCommand("SUB 7");
            pushCommand("LOADI 0");
            pushCommand("STORE 9");
            generateNumber(a.name);
            pushCommand("ADD 9");
            identifiers.erase(a.name);
            memoryIterator--;
            arrays.pop();
        }
    }
}



void subtraction(char* aa, char* bb, bool removing, int yylineno) {
    Identifier a = identifiers.at(aa);
    Identifier b = identifiers.at(bb);
    if(a.type == "NUM" && b.type == "NUM") {
        generateNumber(to_string(stoll(a.name) - stoll(b.name)));
        if(removing) {
            identifiers.erase(a.name);
            memoryIterator--;
            identifiers.erase(b.name);
            memoryIterator--;
        }
    }
    else if(a.type == "NUM" && b.type == "VAR") {
        generateNumber(a.name);
        pushCommand("SUB " + to_string(b.memory));
        if(removing) {
            identifiers.erase(a.name);
            memoryIterator--;
        }
    }
    else if(a.type == "VAR" && b.type == "NUM") {
        generateNumber(b.name);
        pushCommand("STORE 11");
        pushCommand("LOAD "+ to_string(a.memory));
        pushCommand("SUB 11");
        if(removing) {
            identifiers.erase(b.name);
            memoryIterator--;
        }
    }
    else if(a.type == "VAR" && b.type == "VAR") {
        if(a.name == b.name) {
            pushCommand("SUB 0");
        }
        else {
            pushCommand("LOAD " + to_string(a.memory));
            pushCommand("SUB " + to_string(b.memory));
        }
    }
    else if(a.type == "TAB" && b.type =="TAB"){
        if(arrays.top().index.type=="NUM"){
            Array temp= arrays.top();
            arrays.pop();

            if(arrays.top().index.type=="NUM"){
                pushCommand("LOAD " + to_string( arrays.top().index.memory));
                pushCommand("SUB " + to_string( temp.index.memory));
                if(removing){
                    identifiers.erase(arrays.top().index.name);
                    memoryIterator--;
                    identifiers.erase(temp.index.name);
                    memoryIterator--;
                }
                arrays.pop();
            }
            else{
                generateNumber(to_string(a.begin));
                pushCommand("STORE 7");
                generateNumber(to_string(a.memory+1));
                pushCommand("ADD "+ to_string(arrays.top().index.memory));
                pushCommand("SUB 7");
                pushCommand("LOADI 0");
                pushCommand("SUB " + to_string(temp.index.memory));

                if(removing){
                    identifiers.erase(temp.index.name);
                    memoryIterator--;
                }
                arrays.pop();
            }
        }
        else{
            Array temp = arrays.top();
            arrays.pop();

            if(arrays.top().index.type=="NUM"){
                generateNumber(to_string(b.begin));
                pushCommand("STORE 7");
                generateNumber(to_string(b.memory+1));
                pushCommand("ADD "+ to_string(temp.index.memory));
                pushCommand("SUB 7");
                pushCommand("LOADI 0");
                pushCommand("STORE 9");
                pushCommand("LOAD " + to_string(arrays.top().index.memory));
                pushCommand("SUB 9");
                if(removing){
                    identifiers.erase(arrays.top().index.name);
                    memoryIterator--;
                }
                arrays.pop();
            }
            else{
                generateNumber(to_string(b.begin));
                pushCommand("STORE 7");
                generateNumber(to_string(b.memory+1));
                pushCommand("ADD "+ to_string(temp.index.memory));
                pushCommand("SUB 7");
                pushCommand("LOADI 0");
                pushCommand("STORE 9");
                generateNumber(to_string(a.begin));
                pushCommand("STORE 7");
                generateNumber(to_string(a.memory+1));
                pushCommand("ADD "+ to_string(arrays.top().index.memory));
                pushCommand("SUB 7");
                pushCommand("LOADI 0");
                pushCommand("SUB 9");
                arrays.pop();
            }
        }
    }
    else if(a.type == "TAB" && b.type =="VAR"){
        if(arrays.top().index.type=="NUM"){
            pushCommand("LOAD " + to_string( arrays.top().index.memory));
            pushCommand("SUB " + to_string(b.memory));
            if(removing){
                identifiers.erase(arrays.top().index.name);
                memoryIterator--;
            }
            arrays.pop();
        }
        else{
            generateNumber(to_string(a.begin));
            pushCommand("STORE 7");
            generateNumber(to_string(a.memory+1));
            pushCommand("ADD "+ to_string(arrays.top().index.memory));
            pushCommand("SUB 7");
            pushCommand("LOADI 0");
            pushCommand("SUB "+ to_string(b.memory));
            arrays.pop();
        }
    }
    else if(a.type == "VAR" && b.type =="TAB"){
        if(arrays.top().index.type=="NUM"){
            pushCommand("LOAD " + to_string(a.memory));
            pushCommand("SUB " + to_string( arrays.top().index.memory));
            if(removing){
                identifiers.erase(arrays.top().index.name);
                memoryIterator--;
            }
            arrays.pop();
        }
        else{
            generateNumber(to_string(b.begin));
            pushCommand("STORE 7");
            generateNumber(to_string(b.memory+1));
            pushCommand("ADD "+ to_string(arrays.top().index.memory));
            pushCommand("SUB 7");
            pushCommand("LOADI 0");
            pushCommand("STORE 9");
            pushCommand("LOAD " +to_string(a.memory));
            pushCommand("SUB 9");
            arrays.pop();
        }

    }
    else if(a.type == "TAB" && b.type =="NUM"){
        if(arrays.top().index.type=="NUM"){
            generateNumber(b.name);
            pushCommand("STORE 2");
            pushCommand("LOAD "+ to_string( arrays.top().index.memory));
            pushCommand("SUB 2");
            if(removing) {
                identifiers.erase(b.name);
                memoryIterator--;
            }
            arrays.pop();
        }
        else{
            generateNumber(b.name);
            pushCommand("STORE 9");
            generateNumber(to_string(a.begin));
            pushCommand("STORE 7");
            generateNumber(to_string(a.memory+1));
            pushCommand("ADD "+ to_string(arrays.top().index.memory));
            pushCommand("SUB 7");
            pushCommand("LOADI 0");
            pushCommand("SUB 9");
            if(removing) {
                identifiers.erase(b.name);
                memoryIterator--;
            }
            arrays.pop();
        }
    }
    else if(a.type == "NUM" && b.type =="TAB"){
        if(arrays.top().index.type == "NUM"){
            generateNumber(a.name);
            pushCommand("SUB " + to_string( arrays.top().index.memory));
            if(removing) {
                identifiers.erase(a.name);
                memoryIterator--;
            }
            arrays.pop();
        }
        else{
            generateNumber(to_string(b.begin));
            pushCommand("STORE 7");
            generateNumber(to_string(b.memory+1));
            pushCommand("ADD "+ to_string(arrays.top().index.memory));
            pushCommand("SUB 7");
            pushCommand("LOADI 0");
            pushCommand("STORE 9");
            generateNumber(a.name);
            pushCommand("SUB 9");
            if(removing){
                identifiers.erase(a.name);
                memoryIterator--;
            }
            arrays.pop();
        }
    }
}


void multiplication(char* aa, char* bb, int yylineno) {
    Identifier a = identifiers.at(aa);
    Identifier b = identifiers.at(bb);
    if (a.type == "NUM" && b.type == "NUM") {
        generateNumber(to_string(stoll(a.name) * stoll(b.name)));
        identifiers.erase(a.name);
        memoryIterator--;
        identifiers.erase(b.name);
        memoryIterator--;
    }
    else{
        if(a.type == "VAR" && b.type=="NUM") {
            pushCommand("LOAD " + to_string(a.memory));
            pushCommand("STORE 3");
            generateNumber(b.name);
            pushCommand("STORE 4");
            identifiers.erase(b.name);
            memoryIterator--;
        }
        else if(a.type=="NUM" && b.type =="VAR"){
            generateNumber(a.name);
            pushCommand("STORE 3");
            identifiers.erase(a.name);
            memoryIterator--;
            pushCommand("LOAD " + to_string(b.memory));
            pushCommand("STORE 4");
        }
        else if(a.type == "VAR" && b.type == "VAR" ){
            pushCommand("LOAD " + to_string(a.memory));
            pushCommand("STORE 3");
            pushCommand("LOAD " + to_string(b.memory));
            pushCommand("STORE 4");
        }
        else if(a.type == "TAB" && b.type == "TAB") {
            if(arrays.top().index.type=="NUM"){
                pushCommand("LOAD " + to_string( arrays.top().index.memory));
                pushCommand("STORE 4");

                Array temp = arrays.top();
                arrays.pop();

                identifiers.erase(arrays.top().index.name);
                memoryIterator--;

                if(arrays.top().index.type=="NUM"){
                    pushCommand( "LOAD " + to_string( arrays.top().index.memory));
                    pushCommand("STORE 3");
                    identifiers.erase(arrays.top().index.name);
                    memoryIterator--;
                }
                else{
                    generateNumber(to_string(a.begin));
                    pushCommand("STORE 7");
                    generateNumber(to_string(a.memory+1));
                    pushCommand("ADD "+ to_string(arrays.top().index.memory));
                    pushCommand("SUB 7");
                    pushCommand("LOADI 0");
                    pushCommand("STORE 3");
                }
                arrays.pop();
            }
            else{
                generateNumber(to_string(b.begin));
                pushCommand("STORE 7");
                generateNumber(to_string(b.memory+1));
                pushCommand("ADD "+ to_string(arrays.top().index.memory));
                pushCommand("SUB 7");
                pushCommand("LOADI 0");
                pushCommand("STORE 4");

                Array temp = arrays.top();
                arrays.pop();

                if(arrays.top().index.type=="NUM"){
                    pushCommand("LOAD " + to_string( arrays.top().index.memory));
                    pushCommand("STORE 3");
                    identifiers.erase(arrays.top().index.name);
                    memoryIterator--;
                }
                else{
                    generateNumber(to_string(a.begin));
                    pushCommand("STORE 7");
                    generateNumber(to_string(a.memory+1));
                    pushCommand("ADD "+ to_string(arrays.top().index.memory));
                    pushCommand("SUB 7");
                    pushCommand("LOADI 0");
                    pushCommand("STORE 3");
                }
                arrays.pop();
            }
        }
        else if(a.type=="TAB" && b.type == "NUM"){
            if(arrays.top().index.type=="NUM"){
                pushCommand("LOAD " + to_string( arrays.top().index.memory));
                pushCommand("STORE 3");
                identifiers.erase(arrays.top().index.name);
                memoryIterator--;

                generateNumber(b.name);
                pushCommand("STORE 4");
                identifiers.erase(b.name);
                memoryIterator--;
            }
            else{
                generateNumber(to_string(a.begin));
                pushCommand("STORE 7");
                generateNumber(to_string(a.memory+1));
                pushCommand("ADD "+ to_string(arrays.top().index.memory));
                pushCommand("SUB 7");
                pushCommand("LOADI 0");
                pushCommand("STORE 3");

                generateNumber(b.name);
                pushCommand("STORE 4");
                identifiers.erase(b.name);
                memoryIterator--;
            }
            arrays.pop();
        }
        else if(a.type=="NUM" && b.type == "TAB"){
            if(arrays.top().index.type=="NUM"){
                pushCommand("LOAD " + to_string( arrays.top().index.memory));
                pushCommand("STORE 4");
                identifiers.erase(arrays.top().index.name);
                memoryIterator--;

                generateNumber(a.name);
                pushCommand("STORE 3");
                identifiers.erase(a.name);
                memoryIterator--;
            }
            else{
                generateNumber(to_string(b.begin));
                pushCommand("STORE 7");
                generateNumber(to_string(b.memory+1));
                pushCommand("ADD "+ to_string(arrays.top().index.memory));
                pushCommand("SUB 7");
                pushCommand("LOADI 0");
                pushCommand("STORE 4");

                generateNumber(a.name);
                pushCommand("STORE 3");
                identifiers.erase(a.name);
                memoryIterator--;
            }
            arrays.pop();
        }
        else if(a.type=="TAB" && b.type == "VAR"){
            if(arrays.top().index.type=="NUM"){
                pushCommand("LOAD " + to_string( arrays.top().index.memory));
                pushCommand("STORE 3");
                identifiers.erase(arrays.top().index.name);
                memoryIterator--;

                pushCommand("LOAD " + to_string(b.memory));
                pushCommand("STORE 4");
            }
            else{
                generateNumber(to_string(a.begin));
                pushCommand("STORE 7");
                generateNumber(to_string(a.memory+1));
                pushCommand("ADD "+ to_string(arrays.top().index.memory));
                pushCommand("SUB 7");
                pushCommand("LOADI 0");
                pushCommand("STORE 3");

                pushCommand("LOAD " + to_string(b.memory));
                pushCommand("STORE 4");
            }
            arrays.pop();
        }
        else if(a.type=="VAR" && b.type == "TAB"){
            if(arrays.top().index.type=="NUM"){
                pushCommand("LOAD " + to_string( arrays.top().index.memory));
                pushCommand("STORE 4");
                identifiers.erase(arrays.top().index.name);
                memoryIterator--;

                pushCommand("LOAD " + to_string(a.memory));
                pushCommand("STORE 3");
            }
            else{
                pushCommand("LOAD " + to_string(a.memory));
                pushCommand("STORE 3");

                generateNumber(to_string(b.begin));
                pushCommand("STORE 7");
                generateNumber(to_string(b.memory+1));
                pushCommand("ADD "+ to_string(arrays.top().index.memory));
                pushCommand("SUB 7");
                pushCommand("LOADI 0");
                pushCommand("STORE 4");
            }
            arrays.pop();
        }

        pushCommand("SUB 0");
        pushCommand("STORE 5"); // wynik
        pushCommand("STORE 6"); //flaga
        pushCommand("DEC");
        pushCommand("STORE 2");
        pushCommand("LOAD 4");
        pushCommand("JPOS " + to_string(commands.size() + 8));
        pushCommand("SUB 0");
        pushCommand("SUB 4");
        pushCommand("STORE 4");
        pushCommand("LOAD 6");
        pushCommand("INC");
        pushCommand("STORE 6");
        pushCommand("LOAD 4");
        pushCommand("JZERO " + to_string(commands.size() + 15));
        pushCommand("SHIFT 2");
        pushCommand("SHIFT 1");
        pushCommand("SUB 4");
        pushCommand("JZERO " + to_string(commands.size() + 4));
        pushCommand("LOAD 5");
        pushCommand("ADD 3");
        pushCommand("STORE 5");
        pushCommand("LOAD 3");
        pushCommand("SHIFT 1");
        pushCommand("STORE 3");
        pushCommand("LOAD 4");
        pushCommand("SHIFT 2");
        pushCommand("STORE 4");
        pushCommand("JUMP " +  to_string(commands.size() - 14));
        pushCommand("LOAD 6");
        pushCommand("JZERO " + to_string(commands.size() + 4));
        pushCommand("SUB 0");
        pushCommand("SUB 5");
        pushCommand("STORE 5");
        pushCommand("LOAD 5");
    }
}


void division(char* aa, char* bb, int yylineno) {
    Identifier a = identifiers.at(aa);
    Identifier b = identifiers.at(bb);

    if(b.type == "NUM" && stoll(b.name) == 0 || a.type == "NUM" && stoll(a.name) == 0) {
        generateNumber("0");
    }
    else if (a.type == "NUM" && b.type == "NUM") {
        generateNumber(to_string(stoll(a.name) / stoll(b.name)));
        identifiers.erase(a.name);
        memoryIterator--;
        identifiers.erase(b.name);
        memoryIterator--;
    }
    else{
        if(a.type == "VAR" && b.type=="NUM") {
            pushCommand("LOAD " + to_string(a.memory));
            pushCommand("STORE 2");
            pushCommand("STORE 3");

            generateNumber(b.name);
            pushCommand("STORE 4");
            identifiers.erase(b.name);
            memoryIterator--;
        }
        else if(a.type=="NUM" && b.type =="VAR"){
            generateNumber(a.name);
            pushCommand("STORE 2");
            pushCommand("STORE 3");
            identifiers.erase(a.name);
            memoryIterator--;

            pushCommand("LOAD " + to_string(b.memory));
            pushCommand("STORE 4");
        }
        else if(a.type == "VAR" && b.type == "VAR" ){
            pushCommand("LOAD " + to_string(a.memory));
            pushCommand("STORE 2");
            pushCommand("STORE 3");

            pushCommand("LOAD " + to_string(b.memory));
            pushCommand("STORE 4");
        }
        else if(a.type == "TAB" && b.type == "TAB") {
            if(arrays.top().index.type=="NUM"){
                pushCommand("LOAD " + to_string( arrays.top().index.memory));
                pushCommand("STORE 4");

                Array temp = arrays.top();
                arrays.pop();
                identifiers.erase(arrays.top().index.name);
                memoryIterator--;

                if(arrays.top().index.type=="NUM"){
                    pushCommand("LOAD " + to_string( arrays.top().index.memory));
                    pushCommand("STORE 2");
                    pushCommand("STORE 3");
                    identifiers.erase(arrays.top().index.name);
                    memoryIterator--;
                }
                else{
                    generateNumber(to_string(a.begin));
                    pushCommand("STORE 7");
                    generateNumber(to_string(a.memory+1));
                    pushCommand("ADD "+ to_string(arrays.top().index.memory));
                    pushCommand("SUB 7");
                    pushCommand("LOADI 0");

                    pushCommand("STORE 2");
                    pushCommand("STORE 3");
                }
                arrays.pop();

            }
            else{
                generateNumber(to_string(b.begin));
                pushCommand("STORE 7");
                generateNumber(to_string(b.memory+1));
                pushCommand("ADD "+ to_string(arrays.top().index.memory));
                pushCommand("SUB 7");
                pushCommand("LOADI 0");
                pushCommand("STORE 4");

                Array temp = arrays.top();
                arrays.pop();

                if(arrays.top().index.type=="NUM"){
                    pushCommand("LOAD " + to_string( arrays.top().index.memory));
                    identifiers.erase(arrays.top().index.name);
                    memoryIterator--;
                    pushCommand("STORE 2");
                    pushCommand("STORE 3");
                }
                else{
                    generateNumber(to_string(a.begin));
                    pushCommand("STORE 7");
                    generateNumber(to_string(a.memory+1));
                    pushCommand("ADD "+ to_string(arrays.top().index.memory));
                    pushCommand("SUB 7");
                    pushCommand("LOADI 0");

                    pushCommand("STORE 2");
                    pushCommand("STORE 3");
                }
                arrays.pop();
            }
        }
        else if(a.type=="TAB" && b.type == "NUM"){
            if(arrays.top().index.type=="NUM"){
                pushCommand("LOAD " + to_string( arrays.top().index.memory));
                pushCommand("STORE 2");
                pushCommand("STORE 3");
                identifiers.erase(arrays.top().index.name);
                memoryIterator--;

                generateNumber(b.name);
                pushCommand("STORE 4");
                identifiers.erase(b.name);
                memoryIterator--;
            }
            else{
                generateNumber(to_string(a.begin));
                pushCommand("STORE 7");
                generateNumber(to_string(a.memory+1));
                pushCommand("ADD "+ to_string(arrays.top().index.memory));
                pushCommand("SUB 7");
                pushCommand("LOADI 0");

                pushCommand("STORE 2");
                pushCommand("STORE 3");

                generateNumber(b.name);
                pushCommand("STORE 4");
                identifiers.erase(b.name);
                memoryIterator--;
            }
            arrays.pop();
        }
        else if(a.type=="NUM" && b.type == "TAB"){
            if(arrays.top().index.type=="NUM"){
                pushCommand("LOAD " + to_string( arrays.top().index.memory));
                pushCommand("STORE 4");
                identifiers.erase(arrays.top().index.name);
                memoryIterator--;

                generateNumber(a.name);
                pushCommand("STORE 2");
                pushCommand("STORE 3");
                identifiers.erase(a.name);
                memoryIterator--;
            }
            else{
                generateNumber(to_string(b.begin));
                pushCommand("STORE 7");
                generateNumber(to_string(b.memory+1));
                pushCommand("ADD "+ to_string(arrays.top().index.memory));
                pushCommand("SUB 7");
                pushCommand("LOADI 0");
                pushCommand("STORE 4");

                generateNumber(a.name);
                pushCommand("STORE 2");
                pushCommand("STORE 3");
                identifiers.erase(a.name);
                memoryIterator--;
            }
            arrays.pop();
        }
        else if(a.type=="TAB" && b.type == "VAR"){
            if(arrays.top().index.type=="NUM"){
                pushCommand("LOAD " + to_string( arrays.top().index.memory));
                pushCommand("STORE 2");
                pushCommand("STORE 3");
                identifiers.erase(arrays.top().index.name);
                memoryIterator--;

                pushCommand("LOAD " + to_string(b.memory));
                pushCommand("STORE 4");
            }
            else{
                generateNumber(to_string(a.begin));
                pushCommand("STORE 7");
                generateNumber(to_string(a.memory+1));
                pushCommand("ADD "+ to_string(arrays.top().index.memory));
                pushCommand("SUB 7");
                pushCommand("LOADI 0");

                pushCommand("STORE 2");
                pushCommand("STORE 3");

                pushCommand("LOAD " + to_string(b.memory));
                pushCommand("STORE 4");
            }
            arrays.pop();
        }
        else if(a.type=="VAR" && b.type == "TAB"){
            if(arrays.top().index.type=="NUM"){
                pushCommand("LOAD " + to_string( arrays.top().index.memory));
                pushCommand("STORE 4");
                identifiers.erase(arrays.top().index.name);
                memoryIterator--;

                pushCommand("LOAD " + to_string(a.memory));
                pushCommand("STORE 2");
                pushCommand("STORE 3");
            }
            else{
                generateNumber(to_string(b.begin));
                pushCommand("STORE 7");
                generateNumber(to_string(b.memory+1));
                pushCommand("ADD "+ to_string(arrays.top().index.memory));
                pushCommand("SUB 7");
                pushCommand("LOADI 0");
                pushCommand("STORE 4");

                pushCommand("LOAD " + to_string(a.memory));
                pushCommand("STORE 2");
                pushCommand("STORE 3");
            }
            arrays.pop();
        }

        pushCommand("SUB 0");
        pushCommand("STORE 5"); // wynik
        pushCommand("STORE 7"); // flaga
        pushCommand("DEC");
        pushCommand("STORE 6");
        pushCommand("INC");
        pushCommand("INC");
        pushCommand("STORE 8"); // multiple
        pushCommand("LOAD 3");
        pushCommand("JZERO "+  to_string(commands.size() + 60));
        pushCommand("JPOS " + to_string(commands.size() + 10) );
        pushCommand("SUB 0");
        pushCommand("SUB 3");
        pushCommand("STORE 3");
        pushCommand("SUB 0");
        pushCommand("SUB 2");
        pushCommand("STORE 2");
        pushCommand("LOAD 7");
        pushCommand("INC");
        pushCommand("STORE 7");
        pushCommand("LOAD 4");
        pushCommand("JZERO "+  to_string(commands.size() + 49));
        pushCommand("JPOS " + to_string(commands.size() + 11) );
        pushCommand("SUB 0");
        pushCommand("SUB 4");
        pushCommand("STORE 4");
        pushCommand("LOAD 7");
        pushCommand("JPOS "+ to_string(commands.size() + 3));
        pushCommand("INC");
        pushCommand("JUMP "+ to_string(commands.size() + 2));
        pushCommand("DEC");
        pushCommand("STORE 7");
        pushCommand("LOAD 4");
        pushCommand("SUB 3");
        pushCommand("JZERO "+ to_string(commands.size() + 9));
        pushCommand("JPOS "+ to_string(commands.size() + 8));
        pushCommand("LOAD 4");
        pushCommand("ADD 4");
        pushCommand("STORE 4");
        pushCommand("LOAD 8");
        pushCommand("ADD 8");
        pushCommand("STORE 8");
        pushCommand("JUMP "+ to_string(commands.size() - 10));
        pushCommand("LOAD 2");
        pushCommand("SUB 4");
        pushCommand("JNEG "+ to_string(commands.size() +7) );
        pushCommand("LOAD 2");
        pushCommand("SUB 4");
        pushCommand("STORE 2");
        pushCommand("LOAD 5");
        pushCommand("ADD 8");
        pushCommand("STORE 5");
        pushCommand("LOAD 4");
        pushCommand("SHIFT 6");
        pushCommand("STORE 4");
        pushCommand("LOAD 8");
        pushCommand("SHIFT 6");
        pushCommand("STORE 8");
        pushCommand("JZERO " + to_string(commands.size() +2) );
        pushCommand("JUMP " +  to_string(commands.size() -16));
        pushCommand("LOAD 7");
        pushCommand("JZERO " + to_string(commands.size() + 9));
        pushCommand("SUB 0");
        pushCommand("SUB 5");
        pushCommand("STORE 5");
        pushCommand("LOAD 2");
        pushCommand("JZERO "+to_string(commands.size()+ 4));
        pushCommand("LOAD 5");
        pushCommand("DEC");
        pushCommand("STORE 5");
        pushCommand("LOAD 5");
    }
}


void modulo(char* aa, char* bb, int yylineno) {
    Identifier a = identifiers.at(aa);
    Identifier b = identifiers.at(bb);

    if(b.type == "NUM" && stoll(b.name) == 0 || a.type == "NUM" && stoll(a.name) == 0) {
        generateNumber("0");
    }
    else if (a.type == "NUM" && b.type == "NUM") {
        generateNumber(to_string(stoll(a.name) % stoll(b.name)));
        identifiers.erase(a.name);
        memoryIterator--;
        identifiers.erase(b.name);
        memoryIterator--;
    }
    else{
        if(a.type == "VAR" && b.type=="NUM") {
            pushCommand("LOAD " + to_string(a.memory));
            pushCommand("STORE 2");
            pushCommand("STORE 3");

            generateNumber(b.name);
            pushCommand("STORE 4");
            identifiers.erase(b.name);
            memoryIterator--;
        }
        else if(a.type=="NUM" && b.type =="VAR"){
            generateNumber(a.name);
            pushCommand("STORE 2");
            pushCommand("STORE 3");
            identifiers.erase(a.name);
            memoryIterator--;

            pushCommand("LOAD " + to_string(b.memory));
            pushCommand("STORE 4");

        }
        else if(a.type == "VAR" && b.type == "VAR" ){
            pushCommand("LOAD " + to_string(a.memory));
            pushCommand("STORE 2");
            pushCommand("STORE 3");

            pushCommand("LOAD " + to_string(b.memory));
            pushCommand("STORE 4");
        }
        else if(a.type == "TAB" && b.type == "TAB") {
            if(arrays.top().index.type=="NUM"){
                pushCommand("LOAD " + to_string( arrays.top().index.memory));
                pushCommand("STORE 4");

                Array temp = arrays.top();
                arrays.pop();
                identifiers.erase(arrays.top().index.name);
                memoryIterator--;

                if(arrays.top().index.type=="NUM"){
                    pushCommand("LOAD " + to_string( arrays.top().index.memory));
                    pushCommand("STORE 2");
                    pushCommand("STORE 3");
                    identifiers.erase(arrays.top().index.name);
                    memoryIterator--;
                }
                else{
                    generateNumber(to_string(a.begin));
                    pushCommand("STORE 7");
                    generateNumber(to_string(a.memory+1));
                    pushCommand("ADD "+ to_string(arrays.top().index.memory));
                    pushCommand("SUB 7");
                    pushCommand("LOADI 0");

                    pushCommand("STORE 2");
                    pushCommand("STORE 3");
                }
                arrays.pop();

            }
            else{
                generateNumber(to_string(b.begin));
                pushCommand("STORE 7");
                generateNumber(to_string(b.memory+1));
                pushCommand("ADD "+ to_string(arrays.top().index.memory));
                pushCommand("SUB 7");
                pushCommand("LOADI 0");
                pushCommand("STORE 4");

                Array temp = arrays.top();
                arrays.pop();

                if(arrays.top().index.type=="NUM"){
                    pushCommand("LOAD " + to_string( arrays.top().index.memory));
                    identifiers.erase(arrays.top().index.name);
                    memoryIterator--;
                    pushCommand("STORE 2");
                    pushCommand("STORE 3");
                }
                else{
                    generateNumber(to_string(a.begin));
                    pushCommand("STORE 7");
                    generateNumber(to_string(a.memory+1));
                    pushCommand("ADD "+ to_string(arrays.top().index.memory));
                    pushCommand("SUB 7");
                    pushCommand("LOADI 0");

                    pushCommand("STORE 2");
                    pushCommand("STORE 3");
                }
                arrays.pop();
            }
        }
        else if(a.type=="TAB" && b.type == "NUM"){
            if(arrays.top().index.type=="NUM"){
                pushCommand("LOAD " + to_string( arrays.top().index.memory));
                pushCommand("STORE 2");
                pushCommand("STORE 3");
                identifiers.erase(arrays.top().index.name);
                memoryIterator--;

                generateNumber(b.name);
                pushCommand("STORE 4");
                identifiers.erase(b.name);
                memoryIterator--;

            }
            else{
                generateNumber(to_string(a.begin));
                pushCommand("STORE 7");
                generateNumber(to_string(a.memory+1));
                pushCommand("ADD "+ to_string(arrays.top().index.memory));
                pushCommand("SUB 7");
                pushCommand("LOADI 0");
                pushCommand("STORE 2");
                pushCommand("STORE 3");

                generateNumber(b.name);
                pushCommand("STORE 4");
                identifiers.erase(b.name);
                memoryIterator--;
            }
            arrays.pop();
        }
        else if(a.type=="NUM" && b.type == "TAB"){
            if(arrays.top().index.type=="NUM"){
                pushCommand("LOAD " + to_string( arrays.top().index.memory));
                pushCommand("STORE 4");
                identifiers.erase(arrays.top().index.name);
                memoryIterator--;

                generateNumber(a.name);
                pushCommand("STORE 2");
                pushCommand("STORE 3");
                identifiers.erase(a.name);
                memoryIterator--;

            }
            else{
                generateNumber(to_string(b.begin));
                pushCommand("STORE 7");
                generateNumber(to_string(b.memory+1));
                pushCommand("ADD "+ to_string(arrays.top().index.memory));
                pushCommand("SUB 7");
                pushCommand("LOADI 0");
                pushCommand("STORE 4");

                generateNumber(a.name);
                pushCommand("STORE 2");
                pushCommand("STORE 3");
                identifiers.erase(a.name);
                memoryIterator--;
            }
            arrays.pop();
        }
        else if(a.type=="TAB" && b.type == "VAR"){
            if(arrays.top().index.type=="NUM"){
                pushCommand("LOAD " + to_string( arrays.top().index.memory));
                pushCommand("STORE 2");
                pushCommand("STORE 3");
                identifiers.erase(arrays.top().index.name);
                memoryIterator--;

                pushCommand("LOAD " + to_string(b.memory));
                pushCommand("STORE 4");
            }
            else{
                generateNumber(to_string(a.begin));
                pushCommand("STORE 7");
                generateNumber(to_string(a.memory+1));
                pushCommand("ADD "+ to_string(arrays.top().index.memory));
                pushCommand("SUB 7");
                pushCommand("LOADI 0");

                pushCommand("STORE 2");
                pushCommand("STORE 3");

                pushCommand("LOAD " + to_string(b.memory));
                pushCommand("STORE 4");
            }
            arrays.pop();
        }
        else if(a.type=="VAR" && b.type == "TAB"){
            if(arrays.top().index.type=="NUM"){
                pushCommand("LOAD " + to_string( arrays.top().index.memory));
                pushCommand("STORE 4");
                identifiers.erase(arrays.top().index.name);
                memoryIterator--;

                pushCommand("LOAD " + to_string(a.memory));
                pushCommand("STORE 2");
                pushCommand("STORE 3");
            }
            else{
                generateNumber(to_string(b.begin));
                pushCommand("STORE 7");
                generateNumber(to_string(b.memory+1));
                pushCommand("ADD "+ to_string(arrays.top().index.memory));
                pushCommand("SUB 7");
                pushCommand("LOADI 0");
                pushCommand("STORE 4");

                pushCommand("LOAD " + to_string(a.memory));
                pushCommand("STORE 2");
                pushCommand("STORE 3");
            }
            arrays.pop();
        }

        pushCommand("SUB 0");
        pushCommand("STORE 7"); // flaga
        pushCommand("STORE 8"); // flaga
        pushCommand("DEC");
        pushCommand("STORE 6");
        pushCommand("INC");
        pushCommand("INC");
        pushCommand("STORE 5");
        pushCommand("LOAD 3");
        pushCommand("JPOS "+  to_string(commands.size() + 4));
        pushCommand("LOAD 7");
        pushCommand("INC");
        pushCommand("STORE 7");
        pushCommand("LOAD 4");
        pushCommand("STORE 9");
        pushCommand("JPOS "+  to_string(commands.size() + 4));
        pushCommand("LOAD 8");
        pushCommand("INC");
        pushCommand("STORE 8");
        pushCommand("LOAD 3");
        pushCommand("JZERO "+  to_string(commands.size() + 58));
        pushCommand("JPOS " + to_string(commands.size() + 7) );
        pushCommand("SUB 0");
        pushCommand("SUB 3");
        pushCommand("STORE 3");
        pushCommand("SUB 0");
        pushCommand("SUB 2");
        pushCommand("STORE 2");
        pushCommand("LOAD 4");
        pushCommand("JZERO "+  to_string(commands.size() + 49));
        pushCommand("JPOS " + to_string(commands.size() + 4) );
        pushCommand("SUB 0");
        pushCommand("SUB 4");
        pushCommand("STORE 4");
        pushCommand("LOAD 4");
        pushCommand("SUB 3");
        pushCommand("JZERO "+ to_string(commands.size() + 9));
        pushCommand("JPOS "+ to_string(commands.size() + 8));
        pushCommand("LOAD 4");
        pushCommand("ADD 4");
        pushCommand("STORE 4");
        pushCommand("LOAD 5");
        pushCommand("ADD 5");
        pushCommand("STORE 5");
        pushCommand("JUMP "+ to_string(commands.size() - 9));
        pushCommand("LOAD 2");
        pushCommand("SUB 4");
        pushCommand("JNEG "+ to_string(commands.size() +4) );
        pushCommand("LOAD 2");
        pushCommand("SUB 4");
        pushCommand("STORE 2");
        pushCommand("LOAD 4");
        pushCommand("SHIFT 6");
        pushCommand("STORE 4");
        pushCommand("LOAD 5");
        pushCommand("SHIFT 6");
        pushCommand("STORE 5");
        pushCommand("JZERO " + to_string(commands.size() +2) );
        pushCommand("JUMP " +  to_string(commands.size() -13));
        pushCommand("LOAD 7");
        pushCommand("JZERO "+  to_string(commands.size() + 11)); //jeśli a>0
        pushCommand("LOAD 8");
        pushCommand("JZERO "+  to_string(commands.size() + 5)); //jeśli b>0
        pushCommand("SUB 0");
        pushCommand("SUB 2");               // a<0 b<0
        pushCommand("STORE 2");
        pushCommand("JUMP "+  to_string(commands.size() + 10));
        pushCommand("LOAD 9");
        pushCommand("SUB 2");         // a<0 b>0
        pushCommand("STORE 2");
        pushCommand("JUMP "+  to_string(commands.size() + 6));
        pushCommand("LOAD 8");
        pushCommand("JZERO "+  to_string(commands.size() + 4)); //jeśli b>0
        pushCommand("LOAD 2");
        pushCommand("ADD 9");            // a>0 b<0
        pushCommand("STORE 2");
        pushCommand("LOAD 2");
    }
}


void createJump(Jump *j, long long int position, long long int loopNumber){
    j->position = position;
    j->loopNumber = loopNumber;
}


void eq(char* aa, char* bb, int yylineno){
    Identifier a = identifiers.at(aa);
    Identifier b = identifiers.at(bb);

    if(a.type == "NUM" && b.type == "NUM") {
        if(stoll(a.name) == stoll(b.name)){
            generateNumber("1");
        }
        else{
            generateNumber("0");
        }

        identifiers.erase(a.name);
        memoryIterator--;
        identifiers.erase(b.name);
        memoryIterator--;
        Jump j;
        createJump(&j, commands.size(), loopNumber);
        jumps.push_back(j);
        pushCommand("JZERO ");
    }
    else {
        subtraction(bb, aa, false, yylineno);
        pushCommand("JZERO "+ to_string(commands.size()+2));
        Jump j;
        createJump(&j, commands.size(), loopNumber);
        jumps.push_back(j);
        pushCommand("JUMP ");
    }
}

void neq(char* aa, char* bb, int yylineno){
    Identifier a = identifiers.at(aa);
    Identifier b = identifiers.at(bb);

    if(a.type == "NUM" && b.type == "NUM") {
        if(stoll(a.name) != stoll(b.name)){
            generateNumber("1");
        }
        else{
            generateNumber("0");
        }

        identifiers.erase(a.name);
        memoryIterator--;
        identifiers.erase(b.name);
        memoryIterator--;
        Jump j;
        createJump(&j, commands.size(), loopNumber);
        jumps.push_back(j);
        pushCommand("JZERO ");
    }
    else {
        subtraction(bb, aa, false, yylineno);
        pushCommand("JZERO "+ to_string(commands.size()+2));
        Jump j;
        createJump(&j, commands.size(), loopNumber);
        jumps.push_back(j);
        pushCommand("JUMP ");

        commands.at(jumps.at(jumps.size()-1).position) = commands.at(jumps.at(jumps.size()-1).position) + to_string(commands.size()+1);
        jumps.pop_back();

        Jump j2;
        createJump(&j2, commands.size(), loopNumber);
        jumps.push_back(j2);
        pushCommand("JUMP ");
    }
}


void le(char* aa, char* bb, int yylineno){
    Identifier a = identifiers.at(aa);
    Identifier b = identifiers.at(bb);

    if(a.type == "NUM" && b.type == "NUM") {
        if(stoll(a.name) < stoll(b.name)){
            generateNumber("1");
        }
        else{
            generateNumber("0");
        }

        identifiers.erase(a.name);
        memoryIterator--;
        identifiers.erase(b.name);
        memoryIterator--;
    }
    else {
        subtraction(bb, aa, false, yylineno);
    }

    pushCommand("JPOS "+ to_string(commands.size()+2));
    Jump j;
    createJump(&j, commands.size(), loopNumber);
    jumps.push_back(j);
    pushCommand("JUMP ");
}



void ge(char* aa, char* bb, int yylineno){
    Identifier a = identifiers.at(aa);
    Identifier b = identifiers.at(bb);

    if(a.type == "NUM" && b.type == "NUM") {
        if(stoll(a.name) > stoll(b.name)){
            generateNumber("1");
        }
        else{
            generateNumber("0");
        }

        identifiers.erase(a.name);
        memoryIterator--;
        identifiers.erase(b.name);
        memoryIterator--;
    }
    else {
        subtraction(bb, aa, false, yylineno);
    }

    pushCommand("JNEG "+ to_string(commands.size()+2));
    Jump j;
    createJump(&j, commands.size(), loopNumber);
    jumps.push_back(j);
    pushCommand("JUMP ");
}

void leq(char* aa, char* bb, int yylineno){
    Identifier a = identifiers.at(aa);
    Identifier b = identifiers.at(bb);

    if(a.type == "NUM" && b.type == "NUM") {
        if(stoll(a.name) <= stoll(b.name)){
            generateNumber("1");
        }
        else{
            generateNumber("0");
        }

        identifiers.erase(a.name);
        memoryIterator--;
        identifiers.erase(b.name);
        memoryIterator--;
    }
    else {
        subtraction(bb, aa, false, yylineno);
    }

    Jump j;
    createJump(&j, commands.size(), loopNumber);
    jumps.push_back(j);
    pushCommand("JNEG ");
}


void geq(char* aa, char* bb, int yylineno){
    Identifier a = identifiers.at(aa);
    Identifier b = identifiers.at(bb);

    if(a.type == "NUM" && b.type == "NUM") {
        if(stoll(a.name) >= stoll(b.name)){
            generateNumber("1");
        }
        else{
            generateNumber("0");
        }

        identifiers.erase(a.name);
        memoryIterator--;
        identifiers.erase(b.name);
        memoryIterator--;
    }
    else {
        subtraction(bb, aa, false, yylineno);
    }

    Jump j;
    createJump(&j, commands.size(), loopNumber);
    jumps.push_back(j);
    pushCommand("JPOS ");
}


void ifEnd(){
    int numberOfJumps = jumps.size()-1;
    commands.at(jumps.at(numberOfJumps).position) = commands.at(jumps.at(numberOfJumps).position) + to_string(commands.size());
    numberOfJumps--;
    if(numberOfJumps >= 0 && jumps.at(numberOfJumps).loopNumber == loopNumber) {
        commands.at(jumps.at(numberOfJumps).position) = commands.at(jumps.at(numberOfJumps).position) + to_string(commands.size());
        jumps.pop_back();
    }
    jumps.pop_back();
    loopNumber--;
}

void ifElseEnd(){
    commands.at(jumps.at(jumps.size()-1).position) = commands.at(jumps.at(jumps.size()-1).position) + to_string(commands.size());
    jumps.pop_back();
    jumps.pop_back();
    if(jumps.size() >= 1 && jumps.at(jumps.size()-1).loopNumber == loopNumber) {
        jumps.pop_back();
    }
    loopNumber--;
}

void ifElse(){
    Jump j;
    createJump(&j, commands.size(), loopNumber);
    jumps.push_back(j);
    pushCommand("JUMP ");
    long long int numberOfJumps = jumps.size()-2;
    Jump jump = jumps.at(numberOfJumps);
    commands.at(jump.position) = commands.at(jump.position) + to_string(commands.size());
    numberOfJumps--;
    if(numberOfJumps >= 0 && jumps.at(numberOfJumps).loopNumber == loopNumber) {
        commands.at(jumps.at(numberOfJumps).position) = commands.at(jumps.at(numberOfJumps).position) + to_string(commands.size());
    }
}

void beginWhile(){
    loopNumber++;
    Jump j;
    createJump(&j, commands.size(), loopNumber);
    jumps.push_back(j);
}


void endWhile(){
    long long int stack;
    long long int numberOfJumps = jumps.size()-1;
    if(numberOfJumps > 2 && jumps.at(numberOfJumps-2).loopNumber == loopNumber) {
        stack = jumps.at(numberOfJumps-2).position;
        pushCommand("JUMP "+ to_string(stack));
        commands.at(jumps.at(numberOfJumps).position) = commands.at(jumps.at(numberOfJumps).position) + to_string(commands.size());
        commands.at(jumps.at(numberOfJumps-1).position) = commands.at(jumps.at(numberOfJumps-1).position) + to_string(commands.size());
        jumps.pop_back();
    }
    else {
        stack = jumps.at(numberOfJumps-1).position;
        pushCommand("JUMP "+ to_string(stack));
        commands.at(jumps.at(numberOfJumps).position) = commands.at(jumps.at(numberOfJumps).position) + to_string(commands.size());
    }
    jumps.pop_back();
    jumps.pop_back();
    loopNumber--;
}

void endDo(){
    long long int stack;
    long long int numberOfJumps = jumps.size()-1;
    if(numberOfJumps > 2 && jumps.at(numberOfJumps-2).loopNumber == loopNumber) {
        stack = jumps.at(numberOfJumps-2).position;
        pushCommand("JUMP "+ to_string(stack));
        commands.at(jumps.at(numberOfJumps).position) = commands.at(jumps.at(numberOfJumps).position) + to_string(commands.size());
        commands.at(jumps.at(numberOfJumps-1).position) = commands.at(jumps.at(numberOfJumps-1).position) + to_string(commands.size());
        jumps.pop_back();
    }
    else {
        stack = jumps.at(numberOfJumps-1).position;
        pushCommand("JUMP "+ to_string(stack));
        commands.at(jumps.at(numberOfJumps).position) = commands.at(jumps.at(numberOfJumps).position) + to_string(commands.size());
    }
    jumps.pop_back();
    jumps.pop_back();
    loopNumber--;
}

void beginToFor(char* aa, char *bb, char *cc, int yylineno){
    if(identifiers.find(aa)!=identifiers.end()) {
        cout << "Error in line " << yylineno  << ", you can't use -"<< aa << " - global variable in loop " << endl;
        exit(1);
    }
    else {
        Identifier i;
        createIdentifier(&i, aa, 1, "VAR", 0, true);
        insertIdentifier(aa, i);
    }
    loopNumber++;

    Identifier a = identifiers.at(aa);
    Identifier begin = identifiers.at(bb);
    if(identifiers.at(bb).type=="VAR" && identifiers.at(bb).isInitialized==false){
        cout << "Error in line " << yylineno  << ", you must initialize this variable - "<< bb << " before loop " << endl;
        exit(1);
    }
    Identifier end = identifiers.at(cc);
    if(identifiers.at(cc).type=="VAR" && identifiers.at(cc).isInitialized==false){
        cout << "Error in line " << yylineno  << ", you must initialize this variable - "<< cc << " before loop " << endl;
        exit(1);
    }


    if(begin.type == "NUM") {
        generateNumber(begin.name);

    }
    else if(begin.type == "VAR") {
        string command = "LOAD " + to_string(begin.memory);
        pushCommand(command);
    }else{
        if(end.type =="TAB"){
            Array temp = arrays.top();
            arrays.pop();
            if(arrays.top().index.type=="NUM"){
                string command1 = "LOAD " + to_string( arrays.top().index.memory);
                pushCommand(command1);
            }
            else{
                generateNumber(to_string(begin.begin));
                pushCommand("STORE 7");
                generateNumber(to_string(begin.memory+1));
                pushCommand("ADD "+ to_string(arrays.top().index.memory));
                pushCommand("SUB 7");
                pushCommand("LOADI 0");
            }
            arrays.push(temp);
        }
        else{
            if(arrays.top().index.type=="NUM"){
                string command1 = "LOAD " + to_string( arrays.top().index.memory);
                pushCommand(command1);
            }
            else{
                generateNumber(to_string(begin.begin));
                pushCommand("STORE 7");
                generateNumber(to_string(begin.memory+1));
                pushCommand("ADD "+ to_string(arrays.top().index.memory));
                pushCommand("SUB 7");
                pushCommand("LOADI 0");
            }
        }
    }

    pushCommand("STORE "+to_string(a.memory));
    identifiers.at(a.name).isInitialized = true;

    subtraction(cc,aa, false, yylineno);

    Identifier temp;
    string name = "NUMBER" + to_string(loopNumber);
    createIdentifier(&temp, name, 1,  "VAR", 0, false);
    insertIdentifier(name, temp);

    pushCommand("STORE " + to_string(identifiers.at(name).memory));
    generateNumber("1");
    pushCommand("ADD " + to_string(identifiers.at(name).memory));
    pushCommand("STORE " + to_string(identifiers.at(name).memory));

    fors.push_back(identifiers.at(a.name));

    Jump j;
    createJump(&j, commands.size(), loopNumber);
    jumps.push_back(j);
    pushCommand("JNEG ");
}

void endToFor(){
    Identifier iterator = fors.at(fors.size()-1);
    pushCommand("LOAD "+to_string(iterator.memory));
    pushCommand("INC");
    pushCommand("STORE "+to_string(iterator.memory));

    string name = "NUMBER" + to_string(loopNumber);
    pushCommand("LOAD "+to_string(identifiers.at(name).memory));
    pushCommand("DEC");
    pushCommand("STORE "+to_string(identifiers.at(name).memory));
    identifiers.erase(name);
    memoryIterator--;

    long long int numberOfJumps = jumps.size()-1;
    pushCommand("JPOS "+to_string(jumps.at(numberOfJumps).position+1));

    commands.at(jumps.at(numberOfJumps).position) = commands.at(jumps.at(numberOfJumps).position) + to_string(commands.size());
    jumps.pop_back();
    identifiers.erase(iterator.name);
    memoryIterator--;
    fors.pop_back();
    loopNumber--;
}


void beginDownFor(char* aa, char *bb, char *cc, int yylineno){
    if(identifiers.find(aa)!=identifiers.end()) {
        cout << "Error in line " << yylineno  << ", you can't use -"<< aa << " - global variable in loop " << endl;
        exit(1);
    }
    else {
        Identifier i;
        createIdentifier(&i, aa, 1, "VAR", 0, true);
        insertIdentifier(aa, i);
    }
    loopNumber++;

    Identifier a = identifiers.at(aa);
    Identifier begin = identifiers.at(bb);
    if(identifiers.at(bb).type=="VAR" && identifiers.at(bb).isInitialized==false){
        cout << "Error in line " << yylineno  << ", you must initialize this variable - "<< bb << " before loop " << endl;
        exit(1);
    }
    Identifier end = identifiers.at(cc);
    if(identifiers.at(cc).type=="VAR" && identifiers.at(cc).isInitialized==false){
        cout << "Error in line " << yylineno  << ", you must initialize this variable - "<< cc << " before loop " << endl;
        exit(1);
    }


    if(begin.type == "NUM") {
        generateNumber(begin.name);

    }
    else if(begin.type == "VAR") {
        string command = "LOAD " + to_string(begin.memory);
        pushCommand(command);
    }else{
        if(end.type =="TAB"){
            Array temp = arrays.top();
            arrays.pop();
            if(arrays.top().index.type=="NUM"){
                string command1 = "LOAD " + to_string( arrays.top().index.memory);
                pushCommand(command1);
            }
            else{
                generateNumber(to_string(begin.begin));
                pushCommand("STORE 7");
                generateNumber(to_string(begin.memory+1));
                pushCommand("ADD "+ to_string(arrays.top().index.memory));
                pushCommand("SUB 7");
                pushCommand("LOADI 0");
            }
            arrays.push(temp);
        }else{
            if(arrays.top().index.type=="NUM"){
                string command1 = "LOAD " + to_string( arrays.top().index.memory);
                pushCommand(command1);
            }
            else{
                generateNumber(to_string(begin.begin));
                pushCommand("STORE 7");
                generateNumber(to_string(begin.memory+1));
                pushCommand("ADD "+ to_string(arrays.top().index.memory));
                pushCommand("SUB 7");
                pushCommand("LOADI 0");
            }
        }
    }


    pushCommand("STORE "+to_string(a.memory));
    identifiers.at(a.name).isInitialized = true;
    subtraction(aa,cc, false, yylineno);

    Identifier temp;
    string name = "NUMBER" + to_string(loopNumber);
    createIdentifier(&temp, name, 1,  "VAR", 0, false);
    insertIdentifier(name, temp);

    pushCommand("STORE " + to_string(identifiers.at(name).memory));
    fors.push_back(identifiers.at(a.name));

    Jump j;
    createJump(&j, commands.size(), loopNumber);
    jumps.push_back(j);
    pushCommand("JNEG ");

}

void endDownFor(){
    Identifier iterator = fors.at(fors.size()-1);
    pushCommand("LOAD "+to_string(iterator.memory));
    pushCommand("DEC");
    pushCommand("STORE "+to_string(iterator.memory));
    long long int numberOfJumps = jumps.size()-1;
    pushCommand("JUMP "+to_string(jumps.at(numberOfJumps).position-3));

    commands.at(jumps.at(numberOfJumps).position) = commands.at(jumps.at(numberOfJumps).position) + to_string(commands.size());
    jumps.pop_back();

    string name = "NUMBER" + to_string(loopNumber);
    identifiers.erase(name);
    memoryIterator--;
    identifiers.erase(iterator.name);
    memoryIterator--;
    fors.pop_back();
    loopNumber--;
}



void generateNumber(string number){
    long long int n = stoll(number);

    if(n<0){
        n=-n;
        string binary = decToBin(n);
    	for(long long int i = 0; i < binary.size(); ++i){
            if(i==0){
                pushCommand("SUB 0");
            }
    		if(binary[i] == '1'){
    			pushCommand("DEC");
    		}
    		if(i < (binary.size() - 1)){
    	        pushCommand("SHIFT 1");
    		}
    	}
    } else {
        string binary = decToBin(n);
    	for(long long int i = 0; i < binary.size(); ++i){
            if(i==0){
                pushCommand("SUB 0");
            }
    		if(binary[i] == '1'){
    			pushCommand("INC");
    		}
    		if(i < (binary.size() - 1)){
    	        pushCommand("SHIFT 1");
    		}
    	}
    }
}

string decToBin(long long int decimal) {
    string binary;
    long long int i=0;
    while(decimal!=0){
        ++i;
        if (decimal%2==0){
            binary = "0"+binary;
        }
        else{
            binary = "1"+binary;
        }
        decimal=decimal/2;
    }
    if(i!=0){
        return binary;
    } else{
        binary = "0";
        return binary;
    }

}

void pushCommand(string command) {
    commands.push_back(command);
}

void writeFlagState(bool state){
    writeFlag=state;
}

void assignFlagState(bool state){
    assignFlag=state;
}

void increaseLoopNumber(){
    loopNumber=loopNumber+1;
}


void print(char* out) {
    ofstream file;
    file.open(out);

    for (long long int cmd = 0; cmd < commands.size(); cmd++) {
        file << commands.at(cmd) << endl;
    }
    file.close();
}
