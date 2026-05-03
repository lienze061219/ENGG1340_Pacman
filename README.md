ENGG1340 - Pac-Man Console Game
Team Members
Li Enze (lienze061219)  

Ye Zian(yza-andrew)
  
Wang Zihe(Austin1378)

Yang Chenyu(ycygoodgood)

Li Yang(Jimmy88641)

He Jinliang(rudeus6969)

Project Overview
This project is a console-based Pac-Man game developed in C++. It focuses on handling real-time keyboard input and dynamic map rendering within a Unix-like terminal. The core of the project demonstrates the practical application of Object-Oriented Programming (OOP), standard data structures, and fundamental search algorithms.

Core Features
Dynamic Level Loading: Supports loading custom maze layouts from external .txt files (e.g., map_small.txt, map_large.txt).

Game Modes: Features a Normal Mode and an Endless Mode, where pellets automatically respawn upon clearing the map.

Interaction System: Includes standard W/A/S/D movement, a shooting mechanic to destroy specific walls, and "Super Bean" power-ups.

Ghost AI: Includes two distinct behaviors: random movement and active chasing using a Breadth-First Search (BFS) algorithm.

Score Persistence: Saves and retrieves high scores using a local leaderboard.txt file.

Technical Implementation
The following technical elements are implemented to meet the project requirements and are highlighted in the demonstration video:

1. Modular Architecture (Classes & Headers)

Multi-file Structure: The project is organized into Map.h/cpp, Player.h/cpp, and Ghost.h/cpp to separate core logic from entity behaviors.

Encapsulation: This structure ensures that map rendering, player state, and AI pathfinding are handled in isolated, maintainable modules.

2. File Input/Output (fstream)

Map Loading: Uses std::ifstream to parse grid data from text files, allowing for flexible level design without recompiling code.

Data Persistence: Uses std::ofstream to write player scores to the leaderboard, ensuring progress is saved after the program exits.

3. Data Structures & Algorithms

BFS Pathfinding: The ghost AI utilizes a std::queue and the BFS algorithm to calculate the shortest path to the player while avoiding wall obstacles.

STL Containers: A nested std::vector<std::vector<char>> is used to manage the map grid, supporting O(1) access and real-time updates (e.g., removing a pellet when eaten).

4. Terminal Rendering & Interaction

Non-blocking Input: Integrates <termios.h> and <unistd.h> to capture keystrokes instantly without requiring the user to press 'Enter'.

ANSI Escape Codes: Employs terminal escape sequences for colored output and cursor control to enhance the visual clarity of the console interface.

Compilation and Execution
1. Prerequisites

Compiler: g++ (supporting C++11 or higher).

Environment: macOS or Linux terminal.

2. Build Instructions

Using the provided Makefile:

Bash
make
3. Running the Game

Bash
./pacman_game
Controls: W/A/S/D to move, Space to shoot, and Q to quit.

Note on Contributions: Due to different local Git configurations across multiple workstations, team members may appear under multiple nicknames (e.g., ycygoodgood, Justin666, Rudeus69, 李洋, etc.) in the commit history; these represent the combined efforts of the four core team members.