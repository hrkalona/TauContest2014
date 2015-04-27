#ifndef DGRAPH_H
#define DGRAPH_H

#include <stdio.h> 
#include <stdlib.h>
#include <assert.h> 
#include <math.h>

typedef enum
{
	BFS, DFS, SDOM, IBFS, IDFS, LT, SLT, SNCA, METHODS
} Method;

class DominatorGraph
{
private:
	int n;  //number of vertices in the graph
	int narcs;  //number of arcs in the graph
	int onarcs;  //original number of arcs in the graph (before duplicates are removed)
	int source;

#ifdef COUNTOPS
#define incc() {ccount++;}
#define inci() {icount++;}
#define incs() {if(semi[i]==parent[i])scount++;}
#define resetcounters() {icount=scount=ccount=0;}
#else
#define incc() {}
#define inci() {}
#define incs() {}
#define resetcounters() {}
#endif

	//aggregate type for DFS parameters
	typedef struct
	{
		union
		{
			int *label2post;
			int *label2pre;
		};
		union
		{
			int *post2label;
			int *pre2label;
		};
		int next;
		int *parent;
	} DFSParams;

	typedef DFSParams PostDFSParams;
	typedef DFSParams PreDFSParams;

	/*----------------
	 | adjacency list
	 *---------------*/

	//structure used for building the graph
	typedef union
	{
		int value;
		int *ptr;
	} intptr;

	intptr *first_in;  //first_in[v]: pointer to first element in 'in_arcs' representing a neighbor of v
	intptr *first_out;  //first_out[v]: pointer to first element in 'out_arcs' representing a neighbor of v
	int *in_arcs;   //list of incoming arcs (arcs with the same destination are contiguous)
	int *out_arcs;  //list of outgoing arcs (arcs with different destinations are contiguous)

	inline void getOutBounds(int v, int * &start, int * &stop) const
	{
		start = first_out[v].ptr;
		stop = first_out[v + 1].ptr;
	}

	inline void getInBounds(int v, int *&start, int *&stop) const
	{
		start = first_in[v].ptr;
		stop = first_in[v + 1].ptr;
	}

	inline int *getFirstIn(int v) const
	{
		return first_in[v].ptr;
	}
	inline int *getBoundIn(int v) const
	{
		return first_in[v + 1].ptr;
	}

	/*----------------
	 | initialization
	 *---------------*/
	void deleteAll()
	{
		if (first_in)
			delete[] first_in;
		if (first_out)
			delete[] first_out;
		if (in_arcs)
			delete[] in_arcs;
		if (out_arcs)
			delete[] out_arcs;
	}

	void reset()
	{
		icount = scount = ccount = 0;
		in_arcs = out_arcs = NULL;
		first_out = first_in = NULL;
		n = narcs = source = 0;
	}

	inline int log2(int x)
	{
		return ( int ) ceil( log( ( double ) x ) / log( 2.0 ) );
	}

	/*----------------------------------------
	 | Eliminate duplicate arcs:
	 | - Changes both 'first' and 'arcs'.
	 | - assumes 'first' uses the int fields.
	 | - 'arcs' will still be contiguous.
	 *---------------------------------------*/

	void eliminateDuplicates(intptr *first, int *arcs);

	/*---------------------------------------
	 | compress operation: recursive version
	 *--------------------------------------*/

	inline void rcompress(int v, int *ancestor, int *label)
	{
		int t;
		incc();
		if (ancestor[t = ancestor[v]])
		{
			rcompress( t, ancestor, label );  //ancestor[v] does not change
			incc();
			if (label[t] < label[v])
			{
				label[v] = label[t];
			}
			ancestor[v] = ancestor[t];
		}
	}

	/*------------------------------------------------------------
	 | compress used by LT: compresses the path rootF(v)->v in
	 | the link-eval forest
	 *-----------------------------------------------------------*/

	inline void lt_compress(int v, int *ancestor, int *semi, int *label)
	{
		int t;
		incc();
		if (ancestor[t = ancestor[v]])
		{
			lt_compress( t, ancestor, semi, label );  //ancestor[v] does not change
			incc();
			if (semi[label[t]] < semi[label[v]])
			{
				label[v] = label[t];
			}
			ancestor[v] = ancestor[t];
		}
	}

	/*--------------------------------------------------------
	 | compress used by slt: recursive and iterative versions
	 *-------------------------------------------------------*/

	inline void rcompress(int v, int *parent, int *semi, int *label, int c)
	{
		int p;
		incc();
		if ((p = parent[v]) > c)
		{
			rcompress( p, parent, semi, label, c );
			incc();
			if (semi[label[p]] < semi[label[v]])
				label[v] = label[p];
			parent[v] = parent[p];
		}
	}

	/*------------------------------------------------------------
	 | compresses the path rootF(v)->v in the link-eval forest F;
	 | rootF(v) is the root of the tree in F that contains v
	 *-----------------------------------------------------------*/
	inline void rcompress(int v, int *ancestor, int *semi, int *label)
	{
		int t;
		incc();
		if (ancestor[t = ancestor[v]])
		{
			rcompress( t, ancestor, semi, label );  //ancestor[v] does not change
			incc();
			if (semi[label[t]] < semi[label[v]])
			{
				label[v] = label[t];
			}
			ancestor[v] = ancestor[t];
		}
	}

	inline void rcompress(int v, int *parent, int *label, int c)
	{
		incc();
		int p;
		if ((p = parent[v]) > c)
		{
			rcompress( p, parent, label, c );  //does not change parent[v]
			incc();
			if (label[p] < label[v])
				label[v] = label[p];
			parent[v] = parent[p];
		}
	}

	inline void lt_neg_compress(int v, int *ancestor, int *semi, int *label)
	{
		int t;
		incc();
		if (ancestor[t = ancestor[v]] > 0)
		{
			//lt_compress(ancestor[v], ancestor, semi, label);
			incc();
			lt_neg_compress( t, ancestor, semi, label );
			if (semi[label[t]] < semi[label[v]])
			{
				label[v] = label[t];
			}
			ancestor[v] = ancestor[t];
		}
	}

	void lt_neg_link(int v, int w, int *semi, int *label, int *ancestor, int *size);
	int lt_neg_eval(int v, int *ancestor, int *semi, int *label);

	/*-------------------------------------------------------------------
	 | finds the nearest common ancestor of v1 and v2 in the approximate
	 | dominators tree represented by the array dom. First function uses
	 | post-ids, the second uses pre-ids.
	 *------------------------------------------------------------------*/

	inline int intersect(int v1, int v2, int *dom)
	{
		do
		{
			incc();  //outer test
			while (v1 < v2)
			{
				incc();
				v1 = dom[v1];
			}
			incc();  //failed above
			while (v2 < v1)
			{
				incc();
				v2 = dom[v2];
			}
			incc();  //failed above
		} while (v1 != v2);
		return v1;
	}

	inline int preIntersect(int v1, int v2, int *dom)
	{
		do
		{
			incc();
			while (v1 > v2)
			{
				incc();
				v1 = dom[v1];
			}
			incc();
			while (v2 > v1)
			{
				incc();
				v2 = dom[v2];
			}
			incc();
		} while (v1 != v2);
		return v1;
	}

public:
	int ccount;  //comparison counter
	int icount;  //iteration counter
	int scount;  //sdom=parent counter

	inline int getNVertices() const
	{
		return n;
	}
	inline int getNArcs() const
	{
		return narcs;
	}
	inline int getOriginalNArcs() const
	{
		return onarcs;
	}
	inline int getSource() const
	{
		return source;
	}

	/*---------
	 | outputs
	 *--------*/
	void output(FILE *file, bool reverse);
	void outputGraphStatistics(FILE *file);

	/*-----------------------------
	 | initialization / destructor
	 *----------------------------*/
	DominatorGraph()
	{
		reset();
	}
	void buildGraph(int _nvertices, int _narcs, int _source, int *arclist, bool simplify);  //from list of arcs
	void readDimacs(const char *filename, bool reverse, bool simplify);  //from file
	~DominatorGraph()
	{
		deleteAll();
	}

	void destroy()
	{
		deleteAll();
		reset();
	}

	/*---------------------------------
	 | several variants of dfs and bfs
	 *--------------------------------*/
	void rpostDFS(int v, PostDFSParams &params);
	int postDFS(int v, int *label2post, int *post2label);

	void rpostDFSp(int v, PostDFSParams &params);
	int postDFSp(int v, int *label2post, int *post2label, int *parent);

	void rpreDFSp(int v, PreDFSParams &params);
	int preDFSp(int v, int *label2pre, int *pre2label, int *parent);

	int preBFSp(int v, int *label2pre, int *pre2label, int *parent);

	/*------------------
	 | basic algorithms
	 *-----------------*/
	void slt(int r, int *idom);  //former slt_v4
	void lt(int r, int *idom);   //former lt_neg
	void ibfs(int r, int *idom);  //former iter_v3
	void idfs(int r, int *idom);  //former iter_base
	void snca(int r, int *idom);  //former snca_v2

	/*---------------------
	 | baseline algorithms
	 *--------------------*/
	int semi_dominators(int r);
	int run_dfs(int r);
	int run_bfs(int r);

	/*-----------------
	 | link-eval stuff
	 *----------------*/
	int lt_eval(int v, int *ancestor, int *semi, int *label);
	void lt_link(int v, int w, int *semi, int *label, int *ancestor, int *child, int *size);

};

/*inline void run(Method method, DominatorGraph *g, int r, int *idom)
{
	switch (method)
	{
		//main methods
		case IBFS:
			g->ibfs( r, idom );
			break;
		case IDFS:
			g->idfs( r, idom );
			break;
		case SLT:
			g->slt( r, idom );
			break;
		case LT:
			g->lt( r, idom );
			break;
		case SNCA:
			g->snca( r, idom );
			break;

			//auxiliary functions
		case DFS:
			g->run_dfs( r );
			break;
		case BFS:
			g->run_bfs( r );
			break;
		case SDOM:
			g->semi_dominators( r );
			break;

		default:
			break;
	}
}*/

#endif
