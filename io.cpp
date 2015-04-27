/*
 * io.cpp
 *
 *  Created on: Jan 7, 2014
 *      Author: babis
 */

#include <fstream>
#include <iostream>
#include <cstdlib>

#include "types.hpp"

using namespace std;

void input_handler(int argc, char *argv[], TESTS_TYPE& test, int& numTests, int& numPaths)
{
	if (argc < 4)
	{
		cout << "Invalid argument count.\n";
		cout << "Input format:\n";
		cout << "./TKtimer <delay file> <timing input file> <output file> [-hold/-setup/-both] [-numPaths] [-numTests]\n";
		cout << "Terminating.\n";
		exit( -1 );
	}
	else if (argc > 4)
	{
		bool argv_test = false;
		bool argv_numtests = false;
		bool argv_numpaths = false;

		for (int i = 4; i < argc; i++)
		{
			string str = argv[i];
			if (str.compare( "-setup" ) == 0)
			{
				if (argv_test)
				{
					cout << "A test input declaration was found more than once.\n";
					cout << "Terminating.\n";
					exit( -1 );
				}

				test = SETUP_TEST;
				argv_test = true;
			}
			else if (str.compare( "-hold" ) == 0)
			{
				if (argv_test)
				{
					cout << "A test input declaration was found more than once.\n";
					cout << "Terminating.\n";
					exit( -1 );
				}

				test = HOLD_TEST;
				argv_test = true;
			}
			else if (str.compare( "-both" ) == 0)
			{
				if (argv_test)
				{
					cout << "A test input declaration was found more than once.\n";
					cout << "Terminating.\n";
					exit( -1 );
				}

				test = BOTH_TESTS;
				argv_test = true;
			}
			else if (str.compare( "-numTests" ) == 0)
			{
				if (argv_numtests)
				{
					cout << "A number of tests input declaration was found more than once.\n";
					cout << "Terminating.\n";
					exit( -1 );
				}

				i++;
				if (i >= argc)
				{
					cout << "Required numTests value.\n";
					cout << "Terminating.\n";
					exit( -1 );
				}

				numTests = atoi( argv[i] );
				if (numTests <= 0)
				{
					cout << "numTests needs to be greater than zero.\n";
					cout << "Terminating.\n";
					exit( -1 );
				}
				argv_numtests = true;
			}
			else if (str.compare( "-numPaths" ) == 0)
			{
				if (argv_numpaths)
				{
					cout << "A number of paths input declaration was found more than once.\n";
					cout << "Terminating.\n";
					exit( -1 );
				}

				i++;
				if (i >= argc)
				{
					cout << "Required numPaths value.\n";
					cout << "Terminating.\n";
					exit( -1 );
				}

				numPaths = atoi( argv[i] );
				if (numPaths <= 0)
				{
					cout << "numPaths needs to be greater than zero.\n";
					cout << "Terminating.\n";
					exit( -1 );
				}
				argv_numpaths = true;
			}
			else
			{
				cout << "Invalid argument format.\n";
				cout << "Input format:\n";
				cout << "./TKtimer <delay file> <timing input file> <output file> [-hold/-setup/-both] [-numPaths] [-numTests]\n";
				cout << "Terminating.\n";
				exit( -1 );
			}
		}
	}
}

void output_handler(char* output_file, multiset<testInfo>& hold_tests, multiset<testInfo>& setup_tests)
{
	ofstream out_file;

	out_file.open( output_file );

	ios_base::sync_with_stdio(false);

	out_file.precision( 5 );
	out_file << scientific;

	multiset<testInfo>::iterator hold;
	for (hold = hold_tests.begin(); hold != hold_tests.end(); hold++)
	{
		out_file << "hold " << (*hold).pre_slack << " " << (*hold).post_slack << " " << (*hold).paths.size() << "\n";
		multiset<pathInfo>::iterator path;

		for (path = (*hold).paths.begin(); path != (*hold).paths.end(); path++)
		{
			out_file << (*path).pre_slack << " " << (*path).post_slack << " " << (*path).path.size() << "\n";
			for (unsigned int li = 0; li < (*path).path.size(); li++)
			{
				out_file << (*path).path[li]->name << "\n";
			}
		}
	}

	multiset<testInfo>::iterator setup;
	for (setup = setup_tests.begin(); setup != setup_tests.end(); setup++)
	{
		out_file << "setup " << (*setup).pre_slack << " " << (*setup).post_slack << " " << (*setup).paths.size() << "\n";
		multiset<pathInfo>::iterator path;

		for (path = (*setup).paths.begin(); path != (*setup).paths.end(); path++)
		{
			out_file << (*path).pre_slack << " " << (*path).post_slack << " " << (*path).path.size() << "\n";
			for (unsigned int li = 0; li < (*path).path.size(); li++)
			{
				out_file << (*path).path[li]->name << "\n";
			}
		}
	}

	out_file.close();

}

