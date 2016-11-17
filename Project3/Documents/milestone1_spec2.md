
먼저 데이터베이스의 initialize는 innobase_init에서 시작합니다.


 - Ha_innodb.cc
innobase_init의 함수에

err = innobase_start_or_create_for_mysql();

으로 innobase를 시작하고,생성해줍니다.


 - Srv0start.cc
innobase_start_or_create_for_mysql의 함수에는 

err = buf_pool_init(srv_buf_pool_size, srv_buf_pool_instance);

의 코드를 사용하여 buffer pool을 init 해줍니다. 
여기서 2개의 인자는 만일 480G의 메모리를 데이터 베이스로 주었을 때,
srv_buf_pool_size = innobase_buffer_pool_size (480G),
srv_buf_pool_instance = innobase_buffer_pool_instace. -> innobase_buffer_pool_instance = 16 고정값
이 됩니다. 

 - Buf0buf.cc
buf_pool_init함수에는 

size = srv_buf_pool_size / srv_buf_pool_instance -> (480G로 가정했을 때) 30G 이고 

buffer_pool_instance 개수만큼 for문이 돌아갑니다. (16)
for (i = 0; i < n_instances; i++) {
	buf_pool_t*	ptr	= &buf_pool_ptr[i];

	//각 인스턴스 포인터, 버퍼 풀당 사이즈(30G), 인스턴스 번호 가 들어갑니다.
	if (buf_pool_init_instance(ptr, size, i) != DB_SUCCESS) {

		/* Free all the instances created so far. */
		buf_pool_free(i);

		return(DB_ERROR);
	}
}

buf_pool_init_instance함수는 

buf_pool->n_chunks = 1 // chunk를 1개 생성해 줍니다.
buf_pool->chunks = chunk = (buf_chunk_t*) mem_zalloc(sizeof *chunk); // chunk를 생성해줍니다. 
buf_chunk_init(buf_pool, chunk, buf_pool_size) // 이 chunk를 init해줍니다. 
으로 이루어져 있습니다.


buf_chunk_init함수는 (480G로 가정시) 다음과 같이 이루어져 있습니다.

buf_chunk_t *chunks
chunks->mem_size = 30G
chunks->size = 30G / UNIV_PAGE_SIZE(1<<14) -> 33,000,000
buf_block_t *blocks <- 30G size
여기서 block의 크기는 30G로 할당됩니다.

각 block마다 frame 을 연결시켜 줍니다.
frame에 block을 연결해 준 후에는 각각 다음 block과 frame을 가르키게 됩니다.
for (i = chunk->size; i--; ) {

	buf_block_init(buf_pool, block, frame);
	UNIV_MEM_INVALID(block->frame, UNIV_PAGE_SIZE);

	/* Add the block to the free list */
	UT_LIST_ADD_LAST(list, buf_pool->free, (&block->page));

	ut_d(block->page.in_free_list = TRUE);
	ut_ad(buf_pool_from_block(block) == buf_pool);

	block++;
	frame += UNIV_PAGE_SIZE;
}

여기서
buf_block_init함수는 frame과 block을 연결시켜주는 함수입니다. 이 함수를 보면, 

mutex_create(PFS_NOT_INSTRUMENTED, &block->mutex, SYNC_BUF_BLOCK);
rw_lock_create(PFS_NOT_INSTRUMENTED, &block->lock, SYNC_LEVEL_VARYING);

위 2개의 함수가 있는 것을 알 수 있습니다. 

block의 mutex와 lock은 각각 

block->mutex /*!< mutex protecting this block:
		state, io_fix, buf_fix_count,
		and accessed; we introduce this new
		mutex in InnoDB-5.1 to relieve
		contention on the buffer pool mutex */

block->lock /*!< read-write lock of the buffer
					frame */'

위와 같이 쓰입니다. 

먼저 mutex_create가 호출하는 함수를 따라가 보면,  

 - Sync0sync.h
#  define mutex_create(K, M, level)				\
	pfs_mutex_create_func((K), (M), __FILE__, __LINE__, #M)

 - Sync0sync.ic
pfs_mutex_create_func
mutex_create_func(mutex, cfile_name, cline, cmutex_name);

 - Sync0sync.cc
mutex_create_func 함수를 ib_prio_mutex_t인자를 가지고 호출 하는 것을 볼 수 있습니다. 
이 함수는 mutex_create_func을 ib_mutex_t인자를 가지고 호출하는데 

이 함수는 
mutex_enter(&mutex_list_mutex);
UT_LIST_ADD_FIRST(list, mutex_list, mutex);
mutex_exit(&mutex_list_mutex);
으로 구성되어 있습니다. 

먼저 mutex_enter를 보면, 
pfs_mutex_enter_func((M), __FILE__, __LINE__)

 - Sync0sync.ic
pfs_mutex_enter_func
mutex_enter_func(mutex, file_name, line);
mutex_list_mutex를 lock를 잡는 것을 볼 수있습니다. 

mutex_exit는 
//Locks a mutex for the current thread. If the mutex is reserved, the function spins a preset time mutex_enter_func
//Unlocks a priority mutex owned by the current thread
mutex_exit_func(ib_prio_mutex_t* mutex)
mutex_list_mutex를 unlock해줍니다. 

위으 mutex에서 쓰인 mutex_list_mutex는 
/** Mutex protecting the mutex_list variable */
UNIV_INTERN ib_mutex_t mutex_list_mutex;
ib_mutex_t 구조체로 선언되어 있습니다. 

이번에는 임계 영역에 있는 UT_LIST_ADD_FIRST(list, mutex_list, mutex); 를 보겠습니다.

Ut0lst.h
// Adds the node as the first element in a two-way linked list.
#define UT_LIST_ADD_FIRST(NAME, LIST, ELEM)	\
	ut_list_prepend(LIST, *ELEM, IB_OFFSETOF(ELEM, NAME))

리스트 제일 첫 부분에 element를 넣어 주는 것을 알 수 있습니다. 
이 함수를 자세히 보면, 

template <typename List, typename Type>
void
ut_list_prepend(
	List&		list,
	Type&		elem,
	size_t		offset)
{
	ut_list_node<Type>&	elem_node = ut_elem_get_node(elem, offset);

 	elem_node.prev = 0;
 	elem_node.next = list.start;

	// 처음 노드를 찾아감
	if (list.start != 0) {
		ut_list_node<Type>&	base_node =
			ut_elem_get_node(*list.start, offset);

		ut_ad(list.start != &elem);

		base_node.prev = &elem;
	}
	// 첫 노드에 해당 element를 붙여줌
	list.start = &elem;

	if (list.end == 0) {
		list.end = &elem;
	}

	++list.count;
}

이렇게 이루어져 있습니다. 
만일 병렬로 처리를 하려면 이 부분에서 쓰이는 lock을 스마트하게 처리해야 되는 것을 알 수 있습니다. 

다음 함수인 rw_lock_create를 따라가 보면,
 - Sync0rw.h
#   define rw_lock_create(K, L, level)				\
	pfs_rw_lock_create_func((K), (L), #L, __FILE__, __LINE__)

 - Sync0rw.ic
pfs_rw_lock_create_func은 rw_lock_create_func를 호출합니다.

 - Sync0rw.cc
rw_lock_create_func를 보면, mutex_create를 호출합니다. 
mutex_create(rw_lock_mutex_key, rw_lock_get_mutex(lock),
	     SYNC_NO_ORDER_CHECK);

이 함수는 위에서 mutex_create를 설명할 때 쓰였던 함수입니다.
따라서 첫 노드에 해당 element를 붙여주는 부분으로 다시 귀결 되는 것을 볼 수 있습니다.






