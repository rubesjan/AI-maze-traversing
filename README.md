# Maze traversing using BFS, DFS, Random search, Greedy search, A* algorithms

Implementation of BFS, DFS, Random search, Greedy search and A* algorithms for finding path in a maze. Algorithms are same, only uses different container for storing opened nodes. Algorithms differs only in the order of expanding new nodes.

Maze must be in a txt file. Walls should be 'X' and paht must a whitespace (' '). Maze must be bounded by walls!

IMPLEMENTATION DOES NOT CHECK FOR INVALID MAZES, but mazes without any path are valid.

usage:<br>
./a.out filename.txt {BFS, DFS, Random, Greedy, A*}

compile:<br>
g++ main.cpp

example usage:<br>
./a.out ./example/72.txt BFS
