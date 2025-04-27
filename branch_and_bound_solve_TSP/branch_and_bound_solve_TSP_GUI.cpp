#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/graphics.h>
#include <wx/dcbuffer.h>
#include <bits/stdc++.h>

using namespace std;

const int N = 55;
int n, m;

// Node drawing parameters
const int NODE_RADIUS = 20;  // 减小节点半径
const int NODE_SPACING = 150;

// Animation parameters
const int ANIMATION_DELAY_MS = 1000;

// Colors
const wxColour BACKGROUND_COLOR(240, 240, 240);
const wxColour NODE_COLOR(200, 200, 255);
const wxColour EDGE_COLOR(150, 150, 150);
const wxColour ACTIVE_NODE_COLOR(255, 200, 200);
const wxColour VISITED_EDGE_COLOR(0, 150, 0);
const wxColour CURRENT_PATH_COLOR(255, 0, 0);
const wxColour BEST_PATH_COLOR(0, 0, 255);
const wxColour TEXT_COLOR(0, 0, 0);

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

class TSPGraphPanel : public wxPanel {
public:
    int n, m;
    vector<E> edge[N];
    priority_queue<node> q;
    deletable_heap<E> dh;
    vector<int> bestRoute;
    vector<pair<int, int>> nodePositions;
    vector<vector<int>> allSteps;
    int currentStep;
    int shortestPathLength;
    bool algorithmComplete;
    bool isPaused;
    wxTimer* animationTimer;

    TSPGraphPanel(wxWindow* parent) : wxPanel(parent) {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT, &TSPGraphPanel::OnPaint, this);
        Bind(wxEVT_SIZE, &TSPGraphPanel::OnSize, this);

        animationTimer = new wxTimer(this);
        Bind(wxEVT_TIMER, &TSPGraphPanel::OnTimer, this);

        currentStep = 0;
        shortestPathLength = -1;
        algorithmComplete = false;
        isPaused = false;
    }

    ~TSPGraphPanel() {
        if (animationTimer->IsRunning()) {
            animationTimer->Stop();
        }
        delete animationTimer;
    }

    void StartAnimation() {
        if (!animationTimer->IsRunning()) {
            animationTimer->Start(ANIMATION_DELAY_MS);
        }
    }

    void PauseAnimation() {
        if (animationTimer->IsRunning()) {
            animationTimer->Stop();
            isPaused = true;
        }
    }

    void ResumeAnimation() {
        if (!animationTimer->IsRunning() && !algorithmComplete) {
            animationTimer->Start(ANIMATION_DELAY_MS);
            isPaused = false;
        }
    }

    void NextStep() {
        if (currentStep < allSteps.size() - 1) {
            currentStep++;
            Refresh();
        }
    }

    void PrevStep() {
        if (currentStep > 0) {
            currentStep--;
            Refresh();
        }
    }

    void OnTimer(wxTimerEvent& event) {
        if (currentStep < allSteps.size() - 1) {
            currentStep++;
            Refresh();
        }
        else {
            animationTimer->Stop();
            algorithmComplete = true;
        }
    }

    void OnSize(wxSizeEvent& event) {
        Refresh();
        event.Skip();
    }

    void OnPaint(wxPaintEvent& event) {
        wxAutoBufferedPaintDC dc(this);
        Render(dc);
    }

    void Render(wxDC& dc) {
        dc.Clear();

        wxGraphicsRenderer* renderer = wxGraphicsRenderer::GetDefaultRenderer();
        if (!renderer) return;

        wxGraphicsContext* gc = wxGraphicsContext::Create((wxWindowDC&)dc);
        if (!gc) return;

        gc->SetBrush(wxBrush(BACKGROUND_COLOR));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRectangle(0, 0, GetSize().x, GetSize().y);

        if (nodePositions.empty()) return;

        // Draw edges first (so nodes appear on top)
        DrawEdges(gc);

        // Draw nodes
        DrawNodes(gc);

        // Draw information text
        DrawInfoText(gc);

        delete gc;
    }

    void DrawNodes(wxGraphicsContext* gc) {
        for (int i = 0; i < nodePositions.size(); i++) {
            wxPoint pos(nodePositions[i].first, nodePositions[i].second);

            // Determine node color
            bool isInCurrentPath = false;
            bool isCurrentNode = false;

            if (currentStep < allSteps.size()) {
                const auto& step = allSteps[currentStep];
                if (step.size() >= 2) {
                    isCurrentNode = (step[0] == i + 1);
                    isInCurrentPath = find(step.begin() + 1, step.end(), i + 1) != step.end();
                }
            }

            if (isCurrentNode) {
                gc->SetBrush(wxBrush(ACTIVE_NODE_COLOR));
            }
            else if (isInCurrentPath) {
                gc->SetBrush(wxBrush(wxColour(200, 255, 200)));
            }
            else {
                gc->SetBrush(wxBrush(NODE_COLOR));
            }

            gc->SetPen(*wxBLACK_PEN);
            gc->DrawEllipse(pos.x - NODE_RADIUS, pos.y - NODE_RADIUS,
                NODE_RADIUS * 2, NODE_RADIUS * 2);

            // Draw node number
            gc->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD),
                TEXT_COLOR);
            wxString label = wxString::Format("%d", i + 1);
            double textWidth, textHeight;
            gc->GetTextExtent(label, &textWidth, &textHeight);
            gc->DrawText(label, pos.x - textWidth / 2, pos.y - textHeight / 2);
        }
    }

    void DrawEdges(wxGraphicsContext* gc) {
        // Draw all edges first (light gray)
        for (int i = 1; i <= n; i++) {
            for (const auto& e : edge[i]) {
                if (e.to > i) { // Draw each edge only once
                    wxPoint from(nodePositions[e.from - 1].first, nodePositions[e.from - 1].second);
                    wxPoint to(nodePositions[e.to - 1].first, nodePositions[e.to - 1].second);

                    gc->SetPen(wxPen(EDGE_COLOR, 1));
                    gc->StrokeLine(from.x, from.y, to.x, to.y);

                    // Draw weight
                    wxPoint mid((from.x + to.x) / 2, (from.y + to.y) / 2);
                    wxString weightLabel = wxString::Format("%d", e.w);
                    gc->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL),
                        TEXT_COLOR);
                    double textWidth, textHeight;
                    gc->GetTextExtent(weightLabel, &textWidth, &textHeight);
                    gc->DrawText(weightLabel, mid.x - textWidth / 2, mid.y - textHeight / 2);
                }
            }
        }

        // Draw edges in current path
        if (currentStep < allSteps.size()) {
            auto step = allSteps[currentStep];
            step.erase(step.begin());
            if (step.size() > 1) {
                gc->SetPen(wxPen(CURRENT_PATH_COLOR, 3));
                for (size_t i = 1; i < step.size(); i++) {
                    int fromNode = step[i - 1];
                    int toNode = step[i];
                    wxPoint from(nodePositions[fromNode - 1].first, nodePositions[fromNode - 1].second);
                    wxPoint to(nodePositions[toNode - 1].first, nodePositions[toNode - 1].second);
                    gc->StrokeLine(from.x, from.y, to.x, to.y);
                }
            }
        }

        // Draw best route if algorithm is complete
        if (algorithmComplete && !bestRoute.empty()) {
            gc->SetPen(wxPen(BEST_PATH_COLOR, 3));
            for (size_t i = 1; i < bestRoute.size(); i++) {
                int fromNode = bestRoute[i - 1];
                int toNode = bestRoute[i];
                wxPoint from(nodePositions[fromNode - 1].first, nodePositions[fromNode - 1].second);
                wxPoint to(nodePositions[toNode - 1].first, nodePositions[toNode - 1].second);
                gc->StrokeLine(from.x, from.y, to.x, to.y);
            }
        }
    }

    void DrawInfoText(wxGraphicsContext* gc) {
        gc->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL),
            TEXT_COLOR);

        wxString status;
        if (algorithmComplete) {
            status = wxString::Format("Algorithm complete! Shortest path length: %d", shortestPathLength);
        }
        else if (isPaused) {
            status = "Paused";
        }
        else {
            status = "Running...";
        }

        gc->DrawText(status, 10, 10);

        if (currentStep < allSteps.size()) {
            const auto& step = allSteps[currentStep];
            if (!step.empty()) {
                wxString stepInfo = wxString::Format("Current node: %d", step[0]);
                gc->DrawText(stepInfo, 10, 30);

                if (step.size() > 1) {
                    wxString pathInfo = "Current path: ";
                    for (size_t i = 1; i < step.size(); i++) {
                        pathInfo += wxString::Format("%d", step[i]);
                        if (i < step.size() - 1) pathInfo += "->";
                    }
                    gc->DrawText(pathInfo, 10, 50);
                }
            }
        }
    }

    bool check(const vector<int>& route, int nxt, int rest_num) {
        if (find(route.begin(), route.end(), nxt) == route.end())
            return true;
        else if (nxt == 1 && rest_num == 1)
            return true;
        return false;
    }

    void print_route(const vector<int>& route) {
        cout << route[0];
        for (int i = 1; i < route.size(); i++) {
            cout << "-->" << route[i];
        }
        cout << endl;
    }

    int bfs() {
        vector<int> initialRoute = { 1 };
        allSteps.push_back({ 1 }); // Initial step: at node 1

        q.push(node(1, 0, n * dh.get().w, dh, n, initialRoute));
        bool sign = false;
        while (!q.empty()) {
            auto u = q.top();
            q.pop();
            int pos = u.pos;
            int w = u.w;
            int rest_num = u.rest_nodes_num;
            auto current_dh = u.dh;
            auto route = u.route;

            // Record this step
            vector<int> stepInfo = { pos };
            stepInfo.insert(stepInfo.end(), route.begin(), route.end());
            allSteps.push_back(stepInfo);

            if (pos == 1 && sign && rest_num == 0) {
                bestRoute = route;
                shortestPathLength = w;
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
                q.push(node{ to, w + val, new_est, tmp, rest_num - 1, new_route });
            }
        }
        return -1;
    }

    void InitializeGraph() {
        // Calculate node positions in a circle
        nodePositions.clear();
        int centerX = 400;
        int centerY = 300;

        // 调整半径计算，考虑节点大小和边的权重文本
        int radius = std::min(centerX, centerY) - std::max(NODE_RADIUS * 2, 30);

        for (int i = 0; i < n; i++) {
            double angle = 2 * M_PI * i / n;
            int x = centerX + radius * cos(angle);
            int y = centerY + radius * sin(angle);
            nodePositions.push_back(make_pair(x, y));
        }

        // Run BFS and collect all steps
        bfs();
    }
};

class ControlPanel : public wxPanel {
public:
    TSPGraphPanel* graphPanel;
    wxButton* startButton;
    wxButton* pauseButton;
    wxButton* resumeButton;
    wxButton* nextButton;
    wxButton* prevButton;

    ControlPanel(wxWindow* parent, TSPGraphPanel* graphPanel)
        : wxPanel(parent), graphPanel(graphPanel) {
        wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);

        startButton = new wxButton(this, wxID_ANY, "Start");
        pauseButton = new wxButton(this, wxID_ANY, "Pause");
        resumeButton = new wxButton(this, wxID_ANY, "Resume");
        nextButton = new wxButton(this, wxID_ANY, "Next Step");
        prevButton = new wxButton(this, wxID_ANY, "Previous Step");

        sizer->Add(startButton, 0, wxALL, 5);
        sizer->Add(pauseButton, 0, wxALL, 5);
        sizer->Add(resumeButton, 0, wxALL, 5);
        sizer->Add(prevButton, 0, wxALL, 5);
        sizer->Add(nextButton, 0, wxALL, 5);

        SetSizer(sizer);

        startButton->Bind(wxEVT_BUTTON, &ControlPanel::OnStart, this);
        pauseButton->Bind(wxEVT_BUTTON, &ControlPanel::OnPause, this);
        resumeButton->Bind(wxEVT_BUTTON, &ControlPanel::OnResume, this);
        nextButton->Bind(wxEVT_BUTTON, &ControlPanel::OnNext, this);
        prevButton->Bind(wxEVT_BUTTON, &ControlPanel::OnPrev, this);
    }

    void OnStart(wxCommandEvent& event) {
        graphPanel->StartAnimation();
    }

    void OnPause(wxCommandEvent& event) {
        graphPanel->PauseAnimation();
    }

    void OnResume(wxCommandEvent& event) {
        graphPanel->ResumeAnimation();
    }

    void OnNext(wxCommandEvent& event) {
        graphPanel->NextStep();
    }

    void OnPrev(wxCommandEvent& event) {
        graphPanel->PrevStep();
    }
};

class InputDialog : public wxDialog {
public:
    wxTextCtrl* nodeInput;
    wxTextCtrl* edgeInput;

    InputDialog(wxWindow* parent)
        : wxDialog(parent, wxID_ANY, "Input Graph Parameters", wxDefaultPosition, wxSize(300, 200)) {
        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

        wxStaticText* nodeLabel = new wxStaticText(this, wxID_ANY, "Number of nodes:");
        nodeInput = new wxTextCtrl(this, wxID_ANY);

        wxStaticText* edgeLabel = new wxStaticText(this, wxID_ANY, "Edges (format: from to weight, one per line):");
        edgeInput = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);

        wxButton* okButton = new wxButton(this, wxID_OK, "OK");
        wxButton* cancelButton = new wxButton(this, wxID_CANCEL, "Cancel");

        wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
        buttonSizer->Add(okButton, 0, wxALL, 5);
        buttonSizer->Add(cancelButton, 0, wxALL, 5);

        mainSizer->Add(nodeLabel, 0, wxALL, 5);
        mainSizer->Add(nodeInput, 0, wxEXPAND | wxALL, 5);
        mainSizer->Add(edgeLabel, 0, wxALL, 5);
        mainSizer->Add(edgeInput, 1, wxEXPAND | wxALL, 5);
        mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 5);

        SetSizer(mainSizer);
    }
};

class TSPFrame : public wxFrame {
public:
    TSPGraphPanel* graphPanel;

    TSPFrame(const wxString& title)
        : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(1000, 800), wxDEFAULT_FRAME_STYLE | wxRESIZE_BORDER) {
        // Create menu bar
        wxMenuBar* menuBar = new wxMenuBar();
        wxMenu* fileMenu = new wxMenu();
        fileMenu->Append(wxID_NEW, "&New Graph\tCtrl+N");
        fileMenu->Append(wxID_EXIT, "&Exit\tCtrl+Q");
        menuBar->Append(fileMenu, "&File");
        SetMenuBar(menuBar);

        Bind(wxEVT_MENU, &TSPFrame::OnNewGraph, this, wxID_NEW);
        Bind(wxEVT_MENU, &TSPFrame::OnExit, this, wxID_EXIT);

        // Create main panel with graph and controls
        wxPanel* mainPanel = new wxPanel(this);
        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

        graphPanel = new TSPGraphPanel(mainPanel);
        ControlPanel* controlPanel = new ControlPanel(mainPanel, graphPanel);

        mainSizer->Add(graphPanel, 1, wxEXPAND);
        mainSizer->Add(controlPanel, 0, wxEXPAND);

        mainPanel->SetSizer(mainSizer);

        // Show input dialog
        ShowInputDialog();
    }

    void OnNewGraph(wxCommandEvent& event) {
        ShowInputDialog();
    }

    void OnExit(wxCommandEvent& event) {
        Close(true);
    }

    void ShowInputDialog() {
        InputDialog dlg(this);
        if (dlg.ShowModal() == wxID_OK) {
            long nodeCount;
            if (!dlg.nodeInput->GetValue().ToLong(&nodeCount) || nodeCount < 2) {
                wxMessageBox("Please enter a valid number of nodes (>= 2)", "Error", wxOK | wxICON_ERROR);
                return;
            }

            wxString edgeText = dlg.edgeInput->GetValue();
            wxArrayString edgeLines = wxSplit(edgeText, '\n');

            // Clear previous graph data
            graphPanel->n = nodeCount;
            graphPanel->m = edgeLines.size();
            for (int i = 0; i < N; i++) {
                graphPanel->edge[i].clear();
            }
            while (!graphPanel->q.empty()) graphPanel->q.pop();
            graphPanel->dh = deletable_heap<E>();
            graphPanel->bestRoute.clear();
            graphPanel->allSteps.clear();
            graphPanel->currentStep = 0;
            graphPanel->shortestPathLength = -1;
            graphPanel->algorithmComplete = false;
            graphPanel->isPaused = false;

            // Parse edges
            for (size_t i = 0; i < edgeLines.size(); i++) {
                wxArrayString parts = wxSplit(edgeLines[i], ' ');
                if (parts.size() != 3) {
                    wxMessageBox(wxString::Format("Invalid edge format at line %d", i + 1),
                        "Error", wxOK | wxICON_ERROR);
                    return;
                }

                long from, to, weight;
                if (!parts[0].ToLong(&from) || !parts[1].ToLong(&to) || !parts[2].ToLong(&weight) ||
                    from < 1 || from > nodeCount || to < 1 || to > nodeCount || weight <= 0) {
                    wxMessageBox(wxString::Format("Invalid edge data at line %d", i + 1),
                        "Error", wxOK | wxICON_ERROR);
                    return;
                }

                graphPanel->edge[from].push_back(E(from, to, weight));
                graphPanel->edge[to].push_back(E(to, from, weight));
                graphPanel->dh.push(E(from, to, weight));
                graphPanel->dh.push(E(to, from, weight));
            }

            // Initialize graph visualization
            graphPanel->InitializeGraph();
            graphPanel->Refresh();
        }
    }
};

class TSPApp : public wxApp {
public:
    virtual bool OnInit() {
        TSPFrame* frame = new TSPFrame("TSP Solver with Branch and Bound");
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(TSPApp);