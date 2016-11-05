#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <atomic>
#include <unistd.h>

using namespace std;

//--------------------------------------------typedef--------------------------------------------------

//atomic하게 증가하는 version order를 위해 정의합니다.
typedef atomic<uint64_t> atomic_uint64_t;

//Garvage Collector를 위한 큐 노드입니다.
typedef struct QueueNode {
	uint64_t version = 0;
	struct QueueNode *link;
} QueueNode;

//Garvage Collector를 위한 큐입니다.
typedef struct Queue{
	uint64_t count = 0;
	uint64_t minimum = 0;
	uint64_t delete_front = 0;
	QueueNode *front;
	QueueNode *rear;
} Queue;

//GlobalActiveList의 노드 구조체입니다. 
//해당 노드에는 스레드 index와 현재 update되는 version이 들어갑니다.
typedef struct ActiveNode {
	int index_thread;
	uint64_t version;
	struct ActiveNode *link;
} ActiveNode;

//ThreadSingleList의 노드입니다.
//해당 노드에는 업데이트되는 A와 B의 값 그리고 version이 들어갑니다.
typedef struct SinglyNode {
	int A;
	int B;
	uint64_t version;
	struct SinglyNode *link;
} SinglyNode;


//--------------------------------------------global variable--------------------------------------------------


//version order입니다. atomic하게 1씩 증가합니다.
atomic_uint64_t g_atomic_counter;
//GlobalActiveList의 헤더입니다. 해동 코드에서는 GlobalActiveList를 의미합니다. 총 스레드 개수만큼 존재합니다.
ActiveNode *g_active_list = NULL;
//ThreadSingleList의 헤더입니다. 해동 코드에서는 ThreadSingleList를 의미합니다. 총 스레드 개수만큼 존재합니다.
SinglyNode **g_singly_list = NULL;
//garvate collector를 위한 큐입니다. 총 스레드 개수만큼 존재합니다.
Queue **g_queue;
//mutex입니다.
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
//총 스레드의 개수입니다.
int g_num_thread = 0;
//각 스레드별 throughput입니다.
uint64_t *g_thread_throughput;
//이 플래그 true일때만 각 스레드들이 update를 진행할 수 있습니다. (duration동안 동작하도록 하기 위해 존재합니다.
volatile bool g_could_thread_run = true;
//이 플래그가 true라면 verbose옵션이 적용됩니다.
volatile bool g_verbose_flag = false;
//verbose옵션으로 인해 텍스트파일을 만드는 동안 다른 스레드들의 동작을 정지시킵니다.
volatile bool g_is_verbosing = false;


//--------------------------------------------function--------------------------------------------------
//링크리스트 삽입 삭제, 큐 삽입 삭제관련된 함수이므로 자세한 설명은 생략.

//----------------------------------global active list funtion------------------------
void InsertGlobalActiveList(int index_thread, uint64_t version); //g_active_list에 노드를 삽입함.
void DeleteGlobalActiveList(int index_thread); //g_active_list에 해당 스레드 index를 가진 노드를 삭제함.
void InsertCaptureActiveList(ActiveNode *c_active_list, int index_thread, uint64_t version); //g_active_list 캡처에 사용됨.
ActiveNode *CaptureActiveList(); //g_active_list를 capture를 한 g_active_list를 반환함.
void DeleteCaptureActiveListAll(ActiveNode **ActiveList); //captue된 g_active_list를 삭제함.



//----------------------------------thread singly list funtion-------------------------
void InsertSinglyList(int i, int A, int B, uint64_t version); //g_singly_list[i]에 version 값을 넣어줌.


//----------------------------------for garvage collector queue funtion----------------
Queue* CreateQueue(); //큐생성
void DestroyQueue(Queue *qptr); //큐삭제
void AddRear(Queue *qptr, int version); //큐 선입
void showAllQueue(Queue *qptr);
void RemoveFront(Queue *qptr); //큐 선출
void RemoveVersion(Queue *qptr, uint64_t version); //큐에 해당 version을 가진 노드를 삭제함. (중복된 경우 가장 처음노드)


//----------------------------------기타 funtion----------------------------------------
void *ThreadFunc(void *arg); //update동작을 하는 각 스레드들의 동작이 담긴 function
void *ThreadGarvageCollectionFunc(void *arg); //garvage_collector를 위한 function
void randomize(void); // 아래에 randomize()를 정의했다는 사실을 컴파일러에게 알려줌


//----------------------------------bakery lock------------------------------------------
const int TRUE = 1;
const int FALSE = 0;
volatile int *g_bakery_number; //bakery_lock의 티켓의 값을 저장할 배열
volatile int *g_bakery_choosing; //bakery_lock이 선택되었는지를 알려줌.

//큰 값을 반환하여 주는 역할을 합니다.
int BakeryMax() { 
	int i;
	int num_max = 0; //가장 큰 번호표를 담을 변수 초기값을 0으로 설정해준다. 아무도 번호표가
	
	// 없을시 0을 반환하기 위해.
	for (i = 0; i<g_num_thread; i++) { // 프로세스의 개수만큼을 비교한다.
		if (num_max<g_bakery_number[i]) {
			num_max = g_bakery_number[i]; //가장 큰값을 num_max변수에 저장한다.
		}
	}
	return num_max; //num_max 변수를 반환해준다.
}

//빵집알고리즘의 시작
void bakery_lock(int i) {
	
	int j;
	g_bakery_choosing[i] = TRUE; // 티켓을 받기 전 의 상태를 저장.
	g_bakery_number[i] = BakeryMax() + 1; // BakeryMax함수를 통해서 이미 발부된 티켓 중 가장 높은 값의 다음값을 받는다.
	g_bakery_choosing[i] = FALSE; // 티켓을 받은 후 의 상태를 저장.
	
	//이for문으로 모든 프로세스 티켓 과 프로세스번호로 통과 가능한지를 검사함
	for (j = 0; j<g_num_thread; j++) {
		//이 while 문에서 프로세스가 티켓을 받았는지를 검사함.
		while (g_bakery_choosing[j]) { 
			// 티켓을 받지 않았으면 대기상태로 빠짐.
		} 
		usleep(1000);
		//printf("thread %d's first pass to %d!!!\n", i, j);
		// 이 while 문에서 티켓의 번호를 비교 검사함.
		while ((g_bakery_number[j] != 0) && 
			((g_bakery_number[j] < g_bakery_number[i]) || 
			((g_bakery_number[j] == g_bakery_number[i]) && (j < i)))) { 
		}
	}


}

//프로세스가 연산을 하고나면 Busy Waiting중인 프로세스를 풀어주는 함수
void bakery_unlock(int i) {
	g_bakery_number[i] = 0; //연산을 다 한 프로세스의 티켓을 0으로 바꾸어주면 while문의 g_bakery_number[j] !=0
} // 조건에서 false가 되므로 Busy Waiting상태가 풀리게 됨.





int main(int argc, char *argv[]) {
	//랜덤한 값을 초기화시키기 위한 random초기화.
	randomize();

	//version order를 위한 g_atomic_counter를 0으로 초기화하고 인자를 받아옴.
	g_atomic_counter = 0;
	if (argc < 3 || argc>4) {
		printf("please insert complete argument\n");
		exit(1);
	}
	g_num_thread = atoi(argv[1]);
	int duration = atoi(argv[2]);
	
	//만약 verbose옵션이 주어진다면 g_verbose_flag를 true로 바꾸어 verbose동작하도록 함.
	if (argc == 4) {
		if (strcmp("verbose", argv[3]) == 0) {
			g_verbose_flag = true;
		}
		else {
			printf("please insert complete argument\n");
			exit(1);
		}
	}

		
	//correctness체크를 위한 변수
	int before_correctness = 0;
	int after_correctness = 0;

	//각각의 global변수들을 초기화.
	g_singly_list = (SinglyNode **)malloc(sizeof(SinglyNode *) * (g_num_thread)); //각 스레드마다 싱글리스트를 만들어줌.
	g_thread_throughput = (uint64_t *)malloc(sizeof(uint64_t) * g_num_thread);//각 스레드마다 스로풋측정을 함.
	g_queue = (Queue **)malloc(sizeof(Queue *) * (g_num_thread));//각 스레드마다 큐를 가짐.
	g_bakery_number = (int *)malloc(sizeof(int) * g_num_thread);//스레드 개수만큼의 bakery락의 티켓을 가짐.
	g_bakery_choosing = (int *)malloc(sizeof(int) * g_num_thread);//스레드개수만큼의 bakery락의 선택flag를 가짐.

	for (int i = 0; i < g_num_thread; i++) {
		g_singly_list[i] = NULL;
		g_queue[i] = CreateQueue();
		g_bakery_number[i] = 0;
		g_bakery_choosing[i] = FALSE;
	} 

	//최초에 각 스레드에 들어있는 값을 설정해주고 해당 collectness측정을 위한 값 설정.
	for (int i = 0; i < g_num_thread; i++) {
		int a = rand() % 1000;
		int b = rand() % 1000;
		InsertSinglyList(i, a, b, 0);
		before_correctness = before_correctness + (a + b);
	}

	//각 스레드들을 동작시킴. 이때 각 스레드 index값을 매개변수로 전달.
	pthread_t threads[g_num_thread];
	int index[g_num_thread];
	for (int i = 0; i < g_num_thread; i++) {
		index[i] = i;
		if (pthread_create(&threads[i], NULL, ThreadFunc, (void *)&index[i]) < 0) {
			return 0;
		}
	}

	//garvage collector를 위한 스레드 동작.
	pthread_t garvage_collector;
	if (pthread_create(&garvage_collector, NULL, ThreadGarvageCollectionFunc, NULL) < 0) {
		return 0;
	}

	//verbosed 옵션 적용을 위해 파일 포인터를 스레드개수만큼 생성시키고 열어줌.
	FILE *fp[g_num_thread];
	char filename[100];
	if (g_verbose_flag) {
		for (int i = 0; i < g_num_thread; i++) {
			sprintf(filename, "verbose_thread_%d.txt", i);
			fp[i] = fopen(filename, "w");
		}
	}
	
	//verbose옵션이 적용되어 verbose_flag가 true일때 파일에 값을 넣어줌.
	for (int i = 0; i < (10 * duration); i++) {
		//0.1초마다 verbose체크를 해서 총 duration동안 체크를 함.
		usleep(1000 * 100);
		if (g_verbose_flag) {
			//pthread_mutex_lock(&g_mutex);
			g_is_verbosing = true; //해당 flag가 true가 되면 다른 스레드들의 동작이 멈춤.
			for (int i = 0; i < g_num_thread; i++) {
				if (!fp[i])
					continue;
				SinglyNode *tmp;
				SinglyNode *tmp2;
				tmp = g_singly_list[i];
				tmp2 = tmp;
				int num_singly_list = 0;

				//각 스레드의 총 크기를 계산함.
				uint64_t version = tmp->version;
				while (tmp != NULL) {
					num_singly_list = num_singly_list + 1;
					tmp = tmp->link;
				}
				fprintf(fp[i], "%ld\n", num_singly_list * sizeof(SinglyNode));
				//각 스레드에 존재하고 있는 값을 넣어줌.
				while (tmp2 != NULL) {
					fprintf(fp[i], "%ld ", tmp2->version);
					tmp2 = tmp2->link;
				}
				//각 스레드의 최신버전을 넣어줌.
				fprintf(fp[i], "\n%ld\n", version);
			}
			g_is_verbosing = false; //해당 flag가 false가 되면 다른 스레드들의 동작이 다시 시작됨.
		}
	}
	//각 파일을 닫아줌.
	if (g_verbose_flag) {
		for (int i = 0; i < g_num_thread; i++) {
			fclose(fp[i]);
		}
	}
	
	//duration이 지나면 flag를 false로 바꾸어
	//모든 스레드들의 활동을 멈추고 스레드를 종료시킴.
	g_could_thread_run = false;
	for (int i = 0; i < g_num_thread; i++) {
		pthread_join(threads[i], NULL);
	}
	pthread_join(garvage_collector, NULL);

	//모든 업데이트가 진행된 후의 collectness측정과 througput과 fairness를 측정함.
	uint64_t total_throughput = 0;
	uint64_t total_throughput_s = 0;
	for (int i = 0; i < g_num_thread; i++) {
		after_correctness = after_correctness + (g_singly_list[i]->A + g_singly_list[i]->B);
		total_throughput = total_throughput + g_thread_throughput[i];
	}
	for (int i = 0; i < g_num_thread; i++) {
		total_throughput_s = total_throughput_s + (g_thread_throughput[i] * g_thread_throughput[i]);
	}

	printf("%d\n", before_correctness);
	printf("%d\n", after_correctness);
	printf("%ld\n", ((g_atomic_counter.load()) / duration));
	printf("%f\n", ((double)(total_throughput * total_throughput) / (double)(g_num_thread * total_throughput_s)));
	
	return 0;
}


void InsertGlobalActiveList(int index_thread, uint64_t version) {
	//node를 동적할당 해주고 node에 정보를 채워준다.
	ActiveNode *node;
	node = (ActiveNode *)malloc(sizeof(ActiveNode));
	node->index_thread = index_thread;
	node->version = version;
	node->link = g_active_list;
	g_active_list = node;
}

void DeleteGlobalActiveList(int index_thread) {
	//node를 동적할당 해주고 node에 정보를 채워준다.

	ActiveNode *current_node;
	current_node = g_active_list;
	ActiveNode *back_node = NULL;
	while (current_node != NULL) {
		if (current_node->index_thread == index_thread) {
			if (back_node != NULL) {
				back_node->link = current_node->link;
				free(current_node);
				break;
			}
			else {
				g_active_list = current_node->link;
				free(current_node);
				break;
			}
		}
		else {
			back_node = current_node;
		}
		current_node = current_node->link;
	}
}


void InsertCaptureActiveList(ActiveNode **c_active_list, int index_thread, uint64_t version) {
	//node를 동적할당 해주고 node에 정보를 채워준다.
	ActiveNode *node;
	node = (ActiveNode *)malloc(sizeof(ActiveNode));
	node->index_thread = index_thread;
	node->version = version;
	node->link = *c_active_list;
	*c_active_list = node;
	//printf("In insert_node function: %p\n", *c_active_list);
}


ActiveNode *CaptureActiveList() {
	ActiveNode *tmp;
	tmp = g_active_list;
	ActiveNode *c_active_list = NULL;
	while (tmp != NULL) {
		InsertCaptureActiveList(&c_active_list, tmp->index_thread, tmp->version);
		tmp = tmp->link;
	}
	return c_active_list;
}



void DeleteCaptureActiveListAll(ActiveNode **ActiveList) {
	//node를 동적할당 해주고 node에 정보를 채워준다.

	ActiveNode *current_node;
	current_node = *ActiveList;
	ActiveNode *back_node = NULL;
	while (current_node != NULL) {
		back_node = current_node;
		current_node = current_node->link;
		free(back_node);
	}
	//free(*ActiveList);
}

void ShowGlobalActiveListAll() {
	ActiveNode *tmp;
	tmp = g_active_list;
	while (tmp != NULL) {
		printf("thread[%d]'s version : %ld\n", tmp->index_thread, tmp->version);
		tmp = tmp->link;
	}
}

void ShowCaptureActiveListAll(ActiveNode *ActiveList) {
	ActiveNode *tmp;
	tmp = ActiveList;
	while (tmp != NULL) {
		printf("thread[%d]'s version : %ld\n", tmp->index_thread, tmp->version);
		tmp = tmp->link;
	}
}

void InsertSinglyList(int i, int A, int B, uint64_t version) {
	//node를 동적할당 해주고 node에 정보를 채워준다.
	SinglyNode *node;
	node = (SinglyNode *)malloc(sizeof(SinglyNode));
	node->A = A;
	node->B = B;
	node->version = version;
	node->link = g_singly_list[i];
	g_singly_list[i] = node;
}

void ShowListAll(int i) {
	SinglyNode *tmp;
	tmp = g_singly_list[i];
	while (tmp != NULL) {
		printf("thread %d : %d , %d at version %ld\n", i, tmp->A, tmp->B, tmp->version);
		tmp = tmp->link;
	}
}


//큐를 제거합니다.
void RemoveFront(Queue *qptr) {
	if (qptr->count==0) {
		return;
	}
	else {
		//printf("remove hocul\n");
		QueueNode *tmp = qptr->front;
		qptr->front = tmp->link;
		free(tmp);
		qptr->count--;
		return;
	}
}

//큐를 제거합니다.
void RemoveVersion(Queue *qptr, uint64_t version) {
	if (qptr->count == 0) {
		return;
	}
	else {
		QueueNode *current_node = qptr->front;
		QueueNode *back_node = NULL;
		while (current_node != NULL) {
			if (current_node->version == version) {
				if (back_node != NULL) {
					back_node->link = current_node->link;
					free(current_node);
					if (back_node->link == NULL) {
						qptr->rear = back_node;
					}
					qptr->count--;
					break;
				}
				else {
					qptr->front = current_node->link;
					qptr->count--;
					free(current_node);
					break;
				}
			}
			else {
				back_node = current_node;
			}
			current_node = current_node->link;
		}
	}
}

//Queue를 생성하고 Queue포인터를 리턴합니다.
Queue* CreateQueue() {
	Queue *new_queue = (Queue *)malloc(sizeof(Queue));
	new_queue->count = 0;
	new_queue->front = NULL;
	new_queue->rear = NULL;
	return new_queue;
}

//큐를 삭제합니다.
void DestroyQueue(Queue *qptr) {
	while (qptr->count != 0) {
		RemoveFront(qptr);
	}
	free(qptr);
}

//큐에 삽입을 합니다.
void AddRear(Queue *qptr, int version) {
	QueueNode *new_node = (QueueNode *)malloc(sizeof(QueueNode));
	new_node->version = version;
	new_node->link = NULL;

	if (qptr->count == 0) {
		qptr->front = new_node;
		qptr->rear = new_node;
	}
	else {
		qptr->rear->link = new_node;
		qptr->rear = new_node;
	}
	qptr->count++;
}

void *ThreadFunc(void *arg) {
	//내 스레드의 index를 가져옴.
	int index = *((int *)arg);

	//duration동안만 스레드를 동작시키기 위한 while문. 
	//memory barier를 위해 flag를 volatile로 선언
	while (g_could_thread_run) {
		if (g_is_verbosing == true) {
			continue;
		}
		uint64_t current_version; //현재 globar_order를 저장할 변수.
		ActiveNode *capture_active_list;//현재 global_active_list의 캡쳐를 가르킬 리스트 헤더

		//참조할 다른 스레드의 index를 구함. (내 스레드는 선택되지 않도록 함)
		int picked_thread_index;
		while (true) {
			picked_thread_index = rand() % g_num_thread;
			if (picked_thread_index != index) {
				break;
			}
		}

		ActiveNode *cursor_active_node;//active리스트 탐색을 위한 커서
		bool is_thread_active = false;//내가 참조하려는 스레드가 active한지에 대한 여부를 나타내는 flag
		uint64_t active_thread_version = 0;//내가 참조하려는 스레드의 active_list 상의 버전.

		//atomic하게 캡쳐를 하고 global_active_list에 넣어주고 그리고 
		//garvage_collector동잘을 위해서 큐에다가 내가 참조하려는 스레드와 해당 스레드가 active중이라면 
		//active_thread_version을 넣어줌
		pthread_mutex_lock(&g_mutex);
		//bakery_lock(index);
		g_atomic_counter++;
		current_version = g_atomic_counter.load();
		InsertGlobalActiveList(index, g_atomic_counter.load());
		capture_active_list = CaptureActiveList();

		cursor_active_node = capture_active_list;
		while (cursor_active_node != NULL) {
			if (cursor_active_node->index_thread == picked_thread_index) {
				is_thread_active = true;
				active_thread_version = cursor_active_node->version;
				AddRear(g_queue[picked_thread_index], active_thread_version);
				break;
			}
			cursor_active_node = cursor_active_node->link;
		}
		
		//bakery_unlock(index);
		pthread_mutex_unlock(&g_mutex);

		

		
		//update과정 수행함.
		SinglyNode *cursor_singly_node;
		cursor_singly_node = g_singly_list[picked_thread_index];

		int my_A = g_singly_list[index]->A;
		int my_B = g_singly_list[index]->B;
		if (is_thread_active == true) {
			while (true) {
				if (cursor_singly_node->version < active_thread_version) {
					break;
				}
				cursor_singly_node = cursor_singly_node->link;
			}
			my_A = my_A + (cursor_singly_node->A % 100);
			my_B = my_B - (cursor_singly_node->A % 100);
		}
		else {
			my_A = my_A + (cursor_singly_node->A % 100);
			my_B = my_B - (cursor_singly_node->A % 100);
		}
		InsertSinglyList(index, my_A, my_B, current_version);

		//global_active_list에서 내 스레드를 빼고 그리고 garbage_collector를 위한 큐에서 넣었다면 다시 빼줌.
		pthread_mutex_lock(&g_mutex);
		//bakery_lock(index);
		DeleteGlobalActiveList(index);
		if (is_thread_active == true) {
			if (g_queue[picked_thread_index]->front->version == active_thread_version) {
				RemoveFront(g_queue[picked_thread_index]);
				if (g_queue[picked_thread_index]->front != NULL) {
					if (g_queue[picked_thread_index]->front->version != active_thread_version) {
						g_queue[picked_thread_index]->minimum = active_thread_version;
					}
				}
				else {
					if (g_queue[picked_thread_index]->delete_front != active_thread_version) {
						g_queue[picked_thread_index]->minimum = g_queue[picked_thread_index]->delete_front;
					}
				}
				g_queue[picked_thread_index]->delete_front = active_thread_version;
				
			}
			else {
				RemoveVersion(g_queue[picked_thread_index], active_thread_version);
			}
		}
		pthread_mutex_unlock(&g_mutex);
		//bakery_unlock(index);

		//캡쳐한 global_active_list를 제거함.
		DeleteCaptureActiveListAll(&capture_active_list);
		//내 스레드의 througput을 증가시킴.
		g_thread_throughput[index] ++;
	}
	return NULL;
}

//해당 알고리즘은 readme에 존재함.
void *ThreadGarvageCollectionFunc(void *arg) {
	while (g_could_thread_run) {
		if (g_is_verbosing == true) {
			continue;
		}
		for (int i = 0; i < g_num_thread; i++) {
			SinglyNode *tmp;
			SinglyNode *back = NULL;
			uint64_t minimum = g_queue[i]->minimum;
			tmp = g_singly_list[i];
			bool delete_flag = false;
			while (tmp != NULL) {
				if (delete_flag == false && tmp->version < minimum) {
					delete_flag = true;
					back->link = NULL;
				}
				back = tmp;
				
				tmp = tmp->link;
				if (delete_flag) {
					free(back);
				}
				
			}

		}
	}
	return NULL;
}

void randomize(void) {
	srand((unsigned)time(NULL));
	// 첫부분의 숫자는 거의 랜덤하지 않기에, 앞부분에서 랜덤한 개수로 뽑아서 버림
	for (int i = 0; i < (rand() % RAND_MAX); i++) (rand() % RAND_MAX);
}