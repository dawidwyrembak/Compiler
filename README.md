## Informacje
Kompilator na kurs JFTT w semestrze zimowym 2019/2020.
Projekt zawiera pliki:
* compiler.cpp - zawierający wywoływane instrukcje przez parser
* compiler.hpp - plik nagłówkowy 
* compiler.l - lexer
* compiler.y - parser
* Makefile - do zbudowania projektu
	
## Technologie
Wersje technologii wykorzystywanych w projekcie:
* Flex: 2.6.4
* Bison: 3.0.4
* g++: 7.4.0
* GNU Make: 4.1
	
## Jak użyć

```
$ make
$ ./compiler <plik_wejściowy> <plik_wyjściowy>
```
