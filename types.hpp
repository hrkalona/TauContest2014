/*
 * types.hpp
 *
 *  Created on: Nov 11, 2013
 *      Author: babis
 */

#ifndef TYPES_HPP_
#define TYPES_HPP_

#include <map>
#include <string>
#include <vector>
#include <set>

#define DEBUGGING 0

#define CACHING 1

#define TREE_CLK_NW 1

#define BRANCH_AND_BOUND 1

#define BENCHMARKING 0

#define PROGRESS_BAR 1

#define GRAPHING 0

#define INPUT_TESTING 0

#define ALL_TESTS -1
#define ALL_PATHS -1

#define NUM_THREADS 8

using namespace std;

typedef double fp_type;

typedef enum
{
	NOT_A_CNW_NODE, CNW_NODE, CLOCK_NODE, FF_CLOCK_NODE, CNW_TEST_NODE
} CLOCK_NETWORK;

typedef enum
{
	SETUP_TEST, HOLD_TEST, BOTH_TESTS
} TESTS_TYPE;

#if CACHING
typedef struct credit_cache
{
	fp_type credit_hold;
	fp_type credit_setup;

	credit_cache() :
			credit_hold( -1 ), credit_setup( -1 )
	{
	}
} credit_cache;
#endif

/* Depth first searcFh specific */
typedef struct dfs
{
	unsigned int route;
	struct node * launching_pin;
	fp_type pre_slack_hold;
	fp_type pre_slack_setup;
} dfs;

/* Clock network reduced graph specific */
typedef struct container
{
	node *clk_nw_node;  // pointer the the actual node of the circuit

#if !TREE_CLK_NW
	set<container *> prev;// backward connections
	int id;// for the dominators algorithm
#endif

#if TREE_CLK_NW
	container * prev;  // backward connection
#endif

#if CACHING
	int ff_clk_id;
#endif

} container;

/* a clock structure*/
typedef struct _clk
{
	node *clk_node;
	fp_type clk_T;
	container *clk_nw_root;
} clk;

typedef struct _path
{
	vector<node *> path;  // nodes that consist a path

	fp_type pre_slack;  // pre slack of a path
	fp_type post_slack;  // post slack of a path

	/* post_slack is used as the sorting key */
	bool operator<(const _path& rhs) const
	{
		return post_slack < rhs.post_slack;
	}

	bool operator==(const _path& rhs) const
	{
		return post_slack == rhs.post_slack;
	}
} pathInfo;

typedef struct _test
{
	multiset<pathInfo> paths;  // an ordered set of paths

	fp_type pre_slack;  // pre slack of a test
	fp_type post_slack;  // post slack of a test

	/* post_slack is used as the sorting key */
	bool operator<(const _test& rhs) const
	{
		return post_slack < rhs.post_slack;
	}

	bool operator==(const _test& rhs) const
	{
		return post_slack == rhs.post_slack;
	}
} testInfo;

/* The setup & hold constraint */
typedef struct setup_hold_constraint
{
	fp_type setup;
	fp_type hold;

	fp_type rat_early;
	fp_type rat_late;

	fp_type pre_slack_setup;
	fp_type pre_slack_hold;

	fp_type post_slack_setup;
	fp_type post_slack_hold;

	bool critical_setup;
	bool critical_hold;

	node *clk;  // capturing clock pin

} setup_hold_constraint;

/* A delay edge */
typedef struct
{
	fp_type delay_early;  // the early delay of the edge
	fp_type delay_late;  // the late delay of the edge
	struct node *connected_to;  // the connected node
} delay_to_node;

/* A pin-node */
typedef struct node
{
	string name;

	/* Bfs algorithm -- specific */
	/* indicates whether a node has been visited or not */
	bool visited;
	/*****************************/

	/* Clock network graph reduction specific */
	CLOCK_NETWORK clk_network;

#if !TREE_CLK_NW
	set<container*> set_ptr;
#endif
#if TREE_CLK_NW
	container * ptr;
#endif
	/******************************************/

	fp_type at_early;  // arrival time - early
	fp_type at_late;  // arrival time - late

	setup_hold_constraint *test_pin;  // setup & hold constraint only for the test pin of a flip flop
	vector<delay_to_node *> delays_forward;  // forward adjacent nodes
	vector<delay_to_node *> delays_backward;  // backward adjacent nodes
} node;

typedef map<string, node *> node_map;  // a mapping from a string ( pin name ) to a node ( pin-node's timing information )
typedef pair<string, node *> hash_entry;

#endif /* TYPES_HPP_ */
