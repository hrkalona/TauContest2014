/*
 * engine.cpp
 *
 *  Created on: Nov 11, 2013
 *      Author: babis
 */

#include "engine.hpp"
#include "./dominators/dgraph.h"
#include "auxiliary.hpp"

#include <queue>
#include <iostream>
#include <limits>

#include <boost/thread/mutex.hpp>

using namespace std;
using namespace boost;

mutex critical;
mutex critical2;
mutex critical3;

unsigned int counter = 0;

void sta_engine(node_map& PIs, vector<node*>& test_pins, clk *clock, vector<container *>& container_ptrs, int& nof_ff_clocks)
{
	at_propagation( PIs, clock, container_ptrs, nof_ff_clocks );
}

void at_propagation(node_map& PIs, clk *clock, vector<container *>& container_ptrs, int& nof_ff_clocks)
{
	/* Create a Q */
	queue<node*> Q;  // empty queue

	node_map::iterator p;

	/* enqueue all PIs */
	for (p = PIs.begin(); p != PIs.end(); ++p)
	{
		/* mark PI p as visited */
		p->second->visited = true;
		/* Initialize path for PIs */

		/* Enqueue PI */
		Q.push( p->second );

	}

#if !TREE_CLK_NW
	vector<int> temp_arclist;
	int edges = 0;
	bool clk_nw_is_tree = true;
#endif

	while (!Q.empty())
	{
		/* t <- Q.dequeue() */
		node *t = Q.front();

		/* for all edges in G.adjacentEdges(t) loop */
		for (unsigned int i = 0; i < t->delays_forward.size(); ++i)
		{
			/* u <- G.adjancentVertex(t,e) */
			node *u = t->delays_forward[i]->connected_to;

			bool chain_reaction = false;

#if !TREE_CLK_NW
			/* Extract the clock network */
			if (t->clk_network == CLOCK_NODE || t->clk_network == CNW_NODE)  // the father node is either a clock pin or in the clock network
			{
				if (u->clk_network == NOT_A_CNW_NODE)  // the child node is examined for the first time
				{
					if (u->test_pin != NULL)  // special case, the clock reaches a test pin
					{
						u->clk_network = CNW_TEST_NODE;

						container *temp = new container;

						container_ptrs.push_back( temp );

#if CACHING
						temp->ff_clk_id = nof_ff_clocks++;
#endif

						temp->clk_nw_node = u;
						u->set_ptr.insert( temp );

						set<container *>::iterator k;
						for (k = t->set_ptr.begin(); k != t->set_ptr.end(); k++)  // add connections
						{
							temp_arclist.push_back( (*k)->id );
							temp_arclist.push_back( temp->id );
							edges++;

						}
						temp->prev.insert( t->set_ptr.begin(), t->set_ptr.end() );  // backward connections
					}
					else
					{
						u->clk_network = CNW_NODE;  // add it in the clock network

						if (u->delays_forward.size() > 1)// fanout > 1 include the child to the graph
						{
							container *temp = new container;

							container_ptrs.push_back( temp );

							temp->id = container_ptrs.size();

#if CACHING
							temp->ff_clk_id = -1;
#endif

							temp->clk_nw_node = u;
							u->set_ptr.insert( temp );

							set<container *>::iterator k;
							for (k = t->set_ptr.begin(); k != t->set_ptr.end(); k++)  // add connections
							{
								temp_arclist.push_back( (*k)->id );
								temp_arclist.push_back( temp->id );
								edges++;
							}
							temp->prev.insert( t->set_ptr.begin(), t->set_ptr.end() );  // backward connections

						}
						else if (u->delays_forward.size() == 1)  // fanout = 1, exclude the child from the graph
						{
							unsigned int old_size = u->set_ptr.size();

							u->set_ptr.insert( t->set_ptr.begin(), t->set_ptr.end() );  // merge sets

							if (old_size != u->set_ptr.size())
							{
								chain_reaction = true;
							}
						}
					}
				}
				else if (u->clk_network == FF_CLOCK_NODE)  // the child is a flip flop clock pin, include it to the graph despite the fanout
				{
					if (!u->set_ptr.empty())  // this child was processed before, the graph is not a tree!!
					{
						clk_nw_is_tree = false;

						set<container *>::iterator k;
						for (k = t->set_ptr.begin(); k != t->set_ptr.end(); k++)  // add connections
						{
							temp_arclist.push_back( (*k)->id );
							temp_arclist.push_back( (*(u->set_ptr.begin()))->id );
							edges++;
						}
						(*(u->set_ptr.begin()))->prev.insert( t->set_ptr.begin(), t->set_ptr.end() );  // backward connections
					}
					else  // first occurrence of the child
					{
						container *temp = new container;

						container_ptrs.push_back( temp );

						temp->id = container_ptrs.size();

#if CACHING
						temp->ff_clk_id = nof_ff_clocks++;
#endif

						temp->clk_nw_node = u;
						u->set_ptr.insert( temp );

						set<container *>::iterator k;
						for (k = t->set_ptr.begin(); k != t->set_ptr.end(); k++)  // add connections
						{
							temp_arclist.push_back( (*k)->id );
							temp_arclist.push_back( temp->id );
							edges++;

						}
						temp->prev.insert( t->set_ptr.begin(), t->set_ptr.end() );  // backward connections
					}

				}
				else if (u->clk_network == CNW_TEST_NODE)  // the child was processed before, the graph is not a tree!!
				{
					clk_nw_is_tree = false;

					set<container *>::iterator k;
					for (k = t->set_ptr.begin(); k != t->set_ptr.end(); k++)  // add connections
					{
						temp_arclist.push_back( (*k)->id );
						temp_arclist.push_back( (*(u->set_ptr.begin()))->id );
						edges++;
					}
					(*(u->set_ptr.begin()))->prev.insert( t->set_ptr.begin(), t->set_ptr.end() );  // backward connections
				}
				else  // the child was processed before, the graph is not a tree!!
				{
					clk_nw_is_tree = false;

					if (u->delays_forward.size() > 1)  // fanout > 1 include the child to the graph
					{
						set<container *>::iterator k;
						for (k = t->set_ptr.begin(); k != t->set_ptr.end(); k++)  // add connections
						{
							temp_arclist.push_back( (*k)->id );
							temp_arclist.push_back( (*(u->set_ptr.begin()))->id );
							edges++;
						}
						(*(u->set_ptr.begin()))->prev.insert( t->set_ptr.begin(), t->set_ptr.end() );  // backward connections

					}
					else if (u->delays_forward.size() == 1)  // fanout = 1, exclude the child from the graph
					{
						unsigned int old_size = u->set_ptr.size();

						u->set_ptr.insert( t->set_ptr.begin(), t->set_ptr.end() );  // merge sets

						if (old_size != u->set_ptr.size())
						{
							chain_reaction = true;
						}

					}
				}

			}
#endif

#if TREE_CLK_NW
			/* Extract the clock network */
			if (t->clk_network == CLOCK_NODE || t->clk_network == CNW_NODE)  // the father node is either a clock pin or in the clock network
			{
				if (u->clk_network == NOT_A_CNW_NODE)  // the child node is examined for the first time
				{
					if (u->test_pin != NULL)  // special case, the clock reaches a test pin
					{
						u->clk_network = CNW_TEST_NODE;

						container *temp = new container;

						container_ptrs.push_back( temp );
#if CACHING
						temp->ff_clk_id = nof_ff_clocks++;
#endif

						temp->clk_nw_node = u;
						u->ptr = temp;

						temp->prev = t->ptr;  // backward connection
					}
					else
					{
						u->clk_network = CNW_NODE;  // add it in the clock network

						if (u->delays_forward.size() > 1)  // fanout > 1 include the child to the graph
						{
							container *temp = new container;

							container_ptrs.push_back( temp );
#if CACHING
							temp->ff_clk_id = -1;
#endif

							temp->clk_nw_node = u;
							u->ptr = temp;

							temp->prev = t->ptr;  // backward connection
						}
						else if (u->delays_forward.size() == 1)  // fanout = 1, exclude the child from the graph
						{
							u->ptr = t->ptr;  // keep pointer to prev
						}
					}
				}
				else if (u->clk_network == FF_CLOCK_NODE)  // the child is a flip flop clock pin, include it to the graph despite the fanout
				{

					container *temp = new container;

					container_ptrs.push_back( temp );

#if CACHING
					temp->ff_clk_id = nof_ff_clocks++;
#endif

					temp->clk_nw_node = u;
					u->ptr = temp;

					temp->prev = t->ptr;  // backward connection
				}
			}
#endif

			/* update arrival times */

			/* Early */
			fp_type at_early = t->at_early + t->delays_forward[i]->delay_early;

			if (at_early < u->at_early)
			{
				u->at_early = at_early;
				chain_reaction = true;
			}

			/* Late */
			fp_type at_late = t->at_late + t->delays_forward[i]->delay_late;

			if (at_late > u->at_late)
			{
				u->at_late = at_late;
				chain_reaction = true;
			}

			/* if u has not been visited yet */
			if (!u->visited || chain_reaction)
			{
				/* u has been visited */
				u->visited = true;
				/* enqueue u onto Q */
				Q.push( u );

			}
		}

		/* Remove front element */
		Q.pop();
	}

	container_ptrs.shrink_to_fit();  //trim the extra space

#if !TREE_CLK_NW
	/* The clock network is not a tree, find the dominators of each node and transform the graph into a tree */
	if (!clk_nw_is_tree)
	{
		int *arclist = new int[2 * edges];
		for (unsigned int i = 0; i < temp_arclist.size(); i += 2)
		{
			arclist[i] = temp_arclist[i];
			arclist[i + 1] = temp_arclist[i + 1];
		}
		DominatorGraph g;

		temp_arclist.clear();

		g.buildGraph( container_ptrs.size(), edges, clock->clk_nw_root->id, arclist, false );

		int *idom = new int[g.getNVertices() + 1];

		g.snca( clock->clk_nw_root->id, idom );

		for (unsigned int i = 0; i < container_ptrs.size(); i++)  // delete old connections
		{
			container_ptrs[i]->prev.clear();
		}

		for (int i = 1; i <= g.getNVertices(); i++)  // recreate graph
		{
			if (i != idom[i])
			{
				container_ptrs[i - 1]->prev.insert( container_ptrs[idom[i] - 1] );  // backward connection
			}
		}

		delete[] idom;

		delete[] arclist;
	}
#endif

}

void cppr(node * test, int numPaths, multiset<pathInfo>& setup_paths, multiset<pathInfo>& hold_paths, int nof_ff_clocks)
{

	vector<node *> temp_path;

	vector<dfs> S;

	dfs initial_path;
	initial_path.route = 0;
	initial_path.launching_pin = NULL;

	if (test->test_pin->critical_hold)  // this test was critical on hold
	{
		initial_path.pre_slack_hold = -test->test_pin->rat_early;
	}

	if (test->test_pin->critical_setup)  // this test was critical on setup
	{
		initial_path.pre_slack_setup = test->test_pin->rat_late;
	}

	temp_path.push_back( test );
	S.push_back( initial_path );

#if DEBUGGING
	int max_path_length = 0;
	long num_paths = 0;
#endif

#if CACHING
	vector<credit_cache> cache( nof_ff_clocks );
#endif

	fp_type min_slack_post_hold = numeric_limits<fp_type>::max();
	fp_type min_slack_post_setup = numeric_limits<fp_type>::max();

	/* Run Depth First Search and discover all paths */
	while (!S.empty())
	{
		dfs temp2 = S.back();

		node * current = temp_path.back();

		if (temp2.route < current->delays_backward.size())  // There are more routes from this node, not yet explored
		{
			dfs temp3 = temp2;

			delay_to_node *ptr = current->delays_backward[temp2.route];
			node * next = ptr->connected_to;

			temp3.route = 0;  // always start exploring far left child

			if (next->clk_network == FF_CLOCK_NODE && temp3.launching_pin == NULL)  //we found a clock pin of a flip flop for the first time
			{
				temp3.launching_pin = next;
			}

			/* backward calculation of pre slack */
			if (test->test_pin->critical_hold)  // this test was critical on hold
			{
				temp3.pre_slack_hold += ptr->delay_early;
			}
			if (test->test_pin->critical_setup)  // this test was critical on setup
			{
				temp3.pre_slack_setup -= ptr->delay_late;
			}

#if BRANCH_AND_BOUND

			if (!test->test_pin->critical_setup)
			{

				if (temp3.pre_slack_hold >= 0)
				{
					if (!S.empty())
					{
						S.back().route++;
					}

					continue;
				}

			}
#endif

			S.push_back( temp3 );
			temp_path.push_back( next );
		}
		else if (current->delays_backward.empty())  // End of path, We reached a PI
		{
			if (test->test_pin->critical_hold)  // this test was critical on hold
			{
				temp2.pre_slack_hold += current->at_early;
			}
			if (test->test_pin->critical_setup)  // this test was critical on setup
			{
				temp2.pre_slack_setup -= current->at_late;
			}

#if DEBUGGING
			num_paths++;

			max_path_length = temp_path.size() > max_path_length ? temp_path.size() : max_path_length;
#endif

			/* Examine path */
			fp_type credit_setup = 0;
			fp_type credit_hold = 0;

			fp_type path_pre_slack_hold = 0;
			fp_type path_pre_slack_setup = 0;

			if (test->test_pin->critical_hold)  // this test was critical on hold
			{
				path_pre_slack_hold = temp2.pre_slack_hold;  // pre slack for hold
			}

			if (test->test_pin->critical_setup)  // this test was critical on setup
			{
				path_pre_slack_setup = temp2.pre_slack_setup;  // pre slack for setup
			}

			int p = temp_path.size() - 1;

			if (temp2.launching_pin != NULL)  // this path is driven from a ff
			{
#if CACHING

#if !TREE_CLK_NW
				int id = (*(temp2.launching_pin->set_ptr.begin()))->ff_clk_id;
#endif

#if TREE_CLK_NW
				int id = temp2.launching_pin->ptr->ff_clk_id;
#endif

				if (cache[id].credit_hold != -1)
				{
					credit_hold = cache[id].credit_hold;
					credit_setup = cache[id].credit_setup;
				}
				else
				{
#endif

#if !TREE_CLK_NW
					container * common = find_common_node( *(temp2.launching_pin->set_ptr.begin()), *(test->test_pin->clk->set_ptr.begin()) );

					int next = p;

					credit_hold = temp_path[p]->at_late - temp_path[p]->at_early;

					while (p > 0 && temp_path[p] != common->clk_nw_node)
					{
						next--;

						unsigned int i;

						for (i = 0; temp_path[p]->delays_forward[i]->connected_to != temp_path[next]; i++)
						;

						credit_setup += (temp_path[p]->delays_forward[i]->delay_late - temp_path[p]->delays_forward[i]->delay_early);

						p--;
					}

					credit_hold += credit_setup;
#endif

#if TREE_CLK_NW
					container * common = find_common_node( temp2.launching_pin->ptr, test->test_pin->clk->ptr );

					credit_hold = common->clk_nw_node->at_late - common->clk_nw_node->at_early;
					credit_setup = credit_hold - (temp_path[p]->at_late - temp_path[p]->at_early);
#endif

#if CACHING
					cache[id].credit_hold = credit_hold;
					cache[id].credit_setup = credit_setup;

				}
#endif
			}
			else if (test->clk_network == CNW_TEST_NODE && temp_path[p]->clk_network == CLOCK_NODE)  // special case, this path is not driven by a ff, but connects to clock network
			{
#if CACHING

#if !TREE_CLK_NW
				int id = (*(test->set_ptr.begin()))->ff_clk_id;
#endif

#if TREE_CLK_NW
				int id = test->ptr->ff_clk_id;
#endif

				if (cache[id].credit_hold != -1)
				{
					credit_hold = cache[id].credit_hold;
					credit_setup = cache[id].credit_setup;
				}
				else
				{
#endif

#if !TREE_CLK_NW
					container * common = find_common_node( *(test->set_ptr.begin()), *(test->test_pin->clk->set_ptr.begin()) );

					int next = p;

					credit_hold = temp_path[p]->at_late - temp_path[p]->at_early;

					while (p > 0 && temp_path[p] != common->clk_nw_node)
					{
						next--;

						unsigned int i;

						for (i = 0; temp_path[p]->delays_forward[i]->connected_to != temp_path[next]; i++)
						;

						credit_setup += (temp_path[p]->delays_forward[i]->delay_late - temp_path[p]->delays_forward[i]->delay_early);

						p--;
					}

					credit_hold += credit_setup;
#endif

#if TREE_CLK_NW
					container * common = find_common_node( test->ptr, test->test_pin->clk->ptr );

					credit_hold = common->clk_nw_node->at_late - common->clk_nw_node->at_early;
					credit_setup = credit_hold - (temp_path[p]->at_late - temp_path[p]->at_early);
#endif

#if CACHING
					cache[id].credit_hold = credit_hold;
					cache[id].credit_setup = credit_setup;

				}
#endif
			}

			if (test->test_pin->critical_hold)  // this test was critical on hold
			{

				fp_type path_post_slack_hold = path_pre_slack_hold + credit_hold;  // compute post hold slack

				min_slack_post_hold = path_post_slack_hold < min_slack_post_hold ? path_post_slack_hold : min_slack_post_hold;  // keep track of the min post hold slack

				if (path_pre_slack_hold < 0)  // this is a critical hold path
				{
					pathInfo temp3;

					temp3.path = temp_path;

					temp3.pre_slack = path_pre_slack_hold;

					temp3.post_slack = path_post_slack_hold;

					if (numPaths != ALL_PATHS && hold_paths.size() >= numPaths)
					{
						if (temp3.post_slack < (*(hold_paths.rbegin())).post_slack)
						{
							hold_paths.insert( temp3 );  // add this path to the hold set
							hold_paths.erase( *(hold_paths.rbegin()) );  // remove the less critical
						}
					}
					else
					{
						hold_paths.insert( temp3 );  // add this path to the hold set
					}

#if DEBUGGING
					cout << test->name << " has the following critical hold path\n";
					cout << "with pre cppr hold slack = " << temp3.pre_slack << "\n";
					cout << "with post cppr hold slack = " << temp3.post_slack << "\n";

					for (unsigned int l = 0; l < temp3.path.size(); l++)
					{
						cout << temp3.path[l]->name << "\n";
					}
					cout << "\n\n";
#endif
				}
			}

			if (test->test_pin->critical_setup)  // this test was critical on setup
			{

				fp_type path_post_slack_setup = path_pre_slack_setup + credit_setup;  // compute post setup slack

				min_slack_post_setup = path_post_slack_setup < min_slack_post_setup ? path_post_slack_setup : min_slack_post_setup;  // keep track of the min post setup slack

				if (path_pre_slack_setup < 0)  // this is a critical setup path
				{
					pathInfo temp4;

					temp4.path = temp_path;

					temp4.pre_slack = path_pre_slack_setup;

					temp4.post_slack = path_post_slack_setup;

					if (numPaths != ALL_PATHS && setup_paths.size() >= numPaths)
					{
						if (temp4.post_slack < (*(setup_paths.rbegin())).post_slack)
						{
							setup_paths.insert( temp4 );  // add this path to the setup set
							setup_paths.erase( *(setup_paths.rbegin()) );  // remove the less critical
						}
					}
					else
					{
						setup_paths.insert( temp4 );  // add this path to the setup set
					}

#if DEBUGGING
					cout << test->name << " has the following critical setup path\n";
					cout << "with pre cppr setup slack = " << temp4.pre_slack << "\n";
					cout << "with post cppr setup slack = " << temp4.post_slack << "\n";

					for (unsigned int l = 0; l < temp4.path.size(); l++)
					{
						cout << temp4.path[l]->name << "\n";
					}
					cout << "\n\n";
#endif
				}

			}

			temp_path.pop_back();
			S.pop_back();
			S.back().route++;
		}
		else  //This node is completely explored
		{
			temp_path.pop_back();
			S.pop_back();

			if (!S.empty())
			{
				S.back().route++;
			}
		}
	}

	if (test->test_pin->critical_hold)  // this test was critical on hold
	{
		test->test_pin->post_slack_hold = min_slack_post_hold;
	}

	if (test->test_pin->critical_setup)  // this test was critical on setup
	{
		test->test_pin->post_slack_setup = min_slack_post_setup;
	}

#if DEBUGGING
	cout << "Test: " << test->name << ", Num paths = " << num_paths << ", Max length = " << max_path_length << "\n";
#endif

}

#if TREE_CLK_NW
container * find_common_node(container * launching_pin, container * capturing_pin)
{
	set<container *> backward_launching_path;
	set<container *> backward_capturing_path;

	if (launching_pin == NULL || capturing_pin == NULL)
	{
#if DEBUGGING
		cout << "No common node found.\n\n";
#endif
		return NULL;
	}

#if DEBUGGING
	cout << "Searching the common node between " << launching_pin->clk_nw_node->name << " and " << capturing_pin->clk_nw_node->name << ".\n";
#endif

	if (launching_pin == capturing_pin)
	{
#if DEBUGGING
		cout << "The common node is " << launching_pin->clk_nw_node->name << ".\n\n";
#endif
		return launching_pin;
	}

	container * launching_temp = launching_pin;
	container * capturing_temp = capturing_pin;

	backward_launching_path.insert( launching_temp );  // add the node to the backward launch path
	backward_capturing_path.insert( capturing_temp );  // add the node to the backward capture path

	while (launching_temp->prev != NULL || capturing_temp->prev != NULL)  // While exist predecessors
	{

		if (launching_temp->prev != NULL)
		{

			launching_temp = launching_temp->prev;  // extend path from launch node backward

			if (backward_capturing_path.find( launching_temp ) != backward_capturing_path.end())  // is the node in the backward capture path?
			{
#if DEBUGGING
				cout << "The common node is " << launching_temp->clk_nw_node->name << ".\n\n";
#endif
				return launching_temp;
			}
			backward_launching_path.insert( launching_temp );  // add the node to the backward launch path
		}

		if (capturing_temp->prev != NULL)
		{

			capturing_temp = capturing_temp->prev;  // extend path from capture node backward

			if (backward_launching_path.find( capturing_temp ) != backward_launching_path.end())  // is the node in the backward launch path?
			{
#if DEBUGGING
				cout << "The common node is " << capturing_temp->clk_nw_node->name << ".\n\n";
#endif

				return capturing_temp;
			}
			backward_capturing_path.insert( capturing_temp );  // add the node to the backward capture path
		}

	}

#if DEBUGGING
	cout << "No common node found.\n\n";
#endif

	return NULL;

}
#endif

#if !TREE_CLK_NW
container * find_common_node(container * launching_pin, container * capturing_pin)
{
	set<container *> backward_launching_path;
	set<container *> backward_capturing_path;

	if (launching_pin == NULL || capturing_pin == NULL)
	{
#if DEBUGGING
		cout << "No common node found.\n\n";
#endif
		return NULL;
	}

#if DEBUGGING
	cout << "Searching the common node between " << launching_pin->clk_nw_node->name << " and " << capturing_pin->clk_nw_node->name << ".\n";
#endif

	if (launching_pin == capturing_pin)
	{
#if DEBUGGING
		cout << "The common node is " << launching_pin->clk_nw_node->name << ".\n\n";
#endif
		return launching_pin;
	}

	container * launching_temp = launching_pin;
	container * capturing_temp = capturing_pin;

	backward_launching_path.insert( launching_temp );  // add the node to the backward launch path
	backward_capturing_path.insert( capturing_temp );// add the node to the backward capture path

	while (!launching_temp->prev.empty() || !capturing_temp->prev.empty())// While exist predecessors
	{

		if (!launching_temp->prev.empty())
		{

			launching_temp = *(launching_temp->prev.begin());  // extend path from launch node backward

			if (backward_capturing_path.find( launching_temp ) != backward_capturing_path.end())// is the node in the backward capture path?
			{
#if DEBUGGING
				cout << "The common node is " << launching_temp->clk_nw_node->name << ".\n\n";
#endif
				return launching_temp;
			}
			backward_launching_path.insert( launching_temp );  // add the node to the backward launch path
		}

		if (!capturing_temp->prev.empty())
		{

			capturing_temp = *(capturing_temp->prev.begin());  // extend path from capture node backward

			if (backward_launching_path.find( capturing_temp ) != backward_launching_path.end())// is the node in the backward launch path?
			{
#if DEBUGGING
				cout << "The common node is " << capturing_temp->clk_nw_node->name << ".\n\n";
#endif

				return capturing_temp;
			}
			backward_capturing_path.insert( capturing_temp );  // add the node to the backward capture path
		}

	}

#if DEBUGGING
	cout << "No common node found.\n\n";
#endif

	return NULL;

}
#endif

void cppr_worker(vector<node*>& test_pins, clk *clock, TESTS_TYPE test, int numTests, int numPaths, multiset<testInfo> *hold_tests,
		multiset<testInfo>* setup_tests, int nof_ff_clocks)
{
	unsigned int local;

	do
	{
		{
			boost::mutex::scoped_lock lock( critical );

			local = counter;  // atomic get and increment of a global counter, each thread gets the next available job out of the job pool
#if PROGRESS_BAR
			if (local < test_pins.size())
			{
				DrawProgressBar( 50, (( double ) local) / (test_pins.size() + 1) );
			}
#endif
			counter++;

		}

		if (local >= test_pins.size())  // the job pool is empty
		{
			break;
		}

		/* Calculate required arrival time early/late for this test */
		test_pins[local]->test_pin->rat_late = clock->clk_T + test_pins[local]->test_pin->clk->at_early - test_pins[local]->test_pin->setup;
		test_pins[local]->test_pin->rat_early = test_pins[local]->test_pin->clk->at_late + test_pins[local]->test_pin->hold;

		/* Calculate pre slack setup/hold for this test */
		test_pins[local]->test_pin->pre_slack_hold = test_pins[local]->at_early - test_pins[local]->test_pin->rat_early;
		test_pins[local]->test_pin->pre_slack_setup = test_pins[local]->test_pin->rat_late - test_pins[local]->at_late;

		if ((test == HOLD_TEST || test == BOTH_TESTS) && test_pins[local]->test_pin->pre_slack_hold < 0)  // criticality test
		{
			test_pins[local]->test_pin->critical_hold = true;
		}

		if ((test == SETUP_TEST || test == BOTH_TESTS) && test_pins[local]->test_pin->pre_slack_setup < 0)  // criticality test
		{
			test_pins[local]->test_pin->critical_setup = true;
		}

		if (!test_pins[local]->test_pin->critical_hold && !test_pins[local]->test_pin->critical_setup)  // the test was not critical, skip it
		{
			continue;
		}

		multiset<pathInfo> setup_paths;
		multiset<pathInfo> hold_paths;

		cppr( test_pins[local], numPaths, setup_paths, hold_paths, nof_ff_clocks );  //run cppr for this test

		if (!hold_paths.empty())
		{
			boost::mutex::scoped_lock lock( critical2 );

			testInfo temp2;
			temp2.pre_slack = test_pins[local]->test_pin->pre_slack_hold;
			temp2.post_slack = test_pins[local]->test_pin->post_slack_hold;
			temp2.paths = hold_paths;

			if (numTests != ALL_TESTS && hold_tests->size() >= numTests)
			{
				if (temp2.post_slack < (*(hold_tests->rbegin())).post_slack)
				{
					hold_tests->insert( temp2 );  // add this test to the hold set
					hold_tests->erase( *(hold_tests->rbegin()) );  // remove the less critical
				}
			}
			else
			{
				hold_tests->insert( temp2 );  // add this test to the hold set
			}
		}

		if (!setup_paths.empty())
		{
			boost::mutex::scoped_lock lock( critical3 );

			testInfo temp;
			temp.pre_slack = test_pins[local]->test_pin->pre_slack_setup;
			temp.post_slack = test_pins[local]->test_pin->post_slack_setup;
			temp.paths = setup_paths;

			if (numTests != ALL_TESTS && setup_tests->size() >= numTests)
			{
				if (temp.post_slack < (*(setup_tests->rbegin())).post_slack)
				{
					setup_tests->insert( temp );  // add this test to the setup set
					setup_tests->erase( *(setup_tests->rbegin()) );  // remove the less critical
				}
			}
			else
			{
				setup_tests->insert( temp );  // add this test to the setup set
			}
		}

	} while (true);

}

