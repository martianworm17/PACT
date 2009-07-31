/* io.h
IO class definition
This object reads a BEAST or Migrate treefile and performs calculations on the resulting vector of
CoalescentTrees.

Currently, fills a vector with CoalescentTrees.  This is memory-intensive, but faster and more elegant 
than doing multiple readthroughs of the tree file.  CoalescentTrees vector takes about 10X the memory of
the corresponding tree file.
*/

#ifndef IO_H
#define IO_H

#include <string>
using std::string;

#include <vector>
using std::vector;

#include "coaltree.h"

class IO {

public:
	IO();									// constructor takes a input file
	
	void printHPTree();						// print highest posterior tree to .rules
	void printStatistics();					// print coalescent statistics to .stats
																			
private:

	string inputFile;						// complete name of input tree file
	string outputPrefix;					// prefix for output files .rules and .stats

	vector<CoalescentTree> treelist;		// vector of coalescent trees
	vector<double> problist;				// vector of assocatied probabilities

};

#endif