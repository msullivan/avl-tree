#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef int (*avl_cmp_func)(void *, void *);
typedef int (*avl_visit_func)(void *);

typedef enum avl_balance_t { LEFT_HIGHER, RIGHT_HIGHER, SIDES_EQUAL }
	avl_balance_t;

typedef struct avl_node_t {
	struct avl_node_t *left;
	struct avl_node_t *right;
	void *data;
	/*avl_balance_t balance;*/
	int height;
	int debug;
} avl_node_t;

#define TREE_HEIGHT(n) ((n) ? ((n)->height) : 0)

static avl_node_t *avl_rotate_right(avl_node_t *node);
static avl_node_t *avl_rotate_left(avl_node_t *node);
static void avl_update_height(avl_node_t *node);
static void avl_check_invariant(avl_node_t *root);


avl_node_t *avl_new_node(avl_node_t *left, avl_node_t *right, void *data,
                         int height)
{
	static int debug = 1;
	avl_node_t *node = calloc(1, sizeof(avl_node_t));
	node->left = left;
	node->right = right;
	node->data = data;
	node->height = height;
	node->debug = debug++;
	return node;
}

/* FIXME: far too much duplication */
avl_node_t *avl_insert(avl_node_t *root, void *data, avl_cmp_func cmp)
{
	int left_height, right_height;
	int sub_l_height, sub_r_height;
	int sub_l_height_new, sub_r_height_new;
	int n;
	avl_node_t *subtree;
	
	if (!root) {
		return avl_new_node(NULL, NULL, data, 1);
	}


	left_height = TREE_HEIGHT(root->left);
	right_height = TREE_HEIGHT(root->right);
	n = cmp(data, root->data);

	if (n == 0) {
		/* aw, shit. figure out way to signal this */
		return root;
	} else if (n < 0) {
		/* needs to go on the left */
		subtree = root->left;
		
		if (left_height <= right_height) {
			/* simple case: just insert on the left */
			root->left = avl_insert(subtree, data, cmp);
		} else {
			/* tricky case... */
			assert(subtree);

			/* store the heights of those subtrees */
			sub_l_height = TREE_HEIGHT(subtree->left);
			sub_r_height = TREE_HEIGHT(subtree->right);

			root->left = avl_insert(subtree, data, cmp);

			/* get the new heights of the subtrees */
			sub_l_height = TREE_HEIGHT(subtree->left);
			sub_r_height = TREE_HEIGHT(subtree->right);
			
			if (sub_l_height == root->height - 2 &&
			    sub_l_height_new == root->height - 1) {
				/* rotate left */
				root = avl_rotate_right(root);
			} else if (sub_r_height == root->height - 2 &&
			           sub_r_height_new == root->height - 1) {
				/* double rotation */
				root->left = avl_rotate_left(subtree);
				root = avl_rotate_right(root);
			}
		}
	} else {
		/* needs to go on the right */
		subtree = root->right;
		
		if (right_height <= left_height) {
			/* simple case: just insert on the right */
			root->right = avl_insert(subtree, data, cmp);
		} else {
			/* tricky case... */
			/* XXX: FIXME: likely wrong */
			assert(subtree);

			/* store the heights of those subtrees */
			sub_l_height = TREE_HEIGHT(subtree->left);
			sub_r_height = TREE_HEIGHT(subtree->right);

			root->right = avl_insert(subtree, data, cmp);

			/* get the new heights of the subtrees */
			sub_l_height = TREE_HEIGHT(subtree->left);
			sub_r_height = TREE_HEIGHT(subtree->right);
			
			if (sub_r_height == root->height - 2 &&
			    sub_r_height_new == root->height - 1) {
				/* rotate left */
				root = avl_rotate_right(root);
			} else if (sub_l_height == root->height - 2 &&
			           sub_l_height_new == root->height - 1) {
				/* double rotation */
				root->right = avl_rotate_left(subtree);
				root = avl_rotate_right(root);
			}


		}
	}

	avl_update_height(root);
	return root;
}

static void avl_check_invariant(avl_node_t *root)
{
	int left_height, right_height;

	if (!root)
		return;

	left_height = TREE_HEIGHT(root->left);
	right_height = TREE_HEIGHT(root->right);

	if (abs(left_height - right_height) > 1) {
		fprintf(stderr, "invariant violated at %d! %d, %d\n", root->debug,
				left_height, right_height);
		/*abort();*/
	}
	
	avl_check_invariant(root->left);
	avl_check_invariant(root->right);
}

static void avl_update_height(avl_node_t *node)
{
	int left_height = 0, right_height = 0;
	if (node->left)
		left_height = node->left->height;
	if (node->right)
		right_height = node->right->height;

	node->height =
		(left_height > right_height ? left_height : right_height) + 1;
}


static avl_node_t *avl_rotate_right(avl_node_t *node)
{
	avl_node_t *replacement = node->left;
	if (!replacement)
		return node;
	
	node->left = replacement->right;
	replacement->right = node;

	avl_update_height(replacement->right);
	avl_update_height(replacement);
	
	return replacement;
}

static avl_node_t *avl_rotate_left(avl_node_t *node)
{
	avl_node_t *replacement = node->right;
	if (!replacement)
		return node;
	
	node->right = replacement->left;
	replacement->left = node;

	avl_update_height(replacement->left);
	avl_update_height(replacement);
	
	return replacement;
}


#define INT_TO_P(x) ((void *)(long)(x))
#define P_TO_INT(p) ((int)(long)(p))

void avl_debug(avl_node_t *root)
{
	int l, r;
	if (!root)
		return;

	l = root->left ? root->left->debug : 0;
	r = root->right ? root->right->debug : 0;
	
	avl_debug(root->left);
	printf("%d-%d-%d(%d,%d) ",
	       root->debug, P_TO_INT(root->data), root->height, l, r);
	avl_debug(root->right);
}

void avl_display(avl_node_t *root, int level)
{
	int c;
	if (!root)
		return;

	avl_display(root->left, level+1);
	for (c = 2*level; c; c--)
		putchar(' ');
	printf("%02d\n", root->debug);

	avl_display(root->right, level+1);
}


void avl_iterate(avl_node_t *root, avl_visit_func visit)
{
	if (!root)
		return;
	avl_iterate(root->left, visit);
	visit(root->data);
	avl_iterate(root->right, visit);
}

int test_print(void *p)
{
	printf("%d ", P_TO_INT(p));
	return 0;
}

int test_cmp(void *p, void *q)
{
	return P_TO_INT(p) - P_TO_INT(q);
}

int main(void)
{
	int n, i;
	avl_node_t *tree = NULL;

	
	for (i = 0; i < 100; i++) {
		n = rand() % 100;
		tree = avl_insert(tree, INT_TO_P(n), test_cmp);
		avl_debug(tree); printf("\n");
		avl_display(tree, 0);
		avl_check_invariant(tree);
		printf("inserting %d\n", n);
		/*avl_iterate(tree, test_print); printf("\n");*/
	}

	return 0;
}
