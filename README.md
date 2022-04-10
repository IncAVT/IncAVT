## **About the Code** ##

The code implements the **Anchored Vertex Tracking in Dynamic Social Networks** based on the **order-based core maintenance algorithm**.

## **File Structure** ##

The code consists of

* **mainavt.cc**: the main program 
* **defs.h**: this file defines some useful macros
* **core.h**: this header file defines the base class for core maintenance algorithm
* **gadget**: this folder contains gadgets for reading files and implements several data structures
    * **disjoint.h, disjoint.cc**: a union-find-delete data structure with constant time deletion complexity
    * **gadget.h, gadget.cc**: for reading data files, the format of which will be explained in details later
    * **heap.h, heap.cc**: a minimum heap
    * **sb_tree.h, sb_tree.cc**: the size-balanced tree
    * **treap.h, treap.cc**: the treap (tree + heap)
* **glist**: this folder contains the code for the order-based core maintenance algorithm
    * incavt.cpp: including the main functions of AVT algorithm 
    * glist.cpp: including the used data stucture for k-core maintenance and functions for k-core computation 


## **Data File Format** ##

The code assumes the input graph is undirected and simple. In gadget/gadget.h, two functions are provided to read temporal graphs and ordinary graphs, respectively.

* For temporal graphs, the file format is as follows:

        n  m

        u_1 v_1 t_1

        u_2 v_2 t_2

        ......

        u_m v_m t_m

That is, the first line of the file specifies the # of vertices (n) and the # of edges (m) in the graph. The following m lines are of the format "u v t", where (u, v) is an edge and "t" is the timestamp. It is required that **t_1 <= t_2 <= ... <= t_m**.

* For ordinary graphs, the file format is same as that of temporal graphs except that no timestamp is required.
        n  m

        u_1 v_1 

        u_2 v_2 

        ......

        u_m v_m 

## **Command** ##

There are five options to run the code.

* **-p**: the path of the initial graph file
* **-A**: the allocated size
* **-T**: the number of dynamic graphs (or graph snapshots) 
```
        the edge insert and edge remove file is indexed by the index of snapshots. 
        All the files with edge insert and edge remove are stored in the directory ./dataset/edge_insert/ and ./dataset/edge_remove/. 
        The file name is formated as graphname_1,graphname_2, ....  

```
* **-F**: the graph file name
* **-K**: the core number 
* -R: the edge remove directory,remember that the graph file name and the file name in this directory should follow the format: graph name_t.txt 
* -I: the edge insert directory,remember that the graph file name and the file name in this directory should follow the format: graph name_t.txt 


**An example command**: ./incavt -p dataset/email-Enron/Email_graph.txt -R dataset/email-Enron/Email_edge_remove/ -I dataset/email-Enron/Email_edge_insert/ -A 1  -F Email_graph -T 4 -K 5

## **Compiling the Code** ##

Use Makefile included and notice that in default, -O2 optimization option is triggered and the code is compiled based on C++11.
