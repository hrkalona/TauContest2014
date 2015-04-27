/*
 * auxiliary.hpp
 *
 *  Created on: Jan 7, 2014
 *      Author: babis
 */

#ifndef AUXILIARY_HPP_
#define AUXILIARY_HPP_

#include <vector>

#include "types.hpp"

using namespace std;

/*
 * clear_mem: memory deallocation
 */
void clear_mem(node_map& PIs, node_map& POs, node_map& int_nodes, node_map& io_nodes, clk* clock, vector<container *>& container_ptrs);

void init(clk *clock, vector<node*>& test_pins, vector<container *>& container_ptrs, int& nof_ff_clocks);

void thread_handler(vector<node*>& test_pins, multiset<testInfo>* hold_tests, multiset<testInfo>* setup_tests, clk *clock, TESTS_TYPE test,
		int numTests, int numPaths, int nof_ff_clocks);

/* Graph routines */

void DrawProgressBar(int len, double percent);

void plot_Graph(node_map& PIs, node_map& POs, node_map& int_nodes, clk *clock, vector<node*>& test_pins, vector<container *>& container_ptrs);

#endif /* AUXILIARY_HPP_ */
