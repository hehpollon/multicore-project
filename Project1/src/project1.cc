#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <queue>
#include <limits.h>

using namespace std;

typedef struct GraphNode {

	int vertex;
	struct GraphNode *link;
	
} GraphNode;

typedef struct QueueNode {

	int vertex;
	int path;

} QueueNode;

void InsertEdge(GraphNode **vertice, int start, int end);
int search(GraphNode **vertice, int start_vertex, int end_vertex, int vertice_num);
//int g_path_count = 0;

int main(void) {

	int house_num, vertice_num, temp;

	//init house
	scanf("%d", &house_num);
	int house[house_num];
	for(int i=0; i < house_num; i++) {
		scanf("%d", &temp);
		house[i] = temp;
	}

	//init vertice (there is no 0 vertice, so vertice[0] is not used)
	scanf("%d", &vertice_num);
	GraphNode *vertice[(vertice_num+1)];
	for(int i = 0; i<vertice_num+1; i++){
		vertice[i] = NULL;
	}

	//init edge
	scanf("%d", &temp);
	int edge_num, start_vertex, end_vertex;
	for(int i=0; i < temp; i++) {
		scanf("%d", &start_vertex);
		scanf("%d", &edge_num);
		for(int j=0; j < edge_num; j++) {
			scanf("%d", &end_vertex);
			InsertEdge(vertice, start_vertex, end_vertex);
			InsertEdge(vertice, end_vertex, start_vertex);
		}
	}

	//show all vertice.
//	GraphNode *tmp;
//	for(int i=1; i < (vertice_num+1); i++){
//		
//		tmp = vertice[i];
//		printf("\nvertex %d's list",i);
//		while(tmp != NULL){
//			printf("->%d", tmp->vertex);
//			tmp = tmp->link;
//		}
//		printf("\n");
//	}

	int least = INT_MAX;
	int most = 0;
	
	for(int i = 0; i < house_num; i++){
		for(int j = i+1; j < house_num; j++){
			int distance = search(vertice, house[i], house[j], vertice_num);
			if(least >= distance)
				least = distance;
			if(most <= distance)
				most = distance;
		}
	}

	printf("%d\n", least);
	printf("%d\n", most);

    return 0;
}


void InsertEdge(GraphNode **vertice, int start, int end){

	GraphNode *node;
	node = (GraphNode *)malloc(sizeof(GraphNode));
	node->vertex = end;
	node->link = vertice[start];
	vertice[start] = node;
}


int search(GraphNode **vertice, int start_vertex, int end_vertex, int vertice_num){

	int visit_vertice[(vertice_num + 1)];
	memset(visit_vertice, 0, sizeof(int) * vertice_num);

	queue<QueueNode> queue;
	QueueNode q_node;

	int distance = 1;

	GraphNode *tmp = vertice[start_vertex];
	while(true){
		if(tmp->vertex == end_vertex){
			break;
		}
		if(visit_vertice[tmp->vertex] == 0){
			q_node.vertex = tmp->vertex;
			q_node.path = distance;
			queue.push(q_node);
			visit_vertice[tmp->vertex] = 1;
		}
		
//		else{
//			tmp = tmp->link;
//			continue;
//		}
		
		tmp = tmp->link;
		if(tmp == NULL){
			if(distance == queue.front().path){
				distance++;
			}
			tmp = vertice[queue.front().vertex];
			queue.pop();
			continue;
		}
	}

	return distance;

}

/*

void search(GraphNode **vertice, int start_vertex, int end_vertex, int current_count, int recursive_count){

	GraphNode *origin = vertice[start_vertex];
	GraphNode *tmp = vertice[start_vertex];

	while(tmp != NULL){
		if(tmp->vertex == end_vertex){
			printf("find end");
			return
		}else{
			current_count ++;
			if(g_path_count < current_count){
				current_count --;
			}else if(g_path_count == current_count){
				recursive_count = current_count + 1;
				search(tmp->vertex, end_vertex, current_count, recursive_count);
			}
		}
		tmp = tmp->link;
		if(tmp == NULL){
			if((g_path_count + 1) == recursive_count)
			g_path_count++;
			tmp = origin;
		}
	}
	

}


void serch2(int start_vertex, int end_vertex, int *temp_vertice){
	
	while(tmp != NULL){
		if(vertice[start_vertex]->vertex == end_vertex){
			printf("find end");

		}else{
			
			
		}


	}

}

*/
