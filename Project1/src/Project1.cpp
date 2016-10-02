#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>

#define NUM_THREAD 36
#define MAX_EDGE_NUM 10


//
//	Vertex들을 Structure이다. 
//
//	count : 해당 vertex와 인접한 vertex들의 count
//	adjacency[MAX_EDGE_NUM] : 인접한 vertex들의 배열 adjacency
//	is_house : 인접한 vertex들중에 집이 있는지에 대한 여부를 묻는 is_house 플래그
//	house : 인접한 vertex중에 집이 있다면 집의 vertex를 저장
//
typedef struct Vertex {
	
	unsigned int count = 0;
	bool is_house = 0;
	unsigned int adjacency[MAX_EDGE_NUM] = {0};
	unsigned int house;
	
} Vertex;

//
//	스레드를 생성할때 넘겨주는 정보를 담기 위한 구조체이다.
//	*vertice : 구조체 Vertex들의 배열포인터(vertice_num만큼의 수를 가짐)
//	house_num : 총 집의 개수
//	*house_distance_least : 한 집에서 다른 집들간의 최단거리를 저장하는 배열
//	*house_distance_most : 한 집에서 다를 집들간의 최장거리를 저장하는 배열
//	*house : 집들의 vertex를 저장하고 있는 배열(총 house_num만큼의 배열이 존재함)
//	*vertice_num : vertex들의 총 개수. 
//
typedef struct MyArg {

	Vertex *vertice;
	unsigned int house_num;
	unsigned int *house_distance_least;
	unsigned int *house_distance_most; 
	unsigned int *house; 
	unsigned int vertice_num;

} MyArg;


/*
 *	구조체 Vertex들의 배열 vertice에 값을 채워주는 함수이다. 한 vertex가 가지고 있는
 *	인접한 vertex들과 총 숫자, 인접한 vertex들이 집인지 아닌지에 대한 Flag, 해당 집에
 *	대한 정보를 저장한다.
 *	
 *	@param[in,out] *vertice : Vertex들의 배열 포인터
 *	@param[in] start : 기준이 되는 vertex
 *	@param[in] end : 연결되는 vertex
 *	@param[in] *house_vertice : 해당 vertex가 집인지 아닌지를 빠르게 찾기
 *	위해 미리 배열에다가 해당 vertex가 집인지 아닌지에 대한 flag를 저장해둠.
 *
 * */
void InsertEdge(Vertex *vertice, unsigned int start, unsigned int end, bool *house_vertice);

/*
	한 집을 기준으로 다른 집들과의 최단거리와 최장거리를 찾아주는 함수이다.
	너비우선탐색을 베이스로 해서 한 집을 기준으로 distance값을 1만큼씩 증가시키면서 
	1증가할때마다 집이 존재하는 여부를 따져서 집이 존재한다면 해당 집까지의 거리는
	증가된 distance의 값이 되는 함수이다. 단, 너비우선탐색과는 다르게 큐를 이용하지 
	않고 distance값의 증가됨을 표현하기 위해 배열 search_array에다가 vertex들을 
	하나씩 담아두고 다시 해당 vertex를 꺼내서 인접한 곳에 집이 있나 없나를 판단한다.

	@param[in] *vertice : 구조체 Vertex들의 배열포인터(vertice_num만큼의 수를 가짐)
	@param[in] *visit_vertice : 방문한 vertice들의 플래그를 저장하는 배열이다. 해당 vertex를
	방문했는지를 빠르게 검색하고 병행적으로 검색 할 수 있도록 각 스레드마다 각각 
	생성해서 방문여부를 저장해둔다.
	@param[in] house_num : 총 집의 개수
	@param[in, out] *house_distance_least : 한 집에서 다른 집들간의 최단거리를 저장하는 배열
	@param[in, out] *house_distance_most : 한 집에서 다를 집들간의 최장거리를 저장하는 배열
	@param[in] *house : 집들의 vertex를 저장하고 있는 배열(총 house_num만큼의 배열이 존재함)
	@param[in] *vertice_num : vertex들의 총 개수. 
	@param[in] start_num : 서치함수에서 house배열을 통해서 기준이 
	되는 집의 인덱스를 나타낸다. 서치함수는 house[start_num]을 기준으로 최단거리 
	최장거리를 탐색한다.
	@param[in] *search_array : 한 vertex A에를 방문한다가정하면 인접한 곳에 집이 존재하나를 
	검사하고 난 후 인접한 vertex B,C,D들중
	방문한적 없는 vertex가 B,D라고 할때 이 B,D를 search_array에다가 저장해둔다. 
	그리고 search_array를 순차탐색을 통해 저장된 vertex들을 방문하다가 B,D를 방문하게 된다면 
	다시 같은 행위를 반복한다.

	이런식으로 한 집과 다른집들사이의 최단거리 최장거리를 찾고 난후 그 값들을 *house_distance_least배열과
	*house_distance_most배열에 저장하고 난후 함수를 리턴한다.

*/
void Search(Vertex *vertice, bool *visit_vertice, unsigned int *house_distance_least,
		unsigned int *house_distance_most, unsigned int vertice_num,
		unsigned int house_num, unsigned int *house, unsigned int start_num, 
		unsigned int *search_array);

pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
unsigned int g_cnt_global = 0;
unsigned int g_total_max = 0;
unsigned int *g_i_max;
unsigned int **g_house_distance_i_to_all;

//	스레드를 만들어줄 때 실행하는 함수이다. 각각의 스레드마다 기준이 되는 집들을 정해서
//	그 집과 다른집들사이의 최단 최장거리를 각각 구하고 다음 기준이 되는 집을 정해서 또 구하다가
//	모든 집들까지 다 구하면 함수를 종료해준다.
//
void *ThreadFunc(void *arg) {
	MyArg *my_arg = (MyArg *) arg;	
	unsigned int start_num;
	bool *visit_vertice;
	visit_vertice = (bool *) malloc(my_arg->vertice_num + 1);
	unsigned int *search_array;
	search_array = (unsigned int *) malloc(sizeof(unsigned int) * my_arg->vertice_num);
	//house[g_cnt_global]을 기준으로 해서 다른 집들과의 거리르 구한 후, 다 구하면 
	//다른 스레드들이 거리를 구하고 있는 집들을 락을 통해서  건너뛰고 
	//다음 집인 house[g_cnt_global]을 구해준다.
	while(g_cnt_global < (my_arg->house_num - 1)) {		
		pthread_mutex_lock(&g_mutex);
		start_num = g_cnt_global;
		g_cnt_global ++;
		pthread_mutex_unlock(&g_mutex);
		memset(visit_vertice, 0, sizeof(bool) * (my_arg->vertice_num + 1));
		Search(my_arg->vertice, visit_vertice, my_arg->house_distance_least,
		my_arg->house_distance_most, my_arg->vertice_num, my_arg->house_num,
									my_arg->house, start_num, search_array);
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

	g_house_distance_i_to_all = (unsigned int **) malloc(sizeof(unsigned int *) * (house_num));
	for (unsigned int i = 0; i < house_num; i++) {
		g_house_distance_i_to_all[i] = (unsigned int *) malloc(sizeof(unsigned int) * (house_num));
	}

	g_i_max = (unsigned int *) malloc(sizeof(unsigned int) * house_num);


	//한 vertex와 인접한 vertex들정보와 집이 존재하는 여부와 집에 대한 정보를
	//구조체 Vertex배열에다가 저장해둔다.
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

	//병렬적으로 집들의 최단 최장거리를 구하기 위해 배열을 통해서 
	//한 집과 나머지 집들사이의
	//최단최장거리를 관리한다.
	//해당 배열의 인덱스는 기준이 되는 집의 인덱스넘버이다.
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


	//만약 집들이 코어개수보다 작으면 집들 개수만큼 스레드펑션을 실행한다.
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

	//저장된 최단, 최장거리 배열을 통해 최단 최장거리를 구한다.
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
	//모든 변수는 register로 선언해서 좀더 빠르게 루프를 돌 수있도록 한다.
	
	register unsigned int distance = 0;
	search_array[0] = house[start_num];
	bool *visit_house = (bool *)malloc(vertice_num + 1);
	
	//------------------------ for문안에서 지속적으로 변화하는 값
	register unsigned int adjacency_vertex;
	register unsigned int current_vertex;	
	register Vertex vertice_current; 	
	register unsigned int temp_house;
	//------------------------
	
	//distance값이 증가되는 시점을 구하기 위해 사용한다.
	register unsigned int total_count = 0;
	register unsigned int cycle_count = 0;
	register unsigned int search_array_count = 1;
	//-------------------------------------------------
	

	//최단거리는 최초로 집들이 발견되면 그 집과의 거리가 최단거리이므로 이를 
	//구하기 위해서 플래그를 나둔다.
	bool least_flag = true;
	register unsigned int least = 0;
	register unsigned int most = 0;

	visit_vertice[house[start_num]] = 1;
	
	//각각의 루프를 돌리는 루프 배리어블이다.
	register unsigned int i = 0;
	register unsigned int count_end = 1 + start_num;		//만약 집이 64개라 할 때, 0번집은 1~63번지과의 거리를
															//구하면 되고 1번집은 2~63번까지 2번은 3~63번까지 점점
															//줄어드는 식으로 거리를 구하기만 하면 되므로 이를
															//계산하기 위한 변수이다.
	register unsigned int j;
	while(true) {
		current_vertex = search_array[i];		//search_array에 저장된 vertex을 방문한다.
		vertice_current = vertice[current_vertex];//방문한 vertex의 정보를 가져온다.
		if (vertice_current.is_house) {//방문한 vertex가 집인지를 판단한다.
			temp_house = vertice_current.house;
			if(!visit_house[temp_house]) {
				visit_house[temp_house] = 1;
				for(j = start_num + 1; j < house_num; j++) {
					if (house[j] == temp_house) {
						count_end++;
						g_house_distance_i_to_all[start_num][j] = distance + 1;
						break;
					}
				}
				if (!least_flag) {
					most = distance;
				}else {
					if(distance > 1) {
						least = distance;
						least_flag = false;
						for (unsigned int q = 0; q < house_num - 1; q ++) {
							if (g_i_max[q] != 0){
								if (g_house_distance_i_to_all[q][start_num] + g_i_max[q] < g_total_max) {
									house_distance_least[start_num] = least + 1;
									house_distance_most[start_num] = least + 1;
									return;
								}
							}
						}

					}
				}
				if(count_end == house_num) {	//위에서 말한대로 각 집들과의 거리를 다 구하면 리턴한다.
					house_distance_least[start_num] = least + 1;
					house_distance_most[start_num] = most + 1;
					if (g_total_max < most + 1) {
						g_total_max = most + 1;
					}
					g_i_max[start_num] = most + 1;
					return;
				}
			}
		}

		j = 0;//방문한 집들과 인접한 집들을(단, 방문하지 않은) 전부 search_array에 넣어준다.
			//이런식으로 넣게 된다면 
		do {
			adjacency_vertex = vertice_current.adjacency[j];
			if (!visit_vertice[adjacency_vertex]) {
				search_array[search_array_count] = adjacency_vertex;
				visit_vertice[adjacency_vertex] = 1;
				search_array_count ++;
				total_count ++;
			}
			j++;
		} while (j < vertice_current.count);
		
		//만약 같은 distance의 거리의 vertex를 전부 다 방문했다면 다음 distance로 넘어간다.
		if(i == cycle_count) {
			cycle_count = cycle_count + total_count;
			total_count = 0;
			distance++;
		}

		i++;
	}
				

}
