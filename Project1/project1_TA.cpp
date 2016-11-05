/**
 *  [Project 1]
 *  주어진 그래프에서 가장 가까운 두 집과 가장 먼 두 집의 거리를
 *  각각 찾아 출력한다.
 *  몇 가지 메모리 오류, race condition이 숨어있다.
 *
 *  @author Jongbin Kim
 *  @since  2016-08-24
 */
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <utility>
#include <stdint.h>
#include <vector>
#include <iostream>

using namespace std;

#define MAX_NUM_HOUSE   64              // 최대 집 수
#define MAX_EDGE_PER_VERTEX 10          // vertex의 최대 edge 수
#define MAX_VERTICES    10000000        // 최대 vertex 수
#define MAX_DEPTH   (MAX_VERTICES+1)    // 최대 Depth

#define FLAG(x)     (1L << x)           // 각 집에서 출발하는 탐색의 flag

typedef unsigned int vid_t;
typedef uint64_t flag_t;

// vertex를 몇번째 depth에서 발견했는지에 대한 정보
struct VisitInfo {
    vid_t       vid;    // 발견한 vertex
    uint16_t    depth;  // 발견한 depth
};

// 주어진 집 수
int g_num_house;

// 주어진 집
vid_t g_house[MAX_NUM_HOUSE];

// 주어진 그래프
vid_t g_graph[MAX_VERTICES+1][MAX_EDGE_PER_VERTEX+1];

// 각 스레드의 탐색 정보
VisitInfo g_visit_info[MAX_NUM_HOUSE][MAX_NUM_HOUSE];

// 각 vertex의 탐색 마크
flag_t g_vertex_flag[MAX_VERTICES+1];

// 모든 스레드에게 방문되었을 경우의 탐색 마크
flag_t g_full_flag;

// 각 스레드의 탐색에 사용될 queue
vid_t* queue[MAX_NUM_HOUSE];

// queue의 depth fence 위치
vector<int> g_depth_fence_in_queue[MAX_NUM_HOUSE];

// for what?
int g_have_a_nice_weekend;

/**
 *  그래프를 입력받는다.
 */
void InputGraph(void);

/**
 *  각 스레드의 탐색 함수
 *  BFS 사용, 다른 스레드의 탐색과 만난 첫번째 vertex에 대한 정보를 저장한다.
 *  다른 모든 스레드의 탐색과 만나면, 이 스레드의 탐색을 종료한다.
 *  @param[in]  args 탐색을 시작할 집의 index
 */
void *FindMinMaxPath(void *args);

/**
 *  탐색 결과 데이터에서 minimum shortest path와 maximum shortest path를
 *  찾는다.
 */
void MakeResult(void);

int main(void) {
    InputGraph();

    pthread_t thread[MAX_NUM_HOUSE];

    for (long i = 0; i < g_num_house; i++) {
        pthread_create(&thread[i], 0, FindMinMaxPath, (void*)i);
    }

    for (int i = 0; i < g_num_house; i++) {
        pthread_join(thread[i], NULL);
    }

    MakeResult();

    for (int i = 0; i < g_num_house; i++) {
        free(queue[i]);
    }

    return 0;
}

void InputGraph(void) {
    ios::sync_with_stdio(false);
    cin >> g_num_house;

    // 집 입력
    for (int i = 0; i < g_num_house; i++) {
        cin >> g_house[i];
    }

    for (int i = 0; i < g_num_house; i++) {
        g_vertex_flag[g_house[i]] = FLAG(i);
        g_full_flag |= FLAG(i);
    }

    int num_edge_list;
    int num_vertices;

    cin >> num_vertices;
    cin >> num_edge_list;

    vid_t vid;
    vid_t neighbor;
    int num_edge;

    // 그래프 입력
    for (int i = 0; i < num_edge_list; i++) {
        cin >> vid;
        cin >> num_edge;
        for (int j = 0; j < num_edge; j++) {
            cin >> neighbor;
            g_graph[vid][0]++;
            g_graph[vid][g_graph[vid][0]] = neighbor;
            g_graph[neighbor][0]++;
            g_graph[neighbor][g_graph[neighbor][0]] = vid;
        }
    }

}

/**
 *  각 스레드의 탐색 함수
 *  BFS 사용, 다른 스레드의 탐색과 만난 첫번째 vertex에 대한 정보를 저장한다.
 *  다른 모든 스레드의 탐색과 만나면, 이 스레드의 탐색을 종료한다.
 *  @param[in]  args 탐색을 시작할 집의 index
 */
void *FindMinMaxPath(void *args) {
    long start_idx = (long)args;    // 이 탐색이 시작되는 집의 index

    flag_t my_flag = FLAG(start_idx);   // 이 탐색의 탐색 마크
    flag_t visited_flag = my_flag;      // 다른 탐색과 만난 정보

    // 큐 생성, vertex 최대 갯수면 넉넉하다.
    queue[start_idx] = (vid_t *)malloc(MAX_VERTICES * sizeof(vid_t));
    int front;
    int rear;
    front = rear = 0;

    vid_t curr_node;
    int curr_depth = 0;

    int *find_me = (int *)malloc(10 * sizeof(int));

    // 출발지 enqueue
    queue[start_idx][rear] = g_house[start_idx];
    rear++;
    // 첫 depth fence enqueue. depth fence는 vid와 구별을 위해 MAX_DEPTH 더함.
    queue[start_idx][rear] = curr_depth + MAX_DEPTH;
    rear++;

    if (curr_node == 1007) {
        curr_node = curr_node + 1;
    }

    // depth fence의 위치들은 따로 기록해 놓는다.
    // 이후 minimum shortest path, maximum shortest path를 결정할 때 사용.
    g_depth_fence_in_queue[start_idx].push_back(0);
    while (front < rear) {
        // dequeue
        curr_node = queue[start_idx][front];
        front++;

        if (curr_node >= MAX_DEPTH) {
            // dequeue한 것이 depth fence이다.
            // 다음 depth fence를 enqueue한다.
            curr_depth++;
            queue[start_idx][rear] = curr_depth + MAX_DEPTH;
            rear++;

            g_depth_fence_in_queue[start_idx].push_back(front-1);

            if (visited_flag == g_full_flag) {
                // 모든 다른 스레드들을 만났다면 탐색을 종료한다.
                break;
            }

            g_have_a_nice_weekend = curr_depth;

            continue;
        }

        for (int i = 1; i <= g_graph[curr_node][0]; i++) {
            // dequeue한 vertex의 이웃들을 확인한다.
            // 이미 방문했다면 skip
            if ((g_vertex_flag[g_graph[curr_node][i]] & my_flag)) {
                continue;
            }

            // fetch_and_or instruction을 통해 방문 마크를 vertex에 찍는다.
            // 동시에, 기존에 찍힌 마크를 확인해 다른 스레드가 이 vertex를
            // 방문했는지를 확인한다.
            // 두 스레드가 어떤 vertex를 방문한 경우, 한 스레드는
            //  다른 스레드가 이 vertex를 방문했다는 것을 무조건 확인한다.
            flag_t flag_before = __sync_fetch_and_or(
                    &g_vertex_flag[g_graph[curr_node][i]], my_flag);
    
            if (flag_before) {
                // 어떤 다른 스레드가 이 vertex를 방문했다는 것을 확인.
                // 이미 만난 스레드는 거른다.
                flag_before &= (~visited_flag);
                visited_flag |= flag_before;

                int thd = 0;
                while (flag_before) {
                    if (flag_before & 0x1) {
                        // thd 스레드와 처음 만났다.
                        // 처음 만난 vertex와 depth를 저장한다.
                        g_visit_info[start_idx][thd].vid
                            = g_graph[curr_node][i];
                        g_visit_info[start_idx][thd].depth
                            = curr_depth + 1;
                    }
                    thd++;
                    flag_before >>= 1;
                }
            }
            // 이웃 vertex를 enqueue한다.
            queue[start_idx][rear] = g_graph[curr_node][i];
            rear++;
        }
    }
    find_me = NULL;

    return NULL;
}

/**
 *  탐색 결과 데이터에서 minimum shortest path와 maximum shortest path를
 *  찾는다.
 *  A 스레드가 B 스레드를 depth D1에서 처음 만났고,
 *  B 스레드가 A 스레드를 depth D2에서 처음 만났다면
 *  A 스레드와 B 스레드의 실제 거리는 D1+D2 또는 D1+D2-1이다.
 *  이 외의 다른 경우는 없음이 보장된다.
 *  
 *  A 스레드가 B 스레드를 처음 만난 vertex V1이 B 스레드의 depth D2 탐색범위에
 *  있었다면, 실제 거리는 D1+D2가 된다.
 *  B 스레드가 A 스레드를 처음 만난 vertex V2가 A 스레드의 depth D1 탐색범위에
 *  있었다면, 실제 거리는 D1+D2가 된다.
 *  두 경우가 아니라면, 실제 거리는 D1+D2-1이 된다.
 *
 *  D1+D2가 가장 작은 house pair들은 minimum shortest path의 후보가 된다.
 *  D1+D2가 가장 큰 house pair들은 maximum shortest path의 후보가 된다.
 */
void MakeResult(void) {
    int min = 999999999;
    int max = 0;
    vector< pair<VisitInfo, VisitInfo> > min_pairs;
    vector< pair<int, int> > min_pairs_thd;
    vector< pair<VisitInfo, VisitInfo> > max_pairs;
    vector< pair<int, int> > max_pairs_thd;

    // D1 + D2의 최소값과 최대값을 찾는다.
    for (int i = 0; i < g_num_house; i++) {
        for (int j = i+1; j < g_num_house; j++) {
            int dist;
            dist = g_visit_info[i][j].depth + g_visit_info[j][i].depth;

            if (min > dist) {
                min = dist;
            }
            if (max < dist) {
                max = dist;
            }
        }
    }
    // minimum shortest path의 후보와 maximum shortest path의 후보를 찾는다.
    for (int i = 0; i < g_num_house; i++) {
        for (int j = i+1; j < g_num_house; j++) {
            int dist = g_visit_info[i][j].depth + g_visit_info[j][i].depth;
            if (dist == min) {
                min_pairs.push_back(pair<VisitInfo, VisitInfo>(
                            g_visit_info[i][j], g_visit_info[j][i]));
                min_pairs_thd.push_back(pair<int, int>(i, j));
            }
            if (dist == max) {
                max_pairs.push_back(pair<VisitInfo, VisitInfo>(
                            g_visit_info[i][j], g_visit_info[j][i]));
                max_pairs_thd.push_back(pair<int, int>(i, j));
            }
        }
    }

    int i;
    for (i = 0; i < min_pairs.size(); i++) {
        // thd1: minimum shortest path 후보 pair의 스레드1
        // thd2: minimum shortest path 후보 pair의 스레드2
        // thd1_vertex: 스레드1이 스레드2를 처음 발견한 vertex
        // thd1_depth: 스레드1이 스레드2를 처음 발견한 depth
        // thd2_vertex: 스레드2가 스레드1을 처음 발견한 vertex
        // thd2_depth: 스레드2가 스레드1을 처음 발견한 depth
        int thd1 = min_pairs_thd[i].first;
        int thd2 = min_pairs_thd[i].second;
        int thd1_vertex = min_pairs[i].first.vid;
        int thd1_depth = min_pairs[i].first.depth;
        int thd2_vertex = min_pairs[i].second.vid;
        int thd2_depth = min_pairs[i].second.depth;
    
        // thd2가 thd1_vertex를 thd2_depth에서 만났다면, thd1과 thd2의 거리는
        // thd1_depth+thd2_depth이다.
        // thd2가 thd1_vertex를 thd2_depth-1에서 만났다면, thd1과 thd2의
        // 거리는 thd1_depth + thd2_depth - 1이다. 이 경우 이 거리가 무조건
        // minimum shortest distance가 된다.
        // thd1과 thd2를 바꿔도 마찬가지이다.
        int j;
        for (j = g_depth_fence_in_queue[thd2][thd2_depth];
             j < g_depth_fence_in_queue[thd2][thd2_depth+1]; j++ ) {
            if (queue[thd2][j] == thd1_vertex) {
                break;
            }
        }
        if (j == g_depth_fence_in_queue[thd2][thd2_depth+1]) {
            break;
        }
        
        for (j = g_depth_fence_in_queue[thd1][thd1_depth];
             j < g_depth_fence_in_queue[thd1][thd1_depth+1]; j++ ) {
            if (queue[thd1][j] == thd2_vertex) {
                break;
            }
        }
        if (j == g_depth_fence_in_queue[thd1][thd1_depth+1]) {
            break;
        }

    }
    if (i < min_pairs.size()) {
        min -= 1;
    }

    for (i = 0; i < max_pairs.size(); i++) {
        // thd1: maximum shortest path 후보 pair의 스레드1
        // thd2: maximum shortest path 후보 pair의 스레드2
        // thd1_vertex: 스레드1이 스레드2를 처음 발견한 vertex
        // thd1_depth: 스레드1이 스레드2를 처음 발견한 depth
        // thd2_vertex: 스레드2가 스레드1을 처음 발견한 vertex
        // thd2_depth: 스레드2가 스레드1을 처음 발견한 depth    
        int thd1 = max_pairs_thd[i].first;
        int thd2 = max_pairs_thd[i].second;
        int thd1_vertex = max_pairs[i].first.vid;
        int thd1_depth = max_pairs[i].first.depth;
        int thd2_vertex = max_pairs[i].second.vid;
        int thd2_depth = max_pairs[i].second.depth;

        // thd2가 thd1_vertex를 thd2_depth-1에서 만났다면, thd1과 thd2의
        // 거리는 thd1_depth + thd2_depth - 1이다.  
        // thd2가 thd1_vertex를 thd2_depth에서 만났다면, thd1과 thd2의 거리는
        // thd1_depth+thd2_depth이다. 이 경우 이 거리가 무조건
        // maximum shortest distance가 된다.
        int j;  
        for (j = g_depth_fence_in_queue[thd2][thd2_depth-1];
             j < g_depth_fence_in_queue[thd2][thd2_depth]; j++ ) {
            if (queue[thd2][j] == thd1_vertex) {
                break;
            }
        }
        if (j < g_depth_fence_in_queue[thd2][thd2_depth]) {
            continue;
        }
        
        for (j = g_depth_fence_in_queue[thd1][thd1_depth-1];
             j < g_depth_fence_in_queue[thd1][thd1_depth]; j++ ) {

            if (queue[thd1][j] == thd2_vertex) {
                break;
            }
        }
        if (j == g_depth_fence_in_queue[thd1][thd1_depth]) {
            break;
        }

    }
    if (i == max_pairs.size()) {
        max -= 1;
    }

    printf("%d\n%d\n", min, max);
}

