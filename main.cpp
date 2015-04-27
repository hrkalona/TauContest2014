/*
 * main.c
 *
 *  Created on: Nov 11, 2013
 *      Author: babis
 */

#include <sys/time.h>
#include <time.h>
#include <vector>
#include <cstdlib>
#include <iostream>

#include <string>

#include "parser.hpp"
#include "engine.hpp"
#include "io.hpp"
#include "auxiliary.hpp"

using namespace std;

int main(int argc, char* argv[])
{

#if BENCHMARKING
	struct timeval calc_start, calc_end;
#endif

	node_map PIs;  // The primary inputs of the circuit
	node_map POs;  // The primary outputs of the circuit
	node_map int_nodes;  // The internal nodes of the circuit
	node_map io_nodes;  // The I/O of the circuit

	int numTests = ALL_TESTS, numPaths = ALL_PATHS;
	TESTS_TYPE test = BOTH_TESTS;

	int nof_ff_clocks;

	/* all-Dff pins under test */
	vector<node*> test_pins;

	vector<container *> container_ptrs;

	/* Set containing  critical hold/setup tests */
	multiset<testInfo> setup_tests;
	multiset<testInfo> hold_tests;

	clk *clock = new clk;
	clock->clk_node = NULL;

	/* !!!Title!!! */
	cout << "\nCommon Path Pessimism Removal tool \n";
	cout << "\tUniversity of Thessaly Greece, Volos \n";
	cout << "\tDepartment of Electrical and Computer Engineering \n";
	cout << "\tElectronics Lab \n";

	cout << "\tTeam: The TimeKeepers \n";

	/* !!! This is the Input handler!!! */
	input_handler( argc, argv, test, numTests, numPaths );

#if PROGRESS_BAR
	cout << "\nProgress:\n";
	DrawProgressBar( 50, 0 );
#endif

#if BENCHMARKING
	gettimeofday( &calc_start, NULL );
#endif

	/* !!!!! PARSER !!!!! */
	input_parser( argv[1], argv[2], PIs, POs, int_nodes, io_nodes, test_pins, clock );

	/*  !!!!! Initialization !!!!! */
	init( clock, test_pins, container_ptrs, nof_ff_clocks );

	/* !!!!! STA ENGINE !!!!! */
	sta_engine( PIs, test_pins, clock, container_ptrs, nof_ff_clocks );

	/* !!!!! Threads !!!!! */
	thread_handler( test_pins, &hold_tests, &setup_tests, clock, test, numTests, numPaths, nof_ff_clocks );

	/* !!!for DEBUGGINGging reasons!!! */
#if GRAPHING
	plot_Graph( PIs, POs, int_nodes, clock, test_pins, container_ptrs);
#endif

	/* !!!This is the Output handler!!!! */
	output_handler( argv[3], hold_tests, setup_tests );

#if PROGRESS_BAR
	DrawProgressBar( 50, 1 );
#endif

#if BENCHMARKING
	gettimeofday( &calc_end, NULL );

	/* !!!for benchmarking!!! */
	cout << "\n\nDone in, " << (calc_end.tv_sec * 1000 + calc_end.tv_usec / 1000) - (calc_start.tv_sec * 1000 + calc_start.tv_usec / 1000)
			<< " ms.\n\n";
#endif

#if !BENCHMARKING
	cout << "\n";
#endif

	/* !!!Clean up the mess!!! */
	clear_mem( PIs, POs, int_nodes, io_nodes, clock, container_ptrs );

	return 0;
}

