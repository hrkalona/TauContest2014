/*
 * io.hpp
 *
 *  Created on: Jan 7, 2014
 *      Author: babis
 */

#ifndef IO_HPP_
#define IO_HPP_

void input_handler( int argc, char *argv[], TESTS_TYPE& test, int& numTests, int& numPaths );
void output_handler( char* output_file, multiset<testInfo>& hold_tests, multiset<testInfo>& setup_tests );

#endif /* IO_HPP_ */
