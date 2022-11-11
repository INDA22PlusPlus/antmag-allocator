
#include<iostream>
#include<list>
#include<vector>
using namespace std;

template<size_t n>
class arena{
    char* mem_pointer = nullptr;
    int end_idx = 0;

public:
    arena() : mem_pointer(static_cast<char*>(malloc(n))){}

    template<typename T>
    void* allocate(){
        if(end_idx + sizeof(T) > n){
            throw std::out_of_range("Attempting to overflow arena!");
        }

        void* res = static_cast<void*>(mem_pointer + end_idx);
        end_idx += sizeof(T);
        return res;
    }

    void free_mem(){
        free(this->mem_pointer);
    }

    int bytes_remaining(){
        return n - end_idx;
    }

    ~arena(){
        free_mem();
    }
};

template<typename T>
struct li_node{
    li_node* next = nullptr;
    T* data;
};

struct meta{
    size_t sz;
    size_t next;
    size_t prev;
    size_t idx;
};


#define NONE 4294967295

/*
Constant time allocation with fixed size
*/

class free_list{
public:
    static const size_t SIZE = 4096;
    char P[free_list::SIZE];
    size_t head = 0;

    free_list(){
        meta* p = reinterpret_cast<meta*>(P);
        p->sz = free_list::SIZE - sizeof(meta);
        p->idx = 0;
        p->prev = NONE;
        p->next = NONE;
    }

    // O(the number of allocations done thus far)
    void* allocate(size_t alloc_sz){
        meta* p = reinterpret_cast<meta*>(&P[this->head]);
        bool failed = false;

        while(p->sz < alloc_sz + sizeof(meta)){
            if(p->next == NONE){
                failed = true; break; continue;
            }
            p = reinterpret_cast<meta*>(&this->P[p->next]);
        }
        if(failed == true){
            throw "Could not allocate";
        }

        //The memory to assign
        void* res = &this->P[p->idx + sizeof(meta)];

        //Add the remaining space on this index
        size_t new_idx = p->idx + sizeof(meta) + alloc_sz;

        //Configuring the new empty slot node
        meta* new_p = reinterpret_cast<meta*>(&P[new_idx]);
        new_p->sz = p->sz - alloc_sz - sizeof(meta);
        new_p->next = p->next;
        new_p->prev = p->prev;
        new_p->idx = new_idx;

        //Binding
        if(p->prev != NONE){
            meta* prev_p = reinterpret_cast<meta*>(&P[p->prev]);
            prev_p->next = new_idx;
        }
        if(p->next != NONE){
            meta* next_p = reinterpret_cast<meta*>(&P[p->next]);
            next_p->prev = new_idx;
        }

        //Tracing the allocated memory
        p->sz = alloc_sz;

        //Updating the position of the first mem-block
        if(p->idx == this->head) this->head = new_idx;
        return res;
    }

    void free(void* f){
        size_t target_idx = static_cast<char*>(f) - &this->P[0] - sizeof(meta);
        size_t idx = this->head;
        size_t last_idx = NONE;

        while(idx != NONE && idx < target_idx){
            last_idx = idx;
            idx = reinterpret_cast<meta*>(&this->P[idx])->next;
        }

        //Setting up the connections from p(the block to be freed)
        meta* p = reinterpret_cast<meta*>(&this->P[target_idx]);
        p->next = idx;
        p->prev = last_idx;

        //Meaning there is a node to the right
        if(idx != NONE){
            meta* next_p = reinterpret_cast<meta*>(&this->P[idx]);
            next_p->prev = target_idx;
        }
        //Meaning there is a node to the left
        if(last_idx != NONE){
            meta* prev_p = reinterpret_cast<meta*>(&this->P[last_idx]);
            prev_p->next = target_idx;
        }

        if(target_idx < this->head) this->head = target_idx;
    }

    
    

    void print_block(const meta* p){
        cout << "adress: " << p->idx << ", " << "size: " << p->sz << endl;
    }

    void print_out(){
        size_t idx = this->head;

        while(idx != NONE){
            this->print_block(reinterpret_cast<meta*>(&this->P[idx]));
            idx = reinterpret_cast<meta*>(&this->P[idx])->next;
        }
    }
};

/*
The vector will delete and reallocate arena<n>, but this is fine
*/

namespace allocator_tests{
    void test_1(){
        free_list fl;
        vector<size_t> test_list = {20,240,10,32,45,4,4,8};
        vector<int> values = {1,2,3,4,5,6,7,8};

        int* P[8];
        for(int i = 0; i < test_list.size(); i++){
            P[i] = static_cast<int*>(fl.allocate(test_list[i]));
            P[i][0] = values[i];
        }

        fl.print_out();

        for(int i = 1; i < test_list.size(); i+=2){
            fl.free(static_cast<void*>(P[i]));
        }

        for(int i = 0; i < test_list.size(); i+= 2){
            cout << P[i][0] << " ";
        }
        cout << endl;
        fl.print_out();
    }
}


int main(){
    allocator_tests::test_1();
    free_list fl;
    void* p = fl.allocate(20);
    fl.free(p);

}



