import re
from pathlib import Path


def parse_graphs(path):
    text = Path(path).read_text(encoding='utf-8')
    lines = [line.strip() for line in text.splitlines()]
    graphs = []
    current = None
    id_re = re.compile(r'^ID\d+')
    node_re = re.compile(r'^(\d+)\s*[-–]\s*(.*)$')
    for line in lines:
        if not line:
            continue
        if id_re.match(line):
            if current is not None:
                graphs.append(current)
            current = {'id': line, 'lines': []}
        else:
            m = node_re.match(line)
            if m:
                node = int(m.group(1))
                neighbors_raw = m.group(2)
                current['lines'].append((node, neighbors_raw))
            else:
                # ignore other lines
                pass
    if current is not None:
        graphs.append(current)
    return graphs


def build_graph(lines):
    adj = {}
    for node, neighbors_raw in lines:
        neighbors = [int(x) for x in re.findall(r'\d+', neighbors_raw)]
        adj.setdefault(node, set()).update(neighbors)
        for nbr in neighbors:
            adj.setdefault(nbr, set()).add(node)
    # ensure all nodes present
    nodes = sorted(adj)
    return {u: set(v for v in adj[u] if v != u) for u in nodes}


def greedy_coloring(adj, order=None):
    if order is None:
        order = sorted(adj, key=lambda u: (-len(adj[u]), u))
    color = {}
    for u in order:
        used = {color[v] for v in adj[u] if v in color}
        c = 0
        while c in used:
            c += 1
        color[u] = c
    return color


def can_color_k(adj, k, order):
    n = len(order)
    color = {}
    domains = {u: set(range(k)) for u in adj}

    def search(idx, max_color_used):
        if idx == n:
            return color.copy()
        u = order[idx]
        forbidden = {color[v] for v in adj[u] if v in color}
        for c in range(k):
            if c in forbidden:
                continue
            if c >= k:
                continue
            # bound: if using colors beyond k-1 not allowed
            color[u] = c
            if max(max_color_used, c) + 1 > k:
                del color[u]
                continue
            result = search(idx + 1, max(max_color_used, c))
            if result is not None:
                return result
            del color[u]
        return None

    return search(0, -1)


def exact_chromatic(adj):
    nodes = sorted(adj, key=lambda u: (-len(adj[u]), u))
    # compute clique lower bound via greedy heuristic
    clique = []
    for u in nodes:
        if all(u in adj[v] for v in clique):
            clique.append(u)
    lb = len(clique)
    ub = len(nodes)
    # attempt increasing k from lower bound
    for k in range(lb, ub + 1):
        coloring = can_color_k(adj, k, nodes)
        if coloring is not None:
            return k, coloring
    return ub, greedy_coloring(adj, nodes)


def main():
    graphs = parse_graphs('Chromatic 9 Graphs.txt')
    print(f'Parsed {len(graphs)} graphs')
    results = []
    for graph in graphs:
        adj = build_graph(graph['lines'])
        k, coloring = exact_chromatic(adj)
        results.append((graph['id'], len(adj), k, coloring))
    with open('chromatic_results.txt', 'w', encoding='utf-8') as f:
        f.write(f'Parsed {len(graphs)} graphs\n')
        for graph_id, n, k, coloring in results:
            f.write(f'{graph_id}: n={n}, chromatic={k}\n')
            f.write('coloring: ' + str(dict(sorted(coloring.items()))) + '\n')
    for graph_id, n, k, coloring in results:
        print(f'{graph_id}: n={n}, chromatic={k}')
        print('coloring:', dict(sorted(coloring.items())))
    print('Saved results to chromatic_results.txt')

if __name__ == '__main__':
    main()
