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
/* Depracting branch rates */
/* One set of functions for migration events and branch lengths */
CoalescentTree::CoalescentTree(string paren, string options) {

	stepsize = 0.1;	
	
	string::iterator iterStr;
	tree<Node>:: iterator nit, njt;

	// STRIP PAREN STRING ///////////
	// strip spaces from paren string
	// strip & and following character, replace following : with =
	// assumes migration events follow the format [&M 5 3:8.49916e-05]
	iterStr=paren.begin() ;
	bool mig = false;
	while (iterStr < paren.end()) {
		if (*iterStr == ' ')
			iterStr = paren.erase(iterStr);
		else if (*iterStr == '&') {
			iterStr = paren.erase(iterStr);
			iterStr = paren.erase(iterStr);
			mig = true;
		}
		else if (*iterStr == ':' && mig) {
			*iterStr = '=';
			mig = false;
		}
		else
			iterStr++;
	}

	// GATHER TIPS ////////////////
	// read in node names, filling tips vector
	// names can only be 0-9 A-Z a-z
	// exported tree renames tips with consecutive numbering starting at 1
	// go through paren string and collect tips, at the same time replace names with matching numbers in paren string
	// if first character of name is a number and the second character is a letter, assume first character is label

	map<string,int> tips; 
	vector<Node> tipsList;
	leafCount = 0;	
	int stringPos = 0;
	string thisString = "";
	
	while (stringPos < paren.length()) {
		
		char thisChar = paren[stringPos];
		
		if ( (thisChar >= 'A' && thisChar <= 'Z') || (thisChar >= 'a' && thisChar <= 'z') || (thisChar >= '0' && thisChar <= '9') ) {
			thisString += thisChar;
		} 	  	
				
		else if (thisChar == ':' && thisString.length() > 0) {
			
	//		cout << paren << endl;
			
			leafCount++;
				
			/* nodetree update */	
			Node thisNode(leafCount);
			thisNode.setName(thisString);
			
			// label is the first character of node string, incremented by 1
			// only attempt this if first character is number and second character is letter
			// otherwise set to 1
			bool labelExists;
			if ( (thisString[0] >= '0' && thisString[0] <= '9') &&
					( (thisString[1] >= 'A' && thisString[1] <= 'Z') || (thisString[1] >= 'a' && thisString[1] <= '1') ) ) {
				labelExists = true;
			}
			else {
				labelExists = false;
			}
			if (labelExists) {
				thisNode.setLabel(atoi(thisString.substr(0,1).c_str()) + 1);
			}
			
			thisNode.setLeaf(true);
			
			tipsList.push_back(thisNode);

			/* replace name with number */	
			stringstream out;
			out << leafCount;
			paren = paren.substr(0,stringPos - thisString.size()) + out.str() + paren.substr(stringPos,paren.length());
			
			/* move counter back */
			/* need to take into acount the length of the digits */
			stringPos -= thisString.size() - (out.str()).length() + 1;		
			
			thisString = "";
		
		}
		
		else {
			thisString = "";
		}
		
		stringPos++;
		
	}
	
	nodeCount = leafCount;
	int currentNode = leafCount + 1;
	
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
		
	int leftNode, rightNode, openParen, closeParen, openMig, closeMig;	
	vector<double> blList;
	double leftLength, rightLength;

	/* mapping of nodes to the label of their parents */
	map<int,int> lmaptemp;
	int leftLabel,rightLabel;	
		
	// end when all parentheses have been eliminated
	while (paren.at(0) == '(') {
	
		cout << paren << endl;
		
		stringPos = 0;
		leftNode = -1;
		rightNode = -1;
		thisString = "";
	
		// will need to keep track of this across iterations
		int tempN, from, to;
		double migBranch;
					
		for ( iterStr=paren.begin(); iterStr < paren.end(); iterStr++ ) {

			if (*iterStr == '(') {
				openParen = stringPos;
				openMig = stringPos;
			}
			if ( (*iterStr >= '0' && *iterStr <= '9') || (*iterStr >= 'A' && *iterStr <= 'Z') || (*iterStr >= 'a' && *iterStr <= 'z') || *iterStr == '.' || *iterStr == '-' ) {
				thisString += *iterStr;
			}	
			else {
			
				bool stringExists;
				if (thisString.length() > 0)
					stringExists = true;
				else
					stringExists = false;
				
				// branch length
				if (( *iterStr == '[' || *iterStr == ',' || *iterStr == ')' ) && stringExists) {		
					bmap[rightNode] = atof(thisString.c_str());
					leftLength = rightLength;
					rightLength = atof(thisString.c_str());
					nit = findNode(rightNode);
//					(*nit).setLength( atof(thisString.c_str()) );
//					cout << "  bl = " << thisString.c_str() << endl;
				}					
				
				// node number
				if (*iterStr == ':' && stringExists) {	
//					cout << "node = " << atoi(thisString.c_str()) << endl;
					leftNode = rightNode;
					rightNode = atoi(thisString.c_str());		
				}

				if (*iterStr == ',') {
					openMig = stringPos;
				}		
				
				// MIGRATION EVENTS ////////////////////
				
				/* need to extend ctree here */
				/* can only deal with migration events that effect a tip node */
				/* this section is only called when brackets follow a tip node */
				if (*iterStr == '=' && stringExists) {
				
					/* grabbing migration event */
					string labelString = thisString;
					labelString.erase(0,1);
					from = atoi(labelString.c_str()) + 1;
					labelString = thisString;
					labelString.erase(1,1);		
					to = atoi(labelString.c_str()) + 1;
					lmaptemp[rightNode] = to;
					
				}				
				
				if (*iterStr == ']') {
				
					closeMig = stringPos; 
				
					migBranch = atof(thisString.c_str());
					cout << tempN << " mig from " << from << " to " << to << ", at " << migBranch << endl;
									
					tree<Node>:: iterator iRight, iNew, iTemp; 
					iRight = findNode(rightNode);	
					(*iRight).setLength( rightLength - migBranch );
		
					Node migNode(currentNode);
					migNode.setLabel(to);
					migNode.setLength(migBranch);
					
					iNew = nodetree.wrap(iRight,migNode);		
							
					/* replace parenthesis with new node label */	
					/* code is set up to deal with the situation of two labels before a parenthesis */
					stringstream out;
					out << currentNode << ":" << rightLength;
					paren = paren.substr(0,openMig+1) + out.str() + paren.substr(closeMig+1,paren.length());
				//	cout << paren << endl;
					
					currentNode++;
					nodeCount++;
					iterStr=paren.begin();	// reset count
							
					break;
				
				}
								
				thisString = "";
				
			}				
			
			// COALESCENT EVENTS //////////////////
			
			if (*iterStr == ')') { 
				
				cout << "close paren" << endl;
				
				closeParen = stringPos; 
				
				// iterate over the tree
				// once right node is reached, append a new node
				// append this new node with two branches (left node and right node)
				
				cout << "leftNode: " << leftNode << endl;	
				cout << "rightNode: " << rightNode << endl;					
					
				tree<Node>:: iterator iLeft, iRight, iNew, iTemp; 
				iLeft = findNode(leftNode);
				iRight = findNode(rightNode);	

				cout << "coal" << endl;

				(*iLeft).setLength(leftLength);
				(*iRight).setLength(rightLength);				
				
				Node newNode(currentNode);
				newNode.setLabel( (*iLeft).getLabel() );
			
				iNew = nodetree.wrap(iLeft,newNode);		
				nodetree.move_after(iLeft,iRight);
				
				/* replace parenthesis with new node label */		
				stringstream out;
				out << currentNode;
				paren = paren.substr(0,openParen) + out.str() + paren.substr(closeParen+1,paren.length());
			//	cout << paren << endl;
				
				currentNode++;
				nodeCount++;
				iterStr=paren.begin();	// reset count
							
				break;
					
				
			}
	
		stringPos++;
			
		}
	
	}
	
	
	// removing null root and replacing node label of root with 0						
	nodetree.erase(findNode(0));
	(*nodetree.begin()).setNumber(0);
	
	// adding branch length to the parent node's time to get the node's time
	rootTime = 0.0;
	presentTime = 0.0;	
	for (nit = ++nodetree.begin(); nit != nodetree.end(); nit++) {
		njt = nodetree.parent(nit);
		double t = (*njt).getTime() + (*nit).getLength();
		(*nit).setTime(t);
		if (t > presentTime) { presentTime = t; }
	}	
	  			
	/* go through tree and append to trunk set */
	/* only the last 1/100 of the time span is considered by default */	
	trunkTime = presentTime / (double) 100;
	nit = nodetree.begin();
	(*nit).setTrunk(true);
	while(nit != nodetree.end()) {
		/* find leaf nodes at present */
		if ((*nit).getTime() > presentTime - trunkTime && (*nit).getLeaf()) {
			njt = nit;
			/* move up tree adding nodes to trunk set */
			while ((*njt).getNumber() != 0) {
				(*njt).setTrunk(true);
				njt = nodetree.parent(njt);
			}
		}
		nit++;
	}

}

/* push dates to agree with a most recent sample date at endTime */
void CoalescentTree::pushTimesBack(double endTime) {
	
	// need to adjust times by this amount
	double diff = endTime - presentTime;
		
	// modifying tmap and tlist
	presentTime += diff;
	rootTime += diff;
	tlist.clear();
	for (tree<int>::iterator it = ctree.begin(); it != ctree.end(); it++) {
		tmap[*it] += diff;
		tlist.insert(tmap[*it]);
	}	
		  	
}

/* push dates to agree with a most recent sample date at endTime and oldest sample date is startTime */
void CoalescentTree::pushTimesBack(double startTime, double endTime) {
	
		 
	// STRETCH OR SHRINK //////////////	 
		 
	// find oldest sample
	double mostAncientTime = presentTime;
	for (set<int>::iterator is = leafSet.begin(); is != leafSet.end(); is++) {
		if (tmap[*is] < mostAncientTime) {
			mostAncientTime = tmap[*is];
		}
	}	
	
	double mp = (endTime - startTime) / (presentTime - mostAncientTime);
	
	// go through bmap and multiply by mp	
	for (tree<int>::iterator it = ++ctree.begin(); it != ctree.end(); it++) {
		bmap[*it] *= mp;
	}	
	
	// use tree and bmap to get tmap
	for (tree<int>::iterator it = ++ctree.begin(); it != ctree.end(); it++) {
		tmap[*it] = tmap[*ctree.parent(it)] + bmap[*it];
	}

	// fill tlist
	// find most recent time
	presentTime = 0.0;
	for (tree<int>::iterator it = ++ctree.begin(); it != ctree.end(); it++) {	
		tlist.insert(tmap[*it]);
		if (tmap[*it] > presentTime) {
			presentTime = tmap[*it];
		}
	}	
	
	// PUSH BACK /////////////////////

	// need to adjust times by this amount
	double diff = endTime - presentTime;
		
	// modifying tmap and tlist
	presentTime += diff;
	rootTime += diff;
	tlist.clear();
	for (tree<int>::iterator it = ctree.begin(); it != ctree.end(); it++) {
		tmap[*it] += diff;
		tlist.insert(tmap[*it]);
	}		
		 
}

/* padded with extra nodes at coalescent time points */
/* causing problems with migration tree */
void CoalescentTree::padTree() { 

	/* modifying the original tree */

	tree<int>::pre_order_iterator it, end, iterTemp, iterN;

	it = ctree.begin();
	end = ctree.end();
   
	if(!ctree.is_valid(it)) return;
	
	set<double>::const_iterator depth_it;
	int newDepth;
	int currentNode = nodeCount;
	
	/* pad tree with extra nodes, make sure there is a node at each time slice correspoding to coalescent event */
	while(it!=end) {
	
		/* finding what the correct depth of the node should be */
		newDepth = -1;
		for ( depth_it=tlist.end(); depth_it != tlist.find(tmap[*it]); depth_it-- )
    		newDepth++;
    	
    	depth_it++;
	
		if (newDepth > ctree.depth(it)) {
		
			/* padding with number of nodes equal to the difference in depth levels */
			for(int i=0; i<newDepth-ctree.depth(it); i++) {
	
				double rtemp = rmap[*it];
				int ltemp = lmap[*it];
	
				/* need to make a new sibling for a subtree with less than the correct depth */
				iterN = ctree.insert(it, currentNode);
				tmap[currentNode] = *depth_it;
				
				/* create a temporary child for this new node */
				iterTemp = ctree.append_child(iterN,0);
				
				/* replace this child with the subtree it */
				ctree.replace(iterTemp,it);	
			
				/* erase copied nodes */
				ctree.erase(it);
				
				it = iterN;
				currentNode++;
				depth_it++;
				
				rmap[*it] = rtemp;
				lmap[*it] = ltemp;
				
			}
	
		}
		
		it++;
	
	}
		
	/* updating ancmap and nodeCount */
	nodeCount = 0;
	ancmap.clear();
	it = ctree.begin();
	end = ctree.end();

//	cout << "test 1" << endl;		
	it++;							// root is not at 0
	
	while (it != end) {
		if (*it != 0) {
//			cout << *it << endl;
//			cout << *ctree.parent(it) << endl;
			ancmap[*it] = *ctree.parent(it);
			bmap[*it] = tmap[*it] - tmap[*ctree.parent(it)];
		}
		nodeCount++;
		it++;
	}

//	cout << "test 2" << endl;		
	  	
}

/* reduces a tree to just its trunk, takes most recent sample and works backward from this */
void CoalescentTree::pruneToTrunk() {
	
	/* erase other nodes from the tree */
	tree<int>::iterator it;	
	it = ctree.begin();
	while(it!=ctree.end()) {
		if (trunkSet.end() == trunkSet.find(*it)) {
			it = ctree.erase(it);
		}
		else {
    		it++;
    	}
    }
            
	/* update maps and other data accordingly */
	cleanup();
			
}

/* reduces a tree to samples with a single label */
void CoalescentTree::pruneToLabel(int label) {

	/* start by finding all tips with this label */
	set<int> labelset; 
	tree<int>::iterator it, jt;
	int newLeafs = 0;
	it = ctree.begin();
	
	while(it!=ctree.end()) {
		/* find leaf nodes at present */
		if (lmap[*it] == label) {     
			newLeafs++;
			jt = it;
			/* move up tree adding nodes to label set */
			while (*jt != 0) {
				labelset.insert(*jt);
				jt = ctree.parent(jt);
			}
		}
		it++;
	}
	
	/* make sure root is included */
	labelset.insert(0);
	
	/* erase other nodes from the tree */
	it = ctree.begin();
	while(it!=ctree.end()) {
		if (labelset.end() == labelset.find(*it)) {
			it = ctree.erase(it);
		}
		else {
    		it++;
    	}
    }
        
	/* update maps and other data accordingly */
	cleanup();
				
}

/* trims a tree at its edges */
/*
   		   |-------	 			 |-----
from  ------			 	to	--
   		   |----------		 	 |-----
*/
/* redo maps */
void CoalescentTree::trimEnds(double start, double stop) {
		
	/* erase nodes from the tree where neither the node nor its parent are between start and stop */
	tree<int>::iterator it, root, jt;	
	it = ctree.begin();
	root = ctree.begin();
	while(it!=ctree.end()) {	
		/* if node > stop and anc[node] < stop, erase children and prune node back to stop */
		if (tmap[*it] > stop && tmap[ancmap[*it]] <= stop) {
			tmap[*it] = stop;
			bmap[*it] = tmap[*it] - tmap[ancmap[*it]];
			ctree.erase_children(it);
			it = ctree.begin();
		}
		/* if node > start and anc[node] < start, push anc[node] up to start */
		/* and reparent anc[node] to be a child of root */
		/* neither node nore anc[node] can be root */
		else if (tmap[*it] > start && tmap[ancmap[*it]] < start && *it != 0 && ancmap[*it] != 0) {	
			tmap[ancmap[*it]] = start;
			bmap[ancmap[*it]] = 0;
			jt = ctree.append_child(root);
			ctree.move_ontop(jt, ctree.parent(it));
			it = ctree.begin();
		}
		else {
    		it++;
    	}
    }
    
    /* second pass for nodes < start */
    it = ctree.begin();
	while(it!=ctree.end()) {	
		if (tmap[*it] < start && *it != 0) {
			it = ctree.erase(it);
		}
		else {
    		it++;
    	}
    }
    tmap[0] = start;
               
	/* update maps and other data accordingly */
	cleanup();
		
}

/* Print parentheses tree */
void CoalescentTree::printParenTree() { 

	tree<int>::post_order_iterator it;
	tree<int>::post_order_iterator end;

	it = ctree.begin_post();
	end = ctree.end_post();
   	
	int rootdepth = ctree.depth(it);
	int currentdepth = ctree.depth(it);
	for (int i = 0; i < currentdepth; i++) { cout << "("; } 
	cout << (*it) << "";
	it++;
	
	/* basically, need to add a '(' whenever the depth increases and a ')' whenever the depth decreases */
	/* only print leaf nodes */
	while(it!=end) {
		if (ctree.depth(it) > currentdepth) { 
			cout << ", ("; 
			for (int i = 0; i < ctree.depth(it) - currentdepth - 1; i++) { cout << "("; }
			if (*it <= leafCount && *it > 0) { cout << (*it) << ""; }
		}
		if (ctree.depth(it) == currentdepth) { 
			if (*it <= leafCount && *it > 0) { cout << ", " << (*it) << ""; }
		}
		if (ctree.depth(it) < currentdepth) {
			if (*it <= leafCount && *it > 0) { cout << (*it) << ""; }
			cout << ")"; 
		}
		currentdepth = ctree.depth(it);
		it++;
		
	}
	
	cout << endl;

	
}

/* Print tree with times */
void CoalescentTree::printTree() { 

	tree<int>::iterator it;
	it = ctree.begin();
	int rootdepth=ctree.depth(it);
	
	while(it!=ctree.end()) {
		for(int i=0; i<ctree.depth(it)-rootdepth; ++i) 
			cout << "  ";
		cout << (*it) << " (" << tmap[*it] << ")";
		cout << " [" << lmap[*it] << "]";			
		cout << " {" << bmap[*it] << "}";		
		cout << endl << flush;
		it++;
	}
	
}

/* Print tree with times */
void CoalescentTree::printNodeTree() { 

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
	
	cout << "-----------" << endl;
	cout << "root: " << rootTime << endl;
	cout << "present: " << presentTime << endl;
	
}

/* print tree in Mathematica suitable format, 1->2,1->3, etc... */
/* padded with extra nodes at coalescent time points */
/* print mapping of nodes to rates if rates exist */
void CoalescentTree::printPaddedRuleList() { 

	/* pad tree with extra internal nodes */
	padTree();

	/* print tip count */
	cout << leafCount << endl;
	
	/* print nodes that exist at present */
	for (int i=1; i<leafCount; i++) {
		if (tmap[i] < 0.01)				// should specify this better
			cout << i << " ";
	}
	cout << endl;
	
	/* print trunk nodes */
	
	set<int> trunk; 
	
	tree<int>::iterator it, jt, end;
	it = ctree.begin();
	end = ctree.end();
   
	while(it!=end) {
		/* find leaf nodes at present */
		if (tmap[*it] < 0.01) {
			jt = it;
			/* move up tree adding nodes to trunk set */
			while (*jt != 0) {
				trunk.insert(*jt);
				jt = ctree.parent(jt);
			}
		}
		it++;
	}
	
	for ( set<int>::const_iterator lit=trunk.begin(); lit != trunk.end(); lit++ ) {
		cout << *lit << " ";
	}
	cout << endl;
	
	/* print the tree in rule list (Mathematica-ready) format */
	/* print only upward links */
	tree<int>::pre_order_iterator iterTemp, iterN;
	it = ctree.begin();
	end = ctree.end();
	
	it++;
	cout << *it << "->" << *ctree.parent(it);
	it++;
	while(it!=end) {
		cout << " " << *it << "->" << *ctree.parent(it);
		it++;
	}
	cout << endl;
	
	/* print list of coalescent times in Mathematica-ready format */
	cout << *(tlist.begin());
  	for( set<double>::const_iterator iter = ++tlist.begin(); iter != tlist.end(); iter++ ) {
		cout << " " << *iter;
    }
	cout << endl;
	
	/* print mapping of labels in Mathematica format */
	it = ctree.begin();
	it++;
	cout << *it << "->" << lmap[*it];
	it++;
	while(it!=end) {
		cout << " " << *it << "->" << lmap[*it];
		it++;
	}
	cout << endl;
	  	
  	
}


/* print tree in Mathematica suitable format, 1->2,1->3, etc... */
/* padded with extra nodes at coalescent time points */
/* print mapping of nodes to rates if rates exist */
/* Output is:
	leaf list
	trunk list
	tree rules
	label rules
	coordinate rules
*/	
void CoalescentTree::printRuleList() { 

	tree<int>::iterator it, jt;

	/* reorder tree so that the bottom node of two sister nodes always has the most recent child more children */
	/* this combined with preorder traversal will insure the trunk follows a rough diagonal */
	it = ctree.begin();
	while(it!=ctree.end()) {
		jt = ctree.next_sibling(it);
		if (ctree.is_valid(jt)) {
			int cit = ctree.size(it);
			int cjt = ctree.size(jt);
			if (cit > cjt) {
				ctree.swap(jt,it);
				it = ctree.begin();
			}
		}
		it++;
	}

	/* print leaf nodes */
	for ( set<int>::const_iterator lit=leafSet.begin(); lit != leafSet.end(); lit++ ) {
		cout << *lit << " ";
	}
	cout << endl;
	
	/* print trunk nodes */
	for ( set<int>::const_iterator lit=trunkSet.begin(); lit != trunkSet.end(); lit++ ) {
		cout << *lit << " ";
	}
	cout << endl;
			
	/* print the tree in rule list (Mathematica-ready) format */
	/* print only upward links */
	for (it = ++ctree.begin(); it != ctree.end(); it++) {		// increment past root
		cout << *it << "->" << *ctree.parent(it) << " ";
	}
	cout << endl;
	
	
	/* print mapping of labels in Mathematica format */
	for (it = ctree.begin(); it != ctree.end(); it++) {	
		cout << *it << "->" << lmap[*it] << " ";
	}
	cout << endl;
	
	/* print mapping of nodes to coordinates */
  	int count = 0;
	for (it = ctree.begin(); it != ctree.end(); it++) {
		cout << *it << "->{" << tmap[*it] << "," << count << "} ";
		
		// count should only increase when a coalescent event occurs
		if (ctree.number_of_children(it) == 2) {	
			count++;
		}
	}
	cout << endl;	
	  	  	
}

/* print proportion of tree that can trace its history forward to present day samples */
/* trunk traced back from the last 1/100 of the time width */
void CoalescentTree::printTrunkRatio() { 

	double totalLength = 0.0;
	double trunkLength = 0.0;
	
	set<int> trunk; 
	tree<int>::iterator it, jt, end;

	/* sum branch lengths */
	for (int i=0; i<nodeCount; i++) {
		totalLength += bmap[i];
	}
	cout << "total tree length: " << totalLength << endl;
		
	/* move through trunk nodes and total branch lengths */
	for ( set<int>::const_iterator is = trunkSet.begin(); is != trunkSet.end(); is++ ) {
		trunkLength += bmap[*is];
    }
    
    cout << "trunk length: " << trunkLength << endl;
    cout << "ratio: " << trunkLength / totalLength << endl;
    	
}

/* print proportion of tree with each label */
void CoalescentTree::printLabelPro() { 

	/* calculation is to get the total length of each label */
	
	double totalLength = 0.0;
	map <int,double> labelLengths;
	set<int>::const_iterator is;

	for ( is=labelSet.begin(); is != labelSet.end(); is++ ) {
   
		/* sum branch lengths */
		/* have to go through tree structure, nodes can be non-consecutive */
		double length = 0.0;
		for (tree<int>::iterator it = ctree.begin(); it != ctree.end(); it++ ) {
			if (*is == lmap[*it]) {
				length += bmap[*it];
			}
		}
		totalLength += length;
		labelLengths.insert( make_pair(*is,length ) );
		
	}
	
	for ( is=labelSet.begin(); is != labelSet.end(); is++ ) {
		cout << labelLengths[*is] / totalLength << " ";
	}
	cout << endl;
	
}

/* get length of tree with each label */
vector<double> CoalescentTree::getLabelLengths() { 
	
	vector<double> labelLengths;	
	
	for ( set<int>::const_iterator is = labelSet.begin(); is != labelSet.end(); is++ ) {
   
		/* sum branch lengths */
		/* have to go through tree structure, nodes can be non-consecutive */
		double length = 0.0;
		for (tree<int>::iterator it = ctree.begin(); it != ctree.end(); it++ ) {
			if (*is == lmap[*it]) {
				length += bmap[*it];
			}
		}
	
		labelLengths.push_back(length);
		
	}
	
	return labelLengths;
	
}

/* get proportion of tree with each label, works with Skyline object */
vector<double> CoalescentTree::getLabelPro() { 

	vector<double> labelLengths = getLabelLengths();
	
	/* calculate total length */
	double totalLength = 0.0;
	for (int i = 0; i < labelLengths.size(); i++) {
		totalLength += labelLengths[i];
	}
	
	/* divide by this length to get frequencies */
	for (int i = 0; i < labelLengths.size(); i++) {
		labelLengths[i] /= totalLength;
	}
	
	return labelLengths;
	
}

/* print total rate of migration across tree, measured as events/year */
void CoalescentTree::printMigTotal() { 

	/* calculation is to get the total length in years of each label */
	double totalLength = 0;
	set<int>::const_iterator lit, ljt;
	for ( lit=labelSet.begin(); lit != labelSet.end(); lit++ ) {
   
		/* sum branch lengths */
		/* have to go through tree structure, nodes can be non-consecutive */
		for (tree<int>::iterator it = ctree.begin(); it != ctree.end(); it++ ) {
			if (*lit == lmap[*it]) {
				totalLength += bmap[*it];
			}
		}		
	}
//	cout << "total length: " << totalLength << endl;
	
	/* count migration events */	
	int count = 0;
	for ( ljt=labelSet.begin(); ljt != labelSet.end(); ljt++ ) {		// to
		for ( lit=labelSet.begin(); lit != labelSet.end(); lit++ ) {	// from
			if (*lit != *ljt) {
				
				/* go through tree and find situations where parent matches lit and child matches ljt */
				/* using ancmap for speed */
				for (tree<int>::iterator it = ctree.begin(); it != ctree.end(); it++ ) {	
					if ( lmap[*it] == *ljt && lmap[ancmap[*it]] == *lit 
					&& *it != 0 && ancmap[*it] != 0) {						// exclude root
						count++;
//						cout << "from " << i << " [" << *lit << "]" << " to " << ancmap[i] << " [" << *ljt << "]" << endl;
					}
				}
				
			}
		}
	}
	
	/* rate is equal to events / opportunity */
	cout << count / totalLength << endl;
	
}


/* print matrix of migration rates */
void CoalescentTree::printMigRates() { 

	/* calculation is to get the total length in years of each label */

	double totalLength = 0;
	map <int,double> labelLengths;
	set<int>::const_iterator lit, ljt;

	for ( lit=labelSet.begin(); lit != labelSet.end(); lit++ ) {
   
		/* sum branch lengths */
		/* have to go through tree structure, nodes can be non-consecutive */
		double length = 0.0;
		for (tree<int>::iterator it = ctree.begin(); it != ctree.end(); it++ ) {
			if (*lit == lmap[*it]) {
//				cout << *it << " " << bmap[*it] << endl;
				length += bmap[*it];
			}
		}
		totalLength += length;
//		cout << "label: " << *lit << ", length: " << length << endl;
		labelLengths.insert( make_pair(*lit,length ) );
		
	}
//	cout << "total length: " << totalLength << endl;
	
	/* count migration events */
	/* ordering matches that used by Migrate */
	
	for ( ljt=labelSet.begin(); ljt != labelSet.end(); ljt++ ) {		// to
		for ( lit=labelSet.begin(); lit != labelSet.end(); lit++ ) {	// from
			if (*lit != *ljt) {
				
				/* go through tree and find situations where parent matches lit and child matches ljt */
				/* using ancmap for speed */
				int count = 0;
				for (tree<int>::iterator it = ctree.begin(); it != ctree.end(); it++ ) {	
					if ( lmap[*it] == *ljt && lmap[ancmap[*it]] == *lit 
					&& *it != 0 && ancmap[*it] != 0) {						// exclude root
						count++;
//						cout << "from " << i << " [" << *lit << "]" << " to " << ancmap[i] << " [" << *ljt << "]" << endl;
					}
				}
		
				/* rate is equal to events / opportunity */
				/* divide by length of lit */
				
//				cout << "from " << *lit << " to " << *ljt << ": " << "count = " << count << ", rate = ";
				cout << count / labelLengths[*lit] << " ";
//				cout << endl;
		
			}
		}
	}
	cout << endl;
	
}


map<int,double> CoalescentTree::getMigWeights() { 

	/* calculation is to get the total length in years of each label */

	double totalLength = 0;
	map <int,double> labelLengths;
	set<int>::const_iterator lit, ljt;
	
	map<int,double> migWeights;

	for ( lit=labelSet.begin(); lit != labelSet.end(); lit++ ) {
   
		/* sum branch lengths */
		/* have to go through tree structure, nodes can be non-consecutive */
		double length = 0.0;
		for (tree<int>::iterator it = ctree.begin(); it != ctree.end(); it++ ) {
			if (*lit == lmap[*it]) {
				if (bmap[*it] > 0) {			// correcting for the occasional negative branch length
					length += bmap[*it];
				}
			}
		}
		totalLength += length;
//		cout << "label: " << *lit << ", length: " << length << endl;
		labelLengths.insert( make_pair(*lit,length ) );
		
	}
//	cout << "total length: " << totalLength << endl;
		
	for ( ljt=labelSet.begin(); ljt != labelSet.end(); ljt++ ) {		// to
		for ( lit=labelSet.begin(); lit != labelSet.end(); lit++ ) {	// from
			if (*lit != *ljt) {
								
				migWeights[*ljt * 10 + *lit] = labelLengths[*lit];
		
			}
		}
	}
	
	return migWeights;
	
}

map<int,int> CoalescentTree::getMigCounts() { 

	/* calculation is to get the total length in years of each label */

	set<int>::const_iterator lit, ljt;	
	map<int,int> migCounts;
		
	for ( ljt=labelSet.begin(); ljt != labelSet.end(); ljt++ ) {		// to
		for ( lit=labelSet.begin(); lit != labelSet.end(); lit++ ) {	// from
			if (*lit != *ljt) {
				
				/* go through tree and find situations where parent matches lit and child matches ljt */
				/* using ancmap for speed */
				int count = 0;
				for (tree<int>::iterator it = ctree.begin(); it != ctree.end(); it++ ) {	
					if ( lmap[*it] == *ljt && lmap[ancmap[*it]] == *lit 
					&& *it != 0 && ancmap[*it] != 0					// exclude root
					&& bmap[*it] > 0 && bmap[ancmap[*it]] > 0) {	// correcting for the occasional negative branch length			
						count++;
//						cout << "from " << i << " [" << *lit << "]" << " to " << ancmap[i] << " [" << *ljt << "]" << endl;
					}
				}
		
				/* rate is equal to events / opportunity */
				/* divide by length of lit */
				
				migCounts[*ljt * 10 + *lit] = count;
		
			}
		}
	}
	
	return migCounts;
	
}

map<int,double> CoalescentTree::getRevMigWeights() { 

	/* calculation is to get the total length in years of each label */

	double totalLength = 0;
	map <int,double> labelLengths;
	set<int>::const_iterator lit, ljt;
	
	map<int,double> migWeights;

	for ( lit=labelSet.begin(); lit != labelSet.end(); lit++ ) {
   
		/* sum branch lengths */
		/* have to go through tree structure, nodes can be non-consecutive */
		double length = 0.0;
		for (tree<int>::iterator it = ctree.begin(); it != ctree.end(); it++ ) {
			if (*lit == lmap[*it]) {
				if (bmap[*it] > 0) {			// correcting for the occasional negative branch length
					length += bmap[*it];
				}
			}
		}
		totalLength += length;
//		cout << "label: " << *lit << ", length: " << length << endl;
		labelLengths.insert( make_pair(*lit,length ) );
		
	}
//	cout << "total length: " << totalLength << endl;
		
	for ( ljt=labelSet.begin(); ljt != labelSet.end(); ljt++ ) {		// to
		for ( lit=labelSet.begin(); lit != labelSet.end(); lit++ ) {	// from
			if (*lit != *ljt) {
								
				migWeights[*ljt * 10 + *lit] = labelLengths[*ljt];		// forward uses from deme
																		// backward uses to deme
		
			}
		}
	}
	
	return migWeights;
	
}

map<int,int> CoalescentTree::getRevMigCounts() { 

	set<int>::const_iterator lit, ljt;	
	map<int,int> migCounts;
		
	for ( ljt=labelSet.begin(); ljt != labelSet.end(); ljt++ ) {		// to
		for ( lit=labelSet.begin(); lit != labelSet.end(); lit++ ) {	// from
			if (*lit != *ljt) {
				
				/* go through tree and find situations where parent matches lit and child matches ljt */
				/* using ancmap for speed */
				int count = 0;
				for (tree<int>::iterator it = ctree.begin(); it != ctree.end(); it++ ) {	
					if ( lmap[*it] == *lit	 						// from deme is child
					&& lmap[ancmap[*it]] == *ljt 					// to deme is parent
					&& *it != 0 && ancmap[*it] != 0					// exclude root
					&& bmap[*it] > 0 && bmap[ancmap[*it]] > 0) {	// correcting for the occasional negative branch length			
						count++;
//						cout << "from " << i << " [" << *lit << "]" << " to " << ancmap[i] << " [" << *ljt << "]" << endl;
					}
				}
		
				/* rate is equal to events / opportunity */
				/* divide by length of lit */
				
				migCounts[*ljt * 10 + *lit] = count;
		
			}
		}
	}
	
	return migCounts;
	
}


/* go through tree and print the coalescent rate for each label */
void CoalescentTree::printCoalRates() {

	tree<int>::iterator it;
	
	/* go through tree and find end points */
	double start, stop;
	start = tmap[*ctree.begin()];
	stop = tmap[*ctree.begin()];
	for (it = ctree.begin(); it != ctree.end(); it++) {
		if (start > tmap[*it]) {
			start = tmap[*it];
		}
	}

	// setting step to be 1/1000 of the total length of the tree
	double step = (stop - start) / (double) 1000;

//	cout << "start = " << start << ", stop = " << stop << ", step = " << step << endl;
	
	set<int>::const_iterator lit;
	for ( lit=labelSet.begin(); lit != labelSet.end(); lit++ ) {
	
//		cout << "label = " << *lit << endl;
	
		/* go through tree and count concurrent lineages */
		/* weight number by n(n-1)/2 to get coalescent opportunity */
		double labelWeight = 0.0;
		for (double t = start; t <= stop; t += step) {
		
//			cout << t << endl;
			int count = 0;
			for (it = ctree.begin(); it != ctree.end(); it++) {
				if (tmap[*it] < t && tmap[ancmap[*it]] >= t && lmap[*it] == *lit) {		
//					cout << *it << " ";
					count++;
				}
			}
			double weight;
			if (count > 0)
				weight = ( ( count * (count - 1) ) / 2 ) * step;
			else
				weight = 0;
//			cout << "count = " << count << ", weight = " << weight << endl;
			labelWeight += weight;
		
		}
		
		
		/* count coalescent events, these are nodes with two children */
		int count = 0;
		for (it = ctree.begin(); it != ctree.end(); it++) {
			if (ctree.number_of_children(it) == 2 && lmap[*it] == *lit) {		
				count++;
			}
		}
//		cout << "label = " << *lit << ", weight = " << labelWeight << ", count = " << count << ", rate = " << count / labelWeight << ", timescale = " << labelWeight / count << endl;
		cout << count / labelWeight << " ";

	}
	cout << endl;
	
}

/* go through tree and print the coalescent rate for each label */
/* only concerned with coalescent events that include the trunk */
void CoalescentTree::printTrunkRates() {

	tree<int>::iterator it;
	
	/* go through tree and find end points */
	double start, stop;
	start = tmap[*ctree.begin()];
	stop = tmap[*ctree.begin()];
	for (it = ctree.begin(); it != ctree.end(); it++) {
		if (start > tmap[*it]) {
			start = tmap[*it];
		}
	}
	
	// setting step to be 1/1000 of the total length of the tree
	double step = (stop - start) / (double) 1000;	
	
//	cout << "start = " << start << ", stop = " << stop << endl;
	
	set<int>::const_iterator lit;
	for ( lit=labelSet.begin(); lit != labelSet.end(); lit++ ) {
	
//		cout << "label = " << *lit << endl;
	
		// go through tree and count concurrent lineages
		// if there is 1 trunk lineage and 1 branch lineage, this will mean an opportunity of 1
		// 1 trunk, 2 branch = opportunity of 2
		// 1 trunk, 3 branch =  opportunity of 3
		// 2 trunk, 3 branch = opportunity of 7
		// trunk x, branch y =  x*y + 0.5*(x-1)*(x)
		double labelWeight = 0.0;
		for (double t = start; t <= stop; t += step) {
		
//			cout << "time = " << t << endl;
			int branchCount = 0;
			int trunkCount = 0;
			for (it = ctree.begin(); it != ctree.end(); it++) {
				if (tmap[*it] < t && tmap[ancmap[*it]] >= t && lmap[*it] == *lit) {			// found a lineage
					if (trunkSet.end() != trunkSet.find(*it)) {								// lineage is trunk
//						cout << "trunk = "<< *it << endl;
						trunkCount++;
					}
					else {
//						cout << "branch = "<< *it << endl;
						branchCount++;					
					}
				}
			}
			double weight;
			if (trunkCount > 0)
				weight = ( trunkCount*branchCount + 0.5 * (trunkCount-1) * trunkCount ) * step;
			else
				weight = 0;
//			cout << "trunkCount = " << trunkCount << ", branchCount = " << branchCount << ", weight = " << weight << endl;
			labelWeight += weight;
		
		}
		
		
		/* count trunk coalescent events, these are trunk nodes with two children */
		int count = 0;
		for (it = ctree.begin(); it != ctree.end(); it++) {
			if (ctree.number_of_children(it) == 2 && lmap[*it] == *lit && trunkSet.end() != trunkSet.find(*it) ) {		
				count++;
			}
		}
//		cout << "label = " << *lit << ", weight = " << labelWeight << ", count = " << count << ", rate = " << count / labelWeight << ", timescale = " << labelWeight / count << endl;
		cout << count / labelWeight << " ";

	}
	cout << endl;
	
}

/* return map of coalescent weights for each label */
map<int,double> CoalescentTree::getCoalWeights() {

	double step = 0.0001;
	tree<int>::iterator it;
	map<int,double> labelWeights;
	
	/* go through tree and find end points */
	double start, stop;
	start = tmap[*ctree.begin()];
	stop = tmap[*ctree.begin()];
	for (it = ctree.begin(); it != ctree.end(); it++) {
		if (start > tmap[*it]) {
			start = tmap[*it];
		}
	}
		
	for ( set<int>::const_iterator lit = labelSet.begin(); lit != labelSet.end(); lit++ ) {
		
		labelWeights.insert(make_pair(*lit,0.0));
		
		/* go through tree and count concurrent lineages */
		/* weight number by n(n-1)/2 to get coalescent opportunity */
		for (double t = start; t <= stop; t += step) {
		
//			cout << t << endl;
			int count = 0;
			for (it = ctree.begin(); it != ctree.end(); it++) {
				if (tmap[*it] < t && tmap[ancmap[*it]] >= t && lmap[*it] == *lit) {		
//					cout << *it << " ";
					count++;
				}
			}
			double weight;
			if (count > 0)
				weight = ( ( count * (count - 1) ) / 2 ) * step;
			else
				weight = 0;
//			cout << "count = " << count << ", weight = " << weight << endl;
			labelWeights[*lit] += weight;
		
		}
			
	}
	
	return labelWeights;
}

/* return map of coalescent counts for each label */
/* making it a double to talk better with LabelSummary */
map<int,int> CoalescentTree::getCoalCounts() {

	tree<int>::iterator it;
	map<int,int> labelCounts;
	
	/* go through tree and find end points */
	double start, stop;
	start = tmap[*ctree.begin()];
	stop = tmap[*ctree.begin()];
	for (it = ctree.begin(); it != ctree.end(); it++) {
		if (start > tmap[*it]) {
			start = tmap[*it];
		}
	}
		
	for ( set<int>::const_iterator lit = labelSet.begin(); lit != labelSet.end(); lit++ ) {
		
		labelCounts.insert(make_pair(*lit,0));
				
		/* count coalescent events, these are nodes with two children */
		for (it = ctree.begin(); it != ctree.end(); it++) {
			if (ctree.number_of_children(it) == 2 && lmap[*it] == *lit) {		
				labelCounts[*lit]++;
			}
		}
			
	}
	
	return labelCounts;
}



set<int> CoalescentTree::getLabelSet() {
	return labelSet;
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
void CoalescentTree::cleanup() {

	tree<int>::iterator it, jt;

	/* removing pointless nodes, ie nodes that have no coalecent
	events or migration events associated with them */
	for (it = ctree.begin(); it != ctree.end(); it++) {
		if (ctree.number_of_children(it) == 1) {			// no coalescence
			jt = ctree.child(it,0);
			if (lmap[*it] == lmap[*jt]) {					// no migration
//				cout << "it = " << *it << ", jt = " << *jt << endl;
				bmap[*jt] += bmap[*it];
				ancmap[*jt] = ancmap[*it];	
 				ctree.reparent(ctree.parent(it),it);		// push child node up to be sibling of node
 				ctree.erase(it);							// erase node									
				it = ctree.begin();
			}
		}
	}


	/* want to reduce maps to just the subset dealing with these nodes */
	set<int> nodeSet;
	for (it = ctree.begin(); it != ctree.end(); it++) {
		nodeSet.insert(*it);
	}
		
	/* go through and erase nonfunctionaling map elements */
	for (int i = 0; i < nodeCount; i++) {
		if (nodeSet.count(i) == 0) {
			leafSet.erase(i);
			trunkSet.erase(i);
			bmap.erase(i);
			rmap.erase(i);
			tmap.erase(i);
			lmap.erase(i);
			ancmap.erase(i);
		}
	}

    /* update other private data */
    nodeCount = ctree.size();
    tlist.clear();
    leafSet.clear();
    leafCount = 0;
	for (it=ctree.begin(); it!=ctree.end(); it++) {
		tlist.insert(tmap[*it]);
		if (ctree.number_of_children(it)==0) {
			leafCount++;
			leafSet.insert(*it);
		}
	}
	
	rootTime = *tlist.begin();
	presentTime = *tlist.end();

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

tree<Node>::iterator CoalescentTree::findNode(int n) {
	
	tree<Node>::iterator it;
	
	for (it = nodetree.begin(); it != nodetree.end(); it++) {
		if ((*it).getNumber() == n)
			break;
	}

	return it;
	
}
