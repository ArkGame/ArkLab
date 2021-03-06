//// test.cpp : Defines the entry point for the console application.
////
//
#include "stdafx.h"
#include <vector>
#include <algorithm>
#include <stdint.h>
#include <time.h>
#include <windows.h>
#include <iostream>
#include <chrono>

static inline uint64_t GetTimeMs() {
#ifdef _MSC_VER
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000UL + tv.tv_usec / 1000;
#endif
}

typedef void(*FuncPtrOnTimeout)(void *data);

struct TimerElem {
    friend class TimerElemWrap;
    friend class CTimerManager;

private:
    TimerElem();
    FuncPtrOnTimeout expired_func_; // 超时后执行的函数
    void *data_;                    //expired_func_的参数
    uint64_t expired_ms_;           // 绝对的超时时间,单位ms
    int heap_idx_;                  //用来删除定时器
};

struct TimerElemWrap {
    friend class CTimerManager;

public:
    TimerElemWrap(TimerElem *timer_elem, int heap_idx)
        : timer_elem_(timer_elem), heap_idx_(heap_idx) {
        if (timer_elem_) {
            timer_elem_->heap_idx_ = heap_idx_;
        }
    }
    TimerElemWrap(const TimerElemWrap &rhs) {
        (*this).timer_elem_ = rhs.timer_elem_;
        heap_idx_ = rhs.heap_idx_;
    }
    TimerElemWrap &operator=(const TimerElemWrap &rhs) {
        if (this == &rhs) {
            return *this;
        }

        (*this).timer_elem_ = rhs.timer_elem_;
        (*this).timer_elem_->heap_idx_ = heap_idx_;
        return *this;
    }

    bool operator<(const TimerElemWrap &rhs) const {
        return (*this).timer_elem_->expired_ms_ > rhs.timer_elem_->expired_ms_;
    }

private:
    TimerElemWrap();
    TimerElem *timer_elem_;
    int heap_idx_;
};

class CTimerManager {
public:
    // 添加定时器
    TimerElem * AddTimer(FuncPtrOnTimeout expired_func, void *data,
        uint64_t expired_ms) {
        TimerElem *timer_elem = (TimerElem *)malloc(sizeof(TimerElem));
        if (!timer_elem) {
            return NULL;
        }
        timer_elem->expired_func_ = expired_func;
        timer_elem->data_ = data;
        timer_elem->expired_ms_ = expired_ms;
        TimerElemWrap elem_wrap(timer_elem, (int)timer_list_.size());
        timer_list_.push_back(elem_wrap);
        std::push_heap(timer_list_.begin(), timer_list_.end());
        return timer_elem;
    }
    //删除定时器
    int DelTimer(TimerElem *timer_elem) {
        if (timer_elem == NULL) {
            return -1;
        }
        int heap_idx = timer_elem->heap_idx_;
        if (heap_idx > (int)timer_list_.size() - 1) {
            // not found
            return -2;
        }
        TimerElemWrap elem_wrap = timer_list_[heap_idx];
        if (elem_wrap.timer_elem_ != timer_elem) {
            // not found
            return -3;
        }
        elem_wrap.timer_elem_->expired_ms_ = 0;
        // 先弄到堆顶
        std::push_heap(timer_list_.begin(), timer_list_.begin() + heap_idx + 1);
        // 再删除
        std::pop_heap(timer_list_.begin(), timer_list_.end());
        timer_list_.pop_back();
        return 0;
    }
    //检查是否有定时器超时
    bool CheckExpire() {
        bool has_expired = false;
        uint64_t now_ms = GetTimeMs();
        while (!timer_list_.empty() &&
            now_ms > timer_list_[0].timer_elem_->expired_ms_) {
            std::pop_heap(timer_list_.begin(), timer_list_.end());
            TimerElemWrap elem_wrap = timer_list_.back();
            timer_list_.pop_back();

            elem_wrap.timer_elem_->expired_func_(elem_wrap.timer_elem_->data_);
            free(elem_wrap.timer_elem_);
            has_expired = true;
        }

        return has_expired;
    }

private:
    //定时器列表
    std::vector<TimerElemWrap> timer_list_;
};

// test code below
void fooTimeout(void *data) {
    uint64_t *expired_ms = (uint64_t *)data;
    std::cout << __func__ << "  " << *expired_ms << std::endl;
}

int main() {

    std::cout << GetTimeMs() << std::endl;
    //std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << std::endl;

    CTimerManager timer_manager;

    uint64_t expired_ms = GetTimeMs() + 5000;
    uint64_t expired_ms1 = expired_ms;

    timer_manager.AddTimer(fooTimeout, &expired_ms, expired_ms);
    timer_manager.AddTimer(fooTimeout, &expired_ms1, expired_ms1);
    {
        //uint64_t *expired_ms = (uint64_t *)malloc(sizeof(uint64_t));
        //*expired_ms = GetTimeMs() + 9000;
        //添加定时器
        //timer_manager.AddTimer(fooTimeout, expired_ms, *expired_ms);
        //uint64_t *expired_ms1 = (uint64_t *)malloc(sizeof(uint64_t));
        //*expired_ms1 = *expired_ms;
        //timer_manager.AddTimer(fooTimeout, expired_ms1, *expired_ms1);
    }

    //TimerElem *e2;
    //{
    //    uint64_t *expired_ms = (uint64_t *)malloc(sizeof(uint64_t));
    //    *expired_ms = GetTimeMs() + 1000;
    //    //添加定时器
    //    e2 = timer_manager.AddTimer(fooTimeout, expired_ms, *expired_ms);
    //}

    // 删除定时器
    //timer_manager.DelTimer(e2);

    // 检查是否有定时器超时
    while (1)
    {
        timer_manager.CheckExpire();
    }
    
    return 0;
}


//#include <type_traits>
//#include <iostream>
//#include <chrono>
//#include <assert.h>
//#include <sstream>
//
////template<typename T>
////class IsBuildinType
////{
////public:
////	enum { YES = false, NO = true };
////};
////
////#define MAKE_BUILDIN_TYPE(T)                \
////    template<> class IsBuildinType<T>       \
////    {                                       \
////    public:                                 \
////        enum { YES=true, NO=false };               \
////    };
////
////MAKE_BUILDIN_TYPE(int)
////
////
////template <typename T>
////void test(const T& t) {
////	if (IsBuildinType<T>::YES)
////	{
////		std::cout << "T is BuildIn type" << std::endl;
////	}
////	else
////	{
////		std::cout << "T is not BuildIn type" << std::endl;
////	}
////}
//
////#define SWAP(x, y)      \
////do{                     \
////    decltype(x) t = (x);  \
////    (x) = (y);          \
////    (y) = t;            \
////}while(0)
////
////inline int ns_rand_s32()
////{
////    return rand();
////}
////
////// [min, max]  注意两边都为闭区间
////inline int ns_rand_range(int min, int max)
////{
////    if (min > max) SWAP(min, max);
////    return  min == max ? min : (ns_rand_s32() % (max - min + 1) + min);
////}
////
////// 从数组中选出不重复的N个元素
////template <typename T>
////int ns_rand_array(T *arr, int arr_num, T *rand_arr, int &rand_arr_num)
////{
////    if (rand_arr_num >= arr_num)
////    {
////        rand_arr_num = arr_num;
////        memcpy(rand_arr, arr, sizeof(T) * arr_num);
////    }
////    else if (rand_arr_num == 1)
////    {
////        int idx = ns_rand_range(0, arr_num - 1);
////        *rand_arr = arr[idx];
////    }
////    else
////    {
////        T tmp_arr[10];
////        int tmp_arr_num = arr_num;
////        memcpy(tmp_arr, arr, sizeof(T) * arr_num);
////
////        for (int i = 0; i < rand_arr_num; ++i)
////        {
////            int idx = ns_rand_range(0, tmp_arr_num - 1);
////            rand_arr[i] = tmp_arr[idx];
////            tmp_arr[idx] = tmp_arr[tmp_arr_num - 1];
////            --tmp_arr_num;
////        }
////    }
////
////    return 0;
////}
////
////void gen_single_array(_In_ int src_array[][10], _In_ int index, _Out_ int* target_array, _Out_ int& target_length)
////{
////    int tmp_array[10] = { 0 };
////    for (int i = 0; i < 10; ++i)
////    {
////        int max = 10 - index - 1;
////        int rand = ns_rand_range(0, max);
////        tmp_array[i] = src_array[i][rand];
////        target_length++;
////        memmove(&src_array[i][rand], &src_array[i][rand + 1], sizeof(int) * (10 - rand - 1));
////        src_array[i][max] = 0;
////    }
////
////    //打乱
////    for (int i = 0; i < target_length; ++i)
////    {
////        int max = target_length - i - 1;
////        int rand = ns_rand_range(0, max);
////        target_array[i] = tmp_array[rand];
////        memmove(&tmp_array[rand], &tmp_array[rand + 1], sizeof(int) * (target_length - rand - 1));
////        tmp_array[max] = 0;
////    }
////}
////
////void gen_rand_array(int* out_array, int& out_len)
////{
////    int total_array[10][10] = { 0 };
////    for (int i = 0; i < 10; ++i)
////    {
////        for (int j = 0; j < 10; ++j)
////        {
////            total_array[i][j] = i * 10 + j;
////        }
////    }
////
////    for (int i = 0; i < 10; ++i)
////    {
////        int single_array[10] = { 0 };
////        int single_length = 0;
////        gen_single_array(total_array, i, single_array, single_length);
////        assert(single_length == 10);
////
////        for (int j = 0; j < single_length; ++j)
////        {
////            out_array[i * 10 + j] = single_array[j];
////            out_len++;
////        }
////
////        //test
////        int a = 0;
////    }
////}
//
//void quick_sort(int arr[], int left, int right)
//{
//    if (left > right)
//    {
//        return;
//    }
//
//    int i = left;
//    int j = right;
//    int x = arr[left];
//
//    while (i < j)
//    {
//        while (i < j && arr[j] >= x)
//        {
//            j--;
//        }
//
//        if (i < j)
//        {
//            arr[i++] = arr[j];
//        }
//
//        while (i < j && arr[i] < x)
//        {
//            i++;
//        }
//
//        if (i < j)
//        {
//            arr[j--] = arr[i];
//        }
//    }
//
//    arr[i] = x;
//    quick_sort(arr, left, i - 1);
//    quick_sort(arr, i + 1, right);
//}
//
//static uint32_t GetNearest2N(uint32_t size)
//{
//    if (size == 0)
//    {
//        return 0;
//    }
//
//    if ((size & (size - 1)) == 0)
//    {
//        //power(2, n)
//        return size;
//    }
//
//    int count = 0;
//    while (size)
//    {
//        size = size >> 1;
//        ++count;
//    }
//
//    return 1 << count;
//}
//
//int main()
//{
//    int result = GetNearest2N(3);
//
//    //int my_array[100] = { 0 };
//    //int my_len = 0;
//    //gen_rand_array(my_array, my_len);
//    //assert(my_len == 100);
//
//    //for (int i = 0; i < my_len; ++i)
//    //{
//    //    if (i % 10 == 0)
//    //    {
//    //        std::cout << std::endl;
//    //    }
//    //    std::cout << my_array[i] << " ";
//    //}
//    //
//    //std::cout << "\n\n\n" << std::endl;
//
//    //memset(my_array, 0x0, sizeof(my_array));
//    //my_len = 0;
//
//    //gen_rand_array(my_array, my_len);
//    //assert(my_len == 100);
//
//    //for (int i = 0; i < my_len; ++i)
//    //{
//    //    if (i % 10 == 0)
//    //    {
//    //        std::cout << std::endl;
//    //    }
//    //    std::cout << my_array[i] << " ";
//    //}
//    int my_array[] = { 34,65,12,43,67,5,78,10,3,70 };
//    int len = sizeof(my_array) / sizeof(int);
//    quick_sort(my_array, 0, len - 1);
//
//
//    std::system("pause");
//	//std::chrono::time_point<std::chrono::system_clock> now_time = std::chrono::system_clock::now();
//
//    return 0;
//}
//
