#include "glist/glist.h"
#include "debug.h"

#include <algorithm>
#include <cstring>
#include <queue>
#include <set>
#include <map>

// #include <google/dense_hash_set>

#include "defs.h"

namespace core {
    GLIST::GLIST(const int n): n_(n), tree_(n_), heap_(n_) {
        head_ = std::vector<int>(n_, -1);
        tail_ = std::vector<int>(n_, -1);
        node_ = std::vector<ListNode>(n_ + 1);
        mcd_  = std::vector<int>(n_, 0);
        deg_  = std::vector<int>(n_, 0);
        rank_ = std::vector<int>(n_, 0);
        root_ = std::vector<int>(n_, n_);
        visited_ = std::vector<bool>(n_, false);
        evicted_ = std::vector<bool>(n_, false);
    }

    GLIST::~GLIST() {}
    void GLIST::ComputeCore(const std::vector<std::vector<int>>& graph,
            const bool init_idx,
            std::vector<int>& core) {
        // compute the cores
        auto& deg = core;
        int max_deg = 0;
        for (int i = 0; i < n_; ++i) {
            deg[i] = graph[i].size();
            if (deg[i] > max_deg) {
                max_deg = deg[i];
            }
        }
        std::vector<int> bin(max_deg + 1, 0);
        for (int i = 0; i < n_; ++i) {
            ++bin[deg[i]];
        }
        int start = 0;
        for (int i = 0; i <= max_deg; ++i) {
            int temp = bin[i];
            bin[i] = start;//bin[i] is the number of accmulated values from bin[0] to bin[i-1], then the value of  bin[i] is acctually the started index of degree i.
            start += temp;// does bin[max_deg] doesnot contains the accumulated value?
        }
        std::vector<int> vert(n_);
        std::vector<int> pos(n_);
        for (int i = 0; i < n_; ++i) {
            pos[i] = bin[deg[i]];
            vert[pos[i]] = i;
            ++bin[deg[i]];
        }
        for (int i = max_deg; i > 0; --i) {
            bin[i] = bin[i-1];//?
        }
        bin[0] = 0;
        int k = 0;
        auto vis = std::vector<bool>(n_, false);
        for (int i = 0; i < n_; ++i) {
            const int v = vert[i];
            if (deg[v] > k) k = deg[v];
            ASSERT(bin[deg[v]] == i);
            ++bin[deg[v]];
            core[v] = k;
            vis[v] = true;
            int rem = 0;
            for (const int u : graph[v]) {
                if (vis[u]) continue;
                ++rem;
                const int pw = bin[deg[u]];
                const int pu = pos[u];
                if (pw != pu) {
                    const int w = vert[pw];
                    vert[pu] = w;
                    pos[w] = pu;
                    vert[pw] = u;
                    pos[u] = pw;
                }
                ++bin[deg[u]];
                --deg[u];
                if (pos[u] == i + 1) {
                    bin[deg[u]] = pos[u];
                }
            }
            if (init_idx) {
                node_[v].rem = rem;
                if (head_[k] == -1) {
                    node_[v].prev = node_[v].next = n_;
                    head_[k] = tail_[k] = v;
                } else {
                    node_[v].next = n_;
                    node_[v].prev = tail_[k];
                    node_[tail_[k]].next = v;
                    tail_[k] = v;
                }
                //
                tree_.Insert(v, false, root_[k]);
            }
        }
        if (init_idx) {
            for (int v = 0; v < n_; ++v) {
                mcd_[v] = 0;
                for (const int u : graph[v]) {
                    if (core[u] >= core[v]) {
                        ++mcd_[v];
                    }
                }
            }
        }
    }
    void GLIST::EdgeInsert(std::vector<std::pair<int, int>> & edges,
            std::vector<std::vector<int>>& graph,
            std::vector<int>& core,const int K,
            std::vector<bool>& evicted_vi) {
        // Algorithm 4: line 1-line 7
        //evicted_vi = std::vector<bool>(n_, false);

        // initialize deg-/ext as zero

        //for (int i=0;i<n_;i++)
        //{
        //node_[i].ext=0;
        //}

        int m = 0;
        int num_e = edges.size();

        for (int v = 0;v<n_;v++ ){
            if (m<core[v]){
                m = core[v];
            }
        }

        std::vector<int> node_rem_change;// used to record the nodes have deg+ increasing
        for (int i = 0; i < num_e; ++i) {
            int v1 = edges[i].first;
            int v2 = edges[i].second;

            int temp_m = core[v1] >= core[v2]? core[v1]: core[v2];
            //if(temp_m==0) continue; // if v1 or v2 has zero k-core, then not adding this edge?

            graph[v1].push_back(v2);
            graph[v2].push_back(v1);

            if (v1==9)
            {
                for (int u:graph[v1])
                {
                    DEBUG_PRINT("node 9 neighbour [insert]: node %d\n",u);
                }

            }

            //m = m<temp_m ? temp_m : m;

            //line 4, alg 4
            if ((core[v2]>core[v1]) || (core[v2]==core[v1] && GetRank(v1) <= GetRank(v2)))//pseudo code need to be updated.
            {
                ++node_[v1].rem;
                node_rem_change.push_back(v1);
            }
            else
            {
                ++node_[v2].rem;
                node_rem_change.push_back(v2);
            }
            DEBUG_PRINT("finishing insert edge %d,%d,rem: v1 %d, v2 %d \n",v1,v2,node_[v1].rem,node_[v2].rem);
            fflush(stdout);
        }
        //m = m< K ? K: m;
        m = 200;

        //m=11;
        DEBUG_PRINT("finishing insert multi-edges\n");
        DEBUG_PRINT("maximum core number %d\n",m);


        for (int t=0;t<=m;t++)
        {
            //
            DEBUG_PRINT("EdgeInsert processing core list %d\n",t);
            int list_h = -1, list_t = -1;
            std::vector<int> VC;// line 9, does the deg-(i.e.,ext) is zero?
            std::vector<int> swap;// used to record removed node in VC


            //evicted_ = std::vector<bool>(n_, false);


            int src_o_k_1 = -1;
            //std::vector<int> ok1;

            std::queue<int> vc_queue;
            int counter_o_k_original=0;

            if (head_[t]==-1 &&t==0)
            {
                continue;
            }

            if (head_[t]==-1 && t>0)
            {
                t=m+1;
                continue;
            }

            if(t>0)
            {

            for (int cur = head_[t]; n_ != cur;)
            {

                counter_o_k_original+=1;
                DEBUG_PRINT("ok check %d, rank %d, %d\n", cur,GetRank(cur),counter_o_k_original);
                ASSERT(GetRank(cur)==counter_o_k_original);
                cur= node_[cur].next;
            }
            }

            counter_o_k_original=0;
            // line 11
            int ext_increase_counter=0;
            for (int cur = head_[t]; n_ != cur;)
            {
                const int next = node_[cur].next;
                counter_o_k_original+=1;
                // store the next node in head_[t]

                if ((node_[cur].rem + node_[cur].ext)>t) //Algorithm 4: line 12
                {
                    // remove cur from the current k-order list
                    // do we need to consider some special situations, like only one node in O_t?
                    // pseudo code need to remove cur from o(i), but implementation does not require
                    //node_[node_[cur].prev].next = node_[cur].next;
                    //node_[node_[cur].next].prev = node_[cur].prev;

                    // append cur to Vc

                    VC.push_back(cur);
                    evicted_[cur]=true;
                    vc_queue.push(cur);

                    //if (src_o_k_1==-1)
                    //{
                    //// src_o_k_1 could be remove again;
                    //src_o_k_1 = cur;
                    //}


                    if (t==(K-1)) // Algorithm 4: line 14-15
                    {
                        evicted_vi[cur]=true;// vector or
                        //VI.push_back(cur);
                    }
                    // Algorithm 4: line 16-17
                    for (const auto u : graph[cur]) {
                        if (core[u] == t && GetRank(u) >= GetRank(cur)) {
                            ++node_[u].ext;
                            ext_increase_counter+=1;
                        }
                    }
                }
                // line 18
                else
                {
                    if (node_[cur].ext==0) // line 19
                    {
                        // remove cur from the current k-order list
                        // Algorithm 4: line 20
                        node_[node_[cur].prev].next = node_[cur].next;
                        node_[node_[cur].next].prev = node_[cur].prev;
                        //apend cur to the new k-order list
                        if (-1 == list_h) {
                            list_h  = cur;
                            list_t = cur;
                            node_[cur].prev = node_[cur].next = n_;
                        }
                        else {
                            node_[cur].next=n_;
                            node_[cur].prev = list_t;
                            node_[list_t].next = cur;
                            list_t = cur;
                        }

                        //for (int deg_start = list_h;deg_start!=n_;)
                        //{
                        ////DEBUG_PRINT("node  %d \n", deg_start);
                        //ASSERT(node_[deg_start].ext==0);
                        //deg_start = node_[deg_start].next;
                        //}
                    }
                    else
                    {
                        // remove cur from the current k-order list
                        // Algorithm 4: line 22
                        node_[node_[cur].prev].next = node_[cur].next;
                        node_[node_[cur].next].prev = node_[cur].prev;
                        //apend cur to the new k-order list
                        if (-1 == list_h) {
                            list_h = list_t = cur;
                            node_[cur].prev = node_[cur].next = n_;
                        }
                        else {
                            node_[cur].next=n_;
                            node_[cur].prev = list_t;
                            node_[list_t].next = cur;
                            list_t = cur;
                        }
                        // line 23-24
                        node_[cur].rem += node_[cur].ext;
                        node_[cur].ext = 0;
                        // case-2b, line 25, VC is the swap variable in glist
                        Keep_customed(graph, cur, t, core, list_t, swap);//there is a heap variable in Keep function, we should remove it?/// list_t or list_h?, K has some problem?, evicted no use?
                        for (int deg_start = list_h;deg_start!=n_;)
                        {
                            ASSERT(node_[deg_start].ext==0);
                            deg_start = node_[deg_start].next;
                        }
                    }
                }
                cur=next;
                DEBUG_PRINT("processing next node %d ,ext %d\n",cur, node_[cur].ext);
                fflush(stdout);
            }

            // line 27

            while(!vc_queue.empty())
            {
                const int first_node= vc_queue.front(); vc_queue.pop();
                //first_node = vc_queue.pop();
                if(evicted_[first_node] && src_o_k_1==-1)
                {
                    src_o_k_1 = first_node;
                }
            }



            if (src_o_k_1!=-1)
            {
                ASSERT(evicted_[src_o_k_1]==true );
            }

            int vc_counter=0;
            for (const int v : VC) {
                if (evicted_[v])// this is equal to evicted_
                {

                    vc_counter+=1;
                    //tree_.Delete(v, root_[K]);//
                    //tree_.InsertAfter(v, node_[v].prev, root_[K]); //?
                    //line 28
                    node_[v].ext=0;
                    //++core[v];
                    //line 26-27
                    if (t==(K-1))
                    {
                        evicted_vi[v]=false;
                    }
                }
            }


            //line 34
            head_[t] = list_h;
            tail_[t] = list_t;

            //Tree_ operation done in glist.cpp insert function. do we need it here?
            DEBUG_PRINT("swap size: %d\n",swap.size());
            fflush(stdout);


            for (const int v : swap) {
                DEBUG_PRINT("before swap node  %d, rank %d\n",v,GetRank(v));
                tree_.Delete(v, root_[t]);// K or t?
                tree_.InsertAfter(v, node_[v].prev, root_[t]); //?
                DEBUG_PRINT("after swap node  %d, rank %d\n",v,GetRank(v));

            }
            //line 28
            if ( src_o_k_1 !=-1 && evicted_[src_o_k_1] ) {//if src is not true, then all its remaining degree will not be able to be processed.
                auto tail = -1; // tail
                int link_counter=0;
                for (auto v = src_o_k_1; n_ != v; v = node_[v].next) {
                    link_counter+=1;
                    ++core[v]; // Algorithm 2->29
                    node_[v].ext = 0; // Algorithm 2->28
                    tail = v;
                    // update mcd
                    //for (const auto u : graph[v]) {
                    //if (evicted_[u]) continue;
                    //if (K + 1 == core[u]) {
                    //++mcd_[u];
                    //} else if (K == core[u]) {
                    //--mcd_[v];
                    //}
                    //}
                    // remove from the current tree
                    tree_.Delete(v, root_[t]);
                }



                ASSERT(link_counter==vc_counter);
                int end_start_counter = 0;
                for (auto v = tail; n_ != v; v = node_[v].prev) {
                    evicted_[v] = false;
                    tree_.Insert(v, true, root_[t + 1]);
                    end_start_counter +=1;
                }

                ASSERT(link_counter == end_start_counter);
                // merge list
                if (-1 == head_[t + 1]) { //232 to 239 corresponds to 28 in Algorithm 4.
                    head_[t + 1] = src_o_k_1;
                    tail_[t + 1] = tail;
                } else {
                    node_[head_[t + 1]].prev = tail;
                    node_[tail].next = head_[t + 1];
                    head_[t + 1] = src_o_k_1;
                }
            }

            ///


            for (const int v: node_rem_change)
            {

                node_[v].rem = 0;
                for (const auto u : graph[v]) {
                    if (core[u]>core[v])
                    {
                        node_[v].rem +=1;
                    }
                    if (core[u]==core[v] &&tree_.Rank(u)>=tree_.Rank(v))
                    {
                        node_[v].rem +=1;
                    }
                }
                DEBUG_PRINT("deg+ check: node  %d, core %d, deg+ %d\n",v,core[v],node_[v].rem);
            }

            for (const int v:VC)
            {

                node_[v].rem = 0;
                for (const auto u : graph[v]) {
                    if (core[u]>core[v])
                    {
                        node_[v].rem +=1;
                    }
                    if (core[u]==core[v] &&tree_.Rank(u)>=tree_.Rank(v))
                    {
                        node_[v].rem +=1;
                    }
                }
                DEBUG_PRINT("deg+ check: node  %d, core %d, deg+ %d\n",v,core[v],node_[v].rem);
            }

            // line 29-30
            if (t==(K-2))
            {
                for (const int v : VC)
                {
                    if (evicted_[v])
                    {
                        evicted_vi[v]=true;
                    }
                    evicted_[v]=false;
                }
            }

            DEBUG_PRINT("update tree\n");
            fflush(stdout);

            DEBUG_PRINT("core %d\n",t);

            int counter_o_k_2=0;

            for (const int g : garbage_) rank_[g] = 0;
            garbage_.clear();
        }

        DEBUG_PRINT("EdgeInsert core list finished\n");


        std::vector<bool> vis(n_, false);
        for (int v = 0; v < n_; ++v) {
            if (vis[v]) continue;
            int t=core[v];
            for (int tmp = head_[t]; n_ != tmp; tmp = node_[tmp].next) {
                ASSERT(!vis[tmp]);
                vis[tmp]=true;
                ASSERT(visited_[tmp] == 0);
                ASSERT(core[tmp] == t);

                int local = 0;
                for (const auto u : graph[tmp]) {
                    if (core[u] > core[tmp] ||
                            (core[u] == core[tmp] &&
                             tree_.Rank(u) > tree_.Rank(tmp))) {
                        ++local;
                    }
                }
                DEBUG_PRINT("EdgeInsert core %d,  node %d, core %d, deg- %d, deg+ %d,local deg+ %d\n",t,tmp,core[tmp],node_[tmp].ext,node_[tmp].rem,local);
                ASSERT(local == node_[tmp].rem);
                ASSERT(node_[tmp].rem <= t);
            }
        }
    }

    void GLIST::ComputeFollower_bruteforce(const int u,const int v,
            std::vector<std::vector<int>>& graph,
            std::vector<int>& core, std::vector<int>& follower,int K)
    {

        std::vector<int> insert_record;

        int new_neighbour_num = K - core[u];
        int new_neighbour_num_v = K - core[v];
        DEBUG_PRINT("Insert new neighbour for nodes  :%d %d, neighbour number:%d %d\n",u,v, new_neighbour_num,new_neighbour_num_v);
        //if (new_neighbour_num<=0) return;
        //if (new_neighbour_num_v<=0) return;
        int count = 0;
        //int current_core = K+ 1; // head[K+1] can be empty
        int current_core = K; // head[K+1] can be empty

        //for(int cur=0;cur<n_;cur++)
        //{
            //node_[cur].ext=0;
        //}

        std::vector<bool> old_core_k = std::vector<bool>(n_, false); // record the old k core list

        for (int cur = head_[K]; n_ != cur; )
        {
            if(cur==-1)
            {
                break;
            }
            old_core_k[cur]=true;
            cur = node_[cur].next;
        }
        for (int cur = head_[K+1]; n_ != cur; )
        {
            if(cur==-1)
            {
                break;
            }
            old_core_k[cur]=true;
            cur = node_[cur].next;
        }

        DEBUG_PRINT("ComputeFollower node :%d, core number:%d\n",u,core[u]);

        while(count<new_neighbour_num)
        {

            if(head_[current_core]==-1)
            {
                DEBUG_PRINT("ComputeFollower, core list is empty:%d\n",current_core);
                current_core+=1;
                continue;
            }
            for (int cur = head_[current_core];n_ !=cur;)
            {
                if (count<new_neighbour_num)
                {

                    if (std::find(graph[u].begin(),graph[u].end(),cur)==graph[u].end())
                    {
                        DEBUG_PRINT("insert edge: %d,%d\n",u,cur);
                        insert_record.push_back(cur);
                        count+=1;
                        DEBUG_PRINT("ComputeFollower finished insert edge: %d,%d\n",u,cur);
                        //DEBUG_PRINT("next node:  %d\n",node_[cur].next);
                    }
                    cur = node_[cur].next;
                }
                else
                {
                    DEBUG_PRINT("find all neighbour for %d\n",u);
                    break;
                }
            }

            if(count<new_neighbour_num)
            {
                current_core+=1;
                DEBUG_PRINT("current core %d\n",current_core);
            }
        }
        for (auto uu : insert_record)
        {
            graph[uu].push_back(u);
            graph[u].push_back(uu);
            DEBUG_PRINT("insert edge: %d,%d\n",u,uu);
            DEBUG_PRINT("insert edge: %d,%d\n",uu,u);

        }

        current_core = K;
        count = 0;

        std::vector<int> insert_record_v;
        while(count<new_neighbour_num_v)
        {

            if(head_[current_core]==-1)
            {
                DEBUG_PRINT("ComputeFollower, core list is empty:%d\n",current_core);
                current_core+=1;
                continue;
            }
            for (int cur = head_[current_core];n_ !=cur;)
            {
                if (count<new_neighbour_num_v)
                {

                    if (std::find(graph[v].begin(),graph[v].end(),cur)==graph[v].end())
                    {
                        DEBUG_PRINT("insert edge: %d,%d\n",v,cur);
                        insert_record_v.push_back(cur);
                        count+=1;
                        DEBUG_PRINT("ComputeFollower finished insert edge: %d,%d\n",u,cur);
                        //DEBUG_PRINT("next node:  %d\n",node_[cur].next);
                    }
                    cur = node_[cur].next;
                }
                else
                {
                    DEBUG_PRINT("find all neighbour for %d\n",v);
                    break;
                }
            }

            if(count<new_neighbour_num_v)
            {
                current_core+=1;
                DEBUG_PRINT("current core %d\n",current_core);
            }
        }

        for (auto vv : insert_record_v)
        {
            graph[v].push_back(vv);
            graph[vv].push_back(v);
            DEBUG_PRINT("insert edge: %d,%d\n",vv,v);
            DEBUG_PRINT("insert edge: %d,%d\n",v,vv);

        }

        GLIST * new_cm= nullptr;
        new_cm = new GLIST(n_);
        // compute the base core
        std::vector<int> new_core(n_);
        new_cm->ComputeCore(graph,true,new_core);
        new_cm->Check(graph,new_core);

        DEBUG_PRINT("check for edge insert\n");
        //Check(graph,core);
        for (int cur = new_cm->head_[K]; n_ != cur; )
        {
            if(cur==-1)
            {
                break;
            }

            if (old_core_k[cur]!=true)
            { // line 15
                follower.push_back(cur);
                DEBUG_PRINT("find a follower %d\n",cur);
            }
            cur = new_cm->node_[cur].next;
        }

        for (int cur = new_cm->head_[K+1]; n_ != cur; )
        {
            if(cur==-1)
            {
                break;
            }

            if (old_core_k[cur]!=true)
            { // line 15
                follower.push_back(cur);
                DEBUG_PRINT("find a follower %d\n",cur);
            }
            cur = new_cm->node_[cur].next;
        }

        for (auto uu : insert_record)
        {
            graph[u].erase(std::find(graph[u].begin(), graph[u].end(), uu));
            graph[uu].erase(std::find(graph[uu].begin(), graph[uu].end(), u));
            DEBUG_PRINT("remove edge: %d,%d\n",u,uu);
            DEBUG_PRINT("remove edge: %d,%d\n",uu,u);
            fflush(stdout);
        }
        for (auto vv : insert_record_v)
        {
            graph[vv].erase(std::find(graph[vv].begin(), graph[vv].end(), v));
            graph[v].erase(std::find(graph[v].begin(), graph[v].end(), vv));
            DEBUG_PRINT("remove edge: %d,%d\n",vv,v);
            DEBUG_PRINT("remove edge: %d,%d\n",v,vv);
            fflush(stdout);
        }

        DEBUG_PRINT("check for edges remove\n");
        //ComputeCore(graph,true,core);
        delete new_cm;
    }

    void GLIST::ComputeFollower(const int u,
            std::vector<std::vector<int>>& graph,
            std::vector<int>& core, std::vector<int>& follower,int K)
    {

        int new_neighbour_num = K - core[u];
        DEBUG_PRINT("Insert new neighbour for node :%d, neighbour number:%d\n",u, new_neighbour_num);
        if (new_neighbour_num<=0) return;
        int count = 0;
        //int current_core = K+ 1; // head[K+1] can be empty
        int current_core = K; // head[K+1] can be empty

        //for(int cur=0;cur<n_;cur++)
        //{
            //node_[cur].ext=0;
        //}

        std::vector<bool> old_core_k = std::vector<bool>(n_, false); // record the old k core list

        for (int cur = head_[K]; n_ != cur; )
        {
            if(cur==-1)
            {
                break;
            }
            old_core_k[cur]=true;
            cur = node_[cur].next;
        }

        std::vector<int> insert_record;
        DEBUG_PRINT("ComputeFollower node :%d, core number:%d\n",u,core[u]);

        for ( int test_k=0;test_k<20; test_k++)
        {

            DEBUG_PRINT("core list for %d\n",test_k);
            if(head_[test_k]==-1)
            {
                continue;
            }
            for (int cur = head_[test_k];n_ !=cur;)
            {
                DEBUG_PRINT("node %d, ",cur);
                cur = node_[cur].next;
            }
            DEBUG_PRINT("\n");

        }
        while(count<new_neighbour_num)
        {

            if(head_[current_core]==-1)
            {
                DEBUG_PRINT("ComputeFollower, core list is empty:%d\n",current_core);
                current_core+=1;
                continue;
            }
            for (int cur = head_[current_core];n_ !=cur;)
            {
                if (count<new_neighbour_num)
                {

                    if (std::find(graph[u].begin(),graph[u].end(),cur)==graph[u].end())
                    {
                        DEBUG_PRINT("insert edge: %d,%d\n",u,cur);
                        Insert(u,cur,graph,core);
                        insert_record.push_back(cur);
                        count+=1;
                        DEBUG_PRINT("ComputeFollower finished insert edge: %d,%d\n",u,cur);
                        //DEBUG_PRINT("next node:  %d\n",node_[cur].next);
                    }
                    cur = node_[cur].next;
                }
                else
                {
                    DEBUG_PRINT("find all neighbour for %d\n",u);
                    break;
                }
            }

            if(count<new_neighbour_num)
            {
                current_core+=1;
                DEBUG_PRINT("current core %d\n",current_core);
            }
        }
        DEBUG_PRINT("check for edge insert\n");
        //Check(graph,core);
        for (int cur = head_[K]; n_ != cur; )
        {
            if(cur==-1)
            {
                break;
            }

            if (old_core_k[cur]!=true)
            { // line 15
                follower.push_back(cur);
                DEBUG_PRINT("find a follower %d\n",cur);
            }
            cur = node_[cur].next;
        }
        std::reverse(insert_record.begin(), insert_record.end());
        for (auto v : insert_record)
        {
            Remove(u,v,graph,core);
            DEBUG_PRINT("remove edge: %d,%d\n",u,v);
            fflush(stdout);
        }

        DEBUG_PRINT("check for edges remove\n");
        //Check(graph,core);
    }
    //void GLIST::ComputeFollower(const int u,
    //std::vector<std::vector<int>>& graph,
    //std::vector<int>& core, std::vector<int>& follower,int K)
    //{

    //std::vector<int> insert_record;
    //for (const auto v : graph[u]) { // line 2
    //if (core[v]==K-1 && GetRank(u)<=GetRank(v))
    //{
    //std::vector<bool> old_core_k = std::vector<bool>(n_, false); // record the old k core list
    //for (int cur = head_[K]; n_ != cur; ) {
    //old_core_k[cur]=true;
    //cur = node_[cur].next;
    //}

    //// remove the edge
    //graph[v].erase(std::find(graph[v].begin(), graph[v].end(), u));
    //graph[u].erase(std::find(graph[u].begin(), graph[u].end(), v));
    //DEBUG_PRINT("remove graph edge %d,%d\n",u,v);
    //// do we need to update mcd and core ?
    //// update the mcd values
    //if (core[u] <= core[v]) --mcd_[u];
    //if (core[v] <= core[u]) --mcd_[v];
    //DEBUG_PRINT("update core %d,%d\n",u,v);

    //const int temp_core = core[u];

    //core[u]=n_;

    //// do we need to specify the namespace for function Insert?
    //insert_record.push_back(v);
    //Insert(u,v,graph,core);
    //DEBUG_PRINT("insert node: %d,%d\n",u,v);

    //for (int cur = head_[K]; n_ != cur; ) {
    //if (!old_core_k[cur]==true){ // line 15
    //follower.push_back(cur);
    //}
    //cur = node_[cur].next;
    //}

    //core[u] = temp_core;
    //}
    //}

    //for (auto v : insert_record)
    //{
    //Remove(u,v,graph,core);
    //DEBUG_PRINT("remove edge: %d,%d\n",u,v);
    //// insert the edge
    //graph[u].push_back(v);
    //graph[v].push_back(u);
    //// update mcd
    //if (core[u] <= core[v]) ++mcd_[u];
    //if (core[v] <= core[u]) ++mcd_[v];
    //}
    //}

    void GLIST::AnchoredKorder(const int u,
            std::vector<std::vector<int>>& graph,
            std::vector<int>& core, int K)
    {

        int new_neighbour_num = K - core[u];
        int count = 0;
        int current_core = K;

        //std::vector<bool> old_core_k = std::vector<bool>(n_, false); // record the old k core list
        //for (int cur = head_[K]; n_ != cur; )
        //{
        //old_core_k[cur]=true;
        //cur = node_[cur].next;
        //}

        //std::vector<int> insert_record;
        DEBUG_PRINT("node :%d, core number:%d\n",u,core[u]);

        while(count<new_neighbour_num)
        {

            for (int cur = head_[current_core];n_ !=cur;)
            {
                if (count<new_neighbour_num)
                {

                    if (std::find(graph[u].begin(),graph[u].end(),cur)==graph[u].end())
                    {
                        Insert(u,cur,graph,core);
                        count+=1;
                    }
                    cur = node_[cur].next;
                }
                else
                {
                    break;
                }
            }

            if(count<new_neighbour_num)
            {
                current_core+=1;
                DEBUG_PRINT("current core %d\n",current_core);
            }
        }
    }
    void GLIST::AnchoredKorder1(const int u,
            std::vector<std::vector<int>>& graph,
            std::vector<int>& core, int K,std::vector<int> &insert_node1,std::vector<int> &insert_node2)
    {

        int new_neighbour_num = K - core[u];
        int count = 0;
        int current_core = K;

        //std::vector<bool> old_core_k = std::vector<bool>(n_, false); // record the old k core list
        //for (int cur = head_[K]; n_ != cur; )
        //{
        //old_core_k[cur]=true;
        //cur = node_[cur].next;
        //}

        //std::vector<int> insert_record;
        DEBUG_PRINT("node :%d, core number:%d\n",u,core[u]);

        while(count<new_neighbour_num)
        {

            for (int cur = head_[current_core];n_ !=cur;)
            {
                if (count<new_neighbour_num)
                {

                    if (std::find(graph[u].begin(),graph[u].end(),cur)==graph[u].end())
                    {
                        Insert(u,cur,graph,core);
                        insert_node1.push_back(u);
                        insert_node2.push_back(cur);
                        count+=1;
                    }
                    cur = node_[cur].next;
                }
                else
                {
                    break;
                }
            }

            if(count<new_neighbour_num)
            {
                current_core+=1;
                DEBUG_PRINT("current core %d\n",current_core);
            }
        }
    }

    void GLIST::Insert(const int v1, const int v2,
            std::vector<std::vector<int>>& graph,
            std::vector<int>& core) {
        // insert the edge
        graph[v1].push_back(v2);
        graph[v2].push_back(v1);
        // update mcd
        if (core[v1] <= core[v2]) ++mcd_[v1];
        if (core[v2] <= core[v1]) ++mcd_[v2];
        // the source node and the current core number
        int src = v1;
        const int K = core[v1] <= core[v2] ? core[v1] : core[v2];
        if ((core[v1] == core[v2] &&
                    tree_.Rank(v1) > tree_.Rank(v2)) ||
                core[v1] > core[v2]) {
            src = v2;
        }
        // update core number
        ++node_[src].rem;
        // there is no need to update the core numbers
        if (node_[src].rem <= K) {
            return;
        }// src node  may be increase it core number

        // preparing the heap
        DEBUG_PRINT("src node insert to head_:%d\n",src);
        fflush(stdout);
        heap_.Insert(GetRank(src), src);
        //
        std::vector<int> swap;
        // the set of vertices, denoted as A, that doesn't need to be updated
        int list_h = -1, list_t = -1;
        for (int cur = head_[K]; n_ != cur; ) {

            //DEBUG_PRINT("insert function core:%d node %d,ext %d,rem %d,rank %d\n",K,cur,node_[cur].ext,node_[cur].rem,GetRank(cur));
            fflush(stdout);
            cur=node_[cur].next;
        }
        for (int cur = tail_[K]; n_ != cur; ) {

            //DEBUG_PRINT("head core reverse print:%d node %d,ext %d,rem %d\n",K,cur,node_[cur].ext,node_[cur].rem);
            fflush(stdout);
            cur=node_[cur].prev;
        }

        //DEBUG_PRINT("Heap top value: %d\n",heap_.Top().val);
        //DEBUG_PRINT("Heap is empty: %d\n",heap_.Empty());
        for (int cur = head_[K]; n_ != cur; ) {
            //DEBUG_PRINT("Insert function: cur node %d to head_ %d\n",cur,K);
            DEBUG_PRINT("Insert function: cur node %d and its ext %d, rem %d, %d,K:%d\n",cur,node_[cur].ext,node_[cur].rem,(node_[cur].ext == 0 && node_[cur].rem <= K),K);
            fflush(stdout);
            if (heap_.Empty() || (node_[cur].ext == 0 && node_[cur].rem <= K)) {
                //DEBUG_PRINT("Insert function: cur node %d and its ext %d\n",cur,node_[cur].ext);
                const int start = cur;
                const int end = heap_.Empty() ? tail_[K] : node_[heap_.Top().val].prev; //小顶堆， 值小的出堆先出
                // advance the cur pointer
                //DEBUG_PRINT("Insert function: end node %d\n",end);
                cur = node_[end].next;
                //DEBUG_PRINT("Insert function: cur node %d\n",cur);
                fflush(stdout);
                // remove this sub-list and reinsert it into A
                node_[node_[start].prev].next = node_[end].next;
                node_[node_[end].next].prev = node_[start].prev;
                node_[start].prev = n_;
                node_[end].next = n_;
                if (-1 == list_h) {
                    list_h = start;
                    list_t = end;
                } else {
                    node_[start].prev = list_t;
                    node_[list_t].next = start;
                    list_t = end;
                }
                continue;
            }
            // update the heap
            // invariant: heap.Top().val == cur
            //fflush(stdout);
            ASSERT(heap_.Top().val == cur);
            //DEBUG_PRINT("heap delete node val %d, key %d\n", heap_.Top().val,heap_.Top().key);
            heap_.Delete(heap_.Top().key);
            // deal with cur
            const int next = node_[cur].next;
            const int cur_deg = node_[cur].ext + node_[cur].rem;
            if (likely(cur_deg <= K)) {
                // insert into A, remove the cur that will not in in O(k+1)
                node_[node_[cur].prev].next = node_[cur].next;
                node_[node_[cur].next].prev = node_[cur].prev;
                if (likely(-1 != list_h)) {
                    node_[cur].next = n_;//removing cur and add it to O'_k,which is actually the list_h and list_t
                    node_[cur].prev = list_t;
                    node_[list_t].next = cur;
                    list_t = cur;
                } else {
                    node_[cur].prev = node_[cur].next = n_;
                    list_h = list_t = cur;
                }
                node_[cur].rem = cur_deg;
                node_[cur].ext = 0;
                Keep(graph, cur, K, core, list_t, swap);
            } else {// line 9
                // cur is temporarily marked as evicted, i.e.,
                // its core number may be updated finally
                evicted_[cur] = true; //Algorithm 2有一个remove v_i的操作，并放到V_c，evicted_应该就是V_c了
                for (const auto u : graph[cur]) {
                    if (core[u] == core[cur] && GetRank(u) > rank_[cur]) { // core[cur] or K, a potential bug
                        ++node_[u].ext;
                        if (!heap_.Contains(rank_[u])) {
                            heap_.Insert(rank_[u], u);
                            //DEBUG_PRINT("heap insert  node %d\n", u);
                        }
                    }
                }
            }
            cur = next;
        }
        ASSERT(heap_.Empty());
        head_[K] = list_h; //Algorithm2->31
        tail_[K] = list_t;
        for (const int v : swap) {//swap 里面的点是由于动态
            //DEBUG_PRINT("Insert before swap node  %d, rank %d\n",v,GetRank(v));
            tree_.Delete(v, root_[K]);//
            tree_.InsertAfter(v, node_[v].prev, root_[K]); //？
            //DEBUG_PRINT("Insert after swap node  %d, rank %d\n",v,GetRank(v));
        }
        // cope with those vertices whose core need to be updated
        if (evicted_[src]) {//if src is not true, then all its remaining degree will not be able to be processed.
            auto tail = -1; // tail
            for (auto v = src; n_ != v; v = node_[v].next) {
                ++core[v]; // Algorithm 2->29
                node_[v].ext = 0; // Algorithm 2->28
                tail = v;
                // update mcd
                for (const auto u : graph[v]) {
                    if (evicted_[u]) continue;
                    if (K + 1 == core[u]) {
                        ++mcd_[u];
                    } else if (K == core[u]) {
                        --mcd_[v];
                    }
                }
                // remove from the current tree
                tree_.Delete(v, root_[K]);
            }
            for (auto v = tail; n_ != v; v = node_[v].prev) {
                evicted_[v] = false;
                tree_.Insert(v, true, root_[K + 1]);
            }
            // merge list
            if (-1 == head_[K + 1]) { //232 to 239 corresponds to 30 in Algorithm 2.
                head_[K + 1] = src;
                tail_[K + 1] = tail;
            } else {
                node_[head_[K + 1]].prev = tail;
                node_[tail].next = head_[K + 1];
                head_[K + 1] = src;
            }
        }

        int counter_o_k_2=0;
        for (int v=list_h;v!=n_;v= node_[v].next)
        {
            counter_o_k_2+=1;
            DEBUG_PRINT("Insert_func after swap node  %d rank %d\n", v,GetRank(v));
        }
        for (const int v : garbage_) rank_[v] = 0;
        garbage_.clear();
        DEBUG_PRINT("finish insert %d,%d\n",v1,v2);
        fflush(stdout);
    }




    void GLIST::Remove(const int v1, const int v2,
            std::vector<std::vector<int>>& graph,
            std::vector<int>& core) {
        // compute mcd to make sure mcd is right

        for (int v = 0; v < n_; ++v) {
            mcd_[v] = 0;
            for (const int u : graph[v]) {
                if (core[u] >= core[v]) {
                    ++mcd_[v];
                }
            }
        }

        // remove the edge
        graph[v1].erase(std::find(graph[v1].begin(), graph[v1].end(), v2));
        graph[v2].erase(std::find(graph[v2].begin(), graph[v2].end(), v1));
        // update the mcd values
        if (core[v1] <= core[v2]) --mcd_[v1];
        if (core[v2] <= core[v1]) --mcd_[v2];
        // set the root and core number
        const int root = core[v1] <= core[v2] ? v1 : v2;
        const int K = core[root];
        // update rem
        if (core[v1] == core[v2]) {
            if (tree_.Rank(v1) > tree_.Rank(v2)) {
                --node_[v2].rem;
            } else {
                --node_[v1].rem;
            }
        } else {
            --node_[root].rem;
        }
        // update cores
        std::vector<int> to_be_clear;
        std::vector<int> changed;
        if (core[v1] != core[v2]) {
            visited_[root] = true;
            deg_[root] = mcd_[root];
            to_be_clear.push_back(root);
            if (deg_[root] < K) {
                PropagateDismissal(graph, K, root, core, to_be_clear, changed);
            }
        } else {
            visited_[v1] = true;
            deg_[v1] = mcd_[v1];
            to_be_clear.push_back(v1);
            if (deg_[v1] < K) {
                PropagateDismissal(graph, K, v1, core, to_be_clear, changed);
            }
            if (!visited_[v2]) {
                visited_[v2] = true;
                deg_[v2] = mcd_[v2];
                to_be_clear.push_back(v2);
                if (deg_[v2] < K) {
                    PropagateDismissal(graph, K, v2, core, to_be_clear, changed);
                }
            }
        }
        // clear
        for (const int u : to_be_clear) {
            visited_[u] = false;
            deg_[u] = 0;
        }
        if (!changed.empty()) {
            // why n_ !=head_[K], if head_[K] is empty, its value should be -1.
            while (n_ != head_[K] && evicted_[head_[K]]) {
                head_[K] = node_[head_[K]].next;
            }
            while (n_ != tail_[K] && evicted_[tail_[K]]) {
                tail_[K] = node_[tail_[K]].prev;
            }
            if (n_ == head_[K]) {
                head_[K] = tail_[K] = -1;
            }
            for (const int v : changed) {
                node_[v].rem = 0;
                for (const int u : graph[v]) {
                    if (core[u] == K) {
                        --mcd_[u];
                        if (!evicted_[u] && GetRank(v) > GetRank(u)) {
                            --node_[u].rem;
                        }
                    } else if (core[u] == K - 1 && !evicted_[u]) {
                        ++mcd_[v];
                    }
                    if (core[u] >= K || (evicted_[u] && !visited_[u])) {
                        ++node_[v].rem;
                    }
                }
                visited_[v] = true;
            }
            for (const auto v : changed) {
                evicted_[v] = false;
                visited_[v] = false;
                tree_.Delete(v, root_[K]);
                tree_.Insert(v, false, root_[K - 1]); // false or true, any difference
                // remove from current list
                node_[node_[v].next].prev = node_[v].prev;
                node_[node_[v].prev].next = node_[v].next;
                node_[v].next = node_[v].prev = n_;
                // merge list
                if (-1 == head_[K - 1]) {
                    head_[K - 1] = tail_[K - 1] = v;
                } else {
                    node_[tail_[K - 1]].next = v;
                    node_[v].prev = tail_[K - 1];
                    tail_[K - 1] = v;
                }
            }
        }
        for (const int g : garbage_) rank_[g] = 0;
        garbage_.clear();
        DEBUG_PRINT("finish remove %d,%d\n",v1,v2);
        fflush(stdout);
    }
    void GLIST::Check(const std::vector<std::vector<int>>& graph,
            const std::vector<int>& core) const {
        for (int v = 0; v < n_; ++v) {
            int local_mcd = 0;
            for (const auto u : graph[v]) {
                if (core[u] >= core[v]) ++local_mcd;
            }
            //ASSERT(mcd_[v] == local_mcd);
            ASSERT(!visited_[v]);
            ASSERT(!evicted_[v]);
            ASSERT(rank_[v] == 0);
            ASSERT(deg_[v] == 0);
        }
        std::vector<bool> vis(n_, false);
        for (int v = 0; v < n_; ++v) {
            if (vis[v]) continue;
            const int K = core[v];
            int tail = -1;
            ASSERT(-1 != head_[K]);
            for (int tmp = head_[K]; n_ != tmp; tmp = node_[tmp].next) {
                ASSERT(!vis[tmp]);
                vis[tmp] = true;
                tail = tmp;
                ASSERT(core[tmp] == K);
                ASSERT(node_[tmp].ext == 0);
                if (n_ != node_[tmp].next) {
                    ASSERT(node_[node_[tmp].next].prev == tmp);
                }
            }
            ASSERT(tail_[K] == tail);
            ASSERT(node_[head_[K]].prev == n_);
            ASSERT(node_[tail_[K]].next == n_);

            for (int tmp = head_[K], rid = 0; n_ != tmp; tmp = node_[tmp].next) {
                ASSERT(tree_.Rank(tmp) == ++rid);
            }
            for (int tmp = head_[K]; n_ != tmp; tmp = node_[tmp].next) {
                int local = 0;
                for (const auto u : graph[tmp]) {
                    if (core[u] > core[tmp] ||
                            (core[u] == core[tmp] &&
                             tree_.Rank(u) > tree_.Rank(tmp))) {
                        ++local;
                    }
                }
                ASSERT(local == node_[tmp].rem);
                //DEBUG_PRINT("deg+ check: node  %d, core %d, deg+ %d\n",tmp,core[tmp],node_[tmp].rem);
                ASSERT(node_[tmp].rem <= K);
            }
        }
        ASSERT(garbage_.empty());
        ASSERT(heap_.Empty());
    }

    void GLIST::Keep_customed(const std::vector<std::vector<int>>& graph,
            const int v, const int K,
            const std::vector<int>& core,
            int& list_t, std::vector<int>& swap) {
        // update
        std::queue<int> bfs;
        for (const auto u : graph[v]) {
            if (core[u] == core[v] && evicted_[u]) {
                --node_[u].rem;
                if (node_[u].rem + node_[u].ext <= K) {
                    visited_[u] = true;
                    bfs.push(u);
                }
            }
        }
        while (!bfs.empty()) {
            const int u = bfs.front(); bfs.pop();

            visited_[u] = false;
            evicted_[u] = false; //在Vc里标记
            // insert u into the list
            node_[node_[u].prev].next = node_[u].next;//这里相当于把u从list 里去掉
            node_[node_[u].next].prev = node_[u].prev;
            swap.push_back(u);
            node_[list_t].next = u;
            node_[u].next = n_;
            node_[u].prev = list_t;//放到O'_k里
            node_[u].rem += node_[u].ext;
            node_[u].ext = 0;
            list_t = u;
            // advance the tail of list

            //if (u==1)
            //{
            //DEBUG_PRINT("Keep_customed, %d\n",u);
            //for (int cur = list_t; n_ != cur; ) {

            //DEBUG_PRINT("Keep_customed finished insert O' %d \n",cur);
            //fflush(stdout);
            //cur=node_[cur].prev;
            //}
            //}

            // find more vertices to keep
            for (const auto w : graph[u]) {
                if (core[w] != core[u]) continue;
                if (rank_[w] > rank_[v]) {
                    --node_[w].ext;
                    if (0 == node_[w].ext) {
                        //heap_.Delete(rank_[w]);
                    }
                } else if (rank_[w] > rank_[u] && evicted_[w]) {
                    --node_[w].ext;
                    if (!visited_[w] && node_[w].ext + node_[w].rem <= K) {
                        visited_[w] = true;
                        bfs.push(w);
                    }
                } else if (evicted_[w]) {
                    --node_[w].rem;
                    if (!visited_[w] && node_[w].ext + node_[w].rem <= K) {
                        visited_[w] = true;
                        bfs.push(w);
                    }
                }
            }
        }
    }
    void GLIST::Keep(const std::vector<std::vector<int>>& graph,
            const int v, const int K,
            const std::vector<int>& core,
            int& list_t, std::vector<int>& swap) {
        // update
        std::queue<int> bfs;
        for (const auto u : graph[v]) {
            if (core[u] == core[v] && evicted_[u]) {
                --node_[u].rem;
                if (node_[u].rem + node_[u].ext <= K) {
                    visited_[u] = true;
                    bfs.push(u);
                }
            }
        }
        while (!bfs.empty()) {
            const int u = bfs.front(); bfs.pop();
            visited_[u] = false;
            evicted_[u] = false; //在Vc里标记
            // insert u into the list
            node_[node_[u].prev].next = node_[u].next;//这里相当于把u从list 里去掉
            node_[node_[u].next].prev = node_[u].prev;
            swap.push_back(u);
            node_[list_t].next = u;
            node_[u].next = n_;
            node_[u].prev = list_t;//放到O'_k里
            node_[u].rem += node_[u].ext;
            node_[u].ext = 0;
            // advance the tail of list
            list_t = u;
            // find more vertices to keep
            for (const auto w : graph[u]) {
                if (core[w] != core[u]) continue;
                if (rank_[w] > rank_[v]) {
                    --node_[w].ext;
                    if (0 == node_[w].ext) {
                        heap_.Delete(rank_[w]);
                    }
                } else if (rank_[w] > rank_[u] && evicted_[w]) {
                    --node_[w].ext;
                    if (!visited_[w] && node_[w].ext + node_[w].rem <= K) {
                        visited_[w] = true;
                        bfs.push(w);
                    }
                } else if (evicted_[w]) {
                    --node_[w].rem;
                    if (!visited_[w] && node_[w].ext + node_[w].rem <= K) {
                        visited_[w] = true;
                        bfs.push(w);
                    }
                }
            }
        }
    }
    void GLIST::PropagateDismissal(const std::vector<std::vector<int>>& graph,
            const int K, const int v,
            std::vector<int>& core,
            std::vector<int>& to_be_clear,
            std::vector<int>& changed) {
        evicted_[v] = true;
        --core[v];
        changed.push_back(v);
        for (const auto u : graph[v]) {
            if (K == core[u]) {
                if (!visited_[u]) {
                    deg_[u] = mcd_[u];
                    visited_[u] = true;
                    to_be_clear.push_back(u);
                }
                --deg_[u];
                if (deg_[u] < K && !evicted_[u]) {
                    PropagateDismissal(graph, K, u, core, to_be_clear, changed);
                }
            }
        }
    }
    void GLIST::EdgeRemove(std::vector<std::pair<int, int>> &edges,
            std::vector<std::vector<int>>& graph,
            std::vector<int>& core,const int K,
            std::vector<bool>& evicted_vr) {

        //debug rem in k-oder 9
        //evicted_ = std::vector<bool>(n_, false);
        // Algorithm 5: line 2
        std::vector<int> VR;
        std::vector<int> VC;// equal to the variable evicted

        // line 3
        std::queue<int> bfs;
        // used for V*
        std::vector<int> Vs_head = std::vector<int>(n_, -1);
        std::vector<int> Vs_tail = std::vector<int>(n_, -1);

        // a new mcd for avoid conflict
        std::vector<int> mcd_2  = std::vector<int>(n_, 0);
        std::vector<int> visited_flag = std::vector<int>(n_, 0);

        // copy graph before edge removes to graph_old
        //std::vector<std::vector<int>> graph_old = graph;
        //
        std::vector<std::vector<int>> graph_old(n_);
        for (int i=0;i< n_;i++)
        {
            for (auto j: graph[i] )
            {
                graph_old[i].push_back(j);
            }
        }
        //graph_old.assign(graph.begin(), graph.end());
        //auto graph_old(graph);


        int m = 0;
        int num_e = edges.size();
        DEBUG_PRINT("remove edge size:%d\n",num_e);
        // line 6-14
        //for (int cur = head_[42]; n_ != cur;cur=node_[cur].next)
        //{

            //DEBUG_PRINT("print the node %d in 42 list %d\n",cur,42);
        //}
        for (int i = 0; i < num_e; ++i) {

            int v1 = edges[i].first;
            int v2 = edges[i].second;
            DEBUG_PRINT("remove edge node1 %d, node2 %d\n",v1,v2);
            if(v1==9)
            {
                for (int u:graph[9])
                {
                    DEBUG_PRINT("node 9 neighbour: node %d\n",u);
                }
            }
            // remove the edge
            graph[v1].erase(std::find(graph[v1].begin(), graph[v1].end(), v2));
            graph[v2].erase(std::find(graph[v2].begin(), graph[v2].end(), v1));
            // update the mcd values, should the updading be here or later? or should it based on the rank or just core number as in Glist
            //if (core[v1] <= core[v2]) --mcd_[v1];
            //if (core[v2] <= core[v1]) --mcd_[v2];


            int u_;
            if (core[v1]<core[v2])
            {
                u_=v1;
            }
            if (core[v1]>core[v2])
            {
                u_=v2;
            }
            if (core[v1]==core[v2])
            {
                u_ = v1;
            }
            int j = core[u_]; // u_, node in the previous deleted edges
            //if (core[v1]==core[v2])
            //{
                //u_ = tree_.Rank(v1) < tree_.Rank(v2) ? v1 : v2;// remove equal
            //}

            // line 8
            //if (core[v1] == core[v2]) {
                //if (tree_.Rank(v1) > tree_.Rank(v2)) {
                    //--node_[v2].rem;
                //} else {
                    //--node_[v1].rem;
                //}
            //}
            //else {
                //--node_[u_].rem;
            //}

            if (core[v1]!=core[v2])
            {

                // line 9
                mcd_2[u_]=0; // incase v1->v2, v1->v3,and core[v2]>core[v1], core[v3]>core[v1], which may lead mcd v1 increase
                for (auto uu:graph[u_])
                {

                    if(core[uu]>=core[u_])
                    {
                        mcd_2[u_]+=1; // mcd >= deg+
                    }
                }

                int core_u_ = core[u_];
                // line 10
                if(mcd_2[u_]< core[u_])
                {
                    if(!visited_flag[u_])
                    {
                        // line 11, remove from current list, do we need to judge the head or tail ?

                        // operations for head and tail for special cases
                        if(head_[core[u_]]==u_)
                        {
                            head_[core[u_]]=node_[u_].next;
                            if (head_[core[u_]]==n_)
                            {
                                head_[core[u_]]=-1;
                                tail_[core[u_]]=-1;
                            }
                        }

                        if(tail_[core[u_]]==u_)
                        {
                            tail_[core[u_]]=node_[u_].prev; // shit
                            if(tail_[core[u_]]==n_)
                            {
                                head_[core[u_]]=-1;
                                tail_[core[u_]]=-1;
                            }
                        }


                        node_[node_[u_].next].prev = node_[u_].prev;
                        node_[node_[u_].prev].next = node_[u_].next;
                        node_[u_].next = node_[u_].prev = n_;

                        tree_.Delete(u_, root_[core_u_]);
                        bfs.push(u_);
                        visited_flag[u_]=1;
                        if(u_==411)
                        {
                            DEBUG_PRINT("remove edge: node1 %d, node2 %d, core change minus one:%d\n",v1,v2,core[u_]-1);
                        }
                        --core[u_];
                    }
                    else
                    {
                        //line 13 remove u_ from Vj list


                        int t = core[u_];
                        //DEBUG_PRINT("Insert to Vstar core number %d, node %d, core %d\n",t,u,core[u]);
                        //ASSERT(t>0);
                        ASSERT(Vs_head[t]!=-1);

                        if(Vs_head[core[u_]]==u_)
                        {
                            Vs_head[core[u_]]=node_[u_].next;
                            if (Vs_head[core[u_]]==n_)
                            {
                                Vs_head[core[u_]]=-1;
                                Vs_tail[core[u_]]=-1;
                            }
                        }

                        if(Vs_tail[core[u_]]==u_)
                        {
                            Vs_tail[core[u_]]=node_[u_].prev; // node_[u_].prev should be node_[u_].next ?
                            if(Vs_tail[core[u_]]==n_)
                            {
                                Vs_head[core[u_]]=-1;
                                Vs_tail[core[u_]]=-1;
                            }
                        }

                        node_[node_[u_].next].prev = node_[u_].prev;
                        node_[node_[u_].prev].next = node_[u_].next;
                        node_[u_].next = node_[u_].prev = n_;

                        bfs.push(u_);
                        visited_flag[u_]=1;
                        if(u_==411)
                        {
                            DEBUG_PRINT("remove edge: node1 %d, node2 %d, core change minus one:%d\n",v1,v2,core[u_]-1);
                        }
                        --core[u_];
                    }
                }
            }
            else
            {



                // line 9
                u_ = v1;
                mcd_2[u_]=0; // incase v1->v2, v1->v3,and core[v2]>core[v1], core[v3]>core[v1], which may lead mcd v1 increase
                for (auto uu:graph[u_])
                {

                    if(core[uu]>=core[u_])
                    {
                        mcd_2[u_]+=1; // mcd >= deg+
                    }
                }

                int core_u_ = core[u_];
                // line 10
                if(mcd_2[u_]< core[u_])
                {
                    if(!visited_flag[u_])
                    {
                        // line 11, remove from current list, do we need to judge the head or tail ?

                        // operations for head and tail for special cases
                        if(head_[core[u_]]==u_)
                        {
                            head_[core[u_]]=node_[u_].next;
                            if (head_[core[u_]]==n_)
                            {
                                head_[core[u_]]=-1;
                                tail_[core[u_]]=-1;
                            }
                        }

                        if(tail_[core[u_]]==u_)
                        {
                            tail_[core[u_]]=node_[u_].prev; // shit
                            if(tail_[core[u_]]==n_)
                            {
                                head_[core[u_]]=-1;
                                tail_[core[u_]]=-1;
                            }
                        }


                        node_[node_[u_].next].prev = node_[u_].prev;
                        node_[node_[u_].prev].next = node_[u_].next;
                        node_[u_].next = node_[u_].prev = n_;

                        tree_.Delete(u_, root_[core_u_]);
                        bfs.push(u_);
                        visited_flag[u_]=true;
                        if(u_==411)
                        {
                            DEBUG_PRINT("remove edge: node1 %d, node2 %d, core change minus one:%d\n",v1,v2,core[u_]-1);
                        }
                        --core[u_];
                    }
                    else
                    {
                        //line 13 remove u_ from Vj list


                        int t = core[u_];
                        //DEBUG_PRINT("Insert to Vstar core number %d, node %d, core %d\n",t,u,core[u]);
                        //ASSERT(t>0);
                        ASSERT(Vs_head[t]!=-1);

                        if(Vs_head[core[u_]]==u_)
                        {
                            Vs_head[core[u_]]=node_[u_].next;
                            if (Vs_head[core[u_]]==n_)
                            {
                                Vs_head[core[u_]]=-1;
                                Vs_tail[core[u_]]=-1;
                            }
                        }

                        if(Vs_tail[core[u_]]==u_)
                        {
                            Vs_tail[core[u_]]=node_[u_].prev; // node_[u_].prev should be node_[u_].next ?
                            if(Vs_tail[core[u_]]==n_)
                            {
                                Vs_head[core[u_]]=-1;
                                Vs_tail[core[u_]]=-1;
                            }
                        }

                        node_[node_[u_].next].prev = node_[u_].prev;
                        node_[node_[u_].prev].next = node_[u_].next;
                        node_[u_].next = node_[u_].prev = n_;

                        bfs.push(u_);
                        visited_flag[u_]=1;
                        if(u_==411)
                        {
                            DEBUG_PRINT("remove edge: node1 %d, node2 %d, core change minus one:%d\n",v1,v2,core[u_]-1);
                        }
                        --core[u_];
                    }
                }


                u_ = v2;
                mcd_2[u_]=0; // incase v1->v2, v1->v3,and core[v2]>core[v1], core[v3]>core[v1], which may lead mcd v1 increase
                for (auto uu:graph[u_])
                {

                    if(core[uu]>=core[u_])
                    {
                        mcd_2[u_]+=1; // mcd >= deg+
                    }
                }

                //int core_u_ = core[u_];
                // line 10
                if(mcd_2[u_]< core[u_])
                {
                    if(!visited_flag[u_])
                    {
                        // line 11, remove from current list, do we need to judge the head or tail ?

                        // operations for head and tail for special cases
                        if(head_[core[u_]]==u_)
                        {
                            head_[core[u_]]=node_[u_].next;
                            if (head_[core[u_]]==n_)
                            {
                                head_[core[u_]]=-1;
                                tail_[core[u_]]=-1;
                            }
                        }

                        if(tail_[core[u_]]==u_)
                        {
                            tail_[core[u_]]=node_[u_].prev; // shit
                            if(tail_[core[u_]]==n_)
                            {
                                head_[core[u_]]=-1;
                                tail_[core[u_]]=-1;
                            }
                        }


                        node_[node_[u_].next].prev = node_[u_].prev;
                        node_[node_[u_].prev].next = node_[u_].next;
                        node_[u_].next = node_[u_].prev = n_;

                        tree_.Delete(u_, root_[core_u_]);
                        bfs.push(u_);
                        visited_flag[u_]=1;
                        if(u_==411)
                        {
                            DEBUG_PRINT("remove edge: node1 %d, node2 %d, core change minus one:%d\n",v1,v2,core[u_]-1);
                        }
                        --core[u_];
                    }
                    else
                    {
                        //line 13 remove u_ from Vj list


                        int t = core[u_];
                        //DEBUG_PRINT("Insert to Vstar core number %d, node %d, core %d\n",t,u,core[u]);
                        //ASSERT(t>0);
                        ASSERT(Vs_head[t]!=-1);

                        if(Vs_head[core[u_]]==u_)
                        {
                            Vs_head[core[u_]]=node_[u_].next;
                            if (Vs_head[core[u_]]==n_)
                            {
                                Vs_head[core[u_]]=-1;
                                Vs_tail[core[u_]]=-1;
                            }
                        }

                        if(Vs_tail[core[u_]]==u_)
                        {
                            Vs_tail[core[u_]]=node_[u_].prev; // node_[u_].prev should be node_[u_].next ?
                            if(Vs_tail[core[u_]]==n_)
                            {
                                Vs_head[core[u_]]=-1;
                                Vs_tail[core[u_]]=-1;
                            }
                        }

                        node_[node_[u_].next].prev = node_[u_].prev;
                        node_[node_[u_].prev].next = node_[u_].next;
                        node_[u_].next = node_[u_].prev = n_;

                        bfs.push(u_);
                        visited_flag[u_]=1;
                        if(u_==411)
                        {
                            DEBUG_PRINT("remove edge: node1 %d, node2 %d, core change minus one:%d\n",v1,v2,core[u_]-1);
                        }
                        --core[u_];
                    }
                }

            }

            // put Quque here


            // line 15-20

            std::vector<int> check_times(n_,0);

            std::vector<int> record_bfs;
            while (!bfs.empty())
            {
                //line 16
                const int u = bfs.front(); bfs.pop();
                record_bfs.push_back(u);

                check_times[u]+=1;
                int t = core[u];

                //line 15
                m = m<=t ? t: m;
                // line 19-20
                for (const auto u_ : graph_old[u])// need graph before remove edges
                    //for (const auto u_ : graph[u])// need graph before remove edges
                {
                    if (core[u_] != j)
                    {
                        continue;
                    }

                    // line 9
                    mcd_2[u_]=0; // incase v1->v2, v1->v3,and core[v2]>core[v1], core[v3]>core[v1], which may lead mcd v1 increase
                    for (auto uu:graph[u_])
                    {

                        if(core[uu]>=core[u_])
                        {
                            mcd_2[u_]+=1; // mcd >= deg+
                        }
                    }

                    int core_u_ = core[u_];
                    // line 10
                    if(mcd_2[u_]< core[u_])
                    {
                        if(!visited_flag[u_])
                        {
                            // line 11, remove from current list, do we need to judge the head or tail ?

                            // operations for head and tail for special cases
                            if(head_[core[u_]]==u_)
                            {
                                head_[core[u_]]=node_[u_].next;
                                if (head_[core[u_]]==n_)
                                {
                                    head_[core[u_]]=-1;
                                    tail_[core[u_]]=-1;
                                }
                            }

                            if(tail_[core[u_]]==u_)
                            {
                                tail_[core[u_]]=node_[u_].prev; // shit
                                if(tail_[core[u_]]==n_)
                                {
                                    head_[core[u_]]=-1;
                                    tail_[core[u_]]=-1;
                                }
                            }


                            node_[node_[u_].next].prev = node_[u_].prev;
                            node_[node_[u_].prev].next = node_[u_].next;
                            node_[u_].next = node_[u_].prev = n_;

                            tree_.Delete(u_, root_[core_u_]);
                            bfs.push(u_);
                            visited_flag[u_]=true;
                            if(u_==411)
                            {
                                DEBUG_PRINT("Queue remove edge: node1 %d, node2 %d, core change minus one:%d\n",v1,v2,core[u_]-1);
                            }
                            --core[u_];
                        }
                        else
                        {
                            //line 13 remove u_ from Vj list


                            int t = core[u_];
                            //DEBUG_PRINT("Insert to Vstar core number %d, node %d, core %d\n",t,u,core[u]);
                            //ASSERT(t>0);
                            ASSERT(Vs_head[t]!=-1);

                            if(Vs_head[core[u_]]==u_)
                            {
                                Vs_head[core[u_]]=node_[u_].next;
                                if (Vs_head[core[u_]]==n_)
                                {
                                    Vs_head[core[u_]]=-1;
                                    Vs_tail[core[u_]]=-1;
                                }
                            }

                            if(Vs_tail[core[u_]]==u_)
                            {
                                Vs_tail[core[u_]]=node_[u_].prev; // node_[u_].prev should be node_[u_].next ?
                                if(Vs_tail[core[u_]]==n_)
                                {
                                    Vs_head[core[u_]]=-1;
                                    Vs_tail[core[u_]]=-1;
                                }
                            }

                            node_[node_[u_].next].prev = node_[u_].prev;
                            node_[node_[u_].prev].next = node_[u_].next;
                            node_[u_].next = node_[u_].prev = n_;

                            bfs.push(u_);
                            visited_flag[u_]=1;
                            if(u_==411)
                            {
                                DEBUG_PRINT("Queue remove edge: node1 %d, node2 %d, core change minus one:%d\n",v1,v2,core[u_]-1);
                            }
                            --core[u_];
                        }
                    }

                }
            }

            for (int i=0;i< n_;i++)
            {
                for (auto j: graph[i] )
                {
                    graph_old[i].push_back(j);
                }
            }

            for (int u:record_bfs)
            {

                //visited_[u]=false;
                int t = core[u];
                // line 17
                //DEBUG_PRINT("Insert to Vstar core number %d, node %d, core %d\n",t,u,core[u]);
                //ASSERT(t>0);
                if (-1 == Vs_head[t])
                {
                    Vs_head[t] = u;
                    Vs_tail[t] = u;
                }
                else
                {
                    node_[Vs_tail[t]].next = u;
                    node_[u].prev = Vs_tail[t];
                    node_[u].next = n_;
                    Vs_tail[t]=u;
                }
            }
        }

        DEBUG_PRINT("remove all edges\n");

        //line 19
        //for(int t = m; t>=1; t--)
        //{
            //if (Vs_head[t]==-1) continue;
            ////for (int cur = Vs_tail[t];cur!=n_ ;cur=node_[cur].prev)
            //// line 20
            //for (int cur = Vs_head[t];cur!=n_ ;cur=node_[cur].next)
            //{

                //DEBUG_PRINT("Vstar core number %d, node %d, core %d\n",t,cur,core[cur]);
            //}
            //if (head_[t]==-1) continue;
            //for (int cur = head_[t];cur!=n_ ;cur=node_[cur].next)
            //{

                //DEBUG_PRINT("ok1 core number %d, node %d, core %d\n",t,cur,core[cur]);
            //}
        //}
        // line 22
        std::vector<bool> rem_compute = std::vector<bool>(n_,false);
        DEBUG_PRINT("maximum core number in edge remove: %d\n",m);
        for(int t = m; t>=0; t--)
        {

            DEBUG_PRINT("processing the v* core list %d\n",t);
            fflush(stdout);
            if (Vs_head[t]==-1)// Vs_head may be empty
            {

                DEBUG_PRINT("the core number %d is empty\n",t);
                continue;
            }

            // line 23
            std::vector<bool> vs_visited_ = std::vector<bool>(n_, false);



            // line 20
            for (int cur = Vs_head[t]; n_ != cur;)
            {

                //visited_[cur]=false;
                vs_visited_[cur]=true;
                DEBUG_PRINT("processing the node %d in v* list %d\n",cur,t);
                fflush(stdout);
                int next = node_[cur].next;
                //line 21
                node_[cur].rem = 0;
                for (const int u_ : graph[cur])
                {
                    //if (GetRank(u_)>=GetRank(cur) && core[u_] == core[cur]) // old version, change in overleaf

                    int flag = 0;
                    // this is to make sure that if a node of cur neighbour also in V*
                    // and this node is after cur node in V*

                    for (int u_1 =Vs_head[t];n_!=u_1;)
                    {
                        if (u_==u_1 &&!vs_visited_[u_])
                        {
                            flag = 1;
                            break;
                        }
                        u_1 = node_[u_1].next;
                    }

                    if ( core[u_] > core[cur] || flag )
                        //if ( core[u_] > core[cur])
                    {
                        // line 24
                        node_[cur].rem+=1;
                    }


                    //line 25
                    rem_compute[u_]=true;
                }

                //line 26,append cur to the end of order t list

                if (-1 == head_[t] && tail_[t]==-1)
                {
                    ASSERT(core[cur]==t);
                    head_[t] = cur;
                    tail_[t] = cur;
                    node_[cur].prev = node_[cur].next=n_;
                }
                else
                {
                    DEBUG_PRINT("append VS to ok1, node %d, core %d, deg+ %d, VS list %d, tail_node, %d\n",cur,core[cur],node_[cur].rem,t,tail_[t]);
                    ASSERT(core[cur]==t);
                    ASSERT(core[cur]==core[tail_[t]]);
                    node_[tail_[t]].next= cur;
                    node_[cur].prev= tail_[t];
                    tail_[t] = cur;
                }

                // insert cur into tree with root_[t]; not sure if the second parameter is right or not?
                // t or t+1?
                tree_.Insert(cur, false, root_[t]);
                //line 30
                cur = next;
            }
        }

        for (int tmp=0;tmp<n_;tmp++)
        {
            //if(rem_compute[tmp])
            if(1)
            {

                node_[tmp].rem=0;
                for (const auto u : graph[tmp]) {
                    if (core[u] > core[tmp] ||
                            (core[u] == core[tmp] &&
                             tree_.Rank(u) > tree_.Rank(tmp))) {
                        node_[tmp].rem+=1;
                    }
                }

            }

        }
        //line 31, evicted_vr used as Vr.
        //evicted_vr = std::vector<bool>(n_, false);
        if (Vs_head[K-1]!=-1)
        {
            for (int cur = Vs_head[K-1]; n_ != cur;)
            {
                evicted_vr[cur] = true;
                cur = node_[cur].next;
            }

        }
        else{
            DEBUG_PRINT("Vs head %d is empty \n",K-1);
        }

        for (const int g : garbage_) rank_[g] = 0;
        garbage_.clear();

        DEBUG_PRINT("finish remove udpating\n");
        fflush(stdout);

        std::vector<bool> vis(n_, false);
        for (int v = 0; v < n_; ++v) {
            DEBUG_PRINT("EdgeDelete core %d,  node %d,  head_  %d\n",core[v],v,head_[core[v]]);
            if (vis[v]) continue;
            int t=core[v];
            for (int tmp = head_[t]; n_ != tmp; tmp = node_[tmp].next) {
                ASSERT(!vis[tmp]);
                vis[tmp]=true;
                ASSERT(visited_[tmp] == 0);
                ASSERT(core[tmp] == t);

                int local = 0;
                for (const auto u : graph[tmp]) {
                    if (core[u] > core[tmp] ||
                            (core[u] == core[tmp] &&
                             tree_.Rank(u) > tree_.Rank(tmp))) {
                        ++local;
                    }
                }
                DEBUG_PRINT("EdgeDelete core %d,  node %d, core %d, deg- %d, deg+ %d,local deg+ %d\n",t,tmp,core[tmp],node_[tmp].ext,node_[tmp].rem,local);
                ASSERT(local == node_[tmp].rem);
                ASSERT(node_[tmp].rem <= t);
            }
        }
    }
}  // namespace core
