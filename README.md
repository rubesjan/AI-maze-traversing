# Maze traversing using BFS, DFS, Random search, Greedy search and A*

Implementation of BFS, DFS, Random search, Greedy search and A* algorithms for finding path in a maze. Algorithms differ only in the order of expanding new nodes - they only use different containers for storing opened nodes.

Maze must be in a .txt file. Walls should be 'X' and paht must be a whitespace (' '). Maze must be bounded by walls!

IMPLEMENTATION DOES NOT CHECK FOR INVALID MAZE, but maze without any path is valid.

usage:<br>
./a.out filename.txt {BFS, DFS, Random, Greedy, A*}

compile:<br>
g++ main.cpp

example usage:<br>
./a.out ./example/72.txt BFS
