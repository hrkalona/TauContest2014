/*
 * parser.hpp
 *
 *  Created on: Nov 11, 2013
 *      Author: babis
 */

#ifndef PARSER_HPP_
#define PARSER_HPP_

#include "types.hpp"

using namespace std;

void input_parser(char* delay_file, char* timing_file, node_map& PIs, node_map& POs, node_map& int_nodes, node_map& io_nodes,
		vector<node*>& test_pins, clk *clock);

/*
 *  Functions...
 */

/* 
 * add_edge: Connects two adjacent nodes
 */
inline void add_edge(node *first_node, node *second_node, fp_type delay_early, fp_type delay_late);

/* 
 * add_setup_time: sets the setup time for the setup & hold constrain
 */
inline void add_setup_time(node *first_node, node *second_node, fp_type setup, node_map& PIs, node_map& POs, node_map& int_nodes,
		vector<node*>& test_pins);

/* 
 * add_hold_time: sets the hold time for the setup & hold constrain
 */
inline void add_hold_time(node *first_node, node *second_node, fp_type hold, node_map& PIs, node_map& POs, node_map& int_nodes,
		vector<node*>& test_pins);

/* 
 * add_arrival_time: sets the arrival time on a node (Early and Late)
 */
inline void add_arrival_time(node *first_node, fp_type at_early, fp_type at_late);

/* 
 * input_mapping: maps the string that follows the keyword "input" to the node vsource:string
 */
inline void input_mapping(string io_name, node_map& PIs, node_map& POs, node_map& int_nodes, node_map& io_nodes, vector<node*>& test_pins);

/* 
 * output_mapping: maps the string that follows the keyword "output" to the node vsink:string
 */
inline void output_mapping(string io_name, node_map& PIs, node_map& POs, node_map& int_nodes, node_map& io_nodes, vector<node*>& test_pins);

/*
 * node_mapping: maps the string node_name:pin_name to a node
 */
node *node_mapping(string name, node_map& PIs, node_map& POs, node_map& int_nodes, vector<node*>& test_pins);

#endif /* PARSER_HPP_ */
