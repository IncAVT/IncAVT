#include <unistd.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>
#include<algorithm>
#include <map>

#include "core.h"
#include "defs.h"
#include "gadget/gadget.h"
#include "glist/glist.h"
#include "debug.h"
//#include "traversal/traversal.h"

int main(int argc, char** argv) {
    char path[200];      // path of input graph
    char remove_path[200];      // path of remove edge files
    char insert_path[200];      // path of insert edge files
    int L;      // allocated size
    char graphname[200];    // method
    int T = 0;   // temporal graph
    // initialize the options
    int option = -1;
    int K = 0; // parameter K
    while (-1 != (option = getopt(argc, argv, "p:R:I:A:F:T:K:"))) {
        switch (option) {
            case 'p': // inital graph file path
                strcpy(path, optarg);
                break;
            case 'R': // inital graph file path
                strcpy(remove_path, optarg);
                break;
            case 'I': // inital graph file path
                strcpy(insert_path, optarg);
                break;
            case 'A':// allocated size
                L = atoi(optarg);
                break;
            case 'F': // graph file name
                strcpy(graphname, optarg);
                break;
            case 'T': // graph snapshot number
                T = atoi(optarg);
                break;
            case 'K': // core number
                K = atoi(optarg);
                break;
        }
    }


    const auto init_beg1 = std::chrono::steady_clock::now();
    // read the graph
    int n, m, m2;
    const auto read_func = gadget::ReadEdgesS;
    const auto edges = read_func(path, &n, &m);

    // print the configurations
    gadget::RepeatWith('*', 80);
    printf("# of vertices: %d\n", n);
    printf("# of (all) edges: %d\n", m);
    printf("core number: %d\n", K);
    printf("Allocated size : %d\n", L);
    printf("graph snapshot number: %d\n", T);
    printf("path of the input file: %s\n", path);
    printf("graph file name: %s\n", graphname);
    gadget::RepeatWith('*', 80);

    // initialize the core component
    core::GLIST * cm = nullptr;

    cm = new core::GLIST(n);
    // create the adjacent list representation
    std::vector<std::vector<int>> graph(n);
    for (int i = 0; i < m; ++i) {
        int v1 = edges[i].first;
        int v2 = edges[i].second;
        graph[v1].push_back(v2);
        graph[v2].push_back(v1);
    }

    // compute the base core
    std::vector<int> core(n);
    const auto init_beg = std::chrono::steady_clock::now();
    cm->ComputeCore(graph, true, core);
    const auto init_end = std::chrono::steady_clock::now();
    const auto init_dif = init_end - init_beg;
    printf("initialization costs \x1b[1;31m %f\x1b[0m ms\n",
            std::chrono::duration<double, std::milli>(init_dif).count());
    // verify the result
    {
        cm->Check(graph, core);
        // ASSERT(false);
    }


    // anchored vertex sets
    std::vector<std::vector<int>> S(T+1);
    std::vector<int> S_follower(T+1,0);
    std::vector<int> anchored_visited(T+1,0);

    int i=0;
    int t=0;

    int F_max_counter=0;
    std::vector<int> insert_node1;
    std::vector<int> insert_node2;

    while(i<L)
    {

    std::vector<int> candidates; // C_{k-1} U nbr(C_{k-1},G_t) \ {S_t U C_k(G_t)}
    std::vector<int> candidates_final; // C_{k-1} U nbr(C_{k-1},G_t) \ {S_t U C_k(G_t)}
    std::vector<bool> core_k_list= std::vector<bool>(n, false); // store the nodes in order k.

    for (int cur = cm->head_[K]; n != cur;)
    {
        core_k_list[cur]=true;
        cur = cm->node_[cur].next;
    }

    for (int cur = cm->head_[K-1]; n != cur;)
    {
        candidates.push_back(cur);
        for (int j : graph[cur])
        {
            if (!core_k_list[j])
            {
                candidates.push_back(j);
            }
        }
        cur = cm->node_[cur].next;
    }

    for (int u:candidates)

    {

        int flag=0;
        for (int v:graph[u])
        {
            if (core[v]==K-1 && cm->tree_.Rank(u)<=cm->tree_.Rank(v)) // theorem 3
            {
                flag=1;
                break;
            }
        }
        if (flag==1)
        {
            candidates_final.push_back(u);
        }
    }

    int F_max = 0;
    int u_ = -1 ;
    int Final_max = 0;

    for (auto u:candidates_final) // line 7
    {

        anchored_visited[t]+=1;
        if (!S[t].empty() && std::find(S[t].begin(), S[t].end(), u) != S[t].end())  // A2 line 7
        {
            continue;
        }
        std::vector<int> follower_u; // line 10
        cm->ComputeFollower(u,graph,core,follower_u,K);
        //if (F_max < follower_u.size())
        if (F_max < follower_u.size() && (std::find(S[t].begin(), S[t].end(),u) == S[t].end()))
        {
            F_max = follower_u.size();
            u_ = u;

            for(auto f:follower_u)
            {
                DEBUG_PRINT("candidate follower:%d core %d\n",f,core[f]);
            }
        }
    }

    F_max_counter+=F_max;
    if (u_!=-1)
    {
        fflush(stdout);
        DEBUG_PRINT("anchored u_:%d\n",u_);
        cm->AnchoredKorder1(u_,graph,core,K,insert_node1,insert_node2);// u_ is the best anchored vertext in the iteration of i
        DEBUG_PRINT("finish anchored u_:%d\n",u_);
        fflush(stdout);
        S[t].push_back(u_);
    }
    i+=1;
    }

    for (int i=0;i<insert_node1.size();i++)
    {

        cm->Remove(insert_node1[i],insert_node2[i],graph,core);
        DEBUG_PRINT("remove edge: %d,%d\n",insert_node1[i],insert_node2[i]);
        fflush(stdout);
    }



    // remove anchor edges:


    S_follower[t]=F_max_counter;

    DEBUG_PRINT("check after greey\n");
    cm->Check(graph,core);
    fflush(stdout);
    int nn,mm;
    // read the edge insert and remove at different time
    for (int t=1;t<=T;t++)
    {
        char edge_insert[400];
        char edge_remove[400];

        sprintf(edge_insert, "%s/%s_%d.txt",insert_path,graphname,t);
        sprintf(edge_remove, "%s/%s_%d.txt", remove_path,graphname,t);

        printf("read edge insert from path: %s\n",edge_insert);
        printf("read edge remove from path: %s\n",edge_remove);

        fflush(stdout);
        std::vector<std::pair<int, int>> insert_edges = read_func(edge_insert, &nn, &mm);
        std::vector<std::pair<int, int>> remove_edges = read_func(edge_remove, &nn, &mm);

        //line 5
        for (auto u : S[t-1])
        {
            S[t].push_back(u); // also line 5
        }

        // line 6-8
        std::vector<bool> evicted_vi = std::vector<bool>(n, false);
        std::vector<bool> evicted_vr = std::vector<bool>(n, false);

        cm->EdgeInsert(insert_edges,graph,core,K,evicted_vi);
        DEBUG_PRINT("check after EdgeInsert\n");
        cm->Check(graph,core);
        DEBUG_PRINT("finishing all edges insert\n");
        fflush(stdout);

        cm->EdgeRemove(remove_edges,graph,core,K,evicted_vr);
        DEBUG_PRINT("check after EdgeRemove\n");
        cm->Check(graph,core);
        DEBUG_PRINT("finishing all edges remove\n");

        fflush(stdout);

        // line 9-16

        std::vector<bool> k_order_list= std::vector<bool>(n, false); // store the nodes in order k.

        for (int cur = cm->head_[K]; n != cur;)
        {
            if(cur!=-1)
            {
                k_order_list[cur]=true;
                cur = cm->node_[cur].next;
            }
            else
            {
                break;
            }
        }

        std::vector<int> candidates; // (VI) U (VR) U (neighbours of VI and VR) - (O_k)

        printf("evicted_vi size: %d\n", evicted_vi.size());
        printf("evicted_vr size: %d\n", evicted_vr.size());
        for (int i=0;i<n;i++)
        {
            if (evicted_vi[i] && k_order_list[i]==false)
            {
                candidates.push_back(i);
                for (int j : graph[i]) {
                    if (k_order_list[j]==false)
                    {
                        candidates.push_back(j);
                    }
                }

            }
            if (evicted_vr[i] && k_order_list[i]==false)
            {
                candidates.push_back(i);
                for (int j : graph[i]) {
                    if (k_order_list[j]==false)
                    {
                        candidates.push_back(j);
                    }
                }

            }
        }

        printf("candiate size: %d\n", candidates.size());
        std::vector<int> candidates_final; // theorem 3
        for (int u:candidates)
        {

            int flag=0;
            for (int v:graph[u])
            {
                if (core[v]==K-1 && cm->tree_.Rank(u)<=cm->tree_.Rank(v)) // theorem 3
                {
                    flag=1;
                    break;
                }
            }
            if (flag==1)
            {
                candidates_final.push_back(u);
            }
        }

        printf("finishing candidates final compute: size %d\n",candidates_final.size());
        fflush(stdout);

        //line 9
        F_max_counter=0;
        for (auto u : S[t-1])
        {
            //line 10 compute follower for S[t]
            int F = 0;
            std::vector<int> follower_st ;
            std::vector<std::vector<int>> s_t(n);
            for (auto uu : S[t])
            {
                std::vector<int> follower_uu ;

                DEBUG_PRINT("compute follower for node %d\n",uu);
                cm->ComputeFollower(uu,graph,core,follower_uu,K);
                //DEBUG_PRINT("check compute follower for node %d\n",uu);
                //cm->Check(graph,core);
                for (auto v:follower_uu)
                {
                    follower_st.push_back(v);
                    s_t[uu].push_back(v);
                }
            }

            F = follower_st.size();

            // line 11
            int F_max = 0;
            int u_ = u ;
            for (auto v:candidates_final) // line 12
            {

                anchored_visited[t]+=1;
                std::vector<int> follower_u_v;// line 13 follwer of St U V - u
                for (auto uu : S[t])
                {

                    if (uu!=u)
                    {
                        //std::vector<int> follower_uu ;
                        //DEBUG_PRINT("compute follower for node %d\n",uu);
                        //cm->ComputeFollower(uu,graph,core,follower_uu,K);
                        //DEBUG_PRINT("check compute follower for node %d\n",uu);
                        //cm->Check(graph,core);
                        //for (auto vv:follower_uu)
                        for (auto vv:s_t[uu])
                        {
                            follower_u_v.push_back(vv);
                        }

                    }
                }

                std::vector<int> follower_v ;
                DEBUG_PRINT("compute follower for node %d\n",v);
                cm->ComputeFollower(v,graph,core,follower_v,K);
                //DEBUG_PRINT("check compute follower for node %d\n",v);
                //cm->Check(graph,core);
                for (auto vv:follower_v)
                {
                    follower_u_v.push_back(vv);
                }

                // line 13-14
                //if (F_max < follower_u_v.size())
                if (F_max < follower_u_v.size() && (std::find(S[t].begin(), S[t].end(),v) == S[t].end()))
                {
                    F_max = follower_u_v.size();
                    u_ = v;
                }
            }

            // line 15-16
            if (F_max > F)
            {
                S[t].erase(std::find(S[t].begin(), S[t].end(), u));
                S[t].push_back(u_);
                //Final_max +=F_max;
            }

            F_max_counter+=F_max;
        }
        printf("F_max_counter: %d\n",F_max_counter);
        if(F_max_counter>0)
        {
            S_follower[t]=F_max_counter;
        }
        else
        {
            int F = 0;
            std::vector<int> follower_st ;
            std::vector<std::vector<int>> s_t(n);
            for (auto uu : S[t])
            {
                std::vector<int> follower_uu ;

                DEBUG_PRINT("compute follower for node %d\n",uu);
                cm->ComputeFollower(uu,graph,core,follower_uu,K);
                //DEBUG_PRINT("check compute follower for node %d\n",uu);
                //cm->Check(graph,core);
                for (auto v:follower_uu)
                {
                    follower_st.push_back(v);
                    s_t[uu].push_back(v);
                }
            }
            printf("follower size: %d\n",follower_st.size());
            F = follower_st.size();
            S_follower[t]=F;
        }

        DEBUG_PRINT("finishing candidates searching: size \n");
        fflush(stdout);
    }

    fflush(stdout);
    for (int t=0;t<=T;t++)
    {
        //printf("Anchored vertex in time %d:\t",t);
        printf("Anchored vertex in time %d total followers %d visited anchored candidates %d \t anchored noded:",t,S_follower[t],anchored_visited[t]);
        for (auto v:S[t])
        {
            printf("%d\t",v);
        }
        printf("\n");
    }

    fflush(stdout);
    delete cm;
    const auto init_end1 = std::chrono::steady_clock::now();
    const auto init_dif1 = init_end1 - init_beg1;
    printf("initialization costs \x1b[1;31m %f\x1b[0m ms\n",
            std::chrono::duration<double, std::milli>(init_dif1).count());
    printf("total costs %f ms\n",
            std::chrono::duration<double, std::milli>(init_dif1).count());
}
