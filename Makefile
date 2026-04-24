TARGET = pacman_game
$(TARGET): main.o Map.o Player.o
	g++ -pedantic-errors -std=c++11 main.o Map.o Player.o -o $(TARGET)

main.o: main.cpp Map.h Player.h
	g++ -pedantic-errors -std=c++11 -c main.cpp

Map.o: Map.cpp Map.h
	g++ -pedantic-errors -std=c++11 -c Map.cpp

Player.o: Player.cpp Player.h Map.h
	g++ -std=c++11 -c Player.cpp
clean:
	rm -f *.o $(TARGET)