/*
Do not modify this file.
Make all of your changes to main.cpp instead.
*/

#include "program.h"

#include <iostream>

using std::cout;
using std::endl;

static int compare_bytes(const void *pa, const void *pb)
{
    int a = *(char *)pa;
    int b = *(char *)pb;

    if (a < b)
    {
        return -1;
    }
    else if (a == b)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

void focus_program(char *data, int length)
{
    int total = 0;
    int i, j;
    char *data_verify = new char[length];
    int total_verify = 0;

    srand(38290);

    for (i = 0; i < length; i++)
    {
        data[i] = 0;
        data_verify[i] = 0;
    }

    for (j = 0; j < 100; j++)
    {
        int start = rand() % length;
        int size = 25;
        for (i = 0; i < 100; i++)
        {
            int index = (start + rand() % size) % length;
            char value = rand();
            data[index] = value;
            data_verify[index] = value;
        }
    }

    for (i = 0; i < length; i++)
    {
        total += data[i];
        total_verify += data_verify[i];
    }

    delete[] data_verify;

    if (total == total_verify)
    {
        cout << "Focus Successful: Result = " << total << endl;
    }
    else
    {
        cout << "Focus Failed: Result = " << total << ", Expected = " << total_verify << endl;
    }
}

void sort_program(char *data, int length)
{
    int total = 0;
    int i;
    char *data_verify = new char[length];
    int total_verify = 0;

    srand(4856);

    for (i = 0; i < length; i++)
    {
        char value = rand();
        data[i] = value;
        data_verify[i] = value;
    }

    qsort(data, length, 1, compare_bytes);
    qsort(data_verify, length, 1, compare_bytes);

    for (i = 0; i < length; i++)
    {
        total += data[i];
        total_verify += data_verify[i];
    }

    delete[] data_verify;

    if (total == total_verify)
    {
        cout << "Sort Successful: Result = " << total << endl;
    }
    else
    {
        cout << "Sort Failed: Result = " << total << ", Expected = " << total_verify << endl;
    }
}

void scan_program(char *cdata, int length)
{
    unsigned i, j;
    unsigned char *data = (unsigned char *)cdata;
    unsigned total = 0;

    for (i = 0; i < (unsigned)length; i++)
    {
        data[i] = i % 256;
    }

    unsigned total_verify = 0;
    for (j = 0; j < 10; j++)
    {
        for (i = 0; i < (unsigned)length; i++)
        {
            total += data[i];
            total_verify += i % 256;
        }
    }

    if (total == total_verify)
    {
        cout << "Scan Successful: Result = " << total << endl;
    }
    else
    {
        cout << "Scan Failed: Result = " << total << ", Expected = " << total_verify << endl;
    }
}
