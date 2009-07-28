/* coaltree.h
CoalescentTree class definition
This object stores and manipulates coalescent trees, rooted bifurcating trees with nodes mapped to time points
*/

#ifndef CTREE_H
#define CTREE_H

#include <map>
using std::map;
#include <set>
using std::set;
#include <vector>
using std::vector;
#include "tree.hh"

class CoalescentTree {

public:
	CoalescentTree(string);				// constructor, takes a parentheses string as input as well as an options string
										// can only be "migrate" or "beast"

	// TREE MANIPULATION
	void pushTimesBack(double);			// push dates to agree with a most recent sample date at t
	void pushTimesBack(double,double);	// oldest sample and most recent sample	
	void pruneToTrunk();				// reduces CoalescentTree object to trunk
	void pruneToLabel(int);				// reduces CoalescentTree object to only include a particular set of tips
	void trimEnds(double,double);		// reduces CoalescentTree object to only those nodes between
										// time start and time stop	
	void timeSlice(double);				// reduces CoalescentTree to all the ancestors of time slice
	void padTree();						// BROKEN
										// pads CoalescentTre with additional nodes at each coalescent event
										// included mainly for compatibility with TreePlot	

	// TREE STRUCTURE
	void printTree();					// print indented tree with coalescent times
	void printRuleList();				// print tree in Mathematica rule list format with times included
										// used with Graphics primitives
	void printParen();					// TODO: migration events
										// print parentheses tree										

	// BASIC STATISTICS
	double getPresentTime();			// returns most recent time in tree
	double getRootTime();				// returns most ancient time in tree
	double getTMRCA();					// span of time in tree
	int getMaxLabel();					// returns the highest label present
	int getLeafCount();					// returns the count of leaf nodes in tree


	// LABEL STATISTICS		
	double getLength();					// return total tree length
	double getLength(int);				// return length with this label
	vector<double> getLengths();	
	vector<double> getLabelPro();		// proportion of tree with each label
	double getTrunkPro();				// proportion of tree that can trace its history from present day samples
	
	// COALESCENT STATISTICS
	int getCoalCount();					// total count of coalescent events on tree
	int getCoalCount(int);				// count of coalescent events involving label on tree	
	double getCoalWeight();				// total opportunity for coalescence on tree
	double getCoalWeight(int);			// total opportunity for coalescence on tree
	double getCoalRate();
	double getCoalRate(int);
	vector<double> getCoalCounts();
	vector<double> getCoalWeights();	
	vector<double> getCoalRates();		

	// MIGRATION STATISTICS
	int getMigCount();
	int getMigCount(int,int);	
	double getMigRate();				// normalized by 'from' length
	double getMigRate(int,int);
	vector<double> getMigRates();
	
	// DIVERSITY STATISTICS
	double getDiversity();				// return mean of (2 * time to common ancestor) for every pair of leaf nodes
	double getTajimaD();				// return D = pi - S/a1, where pi is diversity, S is the total tree length, 
										// and a1 is a normalization factor

//	REVISE BELOW:
																			
	void tajimaSkyline();				// updates skyline with estimates of Tajima's D
									
		
private:
	tree<int> ctree;					// coalescent tree, has n leaf nodes (labelled 1..n) and n-1 internal nodes
										// root is labelled 0, other internal nodes continue with n+1
										// each node contains its label
	tree<Node> nodetree;						
										
	int leafCount;						// number of leafs in the tree (sample size n)
	int nodeCount;						// number of nodes in the tree
	set<int> leafSet;					// set of leaf nodes
	set<int> trunkSet;					// set of trunk nodes
	set<int> labelSet;					// set of distinct node labels
					
	map<int,double>	bmap;				// maps a node to the length of the branch leading into the node
	map<int,double>	rmap;				// maps a node to the rate of the branch leading into the node
	map<int,double>	tmap;				// maps a node to the age of the node
	map<int,int>	lmap;				// maps a node to a numeric label
	map<int,int>	ancmap;				// maps a node to its parent	
	set<double> tlist;					// sorted list of times starting with root
	
	double presentTime;					// most recent time in the tree
	double rootTime;					// most ancient time in the tree

	double stepsize;					// length of time between samples of parameter values										
	
	vector<double> skylineindex;
	vector<double> skylinevalue;
	

	
	tree<int> extractSubtree(set<int>);	// takes a set of node labels and walks up the coalescent tree
										// returning a tree object, tmap still works with this object											
	double getTreeLength(tree<int> &);	// takes a tree object and returns the total tree length, works with tmap
	

	// HELPER FUNCTIONS
	void reduce();								// goes through tree and removes inconsequential nodes	
	int getMaxNumber();							// return larger number in tree
	tree<Node>::iterator findNode(int);			// return iterator to a Node in nodetree based upon matching number
												// if not found, returns iterator to end of tree
												
	tree<Node>::iterator commonAncestor(tree<Node>::iterator,tree<Node>::iterator);
	
};

#endif