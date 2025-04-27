#include <bits/stdc++.h>
using namespace std;
const int N = 55;
int n, m;

// 先定义 E，确保后续使用时不报错
struct E {
    int from, to, w;
    E(int a, int b, int c) : from(a), to(b), w(c) {}
    bool operator < (const E& b) const { return w > b.w; }
    bool operator == (const E& b) const { return w == b.w; }
};

template<class T>
class deletable_heap {
    priority_queue<T> origin_heap;
    priority_queue<T> delete_heap;
public:
    void push(T item) { origin_heap.push(item); }
    void del(T item) { delete_heap.push(item); }
    T get() {
        while (!origin_heap.empty() && !delete_heap.empty() &&
            origin_heap.top() == delete_heap.top()) {
            origin_heap.pop(), delete_heap.pop();
        }
        return origin_heap.top();
    }
};

struct node {
    int pos, w, est, rest_nodes_num;
    vector<int> route;
    deletable_heap<E> dh;
    node(int p, int weight, int estimate, deletable_heap<E> heap, int rest, vector<int> route)
        : pos(p), w(weight), est(estimate), dh(heap), rest_nodes_num(rest), route(route) {}
    bool operator < (const node& b) const { return est > b.est; }
};

vector<E> edge[N];
priority_queue<node> q;
deletable_heap<E> dh;
bool check(const vector<int>& route, int nxt, int rest_num)
{
    if (find(route.begin(), route.end(), nxt) == route.end())
        return true;
    else if (nxt == 1 && rest_num == 1)
        return true;
    return false;
}

void print_route(const vector<int>& route)
{
    cout << route[0];
    for (int i = 1; i < route.size(); i++)
    {
        cout << "-->" << route[i];
    }
    cout << endl;
}

int bfs() {
    q.push(node(1, 0, n * dh.get().w, dh, n, {1}));  // 注意参数顺序
    bool sign = false;
    while (!q.empty()) {
        auto u = q.top();
        q.pop();
        int pos = u.pos;
        int w = u.w;
        int rest_num = u.rest_nodes_num;
        auto current_dh = u.dh;  // 避免命名冲突
        auto route = u.route;
        cout << "current node:" << pos << " sum:" << w << " rest_num:" << rest_num << endl;
        // cout << fa << "-->" << pos << "sum:" << w << endl;
        if (pos == 1 && sign && rest_num == 0)
        {
            cout << "one of the shortest routes:" << endl;
            print_route(route);
            return w;
        }
        else if (pos == 1) sign = true;
        for (auto e : edge[pos]) {
            int to = e.to, val = e.w;
            if (!check(route, to, rest_num)) continue;
            auto tmp = current_dh;
            tmp.del(E{ pos, to, val });
            tmp.del(E{ to, pos, val });
            int min_w = tmp.get().w;
            int new_est = min_w * (rest_num - 1) + val + w;
            auto new_route = route;
            new_route.push_back(to);
            q.push(node{ to, w + val, new_est, tmp, rest_num - 1, new_route});
            cout << pos << "-->" << to << " sum:" << w + val << endl;
        }
    }
    return -1;  // 未找到路径
}

int main() {
    cin >> n >> m;
    for (int i = 1; i <= m; i++) {
        int a, b, w; cin >> a >> b >> w;
        edge[a].push_back({ a, b, w });
        edge[b].push_back({ b, a, w });
        dh.push({ a, b, w });
        dh.push({ b, a, w });
    }
    cout << bfs() << endl;
    return 0;
}
/*
5 8
1 2 1
2 3 7
3 4 8
4 5 9
5 1 5
1 3 6
2 5 1
2 4 1

*/