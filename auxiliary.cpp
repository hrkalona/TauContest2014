/*
 * auxiliary.cpp
 *
 *  Created on: Jan 7, 2014
 *      Author: babis
 */

#include "auxiliary.hpp"
#include "types.hpp"
#include "engine.hpp"

#include <cstdio>
#include <iostream>

#include <fstream>

#include <boost/tokenizer.hpp>
#include <boost/thread/thread.hpp>

using namespace boost;

void clear_mem(node_map& PIs, node_map& POs, node_map& int_nodes, node_map& io_nodes, clk* clock, vector<container *>& container_ptrs)
{
	for (unsigned int i = 0; i < container_ptrs.size(); i++)
	{
		delete container_ptrs[i];
	}

	node_map::iterator p;

	for (p = PIs.begin(); p != PIs.end(); ++p)
	{
		for (unsigned int i = 0; i < p->second->delays_forward.size(); ++i)
			delete p->second->delays_forward[i];

		for (unsigned int i = 0; i < p->second->delays_backward.size(); ++i)
			delete p->second->delays_backward[i];

		delete p->second;
	}

	for (p = POs.begin(); p != POs.end(); ++p)
	{
		for (unsigned int i = 0; i < p->second->delays_forward.size(); ++i)
			delete p->second->delays_forward[i];

		for (unsigned int i = 0; i < p->second->delays_backward.size(); ++i)
			delete p->second->delays_backward[i];

		if (p->second->test_pin)
			delete p->second->test_pin;

		delete p->second;
	}

	for (p = int_nodes.begin(); p != int_nodes.end(); ++p)
	{
		for (unsigned int i = 0; i < p->second->delays_forward.size(); ++i)
			delete p->second->delays_forward[i];

		for (unsigned int i = 0; i < p->second->delays_backward.size(); ++i)
			delete p->second->delays_backward[i];

		delete p->second;
	}

	delete clock;
}

void init(clk *clock, vector<node*>& test_pins, vector<container *>& container_ptrs, int& nof_ff_clocks)
{

	nof_ff_clocks = 0;

	if (clock->clk_node == NULL)
	{
		cout << "\nClock not found in the circuit.\n";
		cout << "Terminating.\n";
		exit( -1 );
	}

	clock->clk_nw_root = new container;

#if CACHING
	clock->clk_nw_root->ff_clk_id = -1;
#endif
	clock->clk_nw_root->clk_nw_node = clock->clk_node;

#if !TREE_CLK_NW
	clock->clk_node->set_ptr.insert( clock->clk_nw_root );
	clock->clk_nw_root->id = 1;
#endif

#if TREE_CLK_NW
	clock->clk_node->ptr = clock->clk_nw_root;
	clock->clk_nw_root->prev = NULL;
#endif

	test_pins.shrink_to_fit();  //trim the extra allocated space

	container_ptrs.push_back( clock->clk_nw_root );

	for (unsigned int i = 0; i < test_pins.size(); i++)
	{
		if (test_pins[i]->test_pin->setup == -1 || test_pins[i]->test_pin->hold == -1)
		{
			cout << "Uninitialized test found, " << test_pins[i]->name << "\n";
			cout << "Terminating.\n";
			exit( -1 );
		}
	}

}

void thread_handler(vector<node*>& test_pins, multiset<testInfo>* hold_tests, multiset<testInfo>* setup_tests, clk *clock, TESTS_TYPE test,
		int numTests, int numPaths, int nof_ff_clocks)
{

	/* !!!Spawn threads!!! */

	thread_group threads;

	for (int thread_id = 0; thread_id < NUM_THREADS; thread_id++)
		threads.add_thread( new thread( &cppr_worker, test_pins, clock, test, numTests, numPaths, hold_tests, setup_tests, nof_ff_clocks ) );

	/* Master thead waits until all worker threads have finished their work */
	threads.join_all();

}

#if PROGRESS_BAR

void DrawProgressBar(int len, double percent)
{
	//cout << "\x1B[2K";  // Erase the entire current line.
	//cout << "\x1B[0E";  // Move to the beginning of the current line.
	cout << "\r";
	string progress;
	for (int i = 0; i < len; ++i)
	{
		if (i < static_cast<int>( len * percent ))
		{
			progress += "=";
		}
		else
		{
			progress += " ";
		}
	}
	cout << "[" << progress << "] " << (static_cast<int>( 100 * percent )) << "%";
	flush( cout );  // Required.
}

#endif

#if GRAPHING

void plot_Graph(node_map& PIs, node_map& POs, node_map& int_nodes, clk *clock, vector<node*>& test_pins, vector<container *>& container_ptrs)
{
	ofstream graph_file;

	node_map::iterator p;

	graph_file.open( "circuit" );
	graph_file << "digraph G {\nrankdir=LR;\nranksep=equally;\n";

	graph_file << "subgraph \"cluster_circuit\" {style=bold; color=gray; label=\"Cicuit\"; ";

	cout << "\nTest Pins:\n";

	for (unsigned int i = 0; i < test_pins.size(); i++)
	{
		graph_file << "node[shape=rect, color=orange, style=bold];\n";
		graph_file << "\"" << test_pins[i]->name << "\\n\\n" << "at_early: " << test_pins[i]->at_early << "\\n" << "at_late: "
				<< test_pins[i]->at_late << "\";\n";

		typedef tokenizer<char_separator<char> > tokenizer;
		char_separator<char> sep( ":" );
		tokenizer tokens( test_pins[i]->name, sep );

		tokenizer::iterator tok_iter = tokens.begin();

		graph_file << "node[shape=rect, color=blue, style=bold];\n";

		graph_file << "subgraph \"cluster_" << *tok_iter << "\" {style=bold; color=black; label=\"" << *tok_iter << "\"; ";

		graph_file << "\"" << test_pins[i]->name << "\\n\\n" << "at_early: " << test_pins[i]->at_early << "\\n" << "at_late: "
				<< test_pins[i]->at_late << "\"; ";

		graph_file << "}\n";

		cout << test_pins[i]->name << "\n";
	}

	cout << "\nPrimary Outputs:\n";

	for (p = POs.begin(); p != POs.end(); ++p)
	{

		graph_file << "node[shape=rect, color=red, style=bold];\n";
		graph_file << "\"" << p->second->name << "\\n\\n" << "at_early: " << p->second->at_early << "\\n" << "at_late: " << p->second->at_late
				<< "\";\n";

		typedef tokenizer<char_separator<char> > tokenizer;
		char_separator<char> sep( ":" );
		tokenizer tokens( p->first, sep );

		tokenizer::iterator tok_iter = tokens.begin();

		graph_file << "node[shape=rect, color=blue, style=bold];\n";

		graph_file << "subgraph \"cluster_" << *tok_iter << "\" {style=bold; color=black; label=\"" << *tok_iter << "\"; ";

		graph_file << "\"" << p->second->name << "\\n\\n" << "at_early: " << p->second->at_early << "\\n" << "at_late: " << p->second->at_late
				<< "\"; ";

		graph_file << "}\n";

		cout << p->second->name << "\n";
	}

	cout << "\nPrimary Inputs:\n";
	for (p = PIs.begin(); p != PIs.end(); ++p)
	{

		for (unsigned int i = 0; i < p->second->delays_forward.size(); i++)
		{

			graph_file << "node[shape=rect, color=green, style=bold];\n";

			graph_file << "\"" << p->second->name << "\\n\\n" << "at_early: " << p->second->at_early << "\\n" << "at_late: " << p->second->at_late
					<< "\";\n";

			graph_file << "node[shape=rect, color=blue, style=bold];\n";
			graph_file << "\"" << p->second->delays_forward[i]->connected_to->name << "\\n\\n" << "at_early: "
					<< p->second->delays_forward[i]->connected_to->at_early << "\\n" << "at_late: "
					<< p->second->delays_forward[i]->connected_to->at_late << "\";\n";

			if (p->second->clk_network > NOT_A_CNW_NODE && p->second->delays_forward[i]->connected_to->clk_network > NOT_A_CNW_NODE)
			{
				graph_file << "\"" << p->second->name << "\\n\\n" << "at_early: " << p->second->at_early << "\\n" << "at_late: " << p->second->at_late
						<< "\"" << " -> " << "\"" << p->second->delays_forward[i]->connected_to->name << "\\n\\n" << "at_early: "
						<< p->second->delays_forward[i]->connected_to->at_early << "\\n" << "at_late: "
						<< p->second->delays_forward[i]->connected_to->at_late << "\" [style=bold, color=magenta, label=\"delay_early: "
						<< p->second->delays_forward[i]->delay_early << "\\ndelay_late: " << p->second->delays_forward[i]->delay_late << "\"];\n";
			}
			else
			{
				graph_file << "\"" << p->second->name << "\\n\\n" << "at_early: " << p->second->at_early << "\\n" << "at_late: " << p->second->at_late
						<< "\"" << " -> " << "\"" << p->second->delays_forward[i]->connected_to->name << "\\n\\n" << "at_early: "
						<< p->second->delays_forward[i]->connected_to->at_early << "\\n" << "at_late: "
						<< p->second->delays_forward[i]->connected_to->at_late << "\" [style=bold, color=blue, label=\"delay_early: "
						<< p->second->delays_forward[i]->delay_early << "\\ndelay_late: " << p->second->delays_forward[i]->delay_late << "\"];\n";
			}

		}

		typedef tokenizer<char_separator<char> > tokenizer;
		char_separator<char> sep( ":" );
		tokenizer tokens( p->first, sep );

		tokenizer::iterator tok_iter = tokens.begin();

		graph_file << "node[shape=rect, color=blue, style=bold];\n";

		graph_file << "subgraph \"cluster_" << *tok_iter << "\" {style=bold; color=black; label=\"" << *tok_iter << "\"; ";

		graph_file << "\"" << p->second->name << "\\n\\n" << "at_early: " << p->second->at_early << "\\n" << "at_late: " << p->second->at_late
				<< "\"; ";

		graph_file << "}\n";

		cout << p->second->name << "\n";
	}

	cout << "\nInternal Nodes:\n";

	for (p = int_nodes.begin(); p != int_nodes.end(); ++p)
	{
		for (unsigned int i = 0; i < p->second->delays_forward.size(); i++)
		{
			graph_file << "node[shape=rect, color=blue, style=bold];\n";

			graph_file << "\"" << p->second->name << "\\n\\n" << "at_early: " << p->second->at_early << "\\n" << "at_late: " << p->second->at_late
					<< "\";\n";

			graph_file << "node[shape=rect, color=blue, style=bold];\n";

			graph_file << "\"" << p->second->delays_forward[i]->connected_to->name << "\\n\\n" << "at_early: "
					<< p->second->delays_forward[i]->connected_to->at_early << "\\n" << "at_late: "
					<< p->second->delays_forward[i]->connected_to->at_late << "\";\n";

			if (p->second->clk_network > NOT_A_CNW_NODE && p->second->delays_forward[i]->connected_to->clk_network > NOT_A_CNW_NODE)
			{
				graph_file << "\"" << p->second->name << "\\n\\n" << "at_early: " << p->second->at_early << "\\n" << "at_late: " << p->second->at_late
						<< "\"" << " -> " << "\"" << p->second->delays_forward[i]->connected_to->name << "\\n\\n" << "at_early: "
						<< p->second->delays_forward[i]->connected_to->at_early << "\\n" << "at_late: "
						<< p->second->delays_forward[i]->connected_to->at_late << "\" [style=bold, color=magenta, label=\"delay_early: "
						<< p->second->delays_forward[i]->delay_early << "\\ndelay_late: " << p->second->delays_forward[i]->delay_late << "\"];\n";
			}
			else
			{
				graph_file << "\"" << p->second->name << "\\n\\n" << "at_early: " << p->second->at_early << "\\n" << "at_late: " << p->second->at_late
						<< "\"" << " -> " << "\"" << p->second->delays_forward[i]->connected_to->name << "\\n\\n" << "at_early: "
						<< p->second->delays_forward[i]->connected_to->at_early << "\\n" << "at_late: "
						<< p->second->delays_forward[i]->connected_to->at_late << "\" [style=bold, color=blue, label=\"delay_early: "
						<< p->second->delays_forward[i]->delay_early << "\\ndelay_late: " << p->second->delays_forward[i]->delay_late << "\"];\n";
			}

		}

		typedef tokenizer<char_separator<char> > tokenizer;
		char_separator<char> sep( ":" );
		tokenizer tokens( p->first, sep );

		tokenizer::iterator tok_iter = tokens.begin();

		graph_file << "node[shape=rect, color=blue, style=bold];\n";

		graph_file << "subgraph \"cluster_" << *tok_iter << "\" {style=bold; color=black; label=\"" << *tok_iter << "\"; ";

		graph_file << "\"" << p->second->name << "\\n\\n" << "at_early: " << p->second->at_early << "\\n" << "at_late: " << p->second->at_late
				<< "\"; ";

		graph_file << "}\n";

		cout << p->second->name << "\n";

	}

	cout << "\n\n\n";

	graph_file << "}\n";

	graph_file << "}";

	graph_file.close();


	graph_file.open( "reduced_clock_graph" );
	graph_file << "digraph G {\nranksep=equally;\n";
	graph_file << "node[shape=rect, color=blue, style=bold];\n";

	for (unsigned int i = 0; i < container_ptrs.size(); i++)
	{
		container *t = container_ptrs[i];

#if TREE_CLK_NW
		container *u = t->prev;
		if (u != NULL)
		{
			graph_file << "\"" << t->clk_nw_node->name << "\"" << " -> " << "\"" << u->clk_nw_node->name << "\"" << " [style=bold, color=green];\n";
		}
#endif

#if !TREE_CLK_NW
		set<container *>::iterator it;
		for (it = t->prev.begin(); it != t->prev.end(); it++)
		{
			container *u = *it;
			graph_file << "\"" << t->clk_nw_node->name << "\"" << " -> " << "\"" << u->clk_nw_node->name << "\"" << " [style=bold, color=green];\n";
		}
#endif
	}

	graph_file << "}\n";

	graph_file.close();

	FILE *pipe = popen( "xdot circuit &", "w" );
	pipe = popen( "xdot reduced_clock_graph &", "w" );
	fclose( pipe );
}
#endif
