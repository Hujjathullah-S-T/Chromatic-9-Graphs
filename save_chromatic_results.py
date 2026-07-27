from compute_chromatic import parse_graphs, build_graph, exact_chromatic


def main():
    graphs = parse_graphs('Chromatic 9 Graphs.txt')
    with open('chromatic_results.txt', 'w', encoding='utf-8') as f:
        f.write(f'Parsed {len(graphs)} graphs\n')
        for graph in graphs:
            adj = build_graph(graph['lines'])
            k, coloring = exact_chromatic(adj)
            f.write(f'{graph["id"]}: n={len(adj)}, chromatic={k}\n')
            f.write('coloring: ' + str(dict(sorted(coloring.items()))) + '\n')


if __name__ == '__main__':
    main()
