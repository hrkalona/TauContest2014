#include "dgraph.h"


void DominatorGraph::outputGraphStatistics (FILE *file) {
	int n = getNVertices();
	int m = getNArcs();
	int o = getOriginalNArcs();

	fprintf (file, "vertices %d\n", n);
	fprintf (file, "edges %d\n", m);
	fprintf (file, "arcs %d\n", m);
	fprintf (file, "size %d\n", n+m);
	fprintf (file, "density %f\n", (double)m/(double)n);
	fprintf (file, "originalarcs %d\n", o);
	fprintf (file, "originalsize %d\n", o+n);
	fprintf (file, "originalarcs %f\n", (double)o/(double)n);
	fprintf (file, "source %d\n", getSource());
	fprintf (file, "logvertices %d\n", log2(n));
	fprintf (file, "logarcs %d\n", log2(m));
	fprintf (file, "logsize %d\n", log2(m+n));
}
	
	
void DominatorGraph::output (FILE *file, bool reverse) {
	for (int v=1; v<=n; v++) {
		fprintf (file, "%d:", v);
		int *p, *stop;
		if (!reverse) getOutBounds (v, p, stop);
		else getInBounds (v, p, stop);
		for (; p<stop; p++) {
			fprintf (file, " %d", *p);
		}
		fprintf (stderr, "\n");
	}
}

int DominatorGraph::run_dfs (int r) {
	int bsize = n+1;
	int *buffer = new int [3*bsize];
	int *pre2label = &buffer[0];
	int *label2pre = &buffer[bsize];
	int *parent = &buffer[2*bsize];
	int visited  = preDFSp (r, label2pre, pre2label, parent); //, temp);
	delete [] buffer;
	return visited;
}

int DominatorGraph::run_bfs (int r) {
	int bsize = n+1;
	int *buffer = new int[3*bsize];
	int *pre2label = &buffer[0];
	int *label2pre = &buffer[bsize];
	int *parent    = &buffer[2*bsize];
			
	int visited  = preBFSp (r, label2pre, pre2label, parent);

	delete [] buffer;
	return visited;
}



/*-------------------------------------
 | build the graph from a list of arcs
 *------------------------------------*/

void DominatorGraph::buildGraph (int _nvertices, int _narcs, int _source, int *arclist, bool remove_duplicates) {
	
	const bool verbose = false;
	int v;
			
	if (verbose) fprintf (stderr, "Building graph...\n");
	deleteAll(); //just in case

	n = _nvertices;
	narcs = _narcs;
	source = _source;

	//initialize arrays
	first_in = new intptr [n+2];
	first_out = new intptr [n+2];
	in_arcs = new int [narcs];
	out_arcs = new int [narcs];

	//temporarily, first_in and first_out will represent the degrees 
	for (v=n+1; v>=0; v--) {
		first_in[v].value = first_out[v].value = 0;
	}

	//update the degrees of everybody
	int *a = &arclist[0];
	int *stop = &arclist[2*narcs];
	while (a!=stop) {
		int v = *(a++);
		int w = *(a++);
		first_in[w].value++;  //indegree of w increases
		first_out[v].value++; //outdegree of v increases
	}

	//make first_in and first_out point to the position after the last one
	first_in[0].value = first_out[0].value = 0;
	for (v=1; v<=n+1; v++) {
		first_in[v].value = first_in[v-1].value + first_in[v].value;
		first_out[v].value = first_out[v-1].value + first_out[v].value;
	}

	//insert the arcs; in the process, last_in and last_out will end up being correct
	a = &arclist[2*narcs-1];
	stop = &arclist[0];
	while (a>=stop) {
		int w = *(a--);
		int v = *(a--); //arc is (v,w)
		in_arcs[--(first_in[w].value)] = v;  //(v,w) is an incoming arc for w
		out_arcs[--(first_out[v].value)] = w; //(v,w) is an outgoing arc for v
	}

	onarcs = narcs;

	//eliminate duplicate arcs
	if (remove_duplicates) {
		//fprintf (stderr, "Eliminating duplicates...\n");
		eliminateDuplicates(first_in, in_arcs);
		eliminateDuplicates(first_out, out_arcs);
		narcs = first_in[n+1].value;
	}
	
	//convert indices to pointers for faster accesses
	for (int v=1; v<=n+1; v++) {
		first_in[v].ptr = &in_arcs[first_in[v].value];
		first_out[v].ptr = &out_arcs[first_out[v].value];
	}
}
			

/*----------------------------------------
 | Eliminate duplicate arcs:
 | - Changes both 'first' and 'arcs'.
 | - assumes 'first' uses the int fields.
 | - 'arcs' will still be contiguous.
 *---------------------------------------*/

void DominatorGraph::eliminateDuplicates(intptr *first, int *arcs) {
	const bool verbose = false;
	int *mark = new int [n+1];
	int v, w, cur, stable;
	for (v=n; v>0; v--) mark[v] = 0;
	
	cur = first[1].value;    //current position in arcs
	stable = first[1].value; //next in which insertions will be made

	for (v=1; v<=n; v++) {
		mark[v] = v;
		//check all neighbors
		while (cur!=first[v+1].value) {
			w = arcs[cur];
			if (mark[w]!=v) {
				arcs[stable++] = w; //keep (v,w), advance stable
				mark[w]=v;
				if (verbose) fprintf (stderr, "  ");
			} else {
				if (verbose) fprintf (stderr, "> ");
			}
			if (verbose) fprintf (stderr, "%3d %3d\n", v, w);
			cur++; //advance the current
		}
		first[v+1].value = stable;
	}
	delete [] mark;
}



/*-------------------------------
 | read a graph in dimacs format 
 *------------------------------*/

void DominatorGraph::readDimacs (const char *filename, bool reverse, bool simplify) {
	const bool verbose = false;
	if (verbose) fprintf (stderr, "Reading file \"%s\"... \n", filename);

	FILE *input = fopen (filename, "r");
	if (!input) {
		fprintf (stderr, "Error opening file \"%s\".\n", filename);
		exit(-1);
	}
	
	int n, m, src, snk;

	if (fscanf(input,"p %d %d %d %d\n", &n, &m, &src, &snk)!=4) {
		fprintf (stderr, "Error reading graph size (%s).\n", filename);
		exit (-1);
	}		
	if (verbose) fprintf (stderr, "File has %d nodes and %d edges, source is %d, sink is %d... ", n, m, src, snk);
	if (reverse) src = snk;

	int *arclist = new int [2*m];
	int p = 0;
	if (reverse) {
		while (1) {
			int a, b;
			if (fscanf (input, "a %d %d\n", &a, &b)!=2) break; //arc from a to b
			arclist[p++] = b; //in reverse!
			arclist[p++] = a;
		}
	} else {
		while (1) {
			int a, b;
			if (fscanf (input, "a %d %d\n", &a, &b)!=2) break; //arc from a to b
			arclist[p++] = a;
			arclist[p++] = b;
		}
	}
	fclose (input);
	if (verbose) fprintf (stderr, "done.\n");
	buildGraph (n, m, src, arclist, simplify);

	delete [] arclist;
}


/*------------------------
 | pre-dfs (with parents)
 *-----------------------*/

void DominatorGraph::rpreDFSp (int v, PreDFSParams &params) {
	int *p, *stop, pre_v;
	pre_v = params.next;
	params.pre2label[params.next] = v;   //v will have the next label
	params.label2pre[v] = params.next++; //v's label is next (and next is incremented)
	getOutBounds(v,p,stop);
	for (; p<stop; p++) { //visit all outgoing neighbors
		if (!params.label2pre[*p]) {
			params.parent[params.next] = pre_v;
			rpreDFSp(*p,params);
		}
	}
}

int DominatorGraph::preDFSp (int v, int *label2pre, int *pre2label, int *parent) {//, int &next) {
	PreDFSParams params;
	params.label2pre = label2pre;
	params.pre2label = pre2label;
	params.parent = parent;
	params.next = 1;

	for (int w=n; w>=0; w--) params.label2pre[w] = 0; //everybody unvisited
	rpreDFSp (v, params); //visit everybody reachable from the root
	return params.next - 1;
}


/*-------------------------------------------
 | DFS using an array of structs (instead of
 | a series of arrays)
 *------------------------------------------*/

/*
void DominatorGraph::rpreDFSp (int v, int *label2pre, SLTData *data, int &next) {
	int *p, *stop, pre_v;
	pre_v = next;
	data[next].pre2label = v;
	label2pre[v] = next++;

	getOutBounds(v,p,stop);
	for (; p<stop; p++) { //visit all outgoing neighbors
		if (!label2pre[*p]) {
			data[next].parent = pre_v;
			rpreDFSp(*p,label2pre, data, next);
		}
	}
}

int DominatorGraph::preDFSp (int v, int *label2pre, SLTData *data) {
	int next = 1;
	for (int w=n; w>=0; w--) label2pre[w] = 0; //everybody unvisited
	rpreDFSp (v, label2pre, data, next); //visit everybody reachable from the root
	return next - 1;
}
*/






/*----------------------------------
 | post-dfs, does not store parents
 *---------------------------------*/

void DominatorGraph::rpostDFS (int v, PostDFSParams &params) {
	int *p, *stop;
	params.label2post[v] = -1;

	getOutBounds (v, p, stop);
	for (; p<stop; p++) {
		if (!params.label2post[*p]) rpostDFS (*p, params);
	}
	params.post2label[params.next] = v;
	params.label2post[v] = params.next++;
}

int DominatorGraph::postDFS (int v, int *label2post, int *post2label) {
	PostDFSParams params;
	params.label2post = label2post;
	params.post2label = post2label;
	params.next = 1;

	for (int w=n; w>=0; w--) params.label2post[w] = 0;
	rpostDFS (v, params);
	return params.next-1;
}

/*--------------------------------------------
 | pre-bfs with parents
 | NOTE: parent is indexed by pre_id numbers
 *-------------------------------------------*/

int DominatorGraph::preBFSp (int r, int *label2pre, int *pre2label, int *parent) {
	int v, first, last;
		
	for (v=n; v>0; v--) label2pre[v] = 0; //everybody unreachable

	//process first vertex
	parent[1] = 1;    //mark as self-parent
	label2pre[r] = 1; //will be used to keep marks
	pre2label[1] = r; //will be used as a queue
	first = last = 1;

	//process the other vertices
	while (first<=last) { //while queue not empty
		int *p, *stop;
		getOutBounds(pre2label[first], p, stop); //v = pre2label[first]
		for (; p<stop; p++) { //scan first element in the queue
			int w = *p;
			if (!label2pre[w]) { //neighbor not yet processed
				pre2label[++last] = w; //insert w into the back of the queue
				label2pre[w] = last;   //note its position (pre-order)
				parent[last] = first;  //set pointer to parent
			}
		}
		first ++;
	}

	return last; //number of vertices visited
}

/*----------------------------------------
 | post-dfs (with parents)
 | WARNING: PARENT is a label->label function
 *---------------------------------------*/

void DominatorGraph::rpostDFSp (int v, PostDFSParams &params) {
	int *p, *stop;			
	params.label2post[v] = -1;     //mark node as visited
	getOutBounds (v, p, stop);
	for (; p<stop; p++) { //for each neighbor of v
		if (!params.label2post[*p]) { //if neighbor not yet visited
			params.parent[*p] = v;  //mark v as its parent
			rpostDFSp (*p, params);     //and visit it
		}
	}
	params.post2label[params.next] = v;   //v is the vertex with postid 'next'
	params.label2post[v] = params.next++; //postid of v is next; increment next
}

int DominatorGraph::postDFSp (int v, int *label2post, int *post2label, int *parent) { //int &next) {
	PostDFSParams params;
	params.label2post = label2post;
	params.post2label = post2label;
	params.next = 1;
	params.parent = parent;

	for (int w=n; w>=0; w--) params.label2post[w] = 0;
	rpostDFSp (v, params);

	return params.next - 1;
}





