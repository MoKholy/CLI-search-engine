#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <sstream>
#include <iomanip>
using namespace std;

struct node {
    int ingoing = 0;
    int outgoing = 0;
    //double prnorm = 0;
    double pr;
    int impressions;
    double score =0;
    string title;
    int clicks;
    vector<string> keywords;
    vector<node> adjnodes_out;
    vector<node> adjnodes_in;
};
struct Edge {
    node start, end;
};
class Graph {
public:
    unordered_map <string, node> nodes;
    int max_pr = 0;
    int min_pr = 100000;
    Graph(vector<Edge>  &edges) {
        for (auto &edge: edges) {
            if (nodes.find(edge.start.title) == nodes.end()) {
                nodes[edge.start.title] = edge.start;
                nodes[edge.start.title].adjnodes_out.push_back(edge.end);
                if (nodes.find(edge.end.title) == nodes.end()) {
                    nodes[edge.end.title] = edge.end;
                    nodes[edge.end.title].ingoing++;
                }
                else {
                    nodes[edge.end.title].ingoing++;
                }
                nodes[edge.end.title].adjnodes_in.push_back(edge.start);
                nodes[edge.start.title].outgoing++;
            }
            else {
                nodes[edge.start.title].adjnodes_out.push_back(edge.end);
                nodes[edge.end.title].adjnodes_in.push_back(edge.start);
                nodes[edge.start.title].outgoing++;
                if (nodes.find(edge.end.title) == nodes.end()) {
                    nodes[edge.end.title] = edge.end;
                    nodes[edge.end.title].ingoing++;
                }
                else {
                    nodes[edge.end.title].ingoing++;
                }
            }
        }
        // normalizes pr
        calculatePR();
    }
    void calculatePR() {
        // initialize all pageranks to 1/n
        int n = nodes.size();
        unordered_map<string, double> old_iter;
        for (auto& node : nodes) {
            node.second.pr = (1.0/n);
            old_iter[node.first] = (1.0 / n);
        }
         //calculate all pageranks
        for (auto& node : nodes) {
            double page_rank = 0;
            for (auto& node2 : node.second.adjnodes_in) {
                page_rank += old_iter[node2.title] /(nodes[node2.title].outgoing);
            }
            node.second.pr = page_rank;
        }

    }
};


// initialize pages into graph 
Graph read_files() {

    // read hyperlinks
    ifstream links("hyperlinks.csv");
    string line;
    vector <Edge> edges;
    while (links.good()) {
        getline(links, line);
        string src, end;
        node src_n, end_n;
        
        stringstream s(line);
        getline(s, src, ',');
        getline(s, end);
        src_n.title = src;
        end_n.title = end;
        Edge edge;
        edge.start = src_n;
        edge.end = end_n;
        edges.push_back(edge);
    }
    links.close();
    Graph G(edges);
    // read keywords
    ifstream kw("keywords.csv");
    while (kw.good()) {
        getline(kw, line);
        string title, key;
        stringstream s(line);
        getline(s, title, ',');
        while (getline(s, key, ',')) {
            G.nodes[title].keywords.push_back(key);
        }
    }
    kw.close();

    // read impressions
    ifstream imp("impressions.csv");
    while (imp.good()) {
        getline(imp, line);
        string title;
        stringstream s(line);
        getline(s, title, ',');
        string impressions;
        getline(s, impressions);
        G.nodes[title].impressions = stoi(impressions);
    }
    imp.close();
    // read clicks
    ifstream clicks("clicks.csv");
    while (clicks.good()) {
        getline(clicks, line);
        string title;
        stringstream s(line);
        getline(s, title, ',');
        string c;
        getline(s, c);
        G.nodes[title].clicks = stoi(c);
    }
    clicks.close();
    

    return G;
}
Graph G = read_files();
//  score
void score(Graph& G) {
    for (auto it = G.nodes.begin(); it != G.nodes.end(); it++) {
        node nd = it->second;
        it->second.score = 0.4 * nd.pr + 0.6 * ((1 - ((0.1 * nd.impressions) / 1 + (0.1 * nd.impressions))) * nd.pr+ ((0.1 * nd.impressions) / (1 + 0.1 * nd.impressions)) * ((nd.clicks) / nd.impressions));
    }
}
//search query && update impressions
vector<string> search(string query, Graph& G) {
    vector<string> results;
    string and_ = "AND";
    string or_ = "OR";
    if (query[0] == '"' && query[query.size() - 1] == '"') {
        // search exact 
        string keyword = query.substr(1, query.size() - 2);
        for (auto& node : G.nodes) {
            if (find(node.second.keywords.begin(), node.second.keywords.end(), keyword) != node.second.keywords.end()) {
                results.push_back(node.first);
                // update impressions
                node.second.impressions++;
            }
        }
    }
    else if (query.find(and_) != string::npos) {
        stringstream s(query);
        string keyword;
        vector<string> keys;
        while (s >> keyword) {
            if (keyword != "AND") {
                keys.push_back(keyword);
            }
        }
        // loop over keyword 
        for (auto& node : G.nodes) {
            if (find(node.second.keywords.begin(), node.second.keywords.end(), keys[0]) != node.second.keywords.end() && find(node.second.keywords.begin(), node.second.keywords.end(), keys[1]) != node.second.keywords.end()) {
                results.push_back(node.first);
                // update impressions
                node.second.impressions++;
            }
        }
    }
    else{
        stringstream s(query);
        string keyword;
        vector<string> keys;
        while (s >> keyword) {
            if (keyword != "OR") {
                keys.push_back(keyword);
            }
        }
        // loop over keyword 
        for (auto& key : keys) {
            for (auto& node : G.nodes) {
                if (find(node.second.keywords.begin(), node.second.keywords.end(), key) != node.second.keywords.end()) {
                    results.push_back(node.first);
                    // update impressions
                    node.second.impressions++;
                }
            }
        }
    }
    return results;
}
//sorting
bool compare(string a, string b) {
    return G.nodes[a].score > G.nodes[b].score;
}
void sort(vector<string>& results, Graph G){
    sort(results.begin(), results.end(), compare);
}
// update clicks
void update_click(string s, Graph& G) {
    G.nodes[s].clicks++;
}

// update files

void writefiles(Graph& G) {
    // write impressions
    ofstream imp("impressions.csv");
    int j = 0;
    for (auto& node : G.nodes) {
        if (j != G.nodes.size() - 1) {
            imp << node.first << "," << node.second.impressions << endl;
            j++;
        }
        else {
            imp << node.first << "," << node.second.impressions;

        }
    }
    imp.close();
    // read clicks
    ofstream clicks("clicks.csv");
    j = 0;
    for (auto& node : G.nodes) {
        if (j != G.nodes.size() - 1) {
            clicks << node.first << "," << node.second.clicks << endl;
            j++;
        }
        else {
            clicks << node.first << "," << node.second.clicks;
        }
        
    }
    clicks.close();
}

// display results
void display(vector<string> results) {
    int i = 0;
    for (auto& result : results) {
        cout << i << ". " << result << endl;
    }
}

// testing functions
void displaygraph(Graph& G) {
    for (auto& node : G.nodes) {
        cout << node.first << ": " << endl;
        cout << "adj nodes: ";
        for (auto& adj : node.second.adjnodes_out) {
            cout << adj.title << "\t";
        }
        cout << "adj nodes in: ";
        for (auto& adj : node.second.adjnodes_in) {
            cout << adj.title << "\t";
        }
        cout << endl;
        cout << "clicks: " << node.second.clicks << endl;
        cout << "impressions: " << node.second.impressions << endl;
        cout << "outgoing: " << node.second.outgoing << endl;
        cout << "ingoing: " << node.second.ingoing << endl;
        cout << "pr: " << node.second.pr << endl;
        cout << "Score: " <<node.second.score << endl;
        cout << endl;
    }
}
// run function
void run() {
    
    cout << "Welcome" << endl;
    cout << "What would you like to do?" << endl;
    cout << "1. New Search" << endl;
    cout << "2. Exit" << endl;
    cout << "Type in your choice: ";
    bool flag = false;
    int count = 0;
    int num;
    cin >> num;
    while (!flag) {
        if (num == 1) {
            // initialize graph and read from files
            string query;
            cout << "Please enter keywords: ";
            getline(cin >> ws, query);
            cout << "Search results:" << endl;
            vector<string> results = search(query, G);
            sort(results, G);
            int i = 1;
            for (auto& result : results) {
                cout << i << ". " << result << endl;
                i++;
            }
            while(true){
                int num2;
                cout << "Would you like to" << endl;
                cout << "1. Choose a webpage to open" << endl;
                cout << "2. New Search" << endl;
                cout << "3. Exit " << endl;
                cin >> num2;
                if (num2 == 1) {
                    cout << "Type in your page choice: " << endl;
                    int num3;
                    cin >> num3;
                    bool flag2 = false;
                    while (!flag) {
                        if (num3 > results.size()) {
                            cout << "Please enter a valid page choice: ";
                            cin >> num3;
                        }
                        else if (num3 <= 0) {
                            cout << "Please enter a valid page choice: ";
                            cin >> num3;
                        }
                        else {
                            flag = true;
                        }
                    }
                    cout << "You are now viewing " << results[num3 - 1] << "." << endl;
                    update_click(results[num3 - 1], G);
                    cout << "Would you like to " << endl;
                    cout << "1. Back to search results" << endl;
                    cout << "2. New Search" << endl;
                    cout << "3. Exit " << endl;
                    if (num3 == 1) {
                        cout << "Search results:" << endl;
                        int i = 1;
                        for (auto& result : results) {
                            cout << i << ". " << result << endl;
                            i++;
                        }
                        continue;
                    }
                    else if (num3 == 2) {
                        break;
                    }
                    else {
                        writefiles(G);
                        flag = true;
                        break;
                    }
                    
                }
                else if (num2 == 2) {
                    break;
                }
                else if(num2==3){
                    flag = true;
                    writefiles(G);
                    break;
                }
                else {
                    cout << "Please enter a valid choice: ";
                    cin >> num2;
                }
            }
        }
        else if(num == 2) {
            flag = true;
        }
        else {
            cout << "Please enter a valid choice: " << endl;
            cin >> num;
        }
    }
}
int main() {
    run();

    return 0;
}