#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>

#define NUM_THREAD 4

int g_cnt_global = 0;

//각 vertice들에 연결된 vertex들을 저장하기 위한 링크리스트의 노드들이다. 그래프를 그리는데 이용된다.
typedef struct GraphNode {
	int vertex;
	struct GraphNode *link;
	bool is_house = false;
} GraphNode;

//각 vertex사이의 길이를 알기위한 함수에 이용되는 큐에 들어가는 노드이다.
typedef struct QueueNode {
		int vertex;
		int path;
		struct QueueNode *next;
} QueueNode;

//흔히 볼 수 있는 Queue 자료구조이다.
typedef struct Queue{
		int count;
		QueueNode *front;
		QueueNode *rear;
} Queue;


typedef struct MyArg {

	GraphNode **vertice;
	int house_num;
	int *house_distance_least;
	int *house_distance_most; 
	int *house; 
	int vertice_num;
} MyArg;



Queue* CreateQueue();
void DestroyQueue(Queue *qptr);
void AddRear(Queue *qptr, int vertex, int path);
void RemoveFront(Queue *qptr);

/*
 * 각 vertex들과 인접해있는 vertex표현을 위해 해당 인접한 vertex를 링크리스트에 삽입하는 함수이다.
 * 
 * @param[in] **vertice : vertex들의 링크리스트들을 배열로 관리하는 2중포인터
 * @param[in] start : end vertex와 인접을 당하는 vertex이다.
 * @param[in] end : start vertex와 인접하는 vertex이다.
 *
 */
void InsertEdge(GraphNode **vertice, int start, int end, bool *house_vertice);

/*
 * 한 house와 나머지 house들간의 거리를 구하는 함수이다. 그리고 구한 거리는 house_distance라는 
 * 2차원 배열에 저장하게 된다. 
 * 
 * @param[in] **vertice : vertex들의 링크리스트들을 배열로 관리하는 2중포인터
 * @param[in] *visit_vertice : 방문한 vertex들을 표시하기 위해 bool배열로 모두 false로 초기화된 배열의 포인터다.
 * @param[in] house_num : 집들의 숫자이다.
 * @param[in] *house_distance : house_num * house_num만큼의 개수가 존재하는 house들간의 거리가 저장되는 2차원 배열이다.
 * @param[in] *house : 모든 집들의 vertex번호를 저장하고 있는 배열이다.
 * @param[in] start_num : house배열에서 house들간의 거리를 구하기 위해 기준이 되는 시작되는 집의 index를 나타낸다.
 *
 */
void Search(GraphNode **vertice, bool *visit_vertice, int house_num, int *house_distance_least,
					int *house_distance_most, int *house, int start_num);

pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;


void *ThreadFunc(void *arg){
	MyArg *my_arg = (MyArg *) arg;	
	int start_num;
	bool visit_vertice[(my_arg->vertice_num + 1)];
	while(g_cnt_global < (my_arg->house_num - 1)) {		
		pthread_mutex_lock(&g_mutex);
		start_num = g_cnt_global;
		g_cnt_global ++;
		pthread_mutex_unlock(&g_mutex);
		memset(visit_vertice, 0, sizeof(bool) * (my_arg->vertice_num + 1));
		Search(my_arg->vertice, visit_vertice, my_arg->house_num, my_arg->house_distance_least,
				my_arg->house_distance_most, my_arg->house, start_num);
	}
	return NULL;
}
	


int main(void) {

	int house_num, vertice_num;//집들의 숫자와 vetice의 숫자

	int temp;
	//init house
	scanf("%d", &house_num);
	int house[house_num]; //house들의 vertex를 저장하기 위한 배열이다

	for (int i=0; i < house_num; i++) {
		scanf("%d", &temp);
		house[i] = temp;
	}


	//init vertice (there is no 0 vertice, so vertice[0] is not used)
	//배열 index와 vertice들의 vertex숫자를 일치시키기 위해 0번째 배열은 사용하지 않는다.
	scanf("%d", &vertice_num);
	GraphNode *vertice[(vertice_num + 1)];
	
	//
	bool house_vertice[(vertice_num + 1)];
	memset(house_vertice, 0, sizeof(bool) * (vertice_num + 1));
	for (int i=0; i < house_num; i++) {
		house_vertice[house[i]] = 1;
	}
	//
	for (int i = 0; i < vertice_num + 1; i++) {
		vertice[i] = NULL;
	}

	//init edge 각 vertice들의 인접한 관계를 표시하는 리스트들을 연결해주고 vertice배열을 채워준다.
	//vertice배열들은 링크리스트들의 시작노드를 저장하고 있다.
	scanf("%d", &temp);
	int edge_num, start_vertex, end_vertex;
	for (int i=0; i < temp; i++) {
		scanf("%d", &start_vertex);
		scanf("%d", &edge_num);
		for (int j=0; j < edge_num; j++) {
			scanf("%d", &end_vertex);
			InsertEdge(vertice, start_vertex, end_vertex, house_vertice);
			InsertEdge(vertice, end_vertex, start_vertex, house_vertice);
		}
	}
	
	//방문한 vertice들을 표현하기위한 배열이다. 나중 다중스레드로 코딩하기 위해서 스레드 개수만큼의 배열을 
	//생성한다.
//	bool visit_vertice[(vertice_num + 1)];
	//house들간의 거리를 여기다가 저장해둔다.
	int house_distance_least[house_num];
	int house_distance_most[house_num];
	int least = INT_MAX;
	int most = 0;
	pthread_t threads[NUM_THREAD];
	MyArg my_arg;
	my_arg.vertice = vertice;
	my_arg.house_num = house_num;
	my_arg.house_distance_least = house_distance_least;
	my_arg.house_distance_most = house_distance_most;
	my_arg.house = house;
	my_arg.vertice_num = vertice_num;

	if(NUM_THREAD < house_num - 1) {
		temp = NUM_THREAD;
	}
	else {
		temp = house_num - 1;
	}
	
	for (int i = 0; i < temp; i++) {
//		printf("------thread[%d] start!------\n",i);
		if(pthread_create(&threads[i], 0, ThreadFunc, &my_arg) < 0) {
			return 0;
		}
	
	}

	for(int i = 0; i < temp; i++) {
		pthread_join(threads[i], NULL);
//		printf("------thread[%d] join!------\n",i);
	}
	//저장된 집들간의 거리를 이용해서 최소거리와 최대거리를 구한다.
	for (int i = 0; i < house_num - 1; i++) {		
		if (least >= house_distance_least[i]) {
			least = house_distance_least[i]; 
		}
		if (most <= house_distance_most[i]) {
			most = house_distance_most[i];
		}
		
	}

	printf("%d\n", least);
	printf("%d", most);


    return 0;
}


void InsertEdge(GraphNode **vertice, int start, int end, bool *house_vertice) {

	//node를 동적할당 해주고 node에 정보를 채워준다. 그리고 vertice_start node와 연결을 해준다.
	GraphNode *node;
	node = (GraphNode *)malloc(sizeof(GraphNode));
	node->vertex = end;
	if(house_vertice[end])
		node->is_house = 1;
	node->link = vertice[start];
	vertice[start] = node;
}

//Queue를 생성하고 Queue포인터를 리턴합니다.
Queue* CreateQueue() {
	Queue *new_queue =(Queue *)malloc(sizeof(Queue));
	new_queue->count = 0;
	new_queue->front = NULL;
	new_queue->rear = NULL;
	return new_queue;
}

//큐를 삭제합니다.
void DestroyQueue(Queue *qptr) {
	while (qptr->count!=0) {
		RemoveFront(qptr);
	}
	free(qptr);
}

//큐에 삽입을 합니다.
void AddRear(Queue *qptr, int vertex, int path) {
	QueueNode *new_node = (QueueNode *)malloc(sizeof(QueueNode));
	new_node->vertex = vertex;
	new_node->path = path;
	new_node->next = NULL;

	if (qptr->count == 0) {
		qptr->front = new_node;
		qptr->rear = new_node;
	}
	else {
		qptr->rear->next = new_node;
		qptr->rear = new_node;
	}
	qptr->count++;
}

//큐를 제거합니다.
void RemoveFront(Queue *qptr) {
	if (qptr->count==0) {
		return;
	}
	else {
		QueueNode *tmp = qptr->front;
		qptr->front = tmp->next;
		free(tmp);
		qptr->count--;
		return;
	}
}


void Search(GraphNode **vertice, bool *visit_vertice, int house_num, int *house_distance_least,
					int *house_distance_most, int *house, int start_num){

	Queue *queue = CreateQueue();
	
	//house[start_num]과 얼마나 떨어져있는지를 저장한다. 초기에는 한단계 이동하므로 1부터 시작한다.
	int distance = 1;
	int count_end = start_num + 1; //한 집과 나머지 모든 집들간의 거리를 다 구할경우 함수를 return한다.
	int least = INT_MAX;
	int most = 0;
	int house_start_vertex = house[start_num];
	GraphNode *tmp = vertice[house_start_vertex];//해당 시작되는 집을 구한다.
	visit_vertice[house_start_vertex] = 1;
	int current_vertex = 0;
	while (true) {
		if (!visit_vertice[tmp->vertex]) {//해당 vertex와 인접해있는 vertex를 방문하고 만약 방문하지 않았다면
			current_vertex = tmp->vertex;
			AddRear(queue, current_vertex, distance);
			
			visit_vertice[current_vertex] = 1;
			//큐에 노드를 넣고 해당 vertex를 방문했다고 표시.

			//만약 방문한 vertex가 집이라면 해당 집과의 거리는 지금까지 증가된 distance값이고 
			//이 distance값과 짧은 distance와 긴 distance값의 비교를 통해 한 집과 나머지 집사이의 
			//최단거리 집과의 거리와 최장거리 집과의 거리를 구해나간다.
			
			if (tmp->is_house) 
			{
			//	printf("%d, ",distance);
				for (int i = start_num + 1; i < house_num; i++) {
					if (house[i] == current_vertex) {
						count_end ++;
						break;
					}
				}
			//	count_end ++;
				
				if (least >= distance) {
					least = distance; 
				} 
				else if (most <= distance) {
					most = distance; 
				}
				if (count_end == house_num) {
					DestroyQueue(queue);
					house_distance_least[start_num] = least;
					house_distance_most[start_num] = most;
		//			printf("%d's least : %d, most : %d \n",start_num,house_distance_least[start_num], house_distance_most[start_num]);
					return;
				}

				
			}
			
		}

				
		tmp = tmp->link;
		if (tmp == NULL) {
			//만약 큐에 저장된 노드의 거리와 함수의 distance가 같아진다면 다음 해당 거리만큼은 다 구했기 때문에
			//다음 단계의 거리를 구하기 위해 distance값을 1증가 시킨다.
			if (distance == queue->front->path) {
				distance++;
			}
			tmp = vertice[queue->front->vertex];
			RemoveFront(queue);
			continue;
		}
	}

}

