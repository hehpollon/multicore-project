# multicore-project

Buffer pool initialize multicore support

template <typename List, typename Type>
void
ut_list_free_prepend(
	List&		list,
	Type&		elem,
	size_t		offset)
{
	Type *prev = NULL;
	ut_list_node<Type>&	elem_node = ut_elem_get_node(elem, offset);
 	elem_node.prev = 0;
	prev = __sync_lock_test_and_set(&(list.start), &elem);
	elem_node.next = prev;
	if(prev != 0) { 
		ut_list_node<Type>&	prev_node = ut_elem_get_node(*prev, offset);
		prev_node.prev = &elem;
	}

	if (list.end == 0) {
		list.end = &elem;
	}

	__sync_fetch_and_add(&list.count, 1);
	
}

template <typename List, typename Type>
void
ut_list_free_append(
	List&		list,
	Type&		elem,
	size_t		offset)
{
	Type *next = NULL;
	ut_list_node<Type>&	elem_node = ut_elem_get_node(elem, offset);
 	elem_node.next = 0;
	next = __sync_lock_test_and_set(&(list.end), &elem);
	elem_node.prev = next;
	if(next != 0) { 
		ut_list_node<Type>&	next_node = ut_elem_get_node(*next, offset);
		next_node.next = &elem;
	}

	if (list.start == 0) {
		list.start = &elem;
	}

	__sync_fetch_and_add(&list.count, 1);
	
}
