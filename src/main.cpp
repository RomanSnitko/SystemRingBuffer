#include <iostream>
#include <vector>
#include <cassert>
#include <string>
#include <array>
#include "../include/RingBuffer.hpp"

void test_simple_write_read() 
{
    RingBuffer<int> rb(1024);
    std::vector<int> input = {1, 2, 3, 4, 5};
    rb.write(input);
    
    assert(rb.size() == 5);
    
    int out[5];
    size_t n = rb.read(out);
    
    assert(n == 5);
    for(int i = 0; i < 5; ++i) 
    {
        assert(out[i] == input[i]);
    }

    assert(rb.empty());
}

void test_wrap_around() 
{
    RingBuffer<char> rb(4096);
    size_t cap = rb.capacity();
    
    std::vector<char> padding(cap - 10, 'a');
    rb.write(padding);
    
    std::vector<char> overlap = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 'X', 'Y'};
    rb.write(overlap);
    
    assert(rb.size() == cap);
    
    std::vector<char> out(cap);
    rb.read(out);
    
    assert(out[cap - 10] == '1');
    assert(out[cap - 1] == '0');
}

void test_overwrite_logic() 
{
    RingBuffer<int> rb(4096);
    size_t cap = rb.capacity();
    
    std::vector<int> first(cap, 1);
    rb.write(first);
    
    std::vector<int> second = {9, 9, 9};
    rb.write(second);
    
    assert(rb.size() == cap);
    
    std::vector<int> out(cap);
    rb.read(out);
    
    assert(out[cap - 3] == 9);
    assert(out[0] == 1);
}

void test_empty_read() 
{
    RingBuffer<double> rb(1024);
    double out[10];
    size_t n = rb.read(out);
    assert(n == 0);
    assert(rb.empty());
}

void test_clear() 
{
    RingBuffer<int> rb(1024);
    std::vector<int> data = {1, 2, 3};
    rb.write(data);
    rb.clear();
    assert(rb.size() == 0);
    assert(rb.empty());
}

void test_large_input_overwrite() 
{
    RingBuffer<int> rb(4096);
    size_t cap = rb.capacity();
    
    std::vector<int> massive(cap * 2, 7);
    massive[massive.size() - 1] = 8;
    
    rb.write(massive);
    
    assert(rb.size() == cap);
    
    std::vector<int> out(cap);
    rb.read(out);
    assert(out[cap - 1] == 8);
}

void test_partial_reads() 
{
    RingBuffer<int> rb(1024);
    std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    rb.write(data);
    
    int out1[3];
    int out2[7];
    
    assert(rb.read(out1) == 3);
    assert(out1[0] == 1);
    assert(rb.size() == 7);
    
    assert(rb.read(out2) == 7);
    assert(out2[0] == 4);
    assert(rb.empty());
}

void test_float_type() 
{
    RingBuffer<float> rb(1024);
    std::vector<float> data = {1.1f, 2.2f, 3.3f};
    rb.write(data);
    
    float out[3];
    rb.read(out);
    assert(out[0] == 1.1f);
    assert(out[2] == 3.3f);
}

int main() 
{
    try 
    {
        test_simple_write_read();
        test_wrap_around();
        test_overwrite_logic();
        test_empty_read();
        test_clear();
        test_large_input_overwrite();
        test_partial_reads();
        test_float_type();
        
        std::cout << "all tests passed successfully, so easy brrrbrbrbr" << std::endl;
    } catch (const std::exception& e) 
    {
        std::cerr << "test failed :( with exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}