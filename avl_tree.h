#ifndef __AVL_TREE_H
#define __AVL_TREE_H

typedef int (*avl_cmp_func)(void *, void *);

//typedef enum avl_balance_t { LEFT_HIGHER, RIGHT_HIGHER, SIDES_EQUAL }
//	avl_balance_t;
typedef enum avl_dir_t { AVL_LEFT, AVL_RIGHT }
	avl_dir_t;

typedef struct avl_node_t {
	struct avl_node_t *parent;
	union {
		struct {
			struct avl_node_t *left;
			struct avl_node_t *right;
		};
		struct avl_node_t *links[2];
	};
	void *data;
	int height;
} avl_node_t;

typedef struct avl_tree_t {
	avl_node_t dummy;

	avl_cmp_func lookup_cmp;
	avl_cmp_func insert_cmp;
} avl_tree_t;

#define AVL_NODE_INIT {NULL, {NULL, NULL}, NULL, 1, -1}

//// Definite API things
void avl_node_init(avl_node_t *node);
int avl_init(avl_tree_t *tree,
             avl_cmp_func lookup_cmp, avl_cmp_func insert_cmp);
avl_node_t *avl_lookup(avl_tree_t *tree, void *data);
void avl_insert(avl_tree_t *tree, avl_node_t *node, void *data);
void avl_delete(avl_tree_t *tree, avl_node_t *node);

// node-level stuff that definitely needs exposed
avl_node_t *avl_node_end(avl_node_t *node, avl_dir_t dir);
avl_node_t *avl_node_first(avl_node_t *node);
avl_node_t *avl_node_last(avl_node_t *node);
avl_node_t *avl_step(avl_node_t *node, avl_dir_t dir);
avl_node_t *avl_next(avl_node_t *node);
avl_node_t *avl_prev(avl_node_t *node);
static inline avl_node_t *avl_get_root(avl_tree_t *tree) {
	return tree->dummy.right;
}
static inline avl_node_t *avl_first(avl_tree_t *tree) {
	return avl_node_first(avl_get_root(tree));
}
static inline avl_node_t *avl_last(avl_tree_t *tree) {
	return avl_node_last(avl_get_root(tree));
}

int avl_check_node(avl_node_t *root);
void avl_check_tree(avl_tree_t *tree);

#endif
