TARGET = pacman_game
$(TARGET): main.o Map.o
	g++ -pedantic-errors -std=c++11 main.o Map.o -o $(TARGET)

main.o: main.cpp Map.h
	g++ -pedantic-errors -std=c++11 -c main.cpp

Map.o: Map.cpp Map.h
	g++ -pedantic-errors -std=c++11 -c Map.cpp

clean:
	rm -f *.o $