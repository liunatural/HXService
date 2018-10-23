#pragma once
#include <string>
using namespace std;

class TestRec
{
public:
	TestRec();
	~TestRec();

	TestRec(const TestRec& other);

	TestRec& operator= (const TestRec& other);




	void clear();

public:

	int id;
	string name;

};

