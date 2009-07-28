/* coaltree.cpp
Member function definitions for CoalescentTree class
*/

#include "coaltree.h"
#include "tree.hh"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <cstdlib>
#include <map>
#include <set>
#include <vector>
#include <cmath>
#include <sstream>

#define INF pow(double(10),double(100)) // 10^100 (~infinity)

/* Constructor function to initialize private data */
/* Takes NEWICK parentheses tree as string input */
CoalescentTree::CoalescentTree(string paren) {

	stepsize = 0.1;	
	
	string::iterator is;
	tree<Node>:: iterator it, jt;

	// STRIP PAREN STRING ///////////
	// strip spaces from paren string
	// strip & and following character, replace following : with =
	// assumes migration events follow the format [&M 5 3:8.49916e-05]
	is = paren.begin() ;
	bool mig = false;
	while (is < paren.end()) {
		if (*is == ' ')
			is = paren.erase(is);
		else if (*is == '&') {
			is = paren.erase(is);
			is = paren.erase(is);
			mig = true;
		}
		else if (*is == ':' && mig) {
			*is = '=';
			mig = false;
		}
		else
			is++;
	}

	// GATHER TIPS ////////////////
	// read in node names, filling tips vector
	// names can only be 0-9 A-Z a-z
	// exported tree renames tips with consecutive numbering starting at 1
	// go through paren string and collect tips, at the same time replace names with matching numbers in paren string
	// if first character of name is a number and the second character is a letter, assume first character is label

	vector<Node> tipsList;
	int current = 1;
	int stringPos = 0;
	string thisString = "";
	
	while (stringPos < paren.length()) {
		
		char thisChar = paren[stringPos];
		
		if ( (thisChar >= 'A' && thisChar <= 'Z') || (thisChar >= 'a' && thisChar <= 'z') || (thisChar >= '0' && thisChar <= '9') ) {
			thisString += thisChar;
		} 	  	
				
		else if (thisChar == ':' && thisString.length() > 0) {
							
			/* nodetree update */	
			Node thisNode(current);
			thisNode.setName(thisString);
			
			// label is the first character of node string, incremented by 1
			// only attempt this if first character is number and second character is letter
			// otherwise set to 1
			if ( (thisString[0] >= '0' && thisString[0] <= '9') &&
					( (thisString[1] >= 'A' && thisString[1] <= 'Z') || (thisString[1] >= 'a' && thisString[1] <= '1') ) ) {
				thisNode.setLabel(atoi(thisString.substr(0,1).c_str()) + 1);
			}
			
			thisNode.setLeaf(true);
			
			tipsList.push_back(thisNode);

			/* replace name with number */	
			stringstream out;
			out << current;
			paren = paren.substr(0,stringPos - thisString.size()) + out.str() + paren.substr(stringPos,paren.length());
			
			/* move counter back */
			/* need to take into acount the length of the digits */
			stringPos -= thisString.size() - (out.str()).length() + 1;		
			
			thisString = "";
			current++;
		
		}
		
		else {
			thisString = "";
		}
		
		stringPos++;
		
	}
		
	// STARTING TREE /////////////////
	// construct starting point for tree (multifurcation from root)
	Node root(0);
	nodetree.set_head(root);
	for(int i = 0; i < tipsList.size(); i++) {
		nodetree.insert_after(nodetree.begin(), tipsList[i]);
   	}
  	
  	// CONSTRUCT TREE /////////////////////
	// read parentheses string from left to right, stop when a close parenthesis is encountered
	// push the left and right nodes onto their own branch
	// replace parenthesis string with their parent node ((1,2),3)  --->  (4,3)
	// fill bmap / update lengths of nodes
		
	// end when all parentheses have been eliminated
	while (paren.at(0) == '(') {
	
//		cout << paren << endl;
			
		int left, right, from, to, openParen, closeParen, openMig, closeMig;	
		double leftLength, rightLength, migLength;
		stringPos = 0;
		thisString = "";
					
		for ( is=paren.begin(); is < paren.end(); is++ ) {

			if (*is == '(') {
				openParen = stringPos;
				openMig = stringPos;
			}
			if ( (*is >= '0' && *is <= '9') || (*is >= 'A' && *is <= 'Z') || (*is >= 'a' && *is <= 'z') || *is == '.' || *is == '-' ) {
				thisString += *is;
			}	
			else {
			
				bool stringExists;
				if (thisString.length() > 0)
					stringExists = true;
				else
					stringExists = false;
				
				// branch length
				if (( *is == '[' || *is == ',' || *is == ')' ) && stringExists) {		
					leftLength = rightLength;
					rightLength = atof(thisString.c_str());
				}					
				
				// node number
				if (*is == ':' && stringExists) {	
					left = right;
					right = atoi(thisString.c_str());		
				}

				if (*is == ',') {
					openMig = stringPos;
				}		
				
				// MIGRATION EVENTS ////////////////////
				
				/* need to extend ctree here */
				/* can only deal with migration events that effect a tip node */
				/* this section is only called when brackets follow a tip node */
				if (*is == '=' && stringExists) {
				
					/* grabbing migration event */
					string labelString = thisString;
					labelString.erase(0,1);
					from = atoi(labelString.c_str()) + 1;
					labelString = thisString;
					labelString.erase(1,1);		
					to = atoi(labelString.c_str()) + 1;
					
				}				
				
				if (*is == ']') {
				
					closeMig = stringPos; 
				
					migLength = atof(thisString.c_str());
		//			cout << tempN << " mig from " << from << " to " << to << ", at " << migLength << endl;
					
					// push child node back by distance equal to migLength
					it = findNode(right);	
					(*it).setLength( rightLength - migLength );
		
					// create new intermediate node
					Node migNode(current);
					migNode.setLabel(to);
					migNode.setLength(migLength);
					
					// wrap this new node so that it inherits the old node
					nodetree.wrap(it,migNode);		
							
					/* replace parenthesis with new node label */	
					/* code is set up to deal with the situation of two labels before a parenthesis */
					stringstream out;
					out << current << ":" << migLength;
					paren = paren.substr(0,openMig+1) + out.str() + paren.substr(closeMig+1,paren.length());
					
					current++;			
					break;
				
				}
								
				thisString = "";
				
			}				
			
			// COALESCENT EVENTS //////////////////
			
			if (*is == ')') { 
						
				closeParen = stringPos; 
				
				// append a new node
				// append this new node with two branches (left node and right node)
								
				tree<Node>:: iterator iterLeft, iterRight, iterNew;
				iterLeft = findNode(left);
				iterRight = findNode(right);	

				(*iterLeft).setLength(leftLength);
				(*iterRight).setLength(rightLength);				
				
				Node newNode(current);
				newNode.setLabel( (*iterLeft).getLabel() );
			
				iterNew = nodetree.wrap(iterLeft,newNode);		
				nodetree.move_after(iterLeft,iterRight);
				
				/* replace parenthesis with new node label */		
				stringstream out;
				out << current;
				paren = paren.substr(0,openParen) + out.str() + paren.substr(closeParen+1,paren.length());
				
				current++;			
				break;
					
				
			}
	
		stringPos++;
			
		}
	
	}
	
	
	// removing null root and replacing node label of root with 0						
	nodetree.erase(findNode(0));
	(*nodetree.begin()).setNumber(0);
	
	// adding branch length to the parent node's time to get the node's time
	for (it = ++nodetree.begin(); it != nodetree.end(); it++) {
		jt = nodetree.parent(it);
		double t = (*jt).getTime() + (*it).getLength();
		(*it).setTime(t);
	}	
	  			
	/* go through tree and append to trunk set */
	/* only the last 1/100 of the time span is considered */
	presentTime = getPresentTime();
	double trunkTime = presentTime / (double) 100;
	it = nodetree.begin();
	(*it).setTrunk(true);
	while(it != nodetree.end()) {
		/* find nodes at present */
		if ((*it).getTime() > presentTime - trunkTime) {
			jt = it;
			/* move up tree adding nodes to trunk set */
			while ((*jt).getNumber() != 0) {
				(*jt).setTrunk(true);
				jt = nodetree.parent(jt);
			}
		}
		it++;
	}

}

/* push dates to agree with a most recent sample date at endTime */
void CoalescentTree::pushTimesBack(double endTime) {
		
	// need to adjust times by this amount
	double diff = endTime - getPresentTime();
		
	for (tree<Node>::iterator it = nodetree.begin(); it != nodetree.end(); it++) {
		double t = (*it).getTime();
		(*it).setTime(t + diff);
	}	
		  	
}

/* push dates to agree with a most recent sample date at endTime and oldest sample date is startTime */
void CoalescentTree::pushTimesBack(double startTime, double endTime) {
	
	tree<Node>::iterator it, jt;
	presentTime = getPresentTime();
	
	// STRETCH OR SHRINK //////////////	 
		 
	// find oldest sample	
	double oldestSample = presentTime;
	for (tree<Node>::leaf_iterator it = nodetree.begin_leaf(); it != nodetree.end_leaf(); it++) {
		if ((*it).getTime() < oldestSample) { 
			oldestSample = (*it).getTime(); 
		}
	}
	
	double mp = (endTime - startTime) / (presentTime - oldestSample);
	
	// go through tree and multiple lengths by mp	
	for (it = nodetree.begin(); it != nodetree.end(); it++) {
		double l = (*it).getLength();
		(*it).setLength(l * mp);
	}	
	
	// update times in tree
	for (it = ++nodetree.begin(); it != nodetree.end(); it++) {
		jt = nodetree.parent(it);
		double t = (*jt).getTime() + (*it).getLength();
		(*it).setTime(t);
	}	

	// PUSH BACK /////////////////////

	// need to adjust times by this amount
	double diff = endTime - getPresentTime();
		
	for (tree<Node>::iterator it = nodetree.begin(); it != nodetree.end(); it++) {
		double t = (*it).getTime();
		(*it).setTime(t + diff);
	}		
		 
}

/* reduces a tree to just its trunk, takes most recent sample and works backward from this */
void CoalescentTree::pruneToTrunk() {
	
	/* erase other nodes from the tree */
	tree<Node>::iterator it;	
	it = nodetree.begin();
	while(it != nodetree.end()) {
		if ( !(*it).getTrunk() ) {
			it = nodetree.erase(it);
		}
		else {
    		it++;
    	}
    }
            
	reduce();
			
}


/* reduces a tree to samples with a single label */
void CoalescentTree::pruneToLabel(int label) {

	/* start by finding all tips with this label */
	set<int> labelset; 
	tree<Node>::iterator it, jt;
	
	for (it = nodetree.begin(); it != nodetree.end(); it++) {
		if ( (*it).getLabel() == label && (*it).getLeaf() ) {
		
			/* move up tree adding nodes to label set */
			jt = it;
			while (nodetree.is_valid(jt)) {
				labelset.insert( (*jt).getNumber() );
				jt = ctree.parent(jt);
			}
		
		}
	}
			
	/* erase other nodes from the tree */
	it = nodetree.begin();
	while(it != nodetree.end()) {
		if (labelset.end() == labelset.find( (*it).getNumber() )) {
			it = nodetree.erase(it);
		}
		else {
    		it++;
    	}
    }
        
	/* update maps and other data accordingly */
	reduce();
				
}


/* trims a tree at its edges 

   		   |-------	 			 |-----
from  ------			 	to	--
   		   |----------		 	 |-----

*/
void CoalescentTree::trimEnds(double start, double stop) {
		
	/* erase nodes from the tree where neither the node nor its parent are between start and stop */
	tree<Node>::iterator it, root, jt;
	it = nodetree.begin();
	root = nodetree.begin();
	while(it != nodetree.end()) {	
	
		jt = nodetree.parent(it);
	
		/* if node > stop and parent < stop, erase children and prune node back to stop */
		/* this pruning causes a leaf node to become an internal node */
		if ((*it).getTime() > stop && (*jt).getTime() <= stop) {
		
			(*it).setTime( stop );
			(*it).setLength( (*it).getTime() - (*jt).getTime() );
			(*it).setLeaf(false);
			nodetree.erase_children(it);
			it = nodetree.begin();
		
		}
		
		/* if node > start and parent < start, push parent up to start */
		/* and reparent anc[node] to be a child of root */
		/* neither node nore anc[node] can be root */
		else if ((*it).getTime() > start && (*jt).getTime() < start && nodetree.depth(it) > 1) {	
			(*jt).setTime( start );
			(*jt).setLength( 0.0 );
			Node tempNode(-1);
			nodetree.move_ontop(nodetree.append_child(root,tempNode),jt);
			it = nodetree.begin();
		}
		else {
    		it++;
    	}
    }
    
    /* second pass for nodes < start */
    it = nodetree.begin();
    (*it).setTime(start);
    (*it).setLength(0.0);
	while(it != nodetree.end()) {	
		if ((*it).getTime() < start && nodetree.depth(it) > 0) {
			it = nodetree.erase(it);
		}
		else {
    		it++;
    	}
    }
               
	/* update maps and other data accordingly */
	reduce();
		
}

/* Reduces tree to just the ancestors of a single slice in time */
/* Used to calcuate diversity, TMRCA and Tajima's D at a particular time */
void CoalescentTree::timeSlice(double slice) {

	/* desire only nodes spanning the time slice */
	/* find these nodes and add them and their ancestors to a set */
	set<int> sliceset; 
	tree<Node>::iterator it, jt;
	it = nodetree.begin();
	while(it != nodetree.end()) {	
	
		jt = nodetree.parent(it);
	
		/* if node > slice and parent < slice, erase children and prune node back to stop */
		/* this pruning causes a leaf node to become an internal node */
		if ((*it).getTime() > slice && (*jt).getTime() <= slice) {
		
			// adjusting node
			(*it).setTime( slice );
			(*it).setLength( (*it).getTime() - (*jt).getTime() );
			(*it).setLeaf(true);
			nodetree.erase_children(it);
			
			// move up tree adding nodes to sliceset
			jt = it;
			while (nodetree.is_valid(jt)) {
				sliceset.insert( (*jt).getNumber() );
				jt = ctree.parent(jt);
			}
			
			it = nodetree.begin();
		
		}
		
		else {
    		it++;
    	}
    	
    }
    
	/* erase other nodes from the tree */
	it = nodetree.begin();
	while(it != nodetree.end()) {
		if (sliceset.end() == sliceset.find( (*it).getNumber() )) {
			it = nodetree.erase(it);
		}
		else {
    		it++;
    	}
    }    
    
    /* peel back trunk */
	for (it = ++nodetree.begin(); it != nodetree.end(); it++) {
		if (nodetree.number_of_children(it) == 1) {	
			jt = nodetree.child(it,0);
			(*jt).setLength( (*jt).getLength() + (*it).getLength() );	
			nodetree.reparent(nodetree.parent(it),it);				// push child node up to be sibling of node
			nodetree.erase(it);										// erase node									
			it = nodetree.begin();
		}
		else {
			break;
		}
	}
    
	nodetree.move_after(nodetree.begin(),++nodetree.begin());
	nodetree.erase(nodetree.begin());

	reduce();


}

/* padded with extra nodes at coalescent time points */
/* causing problems with migration tree */
void CoalescentTree::padTree() { 

	int current = getMaxNumber() + 1;

	tree<Node>::iterator it, end, iterTemp, iterN;
	
	/* construct set of coalescent times */
	set<double>::const_iterator is;
	set<double> tset;
	for (it = nodetree.begin(); it != nodetree.end(); it++) {
		tset.insert( (*it).getTime() );
	}
	
	/* pad tree with extra nodes, make sure there is a node at each time slice correspoding to coalescent event */
	it = nodetree.begin();
	while(it != nodetree.end()) {
	
		/* finding what the correct depth of the node should be */
		int newDepth = -1;
		for (is = tset.begin(); is != tset.find( (*it).getTime() ); is++) {
    		newDepth++;
    	}
    	
    	is++;
	
		if (newDepth > nodetree.depth(it)) {
		
			/* padding with number of nodes equal to the difference in depth levels */
			for(int i = 0; i < newDepth - nodetree.depth(it); i++) {

				Node newNode(current);
				newNode.setLabel( (*it).getLabel() );
				newNode.setTime( *is );
				newNode.setLength( *is - (*it).getTime() );
				
				nodetree.wrap(it,newNode);
	
				current++;
				it = nodetree.begin();
				
			}
	
		}
		
		it++;
	
	}
			  	
}

/* Print indented tree */
void CoalescentTree::printTree() { 

	int rootdepth = nodetree.depth(nodetree.begin());
		
	for (tree<Node>::iterator it = nodetree.begin(); it != nodetree.end(); it++) {
		for(int i=0; i<nodetree.depth(it)-rootdepth; ++i) 
			cout << "  ";
		cout << (*it).getNumber();
		if ((*it).getName() != "") { 
			cout << " " << (*it).getName();
		}
		cout << " (" << (*it).getTime() << ")";
		cout << " [" << (*it).getLabel() << "]";			
		cout << " {" << (*it).getLength() << "}";		
		cout << endl << flush;
	}
		
}

/* print tree in Mathematica suitable format
Output is:
	leaf list
	trunk list
	tree rules
	label rules
	coordinate rules 
*/	
void CoalescentTree::printRuleList() { 

	tree<Node>::iterator it, jt;

	/* reorder tree so that the bottom node of two sister nodes always has the most recent child more children */
	/* this combined with preorder traversal will insure the trunk follows a rough diagonal */
	it = nodetree.begin();
	while(it != nodetree.end()) {
		jt = nodetree.next_sibling(it);
		if (nodetree.is_valid(jt)) {
			int cit = nodetree.size(it);
			int cjt = nodetree.size(jt);
			if (cit > cjt) {
				nodetree.swap(jt,it);
				it = nodetree.begin();
			}
		}
		it++;
	}
	
	/* print leaf nodes */
	/* a node may be a leaf on the current tree, but not a leaf on the original tree */
	for (it = nodetree.begin(); it != nodetree.end(); it++) {
		if ((*it).getLeaf()) {
			cout << (*it).getNumber() << " ";
		}
	}
	cout << endl;	
	
	/* print trunk nodes */
	for (it = nodetree.begin(); it != nodetree.end(); it++) {
		if ((*it).getTrunk()) {
			cout << (*it).getNumber() << " ";
		}
	}
	cout << endl;	
			
	/* print the tree in rule list (Mathematica-ready) format */
	/* print only upward links */
	for (it = ++nodetree.begin(); it != nodetree.end(); it++) {		// increment past root
		jt = nodetree.parent(it);
		cout << (*it).getNumber() << "->" << (*jt).getNumber() << " ";
	}
	cout << endl;
	
	
	/* print mapping of labels in Mathematica format */
	for (it = nodetree.begin(); it != nodetree.end(); it++) {	
		cout << (*it).getNumber() << "->" << (*it).getLabel() << " ";
	}
	cout << endl;
	
	/* print mapping of nodes to coordinates */
  	int count = 0;
	for (it = nodetree.begin(); it != nodetree.end(); it++) {
		cout << (*it).getNumber() << "->{" << (*it).getTime() << "," << count << "} ";	
		count++;
	}
	cout << endl;	
	  	  	
}

/* Print parentheses tree */
void CoalescentTree::printParen() { 

	tree<Node>::post_order_iterator it;
	it = nodetree.begin_post();
   	
	int currentDepth = nodetree.depth(it);
	for (int i = 0; i < currentDepth; i++) { 
		cout << "("; 
	} 
	cout << (*it).getNumber() << ":" << (*it).getLength(); 
	it++;
	
	/* need to add a '(' whenever the depth increases and a ')' whenever the depth decreases */
	/* only print leaf nodes */
	while(it != nodetree.end_post()) {
		if (nodetree.depth(it) > currentDepth) { 
			cout << ", ("; 
			for (int i = 0; i < nodetree.depth(it) - currentDepth - 1; i++) { 
				cout << "("; 
			}
			if (nodetree.number_of_children(it) == 0) { 
				cout << (*it).getNumber() << ":" << (*it).getLength(); 
			}
		}
		if (nodetree.depth(it) == currentDepth) { 
			if (nodetree.number_of_children(it) == 0) { 
				cout << ", " << (*it).getNumber() << ":" << (*it).getLength(); ; 
			}
		}
		if (nodetree.depth(it) < currentDepth) {
			if (nodetree.number_of_children(it) == 0) { 
				cout << (*it).getNumber() << ":" << (*it).getLength(); 
				cout << ")";		
			}
			else {
				cout << ")";	
				cout << ":" << (*it).getLength();
			}
		}
		currentDepth = nodetree.depth(it);
		it++;
		
	}
	
	cout << endl;

	
}


/* most recent node in tree, will always be a leaf */
double CoalescentTree::getPresentTime() {
	
	double t = 0;
	for (tree<Node>::leaf_iterator it = nodetree.begin_leaf(); it != nodetree.end_leaf(); it++) {
		if ((*it).getTime() > t) { 
			t = (*it).getTime(); 
		}
	}
	return t;

}

/* most ancient node in tree */
double CoalescentTree::getRootTime() {
	return (*nodetree.begin()).getTime();
}

/* amount of time it takes for all samples to coalesce */
double CoalescentTree::getTMRCA() {
	return getPresentTime() - getRootTime();
}

/* number of labels 1 to n */
int CoalescentTree::getMaxLabel() {

	double n = 0;
	for (tree<Node>::iterator it = nodetree.begin(); it != nodetree.end(); it++ ) {
		if ( (*it).getLabel() > n ) {
			n = (*it).getLabel();
		}
	}	
	return n;

}

/* total length of the tree */
double CoalescentTree::getLength() {

	double length = 0.0;
	for (tree<Node>::iterator it = nodetree.begin(); it != nodetree.end(); it++ ) {
		length += (*it).getLength();
	}	
	return length;

}

/* length of the tree with label l */
double CoalescentTree::getLength(int l) {

	double length = 0.0;
	for (tree<Node>::iterator it = nodetree.begin(); it != nodetree.end(); it++ ) {
		if ( (*it).getLabel() == l ) {
			length += (*it).getLength();
		}
	}	
	return length;

}

/* get lenght of each label */
vector<double> CoalescentTree::getLengths() { 

	vector<double> lengths;
	for (int i = 1; i <= getMaxLabel(); i++) {
		lengths.push_back( getLength(i) );
	}
	return lengths;
	
}

/* get proportion of tree with each label */
vector<double> CoalescentTree::getLabelPro() { 

	vector<double> labelPro;
	
	double totalLength = getLength();

	for (int i = 1; i <= getMaxLabel(); i++) {
		labelPro.push_back( getLength(i) / totalLength );
	}
	
	return labelPro;
	
}

/* proportion of tree that can trace its history forward to present day samples */
/* trunk traced back from the last 1/100 of the time width */
double CoalescentTree::getTrunkPro() { 

	double totalLength = getLength();

	double trunkLength = 0.0;
	for (tree<Node>::iterator it = nodetree.begin(); it != nodetree.end(); it++ ) {
		if ( (*it).getTrunk()) {
			trunkLength += (*it).getLength();
		}
	}	
	
	return trunkLength / totalLength;
    	
}

/* returns the count of coalescent events */
int CoalescentTree::getCoalCount() {

	/* count coalescent events, these are nodes with two children */
	int count = 0;
	for (tree<Node>::iterator it = nodetree.begin(); it != nodetree.end(); it++) {
		if (nodetree.number_of_children(it) == 2) {		
			count++;
		}
	}
	return count;

}

/* returns the count of coalescent events with label */
int CoalescentTree::getCoalCount(int l) {

	/* count coalescent events, these are nodes with two children */
	int count = 0;
	for (tree<Node>::iterator it = nodetree.begin(); it != nodetree.end(); it++) {
		if (nodetree.number_of_children(it) == 2 && (*it).getLabel() == l ) {		
			count++;
		}
	}
	return count;

}

/* returns the opportunity for coalescence over the whole tree */
/* running this will padTree() may be faster and more accurate */
double CoalescentTree::getCoalWeight() {

	// setting step to be 1/1000 of the total length of the tree
	double start = getRootTime();
	double stop = getPresentTime();
	double step = (stop - start) / (double) 1000;
	
	// step through tree counting concurrent lineages
	double weight = 0.0;
	for (double t = start; t <= stop; t += step) {
	
		int lineages = 0;
		tree<Node>::iterator it, jt;
		for (it = ++nodetree.begin(); it != nodetree.end(); it++) {
			jt = nodetree.parent(it);
			if ( (*it).getTime() >= t && (*jt).getTime() < t) {		
				lineages++;
			}
		}
		
		if (lineages > 0) {
			weight += ( ( lineages * (lineages - 1) ) / 2 ) * step;
		}
		
	}	
	
	return weight;

}

/* returns the opportunity for coalescence for label */
double CoalescentTree::getCoalWeight(int l) {

	// setting step to be 1/1000 of the total length of the tree
	double start = getRootTime();
	double stop = getPresentTime();
	double step = (stop - start) / (double) 1000;
	
	// step through tree counting concurrent lineages
	double weight = 0.0;
	for (double t = start; t <= stop; t += step) {
	
		int lineages = 0;
		tree<Node>::iterator it, jt;
		for (it = ++nodetree.begin(); it != nodetree.end(); it++) {
			jt = nodetree.parent(it);
			if ( (*it).getTime() >= t && (*jt).getTime() < t && (*it).getLabel() == l ) {		
				lineages++;
			}
		}
		
		if (lineages > 0) {
			weight += ( ( lineages * (lineages - 1) ) / 2 ) * step;
		}
		
	}	
	
	return weight;

}

double CoalescentTree::getCoalRate() {
	return getCoalCount() / getCoalWeight();
}

double CoalescentTree::getCoalRate(int l) {
	return getCoalCount(l) / getCoalWeight(l);
}

/* get count of coalescent involving each label */
vector<double> CoalescentTree::getCoalCounts() { 

	vector<double> counts;
	for (int i = 1; i <= getMaxLabel(); i++) {
		counts.push_back( getCoalCount(i) );
	}
	return counts;
	
}

/* get coalescent opportunity involving each label */
vector<double> CoalescentTree::getCoalWeights() { 

	vector<double> weights;
	for (int i = 1; i <= getMaxLabel(); i++) {
		weights.push_back( getCoalWeight(i) );
	}
	return weights;
	
}

/* get coalescent rate involving each label */
vector<double> CoalescentTree::getCoalRates() { 

	vector<double> rates;
	for (int i = 1; i <= getMaxLabel(); i++) {
		rates.push_back( getCoalCount(i) / getCoalWeight(i) );
	}
	return rates;
	
}

/* returns the count of migration events over entire tree */
int CoalescentTree::getMigCount() {

	/* count migration events, these are nodes in which the parent label differs from child label */
	tree<Node>::iterator it, jt;
	int count = 0;
	for (it = ++nodetree.begin(); it != nodetree.end(); it++) {
		jt = nodetree.parent(it);
		if ( (*it).getLabel() != (*jt).getLabel() ) {		
			count++;
		}
	}
	return count;

}

/* returns the count of migration events from label to label */
int CoalescentTree::getMigCount(int from, int to) {

	/* count migration events, these are nodes in which the parent label differs from child label */
	tree<Node>::iterator it, jt;
	int count = 0;
	for (it = ++nodetree.begin(); it != nodetree.end(); it++) {
		jt = nodetree.parent(it);
		if ( (*it).getLabel() == to && (*jt).getLabel() == from ) {		
			count++;
		}
	}
	return count;

}

/* returns the overall rate of migration */
double CoalescentTree::getMigRate() {
	return getMigCount() / getLength();
}

/* returns the rate of migration from label to label */
double CoalescentTree::getMigRate(int from, int to) {
	return getMigCount(from,to) / getLength(from);
}

/* get migration rate involving each label */
vector<double> CoalescentTree::getMigRates() { 

	vector<double> rates;
	for (int to = 1; to <= getMaxLabel(); to++) {	
		for (int from = 1; from <= getMaxLabel(); from++) {	
			if (to != from) {
		
				rates.push_back( getMigCount(from,to) / getLength(from) );

			}
		}
	}
		
	return rates;
	
}

/* Bayesian skyline for effective coalesent timescale Ne*tau */
void CoalescentTree::NeSkyline() { 

	vector<int> lineages (tlist.size());
	tree<int>::iterator it, end;
	it = ctree.begin();
	end = ctree.end(); 		
	int rootdepth=ctree.depth(it);
	
	while(it!=end) {
		lineages.at(ctree.depth(it)-rootdepth)++;
		it++;
	}
	
	int loc = ctree.max_depth();
	double ne;
	int count;
	double step = stepsize;
  	for( set<double>::const_iterator iter = tlist.begin(); iter != --tlist.end(); iter++ ) {
  		double a = *iter;
  		iter++;
  		double b = *iter;
  		iter--;
  		if (a < b - 0.00000001) {
  			if (lineages.at(loc-1) < lineages.at(loc))
				ne = (b - a) * 0.5 * lineages.at(loc) * (lineages.at(loc) - 1);
	//		cout << "{" << a << "," << b << "} " << lineages.at(loc) << " " << ne << endl;
			while (step > a && step < b) {
	//			cout << step << "\t" << ne << endl;
				skylineindex.push_back(step);
				skylinevalue.push_back(ne);
				step += stepsize;
			}
	//		cout << step << "\t" << ne << endl;
			skylineindex.push_back(step);
			skylinevalue.push_back(ne);
			step += stepsize;
		}
		loc--;
    }
    
}

/* Skyline for rate of substitution, take mean rate for concurrent lineages */
void CoalescentTree::subRateSkyline() { 

	tree<int>::iterator it, jt, end;
	vector<int> lineages (tlist.size());
	vector<double> rates (tlist.size());	
	it = ctree.end();
	end = ctree.end(); 		
	int rootdepth=ctree.depth(it);
	
	/* lineages contains a running tally of mean rate at this depth */
	while(it!=ctree.begin()) {
		lineages.at(ctree.depth(it)-rootdepth)++;
		rates.at(ctree.depth(it)-rootdepth) += rmap[*it];
//		cout << *it << " " << rmap[*it] << endl;
		it--;
	}
	
	int loc = ctree.max_depth();
	double rate;
	int count;
	double step = stepsize;
  	for( set<double>::const_iterator iter = tlist.begin(); iter != --tlist.end(); iter++ ) {
  		double a = *iter;
  		iter++;
  		double b = *iter;
  		iter--;
  		if (a < b - 0.00000001) {
			rate = rates.at(loc) / lineages.at(loc);
//			cout << "{" << a << "," << b << "} " << lineages.at(loc) << " " << rate << endl;
			while (step > a && step < b) {
//				cout << step << "\t" << rate << endl;
				skylineindex.push_back(step);
				skylinevalue.push_back(rate);
				step += stepsize;
			}
//			cout << step << "\t" << rate << endl;
			skylineindex.push_back(step);
			skylinevalue.push_back(rate);
			step += stepsize;
		}
		loc--;
    }
    
}


/* Skyline for diversity (pi) */
/* At every event, take concurrent lineages and make all pairwise comparisons */
/* calculating total distance separating samples, E[div] = 2 Ne*tau */
void CoalescentTree::divSkyline() { 

	/* have a vector[time points] of sets[node labels] */
	tree<int>::iterator it, jt, end;
	vector< set<int> > lineages (tlist.size());
	it = ctree.end();
	end = ctree.end(); 		
	int rootdepth=ctree.depth(it);
	
	/* go through tree and add to set */
	while(it!=ctree.begin()) {
		(lineages.at(ctree.depth(it)-rootdepth)).insert(*it);
		it--;
	}
	
	int loc = ctree.max_depth();
	double rate;
	int count;
	double step = stepsize;
  	for( set<double>::const_iterator iter = tlist.begin(); iter != --tlist.end(); iter++ ) {
  		set<int> nodes = lineages.at(loc);
  		double a = *iter;
  		iter++;
  		double b = *iter;
  		iter--;
  		if (a < b - 0.00000001) {
//			cout << "{" << a << "," << b << "} ";
			int divn = 0;
			double div = 0.0;
			for( set<int>::const_iterator jter = nodes.begin(); jter != --nodes.end(); jter++ ) {
				for( set<int>::const_iterator kter = jter; kter != nodes.end(); kter++ ) {
					if (*jter < *kter) { 
						set<int> s;
						s.insert(*jter);
						s.insert(*kter);
						tree<int> st = extractSubtree(s);
						divn++;
						div += getTreeLength(st);
					}
				}
			}
			div /= divn;
//			cout << div << endl;
			while (step > a && step < b) {
				skylineindex.push_back(step);
				skylinevalue.push_back(div);
				step += stepsize;
			}
			skylineindex.push_back(step);
			skylinevalue.push_back(div);
			step += stepsize;
		}
		loc--;
    }
   
}

/* Skyline for TMRCA */
/* At every event, take concurrent lineages and roll back until they share a common ancestor */
/* compare tmap of this common ancestor to the current time in phylogeny */
void CoalescentTree::tmrcaSkyline() { 

	/* have a vector[time points] of sets[node labels] */
	tree<int>::iterator it, jt, end;
	vector< set<int> > lineages (tlist.size());
	it = ctree.end();
	end = ctree.end(); 		
	int rootdepth=ctree.depth(it);
	
	/* go through tree and add to set */
	while(it!=ctree.begin()) {
		(lineages.at(ctree.depth(it)-rootdepth)).insert(*it);
		it--;
	}
	
	int loc = ctree.max_depth();
	double rate;
	int count;
	double step = stepsize;
  	for( set<double>::const_iterator iter = tlist.begin(); iter != --tlist.end(); iter++ ) {
  		set<int> nodes = lineages.at(loc);
  		double a = *iter;
  		iter++;
  		double b = *iter;
  		iter--;
  		if (a < b - 0.00000001) {
//			cout << "{" << a << "," << b << "} ";

			// take each node and add all its ancestors to the sharedSet;
			map<int,int> sharedMap;		// maps a node to its number of occurences
			for( set<int>::iterator jter = nodes.begin(); jter != nodes.end(); jter++ ) {
	
				int n = *jter;
				while (n != 0) {
					sharedMap[n]++;
					n = ancmap[n];
				}
			
			}
						
			// go through sharedMap and find highest count	
			int count = 0;	
			for( map<int,int>::iterator jter = sharedMap.begin(); jter != sharedMap.end(); jter++ ) {
				if ((*jter).second > count){
					count = (*jter).second;
				}
			}
				
			// go through sharedMap and find most recent node with count matching highest count
			double tmrca = tmap[0];			
			for( map<int,int>::iterator jter = sharedMap.begin(); jter != sharedMap.end(); jter++ ) {
				
			//	cout << "node = " <<  (*jter).first << ", count = " << (*jter).second << endl;
				
				if (tmap[(*jter).first] < tmrca && (*jter).second == count) {
					tmrca = tmap[(*jter).first];
				}
			
			}
			
			// difference in time now and back to TMRCA
			tmrca -= step;		
			
			while (step > a && step < b) {
				skylineindex.push_back(step);
				skylinevalue.push_back(tmrca);
				step += stepsize;
			}
			skylineindex.push_back(step);
			skylinevalue.push_back(tmrca);
			step += stepsize;
		}
		loc--;
    }
   
}


/* Skyline for segregating site (S/a1) */
/* At every event, take concurrent lineages and calculate total tree length */
/* a1 = sum 1 through n-1 of 1/i */
void CoalescentTree::tajimaSkyline() { 

	/* have a vector[time points] of sets[node labels] */
	tree<int>::iterator it, jt, end;
	vector< set<int> > lineages (tlist.size());
	it = ctree.end();
	end = ctree.end(); 		
	int rootdepth=ctree.depth(it);
	
	/* go through tree and add to set */
	while(it!=ctree.begin()) {
		(lineages.at(ctree.depth(it)-rootdepth)).insert(*it);
		it--;
	}
	
	int loc = ctree.max_depth();
	double rate;
	int count;
	double step = stepsize;
  	for( set<double>::const_iterator iter = tlist.begin(); iter != --tlist.end(); iter++ ) {
  		set<int> nodes = lineages.at(loc);
  		double a = *iter;
  		iter++;
  		double b = *iter;
  		iter--;
  		if (a < b - 0.00000001) {
	//		cout << "{" << a << "," << b << "} ";
			int divn = 0;
			double div = 0.0;
			for( set<int>::const_iterator jter = nodes.begin(); jter != --nodes.end(); jter++ ) {
				for( set<int>::const_iterator kter = jter; kter != nodes.end(); kter++ ) {
					if (*jter < *kter) { 
						set<int> s;
						s.insert(*jter);
						s.insert(*kter);
						tree<int> st = extractSubtree(s);
						divn++;
						div += getTreeLength(st);
					}
				}
			}
			div /= divn;
			
			double treeS;
			double a1 = 0.0;
			double a2 = 0.0;
			int n = nodes.size();
			tree<int> st = extractSubtree(nodes);
			treeS = getTreeLength(st);
			for (int i = 1; i < n; i++) {
				a1 += 1.0/i;
				a2 += 1.0/(i*i);
			}
	//		cout << " " << treeS << " " << a1 << " " << treeS/a1 << endl;
			
			double e1 = (1.0/a1) * ((double)(n+1) / (3*(n-1)) - (1.0/a1));
			double e2 = (1.0 / (a1*a1 + a2) ) * ( (double)(2*(n*n+n+3)) / (9*n*(n-1)) - (double)(n+2) / (n*a1) + a2/(a1*a1) );
			double denom = sqrt(e1*treeS + e2*treeS*(treeS-1));
			double tajima = (div - treeS / a1) / denom;
			
	//		double tajima = div - treeS / a1;
			
	//		cout << "a1: " << a1 << " a2: " << a2 << " e1: " << e1 << " e2: " << e2 << " denom: " << denom << " D: " << tajima << endl;
			
			while (step > a && step < b) {
				skylineindex.push_back(step);
				skylinevalue.push_back(tajima);
				step += stepsize;
			}
			skylineindex.push_back(step);
			skylinevalue.push_back(tajima);
			step += stepsize;
		}
		loc--;
    }
   
}

/* Skyline with time for each sampled lineage to coalesce with phylogeny trunk */
/* only looks at samples, not full tree */
void CoalescentTree::tcSkyline() { 

	tree<int>::iterator it, jt, end;
	it = ctree.begin();
	end = ctree.end(); 

	int steps = ceil(tmap[0] / stepsize);
	vector<int> counts (steps);
	vector<double> tc (steps);
	
	for (double i = 0.0; i <= tmap[0]; i+=stepsize) {
		skylineindex.push_back(i);
	}
	
	while(it!=ctree.end()) {
		if (leafSet.end() != leafSet.find(*it)) {
			
			jt = it;
			while (trunkSet.end() == trunkSet.find(*jt)) {
				jt = ctree.parent(jt);
			}
	//		cout << tmap[*it] << " " << tmap[*jt] - tmap[*it] << endl;
			counts.at( floor(tmap[*it] / stepsize + 0.5) )++;
			tc.at( floor(tmap[*it] / stepsize + 0.5) ) += tmap[*jt] - tmap[*it];
			
		}
		it++;
	}
    
    for (int i = 0; i < steps; i++) {
    	skylinevalue.push_back( tc[i] / counts[i] );
	}
		
	
}

/* Skyline for proportion with a particular label, take mean proportion for concurrent lineages */
void CoalescentTree::labelSkyline(int label) { 

	tree<int>::iterator it, jt, end;
	vector<int> lineages (tlist.size());
	vector<double> proportions (tlist.size());	
	it = ctree.end();
	end = ctree.end(); 		
	int rootdepth=ctree.depth(it);
	
	/* lineages contains a running tally of mean rate at this depth */
	while(it!=ctree.begin()) {
		lineages.at(ctree.depth(it)-rootdepth)++;
		if (lmap[*it] == label)
			proportions.at(ctree.depth(it)-rootdepth) += 1;
//		cout << *it << " " << rmap[*it] << endl;
		it--;
	}
	
	int loc = ctree.max_depth();
	double proportion;
	int count;
	double step = stepsize;
	
  	for( set<double>::const_iterator iter = tlist.begin(); iter != --tlist.end(); iter++ ) {
  		double a = *iter;
  		iter++;
  		double b = *iter;
  		iter--;
  		if (a < b - 0.00000001) {
			proportion = proportions.at(loc) / lineages.at(loc);
//			cout << "loc = " << loc << endl;
//			cout << "{" << a << "," << b << "} " << lineages.at(loc) << " " << proportion << endl;
			while (step > a && step < b) {
//				cout << step << "\t" << proportion << endl;
				skylineindex.push_back(step);
				skylinevalue.push_back(proportion);
				step += stepsize;
			}
//			cout << step << "\t" << proportion << endl;
			skylineindex.push_back(step);
			skylinevalue.push_back(proportion);
			step += stepsize;
		}
		if (loc == 0)
			break;
		else
			loc--;
    }
    
}

vector<double> CoalescentTree::getSkylineIndex() {   
    return skylineindex;
}

vector<double> CoalescentTree::getSkylineValue() {   
    return skylinevalue;
}

void CoalescentTree::setStepSize(double ss) {
	stepsize = ss;
}

/* removes cruft from maps and other data, based on current nodes in tree */
void CoalescentTree::reduce() {

	tree<Node>::iterator it, jt;

	/* removing pointless nodes, ie nodes that have no coalecent
	events or migration events associated with them */
	for (it = ++nodetree.begin(); it != nodetree.end(); it++) {
		if (nodetree.number_of_children(it) == 1) {						// no coalescence
			jt = nodetree.child(it,0);
			if ( (*it).getLabel() == (*jt).getLabel() ) {				// no migration
//				cout << "it = " << *it << ", jt = " << *jt << endl;
				(*jt).setLength( (*jt).getLength() + (*it).getLength() );	
 				nodetree.reparent(nodetree.parent(it),it);				// push child node up to be sibling of node
 				nodetree.erase(it);										// erase node									
				it = nodetree.begin();
			}
		}
	}

}

tree<int> CoalescentTree::extractSubtree(set<int> subset) {

	tree<int> stree;
	tree<int>::pre_order_iterator it, jt, end;

	/* fill top of tree with subset */
//	it = stree.set_head(*subset.begin());
	it = stree.set_head(0);
	for( set<int>::const_iterator jter = subset.begin(); jter != subset.end(); jter++ ) {
//		it = stree.insert_after(it, *jter);
		stree.append_child(it,*jter);
	}
	
	map<int,int> anc;
	
	while (stree.number_of_children(stree.begin()) > 1) {
			
		/* update stree based upon this mapping */
		for (tree<int>::sibling_iterator st = stree.begin(stree.begin()); st != stree.end(stree.begin()); st++) {
			st = stree.wrap(st,ancmap[*st]);
		}	
		
		/* compare all pairs of nodes at top level of stree, if match, merge children of these nodes */	
		tree<int>::sibling_iterator st, tt;
		st = stree.begin(stree.begin());
		while (st != --stree.end(stree.begin())) {
			tt = st;
			tt++;
			while (tt != stree.end(stree.begin())) {
				if (*st == *tt) {
					stree.reparent(st,tt);		// moves children of tt to be children of st
				}
				tt++;
			}
			st++;
		}
		
		/* remove nodes in stree without any children */
		for (tree<int>::sibling_iterator st = stree.begin(stree.begin()); st != stree.end(stree.begin()); st++) {
			if (stree.number_of_children(st) == 0) {
				st = stree.erase(st);
			}
		}	
		
		/* new subset is the top level of tree */
		subset.clear();
		for (tree<int>::sibling_iterator st = stree.begin(stree.begin()); st != stree.end(stree.begin()); st++) {
			subset.insert(*st);
		}	
	
	}
	
	/* remove 0 from head of tree */
	stree.move_after(stree.begin(),++stree.begin());
	stree.erase(stree.begin());
			  
	return stree;	

}

double CoalescentTree::getTreeLength(tree<int> &tr) {

	double length = 0.0;

	tree<int>::pre_order_iterator it, end;	
	it = tr.begin();
	end = tr.end();
	while(it!=end) {
		if (tr.depth(it) > 0 )
			length += tmap[*tr.parent(it)] - tmap[*it];
		it++;
	}
	
	return length;

}

/* returns maximium node associated with a node in the tree */
int CoalescentTree::getMaxNumber() {

	int n = 0;
	for (tree<Node>::iterator it = nodetree.begin(); it != nodetree.end(); it++) {
		if ((*it).getNumber() > n) {
			n = (*it).getNumber();
		}
	}
	return n;

}

/* given a number, returns iterator to associated node, or if not found, returns iterator to end of tree */
tree<Node>::iterator CoalescentTree::findNode(int n) {
	
	tree<Node>::iterator it;
	
	for (it = nodetree.begin(); it != nodetree.end(); it++) {
		if ((*it).getNumber() == n)
			break;
	}

	return it;
	
}

