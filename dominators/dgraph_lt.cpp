/*****************************
 * 
 * LENGAUER-TARJAN
 *
 *****************************/

 /* Near-linear-time implementation of the Lengauer-Tarjan 
   dominators algorithm; the calculations are reordered so 
   that each bucket is processed exactly once. 

   Alse uses child[v] = -ancestor[v]; so if v is a (sub)tree 
   root then ancestor[v]<=0 */ 

#include "dgraph.h"

int DominatorGraph::lt_neg_eval (int v, int *ancestor, int *semi, int *label) {
	incc();
	if (ancestor[v] <= 0) return label[v];
	else {
	    lt_neg_compress(v,ancestor,semi,label); // *neg*
		int lv = label[v];            //v's label
		int lav = label[ancestor[v]]; //ancestor's label
		incc();
		return label [semi[lav]>=semi[lv] ? lv : lav]; //return label with smallest sdom
    }
}

/*----------------------------------------------------
 | v becomes the parent of w in the link-eval forrest 
 *---------------------------------------------------*/

void DominatorGraph::lt_neg_link(int v, int w, int *semi, int *label, int *ancestor, int *size) {
	int s = w;
	int t = -ancestor[s];

	/* join subtrees with semis greater than semi[label[w]] */
	while (semi[label[w]] < semi[label[t]]) {
		incc();
		/* union by size */
		if (size[s]+size[-ancestor[t]] >= 2*size[t]) {
			int c = ancestor[t];
			ancestor[t] = s;
			ancestor[s] = c;
			t = -c;
		} else {
			size[t] = size[s];
			t = -ancestor[s=ancestor[s]=t];
		}
	}
	incc(); //for the failure

	label[s] = label[w];

    /* union by size */
	if (size[v]<size[w]) {
		//swap s and child[v]
		int t = -ancestor[v]; 
		ancestor[v] = -s;
		s = t;
	}
	size[v] += size[w];

	/* make v the ancestor of the subtrees of s */
	while (s) {
		incc();
		int t = -ancestor[s];
		ancestor[s] = v;
		s = t;
	}
	incc(); //for the failure
}


void DominatorGraph::lt(int r, int *idom) {
	int bsize = n+1;
	int *buffer    = new int [8*bsize];
	int *pre2label = &buffer[0];
	int *parent    = &buffer[bsize];
	int *ancestor  = &buffer[2*bsize];
	int *semi      = &buffer[3*bsize];
	int *label     = &buffer[4*bsize];
	int *size      = &buffer[5*bsize];
	int *dom       = &buffer[6*bsize];
	int *ubucket   = &buffer[7*bsize];

	int *label2pre = idom;

	resetcounters();

	/*----------------
	 | initialization 
	 *---------------*/
	int i;
	for (i=n; i>=0; i--) {
		label[i] = semi[i] = i;
		ubucket[i] = ancestor[i] = 0;
		size[i] = 1;
	}
	
	//get pre-ids and initialize parents
	int N = preDFSp (r, label2pre, pre2label, parent);

	/*---------------------------------------
	 | process vertices in reverse pre-order
	 *--------------------------------------*/
	for (i=N; i>=2; i--) {

		/*---------------------
		 | process i-th bucket 
		 *--------------------*/
		for (int v=ubucket[i]; v; v=ubucket[v]) { //for each element in the bucket...
			int u = lt_neg_eval (v, ancestor, semi, label);
			incc();
			dom[v] = (semi[u] < semi[v]) ? u : i;
		}

		/*--------------------
		 | scan incoming arcs
		 *-------------------*/
		int *p, *stop;
		getInBounds (pre2label[i], p, stop);

		for (; p<stop; p++) {
			int v = label2pre[*p];
			incc();
			if (v) {
				int u = lt_neg_eval (v, ancestor, semi, label);
				incc();
				if (semi[u] < semi[i]) semi[i] = semi[u];
			}
		}

		/*----------------------------------------------
		 | either set i's dominator or add it to bucket
		 *---------------------------------------------*/
		int s = semi[i];
		incc();
		if (s!=parent[i]) { 
			ubucket[i] = ubucket[s]; //i will be first in bucket s
			ubucket[s] = i;        
		} else dom[i] = s;

		//link i to its parent
		lt_neg_link (parent[i], i, semi, label, ancestor, size);
	}

	/*--------------------------
	 | process the first bucket 
	 *-------------------------*/
	for (int v=ubucket[1]; v; v=ubucket[v]) dom[v] = 1;

	/*-----------
	 | get idoms
	 *----------*/
	dom[1] = 1;
	idom[r] = r;

	for (i=2; i<=N; i++) {
		incc();
		if (dom[i]!=semi[i]) dom[i] = dom[dom[i]]; //make relative absolute
		idom[pre2label[i]] = pre2label[dom[i]];
	}

	delete [] buffer;
}

