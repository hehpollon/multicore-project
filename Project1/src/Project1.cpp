#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>

#define NUM_THREAD 36

typedef struct Vertex {
	
	unsigned int count = 0;
	bool is_house = 0;
	unsigned int adjacency[10] = {0};
	unsigned int house;
	
} Vertex;

typedef struct MyArg {

	Vertex *vertice;
	unsigned int house_num;
	unsigned int *house_distance_least;
	unsigned int *house_distance_most; 
	unsigned int *house; 
	unsigned int vertice_num;

} MyArg;
void InsertEdge(Vertex *vertice, unsigned int start, unsigned int end, bool *house_vertice);
void Search(Vertex *vertice, bool *visit_vertice, unsigned int *house_distance_least,
		unsigned int *house_distance_most, unsigned int vertice_num, unsigned int house_num, unsigned int *house, unsigned int start_num, unsigned int *search_array);



pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
unsigned int g_cnt_global = 0;

void *ThreadFunc(void *arg){
	MyArg *my_arg = (MyArg *) arg;	
	unsigned int start_num;
	bool *visit_vertice;
	visit_vertice = (bool *)malloc(my_arg->vertice_num + 1);
	unsigned int *search_array;
	search_array = (unsigned int *) malloc(sizeof(unsigned int) * my_arg->vertice_num);
	
	while(g_cnt_global < (my_arg->house_num - 1)) {		
		pthread_mutex_lock(&g_mutex);
		start_num = g_cnt_global;
		g_cnt_global ++;
		pthread_mutex_unlock(&g_mutex);
		memset(visit_vertice, 0, sizeof(bool) * (my_arg->vertice_num + 1));
		Search(my_arg->vertice, visit_vertice, my_arg->house_distance_least,
		my_arg->house_distance_most, my_arg->vertice_num, my_arg->house_num, my_arg->house, start_num, search_array);
	}
	free(visit_vertice);
	free(search_array);
	return NULL;
}




int main(void) {

	unsigned int house_num, vertice_num;				//집들과 vertex 총 개수

	unsigned int temp;
	scanf("%d", &house_num);
	unsigned int house[house_num]; 					//house들의 vertex를 저장하기 위한 배열이다

	//init house[]
	for (unsigned int i=0; i < house_num; i++) {
		scanf("%d", &temp);
		house[i] = temp;
	}


	//init vertice (there is no 0 vertice, so vertice[0] is not used)
	scanf("%d", &vertice_num);
	Vertex *vertice;
	vertice = (Vertex *) malloc(sizeof(Vertex) * (vertice_num + 1));
	
	//vertex중 house로 선정된 vertex를 빠르게 찾기 위해 배열 할당
	bool *house_vertice;
	house_vertice = (bool *) malloc(sizeof(bool) * (vertice_num + 1));

	for (unsigned int i=0; i < house_num; i++) {
		house_vertice[house[i]] = 1;
	}

	scanf("%d", &temp);
	unsigned int edge_num, start_vertex, end_vertex;
	for (unsigned int i=0; i < temp; i++) {
		scanf("%d", &start_vertex);
		scanf("%d", &edge_num);
		for (unsigned int j=0; j < edge_num; j++) {
			scanf("%d", &end_vertex);
			InsertEdge(vertice, start_vertex, end_vertex, house_vertice);
			InsertEdge(vertice, end_vertex, start_vertex, house_vertice);
		}
	}

	free(house_vertice);

	unsigned int house_distance_least[house_num];
	unsigned int house_distance_most[house_num];


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
		
	for (unsigned int i = 0; i < temp; i++) {
		if(pthread_create(&threads[i], 0, ThreadFunc, &my_arg) < 0) {
			return 0;
		}	
	}

	for(unsigned int i = 0; i < temp; i++) {
		pthread_join(threads[i], NULL);
	}


	unsigned int least = INT_MAX;
	unsigned int most = 0;
	for (unsigned int i = 0; i < house_num - 1; i++) {		
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

void InsertEdge(Vertex *vertice, unsigned int start, unsigned int end, bool *house_vertice) {
	
	vertice[start].adjacency[vertice[start].count] = end;
	vertice[start].count ++;
	if(house_vertice[end]) {
		vertice[start].is_house = 1;
		vertice[start].house = end;
	}
}

void Search(Vertex *vertice, bool *visit_vertice, unsigned int *house_distance_least,
		unsigned int *house_distance_most, unsigned int vertice_num, unsigned int house_num, unsigned int *house, unsigned int start_num, unsigned int *search_array) {	
	
	register unsigned int distance = 0;
	search_array[0] = house[start_num];
	bool *visit_house = (bool *)malloc(vertice_num + 1);
	
	//------------------------ for문안에서 지속적으로 변화하는 값
	register unsigned int *adjacency;
	register unsigned int adjacency_vertex;
	register unsigned int count;
	register unsigned int current_vertex;
	//------------------------
	
	register unsigned int total_count = 0;
	register unsigned int cycle_count = 0;
	register unsigned int search_array_count = 1;

	bool least_flag = true;
	register unsigned int least = 0;
	register unsigned int most = 0;

	visit_vertice[house[start_num]] = 1;
	
	register unsigned int i = 0;
	register unsigned int count_end = 1 + start_num;
	register unsigned int j;
	register unsigned int temp_house;
	while(true) {
		current_vertex = search_array[i];
		if (vertice[current_vertex].is_house) {
			temp_house = vertice[current_vertex].house;
			if(!visit_house[temp_house]) {
				visit_house[temp_house] = 1;
				
				for(j = start_num + 1; j < house_num; j++){
					if (house[j] == temp_house) {
						count_end++;
						break;
					}
				}
				
				if (!least_flag) {
					most = distance;
				}else {
					if(distance > 1){
						least = distance;
						least_flag = false;
					}
				}
				if(count_end == house_num) {	
					house_distance_least[start_num] = least + 1;
					house_distance_most[start_num] = most + 1;
					return;
				}
			}
		}

		adjacency = vertice[current_vertex].adjacency;
		count = vertice[current_vertex].count;
		j = 0;
		do {
			adjacency_vertex = adjacency[j];
			if (!visit_vertice[adjacency_vertex]) {
				search_array[search_array_count] = adjacency_vertex;
				visit_vertice[adjacency_vertex] = 1;
				search_array_count ++;
				total_count ++;
			}
			j++;
		} while (j < count);
		if(i == cycle_count) {
			cycle_count = cycle_count + total_count;
			total_count = 0;
			distance++;
		}

		i++;
	}
				

}
