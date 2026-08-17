
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/vf2_sub_graph_iso.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <utility>
#include <iomanip>
#include <algorithm>

using namespace std;

// ------------------------------------------------------------
// Graph type
// ------------------------------------------------------------
using Graph = boost::adjacency_list<
    boost::vecS,
    boost::vecS,
    boost::undirectedS
>;

// ------------------------------------------------------------
// Structure for one graph from CSV
// ------------------------------------------------------------
struct GraphRecord {
    long long generated_id;
    int n_nodes;
    int n_edges;
    int chromatic_number;

    string edge_list;
    string adjacency_upper_triangular;

    Graph graph;
};

// ------------------------------------------------------------
// Split CSV line
// The uploaded CSV files do not contain quoted commas inside
// fields, so a simple CSV split is sufficient.
// ------------------------------------------------------------
vector<string> split_csv(const string& line) {

    vector<string> fields;
    string field;
    stringstream ss(line);

    while (getline(ss, field, ',')) {
        fields.push_back(field);
    }

    return fields;
}

// ------------------------------------------------------------
// Remove surrounding quotation marks if present
// ------------------------------------------------------------
string remove_quotes(string s) {

    if (s.size() >= 2 &&
        s.front() == '"' &&
        s.back() == '"') {

        s = s.substr(1, s.size() - 2);
    }

    return s;
}

// ------------------------------------------------------------
// Build graph from edge_list
//
// Example:
// 0-1;0-2;1-3;2-3
// ------------------------------------------------------------
Graph build_graph_from_edge_list(
    int n_nodes,
    const string& edge_list
) {

    Graph g(n_nodes);

    if (edge_list.empty())
        return g;

    stringstream ss(edge_list);
    string edge;

    while (getline(ss, edge, ';')) {

        if (edge.empty())
            continue;

        size_t dash = edge.find('-');

        if (dash == string::npos)
            continue;

        int u = stoi(edge.substr(0, dash));
        int v = stoi(edge.substr(dash + 1));

        if (u >= 0 && u < n_nodes &&
            v >= 0 && v < n_nodes &&
            u != v) {

            add_edge(u, v, g);
        }
    }

    return g;
}

// ------------------------------------------------------------
// Read CSV
// ------------------------------------------------------------
vector<GraphRecord> read_csv(const string& filename) {

    vector<GraphRecord> graphs;

    ifstream file(filename);

    if (!file.is_open()) {
        cerr << "ERROR: Cannot open file: "
             << filename << endl;
        exit(1);
    }

    string line;

    // Header
    getline(file, line);

    while (getline(file, line)) {

        if (line.empty())
            continue;

        vector<string> fields = split_csv(line);

        if (fields.size() < 6) {
            cerr << "WARNING: Invalid CSV row. Skipping.\n";
            continue;
        }

        GraphRecord record;

        try {

            record.generated_id =
                stoll(remove_quotes(fields[0]));

            record.n_nodes =
                stoi(remove_quotes(fields[1]));

            record.n_edges =
                stoi(remove_quotes(fields[2]));

            record.chromatic_number =
                stoi(remove_quotes(fields[3]));

            record.edge_list =
                remove_quotes(fields[4]);

            record.adjacency_upper_triangular =
                remove_quotes(fields[5]);

            record.graph =
                build_graph_from_edge_list(
                    record.n_nodes,
                    record.edge_list
                );

            graphs.push_back(record);

        }
        catch (const exception& e) {

            cerr << "WARNING: Could not parse row: "
                 << e.what() << endl;
        }
    }

    file.close();

    return graphs;
}

// ------------------------------------------------------------
// Boost VF2 exact graph-isomorphism test
// ------------------------------------------------------------
bool boost_isomorphic(
    const Graph& g1,
    const Graph& g2
) {

    bool found = false;

    auto callback =
        [&](auto /*mapping1*/, auto /*mapping2*/) {

            found = true;

            // Stop immediately after first isomorphism
            return false;
        };

    boost::vf2_graph_iso(
        g1,
        g2,
        callback
    );

    return found;
}

// ------------------------------------------------------------
// Main
// ------------------------------------------------------------
int main(int argc, char* argv[]) {

    if (argc != 3) {

        cerr << "Usage:\n";
        cerr << "./isomorphism_check train.csv test.csv\n";

        return 1;
    }

    string train_file = argv[1];
    string test_file  = argv[2];

    cout << "====================================================\n";
    cout << "        BOOST VF2 GRAPH ISOMORPHISM CHECK\n";
    cout << "====================================================\n\n";

    // --------------------------------------------------------
    // Read datasets
    // --------------------------------------------------------

    cout << "Reading training graphs...\n";

    vector<GraphRecord> train_graphs =
        read_csv(train_file);

    cout << "Training graphs: "
         << train_graphs.size()
         << "\n\n";

    cout << "Reading test graphs...\n";

    vector<GraphRecord> test_graphs =
        read_csv(test_file);

    cout << "Test graphs: "
         << test_graphs.size()
         << "\n\n";

    // ========================================================
    // TEST vs TRAIN
    // ========================================================

    cout << "====================================================\n";
    cout << "TEST vs TRAIN\n";
    cout << "====================================================\n";

    string train_output =
        "test_vs_train_isomorphism_results.csv";

    ofstream train_csv(train_output);

    train_csv
        << "test_id,"
        << "train_id,"
        << "test_n_nodes,"
        << "train_n_nodes,"
        << "test_n_edges,"
        << "train_n_edges,"
        << "boost_checked,"
        << "isomorphic\n";

    long long total_train_comparisons = 0;
    long long actual_boost_train_checks = 0;
    long long train_isomorphic_pairs = 0;

    for (size_t i = 0; i < test_graphs.size(); ++i) {

        const auto& test = test_graphs[i];

        cout << "Test graph "
             << test.generated_id
             << " (" << i + 1
             << "/" << test_graphs.size()
             << ")";

        long long matches_for_test = 0;

        for (size_t j = 0;
             j < train_graphs.size();
             ++j) {

            const auto& train = train_graphs[j];

            total_train_comparisons++;

            bool iso = false;
            bool boost_checked = false;

            // ------------------------------------------------
            // Quick necessary-condition filtering
            //
            // Two isomorphic graphs MUST have:
            //   same number of vertices
            //   same number of edges
            //
            // This is not the actual isomorphism test.
            // ------------------------------------------------
            if (test.n_nodes == train.n_nodes &&
                test.n_edges == train.n_edges) {

                boost_checked = true;

                actual_boost_train_checks++;

                iso = boost_isomorphic(
                    test.graph,
                    train.graph
                );
            }

            if (iso) {

                train_isomorphic_pairs++;
                matches_for_test++;
            }

            train_csv
                << test.generated_id << ","
                << train.generated_id << ","
                << test.n_nodes << ","
                << train.n_nodes << ","
                << test.n_edges << ","
                << train.n_edges << ","
                << (boost_checked ? 1 : 0) << ","
                << (iso ? 1 : 0)
                << "\n";
        }

        cout << " -> isomorphic training graphs: "
             << matches_for_test
             << "\n";
    }

    train_csv.close();

    // ========================================================
    // TEST vs TEST
    // ========================================================

    cout << "\n";
    cout << "====================================================\n";
    cout << "TEST vs TEST\n";
    cout << "====================================================\n";

    string test_output =
        "test_vs_test_isomorphism_results.csv";

    ofstream test_csv(test_output);

    test_csv
        << "test_id_1,"
        << "test_id_2,"
        << "n_nodes_1,"
        << "n_nodes_2,"
        << "n_edges_1,"
        << "n_edges_2,"
        << "boost_checked,"
        << "isomorphic\n";

    long long total_test_comparisons = 0;
    long long actual_boost_test_checks = 0;
    long long test_isomorphic_pairs = 0;

    for (size_t i = 0;
         i < test_graphs.size();
         ++i) {

        for (size_t j = i + 1;
             j < test_graphs.size();
             ++j) {

            const auto& g1 = test_graphs[i];
            const auto& g2 = test_graphs[j];

            total_test_comparisons++;

            bool iso = false;
            bool boost_checked = false;

            // Necessary conditions
            if (g1.n_nodes == g2.n_nodes &&
                g1.n_edges == g2.n_edges) {

                boost_checked = true;

                actual_boost_test_checks++;

                iso = boost_isomorphic(
                    g1.graph,
                    g2.graph
                );
            }

            if (iso) {
                test_isomorphic_pairs++;
            }

            test_csv
                << g1.generated_id << ","
                << g2.generated_id << ","
                << g1.n_nodes << ","
                << g2.n_nodes << ","
                << g1.n_edges << ","
                << g2.n_edges << ","
                << (boost_checked ? 1 : 0) << ","
                << (iso ? 1 : 0)
                << "\n";
        }
    }

    test_csv.close();

    // ========================================================
    // FIND NOVEL TEST GRAPHS
    // ========================================================

    vector<long long> novel_test_ids;

    for (const auto& test : test_graphs) {

        bool exists_in_training = false;

        for (const auto& train : train_graphs) {

            // Only possible if these invariants match
            if (test.n_nodes != train.n_nodes ||
                test.n_edges != train.n_edges) {

                continue;
            }

            if (boost_isomorphic(
                    test.graph,
                    train.graph)) {

                exists_in_training = true;
                break;
            }
        }

        if (!exists_in_training) {

            novel_test_ids.push_back(
                test.generated_id
            );
        }
    }

    // ========================================================
    // SUMMARY
    // ========================================================

    string summary_output =
        "isomorphism_summary.csv";

    ofstream summary_csv(summary_output);

    summary_csv << "metric,value\n";

    summary_csv
        << "training_graphs,"
        << train_graphs.size()
        << "\n";

    summary_csv
        << "test_graphs,"
        << test_graphs.size()
        << "\n";

    summary_csv
        << "test_vs_train_total_comparisons,"
        << total_train_comparisons
        << "\n";

    summary_csv
        << "test_vs_train_actual_boost_checks,"
        << actual_boost_train_checks
        << "\n";

    summary_csv
        << "test_vs_train_isomorphic_pairs,"
        << train_isomorphic_pairs
        << "\n";

    summary_csv
        << "test_vs_test_total_comparisons,"
        << total_test_comparisons
        << "\n";

    summary_csv
        << "test_vs_test_actual_boost_checks,"
        << actual_boost_test_checks
        << "\n";

    summary_csv
        << "test_vs_test_isomorphic_pairs,"
        << test_isomorphic_pairs
        << "\n";

    summary_csv
        << "novel_test_graphs,"
        << novel_test_ids.size()
        << "\n";

    summary_csv.close();

    // ========================================================
    // PRINT FINAL RESULTS
    // ========================================================

    cout << "\n";
    cout << "====================================================\n";
    cout << "FINAL RESULTS\n";
    cout << "====================================================\n";

    cout << "\nTraining graphs              : "
         << train_graphs.size();

    cout << "\nTest graphs                  : "
         << test_graphs.size();

    cout << "\n";

    cout << "\nTEST vs TRAIN\n";

    cout << "Total possible comparisons  : "
         << total_train_comparisons;

    cout << "\nActual Boost checks         : "
         << actual_boost_train_checks;

    cout << "\nIsomorphic pairs             : "
         << train_isomorphic_pairs;

    cout << "\n";

    cout << "\nTEST vs TEST\n";

    cout << "Total possible comparisons  : "
         << total_test_comparisons;

    cout << "\nActual Boost checks         : "
         << actual_boost_test_checks;

    cout << "\nIsomorphic pairs             : "
         << test_isomorphic_pairs;

    cout << "\n";

    cout << "\nNovel test graphs            : "
         << novel_test_ids.size();

    cout << "\n";

    cout << "\nNovel test graph IDs:\n";

    for (long long id : novel_test_ids) {
        cout << id << " ";
    }

    cout << "\n";

    cout << "\n====================================================\n";
    cout << "OUTPUT FILES\n";
    cout << "====================================================\n";

    cout << "1. "
         << train_output
         << "\n";

    cout << "2. "
         << test_output
         << "\n";

    cout << "3. "
         << summary_output
         << "\n";

    cout << "\nDone.\n";

    return 0;
}
