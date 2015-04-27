/*
 *  engine.hpp
 *
 *  Created on: Nov 11, 2013
 *      Author: babis
 */

#ifndef ENGINE_HPP_
#define ENGINE_HPP_

#include "types.hpp"
#include <stack>
#include <vector>

/* sta engine */
void sta_engine(node_map& PIs, vector<node*>& test_pins, clk *clock, vector<container *>& container_ptrs, int& nof_ff_clocks);

/* forward propagation of at */
void at_propagation(node_map& PIs, clk *clock, vector<container *>& container_ptrs, int& nof_ff_clocks);

container * find_common_node(container *launching_pin, container *capturing_pin);

void cppr(node * test, int numPaths, multiset<pathInfo>& setup_paths, multiset<pathInfo>& hold_paths, int nof_ff_clocks);

void cppr_worker(vector<node*>& test_pins, clk *clock, TESTS_TYPE test, int numTests, int numPaths, multiset<testInfo> *hold_tests,
		multiset<testInfo>* setup_tests, int nof_ff_clocks);

#endif /* ENGINE_HPP_ */
