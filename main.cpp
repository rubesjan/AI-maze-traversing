#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <map>
#include <queue>
#include <list>
#include <stack>
#include <set>
#include <random>
#include <algorithm>

#include <chrono>
#include <thread>

#include <unistd.h>

#define CLOSED '.'          // char for closed nodes
#define OPENED 'o'          // char for opened nodes
#define USED '.'            // char for already used nodes
#define LINES_PER_WINDOW 53 // number of lines in shell in one window
constexpr int FPS = 20;     // number of frames per second
#define PRINT_PATH          // comment this line to not print found path
#define PRINT_FLOODING      // comment this line to not print flooding

constexpr int DELAY = 1000 / FPS;

using namespace std;

struct Maze
{
    struct Coordinates
    {
        Coordinates() = default;
        Coordinates(unsigned int xx, unsigned int yy)
            : x(xx), y(yy) {}

        vector<Coordinates> neighbours() const
        {
            return vector<Coordinates>{Coordinates(x + 1, y), Coordinates(x, y + 1), Coordinates(x - 1, y), Coordinates(x, y - 1)};
        }

        int x, y;
    };
    Maze() {}
    vector<string> maze;
    vector<string> actual_maze;
    Coordinates start, end;
};

using coordinates = Maze::Coordinates;

ostream &operator<<(ostream &os, const vector<string> &maze)
{
    for (const string &line : maze)
        os << line << '\n';

    os.flush();
    return os;
}

ostream &operator<<(ostream &os, const Maze &maze)
{
    // spacing in terminal
    for (int i = 0; i < (LINES_PER_WINDOW - int(maze.actual_maze.size())); i++)
        os << '\n';

    os.flush();

    // print actual maze
    os << maze.actual_maze;

    // wait before printing another frame
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY));
    return os;
}

bool operator<(const coordinates &a, const coordinates &b)
{
    if (a.x != b.x)
        return a.x < b.x;
    return a.y < b.y;
}

bool operator==(const coordinates &a, const coordinates &b)
{
    if (a.x == b.x && a.y == b.y)
        return true;
    return false;
}

bool operator!=(const coordinates &a, const coordinates &b)
{
    return !(a == b);
}

ostream &operator<<(ostream &os, const coordinates &c)
{
    return os << "(" << c.x << ", " << c.y << ")";
}

string del_ws(string s)
{
    while (s.size() > 0 && isspace(*s.rbegin()))
        s.pop_back();
    return s;
}

Maze load_map(const string &filename)
{
    ifstream in;
    in.open(filename, std::ios::in);

    if (!in.good() || !in.is_open())
        throw invalid_argument("Can not open file: \""s + filename + "\"");

    Maze maze;
    string line;
    while (getline(in, line))
    {
        if (!in.good())
        {
            in.close();
            throw std::ios_base::failure("error when reading line from: \""s + filename + "\"");
        }

        // start point is on input;
        if (line[0] != 'X')
            break;

        maze.maze.emplace_back(del_ws(line));
    }

    // copy the maze for visualization purposes
    for (string line : maze.maze)
        maze.actual_maze.emplace_back(move(line));

    // get start point
    auto comma = line.find(',');
    maze.start = coordinates(stoi(line.substr(5, line.length() - 5)),
                             stoi(line.substr(comma + 1, line.length() - comma - 1)));

    // get end point
    getline(in, line);
    comma = line.find(',');
    maze.end = coordinates(stoi(line.substr(3, line.length() - 3)),
                           stoi(line.substr(comma + 1, line.length() - comma - 1)));
    // close the file
    in.close();

    return maze;
}

class COpenedNodes
{
public:
    COpenedNodes() = default;
    ~COpenedNodes() = default;
    virtual coordinates get() = 0;
    virtual void push(coordinates c, unsigned long distance) = 0;
    virtual bool empty() const = 0;
};

class DFSNodes : public COpenedNodes
{
public:
    DFSNodes() = default;
    ~DFSNodes() = default;

    virtual coordinates get() override
    {
        coordinates tmp = s.top();
        s.pop();
        return tmp;
    }

    virtual void push(coordinates c, unsigned long distance) override
    {
        s.emplace(move(c));
    }

    virtual bool empty() const override
    {
        return s.empty();
    }

private:
    stack<coordinates> s;
};

class BFSNodes : public COpenedNodes
{
public:
    BFSNodes() = default;
    ~BFSNodes() = default;

    virtual coordinates get() override
    {
        coordinates tmp = q.front();
        q.pop();
        return tmp;
    }

    virtual void push(coordinates c, unsigned long distance) override
    {
        q.emplace(move(c));
    }

    virtual bool empty() const override
    {
        return q.empty();
    }

private:
    queue<coordinates> q;
};

class RandomNodes : public COpenedNodes
{
public:
    RandomNodes() = default;
    ~RandomNodes() = default;

    virtual coordinates get() override
    {
        int it = rand() % v.size();
        coordinates tmp = v[it];
        v.erase(v.begin() + it);
        return tmp;
    }

    virtual void push(coordinates c, unsigned long distance) override
    {
        v.emplace_back(move(c));
    }

    virtual bool empty() const override
    {
        return v.empty();
    }

private:
    vector<coordinates> v;
};

class GreedyNodes : public COpenedNodes
{
public:
    GreedyNodes() = delete;
    GreedyNodes(coordinates c)
        : end(c) {}

    ~GreedyNodes() = default;

    virtual coordinates get() override
    {
        coordinates tmp = heap.front().coord;
        pop_heap(heap.begin(), heap.end());
        heap.pop_back();
        return tmp;
    }

    virtual void push(coordinates c, unsigned long distance) override
    {
        // Euclidean distance:
        // unsigned long dist = (c.x - end.x) * (c.x - end.x) + (c.y - end.y) * (c.y - end.y);

        // Manhattan distance:
        unsigned long dist = abs(c.x - end.x) + abs(c.y - end.y);

        heap.push_back(HeapElem(c, dist));
        push_heap(heap.begin(), heap.end());
    }

    virtual bool empty() const override
    {
        return heap.empty();
    }

private:
    struct HeapElem
    {
        HeapElem() = delete;
        HeapElem(coordinates c, unsigned long dist)
            : distance(dist), coord(c) {}
        ~HeapElem() = default;

        bool operator<(const HeapElem &other)
        {
            return this->distance > other.distance; // reversed for creating min heap
        }

        unsigned long distance;
        coordinates coord;
    };

    coordinates end;
    vector<HeapElem> heap;
};

class AStarNodes : public COpenedNodes
{
public:
    AStarNodes() = delete;

    AStarNodes(coordinates c)
        : end(c) {}

    ~AStarNodes() = default;

    virtual coordinates get() override
    {
        coordinates tmp = heap.front().coord;
        pop_heap(heap.begin(), heap.end());
        heap.pop_back();
        return tmp;
    }

    virtual void push(coordinates c, unsigned long distance) override
    {
        // Euclidean distance:
        // unsigned long tmp = sqrt((c.x - end.x) * (c.x - end.x) + (c.y - end.y) * (c.y - end.y));
        // tmp += distance;

        // Manhattan distance:
        unsigned long tmp = abs(c.x - end.x) + abs(c.y - end.y) + distance;

        heap.push_back(HeapElem(c, tmp, distance));
        push_heap(heap.begin(), heap.end());
    }

    virtual bool empty() const override
    {
        return heap.empty();
    }

private:
    struct HeapElem
    {
        HeapElem() = delete;

        HeapElem(coordinates c, unsigned long dist, unsigned long dist_start)
            : val(dist), coord(c), dist_from_start(dist_start) {}

        ~HeapElem() = default;

        bool operator<(const HeapElem &other)
        {
            // reversed for creating min heap
            if (this->val != other.val)
                return this->val > other.val;
            return this->dist_from_start > other.dist_from_start;
        }

        unsigned long val;
        coordinates coord;
        unsigned long dist_from_start;
    };

    coordinates end;
    vector<HeapElem> heap;
};

enum class CAlgorithm
{
    BFS,
    DFS,
    Random,
    Greedy,
    Astar
};

map<coordinates, coordinates> find_path_alg(Maze &maze, COpenedNodes &opened)
{
    if (maze.start == maze.end)
    {
        map<coordinates, coordinates> tmp;
        tmp[maze.start] = coordinates(-1, -1);
        return tmp;
    }

    map<coordinates, coordinates> pred;
    map<coordinates, unsigned> dist;

    pred[maze.start] = coordinates(-1, -1);
    dist[maze.start] = 0;

    maze.actual_maze[maze.start.y][maze.start.x] = 'S';
    maze.actual_maze[maze.end.y][maze.end.x] = 'E';

    opened.push(maze.start, 0);

    while (!opened.empty())
    {
        coordinates from = opened.get();
        for (const coordinates &n : from.neighbours())
        {
            if (isspace(maze.maze[n.y][n.x]) && pred.find(n) == pred.end())
            {
                pred[n] = from;
                unsigned long distance = dist[from] + 1;
                dist[n] = distance;
                opened.push(n, distance);

                if (n == maze.end)
                    return pred;
#ifdef PRINT_FLOODING
                maze.actual_maze[n.y][n.x] = OPENED;
                cout << maze << endl;
#endif
            }
        }

#ifdef PRINT_FLOODING
        maze.actual_maze[from.y][from.x] = CLOSED;
        if (from == maze.start)
            maze.actual_maze[maze.start.y][maze.start.x] = 'S';
#endif
    }
    return pred;
}

map<coordinates, coordinates> find_path(Maze &maze, const CAlgorithm &alg)
{
    switch (alg)
    {
    case CAlgorithm::BFS:
    {
        BFSNodes tmp;
        return find_path_alg(maze, tmp);
    }
    case CAlgorithm::DFS:
    {
        DFSNodes tmp;
        return find_path_alg(maze, tmp);
    }
    case CAlgorithm::Random:
    {
        RandomNodes tmp;
        return find_path_alg(maze, tmp);
    }
    case CAlgorithm::Greedy:
    {
        GreedyNodes tmp(maze.end);
        return find_path_alg(maze, tmp);
    }
    case CAlgorithm::Astar:
    {
        AStarNodes tmp(maze.end);
        return find_path_alg(maze, tmp);
    }
    }
    return map<coordinates, coordinates>(); // only to not get warning when compiling, above are all cases
}

int print_path(Maze &maze, map<coordinates, coordinates> &pred)
{
    // paht not found:
    if (pred.find(maze.end) == pred.end())
    {
        cout << "Path not found" << endl;
        return 1;
    }

#ifdef PRINT_PATH
    for (int i = 0; i < LINES_PER_WINDOW; i++)
        cout << "\n";
    cout.flush();
#endif

    list<coordinates> path;
    coordinates p = maze.end;
    while (!(p == coordinates(-1, -1)))
    {
        path.push_back(p);
        p = pred[p];
    };

    cout << "Expanded: " << pred.size() << endl;
    cout << "Path length: " << path.size() - 1 << endl;

#ifdef PRINT_PATH
    path.reverse();
    auto it = path.begin();
    cout << *it;
    it++;
    for (; it != path.end(); it++)
        cout << " -> " << *it;
    cout << endl;

    // Print path in original maze
    for (const coordinates &node : path)
        maze.maze[node.y][node.x] = USED;

    // print start and end points
    maze.maze[maze.start.y][maze.start.x] = 'S';
    maze.maze[maze.end.y][maze.end.x] = 'E';

    // print the path in a maze
    cout << maze.maze;
#endif

    return 0;
}

string toupper(const string &s)
{
    string tmp;
    tmp.reserve(s.size());
    for (char c : s)
        tmp.push_back(toupper(c));
    return tmp;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        cerr << "Usage: ./test/72.txt {BFS, DFS, Random, Greedy, A*}" << endl;
        return 1;
    }

    // random state for random search
    srand(time(0));

    string filename(argv[1]);
    Maze maze;
    try
    {
        maze = load_map(filename);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }

    string alg(argv[2]);
    alg = toupper(alg);

    map<coordinates, coordinates> pred;
    if (alg == "BFS")
        pred = find_path(maze, CAlgorithm::BFS);
    else if (alg == "DFS")
        pred = find_path(maze, CAlgorithm::DFS);
    else if (alg == "RANDOM")
        pred = find_path(maze, CAlgorithm::Random);
    else if (alg == "GREEDY")
        pred = find_path(maze, CAlgorithm::Greedy);
    else if (alg == "A*")
        pred = find_path(maze, CAlgorithm::Astar);
    else
    {
        cerr << "Unkown algorithm, expected {BFS, DFS, Random, Greedy, A*}" << endl;
        return 1;
    }

    return print_path(maze, pred);
}