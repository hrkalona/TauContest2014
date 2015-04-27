#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <boost/tokenizer.hpp>
#include <string>

#include "parser.hpp"

unsigned int line_cnt = 1;

using namespace std;
using namespace boost;

/* for INPUT_TESTINGging reasons */
char *dbg_file;

void input_parser(char* delay_file, char* timing_file, node_map& PIs, node_map& POs, node_map& int_nodes, node_map& io_nodes,
		vector<node*>& test_pins, clk *clock)
{
	ifstream file;
	string buf;
	node *first_node, *second_node;
	fp_type delay_early, delay_late, setup, hold, at_early, at_late;

	file.open( delay_file );

	dbg_file = delay_file;

	if (!file.good())
	{
		cout << "\nThe file " << dbg_file << " was not found.\n";
		cout << "Terminating.\n";
		exit( -1 );
	}

	while (true)
	{
		getline( file, buf );

		if (file.eof())
			break;

		typedef tokenizer<char_separator<char> > tokenizer;

		char_separator<char> sep( " " );
		tokenizer tokens( buf, sep );

		tokenizer::iterator tok_iter = tokens.begin();

#if INPUT_TESTING
		if (tok_iter == tokens.end())
		{
			cout << "\nExpecting more inputs at line " << line_cnt << " of the file " << dbg_file << ".\n";
			cout << "Terminating.\n";
			exit( -1 );
		}
#endif

		if ((*tok_iter).compare( "input" ) == 0)
		{
			++tok_iter;

#if INPUT_TESTING
			if (tok_iter == tokens.end())
			{
				cout << "\nExpecting more inputs at line " << line_cnt << " of the file " << dbg_file << ".\n";
				cout << "Terminating.\n";
				exit( -1 );
			}
#endif

			input_mapping( *tok_iter, PIs, POs, int_nodes, io_nodes, test_pins );
		}
		else if ((*tok_iter).compare( "output" ) == 0)
		{
			++tok_iter;

#if INPUT_TESTING
			if (tok_iter == tokens.end())
			{
				cout << "\nExpecting more inputs at line " << line_cnt << " of the file " << dbg_file << ".\n";
				cout << "Terminating.\n";
				exit( -1 );
			}
#endif

			output_mapping( *tok_iter, PIs, POs, int_nodes, io_nodes, test_pins );
		}
		else if ((*tok_iter).compare( "setup" ) == 0)
		{
			++tok_iter;

#if INPUT_TESTING
			if (tok_iter == tokens.end())
			{
				cout << "\nExpecting <node_name>:<pin_name> at line " << line_cnt << " of the file " << dbg_file << ".\n";
				cout << "Terminating.\n";
				exit( -1 );
			}
#endif

			first_node = node_mapping( *tok_iter, PIs, POs, int_nodes, test_pins );

			++tok_iter;

#if INPUT_TESTING
			if (tok_iter == tokens.end())
			{
				cout << "\nExpecting <node_name2>:<pin_name> at line " << line_cnt << " of the file " << dbg_file << ".\n";
				cout << "Terminating.\n";
				exit( -1 );
			}
#endif

			second_node = node_mapping( *tok_iter, PIs, POs, int_nodes, test_pins );

			++tok_iter;

#if INPUT_TESTING
			if (tok_iter == tokens.end())
			{
				cout << "Expecting setup time at line " << line_cnt << " of the file " << dbg_file << ".\n";
				cout << "Terminating.\n";
				exit( -1 );
			}
#endif

			setup = atof( (*tok_iter).c_str() );

			add_setup_time( first_node, second_node, setup, PIs, POs, int_nodes, test_pins );
		}
		else if ((*tok_iter).compare( "hold" ) == 0)
		{
			++tok_iter;

#if INPUT_TESTING
			if (tok_iter == tokens.end())
			{
				cout << "\nExpecting <node_name>:<pin_name> at line " << line_cnt << " of the file " << dbg_file << ".\n";
				cout << "Terminating.\n";
				exit( -1 );
			}
#endif

			first_node = node_mapping( *tok_iter, PIs, POs, int_nodes, test_pins );
			++tok_iter;

#if INPUT_TESTING
			if (tok_iter == tokens.end())
			{
				cout << "Expecting <node_name2>:<pin_name> at line " << line_cnt << " of the file " << dbg_file << ".\n";
				cout << "Terminating.\n";
				exit( -1 );
			}
#endif

			second_node = node_mapping( *tok_iter, PIs, POs, int_nodes, test_pins );

			++tok_iter;

#if INPUT_TESTING
			if (tok_iter == tokens.end())
			{
				cout << "\nExpecting hold time at line " << line_cnt << " of the file " << dbg_file << ".\n";
				cout << "Terminating.\n";
				exit( -1 );
			}
#endif

			hold = atof( (*tok_iter).c_str() );

			add_hold_time( first_node, second_node, hold, PIs, POs, int_nodes, test_pins );
		}
		else
		{
			first_node = node_mapping( *tok_iter, PIs, POs, int_nodes, test_pins );

			++tok_iter;

#if INPUT_TESTING
			if (tok_iter == tokens.end())
			{
				cout << "\nExpecting <node_name2>:<pin_name> at line " << line_cnt << " of the file " << dbg_file << ".\n";
				cout << "Terminating.\n";
				exit( -1 );
			}
#endif

			second_node = node_mapping( *tok_iter, PIs, POs, int_nodes, test_pins );

			++tok_iter;

#if INPUT_TESTING
			if (tok_iter == tokens.end())
			{
				cout << "\nExpecting early delay at line " << line_cnt << " of the file " << dbg_file << ".\n";
				cout << "Terminating.\n";
				exit( -1 );
			}
#endif

			delay_early = atof( (*tok_iter).c_str() );

			++tok_iter;

#if INPUT_TESTING
			if (tok_iter == tokens.end())
			{
				cout << "\nExpecting late delay at line " << line_cnt << " of the file " << dbg_file << ".\n";
				cout << "Terminating.\n";
				exit( -1 );
			}
#endif

			delay_late = atof( (*tok_iter).c_str() );

			add_edge( first_node, second_node, delay_early, delay_late );
		}

		line_cnt++;
	}

	/* Close the first input file -- "delay file" */
	file.close();

	/* Open the second input file -- "timing file" */
	file.open( timing_file );

	dbg_file = timing_file;

	if (!file.good())
	{
		cout << "\nThe file " << dbg_file << " was not found.\n";
		cout << "Terminating.\n";
		exit( -1 );
	}

	line_cnt = 1;

	while (1)
	{
		getline( file, buf );

		if (file.eof())
			break;

		typedef tokenizer<char_separator<char> > tokenizer;
		char_separator<char> sep( " " );
		tokenizer tokens( buf, sep );

		tokenizer::iterator tok_iter = tokens.begin();

#if INPUT_TESTING
		if (tok_iter == tokens.end())
		{
			cout << "\nExpecting more inputs at line " << line_cnt << " of the file " << dbg_file << ".\n";
			cout << "Terminating.\n";
			exit( -1 );
		}
#endif

		if ((*tok_iter).compare( "clock" ) == 0)
		{
			++tok_iter;

#if INPUT_TESTING
			if (tok_iter == tokens.end())
			{
				cout << "\nExpecting <clock_name> at line " << line_cnt << " of the file " << dbg_file << ".\n";
				cout << "Terminating.\n";
				exit( -1 );
			}
#endif

			node_map::iterator find_it = io_nodes.find(*tok_iter);

			if (find_it != io_nodes.end())
			{
				clock->clk_node = find_it->second;

				++tok_iter;

#if INPUT_TESTING
				if (tok_iter == tokens.end())
				{
					cout << "\nExpecting clock period at line " << line_cnt << " of the file " << dbg_file << ".\n";
					cout << "Terminating.\n";
					exit( -1 );
				}
#endif

				clock->clk_node->clk_network = CLOCK_NODE;

				clock->clk_T = atof( (*tok_iter).c_str() );

			}
#if INPUT_TESTING
			else
			{
				cout << "\n" << *tok_iter << " was not found in the circuit at line " << line_cnt << " of the file " << dbg_file << ".\n";
				cout << "Terminating.\n";
				exit( -1 );
			}
#endif
		}
		else if ((*tok_iter).compare( "at" ) == 0)
		{
			++tok_iter;

#if INPUT_TESTING
			if (tok_iter == tokens.end())
			{
				cout << "\nExpecting <pin_name> at line " << line_cnt << " of the file " << dbg_file << ".\n";
				cout << "Terminating.\n";
				exit( -1 );
			}
#endif

			node_map::iterator find_it = io_nodes.find(*tok_iter);

			if (find_it != io_nodes.end())
			{
				first_node = find_it->second;

				++tok_iter;

#if INPUT_TESTING
				if (tok_iter == tokens.end())
				{
					cout << "\nExpecting arrival time early at line " << line_cnt << " of the file " << dbg_file << ".\n";
					cout << "Terminating.\n";
					exit( -1 );
				}
#endif

				at_early = atof( (*tok_iter).c_str() );

				++tok_iter;

#if INPUT_TESTING
				if (tok_iter == tokens.end())
				{
					cout << "\nExpecting arrival time late at line " << line_cnt << " of the file " << dbg_file << ".\n";
					cout << "Terminating.\n";
					exit( -1 );
				}
#endif

				at_late = atof( (*tok_iter).c_str() );

				add_arrival_time( first_node, at_early, at_late );
			}

#if INPUT_TESTING
			else
			{
				cout << "\n" << *tok_iter << " was not found in the circuit at line " << line_cnt << " of the file " << dbg_file << ".\n";
				cout << "Terminating.\n";
				exit( -1 );
			}
#endif
		}
#if INPUT_TESTING
		else
		{
			cout << "\nCannot parse line " << line_cnt << " of the file " << dbg_file << ".\n";
			cout << "Terminating.\n";
			exit( -1 );
		}
#endif

		line_cnt++;
	}

	file.close();

}

inline void add_edge(node *first_node, node *second_node, fp_type delay_early, fp_type delay_late)
{
	delay_to_node *temp, *temp2;

	temp = new delay_to_node;

	temp->delay_early = delay_early;
	temp->delay_late = delay_late;
	temp->connected_to = second_node;

	first_node->delays_forward.push_back( temp );

	temp2 = new delay_to_node;
	temp2->delay_early = delay_early;
	temp2->delay_late = delay_late;
	temp2->connected_to = first_node;

	second_node->delays_backward.push_back( temp2 );
}

inline void add_setup_time(node *first_node, node *second_node, fp_type setup, node_map& PIs, node_map& POs, node_map& int_nodes,
		vector<node*>& test_pins)
{
	if (first_node->test_pin == NULL)
	{
		first_node->test_pin = new setup_hold_constraint;

		first_node->test_pin->setup = -1;
		first_node->test_pin->hold = -1;

		first_node->test_pin->critical_setup = false;
		first_node->test_pin->critical_hold = false;

		first_node->test_pin->rat_early = 0;
		first_node->test_pin->rat_late = 0;

		first_node->test_pin->pre_slack_setup = 0;
		first_node->test_pin->pre_slack_hold = 0;

		first_node->test_pin->post_slack_setup = 0;
		first_node->test_pin->post_slack_hold = 0;

#if INPUT_TESTING
		typedef tokenizer<char_separator<char> > tokenizer;
		char_separator<char> sep( ":" );
		tokenizer tokens( first_node->name, sep );
		tokenizer tokens2( second_node->name, sep );

		tokenizer::iterator tok_iter = tokens.begin();
		tokenizer::iterator tok_iter2 = tokens2.begin();

		if ((*tok_iter).compare(*tok_iter2) != 0)
		{
			cout << "\nTest gates must be the same at " << line_cnt << " of the file " << dbg_file << ".\n";
			cout << "Terminating.\n";
			exit( -1 );
		}

		tok_iter2++;

		if((*tok_iter2).compare("CK") != 0)
		{
			cout << "\nThe second pin must be a test gate clock pin at " << line_cnt << " of the file " << dbg_file << ".\n";
			cout << "Terminating.\n";
			exit( -1 );
		}
#endif

		first_node->test_pin->clk = second_node;

		first_node->test_pin->clk->clk_network = FF_CLOCK_NODE;

		test_pins.push_back( first_node );
	}

	first_node->test_pin->setup = setup;
}

inline void add_hold_time(node *first_node, node *second_node, fp_type hold, node_map& PIs, node_map& POs, node_map& int_nodes,
		vector<node*>& test_pins)
{
	if (first_node->test_pin == NULL)
	{
		first_node->test_pin = new setup_hold_constraint;

		first_node->test_pin->setup = -1;
		first_node->test_pin->hold = -1;

		first_node->test_pin->critical_setup = false;
		first_node->test_pin->critical_hold = false;

		first_node->test_pin->rat_early = 0;
		first_node->test_pin->rat_late = 0;

		first_node->test_pin->pre_slack_setup = 0;
		first_node->test_pin->pre_slack_hold = 0;

		first_node->test_pin->post_slack_setup = 0;
		first_node->test_pin->post_slack_hold = 0;

#if INPUT_TESTING
		typedef tokenizer<char_separator<char> > tokenizer;
		char_separator<char> sep( ":" );
		tokenizer tokens( first_node->name, sep );
		tokenizer tokens2( second_node->name, sep );

		tokenizer::iterator tok_iter = tokens.begin();
		tokenizer::iterator tok_iter2 = tokens2.begin();

		if ((*tok_iter).compare(*tok_iter2) != 0)
		{
			cout << "\nTest gates must be the same at " << line_cnt << " of the file " << dbg_file << ".\n";
			cout << "Terminating.\n";
			exit( -1 );
		}

		tok_iter2++;

		if((*tok_iter2).compare("CK") != 0)
		{
			cout << "\nThe second pin must be a test gate clock pin at " << line_cnt << " of the file " << dbg_file << ".\n";
			cout << "Terminating.\n";
			exit( -1 );
		}
#endif

		first_node->test_pin->clk = second_node;

		first_node->test_pin->clk->clk_network = FF_CLOCK_NODE;

		test_pins.push_back( first_node );
	}

	first_node->test_pin->hold = hold;
}

inline void add_arrival_time(node *temp_node, fp_type at_early, fp_type at_late)
{
	temp_node->at_early = at_early;
	temp_node->at_late = at_late;
}

inline void input_mapping(string io_name, node_map& PIs, node_map& POs, node_map& int_nodes, node_map& io_nodes, vector<node*>& test_pins)
{
	io_nodes.insert( hash_entry( io_name, node_mapping( "vsource:" + io_name, PIs, POs, int_nodes, test_pins ) ) );
}

inline void output_mapping(string io_name, node_map& PIs, node_map& POs, node_map& int_nodes, node_map& io_nodes, vector<node*>& test_pins)
{
	io_nodes.insert( hash_entry( io_name, node_mapping( "vsink:" + io_name, PIs, POs, int_nodes, test_pins ) ) );
}

node *node_mapping(string name, node_map& PIs, node_map& POs, node_map& int_nodes, vector<node*>& test_pins)
{
	node * temp = NULL;
	string token1, token2;

	typedef tokenizer<char_separator<char> > tokenizer;
	char_separator<char> sep( ":" );
	tokenizer tokens( name, sep );

	tokenizer::iterator tok_iter = tokens.begin();

#if INPUT_TESTING
	if (tok_iter == tokens.end())
	{
		cout << "Searching for node failed at line " << line_cnt << " of the file " << dbg_file << ".\n";
		cout << "The name should be formatted as <node_name>:<pin_name>.\n";
		cout << "Terminating.\n";
		exit( -1 );
	}
#endif

	token1 = *tok_iter;

	if (token1.compare( "vsource" ) == 0)
	{
		node_map::iterator it = PIs.lower_bound( name );

		if (it != PIs.end() && (*it).first.compare( name ) == 0)
		{
			return (*it).second;
		}
		else
		{
			temp = new node;

			temp->test_pin = NULL;
			temp->at_early = numeric_limits<fp_type>::max();
			temp->at_late = 0;

			temp->clk_network = NOT_A_CNW_NODE;

			tokenizer tokens( name, sep );

			tokenizer::iterator tok_iter2 = tokens.begin();
			tok_iter2++;

#if INPUT_TESTING
			if (tok_iter2 == tokens.end())
			{
				cout << "\nSearching for node failed at line " << line_cnt << " of the file " << dbg_file << ".\n";
				cout << "The name should be formatted as <node_name>:<pin_name>.\n";
				cout << "Terminating.\n";
				exit( -1 );
			}
#endif

			temp->name = *tok_iter2;  //name

			/* Bfs algorithm -- specific */
			temp->visited = false;
			/*****************************/

			PIs.insert( it, hash_entry( name, temp ) );

			return temp;
		}

	}
	else if (token1.compare( "vsink" ) == 0)
	{

		node_map::iterator it = POs.lower_bound( name );

		if (it != POs.end() && (*it).first.compare( name ) == 0)
		{
			return (*it).second;
		}
		else
		{
			temp = new node;

			temp->test_pin = NULL;
			temp->at_early = numeric_limits<fp_type>::max();
			temp->at_late = 0;

			temp->clk_network = NOT_A_CNW_NODE;

			tokenizer tokens( name, sep );

			tokenizer::iterator tok_iter2 = tokens.begin();
			tok_iter2++;

#if INPUT_TESTING
			if (tok_iter2 == tokens.end())
			{
				cout << "\nSearching for node failed at line " << line_cnt << " of the file " << dbg_file << ".\n";
				cout << "The name should be formatted as <node_name>:<pin_name>.\n";
				cout << "Terminating.\n";
				exit( -1 );
			}
#endif

			temp->name = *tok_iter2;  //name

			/* Bfs algorithm -- specific */
			temp->visited = false;
			/*****************************/

			POs.insert( it, hash_entry( name, temp ) );

			return temp;

		}

	}

	++tok_iter;

#if INPUT_TESTING
	if (tok_iter == tokens.end())
	{
		cout << "\nSearching for node failed at line " << line_cnt << " of the file " << dbg_file << ".\n";
		cout << "The name should be formatted as <node_name>:<pin_name>.\n";
		cout << "Terminating.\n";
		exit( -1 );
	}
#endif

	token2 = *tok_iter;

	node_map::iterator it = int_nodes.lower_bound( name );

	if (it != int_nodes.end() && (*it).first.compare( name ) == 0)
	{
		return (*it).second;
	}
	else
	{
		temp = new node;

		temp->test_pin = NULL;
		temp->at_early = numeric_limits<fp_type>::max();
		temp->at_late = 0;

		temp->clk_network = NOT_A_CNW_NODE;

		temp->name = name;

		/* Bfs algorithm -- specific */
		temp->visited = false;
		/*****************************/

		int_nodes.insert( it, hash_entry( name, temp ) );

		return temp;
	}

}
